#include <fstream>
#include <metadata_export.hpp>
#include <metadata.hpp>
#include <metahelpers.hpp>
#include <xml.hpp>
#include <MySQL.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <myerror.hpp>

using std::endl;
using std::string;
using strutils::capitalize;
using strutils::split;
using strutils::substitute;
using strutils::to_lower;

namespace metadataExport {

bool export_to_datacite_3(std::ostream& ofs, string dsnum, XMLDocument& xdoc,
    size_t indent_length) {
  MySQL::Server server(metautils::directives.database_server, metautils::
      directives.metadb_username, metautils::directives.metadb_password, "");
  string indent(indent_length, ' ');
  ofs << indent << "<resource xmlns=\"http://datacite.org/schema/kernel-3\" "
      "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:"
      "schemaLocation=\"http://datacite.org/schema/kernel-3 http://schema."
      "datacite.org/meta/kernel-3/metadata.xsd\">" << endl;
  ofs << indent << "  <identifier identifierType=\"DOI\">";
  MySQL::LocalQuery query("doi", "dssdb.dsvrsn", "dsid = 'ds" + dsnum + "' and "
      "end_date is null");
  MySQL::Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << row[0];
  }
  ofs << "</identifier>" << endl;
  ofs << indent << "  <creators>" << endl;
  auto elist = xdoc.element_list("dsOverview/author");
  if (!elist.empty()) {
    for (const auto& author : elist) {
      ofs << indent << "    <creator>" << endl;
      ofs << indent << "      <creatorName>";
      auto author_type = author.attribute_value("xsi:type");
      if (author_type == "authorPerson" || author_type.empty()) {
        ofs << author.attribute_value("lname") << ", " << author.
            attribute_value("fname");
        auto mname = author.attribute_value("mname");
        if (!mname.empty()) {
          ofs << " " << mname;
        }
      } else {
        ofs << author.attribute_value("name");
      }
      ofs << "</creatorName>" << endl;
      if (author_type == "authorPerson" || author_type.empty()) {
        auto orcid_id = author.attribute_value("orcid_id");
        if (!orcid_id.empty()) {
          ofs << "      <nameIdentifier>" << orcid_id << "</nameIdentifier>" <<
              endl;
          ofs << "      <nameIdentifierScheme>ORCID</nameIdentifierScheme>" <<
              endl;
          ofs << "      <schemeURI>https://orcid.org/</schemeURI>" << endl;
        }
      }
      ofs << indent << "    </creator>" << endl;
    }
  } else {
    query.set("select g.path, c.contact from search.contributors_new as c left "
        "join search.gcmd_providers as g on g.uuid = c.keyword where c.dsid = '"
        + dsnum + "' and c.vocabulary = 'GCMD'");
    if (query.submit(server) < 0) {
      myerror = "database error: " + server.error();
      return false;
    }
    if (query.num_rows() == 0) {
      myerror = "no contributors were found for ds" + dsnum;
      return false;
    }
    auto n = 0;
    for (const auto& row : query) {
      auto sp = split(row[0], " > ");
      if (sp.back() == "UNAFFILIATED INDIVIDUAL") {
        auto sp2 = split(row[1], ",");
        if (!sp2.empty()) {
          sp.back() = capitalize(substitute(to_lower(sp.back()), " ", "_")) +
              ", " + sp2.front();
        } else {
          sp.back() = "";
        }
      }
      if (!sp.back().empty()) {
        ofs << indent << "    <creator>" << endl;
        ofs << indent << "      <creatorName>" << substitute(sp.back(), ", ",
            "/") << "</creatorName>" << endl;
        ofs << indent << "    </creator>" << endl;
        ++n;
      }
    }
    if (n == 0) {
      myerror = "no useable contributors were found for ds" + dsnum;
      return false;
    }
  }
  ofs << indent << "  </creators>" << endl;
  ofs << indent << "  <titles>" << endl;
  auto e = xdoc.element("dsOverview/title");
  ofs << indent << "    <title>" << e.content() << "</title>" << endl;
  ofs << indent << "  </titles>" << endl;
  ofs << indent << "  <publisher>" << PUBLISHER << "</publisher>" << endl;
  e = xdoc.element("dsOverview/publicationDate");
  auto pub_date = e.content();
  if (pub_date.empty()) {
    query.set("pub_date", "search.datasets", "dsid = '" + dsnum + "'");
    if (query.submit(server) == 0 && query.fetch_row(row)) {
      pub_date = row[0];
    }
  }
  ofs << indent << "  <publicationYear>" << pub_date.substr(0, 4) <<
      "</publicationYear>" << endl;
  ofs << indent << "  <subjects>" << endl;
  query.set("select g.path from search.variables as v left join search."
      "gcmd_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '" + dsnum
      + "' and v.vocabulary = 'GCMD'");
  if (query.submit(server) == 0) {
    for (const auto& row : query) {
      ofs << indent << "    <subject subjectScheme=\"GCMD\">" << row[0] <<
          "</subject>" << endl;
    }
  }
  ofs << indent << "  </subjects>" << endl;
  ofs << indent << "  <contributors>" << endl;
  ofs << indent << "    <contributor contributorType=\"HostingInstitution\">" <<
      endl;
  ofs << indent << "      <contributorName>" << DATACITE_HOSTING_INSTITUTION <<
      "</contributorName>" << endl;
  ofs << indent << "    </contributor>" << endl;
  ofs << indent << "  </contributors>" << endl;
  query.set("select min(concat(date_start, ' ', time_start)), min(start_flag), "
      "max(concat(date_end, ' ', time_end)), min(end_flag), time_zone from "
      "dssdb.dsperiod where dsid = 'ds" + dsnum + "' and date_start > "
      "'0000-00-00' and date_start < '3000-01-01' and date_end > '0000-00-00' "
      "and date_end < '3000-01-01'");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << indent << "  <dates>" << endl;
    ofs << indent << "    <date dateType=\"Valid\">" << metatranslations::
        date_time(row[0], row[1], row[4], "T") << " to " << metatranslations::
        date_time(row[2], row[3], row[4], "T") << "</date>" << endl;
    ofs << indent << "  </dates>" << endl;
  }
  ofs << indent << "  <language>eng</language>" << endl;
  ofs << indent << "  <resourceType resourceTypeGeneral=\"Dataset\" />" << endl;
  ofs << indent << "  <alternateIdentifiers>" << endl;
  ofs << indent << "    <alternateIdentifier alternateIdentifierType=\"Local\">"
      "ds" << dsnum << "</alternateIdentifier>" << endl;
  ofs << indent << "  </alternateIdentifiers>" << endl;
  elist = xdoc.element_list("dsOverview/relatedDOI");
  if (!elist.empty()) {
    ofs << indent << "  <relatedIdentifiers>" << endl;
    for (const auto& doi : elist) {
      ofs << indent << "    <relatedIdentifier relatedIdentifierType=\"DOI\" "
          "relationType=\"" << doi.attribute_value("relationType") << "\">" <<
          doi.content() << "</relatedIdentifier>" << endl;
    }
    ofs << indent << "  </relatedIdentifiers>" << endl;
  }
  auto size = primary_size(dsnum, server);
  if (!size.empty()) {
    ofs << indent << "  <sizes>" << endl;
    ofs << indent << "    <size>" << size << "</size>" << endl;
    ofs << indent << "  </sizes>" << endl;
  }
  query.set("distinct keyword", "search.formats", "dsid = '" + dsnum + "'");
  if (query.submit(server) == 0 && query.num_rows() > 0) {
    ofs << indent << "  <formats>" << endl;
    for (const auto& row : query) {
      ofs << indent << "    <format>" << substitute(row[0],"_"," ") <<
          "</format>" << endl;
    }
    ofs << indent << "  </formats>" << endl;
  }
  ofs << indent << "  <descriptions>" << endl;
  ofs << indent << "    <description descriptionType=\"Abstract\">" <<
      htmlutils::convert_html_summary_to_ascii(xdoc.element(
      "dsOverview/summary").to_string(),32768,0) << "</description>" << endl;
  ofs << indent << "  </descriptions>" << endl;
  ofs << indent << "</resource>" << endl;
  server.disconnect();
  return true;
}

} // end namespace metadataExport
