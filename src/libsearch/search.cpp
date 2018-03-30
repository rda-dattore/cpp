#include <string>
#include <MySQL.hpp>
#include <search.hpp>
#include <strutils.hpp>

namespace searchutils {

bool is_compound_term(std::string word,std::string& separator)
{
  if (strutils::contains(word,"-")) {
    separator="-";
    return true;
  }
  else if (strutils::contains(word,"/")) {
    separator="/";
    return true;
  }
  else if (strutils::contains(word,";")) {
    separator="/";
    return true;
  }
  return false;
}

std::string root_of_word(std::string word)
{
  std::string rword=word;
  if (!strutils::is_alpha(rword)) {
    return "";
  }
  if (strutils::has_ending(rword,"is") || strutils::has_ending(rword,"es")) {
    rword=rword.substr(0,rword.length()-2);
  }
  else if (strutils::has_ending(rword,"s")) {
    strutils::chop(rword);
  }
  if (strutils::has_ending(rword,"ing") || strutils::has_ending(rword,"ity") || strutils::has_ending(rword,"ian")) {
    rword=rword.substr(0,rword.length()-3);
  }
  if (strutils::has_ending(rword,"ly")) {
    rword=rword.substr(0,rword.length()-2);
  }
  if (strutils::has_ending(rword,"al")) {
    rword=rword.substr(0,rword.length()-2);
  }
  if (strutils::has_ending(rword,"ic")) {
    rword=rword.substr(0,rword.length()-2);
  }
  if (strutils::has_ending(rword,"ed")) {
    rword=rword.substr(0,rword.length()-2);
  }
  if (strutils::has_ending(rword,"ous")) {
    rword=rword.substr(0,rword.length()-3);
  }
  auto n=rword.length()-1;
  while (n > 0 && (rword[n] == rword[n-1] || (rword[n] >= '0' && rword[n] <= '9'))) {
    strutils::chop(rword);
    --n;
  }
  return rword;
}

std::string cleaned_search_word(std::string& word,bool& ignore)
{
  ignore=false;
  strutils::trim(word);
  word=strutils::to_lower(word);
  strutils::replace_all(word,"'","\\'");
// ignore full tags
  if (strutils::has_beginning(word,"<") && strutils::has_ending(word,">")) {
    ignore=true;
    return "";
  }
// strip tags from word
  while (word.length() > 0 && strutils::contains(word,"<") && strutils::contains(word,">")) {
    auto idx1=word.find("<");
    auto idx2=word.find(">");
    if (idx1 < idx2) {
	auto tag=word.substr(idx1);
	tag=tag.substr(0,tag.find(">")+1);
	strutils::replace_all(word,tag,"");
    }
    else {
	word=word.substr(idx1);
    }
  }
// ignore partial tags
  if (strutils::has_beginning(word,"<")) {
    ignore=true;
    return "";
  }
  word=word.substr(word.find(">")+1);
// strip punctuation
  if (strutils::has_ending(word,".") && strutils::occurs(word,".") == 1) {
    strutils::chop(word);
  }
  while (strutils::has_beginning(word,",") || strutils::has_beginning(word,":") || strutils::has_beginning(word,"'") || strutils::has_beginning(word,"\"") || strutils::has_beginning(word,"\\")) {
    word=word.substr(1);
  }
  while (strutils::has_ending(word,",") || strutils::has_ending(word,":") || strutils::has_ending(word,"'") || strutils::has_ending(word,"\"") || strutils::has_ending(word,"\\")) {
    strutils::chop(word);
  }
// strip out special characters
  if (strutils::has_beginning(word,"(")) {
    word=word.substr(1);
  }
  if (strutils::has_ending(word,")")) {
    strutils::chop(word);
  }
  if (word.find("(") != std::string::npos) {
    word+=")";
  }
  auto idx=word.find(")");
  if (idx != std::string::npos && idx < (word.length()-1)) {
    word="("+word;
  }
  if (strutils::has_beginning(word,"\"")) {
    word=word.substr(1);
  }
  if (strutils::has_beginning(word,"'")) {
    word=word.substr(1);
  }
  if (strutils::has_ending(word,"\\'s")) {
    word=word.substr(0,word.length()-3);
  }
// strip punctuation again
  if (strutils::has_ending(word,".") && strutils::occurs(word,".") == 1) {
    strutils::chop(word);
  }
  while (strutils::has_ending(word,",") || strutils::has_ending(word,":") || strutils::has_ending(word,"'") || strutils::has_ending(word,"\"") || strutils::has_ending(word,"\\") || strutils::has_ending(word,"+")) {
    strutils::chop(word);
  }
  if (word.length() == 0) {
    ignore=true;
    return "";
  }
  auto rword=word;
  int n=rword.length()-1;
  while (n > 0 && rword[n] >= '0' && rword[n] < '9') {
    strutils::chop(rword);
    --n;
  }
  if (rword.length() > 0 && strutils::is_alpha(rword)) {
    rword=root_of_word(rword);
  }
  else {
// ignore ds numbers
    if (word.length() == 7 && strutils::has_beginning(word,"ds") && word[5] == '.') {
	ignore=true;
	return "";
    }
    rword="";
  }
  return rword;
}

std::string insert_word_into_search_wordlist(MySQL::Server& server,const std::string& table,std::string dsnum,std::string word,size_t& location)
{
  bool ignore;
  auto sword=cleaned_search_word(word,ignore);
  std::string iloc=strutils::itos(location);
  if (!ignore) {
    if (server.insert(table,"'"+word+"','"+strutils::soundex(sword)+"',"+iloc+",'"+dsnum+"'") < 0) {
	return server.error();
    }
    MySQL::LocalQuery query("select a,b from search.cross_reference where a = '"+word+"' union select b,a from search.cross_reference where b = '"+word+"'");
    if (query.submit(server) < 0) {
	return query.error();
    }
    MySQL::Row row;
    while (query.fetch_row(row)) {
	auto word=row[1];
	sword=cleaned_search_word(word,ignore);
	if (!ignore) {
	  if (server.insert(table,"'"+word+"','"+strutils::soundex(sword)+"',"+iloc+",'"+dsnum+"'") < 0) {
	    if (!strutils::contains(server.error(),"Duplicate entry")) {
		return server.error();
	    }
	  }
	}
    }
    std::deque<std::string> word_parts;
    if (strutils::contains(word,"-")) {
	word_parts=strutils::split(word,"-");
    }
    else if (strutils::contains(word,"/")) {
	word_parts=strutils::split(word,"/");
    }
    else {
	word_parts=strutils::split("","");
    }
    std::string cword;
    for (size_t m=0; m < word_parts.size(); m++) {
	cword+=word_parts[m];
	if (word_parts[m].empty()) {
	  cword="ignore"+cword;
	}
	auto word=word_parts[m];
	sword=cleaned_search_word(word,ignore);
	if (!ignore) {
	  if (server.insert(table,"'"+word+"','"+strutils::soundex(sword)+"',"+iloc+",'"+dsnum+"'") < 0) {
	    if (!strutils::contains(server.error(),"Duplicate entry")) {
		return server.error();
	    }
	  }
	  query.set("select a,b from search.cross_reference where a = '"+word+"' union select b,a from search.cross_reference where b = '"+word+"'");
	  if (query.submit(server) < 0) {
	    return query.error();
	  }
	  while (query.fetch_row(row)) {
	    auto word=row[1];
	    sword=cleaned_search_word(word,ignore);
	    if (!ignore) {
		if (server.insert(table,"'"+word+"','"+strutils::soundex(sword)+"',"+iloc+",'"+dsnum+"'") < 0) {
		  if (!strutils::contains(server.error(),"Duplicate entry")) {
		    return server.error();
		  }
		}
	    }
	  }
	}
    }
    if (word_parts.size() > 0) {
	if (!strutils::has_beginning(cword,"ignore")) {
	  sword=cleaned_search_word(cword,ignore);
	  if (!ignore) {
	    if (server.insert(table,"'"+cword+"','"+strutils::soundex(sword)+"',"+iloc+",'"+dsnum+"'") < 0) {
		return server.error();
	    }
	  }
	}
    }
    ++location;
  }
  return "";
}

std::string index_variables(MySQL::Server& server,std::string dsnum)
{
// get GCMD variables
  MySQL::LocalQuery query("keyword","search.variables","dsid = '"+dsnum+"' and vocabulary = 'GCMD'");
  if (query.submit(server) < 0) {
    return query.error();
  }
  std::string varlist;
  MySQL::Row row;
  while (query.fetch_row(row)) {
    if (!varlist.empty()) {
	varlist+=" ";
    }
    auto keyword=row[0];
    strutils::replace_all(keyword,"EARTH SCIENCE > ","");
    varlist+=keyword;
  }
// get CMDMAP variables (mapped from content metadata)
  query.set("keyword","search.variables","dsid = '"+dsnum+"' and vocabulary = 'CMDMAP'");
  if (query.submit(server) < 0) {
    return query.error();
  }
// if no CMDMAP variables, get CMDMM variables (specified with metadata manager)
  if (query.num_rows() == 0) {
    query.set("keyword","search.variables","dsid = '"+dsnum+"' and vocabulary = 'CMDMM'");
    if (query.submit(server) < 0) {
	return query.error();
    }
  }
  while (query.fetch_row(row)) {
    if (!varlist.empty()) {
	varlist+=" ";
    }
    varlist+=row[0];
  }
  auto vars=strutils::split(varlist);
  server._delete("search.variables_wordlist","dsid = '"+dsnum+"'");
  size_t m;
  for (const auto& var : vars) {
    auto error=insert_word_into_search_wordlist(server,"search.variables_wordlist",dsnum,var,m);
    if (!error.empty()) {
	return error;
    }
  }
  server.disconnect();
  return "";
}

std::string index_locations(MySQL::Server& server,std::string dsnum)
{
// get locations
  MySQL::Query query("keyword","search.locations","dsid = '"+dsnum+"'");
  if (query.submit(server) < 0) {
    return query.error();
  }
  std::string loclist;
  MySQL::Row row;
  while (query.fetch_row(row)) {
    if (loclist.length() > 0)
	loclist+=" ";
    loclist+=row[0];
  }
  auto locs=strutils::split(loclist);
  server._delete("search.locations_wordlist","dsid = '"+dsnum+"'");
  size_t m;
  for (const auto& loc : locs) {
    auto error=insert_word_into_search_wordlist(server,"search.locations_wordlist",dsnum,loc,m);
    if (!error.empty() && !strutils::contains(error,"Duplicate entry")) {
	return error;
    }
  }
  server.disconnect();
  return "";
}

std::string convert_to_expandable_summary(std::string summary,size_t visible_length,size_t& iterator)
{
  size_t n,m;

  auto summary_parts=strutils::split(summary);
  std::string expandable_summary;
  if (summary_parts.size() < visible_length) {
    expandable_summary=summary;
    strutils::trim(expandable_summary);
    strutils::replace_all(expandable_summary,"<p>","<span>");
    strutils::replace_all(expandable_summary,"</p>","</span><br /><br />");
    if (strutils::has_ending(expandable_summary,"\n")) {
	expandable_summary=expandable_summary.substr(0,expandable_summary.length()-13);
    }
    else {
	expandable_summary=expandable_summary.substr(0,expandable_summary.length()-12);
    }
  }
  else {
    expandable_summary="";
    auto in_tag=0;
    for (n=0,m=1; n < visible_length; n++,m++) {
	if (strutils::contains(summary_parts[n],"</p>") || (m < summary_parts.size() && strutils::contains(summary_parts[m],"</p>"))) {
	  break;
	}
	if (expandable_summary.length() > 0) {
	  expandable_summary+=" ";
	}
	expandable_summary+=summary_parts[n];
	in_tag+=strutils::occurs(summary_parts[n],"<")-strutils::occurs(summary_parts[n],"</")*2+strutils::occurs(summary_parts[n],"</p>")-strutils::occurs(summary_parts[n],"<p>");
    }
    if (in_tag > 0) {
	while (n < summary_parts.size() && in_tag > 0) {
	  expandable_summary+=" "+summary_parts[n];
	  if (strutils::contains(summary_parts[n],"</")) {
	    --in_tag;
	  }
	  ++n;
	}
    }
    strutils::replace_all(expandable_summary,"<p>","<span>");
    strutils::replace_all(expandable_summary,"</p>","</span><br /><br />");
    std::string summary_remainder="<span>";
    for (; n < summary_parts.size(); ++n) {
	summary_remainder+=" "+summary_parts[n];
    }
    strutils::replace_all(summary_remainder,"<p>","<span>");
    strutils::replace_all(summary_remainder,"</p>","</span><br /><br />");
    strutils::trim(summary_remainder);
    if (strutils::has_ending(expandable_summary,("<br /><br />\n"))) {
	expandable_summary=expandable_summary.substr(0,expandable_summary.length()-13);
	summary_remainder="<br /><br />"+summary_remainder;
    }
    if (summary_remainder.length() > 12) {
	expandable_summary+="<span id=\"D"+strutils::itos(iterator)+"\">&nbsp;...&nbsp;<a href=\"javascript:swapDivs("+strutils::itos(iterator)+","+strutils::itos(iterator+1)+")\" title=\"Expand\"><img src=\"/images/expand.gif\" width=\"11\" height=\"11\" border=\"0\"></a></span></span><span style=\"visibility: hidden; position: absolute; left: 0px\" id=\"D"+strutils::itos(iterator+1)+"\">"+summary_remainder.substr(0,summary_remainder.length()-12)+" <a href=\"javascript:swapDivs("+strutils::itos(iterator+1)+","+strutils::itos(iterator)+")\" title=\"Collapse\"><img src=\"/images/contract.gif\" width=\"11\" height=\"11\" border=\"0\"></a></span>";
    }
    if (strutils::has_ending(expandable_summary,"\n")) {
	expandable_summary=expandable_summary.substr(0,expandable_summary.length()-1);
    }
    iterator+=2;
  }
  return expandable_summary;
}

std::string horizontal_resolution_keyword(double max_horizontal_resolution,size_t resolution_type)
{
// resolution_type = 0 for resolutions in degrees
// resolution_type = 1 for resolutions in km

  std::string keyword;
  if ((resolution_type == 0 && max_horizontal_resolution >= 0.01 && max_horizontal_resolution < 0.09) || (resolution_type == 1 && max_horizontal_resolution >= 1. && max_horizontal_resolution < 10.)) {
    keyword="H : 1 km - < 10 km or approximately .01 degree - < .09 degree";
  }
  else if ((resolution_type == 0 && max_horizontal_resolution >= 0.09 && max_horizontal_resolution < 0.5) || (resolution_type == 1 && max_horizontal_resolution >= 10. && max_horizontal_resolution < 50.)) {
    keyword="H : 10 km - < 50 km or approximately .09 degree - < .5 degree";
  }
  else if ((resolution_type == 0 && max_horizontal_resolution >= 0.5 && max_horizontal_resolution < 1.) || (resolution_type == 1 && max_horizontal_resolution >= 50. && max_horizontal_resolution < 100.)) {
    keyword="H : 50 km - < 100 km or approximately .5 degree - < 1 degree";
  }
  else if ((resolution_type == 0 && max_horizontal_resolution >= 1. && max_horizontal_resolution < 2.5) || (resolution_type == 1 && max_horizontal_resolution >= 100. && max_horizontal_resolution < 250.)) {
    keyword="H : 100 km - < 250 km or approximately 1 degree - < 2.5 degrees";
  }
  else if ((resolution_type == 0 && max_horizontal_resolution >= 2.5 && max_horizontal_resolution < 5.) || (resolution_type == 1 && max_horizontal_resolution >= 250. && max_horizontal_resolution < 500.)) {
    keyword="H : 250 km - < 500 km or approximately 2.5 degrees - < 5.0 degrees";
  }
  else if ((resolution_type == 0 && max_horizontal_resolution >= 5. && max_horizontal_resolution < 10.) || (resolution_type == 1 && max_horizontal_resolution >= 500. && max_horizontal_resolution < 1000.)) {
    keyword="H : 500 km - < 1000 km or approximately 5 degrees - < 10 degrees";
  }
  else if ((resolution_type == 0 && max_horizontal_resolution < 0.00001) || (resolution_type == 1 && max_horizontal_resolution < 0.001)) {
    keyword="H : < 1 meter";
  }
  else if ((resolution_type == 0 && max_horizontal_resolution >= 0.00001 && max_horizontal_resolution < 0.0003) || (resolution_type == 1 && max_horizontal_resolution >= 0.001 && max_horizontal_resolution < 0.03)) {
    keyword="H : 1 meter - < 30 meters";
  }
  else if ((resolution_type == 0 && max_horizontal_resolution >= 0.0003 && max_horizontal_resolution < 0.001) || (resolution_type == 1 && max_horizontal_resolution >= 0.03 && max_horizontal_resolution < 0.1)) {
    keyword="H : 30 meters - < 100 meters";
  }
  else if ((resolution_type == 0 && max_horizontal_resolution >= 0.001 && max_horizontal_resolution < 0.0025) || (resolution_type == 1 && max_horizontal_resolution >= 0.1 && max_horizontal_resolution < 0.25)) {
    keyword="H : 100 meters - < 250 meters";
  }
  else if ((resolution_type == 0 && max_horizontal_resolution >= 0.0025 && max_horizontal_resolution < 0.005) || (resolution_type == 1 && max_horizontal_resolution >= 0.25 && max_horizontal_resolution < 0.5)) {
    keyword="H : 250 meters - < 500 meters";
  }
  else if ((resolution_type == 0 && max_horizontal_resolution >= 0.005 && max_horizontal_resolution < 0.01) || (resolution_type == 1 && max_horizontal_resolution >= 0.5 && max_horizontal_resolution < 1.)) {
    keyword="H : 500 meters - < 1 km";
  }
  else {
    keyword="H : >= 1000 km or >= 10 degrees";
  }
  return keyword;
}

std::string time_resolution_keyword(std::string frequency_type,int number,std::string unit,std::string statistics)
{
  std::string keyword;
  if (frequency_type == "climatology") {
    if (unit == "hour") {
	keyword="T : Hourly Climatology";
    }
    else if (unit == "day") {
	if (number == 5) {
	  keyword="T : Pentad Climatology";
	}
	else if (number == 7) {
	  keyword="T : Weekly Climatology";
	}
	else {
	  keyword="T : Daily Climatology";
	}
    }
    else if (unit == "week") {
	keyword="T : Weekly Climatology";
    }
    else if (unit == "month") {
	keyword="T : Monthly Climatology";
    }
    else if (unit == "season" || unit == "winter" || unit == "spring" || unit == "summer" || unit == "autumn") {
	keyword="T : Seasonal Climatology";
    }
    else if (unit == "year") {
	keyword="T : Annual Climatology";
    }
    else if (unit == "30-year") {
	keyword="T : Climate Normal (30-year climatology)";
    }
  }
  else if (frequency_type == "regular") {
    if (unit == "second") {
      keyword="T : 1 second - < 1 minute";
    }
    else if (unit == "minute") {
      keyword="T : 1 minute - < 1 hour";
    }
    else if (unit == "hour") {
      keyword="T : Hourly - < Daily";
    }
    else if (unit == "day") {
      keyword="T : Daily - < Weekly";
    }
    else if (unit == "week") {
      keyword="T : Weekly - < Monthly";
    }
    else if (unit == "month" || unit == "season") {
      keyword="T : Monthly - < Annual";
    }
    else if (unit == "year" || unit == "winter" || unit == "spring" || unit == "summer" || unit == "autumn") {
      keyword="T : Annual";
    }
    else if (unit == "decade") {
      keyword="T : Decadal";
    }
  }
  else {
    if (unit == "second") {
	if (number > 1) {
	  keyword="T : < 1 second";
	}
	else {
	  keyword="T : 1 second - < 1 minute";
	}
    }
     else if (unit == "minute") {
	if (number > 1) {
	  keyword="T : 1 second - < 1 minute";
	}
	else {
	  keyword="T : 1 minute - < 1 hour";
	}
    }
    else if (unit == "hour") {
	if (number > 1) {
	  keyword="T : 1 minute - < 1 hour";
	}
	else {
	  keyword="T : Hourly - < Daily";
	}
    }
    else if (unit == "day") {
	if (number > 1) {
	  keyword="T : Hourly - < Daily";
	}
	else {
	  keyword="T : Daily - < Weekly";
	}
    }
    else if (unit == "week") {
	if (number > 1) {
	  keyword="T : Daily - < Weekly";
	}
	else {
	  keyword="T : Weekly - < Monthly";
	}
    }
    else if (unit == "month") {
	if (number > 1) {
	  keyword="T : Weekly - < Monthly";
	}
	else {
	  keyword="T : Monthly - < Annual";
	}
    }
    else if (unit == "season" || unit == "winter" || unit == "spring" || unit == "summer" || unit == "autumn") {
      keyword="T : Monthly - < Annual";
    }
    else if (unit == "year") {
	if (number > 1) {
	  keyword="T : Monthly - < Annual";
	}
	else {
	  keyword="T : Annual";
	}
    }
    else if (unit == "decade") {
	if (number > 1) {
	  keyword="T : Annual";
	}
	else {
	  keyword="T : Decadal";
	}
    }
  }
  return keyword;
}

} // end namespace searchutils
