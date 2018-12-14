#include <fstream>
#include <metadata_export.hpp>
#include <metadata.hpp>
#include <metahelpers.hpp>
#include <xml.hpp>
#include <MySQL.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <myerror.hpp>

namespace metadataExport {

bool export_to_datacite(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  MySQL::Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"");
  std::string indent(indent_length,' ');
  ofs << indent << "<resource xmlns=\"http://datacite.org/schema/kernel-3\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://datacite.org/schema/kernel-3 http://schema.datacite.org/meta/kernel-3/metadata.xsd\">" << std::endl;
  ofs << indent << "  <identifier identifierType=\"DOI\">";
  MySQL::LocalQuery query("doi","dssdb.dsvrsn","dsid = 'ds"+dsnum+"' and isnull(end_date)");
  MySQL::Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << row[0];
  }
  ofs << "</identifier>" << std::endl;
  ofs << indent << "  <creators>" << std::endl;
  auto elist=xdoc.element_list("dsOverview/author");
  if (elist.size() > 0) {
    for (const auto& author : elist) {
	ofs << indent << "    <creator>" << std::endl;
	ofs << indent << "      <creatorName>" << author.attribute_value("lname") << ", " << author.attribute_value("fname");
	auto mname=author.attribute_value("mname");
	if (!mname.empty()) {
	  ofs << " " << mname;
	}
	ofs << "</creatorName>" << std::endl;
	ofs << indent << "    </creator>" << std::endl;
    }
  }
  else {
    query.set("select g.path,c.contact from search.contributors_new as c left join search.GCMD_providers as g on g.uuid = c.keyword where c.dsid = '"+dsnum+"' and c.vocabulary = 'GCMD'");
    if (query.submit(server) < 0) {
	myerror="database error: "+server.error();
	return false;
    }
    if (query.num_rows() == 0) {
	myerror="no contributors were found for ds"+dsnum;
	return false;
    }
    auto n=0;
    while (query.fetch_row(row)) {
	auto name_parts=strutils::split(row[0]," > ");
	if (name_parts.back() == "UNAFFILIATED INDIVIDUAL") {
	  auto contact_parts=strutils::split(row[1],",");
	  if (contact_parts.size() > 0) {
	    name_parts.back()=strutils::capitalize(strutils::substitute(strutils::to_lower(name_parts.back())," ","_"))+", "+contact_parts.front();
	  }
	  else {
	    name_parts.back()="";
	  }
	}
	if (!name_parts.back().empty()) {
	  ofs << indent << "    <creator>" << std::endl;
	  ofs << indent << "      <creatorName>" << strutils::substitute(name_parts.back(),", ","/") << "</creatorName>" << std::endl;
	  ofs << indent << "    </creator>" << std::endl;
	  ++n;
	}
    }
    if (n == 0) {
	myerror="no useable contributors were found for ds"+dsnum;
	return false;
    }
  }
  ofs << indent << "  </creators>" << std::endl;
  ofs << indent << "  <titles>" << std::endl;
  auto e=xdoc.element("dsOverview/title");
  ofs << indent << "    <title>" << e.content() << "</title>" << std::endl;
  ofs << indent << "  </titles>" << std::endl;
  ofs << indent << "  <publisher>" << PUBLISHER << "</publisher>" << std::endl;
  e=xdoc.element("dsOverview/publicationDate");
  ofs << indent << "  <publicationYear>" << e.content().substr(0,4) << "</publicationYear>" << std::endl;
  ofs << indent << "  <subjects>" << std::endl;
  query.set("select g.path from search.variables_new as v left join search.GCMD_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"+dsnum+"' and v.vocabulary = 'GCMD'");
  if (query.submit(server) == 0) {
    while (query.fetch_row(row)) {
	ofs << indent << "    <subject subjectScheme=\"GCMD\">" << row[0] << "</subject>" << std::endl;
    }
  }
  ofs << indent << "  </subjects>" << std::endl;
  ofs << indent << "  <contributors>" << std::endl;
  ofs << indent << "    <contributor contributorType=\"HostingInstitution\">" << std::endl;
  ofs << indent << "      <contributorName>" << DATACITE_HOSTING_INSTITUTION << "</contributorName>" << std::endl;
  ofs << indent << "    </contributor>" << std::endl;
  ofs << indent << "  </contributors>" << std::endl;
  query.set("select min(concat(date_start,' ',time_start)),min(start_flag),max(concat(date_end,' ',time_end)),min(end_flag),time_zone from dssdb.dsperiod where dsid = 'ds"+dsnum+"' and date_start > '0000-00-00' and date_start < '3000-01-01' and date_end > '0000-00-00' and date_end < '3000-01-01'");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << indent << "  <dates>" << std::endl;
    ofs << indent << "    <date dateType=\"Valid\">" << metatranslations::date_time(row[0],row[1],row[4],"T") << " to " << metatranslations::date_time(row[2],row[3],row[4],"T") << "</date>" << std::endl;
    ofs << indent << "  </dates>" << std::endl;
  }
  ofs << indent << "  <language>eng</language>" << std::endl;
  ofs << indent << "  <resourceType resourceTypeGeneral=\"Dataset\" />" << std::endl;
  ofs << indent << "  <alternateIdentifiers>" << std::endl;
  ofs << indent << "    <alternateIdentifier alternateIdentifierType=\"Local\">ds" << dsnum << "</alternateIdentifier>" << std::endl;
  ofs << indent << "  </alternateIdentifiers>" << std::endl;
  elist=xdoc.element_list("dsOverview/relatedDOI");
  if (elist.size() > 0) {
    ofs << indent << "  <relatedIdentifiers>" << std::endl;
    for (const auto& doi : elist) {
	ofs << indent << "    <relatedIdentifier relatedIdentifierType=\"DOI\" relationType=\"" << doi.attribute_value("relationType") << "\">" << doi.content() << "</relatedIdentifier>" << std::endl;
    }
    ofs << indent << "  </relatedIdentifiers>" << std::endl;
  }
  auto size=primary_size(dsnum,server);
  if (!size.empty()) {
    ofs << indent << "  <sizes>" << std::endl;
    ofs << indent << "    <size>" << size << "</size>" << std::endl;
    ofs << indent << "  </sizes>" << std::endl;
  }
  query.set("distinct keyword","search.formats","dsid = '"+dsnum+"'");
  if (query.submit(server) == 0 && query.num_rows() > 0) {
    ofs << indent << "  <formats>" << std::endl;
    while (query.fetch_row(row)) {
	ofs << indent << "    <format>" << strutils::substitute(row[0],"_"," ") << "</format>" << std::endl;
    }
    ofs << indent << "  </formats>" << std::endl;
  }
  ofs << indent << "  <descriptions>" << std::endl;
  ofs << indent << "    <description descriptionType=\"Abstract\">" << std::endl;
  ofs << htmlutils::convert_html_summary_to_ascii(xdoc.element("dsOverview/summary").to_string(),120,indent_length+6) << std::endl;
  ofs << indent << "    </description>" << std::endl;
  ofs << indent << "  </descriptions>" << std::endl;
  ofs << indent << "</resource>" << std::endl;
  server.disconnect();
  return true;
}

} // end namespace metadataExport
