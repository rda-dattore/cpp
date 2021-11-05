#include <sys/stat.h>
#include <metadata.hpp>
#include <strutils.hpp>
#include <utils.hpp>

namespace metautils {

std::vector<CMD_DATABASE> cmd_databases(std::string caller,std::string user)
{
  std::vector<CMD_DATABASE> databases;
  MySQL::Server server(directives.database_server,directives.metadb_username,directives.metadb_password,"");
  if (!server) {
    log_error("cmd_databases() could not connect to the metadata server",caller,user);
  }
  MySQL::LocalQuery databases_query("show databases where `Database` like '%ML'");
  if (databases_query.submit(server) < 0) {
    log_error("cmd_databases(): '"+databases_query.error()+"'",caller,user);
  }
  for (const auto& databases_row : databases_query) {
    std::string data_type;
    MySQL::LocalQuery data_type_query("select data_type from metautil.cmd_databases where name = '"+databases_row[0]+"'");
    if (data_type_query.submit(server) == 0 && data_type_query.num_rows() > 0) {
	MySQL::Row data_type_row;
	data_type_query.fetch_row(data_type_row);
	data_type=data_type_row[0];
    }
    databases.emplace_back(std::make_tuple(databases_row[0],data_type));
  }
  server.disconnect();
  return databases;
}

void check_for_existing_cmd(std::string cmd_type)
{
  MySQL::Server server(directives.database_server,directives.metadb_username,directives.metadb_password,"");
  if (!server) {
    std::cerr << "unable to connect to the metadata server" << std::endl;
    exit(1);
  }
  MySQL::LocalQuery query;
  std::string cmd_dir;
  std::string path=args.path;
  if (std::regex_search(path,std::regex("^/FS/DSS")) || std::regex_search(path,std::regex("^/DSS"))) {
    query.set("date_modified,time_modified","dssdb.mssfile","dsid = 'ds"+args.dsnum+"' and mssfile = '"+path+"/"+args.filename+"'");
    cmd_dir="fmd";
  }
  else {
    strutils::replace_all(path,"https://rda.ucar.edu"+web_home()+"/","");
    query.set("date_modified,time_modified","dssdb.wfile","dsid = 'ds"+args.dsnum+"' and wfile = '"+path+"/"+args.filename+"'");
    cmd_dir="wfmd";
  }
  if (query.submit(server) < 0) {
    std::cerr << query.error() << std::endl;
    exit(1);
  }
  MySQL::Row row;
  if (query.fetch_row(row)) {
    std::string cmd_file=path+"/"+args.filename;
    strutils::replace_all(cmd_file,"/FS/DSS/","");
    strutils::replace_all(cmd_file,"/DSS/","");
    strutils::replace_all(cmd_file,"/","%");
    if (unixutils::exists_on_server("rda-web-prod.ucar.edu","/data/web/datasets/ds"+args.dsnum+"/metadata/"+cmd_dir+"/"+cmd_file+"."+cmd_type,directives.rdadata_home)) {
	DateTime archive_date(std::stoll(strutils::substitute(row[0],"-",""))*1000000+std::stoll(strutils::substitute(row[1],":","")));
	auto tm=time(nullptr);
	auto t=localtime(&tm);
	if (t->tm_isdst > 0) {
	  archive_date.add_hours(6);
	}
	else {
	  archive_date.add_hours(7);
	}
	struct stat buf;
	auto metadata_date=DateTime(1970,1,1,0,0).seconds_added(buf.st_mtime);
	if (metadata_date >= archive_date) {
	  if (std::regex_search(path,std::regex("^/FS/DSS")) || std::regex_search(path,std::regex("^/DSS"))) {
	    query.set("select mssID from "+cmd_type+".ds"+strutils::substitute(args.dsnum,".","")+"_primaries where mssID = '"+path+"/"+args.filename+"' and start_date != 0");
	  }
	  else {
	    query.set("select webID from W"+cmd_type+".ds"+strutils::substitute(args.dsnum,".","")+"_webfiles where webID = '"+path+"/"+args.filename+"' and start_date != 0");
	  }
	  if (query.submit(server) == 0 && query.num_rows() == 0) {
	    std::string flag;
	    if (cmd_dir == "fmd") {
		flag="-f";
	    }
	    else if (cmd_dir == "wfmd") {
		flag="-wf";
	    }
	    std::stringstream output,error;
	    unixutils::mysystem2(directives.local_root+"/bin/scm -d "+args.dsnum+" "+flag+" "+cmd_file+"."+cmd_type,output,error);
	  }
	  else {
	    std::cerr << "Terminating: the file metadata for '" << cmd_file << "' are already up-to-date" << std::endl;
	  }
	  exit(1);
	}
    }
  }
  server.disconnect();
}

void cmd_register(std::string cmd,std::string user)
{
  MySQL::Server server_d(directives.database_server,directives.rdadb_username,directives.rdadb_password,"dssdb");
  if (!server_d) {
    log_error("cmd_register() could not connect to the metadata server",cmd,user);
  }
  std::string file=args.path+"/"+args.filename;
  if (!args.member_name.empty()) {
    file+="..m.."+args.member_name;
  }
  MySQL::LocalQuery query("server","cmd_reg","cmd = '"+cmd+"' and file = '"+file+"'");
  if (query.submit(server_d) < 0) {
    log_error("cmd_register(): "+query.error(),cmd,user);
  }
  if (query.num_rows() > 0) {
    server_d._delete("cmd_reg","timestamp < date_sub(now(),interval 1 hour)");
    if (query.submit(server_d) < 0) {
	log_error("cmd_register(): "+query.error(),cmd,user);
    }
    if (query.num_rows() > 0) {
	MySQL::Row row;
	query.fetch_row(row);
	std::cerr << "Terminating:  there is a process currently running on '" << row[0] << "' to extract content metadata for '" << args.path << "/" << args.filename << "'" << std::endl;
	exit(1);
    }
  }
  args.reg_key=strutils::strand(15);
  if (server_d.insert("cmd_reg","regkey,server,cmd,file,timestamp","'"+args.reg_key+"','"+directives.host+"','"+cmd+"','"+file+"','"+dateutils::current_date_time().to_string("%Y-%m-%d %H:%MM:%SS")+"'","") < 0) {
    log_error("error while registering: "+server_d.error(),cmd,user);
  }
  atexit(cmd_unregister);
  server_d.disconnect();
}

extern "C" void cmd_unregister()
{
  MySQL::Server server_d(directives.database_server,directives.rdadb_username,directives.rdadb_password,"dssdb");
  if (!args.reg_key.empty()) {
    server_d._delete("cmd_reg","regkey = '"+args.reg_key+"'");
  }
  server_d.disconnect();
}

} // end namespace metautils
