#include <fstream>
#include <sstream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utime.h>
#include <sys/stat.h>
#include <PostgreSQL.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bitmap.hpp>
#include <xmlutils.hpp>
#include <metahelpers.hpp>
#include <myerror.hpp>

using namespace PostgreSQL;
using std::endl;
using std::distance;
using std::map;
using std::stoi;
using std::string;
using std::stringstream;
using std::to_string;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using strutils::append;
using strutils::split;
using strutils::substitute;
using strutils::to_lower;
using unixutils::mysystem2;

namespace webutils {

bool filled_inventory(DBconfig db_config, string dsnum, string data_file,
    string type, stringstream& inventory) {
  Server server(db_config);
  if (!server) {
    myerror = "database connection error - " + server.error();
    return false;
  }
  auto dsnum2 = substitute(dsnum, ".", "");
  LocalQuery query("select w.code, w.format_code, f.format, w.uflag from "
      "\"WGrML\".ds" + dsnum2 + "_webfiles2 as w left join \"WGrML\".formats "
      "as f on f.code = w.format_code where id = '" + data_file + "'");
  Row row;
  if (query.submit(server) < 0 || !query.fetch_row(row)) {
    myerror = "database error - " + query.error();
    return false;
  }
  auto file_code = row[0];
  auto data_format_code = row[1];
  auto data_format = row[2];
  auto uflag = row[3] + "\n";
  auto inv_file = "/usr/local/www/server_root/web/datasets/ds" + dsnum +
      "/metadata/inv/" + data_file + "." + type + "_inv";
  auto versions_match = false;
  struct stat buf;
  if (stat((inv_file + ".vrsn").c_str(), &buf) == 0) {
    stringstream oss, ess;
    mysystem2("/bin/tcsh -c \"cat " + inv_file + ".vrsn\"", oss, ess);
    versions_match = (oss.str() == uflag);
  }
  if (stat(inv_file.c_str(), &buf) != 0 || !versions_match) {
    query.set("distinct process", "WGrML.ds" + dsnum2 + "_processes",
        "file_code = " + file_code);
    if (query.submit(server) < 0) {
      myerror = "database error - " + query.error();
      return false;
    }
    unordered_set<string> processes;
    for (const auto& res : query) {
      processes.emplace(res[0]);
    }
    query.set("select time_range_codes, grid_definition_codes, parameter, "
        "level_type_codes from \"WGrML\".ds" + dsnum2 + "_agrids2 where "
        "file_code = " + file_code);
    if (query.submit(server) < 0) {
      myerror = "database error - " + query.error();
      return false;
    } else if (query.num_rows() == 0) {
std::cerr << "no result returned - " << query.show() << endl;
exit(1);
    }
    unordered_map<size_t, string> time_ranges, grid_definitions, levels;
    unordered_map<string, string> parameters;
    unordered_set<string> t, g, l;
    string inv_query;
    xmlutils::ParameterMapper pmapper("/usr/local/www/server_root/web/metadata/"
        "ParameterTables");
    for (const auto& res : query) {
      vector<size_t> values;
      if (t.find(res[0]) == t.end()) {
        bitmap::uncompress_values(res[0], values);
        for (const auto& code : values) {
          if (time_ranges.find(code) == time_ranges.end()) {
            LocalQuery tr_query("time_range", "WGrML.time_ranges", "code = "
                    + to_string(code));
            Row row;
            string time_range;
            if (tr_query.submit(server) == 0 && tr_query.fetch_row(row)) {
              time_range = row[0];
            }
            time_ranges.emplace(code, time_range);
          }
        }
        t.emplace(res[0]);
      }
      if (g.find(res[1]) == g.end()) {
        bitmap::uncompress_values(res[1], values);
        for (const auto& code : values) {
          if (grid_definitions.find(code) == grid_definitions.end()) {
            LocalQuery gd_query("definition, defParams", "WGrML."
                    "grid_definitions", "code = " + to_string(code));
            Row row;
            string grid_definition;
            if (gd_query.submit(server) == 0 && gd_query.fetch_row(row)) {
              grid_definition = row[0] + "," + substitute(row[1], ":", ",");
            }
            grid_definitions.emplace(code, grid_definition);
          }
        }
        g.emplace(res[1]);
      }
      if (parameters.find(res[2]) == parameters.end()) {
        LocalQuery q("code", "IGrML.parameters", "parameter = '" +
            data_format_code + "!" + res[2] + "'");
        Row r;
        if (q.submit(server) < 0 || !q.fetch_row(r)) {
          myerror = "database error getting parameter code";
          return false;
        }
        append(inv_query, "select byte_offset, byte_length, valid_date, "
              "time_range_code, grid_definition_code, level_code, '" + res[2] +
              "', process from \"IGrML\".ds" + dsnum2 + "_inventory_" +
              r[0] + " where file_code = " + file_code, " union all ");
        parameters.emplace(res[2], pmapper.description(data_format, res[2]));
      }
      if (l.find(res[3]) == l.end()) {
        bitmap::uncompress_values(res[3], values);
        for (const auto& code : values) {
          if (levels.find(code) == levels.end()) {
            LocalQuery l_query("map, type, value", "WGrML.levels", "code = "
                    + to_string(code));
            Row row;
            string level;
            if (l_query.submit(server) == 0 && l_query.fetch_row(row)) {
              level = row[0] + "," + row[1] + ":" + row[2];
            }
            levels.emplace(code, level);
          }
        }
        l.emplace(res[3]);
      }
    }
    auto time_range_compare =
        [](const string& left, const string& right) -> bool {
          return metacompares::compare_time_ranges(left, right);
        };
    map<string, size_t, decltype(time_range_compare)> sorted_time_ranges(
        time_range_compare);
    for (const auto& e : time_ranges) {
      sorted_time_ranges.emplace(e.second, e.first);
    }
if (sorted_time_ranges.size() != time_ranges.size()) {
std::cerr << "***inventory time ranges: " << sorted_time_ranges.size() << " " << time_ranges.size() << endl;
}
    auto level_compare =
        [](const string& left, const string& right) -> bool
        {
          return metacompares::compare_levels(left, right);
        };
    map<string, size_t, decltype(level_compare)> sorted_levels(level_compare);
    xmlutils::LevelMapper lmapper("/usr/local/www/server_root/web/metadata/"
        "LevelTables");
    for (auto& e : levels) {
      auto parts = split(e.second, ":");
      auto l_parts = split(parts.front(), ",");
      vector<string> level_units;
      auto level_values = split(parts.back(), ",");
      if (l_parts.front() != l_parts.back()) {
        e.second=lmapper.description(data_format, l_parts.back(), l_parts.
            front());
        auto level_codes = split(l_parts.back(), "-");
        for (const auto& level_code : level_codes) {
          level_units.emplace_back(lmapper.units(data_format, level_code,
              l_parts.front()));
        }
      } else {
        e.second=lmapper.description(data_format, l_parts.front());
        auto level_codes = split(l_parts.front(), "-");
        for (const auto& level_code : level_codes) {
          level_units.emplace_back(lmapper.units(data_format, level_code));
        }
      }
      string level;
      if (level_values.size() > 1 || level_values[0] != "0" || !level_units[0].
          empty()) {
        level = level_values.front();
        if (!level_units.front().empty()) {
          level += " " + level_units.front();
        }
        if (level_values.size() > 1) {
          level += ", " + level_values.back();
          if (!level_units.back().empty()) {
            level += " " + level_units.back();
          }
        }
      }
      if (!level.empty()) {
        e.second += ": " + level;
      }
      sorted_levels.emplace(e.second, e.first);
    }
if (sorted_levels.size() != levels.size()) {
std::cerr << "***inventory levels: " << sorted_levels.size() << " " << levels.size() << endl;
}
    auto parameter_compare =
        [](const string& left, const string& right) -> bool
        {
          return (to_lower(left) < to_lower(right));
        };
    map<string, string, decltype(parameter_compare)> sorted_parameters(
        parameter_compare);
    for (const auto& e : parameters) {
      sorted_parameters.emplace(e.second, e.first);
    }
if (sorted_parameters.size() != parameters.size()) {
std::cerr << "***inventory parameters: " << sorted_parameters.size() << " " << parameters.size() << endl;
}
    stringstream oss, ess;
    if (mysystem2("/bin/mkdir -p /usr/local/www/server_root/web/datasets/ds" +
        dsnum + "/metadata/inv/" + data_file.substr(0, data_file.rfind("/")),
        oss, ess) < 0) {
      myerror = "directory error - " + ess.str();
      return false;
    }
    std::ofstream ofs(inv_file.c_str());
    if (!ofs.is_open()) {
      myerror = "unable to create inventory";
      return false;
    }
    for (const auto& e : sorted_time_ranges) {
      ofs << "U<!>" << distance(sorted_time_ranges.begin(), sorted_time_ranges.
          find(e.first)) << "<!>" << e.second << "<!>" << e.first << endl;
    }
    for (const auto& e : grid_definitions) {
      ofs << "G<!>" << distance(grid_definitions.begin(), grid_definitions.find(
          e.first)) << "<!>" << e.first << "<!>" << e.second << endl;
    }
    for (const auto& e : sorted_levels) {
      ofs << "L<!>" << distance(sorted_levels.begin(), sorted_levels.find(e.
          first)) << "<!>" << e.second << "<!>" << e.first << endl;
    }
    for (const auto& e : sorted_parameters) {
      ofs << "P<!>" << distance(sorted_parameters.begin(), sorted_parameters.
          find(e.first)) << "<!>" << data_format_code << "!" << e.second <<
          "<!>" << e.first << endl;
    }
    for (const auto& p : processes) {
      ofs << "R<!>" << distance(processes.begin(), processes.find(p)) << "<!>"
          << p << endl;
    }
    ofs << "-----" << endl;
    query.set(inv_query + " order by byte_offset");
    if (query.submit(server) == 0) {
      for (const auto& res : query) {
        ofs << res[0] << "|" << res[1] << "|" << res[2] << "|" << distance(
            sorted_time_ranges.begin(), sorted_time_ranges.find(time_ranges[
            stoi(res[3])])) << "|" << distance(grid_definitions.begin(),
            grid_definitions.find(stoi(res[4]))) << "|" << distance(
            sorted_levels.begin(), sorted_levels.find(levels[stoi(res[5])])) <<
            "|" << distance(sorted_parameters.begin(), sorted_parameters.find(
            parameters[res[6]])) << "|" << distance(processes.begin(),
            processes.find(res[7])) << endl;
      }
    }
    ofs.close();
    ofs.open((inv_file + ".vrsn").c_str());
    ofs << uflag;
    ofs.close();
    server.disconnect();
  } else {
    utime(inv_file.c_str(), nullptr);
  }
  stringstream error;
  if (mysystem2("/bin/tcsh -c \"cat " + inv_file + "\"", inventory, error) <
      0) {
    myerror = error.str();
    return false;
  }
  return true;
}

} // end namespace webutils
