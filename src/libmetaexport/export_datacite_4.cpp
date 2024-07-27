#include <fstream>
#include <metadata_export_pg.hpp>
#include <metahelpers.hpp>
#include <xml.hpp>
#include <PostgreSQL.hpp>
#include <metadata.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bitmap.hpp>
#include <myerror.hpp>

using namespace PostgreSQL;
using std::endl;
using std::min;
using std::max;
using std::string;
using std::stringstream;
using std::to_string;
using std::vector;
using strutils::split;
using strutils::capitalize;
using strutils::substitute;
using strutils::to_lower;

namespace metadataExport {

string indent;

bool added_identifier(Server& server, std::ostream& ofs, string dsnum) {
  ofs << indent << "  <identifier identifierType=\"DOI\">";
  LocalQuery query("doi", "dssdb.dsvrsn", "dsid = 'ds" + dsnum + "' and "
      "end_date is null");
  Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << row[0];
  }
  ofs << "</identifier>" << endl;
  return true;
}

bool added_creators(Server& server, std::ostream& ofs, XMLDocument& xdoc, string
    dsnum) {
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
    LocalQuery query("select g.path, c.contact from search.contributors_new as "
        "c left join search.gcmd_providers as g on g.uuid = c.keyword where c."
        "dsid = '" + dsnum + "' and c.vocabulary = 'GCMD'");
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

bool added_publication_year(Server& server, std::ostream& ofs, XMLDocument&
    xdoc, string dsnum) {
  auto e = xdoc.element("dsOverview/publicationDate");
  auto pub_date = e.content();
  if (pub_date.empty()) {
    LocalQuery query("pub_date", "search.datasets", "dsid = '" + dsnum + "'");
    if (query.submit(server) < 0) {
      myerror = "Unable to retrieve publication date - error: '" + server.
          error() + "'.";
      return false;
    }
    Row row;
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

bool added_resource_type(std::ostream& ofs, XMLDocument& xdoc) {
  auto e = xdoc.element("dsOverview/topic@vocabulary=ISO");
  ofs << indent << "  <resourceType resourceTypeGeneral=\"Dataset\">" << e.
      content() << "</resourceType>" << endl;
  return true;
}

bool added_mandatory_fields(Server& server, std::ostream& ofs, XMLDocument&
    xdoc, string dsnum) {
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
  if (!added_resource_type(ofs, xdoc)) {
    return false;
  }
  return true;
}

void add_subjects(Server& server, std::ostream& ofs, string dsnum) {
  LocalQuery query("select g.path, g.uuid from search.variables as v left join "
      "search.gcmd_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"
      + dsnum + "' and v.vocabulary = 'GCMD'");
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

void add_date(Server& server, std::ostream& ofs, string dsnum) {
  LocalQuery query("select min(concat(date_start, ' ', time_start)), min("
      "start_flag), max(concat(date_end, ' ', time_end)), min(end_flag), "
      "any_value(time_zone) from dssdb.dsperiod where dsid = 'ds" + dsnum +
      "' and date_start > '0001-01-01' and date_start < '3000-01-01' and "
      "date_end > '0001-01-01' and date_end < '3000-01-01'");
  Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << indent << "  <dates>" << endl;
    ofs << indent << "    <date dateType=\"Valid\">" << metatranslations::
        date_time(row[0], row[1], row[4], "T") << " to " << metatranslations::
        date_time(row[2], row[3], row[4], "T") << "</date>" << endl;
    ofs << indent << "  </dates>" << endl;
  }
}

void add_related_identifier(Server& server, XMLDocument& xdoc, string dsnum,
    stringstream& rids_ss) {
  auto elist = xdoc.element_list("dsOverview/relatedDOI");
  if (!elist.empty()) {
    for (const auto& doi : elist) {
      rids_ss << indent << "    <relatedIdentifier relatedIdentifierType=\""
          "DOI\" relationType=\"" << doi.attribute_value("relationType") <<
          "\">" << doi.content() << "</relatedIdentifier>" << endl;
    }
  }
  LocalQuery q("select c.doi_work, w.type, count(a.last_name) from citation."
      "data_citations as c left join (select distinct doi from dssdb.dsvrsn "
      "where dsid = 'ds" + dsnum + "') as v on v.doi = c.doi_data left join "
      "citation.works_authors as a on a.id = c.doi_work left join citation."
      "works as w on w.doi = c.doi_work where v.doi is not null group by c."
      "doi_work, w.type having count(a.last_name) > 0");
  if (q.submit(server) == 0) {
    for (const auto& r : q) {
      rids_ss << indent << "    <relatedIdentifier relatedIdentifierType=\""
          "DOI\" relationType=\"IsCitedBy\"";
      if (r[1] == "J") {
        rids_ss << " resourceTypeGeneral=\"JournalArticle\"";
      } else if (r[1] == "C") {
        rids_ss << " resourceTypeGeneral=\"BookChapter\"";
      } else if (r[1] == "P") {
        rids_ss << " resourceTypeGeneral=\"ConferenceProceeding\"";
      }
      rids_ss << ">" << r[0] << "</relatedIdentifier>" << endl;
    }
  }
}

void add_description(std::ostream& ofs, XMLDocument& xdoc) {
  ofs << indent << "  <descriptions>" << endl;
  ofs << indent << "    <description descriptionType=\"Abstract\">" <<
      htmlutils::convert_html_summary_to_ascii(xdoc.element(
      "dsOverview/summary").to_string(),32768,0) << "</description>" << endl;
  ofs << indent << "  </descriptions>" << endl;
}

void add_geolocation_from_xml(std::ostream& ofs, XMLElement& ele) {
  auto elist = ele.element_list("grid");
  double mnlon = 999., mnlat = 999., mxlon = -999., mxlat = -999.;
  for (const auto& e : elist) {
    auto d = e.attribute_value("definition");
    if (e.attribute_value("isCell") == "true") {
      d += "Cell";
    }
    d += "<!>" + e.attribute_value("numX") + ":" + e.attribute_value("numY");
    if (d.find("latLon") == 0 || d.find("mercator") == 0) {
      d += ":" + e.attribute_value("startLat") + ":" + e.attribute_value(
          "startLon") + ":" + e.attribute_value("endLat") + ":" + e.
          attribute_value("endLon") + ":" + e.attribute_value("xRes") + ":" + e.
          attribute_value("yRes");
    } else if (d.find("gaussLatLon") == 0) {
      d += ":" + e.attribute_value("startLat") + ":" + e.attribute_value(
          "startLon") + ":" + e.attribute_value("endLat") + ":" + e.
          attribute_value("endLon") + ":" + e.attribute_value("xRes") + ":" + e.
          attribute_value("numY");
    } else if (d.find("polarStereographic") == 0) {
      d += ":" + e.attribute_value("startLat") + ":" + e.attribute_value(
          "startLon") + ":60" + e.attribute_value("pole") + ":" + e.
          attribute_value("projLon") + ":" + e.attribute_value("pole") + ":" +
          e.attribute_value("xRes") + ":" + e.attribute_value("yRes");
    } else if (d.find("lambertConformal") == 0) {
      d += ":" + e.attribute_value("startLat") + ":" + e.attribute_value(
          "startLon") + ":" + e.attribute_value("resLat") + ":" + e.
          attribute_value("projLon") + ":" + e.attribute_value("pole") + ":" +
          e.attribute_value("xRes") + ":" + e.attribute_value("yRes") + ":" + e.
          attribute_value("stdParallel1") + ":" + e.attribute_value(
          "stdParallel2");
    }
    double wlon, slat, elon, nlat;
    gridutils::fill_spatial_domain_from_grid_definition(d, "primeMeridian",
        wlon, slat, elon, nlat);
    mnlon = min(mnlon, wlon);          
    mnlat = min(mnlat, slat);
    mxlon = max(mxlon, elon);
    mxlat = max(mxlat, nlat);
  }
  if (mnlon < 999.) {
    ofs << indent << "  <geoLocations>" << endl;
    ofs << indent << "    <geoLocation>" << endl;
    ofs << indent << "      <geoLocationBox>" << endl;
    ofs << indent << "        <westBoundLongitude>" << mnlon <<
        "</westBoundLongitude>" << endl;
    ofs << indent << "        <eastBoundLongitude>" << mxlon <<
        "</eastBoundLongitude>" << endl;
    ofs << indent << "        <southBoundLatitude>" << mnlat <<
        "</southBoundLatitude>" << endl;
    ofs << indent << "        <northBoundLatitude>" << mxlat <<
        "</northBoundLatitude>" << endl;
    ofs << indent << "      </geoLocationBox>" << endl;
    ofs << indent << "    </geoLocation>" << endl;
    ofs << indent << "  </geoLocations>" << endl;
  }
}

void add_geolocation_from_database(Server& server, std::ostream& ofs, string
    dsnum) {
  auto d2 = substitute(dsnum, ".", "");
  double mnlon = 999., mnlat = 999., mxlon = -999., mxlat = -999.;
  LocalQuery q("distinct grid_definition_codes", "WGrML.ds" + d2 + "_agrids2");
  if (q.submit(server) == 0) {
    for (const auto& r : q) {
      vector<size_t> v;
      bitmap::uncompress_values(r[0], v);
      for (const auto& e : v) {
        LocalQuery q2("definition, def_params", "WGrML.grid_definitions",
            "code = " + to_string(e));
        Row r2;
        if (q2.submit(server) == 0 && q2.fetch_row(r2)) {
          double wlon, slat, elon, nlat;
          gridutils::fill_spatial_domain_from_grid_definition(r2[0] + "<!>" +
              r2[1], "primeMeridian", wlon, slat, elon, nlat);
          mnlon = min(mnlon, wlon);          
          mnlat = min(mnlat, slat);
          mxlon = max(mxlon, elon);
          mxlat = max(mxlat, nlat);
        }
      }
    }
  }
  vector<string> locs;
  q.set("select g.path from search.locations_new as l left join search."
      "gcmd_locations as g on g.uuid = l.keyword where l.dsid = '" + dsnum +
      "' and l.vocabulary = 'GCMD' order by g.path");
  if (q.submit(server) == 0) {
    locs.reserve(q.num_rows());
    for (const auto& r : q) {
      locs.emplace_back(r[0]);
    }
  }
  if (mnlon < 999. || !locs.empty()) {
    ofs << indent << "  <geoLocations>" << endl;
    if (mnlon < 999.) {
      ofs << indent << "    <geoLocation>" << endl;
      ofs << indent << "      <geoLocationBox>" << endl;
      ofs << indent << "        <westBoundLongitude>" << mnlon <<
          "</westBoundLongitude>" << endl;
      ofs << indent << "        <eastBoundLongitude>" << mxlon <<
          "</eastBoundLongitude>" << endl;
      ofs << indent << "        <southBoundLatitude>" << mnlat <<
          "</southBoundLatitude>" << endl;
      ofs << indent << "        <northBoundLatitude>" << mxlat <<
          "</northBoundLatitude>" << endl;
      ofs << indent << "      </geoLocationBox>" << endl;
      ofs << indent << "    </geoLocation>" << endl;
    }
    if (!locs.empty()) {
      ofs << indent << "    <geoLocation>" << endl;
      for (const auto& loc : locs) {
        ofs << indent << "      <geoLocationPlace>" << loc <<
            "</geoLocationPlace>" << endl;
      }
      ofs << indent << "    </geoLocation>" << endl;
    }
    ofs << indent << "  </geoLocations>" << endl;
  }
}

void add_geolocation(Server& server, std::ostream& ofs, XMLDocument& xdoc,
    string dsnum) {
  auto e = xdoc.element("dsOverview/contentMetadata/geospatialCoverage");
  if (e.name() == "geospatialCoverage") {
    add_geolocation_from_xml(ofs, e);
  } else {
    add_geolocation_from_database(server, ofs, dsnum);
  }
}

void add_recommended_fields(Server& server, XMLDocument& xdoc, string dsnum,
    std::ostream& ofs, stringstream& rids_ss) {
  add_subjects(server, ofs, dsnum);
  add_contributors(ofs);
  add_date(server, ofs, dsnum);
  add_related_identifier(server, xdoc, dsnum, rids_ss);
  add_description(ofs, xdoc);
  add_geolocation(server, ofs, xdoc, dsnum);
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

void add_size(Server& server, std::ostream& ofs, string dsnum) {
  auto size = primary_size(dsnum, server);
  if (!size.empty()) {
    ofs << indent << "  <sizes>" << endl;
    ofs << indent << "    <size>" << size << "</size>" << endl;
    ofs << indent << "  </sizes>" << endl;
  }
}

void add_format(Server& server, std::ostream& ofs, string dsnum) {
  LocalQuery query("distinct keyword", "search.formats", "dsid = '" + dsnum +
      "'");
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

void add_rights(Server& server, std::ostream& ofs, XMLDocument& xdoc) {
  auto e = xdoc.element("dsOverview/dataLicense");
  auto id = e.content();
  if (id.empty()) {
    id = "CC-BY-4.0";
  }
  LocalQuery q("url, name", "wagtail.home_datalicense", "id = '" + id + "'");
  Row row;
  if (q.submit(server) == 0 && q.fetch_row(row)) {
    ofs << indent << "  <rightsList>" << endl;
    ofs << indent << "    <rights rightsIdentifier=\"" << id << "\" rightsURI="
        "\"" << row[0] << "\">" << row[1] << "</rights>" << endl;
    ofs << indent << "  </rightsList>" << endl;
  }
}

void add_funding_reference() {
}

void add_related_item_identifiers(const XMLElement& e, stringstream& rss) {
  auto doi = e.element("doi").content();
  if (!doi.empty()) {
    rss << indent << "      <relatedItemIdentifier relatedItemIdentifierType=\""
        "DOI\">" << doi << "</relatedItemIdentifier>" << endl;
    return;
  }
  auto url = e.element("url").content();
  if (!url.empty()) {
    rss << indent << "      <relatedItemIdentifier relatedItemIdentifierType=\""
        "URL\">" << url << "</relatedItemIdentifier>" << endl;
    return;
  }
}

void add_pub_head(const XMLElement& e, stringstream& rss) {
  rss << indent << "      <creators>" << endl;
  rss << indent << "        <creator>" << endl;
  rss << indent << "          <creatorName>" << e.element("authorList").
      content() << "</creatorName>" << endl;
  rss << indent << "        </creator>" << endl;
  rss << indent << "      </creators>" << endl;
  rss << indent << "      <titles>" << endl;
  rss << indent << "        <title>" << e.element("title").content() <<
      "</title>" << endl;
  rss << indent << "      </titles>" << endl;
  rss << indent << "      <publicationYear>" << e.element("year").content() <<
      "</publicationYear>" << endl;
}

void add_pages(string pages, stringstream& rss) {
  if (!pages.empty()) {
    auto sp = split(pages, "-");
    rss << indent << "      <firstPage>" << sp.front() << "</firstPage>" << endl;
    rss << indent << "      <lastPage>" << sp.back() << "</lastPage>" << endl;
  }
}

void add_book(const XMLElement& e, stringstream& rss, stringstream& rids) {
  string type = "Book";
  auto doi = e.element("doi").content();
  if (doi.empty()) {
    rss << " relatedItemType=\"" << type << "\">" << endl;
    add_related_item_identifiers(e, rss);
    add_pub_head(e, rss);
    auto p = e.element("publisher");
    rss << indent << "      <publisher>" << p.content() << ", " << p.
        attribute_value("place") << "</publisher>" << endl;
    rss << indent << "    </relatedItem>" << endl;
  } else {
    rids << " resourceTypeGeneral=\"" << type << "\">" << doi <<
        "</relatedIdentifier>" << endl;
  }
}

void add_book_chapter(const XMLElement& e, stringstream& rss, stringstream&
    rids) {
  string type = "BookChapter";
  auto doi = e.element("doi").content();
  if (doi.empty()) {
    rss << " relatedItemType=\"" << type << "\">" << endl;
    add_related_item_identifiers(e, rss);
    add_pub_head(e, rss);
    auto b = e.element("book");
    rss << indent << "      <issue>" << b.content() << "</issue>" << endl;
    add_pages(b.attribute_value("pages"), rss);
    rss << indent << "      <publisher>Ed. " << b.attribute_value("editor") <<
        ", " << b.attribute_value("publisher") << "</publisher>" << endl;
    rss << indent << "    </relatedItem>" << endl;
  } else {
    rids << " resourceTypeGeneral=\"" << type << "\">" << doi <<
        "</relatedIdentifier>" << endl;
  }
}

void add_journal_article(const XMLElement& e, stringstream& rss, stringstream&
    rids) {
  auto ds_relation = e.attribute_value("ds_relation");
  string type;
  if (ds_relation == "IsDescribedBy") {
    type = "DataPaper";
  } else {
    type = "JournalArticle";
  }
  auto doi = e.element("doi").content();
  if (doi.empty()) {
    rss << " relatedItemType=\"" << type << "\">" << endl;
    add_related_item_identifiers(e, rss);
    add_pub_head(e, rss);
    auto p = e.element("periodical");
    rss << indent << "      <issue>" << p.content() << "</issue>" << endl;
    rss << indent << "      <number>" << p.attribute_value("number") <<
        "</number>" << endl;
    add_pages(p.attribute_value("pages"), rss);
    rss << indent << "    </relatedItem>" << endl;
  } else {
    rids << " resourceTypeGeneral=\"" << type << "\">" << doi <<
        "</relatedIdentifier>" << endl;
  }
}

void add_conference_proceeding(const XMLElement& e, stringstream& rss,
    stringstream& rids) {
  string type = "ConferenceProceeding";
  auto doi = e.element("doi").content();
  if (doi.empty()) {
    rss << " relatedItemType=\"" << type << "\">" << endl;
    add_related_item_identifiers(e, rss);
    add_pub_head(e, rss);
    auto c = e.element("conference");
    rss << indent << "      <issue>" << c.content() << "</issue>" << endl;
    add_pages(c.attribute_value("pages"), rss);
    rss << indent << "      <publisher>" << c.attribute_value("host") << ", " <<
        c.attribute_value("location") << "</publisher>" << endl;
    rss << indent << "    </relatedItem>" << endl;
  } else {
    rids << " resourceTypeGeneral=\"" << type << "\">" << doi <<
        "</relatedIdentifier>" << endl;
  }
}

void add_report(const XMLElement& e, stringstream& rss, stringstream& rids) {
  string type = "Report";
  auto doi = e.element("doi").content();
  if (doi.empty()) {
    rss << " relatedItemType=\"" << type << "\">" << endl;
    add_related_item_identifiers(e, rss);
    add_pub_head(e, rss);
    auto o = e.element("organization");
    auto report_id = o.attribute_value("reportID");
    if (!report_id.empty()) {
      rss << indent << "      <number>" << report_id << "</number>" << endl;
    }
    add_pages(o.attribute_value("pages"), rss);
    rss << indent << "      <publisher>" << o.content() << "</publisher>" <<
        endl;
    rss << indent << "    </relatedItem>" << endl;
  } else {
    rids << " resourceTypeGeneral=\"" << type << "\">" << doi <<
        "</relatedIdentifier>" << endl;
  }
}

void add_related_item(XMLDocument& xdoc, std::ostream& ofs, stringstream&
    rids_ss) {
  auto elist = xdoc.element_list("dsOverview/reference");
  stringstream rss;
  for (const auto& e : elist) {
    auto doi = e.element("doi").content();
    auto ds_relation = e.attribute_value("ds_relation");
    if (doi.empty()) {
      rss << indent << "    <relatedItem relationType=\"" << ds_relation <<
          "\"";
    } else {
      rids_ss << indent << "    <relatedIdentifier relatedIdentifierType=\""
          "DOI\" relationType=\"" << ds_relation << "\"";
    }
    auto type = e.attribute_value("type");
    if (type == "book") {
      add_book(e, rss, rids_ss);
    } else if (type == "book_chapter") {
      add_book_chapter(e, rss, rids_ss);
    } else if (type == "journal") {
      add_journal_article(e, rss, rids_ss);
    } else if (type == "preprint") {
      add_conference_proceeding(e, rss, rids_ss);
    } else if (type == "technical_report") {
      add_report(e, rss, rids_ss);
    }
  }
  if (!rss.str().empty()) {
    ofs << indent << "  <relatedItems>" << endl;
    ofs << rss.str();
    ofs << indent << "  </relatedItems>" << endl;
  }
}

void add_optional_fields(Server& server, XMLDocument& xdoc, string dsnum, std::
    ostream& ofs, stringstream& rids_ss) {
  add_language(ofs);
  add_alternate_identifier(ofs, dsnum);
  add_related_item(xdoc, ofs, rids_ss);
  add_size(server, ofs, dsnum);
  add_format(server, ofs, dsnum);
  add_version();
  add_rights(server, ofs, xdoc);
  add_funding_reference();
}

bool export_to_datacite_4(std::ostream& ofs, string dsnum, XMLDocument& xdoc,
    size_t indent_length) {
  indent = string(indent_length, ' ');
  Server server(metautils::directives.database_server, metautils::directives.
      metadb_username, metautils::directives.metadb_password, "rdadb");
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
  stringstream rids_ss;
  add_recommended_fields(server, xdoc, dsnum, ofs, rids_ss);
  add_optional_fields(server, xdoc, dsnum, ofs, rids_ss);
  if (!rids_ss.str().empty()) {
    ofs << indent << "  <relatedIdentifiers>" << endl;
    ofs << indent << rids_ss.str();
    ofs << indent << "  </relatedIdentifiers>" << endl;
  }
  ofs << indent << "</resource>" << endl;
  return true;
}

} // end namespace metadataExport
