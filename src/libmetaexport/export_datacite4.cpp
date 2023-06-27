#include <fstream>
#include <metadata_export.hpp>
#include <metahelpers.hpp>
#include <xml.hpp>
#include <MySQL.hpp>
#include <metadata.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <myerror.hpp>

using std::endl;
using std::string;
using strutils::split;
using strutils::capitalize;
using strutils::substitute;
using strutils::to_lower;

namespace metadataExport {

string indent;

bool added_identifier(MySQL::Server& server, std::ostream& ofs,
    string dsnum) {
  ofs << indent << "  <identifier identifierType=\"DOI\">";
  MySQL::LocalQuery query("doi", "dssdb.dsvrsn", "dsid = 'ds" + dsnum + "' and "
      "end_date is null");
  MySQL::Row row;
  if (query.submit(server) < 0) {
    myerror = "Unable to retrieve DOI - error: '" + server.error() + "'.";
    return false;
  }
  if (!query.fetch_row(row)) {
    myerror = "Unable to retrieve DOI - error: '" + query.error() + "'.";
    return false;
  }
  ofs << row[0];
  ofs << "</identifier>" << endl;
  return true;
}

bool added_creators(MySQL::Server& server, std::ostream& ofs,
    XMLDocument& xdoc, string dsnum) {
  ofs << indent << "  <creators>" << endl;
  auto elist = xdoc.element_list("dsOverview/author");
  if (!elist.empty()) {
    for (const auto& author : elist) {
      auto author_type = author.attribute_value("xsi:type");
      ofs << indent << "    <creator>" << endl;
      if (author_type == "authorPerson" || author_type.empty()) {
        ofs << indent << "      <creatorName nameType=\"Personal\">" << author.
            attribute_value("lname") << ", " << author.attribute_value("fname")
            << "</creatorName>" << endl;
        ofs << indent << "      <givenName>" << author.attribute_value("fname")
            << "</givenName>" << endl;
        ofs << indent << "      <familyName>" << author.attribute_value("lname")
            << "</familyName>" << endl;
        auto orcid_id = author.attribute_value("orcid_id");
        if (!orcid_id.empty()) {
          ofs << indent << "      <nameIdentifier nameIdentifierScheme=\""
              "ORCID\" schemURI=\"https://orcid.org/\">" << orcid_id <<
              "</nameIdentifier>" << endl;
        }
      } else {
        ofs << indent << "      <creatorName nameType=\"Organizational\">" <<
            author.attribute_value("name") << "</creatorName>" << endl;
      }
      ofs << indent << "    </creator>" << endl;
    }
  } else {
    MySQL::LocalQuery query("select g.path, c.contact from search."
        "contributors_new as c left join search.gcmd_providers as g on g.uuid "
        "= c.keyword where c.dsid = '" + dsnum + "' and c.vocabulary = 'GCMD'");
    if (query.submit(server) < 0) {
      myerror = "Unable to retrieve contributors -  error: '" + server.error() +
          "'.";
      return false;
    }
    if (query.num_rows() == 0) {
      myerror = "No contributors found for ds" + dsnum + ".";
      return false;
    }
    auto found_contributor = false;
    for (const auto& row : query) {
      auto sp = split(row[0], " > ");
      if (sp.back() == "UNAFFILIATED INDIVIDUAL") {
        auto sp2 = split(row[1], ",");
        if (!sp2.empty()) {
          auto lname = capitalize(substitute(to_lower(sp.back()), " ", "_"));
          ofs << indent << "    <creator>" << endl;
          ofs << indent << "      <creatorName nameType=\"Personal\">" << lname
              << ", " << sp2.front() << "</creatorName>" << endl;
          ofs << indent << "      <givenName>" << sp2.front() << "</givenName>"
              << endl;
          ofs << indent << "      <familyName>" << lname << "</familyName>" <<
              endl;
          ofs << indent << "    </creator>" << endl;
          found_contributor = true;
        }
      } else {
        ofs << indent << "    <creator>" << endl;
        ofs << indent << "      <creatorName nameType=\"Organizational\">" <<
            substitute(sp.back(), ", ", "/") << "</creatorName>" << endl;
        ofs << indent << "    </creator>" << endl;
        found_contributor = true;
      }
    }
    if (!found_contributor) {
      myerror = "No useable contributors were found for ds" + dsnum + ".";
      return false;
    }
  }
  ofs << indent << "  </creators>" << endl;
  return true;
}

bool added_title(std::ostream& ofs, XMLDocument& xdoc) {
  ofs << indent << "  <titles>" << endl;
  auto e = xdoc.element("dsOverview/title");
  ofs << indent << "    <title>" << e.content() << "</title>" << endl;
  ofs << indent << "  </titles>" << endl;
  return true;
}

bool added_publisher(std::ostream& ofs) {
  ofs << indent << "  <publisher>" << PUBLISHER << "</publisher>" << endl;
  return true;
}

bool added_publication_year(MySQL::Server& server, std::ostream& ofs,
    XMLDocument& xdoc, string dsnum) {
  auto e = xdoc.element("dsOverview/publicationDate");
  auto pub_date = e.content();
  if (pub_date.empty()) {
    MySQL::LocalQuery query("pub_date", "search.datasets", "dsid = '" + dsnum +
        "'");
    if (query.submit(server) < 0) {
      myerror = "Unable to retrieve publication date - error: '" + server.
          error() + "'.";
      return false;
    }
    MySQL::Row row;
    if (!query.fetch_row(row)) {
      myerror = "Unable to retrieve publication date - error: '" + query.error()
          + "'.";
      return false;
    }
    pub_date = row[0];
  }
  ofs << indent << "  <publicationYear>" << pub_date.substr(0, 4) <<
      "</publicationYear>" << endl;
  return true;
}

bool added_resource_type(std::ostream& ofs) {
  ofs << indent << "  <resourceType resourceTypeGeneral=\"Dataset\">"
      "</resourceType>" << endl;
  return true;
}

bool added_mandatory_fields(MySQL::Server& server, std::ostream& ofs,
    XMLDocument& xdoc, string dsnum) {
  if (!added_identifier(server, ofs, dsnum)) {
    return false;
  }
  if (!added_creators(server, ofs, xdoc, dsnum)) {
    return false;
  }
  if (!added_title(ofs, xdoc)) {
    return false;
  }
  if (!added_publisher(ofs)) {
    return false;
  }
  if (!added_publication_year(server, ofs, xdoc, dsnum)) {
    return false;
  }
  if (!added_resource_type(ofs)) {
    return false;
  }
  return true;
}

void add_subjects(MySQL::Server& server, std::ostream& ofs, string dsnum) {
  MySQL::LocalQuery query("select g.path, g.uuid from search.variables as v "
      "left join search.gcmd_sciencekeywords as g on g.uuid = v.keyword where "
      "v.dsid = '" + dsnum + "' and v.vocabulary = 'GCMD'");
  if (query.submit(server) == 0) {
    ofs << indent << "  <subjects>" << endl;
    for (const auto& row : query) {
      ofs << indent << "    <subject subjectScheme=\"GCMD\" schemeURI=\""
          "https://gcmd.earthdata.nasa.gov/kms\" valueURI=\"https://gcmd."
          "earthdata.nasa.gov/kms/concept/" << row[1] << "\">" << row[0] <<
          "</subject>" << endl;
    }
    ofs << indent << "  </subjects>" << endl;
  }
}

void add_contributors(std::ostream& ofs) {
  ofs << indent << "  <contributors>" << endl;
  ofs << indent << "    <contributor contributorType=\"HostingInstitution\">" <<
      endl;
  ofs << indent << "      <contributorName>" << DATACITE_HOSTING_INSTITUTION <<
      "</contributorName>" << endl;
  ofs << indent << "    </contributor>" << endl;
  ofs << indent << "  </contributors>" << endl;
}

void add_date(MySQL::Server& server, std::ostream& ofs, string dsnum) {
  MySQL::LocalQuery query("select min(concat(date_start, ' ', time_start)), "
      "min(start_flag), max(concat(date_end, ' ', time_end)), min(end_flag), "
      "any_value(time_zone) from dssdb.dsperiod where dsid = 'ds" + dsnum +
      "' and date_start > '0001-01-01' and date_start < '3000-01-01' and "
      "date_end > '0001-01-01' and date_end < '3000-01-01'");
  MySQL::Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << indent << "  <dates>" << endl;
    ofs << indent << "    <date dateType=\"Valid\">" << metatranslations::
        date_time(row[0], row[1], row[4], "T") << " to " << metatranslations::
        date_time(row[2], row[3], row[4], "T") << "</date>" << endl;
    ofs << indent << "  </dates>" << endl;
  }
}

void add_related_identifier(std::ostream& ofs, XMLDocument& xdoc) {
  auto elist = xdoc.element_list("dsOverview/relatedDOI");
  if (!elist.empty()) {
    ofs << indent << "  <relatedIdentifiers>" << endl;
    for (const auto& doi : elist) {
      ofs << indent << "    <relatedIdentifier relatedIdentifierType=\"DOI\" "
          "relationType=\"" << doi.attribute_value("relationType") << "\">" <<
          doi.content() << "</relatedIdentifier>" << endl;
    }
    ofs << indent << "  </relatedIdentifiers>" << endl;
  }
}

void add_description(XMLDocument& xdoc, std::ostream& ofs) {
  ofs << indent << "  <descriptions>" << endl;
  ofs << indent << "    <description descriptionType=\"Abstract\">" <<
      htmlutils::convert_html_summary_to_ascii(xdoc.element(
      "dsOverview/summary").to_string(),32768,0) << "</description>" << endl;
  ofs << indent << "  </descriptions>" << endl;
}

void add_geolocation() {
}

void add_recommended_fields(MySQL::Server& server, std::ostream& ofs,
    XMLDocument& xdoc, string dsnum) {
  add_subjects(server, ofs, dsnum);
  add_contributors(ofs);
  add_date(server, ofs, dsnum);
  add_related_identifier(ofs, xdoc);
  add_description(xdoc, ofs);
  add_geolocation();
}

void add_language(std::ostream& ofs) {
  ofs << indent << "  <language>en-US</language>" << endl;
}

void add_alternate_identifier(std::ostream& ofs, string dsnum) {
  ofs << indent << "  <alternateIdentifiers>" << endl;
  ofs << indent << "    <alternateIdentifier alternateIdentifierType=\"URL\">"
      "https://rda.ucar.edu/datasets/ds" << dsnum << "/</alternateIdentifier>"
      << endl;
  ofs << indent << "    <alternateIdentifier alternateIdentifierType=\"Local\">"
      "ds" << dsnum << "</alternateIdentifier>" << endl;
  ofs << indent << "  </alternateIdentifiers>" << endl;
}

void add_size(MySQL::Server& server, std::ostream& ofs, string dsnum) {
  auto size = primary_size(dsnum, server);
  if (!size.empty()) {
    ofs << indent << "  <sizes>" << endl;
    ofs << indent << "    <size>" << size << "</size>" << endl;
    ofs << indent << "  </sizes>" << endl;
  }
}

void add_format(MySQL::Server& server, std::ostream& ofs, string dsnum) {
  MySQL::LocalQuery query("distinct keyword", "search.formats", "dsid = '" +
      dsnum + "'");
  if (query.submit(server) == 0 && query.num_rows() > 0) {
    ofs << indent << "  <formats>" << endl;
    for (const auto& row : query) {
      ofs << indent << "    <format>" << substitute(row[0],"_"," ") <<
          "</format>" << endl;
    }
    ofs << indent << "  </formats>" << endl;
  }
}

void add_version() {
}

void add_rights(MySQL::Server& server, std::ostream& ofs, XMLDocument& xdoc) {
  auto e = xdoc.element("dsOverview/dataLicense");
  auto id = e.content();
  if (id.empty()) {
    id = "CC-BY-4.0";
  }
  MySQL::LocalQuery q("url, name", "wagtail.home_datalicense", "id = '" + id +
      "'");
  MySQL::Row row;
  if (q.submit(server) == 0 && q.fetch_row(row)) {
    ofs << indent << "  <rightsList>" << endl;
    ofs << indent << "    <rights rightsIdentifier=\"" << id << "\" rightsURI="
        "\"" << row[0] << "\">" << row[1] << "</rights>" << endl;
    ofs << indent << "  </rightsList>" << endl;
  }
}

void add_funding_reference() {
}

void add_related_item() {
}

void add_optional_fields(MySQL::Server& server, std::ostream& ofs,
    XMLDocument& xdoc, string dsnum) {
  add_language(ofs);
  add_alternate_identifier(ofs, dsnum);
  add_size(server, ofs, dsnum);
  add_format(server, ofs, dsnum);
  add_version();
  add_rights(server, ofs, xdoc);
  add_funding_reference();
  add_related_item();
}

bool export_to_datacite4(std::ostream& ofs, string dsnum, XMLDocument& xdoc,
    size_t indent_length) {
  indent = string(indent_length, ' ');
  MySQL::Server server(metautils::directives.database_server, metautils::
      directives.metadb_username, metautils::directives.metadb_password, "");
  if (!server) {
    myerror = "Unable to connect to database.";
    return false;
  }
  ofs << indent << "<resource xmlns=\"http://datacite.org/schema/kernel-4\" "
      "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:"
      "schemaLocation=\"http://datacite.org/schema/kernel-4 http://schema."
      "datacite.org/meta/kernel-4.4/metadata.xsd\">" << endl;
  if (!added_mandatory_fields(server, ofs, xdoc, dsnum)) {
    return false;
  }
  add_recommended_fields(server, ofs, xdoc, dsnum);
  add_optional_fields(server, ofs, xdoc, dsnum);
  ofs << indent << "</resource>" << endl;
  return true;
}

} // end namespace metadataExport
