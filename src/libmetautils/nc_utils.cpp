#include <metadata.hpp>
#include <strutils.hpp>
#include <utils.hpp>

using std::endl;
using std::regex;
using std::regex_replace;
using std::regex_search;
using std::stod;
using std::stoll;
using std::string;
using std::stringstream;
using std::unique_ptr;
using std::unordered_set;
using std::vector;
using strutils::chop;
using strutils::itos;
using strutils::replace_all;
using strutils::split;
using strutils::to_lower;
using strutils::trim;

namespace metautils {

namespace NcTime {

DateTime actual_date_time(double time, const TimeData& time_data, string&
    error) {
  DateTime dt; // return value
  if (time_data.units == "seconds") {
if (fabs(time - static_cast<long long>(time)) > 0.01) {
error = "can't compute dates from fractional seconds";
}
    if (time >= 0.) {
      dt = time_data.reference.seconds_added(time, time_data.calendar);
    } else {
      dt = time_data.reference.seconds_subtracted(-time, time_data.calendar);
    }
  } else if (time_data.units == "minutes") {
if (fabs(time - static_cast<long long>(time)) > 0.01) {
error = "can't compute dates from fractional minutes";
}
    if (time >= 0.) {
      dt = time_data.reference.minutes_added(time, time_data.calendar);
    } else {
      dt = time_data.reference.minutes_subtracted(-time, time_data.calendar);
    }
  } else if (time_data.units == "hours") {
    if (fabs(time - static_cast<long long>(time)) > 0.01) {
      time *= 60.;
      if (fabs(time - static_cast<long long>(time)) > 0.01) {
        error = "can't compute dates from fractional hours";
      } else {
        if (time >= 0.) {
          dt = time_data.reference.minutes_added(time, time_data.calendar);
        } else {
          dt = time_data.reference.minutes_subtracted(-time,
              time_data.calendar);
        }
      }
    } else {
      if (time >= 0.) {
        dt = time_data.reference.hours_added(time, time_data.calendar);
      } else {
        dt = time_data.reference.hours_subtracted(-time, time_data.calendar);
      }
    }
  } else if (time_data.units == "days") {
    if (time >= 0.) {
      dt = time_data.reference.days_added(time, time_data.calendar);
    } else {
      dt = time_data.reference.days_subtracted(-time, time_data.calendar);
    }
    auto f = fabs(time - static_cast<long long>(time));
    if (f > 0.01) {
      f *= 24.;
      if (fabs(f - lround(f)) > 0.01) {
        f *= 60.;
        if (fabs(f - lround(f)) > 0.01) {
          f *= 60.;
          if (fabs(f - lround(f)) > 0.01) {
            error = "can't compute dates from fractional seconds in fractional "
                "days (" + strutils::ftos(time,10) + ")";
          } else {
            dt.add_seconds(lround(f), time_data.calendar);
          }
        } else {
          dt.add_minutes(lround(f), time_data.calendar);
        }
      } else {
        dt.add_hours(lround(f), time_data.calendar);
      }
    }
  } else if (time_data.units == "months") {
if (fabs(time - static_cast<long long>(time)) > 0.01) {
error = "can't compute dates from fractional months";
}
    if (time >= 0.) {
      dt = time_data.reference.months_added(time);
    } else {
      dt = time_data.reference.months_subtracted(-time);
    }
  } else {
    error = "don't understand time units in " + time_data.units;
  }
  return dt;
}

DateTime reference_date_time(string units_attribute_value) {
  DateTime dt; // return value
  auto idx = units_attribute_value.find("since");
  if (idx == string::npos) {
    return dt;
  }
  units_attribute_value = units_attribute_value.substr(idx + 5);
  replace_all(units_attribute_value, "T", " ");
  trim(units_attribute_value);
  if (regex_search(units_attribute_value, regex("Z$"))) {
    chop(units_attribute_value);
  }
  auto sp = split(units_attribute_value);
  auto sp2 = split(sp[0], "-");
  if (sp2.size() != 3) {
    return dt;
  }
  auto l = stoll(sp2[0]) * 10000000000 + stoll(sp2[1]) * 100000000 + stoll(sp2[
      2]) * 1000000;
  if (sp.size() > 1) {
    auto sp3 = split(sp[1], ":");
    l += stoll(sp3[0]) * 10000;
    if (sp3.size() > 1) {
      l += stoll(sp3[1]) * 100;
    }
    if (sp3.size() > 2) {
      l += static_cast<long long>(stod(sp3[2]));
    }
  }
  dt.set(l);
  return dt;
}

string gridded_netcdf_time_range_description(const TimeRangeEntry& tre, const
    TimeData& time_data, string time_method, string& error) {
  string s; // return value
  if (static_cast<int>(tre.key) < 0) {
    if (static_cast<int>(tre.key) == -11) {
      s = "Climate Model Simulation";
    } else if (!time_method.empty()) {
      auto l = to_lower(time_method);
      if (l.find("monthly") != string::npos || (tre.bounded.
          first_valid_datetime.day() == tre.bounded.last_valid_datetime.day() &&
          tre.bounded.last_valid_datetime.months_since(tre.bounded.
          first_valid_datetime) == tre.num_steps)) {
        s = "Monthly ";
      } else if (time_data.units == "minutes") {
        auto n = tre.bounded.last_valid_datetime.minutes_since(tre.bounded.
            first_valid_datetime) / tre.num_steps;
        if (n == 1440) {
          s = "Daily ";
        } else {
          s = itos(n) + "-minute ";
        }
      } else if (time_data.units == "hours") {
        if (tre.bounded.first_valid_datetime.year() > 0) {
          auto n = tre.instantaneous.first_valid_datetime.seconds_since(tre.
              bounded.first_valid_datetime) * 2 / 3600;
          switch (n) {
            case 1: {
              s = "Hourly ";
            }
            default: {
              s = itos(n) + "-hour ";
            }
          }
        } else {
          s = "Hourly ";
        }
      } else if (time_data.units == "days") {
        if (tre.bounded.first_valid_datetime.year() > 0) {
          auto a = tre.bounded.last_valid_datetime.days_since(tre.bounded.
              first_valid_datetime, time_data.calendar);
          auto n = a / tre.num_steps;
          switch (n) {
            case 1: {
              s = "Daily ";
              break;
            }
            case 5: {
              s = "Pentad ";
              break;
            }
            case 30:
            case 31: {
              if (tre.bounded.last_valid_datetime.years_since(tre.bounded.
                  first_valid_datetime) > 0) {
                s = "Monthly ";
              }
              break;
            }
            case 365: {
              s = "Annual ";
              break;
            }
            default: {
              s = itos(n) + "-day ";
            }
          }
        } else {
          s = "Daily ";
        }
      } else if (time_data.units == "months") {
        s = "Monthly ";
      } else {
        error = "gridded_netcdf_time_range_description(): don't understand "
            "time units '" + time_data.units + "' for cell method '" +
            time_method + "'";
      }
      s += strutils::to_capital(time_method);
    } else {
      if (time_data.units == "months") {
        s = "Monthly Mean";
      } else {
        if (static_cast<int>(tre.key) == -1) {
          s = "Analysis";
        } else {
          s = itos(-static_cast<int>(tre.key) / 100) + "-" + time_data.units;
          chop(s);
          s += " Forecast";
        }
      }
    }
  } else if (!time_method.empty()) {
    auto l = to_lower(time_method);
    if (tre.key == 0x7fffffff) {
      s = "All-year Climatology of ";
    } else {
      s = itos(tre.key) + "-year Climatology of ";
    }
    if (regex_search(l, regex("mean over years$"))) {
      auto idx = l.find(" within");
      if (idx != string::npos) {
        auto c = strutils::capitalize(l.substr(0, idx) + "s");
        switch (tre.unit) {
          case 1: {
            s += "Monthly " + c;
            break;
          }
          case 2: {
            s += "Seasonal " + c;
            break;
          }
          case 3: {
            s += "Annual " + c;
            break;
          }
        }
      } else {
        s += "??";
      }
    } else {
      s += "??";
    }
  }
  return s;
}

string gridded_netcdf_time_range_description2(const TimeRangeEntry2& tre2, const
    TimeData& time_data, string time_method, string& error) {
  TimeRangeEntry tre;
  tre.key = tre2.key;
  tre.unit = tre2.unit;
  tre.num_steps = tre2.num_steps;
  tre.instantaneous = tre2.instantaneous;
  tre.bounded = tre2.bounded;
  return gridded_netcdf_time_range_description(tre, time_data, time_method,
      error);
}

string time_method_from_cell_methods(string cell_methods, string timeid) {
  if (cell_methods.empty()) {
    return "";
  }
  while (cell_methods.find("  ") != string::npos) {
    replace_all(cell_methods, "  ", " ");
  }
  replace_all(cell_methods, "comment: ", "");
  replace_all(cell_methods, "comments: ", "");
  replace_all(cell_methods, "comment:", "");
  replace_all(cell_methods, "comments:", "");
  auto idx = cell_methods.find(timeid + ": ");
  if (idx == string::npos) {
    return "";
  }
  if (idx != 0) {
    cell_methods = cell_methods.substr(idx);
  }
  replace_all(cell_methods, timeid + ": ", "");
  trim(cell_methods);
  idx = cell_methods.find(": ");
  if (idx != string::npos) {
    auto idx2 = cell_methods.find(")");
    if (idx2 == string::npos) {

      // no extra information in parentheses
      cell_methods = cell_methods.substr(0, idx);
      cell_methods = cell_methods.substr(0, cell_methods.rfind(" "));
    } else {

      // found extra information so include that in the methods
//        cell_methods = cell_methods.substr(0, idx2 + 1);
cell_methods = cell_methods.substr(0, idx);
cell_methods = cell_methods.substr(0, cell_methods.rfind(" "));
    }
  }
  static unique_ptr<unordered_set<string>> cmset(nullptr);
  if (cmset == nullptr) {
    cmset.reset(new unordered_set<string>);
    cmset->emplace("point");
    cmset->emplace("sum");
    cmset->emplace("maximum");
    cmset->emplace("median");
    cmset->emplace("mid_range");
    cmset->emplace("minimum");
    cmset->emplace("mean");
    cmset->emplace("mode");
    cmset->emplace("standard_deviation");
    cmset->emplace("variance");
  }
  if (cmset->find(cell_methods) == cmset->end()) {
    cell_methods = "!" + cell_methods;
  }
  return cell_methods;
}

} // end namespace NcTime

namespace NcLevel {

string write_level_map(const vector<LevelInfo>& level_info) {
  string f;
  if (args.data_format == "hdf5nc4") {
    f = "netCDF4";
  } else {
    f = "netCDF";
  }
  string x = unixutils::remote_web_file("https://" + directives.web_server +
      "/metadata/LevelTables/" + f + "." + args.dsid + ".xml",
      directives.temp_path);
  LevelMap lmap;
  vector<string> v;
  if (lmap.fill(x)) {
    std::ifstream ifs(x.c_str());
    char l[32768];
    ifs.getline(l, 32768);
    while (!ifs.eof()) {
      v.emplace_back(l);
      ifs.getline(l, 32768);
    }
    ifs.close();
    v.pop_back();
  }
  TempDir t;
  if (!t.create(directives.temp_path)) {
    return "can't create temporary directory for writing netCDF levels";
  }
  std::ofstream ofs((t.name() + "/" + f + "." + args.dsid + ".xml").c_str());
  if (!ofs.is_open()) {
    return "can't open " + t.name() + "/" + f + "." + args.dsid + ".xml "
        "file for writing netCDF levels";
  }
  if (v.size() > 0) {
    for (auto e : v) {
      ofs << e << endl;
    }
  } else {
    ofs << "<?xml version=\"1.0\" ?>" << endl;
    ofs << "<levelMap>" << endl;
  }
  for (const auto& e : level_info) {
    if (e.write == 1 && (v.size() == 0 || (v.size() > 0 && lmap.is_layer(e.ID) <
        0))) {
      ofs << "  <level code=\"" << e.ID << "\">" << endl;
      ofs << "    <description>" << e.description << "</description>" << endl;
      ofs << "    <units>" << e.units << "</units>" << endl;
      ofs << "  </level>" << endl;
    }
  }
  ofs << "</levelMap>" << endl;
  ofs.close();
  string e;
  if (unixutils::gdex_upload_dir(t.name(), ".",
      "/data/web/metadata/LevelTables", "", e) < 0) {
    return e;
  }
  stringstream oss, ess;
  unixutils::mysystem2("/bin/cp " + t.name() + "/" + f + "." + args.dsid +
      ".xml " + directives.level_map_path + "/", oss, ess);
  return ess.str();
}

} // end namespace NcLevel

namespace NcParameter {

string write_parameter_map(std::list<string>& varlist, unordered_set<string>&
    var_changes_table, string map_type, string map_name, bool found_map, string&
    warning) {
  stringstream ess; // return value
  if (varlist.size() > 0) {
    vector<string> v;
    if (found_map) {
      std::ifstream ifs(map_name.c_str());
      char l[32768];
      ifs.getline(l, 32768);
      while (!ifs.eof()) {
        v.emplace_back(l);
        ifs.getline(l, 32768);
      }
      ifs.close();
      v.pop_back();
    }
    TempDir t;
    if (!t.create(directives.temp_path)) {
      return "can't create temporary directory for parameter map";
    }
    string f;
    if (args.data_format == "hdf5nc4") {
      f="netCDF4";
    } else if (args.data_format == "hdf5") {
      f="HDF5";
    } else {
      f="netCDF";
    }
    std::ofstream ofs((t.name() + "/" + f + "." + args.dsid + ".xml").
        c_str());
    if (!ofs.is_open()) {
      return "can't open parameter map file for output";
    }
    if (!found_map) {
      ofs << "<?xml version=\"1.0\" ?>" << endl;
      ofs << "<" << map_type << "Map>" << endl;
    } else {
      auto b = false;
      for (const auto& e : v) {
        if (e.find(" code=\"") != string::npos) {
          auto sp = split(e, "\"");
          if (var_changes_table.find(sp[1]) != var_changes_table.end()) {
            b = true;
          } else {
            var_changes_table.emplace(sp[1]);
          }
        }
        if (!b) {
          ofs << e << endl;
        }
        if (e.find("</" + map_type + ">") != string::npos) {
          b = false;
        }
      }
    }
    for (const auto& e : varlist) {
      auto sp = split(e, "<!>");
      if (map_type == "parameter") {
        ofs << "  <parameter code=\"" << sp[0] << "\">" << endl;
        ofs << "    <shortName>" << sp[0] << "</shortName>" << endl;
        if (!sp[1].empty()) {
          ofs << "    <description>" << sp[1] << "</description>" << endl;
        }
        if (!sp[2].empty()) {
          sp[2] = regex_replace(sp[2], regex("\\*\\*"), "^");
          sp[2] = regex_replace(sp[2], regex("([a-z]{1,})(-){0,1}([0-9]{1,})"),
              "$1^$2$3");
          ofs << "    <units>" << sp[2] << "</units>" << endl;
        }
        if (sp.size() > 3 && !sp[3].empty()) {
          ofs << "    <standardName>" << sp[3] << "</standardName>" << endl;
        }
        ofs << "  </parameter>" << endl;
      } else if (map_type == "dataType") {
        ofs << "  <dataType code=\"" << sp[0] << "\">" << endl;
        ofs << "    <description>" << sp[1];
        if (!sp[2].empty()) {
          ofs << " (" << sp[2] << ")";
        }
        ofs << "</description>" << endl;
        if (sp.size() > 3 && !sp[3].empty()) {
          ofs << "    <standardName>" << sp[3] << "</standardName>" << endl;
        }
        ofs << "  </dataType>" << endl;
      }
    }
    ofs << "</" << map_type << "Map>" << endl;
    ofs.close();
    string e;
    if (unixutils::gdex_upload_dir(t.name(), ".", "/data/web/metadata/"
        "ParameterTables", "", e) < 0) {
      warning = "parameter map was not synced - error(s): '" + e + "'";
    }
warning = e;
    stringstream oss;
    unixutils::mysystem2("/bin/cp " + t.name() + "/" + f + "." + args.dsid +
        ".xml " + directives.parameter_map_path + "/", oss, ess);
  }
  return ess.str();
}

} // end namespace NcParameter

} // end namespace metautils
