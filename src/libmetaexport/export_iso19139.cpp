#include <fstream>
#include <regex>
#include <sys/stat.h>
#include <metadata_export.hpp>
#include <metadata.hpp>
#include <xml.hpp>
#include <PostgreSQL.hpp>
#include <strutils.hpp>
#include <utils.hpp>

using namespace PostgreSQL;
using floatutils::myequalf;
using std::stoi;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using strutils::append;
using strutils::ds_aliases;
using strutils::ng_gdex_id;
using strutils::split;
using strutils::substitute;
using strutils::to_capital;
using strutils::to_sql_tuple_string;
using strutils::to_upper;
using strutils::trim;
using strutils::trimmed;

namespace metadataExport {

bool export_to_iso19139(unique_ptr<TokenDocument>& token_doc, std::ostream& ofs,
    string dsid, XMLDocument& xdoc, size_t indent_length) {
  if (token_doc == nullptr) {
    token_doc.reset(new TokenDocument("/usr/local/www/server_root/web/html/oai/"
        "iso19139v2.xml", indent_length));
  }
  Server server(metautils::directives.metadb_config);
  token_doc->add_replacement("__DSID__", dsid);
  auto ds_set = to_sql_tuple_string(ds_aliases(ng_gdex_id(dsid)));
  auto s = xdoc.element("dsOverview/timeStamp").attribute_value("value");
  auto sp = split(s);
  auto dp = split(sp[0], "-");
  auto tp = split(sp[1], ":");
  auto utc_off = stoi(sp[2]) / 100;
  auto d1 = DateTime(stoi(dp[0]), stoi(dp[1]), stoi(dp[2]), stoi(tp[0]) * 10000
      + stoi(tp[1]) * 100 + stoi(tp[2]), 0);
  if (utc_off < 0) {
    d1.add_hours(-utc_off);
  } else {
    d1.subtract_hours(utc_off);
  }
  DateTime d2;
  LocalQuery query("timestamp_utc", "search.datasets", "dsid in " + ds_set);
  Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    sp = split(row[0]);
    auto dp = split(sp[0], "-");
    auto tp = split(sp[1], ":");
    d2.set(stoi(dp[0]), stoi(dp[1]), stoi(dp[2]), stoi(tp[0]) * 10000 + stoi(
        tp[1]) * 100 + stoi(tp[2]), 0);
  }
  DateTime d3;
  query.set("max(concat(date_created, ' ', time_created))", "dssdb.wfile_" +
      dsid);
  if (query.submit(server) == 0 && query.fetch_row(row) && !row[0].empty()) {
    sp = split(row[0]);
    auto dp = split(sp[0], "-");
    auto tp = split(sp[1], ":");
    d3.set(stoi(dp[0]), stoi(dp[1]), stoi(dp[2]), stoi(tp[0]) * 10000 + stoi(
        tp[1]) * 100 + stoi(tp[2]), 0);
    d3.add_hours(7);
  }
  token_doc->add_replacement("__MDATE__", std::max({d1, d2, d3}).to_string(
      "%Y-%m-%dT%H:%MM:%SSZ"));
  auto e = xdoc.element("dsOverview/title");
  token_doc->add_replacement("__TITLE__", e.content());
  e = xdoc.element("dsOverview/publicationDate");
  auto pub_date = e.content();
  if (pub_date.empty()) {
    query.set("pub_date", "search.datasets", "dsid in " + ds_set);
    if (query.submit(server) == 0 && query.fetch_row(row)) {
      pub_date = row[0];
    }
  }
  if (!pub_date.empty()) {
    token_doc->add_replacement("__PUBLICATION_DATE_XML__", "<gmd:date>"
        "<gco:Date>" + pub_date + "</gco:Date></gmd:date>");
  } else {
    token_doc->add_replacement("__PUBLICATION_DATE_XML__", "<gmd:date "
        "gco:nilReason=\"unknown\" />");
  }
  query.set("doi", "dssdb.dsvrsn", "dsid in " + ds_set + " and end_date is "
      "null");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    token_doc->add_if("__HAS_DOI__");
    token_doc->add_replacement("__DOI__", row[0]);
  }
  auto elist = xdoc.element_list("dsOverview/author");
  if (!elist.empty()) {
    token_doc->add_if("__HAS_AUTHOR_PERSONS__");
    for (const auto& e : elist) {
      string author, title;
      auto author_type = e.attribute_value("xsi:type");
      if (author_type == "authorPerson" || author_type.empty()) {
        author = e.attribute_value("lname") + ", " + e.attribute_value("fname")
            + " " + e.attribute_value("mname");
        title = trimmed(e.attribute_value("fname") + " " + e.attribute_value(
            "mname")) + " " + e.attribute_value("lname");
      } else {
        author = e.attribute_value("name");
      }
      trim(author);
      author = "NAME[!]" + author;
      auto orcid_id = e.attribute_value("orcid_id");
      if (!orcid_id.empty()) {
        author += "<!>TITLE[!]" + title + "<!>ORCID_ID[!]" + orcid_id;
      } else {
        author += "<!>NO_ORCID_ID[!]true";
      }
      token_doc->add_repeat("__AUTHOR_PERSON__", author);
    }
  } else {
    query.set("select g.path from search.contributors_new as c left join "
        "search.gcmd_providers as g on g.uuid = c.keyword where c.dsid in " +
        ds_set + " and c.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
      token_doc->add_if("__HAS_AUTHOR_ORGS__");
      for (const auto& row : query) {
        token_doc->add_repeat("__AUTHOR_ORG__", substitute(row[0].substr(row[
            0].find(" > ")+3), "&", "&amp;"));
      }
    }
  }
  e = xdoc.element("dsOverview/summary");
  token_doc->add_replacement("__ABSTRACT__", htmlutils::
      convert_html_summary_to_ascii(e.to_string(), 32768, 0));
  e = xdoc.element("dsOverview/continuingUpdate");
  string frequency;
  if (e.attribute_value("value") == "yes") {
    token_doc->add_replacement("__PROGRESS_STATUS__", "onGoing");
    frequency = e.attribute_value("frequency");
    if (frequency == "bi-monthly") {
      frequency = "monthly";
    } else if (frequency == "half-yearly") {
      frequency = "biannually";
    } else if (frequency == "yearly") {
      frequency = "annually";
    } else if (frequency == "irregularly") {
      frequency = "irregular";
    }
  } else {
    token_doc->add_replacement("__PROGRESS_STATUS__", "completed");
    frequency = "notPlanned";
  }
  token_doc->add_replacement("__UPDATE_FREQUENCY__", frequency);
  e = xdoc.element("dsOverview/logo");
  if (!e.name().empty()) {
    token_doc->add_if("__HAS_LOGO__");
    token_doc->add_replacement("__LOGO_URL__", "https://rda.ucar.edu/images/"
        "ds_logos/" + e.content());
    token_doc->add_replacement("__LOGO_IMAGE_FORMAT__", to_upper(e.content().
        substr(e.content().rfind(".")+1)));
  }
  elist = xdoc.element_list("dsOverview/contact");
  token_doc->add_replacement("__SPECIALIST_NAME__", elist.front().content());
  sp = split(elist.front().content());
  query.set("logname, phoneno", "dssdb.dssgrp", "fstname = '" + sp[0] + "' and "
      "lstname = '" + sp[1] + "'");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    token_doc->add_replacement("__SPECIALIST_PHONE__", row[1]);
    token_doc->add_replacement("__SPECIALIST_EMAIL__", row[0] + "@ucar.edu");
  }
  struct stat buf;
  string format_ref;
  TempDir temp_dir;
  temp_dir.create("/tmp");
  if (stat((metautils::directives.server_root + "/web/metadata/"
      "FormatReferences.xml.new").c_str(), &buf) == 0) {
    format_ref = metautils::directives.server_root + "/web/metadata/"
        "FormatReferences.xml.new";
  } else {
    format_ref = unixutils::remote_web_file("https://rda.ucar.edu/metadata/"
        "FormatReferences.xml.new", temp_dir.name());
  }
  unordered_map<string, string> formats;
  XMLDocument fdoc(format_ref);
  elist = fdoc.element_list("formatReferences/format");
  for (const auto& e : elist) {
    formats.emplace(e.attribute_value("name"), e.element("type").content());
  }
  query.set("distinct keyword", "search.formats", "dsid in " + ds_set);
  if (query.submit(server) == 0 && query.num_rows() > 0) {
    token_doc->add_if("__HAS_FORMATS__");
    for (const auto& row : query) {
      string type;
      if (formats.find(row[0]) != formats.end()) {
        type = formats[row[0]];
      }
      auto fmt = to_capital(substitute(row[0], "proprietary_", ""));
      if (!type.empty()) {
        fmt += " (" + type + ")";
      }
      token_doc->add_repeat("__FORMAT__", fmt);
    }
  }
  query.set("concept_scheme, version, revision_date", "search.gcmd_versions");
  if (query.submit(server) == 0) {
    for (const auto& row : query) {
      auto concept_scheme = to_upper(row[0]);
      token_doc->add_replacement("__"+concept_scheme+"_VERSION__", trimmed(row[
          1]));
      token_doc->add_replacement("__"+concept_scheme+"_REVISION_DATE__", row[
          2]);
    }
    query.set("select g.path from search.platforms_new as p left join search."
        "gcmd_platforms as g on g.uuid = p.keyword where p.dsid in " + ds_set +
        " and p.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
      token_doc->add_if("__HAS_PLATFORMS__");
      for (const auto& row : query) {
        token_doc->add_repeat("__PLATFORM__", substitute(row[0], "&", "&amp;"));
      }
    }
    query.set("select g.path from search.instruments as i left join search."
        "gcmd_instruments as g on g.uuid = i.keyword where i.dsid in " + ds_set
        + " and i.vocabulary = 'GCMD'");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
      token_doc->add_if("__HAS_INSTRUMENTS__");
      for (const auto& row : query) {
        token_doc->add_repeat("__INSTRUMENT__", row[0]);
      }
    }
    query.set("select distinct g.path from (select keyword from search."
        "projects_new where dsid in " + ds_set + " and vocabulary = 'GCMD' "
        "union select keyword from search.supported_projects where dsid in " +
        ds_set + " and vocabulary = 'GCMD') as p left join search."
        "gcmd_projects as g on g.uuid = p.keyword");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
      token_doc->add_if("__HAS_PROJECTS__");
      for (const auto& row : query) {
        token_doc->add_repeat("__PROJECT__", row[0]);
      }
    }
    query.set("select g.path from search.variables as v left join search."
        "gcmd_sciencekeywords as g on g.uuid = v.keyword where v.dsid in " +
        ds_set + " and v.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
      for (const auto& row : query) {
        token_doc->add_repeat("__SCIENCEKEYWORD__", row[0]);
      }
    }
  }
  e = xdoc.element("dsOverview/topic@vocabulary=ISO");
  token_doc->add_replacement("__ISO_TOPIC__", e.content());
  double min_west_lon, min_south_lat, max_east_lon, max_north_lat;
  bool is_grid;
  vector<HorizontalResolutionEntry> hres_list;
  my::map<Entry> unique_places_table;
  fill_geographic_extent_data(server, dsid, xdoc, min_west_lon, min_south_lat,
      max_east_lon, max_north_lat, is_grid, &hres_list, &unique_places_table);
  for (const auto& hres : hres_list) {
    token_doc->add_repeat("__SPATIAL_RESOLUTION__", "UOM[!]" + hres.uom +
        "<!>RESOLUTION[!]" + strutils::dtos(hres.resolution,4));
  }
  if (is_grid) {
    token_doc->add_if("__IS_GRID__");
  }
  auto has_any_extent = false;
  if (min_west_lon < 9999.) {
    if (myequalf(min_west_lon, max_east_lon) && myequalf(min_south_lat,
        max_north_lat)) {
      token_doc->add_if("__HAS_POINT__");
      token_doc->add_replacement("__LAT__", strutils::ftos(min_south_lat, 4));
      token_doc->add_replacement("__LON__", strutils::ftos(min_west_lon, 4));
    } else {
      token_doc->add_if("__HAS_BOUNDING_BOX__");
      token_doc->add_replacement("__WEST_LON__", strutils::ftos(min_west_lon,
          4));
      token_doc->add_replacement("__EAST_LON__", strutils::ftos(max_east_lon,
          4));
      token_doc->add_replacement("__SOUTH_LAT__", strutils::ftos(min_south_lat,
          4));
      token_doc->add_replacement("__NORTH_LAT__", strutils::ftos(max_north_lat,
          4));
    }
    has_any_extent = true;
  }
  if (!unique_places_table.empty()) {
    token_doc->add_if("__HAS_GEOGRAPHIC_IDENTIFIERS__");
    for (const auto& key : unique_places_table.keys()) {
      token_doc->add_repeat("__GEOGRAPHIC_IDENTIFIER__", key);
    }
    has_any_extent = true;
  }
  query.set("select min(date_start), min(time_start), max(date_end), max("
      "time_end), min(start_flag), time_zone from dssdb.dsperiod where dsid in "
      + ds_set + " and date_start < '9998-01-01' and date_end < '9998-01-01' "
      "group by dsid, time_zone");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    size_t max_parts = stoi(row[4]);
    auto parts = split(row[0], "-");
    string date;
    size_t m = (max_parts < 3) ? max_parts : 3;
    for (size_t n = 0; n < m; ++n) {
      append(date, parts[n], "-");
    }
    string time;
    if (max_parts > 3) {
      if (row[1].find("31:") == 0) {
        parts = split("00:00:00", ":");
      } else {
        parts = split(row[1], ":");
      }
      auto m = max_parts - 3;
      for (size_t n = 0; n < 3; ++n) {
        if (n < m) {
          append(time, parts[n], ":");
        } else {
          append(time, "00", ":");
        }
      }
      if (row[5] == "+0000") {
        time += "Z";
      } else {
        time += row[5].substr(0, 3) + ":" + row[5].substr(3);
      }
    }
    if (!time.empty()) {
      date += "T" + time;
    } else if (row[5] == "BCE") {
      date = "-" + date;
    }
    token_doc->add_replacement("__BEGIN_DATE__", date);
    parts = split(row[2], "-");
    date.clear();
    m = (max_parts < 3) ? max_parts : 3;
    for (size_t n = 0; n < m; ++n) {
      append(date, parts[n], "-");
    }
    time.clear();
    if (max_parts > 3) {
      if (row[3].find("31:") == 0) {
        parts = split("23:59:59", ":");
      } else {
        parts = split(row[3], ":");
      }
      auto m = max_parts-3;
      for (size_t n = 0; n < 3; ++n) {
        if (n < m) {
          append(time, parts[n], ":");
        } else {
          append(time, "00", ":");
        }
      }
      if (row[5] == "+0000") {
        time += "Z";
      } else {
        time += row[5].substr(0, 3) + ":" + row[5].substr(3);
      }
    }
    if (!time.empty()) {
      date += "T" + time;
    } else if (row[5] == "BCE") {
      date = "-" + date;
    }
    token_doc->add_replacement("__END_DATE__", date);
    token_doc->add_if("__HAS_TEMPORAL_EXTENT__");
    has_any_extent = true;
  }
  if (has_any_extent) {
    token_doc->add_if("__HAS_ANY_EXTENT__");
  }
  e = xdoc.element("dsOverview/restrictions/usage");
  if (!e.name().empty()) {
    token_doc->add_replacement("__USAGE_RESTRICTIONS__", htmlutils::
        convert_html_summary_to_ascii(e.to_string(), 32768, 0));
  } else {
    string license = "";
    e = xdoc.element("dsOverview/dataLicense");
    if (!e.name().empty()) {
      Server srv(metautils::directives.wagtail_config);
      LocalQuery q("name", "wagtail.home_datalicense", "id = '" + e.content() +
          "'");
      Row r;
      if (q.submit(srv) == 0 && q.fetch_row(r)) {
        license = r[0];
      }
    }
    if (!license.empty()) {
      token_doc->add_replacement("__USAGE_RESTRICTIONS__", license);
    } else {
      token_doc->add_replacement("__USAGE_RESTRICTIONS__", "Creative Commons "
          "Attribution 4.0 International License");
    }
  }
  e = xdoc.element("dsOverview/restrictions/access");
  if (!e.name().empty()) {
    token_doc->add_if("__HAS_ACCESS_RESTRICTIONS__");
    token_doc->add_replacement("__ACCESS_RESTRICTIONS__", htmlutils::
        convert_html_summary_to_ascii(e.to_string(), 32768, 0));
  }
//  query.set("select if(primary_size > 999999999,round(primary_size/1000000,0),if(primary_size > 999999,round(primary_size/1000000,2),if(primary_size > 9999,truncate(round(primary_size/10000,0)/100,2),truncate(round(primary_size/100,0)/10000,2)))) from dssdb.dataset where dsid = 'ds"+dsnum+"'");
  query.set("select case when primary_size > 999999999 then round(primary_size "
      "/ 1000000, 0) else case when primary_size > 999999 then round("
      "primary_size / 1000000, 2) else case when primary_size > 9999 then "
      "trunc(round(primary_size / 10000, 0) / 100, 2) else trunc(round("
      "primary_size / 100, 0) / 10000, 2) end end end from dssdb.dataset where "
      "dsid in " + ds_set);
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    token_doc->add_if("__VOLUME_SPECIFIED__");
    token_doc->add_replacement("__MB_VOLUME__", row[0]);
  }
  elist = xdoc.element_list("dsOverview/relatedResource");
  auto n = 0;
  for (const auto& e : elist) {
    auto url = e.attribute_value("url");
    token_doc->add_repeat("__RELATED_RESOURCE__", "URL[!]" + url + "<!>"
        "PROTOCOL[!]" + url.substr(0, url.find("://")) + "<!>NAME[!]Related "
        "Resource #" + to_string(++n) + "<!>DESCRIPTION[!]" + e.content());
  }
  ofs << *token_doc << std::endl;
  return true;
}

} // end namespace metadataExport
