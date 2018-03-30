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
	if (strutils::to_lower(lval) <= strutils::to_lower(rval)) {
	  return true;
	}
	else {
	  return false;
	}
    }
    else {
	lval=lval.substr(lidx+2);
	lidx=lval.find(" ");
	auto lunits=lval.substr(lidx+1);
	lval=lval.substr(0,lidx);
	rval=rval.substr(ridx+2);
	ridx=rval.find(" ");
	auto runits=rval.substr(ridx+1);
	rval=rval.substr(0,ridx);
	if (strutils::contains(lval,".") || strutils::contains(rval,".")) {
	  if (!strutils::contains(lval,".")) {
	    lval+=".";
	  }
	  if (!strutils::contains(rval,".")) {
	    rval+=".";
	  }
	  if ( (lidx=lval.find(".")) == (ridx=rval.find("."))) {
	    while (lval.length() < 8) {
		lval+="0";
	    }
	    while (rval.length() < 8) {
		rval+="0";
	    }
	  }
	  else if (lidx < ridx) {
	    while ( (lidx=lval.find(".") < ridx)) {
		lval="0"+lval;
	    }
	    while (lval.length() < 16) {
		lval+="0";
	    }
	    while (rval.length() < 16) {
		rval+="0";
	    }
	  }
	  else {
	    while ( (ridx=rval.find(".") < lidx)) {
		rval="0"+rval;
	    }
	    while (lval.length() < 16) {
		lval+="0";
	    }
	    while (rval.length() < 16) {
		rval+="0";
	    }
	  }
	}
	else {
	  while (lval.length() < 8) {
	    lval="0"+lval;
	  }
	  while (rval.length() < 8) {
	    rval="0"+rval;
	  }
	}
	if ((strutils::contains(lunits,"mbar") && strutils::contains(runits,"mbar")) || (strutils::contains(lunits,"Pa") && strutils::contains(runits,"Pa")) || (strutils::contains(lunits,"sigma") && strutils::contains(runits,"sigma"))) {
	  if (lval >= rval) {
	    return true;
	  }
	  else {
	    return false;
	  }
	}
	else {
	  if (lval <= rval) {
	    return true;
	  }
	  else {
	    return false;
	  }
	}
    }
  }
  else {
    if (lval.empty()) {
	return true;
    }
    else if (rval.empty()) {
	return false;
    }
    else {
	if (strutils::to_lower(lval) <= strutils::to_lower(rval)) {
	  return true;
	}
	else {
	  return false;
	}
    }
  }
}

bool compare_time_ranges(const std::string& left,const std::string& right)
{
  if (left == "Analysis" || strutils::contains(left,"Analyses")) {
    return true;
  }
  else if (right == "Analysis" || strutils::contains(right,"Analyses")) {
    return false;
  }
  int l_start,r_start;
  std::string l_end,r_end;
  if (strutils::contains(left,"(initial+")) {
    auto l_parts=strutils::split(left,"+");
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
  else if (l_end == r_end) {
    if (l_start <= r_start) {
	return true;
    }
    else {
	return false;
    }
  }
  else {
    return false;
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
