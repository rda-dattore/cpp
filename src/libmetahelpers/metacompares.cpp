#include <regex>
#include <strutils.hpp>

namespace metacompares {

bool compare_grid_definitions(const std::string& left,const std::string& right)
{
  if (left == right) {
    return true;
  }
  else {
    auto lidx=left.find("&deg;");
    auto ridx=right.find("&deg;");
    if (lidx != std::string::npos && ridx != std::string::npos) {
	auto l=std::stof(left.substr(0,lidx));
	auto r=std::stof(right.substr(0,ridx));
	if (l <= r) {
	  return true;
	}
	else {
	  return false;
	}
    }
    else {
	if (lidx != std::string::npos) {
	  return true;
	}
	else if (ridx != std::string::npos) {
	  return false;
	}
	else {
	  if (left <= right) {
	    return true;
	  }
	  else {
	    return false;
	  }
	}
    }
  }
}

bool compare_levels(const std::string& left,const std::string& right)
{
  auto lval=left;
  auto lidx=lval.find("<!>");
  if (lidx != std::string::npos) {
    lval=lval.substr(0,lidx);
  }
  auto rval=right;
  auto ridx=rval.find("<!>");
  if (ridx != std::string::npos) {
    rval=rval.substr(0,ridx);
  }
  lidx=lval.find(":");
  ridx=rval.find(":");
  if (lidx != std::string::npos && ridx != std::string::npos) {
    if (lval.substr(0,lidx) != rval.substr(0,ridx)) {
	return strutils::to_lower(lval) < strutils::to_lower(rval);
    }
    else {
	auto lparts=strutils::split(lval.substr(lidx+2),",");
	auto rparts=strutils::split(rval.substr(ridx+2),",");
	std::vector<std::string> lvals,lunits,rvals,runits;
	for (auto part : lparts) {
	  strutils::trim(part);
	  auto idx=part.find(" ");
	  lvals.emplace_back(part.substr(0,idx));
	  if (idx != std::string::npos) {
	    lunits.emplace_back(part.substr(idx+1));
	  }
	  else {
	    lunits.emplace_back("");
	  }
	}
	for (auto part : rparts) {
	  strutils::trim(part);
	  auto idx=part.find(" ");
	  rvals.emplace_back(part.substr(0,idx));
	  if (idx != std::string::npos) {
	    runits.emplace_back(part.substr(idx+1));
	  }
	  else {
	    runits.emplace_back("");
	  }
	}
	for (size_t n=0; n < lvals.size(); ++n) {
	  if (strutils::contains(lvals[n],".") || strutils::contains(rvals[n],".")) {
	    if (!strutils::contains(lvals[n],".")) {
		lvals[n]+=".";
	    }
	    if (!strutils::contains(rvals[n],".")) {
		rvals[n]+=".";
	    }
	    auto lidx=lvals[n].find(".");
	    auto ridx=rvals[n].find(".");
	    if (lidx == ridx) {
		if (lvals[n].length() < 8) {
		  lvals[n].append(8-lvals[n].length(),'0');
		}
		if (rvals[n].length() < 8) {
		  rvals[n].append(8-rvals[n].length(),'0');
		}
	    }
	    else if (lidx < ridx) {
		lvals[n].insert(0,ridx-lidx,'0');
		if (lvals[n].length() < 16) {
		  lvals[n].append(16-lvals[n].length(),'0');
		}
		if (rvals[n].length() < 16) {
		  rvals[n].append(16-rvals[n].length(),'0');
		}
	    }
	    else {
		rvals[n].insert(0,lidx-ridx,'0');
		if (lvals[n].length() < 16) {
		  lvals[n].append(16-lvals[n].length(),'0');
		}
		if (rvals[n].length() < 16) {
		  rvals[n].append(16-rvals[n].length(),'0');
		}
	    }
	  }
	  else {
	    if (lvals[n].length() < 8) {
		lvals[n].insert(0,8-lvals[n].length(),'0');
	    }
	    if (rvals[n].length() < 8) {
		rvals[n].insert(0,8-rvals[n].length(),'0');
	    }
	  }
	  if ((strutils::contains(lunits[n],"mbar") && strutils::contains(runits[n],"mbar")) || (strutils::contains(lunits[n],"Pa") && strutils::contains(runits[n],"Pa")) || (strutils::contains(lunits[n],"sigma") && strutils::contains(runits[n],"sigma"))) {
	    if (lvals[n] > rvals[n]) {
		return true;
	    }
	    else if (lvals[n] < rvals[n]) {
		return false;
	    }
	  }
	  else {
	    if (lvals[n] < rvals[n]) {
		return true;
	    }
	    else if (lvals[n] > rvals[n]) {
		return false;
	    }
	  }
	}
	if ((strutils::contains(lunits.back(),"mbar") && strutils::contains(runits.back(),"mbar")) || (strutils::contains(lunits.back(),"Pa") && strutils::contains(runits.back(),"Pa")) || (strutils::contains(lunits.back(),"sigma") && strutils::contains(runits.back(),"sigma"))) {
	  return lvals.back() > rvals.back();
	}
	else {
	  return lvals.back() < rvals.back();
	}
    }
  }
  else {
    if (lval.empty() && !rval.empty()) {
	return true;
    }
    else if (rval.empty() && !lval.empty()) {
	return false;
    }
    else {
	return strutils::to_lower(lval) < strutils::to_lower(rval);
    }
  }
}

bool compare_time_ranges(const std::string& left,const std::string& right)
{
  std::regex re_anal("Analys[ie]s");
  if (std::regex_search(left,re_anal) && !std::regex_search(right,re_anal)) {
    return true;
  }
  else if (std::regex_search(right,re_anal) && !std::regex_search(left,re_anal)) {
    return false;
  }
  else {
    int l_start,r_start;
    std::string l_end,r_end;
    std::string l_label,r_label;
    if (strutils::contains(left,"(initial+")) {
	auto l_parts=strutils::split(left,"+");
	auto label_parts=strutils::split(l_parts[0]);
	l_label=label_parts[label_parts.size()-2];
	l_start=std::stoi(l_parts[1].substr(0,l_parts[1].find(" ")));
	l_end=l_parts[2];
	strutils::chop(l_end);
    }
    else if (strutils::contains(left,"-")) {
	if (strutils::contains(left,"Forecast")) {
	  l_start=-1;
	}
	else {
	  l_start=0;
	}
	l_end=left.substr(0,left.find("-"));
	auto idx=l_end.rfind(" ");
	if (idx != std::string::npos) {
	  l_end=l_end.substr(idx+1);
	}
    }
    else {
	l_start=0;
	l_end=left;
    }
    if (strutils::contains(right,"(initial+")) {
	auto r_parts=strutils::split(right,"+");
	auto label_parts=strutils::split(r_parts[0]);
	r_label=label_parts[label_parts.size()-2];
	r_start=std::stoi(r_parts[1].substr(0,r_parts[1].find(" ")));
	r_end=r_parts[2];
	strutils::chop(r_end);
    }
    else if (strutils::contains(right,"-")) {
	if (strutils::contains(right,"Forecast")) {
	  r_start=-1;
	}
	else {
	  r_start=0;
	}
	r_end=right.substr(0,right.find("-"));
	auto idx=r_end.rfind(" ");
	if (idx != std::string::npos) {
	  r_end=r_end.substr(idx+1);
	}
    }
    else {
	r_start=0;
	r_end=right;
    }
    if (l_end.length() < r_end.length()) {
	l_end.insert(0,r_end.length()-l_end.length(),'0');
    }
    if (r_end.length() < l_end.length()) {
	r_end.insert(0,l_end.length()-r_end.length(),'0');
    }
    if (l_end < r_end) {
	return true;
    }
    else if (r_end < l_end) {
	return false;
    }
    else if (l_start < r_start) {
	return true;
    }
    else if (r_start < l_start) {
	return false;
    }
    else {
	return l_label < r_label;
    }
  }
}

bool default_compare(const std::string& left,const std::string& right)
{
  if (left.empty()) {
    return true;
  }
  else if (right.empty()) {
    return false;
  }
  else {
    if (strutils::to_lower(left) <= strutils::to_lower(right)) {
	return true;
    }
    else {
	return false;
    }
  }
}

} // end namespace metacompares
