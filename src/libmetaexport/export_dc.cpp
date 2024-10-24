#include <fstream>
#include <metadata_export_pg.hpp>
#include <metadata.hpp>
#include <xml.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <PostgreSQL.hpp>
#include <myerror.hpp>

using namespace PostgreSQL;
using std::endl;
using std::string;
using strutils::ds_aliases;
using strutils::ng_gdex_id;
using strutils::replace_all;
using strutils::split;
using strutils::substitute;
using strutils::to_sql_tuple_string;

namespace metadataExport {

bool export_to_dc_meta_tags(std::ostream& ofs, string dsid, XMLDocument& xdoc,
    size_t indent_length) {
  ofs << "<meta name=\"DC.type\" content=\"Dataset\" />" << endl;
  ofs << "<meta name=\"DC.identifier\" content=\"";
  Server server(metautils::directives.database_server, metautils::directives.
      metadb_username, metautils::directives.metadb_password, "rdadb");
  auto ds_set = to_sql_tuple_string(ds_aliases(ng_gdex_id(dsid)));
  LocalQuery query("doi", "dssdb.dsvrsn", "dsid in " + ds_set + " and end_date "
      "is null");
  Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << "https://doi.org/" << row[0];
  } else {
    ofs << "https://rda.ucar.edu/datasets/" << dsid << "/";
  }
  ofs << "\" />" << endl;
  auto elist = xdoc.element_list("dsOverview/author");
  if (!elist.empty()) {
    for (const auto& author : elist) {
      ofs << "<meta name=\"DC.creator\" content=\"";
      auto author_type = author.attribute_value("xsi:type");
      if (author_type == "authorPerson" || author_type.empty()) {
        ofs << author.attribute_value("lname") << ", " << author.
            attribute_value("fname");
      } else {
        ofs << author.attribute_value("name");
      }
      ofs << "\" />" << endl;
    }
  } else {
    query.set("select g.path,c.contact from search.contributors_new as c left "
        "join search.gcmd_providers as g on g.uuid = c.keyword where c.dsid in "
        + ds_set + " and c.vocabulary = 'GCMD'");
    if (query.submit(server) < 0) {
      myerror = "database error: " + server.error();
      return false;
    }
    if (query.num_rows() == 0) {
      myerror = "no contributors were found for ds" + dsid;
      return false;
    }
    auto num_contributors = 0;
    for (const auto& row : query) {
      if (!row[0].empty()) {
        auto name_parts = split(row[0]," > ");
        if (name_parts.back() == "UNAFFILIATED INDIVIDUAL") {
          auto contact_parts = split(row[1], ",");
          if (!contact_parts.empty()) {
            ofs << "<meta name=\"DC.creator\" content\"" << contact_parts.
                front() << "\" />" << endl;
            ++num_contributors;
          }
        } else {
          ofs << "<meta name=\"DC.creator\" content=\"" << substitute(
              name_parts.back(), ", ", "/") << "\" />" << endl;
          ++num_contributors;
        }
      }
    }
    if (num_contributors == 0) {
      myerror = "no useable contributors were found for ds" + dsid;
      return false;
    }
  }
  ofs << "<meta name=\"DC.title\" content=\"" << substitute(xdoc.element(
      "dsOverview/title").content(), "\"", "\\\"") << "\" />" << endl;
  ofs << "<meta name=\"DC.date\" content=\"" << xdoc.element("dsOverview/"
      "publicationDate").content() << "\" scheme=\"DCTERMS.W3CDTF\" />" << endl;
  ofs << "<meta name=\"DC.publisher\" content=\"" << PUBLISHER << "\" />" <<
      endl;
  auto summary = htmlutils::convert_html_summary_to_ascii(xdoc.element(
      "dsOverview/summary").to_string(), 0x7fffffff, 0);
  replace_all(summary, "\n", "\\n");
  ofs << "<meta name=\"DC.description\" content=\"" << substitute(summary, "\"",
      "\\\"") << "\" />" << endl;
  query.set("select g.path from search.variables as v left join search."
      "gcmd_sciencekeywords as g on g.uuid = v.keyword where v.dsid in " +
      ds_set + " and v.vocabulary = 'GCMD'");
  if (query.submit(server) == 0) {
    for (const auto& row : query) {
      ofs << "<meta name=\"DC.subject\" content=\"" << row[0] << "\" />" <<
          endl;
    }
  }
  return true;
}

bool export_to_oai_dc(std::ostream& ofs, string dsid, XMLDocument& xdoc, size_t
    indent_length) {
  string indent(indent_length, ' ');
  auto e = xdoc.element("dsOverview/creator@vocabulary=GCMD");
  auto creator = e.attribute_value("name");
  ofs << indent << "<oai_dc:dc xmlns:oai_dc=\"http://www.openarchives.org/OAI/"
      "2.0/oai_dc/\"" << endl;
  ofs << indent << "           xmlns:dc=\"http://purl.org/dc/elements/1.1/\"" <<
      endl;
  ofs << indent << "           xmlns:xsi=\"http://www.w3.org/2001/"
      "XMLSchema-instance\"" << endl;
  ofs << indent << "           xsi:schemaLocation=\"http://"
      "www.openarchives.org/OAI/2.0/oai_dc/" << endl;
  ofs << indent << "                               http://www.openarchives.org/"
      "OAI/2.0/oai_dc.xsd\">" << endl;
  e = xdoc.element("dsOverview/title");
  ofs << indent << "  <dc:title>" << e.content() << "</dc:title>" << endl;
  ofs << indent << "  <dc:creator>" << creator << "</dc:creator>" << endl;
  ofs << indent << "  <dc:publisher>" << creator << "</dc:publisher>" << endl;
  e = xdoc.element("dsOverview/topic@vocabulary=ISO");
  ofs << indent << "  <dc:subject>" << e.content() << "</dc:subject>" << endl;
  e = xdoc.element("dsOverview/summary");
  ofs << indent << "  <dc:description>" << endl;
  ofs << htmlutils::convert_html_summary_to_ascii(e.to_string(), 80,
      indent_length + 4) << endl;
  ofs << indent << "  </dc:description>" << endl;
  auto elist = xdoc.element_list("dsOverview/contributor@vocabulary=GCMD");
  for (const auto& element : elist) {
    ofs << indent << "  <dc:contributor>" << element.attribute_value("name") <<
        "</dc:contributor>" << endl;
  }
  ofs << indent << "  <dc:type>Dataset</dc:type>" << endl;
  ofs << indent << "  <dc:identifier>" << dsid << "</dc:identifier>" << endl;
  ofs << indent << "  <dc:language>english</dc:language>" << endl;
  elist = xdoc.element_list("dsOverview/relatedDataset");
  for (const auto& element : elist) {
    ofs << indent << "  <dc:relation>rda.ucar.edu:ds" << element.
        attribute_value("ID") << "</dc:relation>" << endl;
  }
  elist = xdoc.element_list("dsOverview/relatedResource");
  for (const auto& element : elist) {
    ofs << indent << "  <dc:relation>" << element.content() << " [" << element.
        attribute_value("url") << "]</dc:relation>" << endl;
  }
  e = xdoc.element("dsOverview/restrictions/access");
  if (e.name() == "access") {
    ofs << indent << "  <dc:rights>" << endl;
    ofs << htmlutils::convert_html_summary_to_ascii(e.to_string(), 80, 4) <<
        endl;
    ofs << indent << "  </dc:rights>" << endl;
  }
  e = xdoc.element("dsOverview/restrictions/usage");
  if (e.name() == "usage") {
    ofs << indent << "  <dc:rights>" << endl;
    ofs << htmlutils::convert_html_summary_to_ascii(e.to_string(), 80, 4) <<
        endl;
    ofs << indent << "  </dc:rights>" << endl;
  }
  ofs << indent << "</oai_dc:dc>" << endl;
  return true;
}

} // end namespace metadataExport
