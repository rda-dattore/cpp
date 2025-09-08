#include <metadata.hpp>
#include <strutils.hpp>
#include <datetime.hpp>

using std::regex;
using std::regex_search;
using std::stoi;
using std::string;
using strutils::contains;
using strutils::replace_all;
using strutils::substitute;
using strutils::to_upper;
using strutils::trim;

namespace metautils {

void obs_per(string obs_type, size_t num_obs, DateTime start, DateTime end,
    double& obsper, string& unit) {
  unit = "";
  if (contains(obs_type, "climatology")) {
    auto idx = obs_type.find("year") - 1;
    if (idx != string::npos) {
      while (idx > 0 && obs_type[idx] != '_')
        --idx;
    }
    auto len_s = obs_type.substr(idx + 1);
    len_s = len_s.substr(0, len_s.find("-year"));
    auto len = stoi(len_s);
    auto nyrs = end.year() - start.year();
    if (nyrs >= len) {
      int nper;
      if (len == 30) {
        nper = (nyrs % len) / 10 + 1;
      } else {
        nper = nyrs / len;
      }
      obsper = num_obs / static_cast<float>(nper);
      if (static_cast<int>(num_obs) == nper) {
        unit = "year";
      } else if (num_obs / nper == 4) {
        unit = "season";
      } else if (num_obs / nper == 12) {
        unit = "month";
      } else if (nper > 1 && start.month() != end.month()) {
        nper = (nper - 1) * 12 + 12 - start.month() + 1;
        if (static_cast<int>(num_obs) == nper) {
          unit = "month";
        }
      }
    } else {
      if (end.month() - start.month() == static_cast<int>(num_obs)) {
        unit = "month";
      }
    }
    obsper -= len;
  } else {
    obsper = num_obs / static_cast<double>(end.seconds_since(start));
    const float TOLERANCE = 0.15;
    if (obsper + TOLERANCE > 1.) {
      unit = "second";
    } else {
      obsper *= 60.;
      if (obsper + TOLERANCE > 1.) {
        unit = "minute";
      } else {
        obsper *= 60;
        if (obsper + TOLERANCE > 1.) {
          unit = "hour";
        } else {
          obsper *= 24;
          if (obsper + TOLERANCE > 1.) {
            unit = "day";
          } else {
            obsper *= 7;
            if (obsper + TOLERANCE > 1.) {
              unit = "week";
            } else {
              obsper *= dateutils::days_in_month(end.year(), end.month()) / 7.;
              if (obsper + TOLERANCE > 1.) {
                unit = "month";
              } else {
                obsper *= 12;
                if (obsper + TOLERANCE > 1.) {
                  unit = "year";
                }
              }
            }
          }
        }
      }
    }
  }
}

string web_home() {
  return "/glade/collections/rda/data/" + args.dsid;
}

string relative_web_filename(string url) {
  replace_all(url, "https://rda.ucar.edu", "");
  replace_all(url, "http://rda.ucar.edu", "");
  replace_all(url, "http://dss.ucar.edu", "");
  return substitute(url, directives.data_root_alias + "/" + args.dsid + "/",
      "");
}

string relative_web_filename(string url, string dsid) {
  auto d = args.dsid;
  args.dsid = dsid;
  auto s = relative_web_filename(url);
  args.dsid = d;
  return s;
}

string clean_id(string id) {
  trim(id);
  for (size_t n = 0; n < id.length(); ++n) {
    if (static_cast<int>(id[n]) < 32 || static_cast<int>(id[n]) > 127) {
      if (n > 0) {
        id = id.substr(0, n) + "/" + id.substr(n + 1);
      } else {
        id = "/" + id.substr(1);
      }
    }
  }
  replace_all(id, "\"", "'");
  replace_all(id, "&", "&amp;");
  replace_all(id, ">", "&gt;");
  replace_all(id, "<", "&lt;");
  return to_upper(id);
}

} // end namespace metautils
