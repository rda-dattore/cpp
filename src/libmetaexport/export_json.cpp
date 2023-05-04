#include <fstream>
#include <regex>
#include <metadata_export.hpp>
#include <metadata.hpp>
#include <xml.hpp>
#include <MySQL.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <myerror.hpp>

namespace metadataExport {

bool export_to_json_ld(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  ofs << "<script type=\"application/ld+json\">" << std::endl;
  ofs << "  {" << std::endl;
  ofs << "    \"@context\": \"http://schema.org\"," << std::endl;
  ofs << "    \"@type\": \"Dataset\"," << std::endl;
  ofs << "    \"@id\": \"";
  MySQL::Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"");
  MySQL::LocalQuery query("doi","dssdb.dsvrsn","dsid = 'ds"+dsnum+"' and isnull(end_date)");
  MySQL::Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << "https://doi.org/" << row[0];
  }
  else {
    ofs << "https://rda.ucar.edu/datasets/ds" << dsnum << "/";
  }
  ofs << "\"," << std::endl;
  ofs << "    \"name\": \"" << strutils::substitute(xdoc.element("dsOverview/title").content(),"\"","\\\"") << "\"," << std::endl;
  auto summary=htmlutils::convert_html_summary_to_ascii(xdoc.element("dsOverview/summary").to_string(),0x7fffffff,0);
  strutils::replace_all(summary,"\n","\\n");
  ofs << "    \"description\": \"" << strutils::substitute(summary,"\"","\\\"") << "\"," << std::endl;
  auto elist=xdoc.element_list("dsOverview/author");
  ofs << "    \"author\": {" << std::endl;
  if (elist.size() > 0) {
    std::string local_indent="";
    if (elist.size() > 1) {
	ofs << "      \"@list\": [" << std::endl;
	local_indent="    ";
    }
    for (const auto& author : elist) {
	if (elist.size() > 1) {
	  ofs << "        " << "{" << std::endl;
	}
	auto author_type=author.attribute_value("xsi:type");
	if (author_type == "authorPerson" || author_type.empty()) {
	  ofs << "      " << local_indent << "\"@type\": \"Person\"," << std::endl;
	  ofs << "      " << local_indent << "\"givenName\": \"" << author.attribute_value("fname") << "\"," << std::endl;
	  ofs << "      " << local_indent << "\"familyName\": \"" << author.attribute_value("lname") << "\"" << std::endl;
	}
	else {
	  ofs << "      " << local_indent << "\"@type\": \"Organization\"," << std::endl;
	  ofs << "      " << local_indent << "\"name\": \"" << author.attribute_value("name") << "\"" << std::endl;
	}
	ofs << "    " << local_indent << "}";
	if (elist.size() > 1) {
	  if (&author != &elist.back()) {
	    ofs << ",";
	  }
	  ofs << std::endl;
	}
    }
    if (elist.size() > 1) {
	ofs << "      ]" << std::endl;
	ofs << "    }";
    }
    ofs << "," << std::endl;
  }
  else {
    query.set("select g.path,c.contact from search.contributors_new as c left join search.gcmd_providers as g on g.uuid = c.keyword where c.dsid = '"+dsnum+"' and c.vocabulary = 'GCMD'");
    if (query.submit(server) < 0) {
	myerror="database error: "+server.error();
	return false;
    }
    if (query.num_rows() == 0) {
	myerror="no contributors were found for ds"+dsnum;
	return false;
    }
    std::string local_indent="";
    if (query.num_rows() > 1) {
	ofs << "      \"@list\": [" << std::endl;
	local_indent="    ";
    }
    size_t num_contributors=0,num_rows=1;
    while (query.fetch_row(row)) {
	if (query.num_rows() > 1) {
	  ofs << "        " << "{" << std::endl;
	}
	auto name_parts=strutils::split(row[0]," > ");
	if (name_parts.back() == "UNAFFILIATED INDIVIDUAL") {
	  auto contact_parts=strutils::split(row[1],",");
	  if (contact_parts.size() > 0) {
	    ofs << "      " << local_indent << "\"@type\": \"Person\"," << std::endl;
	    ofs << "      " << local_indent << "\"name\": \"" << contact_parts.front() << "\"" << std::endl;
	    ofs << "    " << local_indent << "}";
	    if (query.num_rows() > 1) {
		if (num_rows < query.num_rows()) {
		  ofs << ",";
		}
		ofs << std::endl;
	    }
	    ++num_contributors;
	  }
	}
	else {
	  ofs << "      " << local_indent << "\"@type\": \"Organization\"," << std::endl;
	  ofs << "      " << local_indent << "\"name\": \"" << strutils::substitute(name_parts.back(),", ","/") << "\"" << std::endl;
	  ofs << "    " << local_indent << "}";
	  if (query.num_rows() > 1) {
	    if (num_rows < query.num_rows()) {
		ofs << ",";
	    }
	    ofs << std::endl;
	  }
	  ++num_contributors;
	}
	++num_rows;
    }
    if (num_contributors == 0) {
	myerror="no useable contributors were found for ds"+dsnum;
	return false;
    }
    if (query.num_rows() > 1) {
	ofs << "      ]" << std::endl;
	ofs << "    }";
    }
    ofs << "," << std::endl;
  }
  query.set("select g.path from search.variables as v left join search.gcmd_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"+dsnum+"' and v.vocabulary = 'GCMD'");
  if (query.submit(server) == 0) {
    ofs << "    \"keywords\": ";
    if (query.num_rows() == 1) {
	query.fetch_row(row);
	ofs << "\"" << row[0] << "\"," << std::endl;
    }
    else {
	ofs << "[" << std::endl;;
	size_t n=1;
	while (query.fetch_row(row)) {
	  ofs << "      \"" << row[0] << "\"";
	  if (n < query.num_rows()) {
	    ofs << ",";
	  }
	  ofs << std::endl;
	  ++n;
	}
 	ofs << "    ]," << std::endl;
    }
  }
  query.set("select min(date_start),min(time_start),max(date_end),max(time_end),min(start_flag),any_value(time_zone) from dssdb.dsperiod where dsid = 'ds"+dsnum+"' and date_start < '9998-01-01' and date_end < '9998-01-01' group by dsid");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    auto num_parts=std::stoi(row[4]);
    auto sdate=row[0];
    auto edate=row[2];
    if (num_parts < 3) {
	strutils::chop(sdate,3*(3-num_parts));
	strutils::chop(edate,3*(3-num_parts));
    }
    else {
	sdate+="T"+row[1];
	edate+="T"+row[3];
	if (num_parts < 6) {
	  strutils::chop(sdate,3*(6-num_parts));
	  strutils::chop(edate,3*(6-num_parts));
	}
    }
    ofs << "    \"temporalCoverage\": \"" << sdate << "/" << edate << "\"," << std::endl;
  }
  double min_west_lon,min_south_lat,max_east_lon,max_north_lat;
  bool is_grid;
  my::map<Entry> unique_places_table;
  fill_geographic_extent_data(server,dsnum,xdoc,min_west_lon,min_south_lat,max_east_lon,max_north_lat,is_grid,nullptr,&unique_places_table);
  if (is_grid) {
    ofs << "    \"spatialCoverage\": {" << std::endl;
    ofs << "      \"@type\": \"Place\"," << std::endl;
    ofs << "      \"geo\": {" << std::endl;
    if (min_west_lon < 9999.) {
	if (floatutils::myequalf(min_west_lon,max_east_lon) && floatutils::myequalf(min_south_lat,max_north_lat)) {
	  ofs << "        \"@type\": \"GeoCoordinates\"," << std::endl;
	  ofs << "        \"latitude\": " << min_south_lat << std::endl;
	  ofs << "        \"longitude\": " << min_west_lon << std::endl;
	}
	else {
	  ofs << "        \"@type\": \"GeoShape\"," << std::endl;
	  ofs << "        \"box\": \"" << min_south_lat << "," << min_west_lon << " " << max_north_lat << "," << max_east_lon << "\"" << std::endl;
	}
    }
    ofs << "      }" << std::endl;
    ofs << "    }," << std::endl;
  }
  else if (unique_places_table.size() > 0) {
/*
    ife.key="__HAS_GEOGRAPHIC_IDENTIFIERS__";
    tokens.ifs.insert(ife);
    re.key="__GEOGRAPHIC_IDENTIFIER__";
    re.list.reset(new std::deque<std::string>);
    tokens.repeats.insert(re);
    for (const auto& key : unique_places_table.keys()) {
	re.list->emplace_back(key);
    }
*/
  }
  ofs << "    \"publisher\": {" << std::endl;
  ofs << "      \"@type\": \"Organization\"," << std::endl;
  ofs << "      \"name\": \"" << PUBLISHER << "\"" << std::endl;
  ofs << "    }," << std::endl;
  ofs << "    \"datePublished\": \"" << xdoc.element("dsOverview/publicationDate").content() << "\"" << std::endl;
  ofs << "  }" << std::endl;
  ofs << "</script>" << std::endl;
  return true;
}

} // end namespace metadataExport
