#include <sys/stat.h>
#include <metadata.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <PostgreSQL.hpp>

using namespace PostgreSQL;
using metautils::log_error2;
using miscutils::this_function_label;
using std::cerr;
using std::endl;
using std::make_tuple;
using std::regex;
using std::regex_search;
using std::stoll;
using std::string;
using std::stringstream;
using std::vector;
using strutils::ds_aliases;
using strutils::replace_all;
using strutils::strand;
using strutils::substitute;
using strutils::to_sql_tuple_string;
using unixutils::mysystem2;

namespace metautils {

vector<CMD_DATABASE> cmd_databases(string caller, string user) {
  static const string F = this_function_label(__func__);
  vector<CMD_DATABASE> databases;
  Server server(directives.database_server, directives.metadb_username,
      directives.metadb_password, "rdadb");
  if (!server) {
    log_error2("could not connect to the metadata server", F, caller, user);
  }
  LocalQuery databases_query("nspname", "pg_catalog.pg_namespace", "nspname "
      "like '%ML'");
  if (databases_query.submit(server) < 0) {
    log_error2("'" + databases_query.error() + "'", F, caller, user);
  }
  for (const auto& databases_row : databases_query) {
    string data_type;
    LocalQuery data_type_query("select data_type from metautil.cmd_databases "
        "where name = '" + databases_row[0] + "'");
    if (data_type_query.submit(server) == 0 && data_type_query.num_rows() > 0) {
      Row data_type_row;
      data_type_query.fetch_row(data_type_row);
      data_type = data_type_row[0];
    }
    databases.emplace_back(make_tuple(databases_row[0], data_type));
  }
  server.disconnect();
  return databases;
}

void check_for_existing_cmd(string cmd_type, string caller, string user) {
  static const string F = this_function_label(__func__);
  Server server(directives.database_server, directives.metadb_username,
      directives.metadb_password, "rdadb");
  if (!server) {
    log_error2("unable to connect to the metadata server", F, caller, user);
  }
  LocalQuery query;
  string cmd_dir;
  string path = args.path;
  replace_all(path, "https://rda.ucar.edu" + web_home() + "/", "");
  query.set("date_modified, time_modified", "dssdb.wfile_" + args.dsid, "wfile "
      "= '" + path + "/" + args.filename + "'");
  cmd_dir = "wfmd";
  if (query.submit(server) < 0) {
    log_error2("error for query: '" + query.show() + "': '" + query.error() +
        "'", F, caller, user);
  }
  Row row;
  if (query.fetch_row(row)) {
    string cmd_file = path + "/" + args.filename;
    replace_all(cmd_file, "/", "%");
    if (unixutils::exists_on_server("gdex.ucar.edu", "/data/web/datasets/" +
        args.dsid + "/metadata/" + cmd_dir + "/" + cmd_file + "." + cmd_type)) {
      DateTime archive_date(stoll(substitute(row[0], "-", "")) * 1000000 +
          stoll(substitute(row[1], ":", "")));
      auto tm = time(nullptr);
      auto t = localtime(&tm);
      if (t->tm_isdst > 0) {
        archive_date.add_hours(6);
      } else {
        archive_date.add_hours(7);
      }
      struct stat buf;
      auto metadata_date = DateTime(1970, 1, 1, 0, 0).seconds_added(buf.
          st_mtime);
      if (metadata_date >= archive_date) {
        query.set("select id from W" + cmd_type + "." + args.dsid + "_webfiles "
            "where id = '" + path + "/" + args.filename + "' and start_date != "
            "0");
        if (query.submit(server) == 0 && query.num_rows() == 0) {
          string flag;
          if (cmd_dir == "wfmd") {
            flag = "-wf";
          }
          stringstream oss, ess;
          mysystem2(directives.local_root + "/bin/scm -d " + args.dsid + " " +
              flag + " " + cmd_file + "." + cmd_type, oss, ess);
        } else {
          cerr << "Terminating: the file metadata for '" << cmd_file << "' are "
              "already up-to-date" << endl;
        }
        exit(1);
      }
    }
  }
  server.disconnect();
}

void cmd_register(string cmd, string user) {
  static const string F = this_function_label(__func__);
  Server server_d(directives.database_server, directives.rdadb_username,
      directives.rdadb_password, "rdadb");
  if (!server_d) {
    log_error2("could not connect to the metadata server", F, cmd, user);
  }
  string file = args.path + "/" + args.filename;
  LocalQuery query("server", "dssdb.cmd_reg", "cmd = '" + cmd + "' and file = '"
      + file + "'");
  if (query.submit(server_d) < 0) {
    log_error2(query.error(), F, cmd, user);
  }
  if (query.num_rows() > 0) {
    server_d._delete("dssdb.cmd_reg", "timestamp < current_timestamp-interval "
        "'1 hour'");
    if (query.submit(server_d) < 0) {
      log_error2(query.error(), F, cmd, user);
    }
    if (query.num_rows() > 0) {
      Row row;
      query.fetch_row(row);
      cerr << "Terminating:  there is a process currently running on '" << row[
          0] << "' to extract content metadata for '" << args.path << "/" <<
          args.filename << "'" << endl;
      exit(1);
    }
  }
  args.reg_key = strand(15);
  if (server_d.insert(
        "dssdb.cmd_reg",
        "regkey, server, cmd, file, timestamp",
        "'" + args.reg_key + "', '" + directives.host + "', '" + cmd + "', '" +
            file + "', '" + dateutils::current_date_time().to_string(
            "%Y-%m-%d %H:%MM:%SS") + "'",
        ""
        ) < 0) {
    log_error2("error while registering: " + server_d.error(), F, cmd, user);
  }
  atexit(cmd_unregister);
  server_d.disconnect();
}

extern "C" void cmd_unregister() {
  Server server_d(directives.database_server, directives.rdadb_username,
      directives.rdadb_password, "rdadb");
  if (!args.reg_key.empty()) {
    server_d._delete("dssdb.cmd_reg", "regkey = '" + args.reg_key + "'");
  }
  server_d.disconnect();
}

} // end namespace metautils
