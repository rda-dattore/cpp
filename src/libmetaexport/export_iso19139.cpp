#include <fstream>
#include <regex>
#include <metadata_export.hpp>
#include <metadata.hpp>
#include <xml.hpp>
#include <MySQL.hpp>
#include <strutils.hpp>
#include <utils.hpp>

namespace metadataExport {

bool export_to_iso19139(std::unique_ptr<TokenDocument>& token_doc,std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  if (token_doc == nullptr) {
    token_doc.reset(new TokenDocument("/usr/local/www/server_root/web/html/oai/iso19139.xml",indent_length));
  }
  MySQL::Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"");
  token_doc->add_replacement("__DSNUM__",dsnum);
  XMLElement e=xdoc.element("dsOverview/timeStamp");
  auto mdate=e.attribute_value("value").substr(0,10);
  MySQL::LocalQuery query("mssdate","dssdb.dataset","dsid = 'ds"+dsnum+"'");
  MySQL::Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    if (row[0] > mdate) {
	mdate=row[0]+"T00:00:00";
    }
    else {
	mdate="";
    }
  }
  if (mdate.empty()) {
    mdate=e.attribute_value("value");
    mdate[10]='T';
    mdate.erase(19);
  }
  token_doc->add_replacement("__MDATE__",mdate);
  e=xdoc.element("dsOverview/title");
  token_doc->add_replacement("__TITLE__",e.content());
  e=xdoc.element("dsOverview/publicationDate");
  auto pub_date=e.content();
  if (pub_date.empty()) {
    query.set("pub_date","search.datasets","dsid = '"+dsnum+"'");
    if (query.submit(server) == 0 && query.fetch_row(row)) {
	pub_date=row[0];
    }
  }
  if (!pub_date.empty()) {
    token_doc->add_replacement("__PUBLICATION_DATE_XML__","<gmd:date><gco:Date>"+pub_date+"</gco:Date></gmd:date>");
  }
  else {
    token_doc->add_replacement("__PUBLICATION_DATE_XML__","<gmd:date gco:nilReason=\"unknown\" />");
  }
  query.set("doi","dssdb.dsvrsn","dsid = 'ds"+dsnum+"' and isnull(end_date)");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    token_doc->add_if("__HAS_DOI__");
    token_doc->add_replacement("__DOI__",row[0]);
  }
  std::list<XMLElement> elist=xdoc.element_list("dsOverview/author");
  if (elist.size() > 0) {
    token_doc->add_if("__HAS_AUTHOR_PERSONS__");
    for (const auto& e : elist) {
	std::string author;
	auto author_type=e.attribute_value("xsi:type");
	if (author_type == "authorPerson" || author_type.empty()) {
	  author=e.attribute_value("lname")+", "+e.attribute_value("fname")+" "+e.attribute_value("mname");
	}
	else {
	  author=e.attribute_value("name");
	}
	strutils::trim(author);
	token_doc->add_repeat("__AUTHOR_PERSON__",author);
    }
  }
  else {
    query.set("select g.path from search.contributors_new as c left join search.gcmd_providers as g on g.uuid = c.keyword where c.dsid = '"+dsnum+"' and c.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
	token_doc->add_if("__HAS_AUTHOR_ORGS__");
	for (const auto& row : query) {
	  token_doc->add_repeat("__AUTHOR_ORG__",strutils::substitute(row[0].substr(row[0].find(" > ")+3),"&","&amp;"));
	}
    }
  }
  e=xdoc.element("dsOverview/summary");
  token_doc->add_replacement("__ABSTRACT__",htmlutils::convert_html_summary_to_ascii(e.to_string(),32768,0));
  e=xdoc.element("dsOverview/continuingUpdate");
  std::string frequency;
  if (e.attribute_value("value") == "yes") {
    token_doc->add_replacement("__PROGRESS_STATUS__","onGoing");
    frequency=e.attribute_value("frequency");
    if (frequency == "bi-monthly") {
	frequency="monthly";
    }
    else if (frequency == "half-yearly") {
	frequency="biannually";
    }
    else if (frequency == "yearly") {
	frequency="annually";
    }
    else if (frequency == "irregularly") {
	frequency="irregular";
    }
  }
  else {
    token_doc->add_replacement("__PROGRESS_STATUS__","completed");
    frequency="notPlanned";
  }
  token_doc->add_replacement("__UPDATE_FREQUENCY__",frequency);
  e=xdoc.element("dsOverview/logo");
  if (!e.name().empty()) {
    token_doc->add_if("__HAS_LOGO__");
    token_doc->add_replacement("__LOGO_URL__","https://rda.ucar.edu/images/ds_logos/"+e.content());
    token_doc->add_replacement("__LOGO_IMAGE_FORMAT__",strutils::to_upper(e.content().substr(e.content().rfind(".")+1)));
  }
  elist=xdoc.element_list("dsOverview/contact");
  token_doc->add_replacement("__SPECIALIST_NAME__",elist.front().content());
  std::deque<std::string> sp=strutils::split(elist.front().content());
  query.set("logname,phoneno","dssdb.dssgrp","fstname = '"+sp[0]+"' and lstname = '"+sp[1]+"'");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    token_doc->add_replacement("__SPECIALIST_PHONE__",row[1]);
    token_doc->add_replacement("__SPECIALIST_EMAIL__",row[0]+"@ucar.edu");
  }
  query.set("keyword","search.formats","dsid = '"+dsnum+"'");
  if (query.submit(server) == 0 && query.num_rows() > 0) {
    token_doc->add_if("__HAS_FORMATS__");
    for (const auto& row : query) {
	token_doc->add_repeat("__FORMAT__",strutils::to_capital(strutils::substitute(row[0],"proprietary_","")));
    }
  }
  query.set("select concept_scheme,version,revision_date from search.gcmd_versions");
  if (query.submit(server) == 0) {
    for (const auto& row : query) {
	auto concept_scheme=strutils::to_upper(row[0]);
	token_doc->add_replacement("__"+concept_scheme+"_VERSION__",row[1]);
	token_doc->add_replacement("__"+concept_scheme+"_REVISION_DATE__",row[2]);
    }
    query.set("select g.path from search.platforms_new as p left join search.gcmd_platforms as g on g.uuid = p.keyword where p.dsid = '"+dsnum+"' and p.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
	token_doc->add_if("__HAS_PLATFORMS__");
	for (const auto& row : query) {
	  token_doc->add_repeat("__PLATFORM__",strutils::substitute(row[0],"&","&amp;"));
	}
    }
    query.set("select g.path from search.instruments as i left join search.gcmd_instruments as g on g.uuid = i.keyword where i.dsid = '"+dsnum+"' and i.vocabulary = 'GCMD'");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
	token_doc->add_if("__HAS_INSTRUMENTS__");
	for (const auto& row : query) {
	  token_doc->add_repeat("__INSTRUMENT__",row[0]);
	}
    }
//    query.set("select g.path from search.projects_new as p left join search.gcmd_projects as g on g.uuid = p.keyword where p.dsid = '"+dsnum+"' and p.vocabulary = 'GCMD'");
query.set("select distinct g.path from (select keyword from search.projects_new where dsid = '"+dsnum+"' and vocabulary = 'GCMD' union select keyword from search.supported_projects where dsid = '"+dsnum+"' and vocabulary = 'GCMD') as p left join search.gcmd_projects as g on g.uuid = p.keyword");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
	token_doc->add_if("__HAS_PROJECTS__");
	for (const auto& row : query) {
	  token_doc->add_repeat("__PROJECT__",row[0]);
	}
    }
    query.set("select g.path from search.variables as v left join search.gcmd_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"+dsnum+"' and v.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
	for (const auto& row : query) {
	  token_doc->add_repeat("__SCIENCEKEYWORD__",row[0]);
	}
    }
  }
  e=xdoc.element("dsOverview/topic@vocabulary=ISO");
  token_doc->add_replacement("__ISO_TOPIC__",e.content());
  double min_west_lon,min_south_lat,max_east_lon,max_north_lat;
  bool is_grid;
  std::vector<HorizontalResolutionEntry> hres_list;
  my::map<Entry> unique_places_table;
  fill_geographic_extent_data(server,dsnum,xdoc,min_west_lon,min_south_lat,max_east_lon,max_north_lat,is_grid,&hres_list,&unique_places_table);
  for (const auto& hres : hres_list) {
    token_doc->add_repeat("__SPATIAL_RESOLUTION__","UOM[!]"+hres.uom+"<!>RESOLUTION[!]"+strutils::dtos(hres.resolution,4));
  }
  if (is_grid) {
    token_doc->add_if("__IS_GRID__");
  }
  auto has_any_extent=false;
  if (min_west_lon < 9999.) {
    if (floatutils::myequalf(min_west_lon,max_east_lon) && floatutils::myequalf(min_south_lat,max_north_lat)) {
	token_doc->add_if("__HAS_POINT__");
	token_doc->add_replacement("__LAT__",strutils::ftos(min_south_lat,4));
	token_doc->add_replacement("__LON__",strutils::ftos(min_west_lon,4));
    }
    else {
	token_doc->add_if("__HAS_BOUNDING_BOX__");
	token_doc->add_replacement("__WEST_LON__",strutils::ftos(min_west_lon,4));
	token_doc->add_replacement("__EAST_LON__",strutils::ftos(max_east_lon,4));
	token_doc->add_replacement("__SOUTH_LAT__",strutils::ftos(min_south_lat,4));
	token_doc->add_replacement("__NORTH_LAT__",strutils::ftos(max_north_lat,4));
    }
    has_any_extent=true;
  }
  if (unique_places_table.size() > 0) {
    token_doc->add_if("__HAS_GEOGRAPHIC_IDENTIFIERS__");
    for (const auto& key : unique_places_table.keys()) {
	token_doc->add_repeat("__GEOGRAPHIC_IDENTIFIER__",key);
    }
    has_any_extent=true;
  }
  query.set("select min(date_start),min(time_start),max(date_end),max(time_end),min(start_flag),any_value(time_zone) from dssdb.dsperiod where dsid = 'ds"+dsnum+"' and date_start < '9998-01-01' and date_end < '9998-01-01' group by dsid");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    size_t max_parts=std::stoi(row[4]);
    auto parts=strutils::split(row[0],"-");
    std::string s;
    size_t m= (max_parts < 3) ? max_parts : 3;
    for (size_t n=0; n < m; ++n) {
	if (n > 0) {
	  s+="-";
	}
	s+=parts[n];
    }
    if (max_parts > 3) {
	s+="T";
	if (std::regex_search(row[1],std::regex("^31:"))) {
	  parts=strutils::split("00:00:00",":");
	}
	else {
	  parts=strutils::split(row[1],":");
	}
	auto m=max_parts-3;
	for (size_t n=0; n < 3; ++n) {
	  if (n > 0) {
	    s+=":";
	  }
	  if (n < m) {
	    s+=parts[n];
	  }
	  else {
	    s+="00";
	  }
	}
	if (row[5] == "+0000") {
	  s+="Z";
	}
	else {
	  s+=row[5].substr(0,3)+":"+row[5].substr(3);
	}
    }
    token_doc->add_replacement("__BEGIN_DATE__",s);
    parts=strutils::split(row[2],"-");
    s="";
    m= (max_parts < 3) ? max_parts : 3;
    for (size_t n=0; n < m; ++n) {
	if (n > 0) {
	  s+="-";
	}
	s+=parts[n];
    }
    if (max_parts > 3) {
	s+="T";
	if (std::regex_search(row[3],std::regex("^31:"))) {
	  parts=strutils::split("23:59:59",":");
	}
	else {
	  parts=strutils::split(row[3],":");
	}
	auto m=max_parts-3;
	for (size_t n=0; n < 3; ++n) {
	  if (n > 0) {
	    s+=":";
	  }
	  if (n < m) {
	    s+=parts[n];
	  }
	  else {
	    s+="00";
	  }
	}
	if (row[5] == "+0000") {
	  s+="Z";
	}
	else {
	  s+=row[5].substr(0,3)+":"+row[5].substr(3);
	}
    }
    token_doc->add_replacement("__END_DATE__",s);
    token_doc->add_if("__HAS_TEMPORAL_EXTENT__");
    has_any_extent=true;
  }
  if (has_any_extent) {
    token_doc->add_if("__HAS_ANY_EXTENT__");
  }
  e=xdoc.element("dsOverview/restrictions/usage");
  if (!e.name().empty()) {
    token_doc->add_if("__HAS_USAGE_RESTRICTIONS__");
    token_doc->add_replacement("__USAGE_RESTRICTIONS__",htmlutils::convert_html_summary_to_ascii(e.to_string(),32768,0));
  }
  e=xdoc.element("dsOverview/restrictions/access");
  if (!e.name().empty()) {
    token_doc->add_if("__HAS_ACCESS_RESTRICTIONS__");
    token_doc->add_replacement("__ACCESS_RESTRICTIONS__",htmlutils::convert_html_summary_to_ascii(e.to_string(),32768,0));
  }
  query.set("select if(primary_size > 999999999,round(primary_size/1000000,0),if(primary_size > 999999,round(primary_size/1000000,2),if(primary_size > 9999,truncate(round(primary_size/10000,0)/100,2),truncate(round(primary_size/100,0)/10000,2)))) from dssdb.dataset where dsid = 'ds"+dsnum+"'");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    token_doc->add_if("__VOLUME_SPECIFIED__");
    token_doc->add_replacement("__MB_VOLUME__",row[0]);
  }
  elist=xdoc.element_list("dsOverview/relatedResource");
  auto n=1;
  for (const auto& e : elist) {
    auto url=e.attribute_value("url");
    token_doc->add_repeat("__RELATED_RESOURCE__","URL[!]"+url+"<!>PROTOCOL[!]"+url.substr(0,url.find("://"))+"<!>NAME[!]Related Resource #"+strutils::itos(n++)+"<!>DESCRIPTION[!]"+e.content());
  }
  ofs << *token_doc << std::endl;
  return true;
}

} // end namespace metadataExport
