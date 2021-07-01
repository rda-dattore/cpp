#include <metadata.hpp>
#include <strutils.hpp>
#include <utils.hpp>

using metautils::log_error2;
using std::regex;
using std::regex_search;
using std::stof;
using std::stoi;
using std::string;
using strutils::to_lower;
using strutils::trim;

namespace metautils {

namespace CF {

void fill_nc_time_data(string units_attribute_value, NcTime::TimeData&
    time_data, string user) {
  static const string FUNC = miscutils::this_function_label(__func__);
  if (regex_search(units_attribute_value, regex("since"))) {
    auto idx = units_attribute_value.find("since");
    time_data.units = to_lower(units_attribute_value.substr(0, idx));
    trim(time_data.units);
    units_attribute_value = units_attribute_value.substr(idx + 5);
    while (!units_attribute_value.empty() && (units_attribute_value[0] < '0' ||
        units_attribute_value[0] > '9')) {
      units_attribute_value = units_attribute_value.substr(1);
    }
    auto n = units_attribute_value.length() - 1;
    while (n > 0 && (units_attribute_value[n] < '0' || units_attribute_value[n]
        > '9')) {
      --n;
    }
    ++n;
    if (n < units_attribute_value.length()) {
        units_attribute_value = units_attribute_value.substr(0, n);
    }
    trim(units_attribute_value);
    auto sp = strutils::split(units_attribute_value);
    if (sp.size() < 1 || sp.size() > 3) {
      log_error2("unable to get reference time from units specified as: '" +
          units_attribute_value + "'", FUNC, "nc2xml", user);
    }
    auto dsp = strutils::split(sp[0], "-");
    if (dsp.size() != 3) {
        log_error2("unable to get reference time from units specified as: '" +
            units_attribute_value + "'", FUNC, "nc2xml", user);
    }
    time_data.reference.set_year(stoi(dsp[0]));
    time_data.reference.set_month(stoi(dsp[1]));
    time_data.reference.set_day(stoi(dsp[2]));
    if (sp.size() > 1) {
      auto tsp = strutils::split(sp[1], ":");
      switch (tsp.size()) {
        case 1: {
          time_data.reference.set_time(stoi(tsp[0]) * 10000);
          break;
        }
        case 2: {
          time_data.reference.set_time(stoi(tsp[0]) * 10000 + stoi(tsp[1]) *
              100);
          break;
        }
        case 3: {
          time_data.reference.set_time(stoi(tsp[0]) * 10000 + stoi(tsp[1]) * 100
              + static_cast<int>(stof(tsp[2])));
          break;
        }
      }
    }
  } else {
    log_error2("unable to get CF time from time variable units", FUNC, "nc2xml",
        user);
  }
}

} // end namespace CF

} // end namespace metautils
