#include <regex>
#include <strutils.hpp>

using std::regex;
using std::regex_search;
using std::stof;
using std::stoi;
using std::string;
using std::vector;
using strutils::chop;
using strutils::split;
using strutils::to_lower;
using strutils::trim;

namespace metacompares {

bool compare_grid_definitions(const string& left, const string& right) {
  auto lf = left.find("&deg;");
  auto rf = right.find("&deg;");
  if (lf != string::npos && rf != string::npos) {
    auto l = stof(left.substr(0, lf));
    auto r = stof(right.substr(0, rf));
    return l < r;
  }
  if (lf != string::npos) {
    return true;
  }
  if (rf != string::npos) {
    return false;
  }
  return left < right;
}

void standardize_value(string& value) {
  if (value.find(".") == string::npos) {
    value += ".";
  }
  auto idx = value.find(".");
  if (idx < 8) {
    value.insert(0, 8 - idx, '0');
  }
  if (value.length() < 17) {
    value.append(17 - value.length(), '0');
  }
}

bool level_value_compare(string lval, string rval, string lunit, string runit) {
  if ((lunit.find("mbar") != string::npos && runit.find("mbar") != string::npos)
      || (lunit.find("Pa") != string::npos && runit.find("Pa")  != string::npos)
      || (lunit.find("sigma") != string::npos && runit.find("sigma") != string::
      npos)) {
    return lval > rval;
  }
  return lval < rval;
}

bool compare_levels(const string& left, const string& right) {
  auto lval = left;
  auto lf = lval.find("<!>");
  if (lf != string::npos) {
    lval = lval.substr(0, lf);
  }
  auto rval = right;
  auto rf = rval.find("<!>");
  if (rf != string::npos) {
    rval = rval.substr(0, rf);
  }
  lf = lval.find(":");
  rf = rval.find(":");
  if (lf != string::npos && rf != string::npos) {
    if (lval.substr(0, lf) != rval.substr(0, rf)) {
      return to_lower(lval) < to_lower(rval);
    }
    auto sp_l = split(lval.substr(lf + 2), ",");
    auto sp_r = split(rval.substr(rf + 2), ",");
    vector<string> lvals, lunits, rvals, runits;
    for (auto p : sp_l) {
      trim(p);
      auto idx = p.find(" ");
      lvals.emplace_back(p.substr(0, idx));
      if (idx != string::npos) {
        lunits.emplace_back(p.substr(idx + 1));
      } else {
        lunits.emplace_back("");
      }
    }
    for (auto p : sp_r) {
      trim(p);
      auto idx=p.find(" ");
      rvals.emplace_back(p.substr(0, idx));
      if (idx != string::npos) {
        runits.emplace_back(p.substr(idx + 1));
      } else {
        runits.emplace_back("");
      }
    }
    for (size_t n = 0; n < lvals.size(); ++n) {
      standardize_value(lvals[n]);
      standardize_value(rvals[n]);
      if (lvals[n] != rvals[n]) {
        return level_value_compare(lvals[n], rvals[n], lunits[n], runits[n]);
      }
    }
    return level_value_compare(lvals.back(), rvals.back(), lunits.back(),
        runits.back());
  } else {
    if (lval.empty() && !rval.empty()) {
      return true;
    }
    if (rval.empty() && !lval.empty()) {
      return false;
    }
    return to_lower(lval) < to_lower(rval);
  }
}

bool compare_time_ranges(const string& left, const string& right) {
  regex re_anal("Analys[ie]s");
  if (regex_search(left, re_anal) && !regex_search(right, re_anal)) {
    return true;
  }
  if (regex_search(right, re_anal) && !regex_search(left, re_anal)) {
    return false;
  }
  int l_start, r_start;
  string l_end, r_end;
  string l_label, r_label;
  if (left.find("(initial+") != string::npos) {
    auto sp = split(left, "+");
    auto sp2 = split(sp[0]);
    l_label = sp2[sp2.size() - 2];
    l_start = stoi(sp[1].substr(0, sp[1].find(" ")));
    l_end = sp[2];
    chop(l_end);
  } else if (left.find("-") != string::npos) {
    if (left.find("Forecast") != string::npos) {
      l_start = -1;
    } else {
      l_start = 0;
    }
    auto idx = left.find("-");
    l_end = left.substr(0, idx);
    l_label = left.substr(idx + 1);
    idx = l_end.rfind(" ");
    if (idx != string::npos) {
      l_end = l_end.substr(idx + 1);
    }
  } else {
    l_start = 0;
    l_end = left;
  }
  if (right.find("(initial+") != string::npos) {
    auto sp = split(right, "+");
    auto sp2 = split(sp[0]);
    r_label = sp2[sp2.size() - 2];
    r_start = stoi(sp[1].substr(0, sp[1].find(" ")));
    r_end = sp[2];
    chop(r_end);
  } else if (right.find("-") != string::npos) {
    if (right.find("Forecast") != string::npos) {
      r_start = -1;
    } else {
      r_start = 0;
    }
    auto idx = right.find("-");
    r_end = right.substr(0, idx);
    r_label = right.substr(idx + 1);
    idx = r_end.rfind(" ");
    if (idx != string::npos) {
      r_end = r_end.substr(idx + 1);
    }
  } else {
    r_start = 0;
    r_end = right;
  }
  if (l_end.length() < r_end.length()) {
    l_end.insert(0, r_end.length() - l_end.length(), '0');
  }
  if (r_end.length() < l_end.length()) {
    r_end.insert(0, l_end.length() - r_end.length(), '0');
  }
  if (l_end < r_end) {
    return true;
  }
  if (r_end < l_end) {
    return false;
  }
  if (l_start < r_start) {
    return true;
  }
  if (r_start < l_start) {
    return false;
  }
  return l_label < r_label;
}

bool default_compare(const string& left, const string& right) {
  if (left.empty()) {
    return true;
  }
  if (right.empty()) {
    return false;
  }
  return to_lower(left) < to_lower(right);
}

} // end namespace metacompares
