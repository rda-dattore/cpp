#include <fstream>
#include <metadata_export.hpp>
#include <metadata.hpp>
#include <xml.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <MySQL.hpp>
#include <myerror.hpp>

namespace metadataExport {

bool export_to_dc_meta_tags(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  ofs << "<meta name=\"DC.type\" content=\"Dataset\" />" << std::endl;
  ofs << "<meta name=\"DC.identifier\" content=\"";
  MySQL::Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"");
  MySQL::LocalQuery query("doi","dssdb.dsvrsn","dsid = 'ds"+dsnum+"' and isnull(end_date)");
  MySQL::Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << "https://doi.org/" << row[0];
  }
  else {
    ofs << "https://rda.ucar.edu/datasets/ds" << dsnum << "/";
  }
  ofs << "\" />" << std::endl;
  auto elist=xdoc.element_list("dsOverview/author");
  if (elist.size() > 0) {
    for (const auto& author : elist) {
	ofs << "<meta name=\"DC.creator\" content=\"";
	auto author_type=author.attribute_value("xsi:type");
	if (author_type == "authorPerson" || author_type.empty()) {
	  ofs << author.attribute_value("lname") << ", " << author.attribute_value("fname");
	}
	else {
	  ofs << author.attribute_value("name");
	}
	ofs << "\" />" << std::endl;
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
    auto num_contributors=0;
    while (query.fetch_row(row)) {
	if (!row[0].empty()) {
	  auto name_parts=strutils::split(row[0]," > ");
	  if (name_parts.back() == "UNAFFILIATED INDIVIDUAL") {
	    auto contact_parts=strutils::split(row[1],",");
	    if (contact_parts.size() > 0) {
		ofs << "<meta name=\"DC.creator\" content\"" << contact_parts.front() << "\" />" << std::endl;
		++num_contributors;
	    }
	  }
	  else {
	    ofs << "<meta name=\"DC.creator\" content=\"" << strutils::substitute(name_parts.back(),", ","/") << "\" />" << std::endl;
	    ++num_contributors;
	  }
	}
    }
    if (num_contributors == 0) {
	myerror="no useable contributors were found for ds"+dsnum;
	return false;
    }
  }
  ofs << "<meta name=\"DC.title\" content=\"" << strutils::substitute(xdoc.element("dsOverview/title").content(),"\"","\\\"") << "\" />" << std::endl;
  ofs << "<meta name=\"DC.date\" content=\"" << xdoc.element("dsOverview/publicationDate").content() << "\" scheme=\"DCTERMS.W3CDTF\" />" << std::endl;
  ofs << "<meta name=\"DC.publisher\" content=\"" << PUBLISHER << "\" />" << std::endl;
  auto summary=htmlutils::convert_html_summary_to_ascii(xdoc.element("dsOverview/summary").to_string(),0x7fffffff,0);
  strutils::replace_all(summary,"\n","\\n");
  ofs << "<meta name=\"DC.description\" content=\"" << strutils::substitute(summary,"\"","\\\"") << "\" />" << std::endl;
  query.set("select g.path from search.variables_new as v left join search.GCMD_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"+dsnum+"' and v.vocabulary = 'GCMD'");
  if (query.submit(server) == 0) {
    while (query.fetch_row(row)) {
	ofs << "<meta name=\"DC.subject\" content=\"" << row[0] << "\" />" << std::endl;
    }
  }
  return true;
}

bool export_to_oai_dc(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  std::string indent(indent_length,' ');
  XMLElement e=xdoc.element("dsOverview/creator@vocabulary=GCMD");
  auto dss_centername=e.attribute_value("name");
  if (strutils::contains(dss_centername,"/SCD/")) {
    strutils::replace_all(dss_centername,"SCD","CISL");
  }
  ofs << indent << "<oai_dc:dc xmlns:oai_dc=\"http://www.openarchives.org/OAI/2.0/oai_dc/\"" << std::endl;
  ofs << indent << "           xmlns:dc=\"http://purl.org/dc/elements/1.1/\"" << std::endl;
  ofs << indent << "           xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << indent << "           xsi:schemaLocation=\"http://www.openarchives.org/OAI/2.0/oai_dc/" << std::endl;
  ofs << indent << "                               http://www.openarchives.org/OAI/2.0/oai_dc.xsd\">" << std::endl;
  e=xdoc.element("dsOverview/title");
  ofs << indent << "  <dc:title>" << e.content() << "</dc:title>" << std::endl;
  ofs << indent << "  <dc:creator>" << dss_centername << "</dc:creator>" << std::endl;
  ofs << indent << "  <dc:publisher>" << dss_centername << "</dc:publisher>" << std::endl;
  e=xdoc.element("dsOverview/topic@vocabulary=ISO");
  ofs << indent << "  <dc:subject>" << e.content() << "</dc:subject>" << std::endl;
  e=xdoc.element("dsOverview/summary");
  ofs << indent << "  <dc:description>" << std::endl;
  ofs << htmlutils::convert_html_summary_to_ascii(e.to_string(),80,indent_length+4) << std::endl;
  ofs << indent << "  </dc:description>" << std::endl;
  std::list<XMLElement> elist=xdoc.element_list("dsOverview/contributor@vocabulary=GCMD");
  for (const auto& element : elist) {
    ofs << indent << "  <dc:contributor>" << element.attribute_value("name") << "</dc:contributor>" << std::endl;
  }
  ofs << indent << "  <dc:type>Dataset</dc:type>" << std::endl;
  ofs << indent << "  <dc:identifier>ds" << dsnum << "</dc:identifier>" << std::endl;
  ofs << indent << "  <dc:language>english</dc:language>" << std::endl;
  elist=xdoc.element_list("dsOverview/relatedDataset");
  for (const auto& element : elist) {
    ofs << indent << "  <dc:relation>rda.ucar.edu:ds" << element.attribute_value("ID") << "</dc:relation>" << std::endl;
  }
  elist=xdoc.element_list("dsOverview/relatedResource");
  for (const auto& element : elist) {
    ofs << indent << "  <dc:relation>" << element.content() << " [" << element.attribute_value("url") << "]</dc:relation>" << std::endl;
  }
  e=xdoc.element("dsOverview/restrictions/access");
  if (e.name() == "access") {
    ofs << indent << "  <dc:rights>" << std::endl;
    ofs << htmlutils::convert_html_summary_to_ascii(e.to_string(),80,4) << std::endl;
    ofs << indent << "  </dc:rights>" << std::endl;
  }
  e=xdoc.element("dsOverview/restrictions/usage");
  if (e.name() == "usage") {
    ofs << indent << "  <dc:rights>" << std::endl;
    ofs << htmlutils::convert_html_summary_to_ascii(e.to_string(),80,4) << std::endl;
    ofs << indent << "  </dc:rights>" << std::endl;
  }
  ofs << indent << "</oai_dc:dc>" << std::endl;
  return true;
}

} // end namespace metadataExport
