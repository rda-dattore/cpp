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

void fill_nc_time_data(string time_units, NcTime::TimeData& time_data, string
    user) {
  static const string F = miscutils::this_function_label(__func__);
  if (!regex_search(time_units, regex("since"))) {
    log_error2("unable to get CF time from time variable units", F, "nc2xml",
        user);
  }
  auto idx = time_units.find("since");
  time_data.units = to_lower(time_units.substr(0, idx));
  trim(time_data.units);
  time_units = time_units.substr(idx + 5);
  while (!time_units.empty() && (time_units[0] < '0' || time_units[0] > '9')) {
    time_units = time_units.substr(1);
  }
  auto n = time_units.length() - 1;
  while (n > 0 && (time_units[n] < '0' || time_units[n] > '9')) {
    --n;
  }
  ++n;
  if (n < time_units.length()) {
    time_units = time_units.substr(0, n);
  }
  trim(time_units);
  auto sp = strutils::split(time_units);
  if (sp.size() < 1 || sp.size() > 3) {
    log_error2("unable to get reference time from units specified as: '" +
        time_units + "'", F, "nc2xml", user);
  }
  auto dsp = strutils::split(sp[0], "-");
  if (dsp.size() != 3) {
    log_error2("unable to get reference time from units specified as: '" +
        time_units + "'", F, "nc2xml", user);
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
        time_data.reference.set_time(stoi(tsp[0]) * 10000 + stoi(tsp[1]) * 100);
        break;
      }
      case 3: {
        time_data.reference.set_time(stoi(tsp[0]) * 10000 + stoi(tsp[1]) * 100 +
            static_cast<int>(stof(tsp[2])));
        break;
      }
    }
  }
}

} // end namespace CF

} // end namespace metautils
