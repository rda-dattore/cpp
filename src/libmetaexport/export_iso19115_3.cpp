#include <fstream>
#include <metadata_export.hpp>
#include <metadata.hpp>
#include <xml.hpp>
#include <MySQL.hpp>
#include <strutils.hpp>

namespace metadataExport {

bool export_to_iso19115_3(std::unique_ptr<TokenDocument>& token_doc,std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  if (token_doc == nullptr) {
    token_doc.reset(new TokenDocument("/usr/local/www/server_root/web/html/oai/iso19115-3.xml"));
  }
  XMLElement e=xdoc.element("dsOverview/timeStamp");
  auto mdate=e.attribute_value("value").substr(0,10);
  MySQL::Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"");
  MySQL::LocalQuery query;
  query.set("mssdate","dssdb.dataset","dsid = 'ds"+dsnum+"'");
  MySQL::Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    if (row[0] > mdate) {
	mdate=row[0];
    }
  }
  token_doc->add_replacement("__MDATE__",mdate);
  token_doc->add_replacement("__DSNUM__",dsnum);
  add_replace_from_element_content(xdoc,"dsOverview/title","__TITLE__",*token_doc);
  e=xdoc.element("dsOverview/publicationDate");
  auto pub_date=e.content();
  if (pub_date.empty()) {
    query.set("pub_date","search.datasets","dsid = '"+dsnum+"'");
    if (query.submit(server) == 0 && query.fetch_row(row)) {
	pub_date=row[0];
    }
  }
  if (!pub_date.empty()) {
    token_doc->add_if("__HAS_PUBLICATION_DATE__");
    token_doc->add_replacement("__PUBLICATION_DATE__",pub_date);
  }
  query.set("doi","dssdb.dsvrsn","dsid = 'ds"+dsnum+"' and isnull(end_date)");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    token_doc->add_if("__HAS_DOI__");
    token_doc->add_replacement("__DOI__",row[0]);
  }
  auto elist=xdoc.element_list("dsOverview/author");
  if (elist.size() > 0) {
// dataset has people and/or corporate authors, which overrides contributors
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
// dataset does not have specific authors, so use contributors
    query.set("select g.last_in_path from search.contributors_new as c left join search.GCMD_providers as g on g.uuid = c.keyword where c.dsid = '"+dsnum+"' and c.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
	token_doc->add_if("__HAS_AUTHOR_ORGS__");
	for (const auto& row : query) {
	  token_doc->add_repeat("__AUTHOR_ORG__",row[0]);
	}
    }
  }
  add_replace_from_element_content(xdoc,"dsOverview/summary","__ABSTRACT__",*token_doc);
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
  elist=xdoc.element_list("dsOverview/contact");
  token_doc->add_replacement("__SPECIALIST_NAME__",elist.front().content());
  auto specparts=strutils::split(elist.front().content());
  query.set("logname,phoneno,officeno","dssdb.dssgrp","fstname = '"+specparts[0]+"' and lstname = '"+specparts[1]+"'");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    token_doc->add_replacement("__SPECIALIST_PHONE__",row[1]);
    token_doc->add_replacement("__SPECIALIST_OFFICE__","ML-"+row[2]);
    token_doc->add_replacement("__SPECIALIST_EMAIL__",row[0]+"@ucar.edu");
  }
  double min_west_lon,min_south_lat,max_east_lon,max_north_lat;
  bool is_grid=false;
  std::vector<HorizontalResolutionEntry> hres_list;
  my::map<Entry> unique_places_table;
  fill_geographic_extent_data(server,dsnum,xdoc,min_west_lon,min_south_lat,max_east_lon,max_north_lat,is_grid,&hres_list,&unique_places_table);
  if (is_grid) {
    token_doc->add_if("__IS_GRID__");
  }
  if (!hres_list.empty()) {
    token_doc->add_if("__HAS_SPATIAL_RESOLUTION__");
    for (const auto& h : hres_list) {
	token_doc->add_repeat("__SPATIAL_RESOLUTION__","UOM[!]"+h.uom+"<!>RES[!]"+strutils::dtos(h.resolution,10));
    }
  }
  auto has_any_extent=false;
  if (min_west_lon < 9999.) {
    token_doc->add_if("__HAS_BOUNDING_BOX__");
    token_doc->add_replacement("__WEST_LON__",strutils::dtos(min_west_lon,10));
    token_doc->add_replacement("__EAST_LON__",strutils::dtos(max_east_lon,10));
    token_doc->add_replacement("__SOUTH_LAT__",strutils::dtos(min_south_lat,10));
    token_doc->add_replacement("__NORTH_LAT__",strutils::dtos(max_north_lat,10));
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
    size_t m= (max_parts < 3) ? max_parts : 3;
    for (size_t l=0; l < 2; ++l) {
	auto parts=strutils::split(row[l*2],"-");
	std::string s;
	for (size_t n=0; n < m; ++n) {
	  if (n > 0) {
	    s+="-";
	  }
	  s+=parts[n];
	}
	if (max_parts > 3) {
	  s+="T";
	  parts=strutils::split(row[l*2+1],":");
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
	if (l == 0) {
	  token_doc->add_replacement("__BEGIN_DATE_TIME__",s);
	  token_doc->add_if("__HAS_TEMPORAL_EXTENT__");
	  has_any_extent=true;
	}
	else {
	  token_doc->add_replacement("__END_DATE_TIME__",s);
	}
    }
  }
  if (has_any_extent) {
    token_doc->add_if("__HAS_ANY_EXTENT__");
  }
  elist=xdoc.element_list("dsOverview/reference@type=journal");
  if (elist.size() > 0) {
    token_doc->add_if("__HAS_PUBLICATIONS__");
    for (const auto& e : elist) {
	auto alist=strutils::split(e.element("authorList").content(),",");
	auto p=e.element("periodical");
	auto rval="TITLE[!]"+e.element("title").content()+"<!>YEAR[!]"+e.element("year").content()+"<!>AUTHOR[!]"+alist[0];
	if (alist.size() > 1) {
	  rval+=", "+alist[1];
	}
	rval+="<!>NAME[!]"+p.content()+"<!>ISSUE[!]"+p.attribute_value("number")+"<!>PAGES[!]"+p.attribute_value("pages");
	for (size_t n=2; n < alist.size(); ++n) {
	  rval+="<!>COAUTHOR[!]"+alist[n];
	}
	token_doc->add_repeat("__PUBLICATION__",rval);
    }
  }
  e=xdoc.element("dsOverview/logo");
  if (e.name().length() > 0) {
    token_doc->add_if("__HAS_LOGO__");
    token_doc->add_replacement("__LOGO_URL__","https://rda.ucar.edu/images/ds_logos/"+e.content());
    token_doc->add_replacement("__LOGO_IMAGE_FORMAT__",strutils::to_upper(e.content().substr(e.content().rfind(".")+1)));
  }
  query.set("keyword","search.formats","dsid = '"+dsnum+"'");
  if (query.submit(server) == 0 && query.num_rows() > 0) {
    token_doc->add_if("__HAS_FORMATS__");
    while (query.fetch_row(row)) {
	token_doc->add_repeat("__FORMAT__",strutils::to_capital(strutils::substitute(row[0],"proprietary_","")));
    }
  }
  query.set("select concept_scheme,version,revision_date from search.GCMD_versions");
  if (query.submit(server) == 0) {
    while (query.fetch_row(row)) {
	auto concept_scheme=strutils::to_upper(row[0]);
	token_doc->add_replacement("__"+concept_scheme+"_VERSION__",row[1]);
//	token_doc->add_replacement("__"+concept_scheme+"_REVISION_DATE__",row[2]);
    }
    query.set("select g.path from search.variables_new as v left join search.GCMD_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"+dsnum+"' and v.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
	while (query.fetch_row(row)) {
	  token_doc->add_repeat("__SCIENCEKEYWORD__",row[0]);
	}
    }
    query.set("select g.path from search.platforms_new as p left join search.GCMD_platforms as g on g.uuid = p.keyword where p.dsid = '"+dsnum+"' and p.vocabulary = 'GCMD'");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
	token_doc->add_if("__HAS_PLATFORMS__");
	while (query.fetch_row(row)) {
	  token_doc->add_repeat("__PLATFORM__",row[0]);
	}
    }
    query.set("select distinct g.path from (select keyword from search.projects_new where dsid = '"+dsnum+"' and vocabulary = 'GCMD' union select keyword from search.supportedProjects_new where dsid = '"+dsnum+"' and vocabulary = 'GCMD') as p left join search.GCMD_projects as g on g.uuid = p.keyword");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
	token_doc->add_if("__HAS_PROJECTS__");
	while (query.fetch_row(row)) {
	  token_doc->add_repeat("__PROJECT__",row[0]);
	}
    }
    query.set("select g.path from search.instruments_new as i left join search.GCMD_instruments as g on g.uuid = i.keyword where i.dsid = '"+dsnum+"' and i.vocabulary = 'GCMD'");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
	token_doc->add_if("__HAS_INSTRUMENTS__");
	while (query.fetch_row(row)) {
	  token_doc->add_repeat("__INSTRUMENT__",row[0]);
	}
    }
  }
  add_replace_from_element_content(xdoc,"dsOverview/restrictions/access","__ACCESS_RESTRICTIONS__",*token_doc,"__HAS_ACCESS_RESTRICTIONS__");
  add_replace_from_element_content(xdoc,"dsOverview/restrictions/usage","__USAGE_RESTRICTIONS__",*token_doc,"__HAS_USAGE_RESTRICTIONS__");
  elist=xdoc.element_list("dsOverview/relatedResource");
  if (elist.size() > 0) {
    token_doc->add_if("__HAS_RELATED_RESOURCES__");
    for (const auto& e : elist) {
	token_doc->add_repeat("__RELATED_RESOURCE__","TITLE[!]"+e.content()+"<!>URL[!]"+e.attribute_value("url"));
    }
  }
  elist=xdoc.element_list("dsOverview/relatedDataset");
  if (elist.size() > 0) {
    token_doc->add_if("__HAS_RELATED_RESOURCES__");
    for (const auto& e : elist) {
	auto dsid=e.attribute_value("ID");
	query.set("title","search.datasets","dsid = '"+dsid+"'");
	if (query.submit(server) == 0 && query.fetch_row(row)) {
	  token_doc->add_repeat("__RELATED_RESOURCE__","TITLE[!]"+row[0]+"<!>URL[!]https://rda.ucar.edu/datasets/ds"+dsid+"/");
	}
    }
  }
  server.disconnect();
  ofs << *token_doc << std::endl;
  return true;
}

} // end namespace metadataExport
