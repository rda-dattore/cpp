#include <sys/stat.h>
#include <regex>
#include <citation.hpp>
#include <xmlutils.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <tempfile.hpp>
#include <MySQL.hpp>
#include <datetime.hpp>

namespace citation {

const std::string DOI_URL_BASE="https://doi.org";

bool open_ds_overview(XMLDocument& xdoc,std::string dsnum)
{
  std::string fname;
  struct stat buf;
  if (stat(("/usr/local/www/server_root/web/datasets/ds"+dsnum+"/metadata/dsOverview.xml").c_str(),&buf) == 0) {
    fname="/usr/local/www/server_root/web/datasets/ds"+dsnum+"/metadata/dsOverview.xml";
  }
  else {
    TempDir temp_dir;
    if (!temp_dir.create("/tmp")) {
	return false;
    }
    fname=unixutils::remote_web_file("http://rda.ucar.edu/datasets/ds"+dsnum+"/metadata/dsOverview.xml",temp_dir.name());
  }
  if (xdoc.open(fname)) {
    return true;
  }
  else {
    return false;
  }
}

std::string list_authors(XMLSnippet& xsnip,MySQL::Server& server,size_t max_num_authors,std::string et_al_string,bool last_first_middle_on_first_author_only)
{
  std::string authlist;
  auto elist=xsnip.element_list("dsOverview/author");
  if (elist.size() > 0) {
    if (elist.size() > max_num_authors) {
	auto f=elist.front().attribute_value("fname");
	auto m=elist.front().attribute_value("mname");
	auto l=elist.front().attribute_value("lname");
	authlist=l+", "+f.substr(0,1)+".";
	if (!m.empty()) {
	  if (strutils::occurs(m,".") > 0) {
	    authlist+=" "+m;
	  }
	  else {
	    authlist+=" "+m.substr(0,1)+".";
	  }
	}
	authlist+=", "+et_al_string;
    }
    else {
	size_t n=0;
	for (const auto& e : elist) {
	  auto f=e.attribute_value("fname");
	  auto m=e.attribute_value("mname");
	  auto l=e.attribute_value("lname");
	  if (last_first_middle_on_first_author_only && n > 0) {
	    authlist+=", ";
	    if (n > 0 && (n+1) == elist.size()) {
		authlist+="and ";
	    }
	    authlist+=f.substr(0,1)+".";
	    if (!m.empty()) {
		if (strutils::occurs(m,".") > 0) {
		  authlist+=" "+m;
		}
		else {
		  authlist+=" "+m.substr(0,1)+".";
		}
	    }
	    authlist+=" "+l;
	  }
	  else {
	    if (!authlist.empty()) {
		authlist+=", ";
		if (n > 0 && (n+1) == elist.size()) {
		  authlist+="and ";
		}
	    }
	    authlist+=l+", "+f.substr(0,1)+".";
	    if (!m.empty()) {
		if (strutils::occurs(m,".") > 0) {
		  authlist+=" "+m;
		}
		else {
		  authlist+=" "+m.substr(0,1)+".";
		}
	    }
	  }
	  ++n;
	}
    }
  }
  else {
    MySQL::LocalQuery query("select g.path,c.contact from search.contributors_new as c left join search.GCMD_providers as g on g.uuid = c.keyword where c.dsid = '"+xsnip.element("dsOverview").attribute_value("ID")+"' and c.vocabulary = 'GCMD' and c.citable = 'Y' order by c.disp_order");
    if (query.submit(server) == 0) {
	std::vector<std::string> c_list;
	MySQL::Row row;
	while (query.fetch_row(row)) {
	  c_list.emplace_back(row[0]+"<!>"+row[1]);
	}
	if (c_list.size() > max_num_authors) {
	  auto names=strutils::split(c_list.front()," > ");
	  authlist+=strutils::substitute(names.back(),", ","/")+", "+et_al_string;
	}
	else {
	  size_t n=0;
	  for (const auto& c : c_list) {
	    auto c_parts=strutils::split(c,"<!>");
	    if (!authlist.empty()) {
		authlist+=", ";
	    }
	    if (n > 0 && (n+1) == c_list.size()) {
		authlist+="and ";
	    }
	    auto names=strutils::split(c_parts.front()," > ");
	    if (names.back() == "UNAFFILIATED INDIVIDUAL") {
		auto contact_parts=strutils::split(c_parts.back(),",");
		if (contact_parts.size() > 0) {
		  names.back()=strutils::capitalize(names.back())+", "+contact_parts.front();
		}
		else {
		  names.back()="";
		}
	    }
	    if (!names.back().empty()) {
		authlist+=strutils::substitute(names.back(),", ","/");
	    }
	    ++n;
	  }
	}
    }
  }
  return authlist;
}

std::string formatted_access_date(std::string access_date,bool htmlized)
{
  std::string formatted_access_date_="Accessed";
  if (!access_date.empty() && strutils::occurs(access_date,"-") == 2 && strutils::is_numeric(strutils::substitute(access_date,"-",""))) {
    auto dparts=strutils::split(access_date,"-");
    DateTime dt(std::stoi(dparts[0]),std::stoi(dparts[1]),std::stoi(dparts[2]),0,0);
    formatted_access_date_+=" "+dt.to_string("%dd %h %Y")+".";
  }
  else {
    if (htmlized) {
	formatted_access_date_+="<font color=\"red\">&dagger;</font>";
    }
    formatted_access_date_+=" dd mmm yyyy.";
  }
  return formatted_access_date_;
}

std::string citation(std::string dsnum,std::string style,std::string access_date,MySQL::Row& row,XMLSnippet& xsnip,MySQL::Server& server,bool htmlized)
{
  std::string citation;
  style=strutils::to_lower(style);
  if (style == "esip") {
    citation=list_authors(xsnip,server,10,"et al")+". "+row[1].substr(0,4);
    auto e=xsnip.element("dsOverview/continuingUpdate");
    if (e.attribute_value("value") == "yes" && e.attribute_value("frequency") != "irregularly") {
	citation+=", updated "+e.attribute_value("frequency");
    }
    citation+=". ";
    if (htmlized) {
	citation+="<i>";
    }
    citation+=row[0];
    if (htmlized) {
	citation+="</i>";
    }
    citation+=". "+DATA_CENTER+". ";
    if (!row[2].empty() && strutils::has_beginning(row[2],"10.5065/")) {
	citation+=DOI_URL_BASE+"/"+row[2];
    }
    else {
	citation+="http://rda.ucar.edu/datasets/ds"+dsnum+"/";
    }
    citation+=". "+formatted_access_date(access_date,htmlized);
  }
  else if (style == "agu") {
    citation=list_authors(xsnip,server,9,"et al.")+" ("+row[1].substr(0,4)+"), "+row[0]+", ";
    if (!row[2].empty()) {
	citation+=DOI_URL_BASE+"/"+row[2];
    }
    else {
	citation+="http://rda.ucar.edu/datasets/ds"+dsnum+"/";
    }
    citation+=", "+DATA_CENTER+", Boulder, Colo.";
    auto e=xsnip.element("dsOverview/continuingUpdate");
    if (e.attribute_value("value") == "yes") {
	citation+=" (Updated "+e.attribute_value("frequency")+".)";
    }
    citation+=" "+formatted_access_date(access_date,htmlized);
  }
  else if (style == "ams") {
    citation=list_authors(xsnip,server,8,"and Coauthors")+", "+row[1].substr(0,4)+": "+row[0]+". "+DATA_CENTER+", Boulder, CO. [Available online at ";
    if (!row[2].empty()) {
	citation+=DOI_URL_BASE+"/"+row[2];
    }
    else {
	citation+="http://rda.ucar.edu/datasets/ds"+dsnum+"/";
    }
    citation+=".] "+formatted_access_date(access_date,htmlized);
  }
  else if (style == "datacite") {
    citation=list_authors(xsnip,server,32768,"",false)+" ("+row[1].substr(0,4)+"): "+row[0]+". "+DATA_CENTER+". Dataset. ";
    if (!row[2].empty()) {
	citation+=DOI_URL_BASE+"/"+row[2];
    }
    else {
	citation+="http://rda.ucar.edu/datasets/ds"+dsnum+"/";
    }
    citation+=". "+formatted_access_date(access_date,htmlized);
  }
  else if (style == "gdj") {
    citation=list_authors(xsnip,server,32768,"",false)+" ("+row[1].substr(0,4)+"): "+row[0]+". "+DATA_CENTER+". ";
    if (!row[2].empty()) {
	citation+=DOI_URL_BASE+"/"+row[2];
    }
    else {
	citation+="http://rda.ucar.edu/datasets/ds"+dsnum+"/";
    }
    citation+=". "+formatted_access_date(access_date,htmlized);
  }
  if (htmlized && (access_date.empty() || strutils::occurs(access_date,"-") != 2 || !strutils::is_numeric(strutils::substitute(access_date,"-","")))) {
    citation+="<br />&nbsp;&nbsp;<small><font color=\"red\">&dagger;</font>Please fill in the \"Accessed\" date with the day, month, and year (e.g. - 5 Aug 2011) you last accessed the data from the RDA.</small>";
  }
  return citation;
}

std::string add_citation_style_changer(std::string style)
{
  std::string style_changer="<br /><small>Bibliographic citation shown in <form name=\"citation\" style=\"display: inline\"><select name=\"style\" onChange=\"changeCitation()\">";
  style_changer+="<option value=\"agu\"";
  if (style == "agu") {
    style_changer+=" selected";
  }
  style_changer+=">American Geophysical Union (AGU)</option>";
  style_changer+="<option value=\"ams\"";
  if (style == "ams") {
    style_changer+=" selected";
  }
  style_changer+=">American Meteorological Society (AMS)</option>";
  style_changer+="<option value=\"datacite\"";
  if (style == "datacite") {
    style_changer+=" selected";
  }
  style_changer+=">DataCite (DC)</option>";
  style_changer+="<option value=\"esip\"";
  if (style == "esip") {
    style_changer+=" selected";
  }
  style_changer+=">Federation of Earth Science Information Partners (ESIP)</option>";
  style_changer+="<option value=\"gdj\"";
  if (style == "gdj") {
    style_changer+=" selected";
  }
  style_changer+=">Geoscience Data Journal</option>";
  style_changer+="</select></form> style</small>";
  return style_changer;
}

std::string citation(std::string dsnum,std::string style,std::string access_date,MySQL::Server& server,bool htmlized)
{
  std::string citation_;
  auto dsparts=strutils::split(dsnum,"<!>");
  if (std::regex_search(dsparts[0],std::regex("^ds"))) {
    dsparts[0]=dsparts[0].substr(2);
  }
  std::string doi;
  if (dsparts.size() > 1 && !dsparts[1].empty()) {
    doi=dsparts[1];
  }
  XMLDocument xdoc;
  if (open_ds_overview(xdoc,dsparts[0])) {
    MySQL::LocalQuery query;
    query.set("select s.title,s.pub_date,d.doi,min(d.start_date) sd,max(d.end_date) ed,group_concat(d.status) from search.datasets as s left join dssdb.dsvrsn as d on d.dsid = concat('ds',s.dsid) where s.dsid = '"+dsparts[0]+"' and (d.status in ('A','H') or isnull(d.doi)) group by d.doi order by locate('A',group_concat(d.status)) desc,sd desc");
    MySQL::Row row;
    if (query.submit(server) == 0) {
	if (query.num_rows() > 1) {
	  if (htmlized) {
	    citation_+="<small>";
	  }
	  citation_+="<img src=\"/images/alert.gif\" width=\"12\" height=\"12\" />&nbsp;This dataset has more than one DOI. You are viewing the citation for data downloaded <form name=\"doi_select\" style=\"display: inline\"><select name=\"doi\" onChange=\"changeCitation()\">";
	  std::string cm;
	  auto is_first=true;
	  while (query.fetch_row(row)) {
	    if (is_first && std::regex_search(row[5],std::regex("A"))) {
		citation_+="<option value=\""+row[2]+"\"";
		if (row[2] == doi) {
		  citation_+=" selected";
		}
		citation_+=">on or after "+row[3]+"</option>";
	    }
	    else {
		citation_+="<option value=\""+row[2]+"\"";
		if (row[2] == doi) {
		  citation_+=" selected";
		}
		citation_+=">between "+row[3]+" and "+DateTime(std::stoll(strutils::substitute(row[4],"-",""))*1000000).days_subtracted(1).to_string("%Y-%m-%d")+"</option>";
	    }
	    if (!doi.empty() && doi == row[2]) {
		cm=citation(dsparts[0],style,access_date,row,xdoc,server,htmlized);
	    }
	    is_first=false;
	  }
	  if (cm.empty()) {
	    query.rewind();
	    query.fetch_row(row);
	    cm=citation(dsparts[0],style,access_date,row,xdoc,server,htmlized);
	  }
	  citation_+="</select></form>";
	  if (htmlized) {
	    citation_+="</small><br />";
	  }
	  citation_+=cm;
	}
	else if (query.fetch_row(row)) {
	  citation_+=citation(dsparts[0],style,access_date,row,xdoc,server,htmlized);
	}
    }
    if (htmlized) {
	citation_+=add_citation_style_changer(style);
    }
    xdoc.close();
  }
  return citation_;
}

std::string temporary_citation(std::string dsnum,std::string style,std::string access_date,MySQL::Server& server,bool htmlized)
{
  std::string citation_;
  std::string xml;
  xml.reserve(30000);
  if (strutils::has_beginning(dsnum,"ds")) {
    dsnum=dsnum.substr(2);
  }
  MySQL::LocalQuery query("select authors from metautil.metaman where dsid = '"+dsnum+"'");
  MySQL::Row row;
  if (query.submit(server) == 0 && query.fetch_row(row) && !row[0].empty()) {
    xml="<dsOverview ID=\""+dsnum+"\">";
    auto authors=strutils::split(row[0],"\n");
    for (const auto& a : authors) {
	auto aparts=strutils::split(a,"[!]");
	xml+="<author fname=\""+aparts[0]+"\" mname=\""+aparts[1]+"\" lname=\""+aparts[2]+"\" />";
    }
    xml+="</dsOverview>";
  }
  else {
    query.set("select contributors from metautil.metaman where dsid = '"+dsnum+"'");
    if (query.submit(server) == 0 && query.fetch_row(row) && !row[0].empty()) {
	xml="<dsOverview ID=\""+dsnum+"\">";
	auto contributors=strutils::split(row[0],"\n");
	for (const auto& c : contributors) {
	  auto cparts=strutils::split(c,"[!]");
	  if (cparts.size() == 5 || cparts[3] == "Y") {
	    xml+="<contributor name=\""+cparts[0]+"\" />";
	  }
	}
	xml+="</dsOverview>";
    }
  }
  if (!xml.empty()) {
    query.set("select m.title,s.pub_date,d.doi from metautil.metaman as m left join search.datasets as s on s.dsid = m.dsid left join dssdb.dsvrsn as d on d.dsid = concat('ds',m.dsid) where m.dsid = '"+dsnum+"'");
    if (query.submit(server) == 0 && query.fetch_row(row)) {
	XMLSnippet xsnip(xml);
	citation_=citation(dsnum,style,access_date,row,xsnip,server,htmlized);
	if (htmlized) {
	  citation_+=add_citation_style_changer(style);
	}
    }
  }
  return citation_;
}

void export_to_ris(std::ostream& ofs,std::string dsnum,std::string access_date,MySQL::Server& server)
{
  XMLDocument xdoc;
  if (open_ds_overview(xdoc,dsnum)) {
    ofs << "Provider: " << DATA_CENTER << "\r" << std::endl;
    ofs << "Tagformat: ris\r" << std::endl;
    ofs << "\r" << std::endl;
    ofs << "TY  - DATA\r" << std::endl;
    auto elist=xdoc.element_list("dsOverview/author");
    if (elist.size() > 0) {
	for (const auto& e : elist) {
	  ofs << "AU  - " << e.attribute_value("lname") << ", " << e.attribute_value("fname");
	  auto mname=e.attribute_value("mname");
	  if (!mname.empty()) {
	    ofs << " " << mname;
	  }
	  ofs << "\r" << std::endl;
	}
    }
    else {
	elist=xdoc.element_list("dsOverview/contributor");
	for (const auto& e : elist) {
	  auto nparts=strutils::split(e.attribute_value("name")," > ");
	  ofs << "AU  - " << nparts[1] << "\r" << std::endl;
	}
    }
    MySQL::LocalQuery query("select s.title,s.summary,s.pub_date,d.doi from search.datasets as s left join dssdb.dsvrsn as d on d.dsid = concat('ds',s.dsid) where s.dsid = '"+dsnum+"'");
    MySQL::Row row;
    if (query.submit(server) == 0 && query.fetch_row(row)) {
	ofs << "T1  - " << row[0] << "\r" << std::endl;
	auto s=row[1];
	strutils::replace_all(s,"\n"," ");
	std::regex r=std::regex("  ");
	while (std::regex_search(s,r)) {
	  strutils::replace_all(s,"  "," ");
	}
	XMLSnippet xs("<summary>"+s+"</summary>");
	auto e=xs.element("summary");
	auto abstract=xmlutils::to_plain_text(e,80,6).substr(6);
	strutils::replace_all(abstract,"\n","\r\n");
	ofs << "AB  - " << abstract << "\r" << std::endl;
	ofs << "PY  - " << row[2].substr(0,4) << "\r" << std::endl;
	if (!row[3].empty()) {
	  ofs << "DO  - " << row[3] << "\r" << std::endl;
	  ofs << "UR  - " << DOI_URL_BASE << "/" << row[3] << "\r" << std::endl;
	}
	else {
	  ofs << "UR  - http://rda.ucar.edu/datasets/ds" << dsnum << "/\r" << std::endl;
	}
    }
    ofs << "PB  - " << DATA_CENTER << "\r" << std::endl;
    ofs << "CY  - Boulder, CO\r" << std::endl;
    ofs << "LA  - English\r" << std::endl;
    if (!access_date.empty() && strutils::occurs(access_date,"-") == 2 && strutils::is_numeric(strutils::substitute(access_date,"-",""))) {
	auto dparts=strutils::split(access_date,"-");
	ofs << "Y2  - " << dparts[0] << "/" << dparts[1] << "/" << dparts[2] << "\r" << std::endl;
    }
    ofs << "ER  - \r" << std::endl;
  }
}

void export_to_bibtex(std::ostream& ofs,std::string dsnum,std::string access_date,MySQL::Server& server)
{
  XMLDocument xdoc;
  if (open_ds_overview(xdoc,dsnum)) {
    ofs << "@misc{cisl_rda_ds" << dsnum << std::endl;
    ofs << " author = \"";
    auto elist=xdoc.element_list("dsOverview/author");
    auto n=0;
    if (elist.size() > 0) {
	for (const auto& e : elist) {
	  if (n > 0) {
	    ofs << " and ";
	  }
	  ofs << e.attribute_value("fname");
	  auto mname=e.attribute_value("mname");
	  if (!mname.empty()) {
	    ofs << " " << mname;
	  }
	  ofs << " {" << e.attribute_value("lname") << "}";
	  ++n;
	}
    }
    else {
	elist=xdoc.element_list("dsOverview/contributor");
	for (const auto& e : elist) {
	  if (n > 0) {
	    ofs << " and ";
	  }
	  auto nparts=strutils::split(e.attribute_value("name")," > ");
	  ofs << nparts[1];
	  ++n;
	}
    }
    ofs << "\"," << std::endl;
    MySQL::LocalQuery query("select s.title,s.summary,s.pub_date,d.doi from search.datasets as s left join dssdb.dsvrsn as d on d.dsid = concat('ds',s.dsid) where s.dsid = '"+dsnum+"'");
    MySQL::Row row;
    if (query.submit(server) == 0 && query.fetch_row(row)) {
	ofs << " title = \"" << row[0] << "\"," << std::endl;
	ofs << " publisher  = \"" << DATA_CENTER << "\"," << std::endl;
	ofs << " address = {Boulder CO}," << std::endl;
	ofs << " year  = \"" << row[2].substr(0,4) << "\"," << std::endl;
	if (!row[3].empty()) {
	  ofs << " url = \"" << DOI_URL_BASE << "/" << row[3] << "\"" << std::endl;
	}
	else {
	  ofs << " url = \"http://rda.ucar.edu/datasets/ds" << dsnum << "/\"" << std::endl;
	}
    }
    ofs << "}" << std::endl;
  }
}

} // end namespace citation
