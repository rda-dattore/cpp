#include <fstream>
#include <regex>
#include <metadata_export_pg.hpp>
#include <metadata.hpp>
#include <xml.hpp>
#include <PostgreSQL.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <myerror.hpp>

using namespace PostgreSQL;
using floatutils::myequalf;
using std::endl;
using std::stoi;
using std::string;
using strutils::chop;
using strutils::ds_aliases;
using strutils::replace_all;
using strutils::split;
using strutils::substitute;
using strutils::to_sql_tuple_string;

namespace metadataExport {

bool export_to_json_ld(std::ostream& ofs, string dsid, XMLDocument& xdoc, size_t
    indent_length) {
  ofs << "<script type=\"application/ld+json\">" << endl;
  ofs << "  {" << endl;
  ofs << "    \"@context\": \"http://schema.org\"," << endl;
  ofs << "    \"@type\": \"Dataset\"," << endl;
  ofs << "    \"@id\": \"";
  Server server(metautils::directives.metadb_config);
  auto ds_set = to_sql_tuple_string(ds_aliases(dsid));
  LocalQuery query("doi", "dssdb.dsvrsn", "dsid in " + ds_set + " and end_date "
      "is null");
  Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << "https://doi.org/" << row[0];
  } else {
    ofs << "https://rda.ucar.edu/datasets/" << dsid << "/";
  }
  ofs << "\"," << endl;
  ofs << "    \"name\": \"" << substitute(xdoc.element("dsOverview/title").
      content(), "\"", "\\\"") << "\"," << endl;
  auto summary = htmlutils::convert_html_summary_to_ascii(xdoc.element(
      "dsOverview/summary").to_string(), 0x7fffffff, 0);
  replace_all(summary, "\n", "\\n");
  ofs << "    \"description\": \"" << substitute(summary, "\"", "\\\"") << "\","
      << endl;
  auto elist = xdoc.element_list("dsOverview/author");
  ofs << "    \"author\": {" << endl;
  if (!elist.empty()) {
    string local_indent = "";
    if (elist.size() > 1) {
      ofs << "      \"@list\": [" << endl;
      local_indent = "    ";
    }
    for (const auto& author : elist) {
      if (elist.size() > 1) {
        ofs << "        " << "{" << endl;
      }
      auto author_type = author.attribute_value("xsi:type");
      if (author_type == "authorPerson" || author_type.empty()) {
        ofs << "      " << local_indent << "\"@type\": \"Person\"," << endl;
        ofs << "      " << local_indent << "\"givenName\": \"" << author.
            attribute_value("fname") << "\"," << endl;
        ofs << "      " << local_indent << "\"familyName\": \"" << author.
            attribute_value("lname") << "\"" << endl;
      } else {
        ofs << "      " << local_indent << "\"@type\": \"Organization\"," <<
            endl;
        ofs << "      " << local_indent << "\"name\": \"" << author.
            attribute_value("name") << "\"" << endl;
      }
      ofs << "    " << local_indent << "}";
      if (elist.size() > 1) {
        if (&author != &elist.back()) {
          ofs << ",";
        }
        ofs << endl;
      }
    }
    if (elist.size() > 1) {
      ofs << "      ]" << endl;
      ofs << "    }";
    }
    ofs << "," << endl;
  } else {
    query.set("select g.path, c.contact from search.contributors_new as c left "
        "join search.gcmd_providers as g on g.uuid = c.keyword where c.dsid in "
        + ds_set + " and c.vocabulary = 'GCMD'");
    if (query.submit(server) < 0) {
      myerror = "database error: " + server.error();
      return false;
    }
    if (query.num_rows() == 0) {
      myerror = "no contributors were found for " + dsid;
      return false;
    }
    string local_indent = "";
    if (query.num_rows() > 1) {
      ofs << "      \"@list\": [" << endl;
      local_indent = "    ";
    }
    size_t num_contributors = 0, num_rows = 1;
    for (const auto& row : query) {
      if (query.num_rows() > 1) {
        ofs << "        " << "{" << endl;
      }
      auto name_parts = split(row[0], " > ");
      if (name_parts.back() == "UNAFFILIATED INDIVIDUAL") {
        auto contact_parts = split(row[1], ",");
        if (!contact_parts.empty()) {
          ofs << "      " << local_indent << "\"@type\": \"Person\"," << endl;
          ofs << "      " << local_indent << "\"name\": \"" << contact_parts.
              front() << "\"" << endl;
          ofs << "    " << local_indent << "}";
          if (query.num_rows() > 1) {
            if (num_rows < query.num_rows()) {
              ofs << ",";
            }
            ofs << endl;
          }
          ++num_contributors;
        }
      } else {
        ofs << "      " << local_indent << "\"@type\": \"Organization\"," <<
            endl;
        ofs << "      " << local_indent << "\"name\": \"" << substitute(
            name_parts.back(), ", ", "/") << "\"" << endl;
        ofs << "    " << local_indent << "}";
        if (query.num_rows() > 1) {
          if (num_rows < query.num_rows()) {
            ofs << ",";
          }
          ofs << endl;
        }
        ++num_contributors;
      }
      ++num_rows;
    }
    if (num_contributors == 0) {
      myerror = "no useable contributors were found for " + dsid;
      return false;
    }
    if (query.num_rows() > 1) {
      ofs << "      ]" << endl;
      ofs << "    }";
    }
    ofs << "," << endl;
  }
  query.set("select g.path from search.variables as v left join search."
      "gcmd_sciencekeywords as g on g.uuid = v.keyword where v.dsid in " +
      ds_set + " and v.vocabulary = 'GCMD'");
  if (query.submit(server) == 0) {
    ofs << "    \"keywords\": ";
    if (query.num_rows() == 1) {
      query.fetch_row(row);
      ofs << "\"" << row[0] << "\"," << endl;
    } else {
      ofs << "[" << endl;;
      size_t n = 1;
      for (const auto& row : query) {
        ofs << "      \"" << row[0] << "\"";
        if (n < query.num_rows()) {
          ofs << ",";
        }
        ofs << endl;
        ++n;
      }
       ofs << "    ]," << endl;
    }
  }
  query.set("select min(date_start), min(time_start), max(date_end), max("
      "time_end), min(start_flag), min(time_zone) from dssdb.dsperiod where "
      "dsid in " + ds_set + " and date_start < '9998-01-01' and date_end < "
      "'9998-01-01' group by dsid");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    auto num_parts = stoi(row[4]);
    auto sdate = row[0];
    auto edate = row[2];
    if (num_parts < 3) {
      chop(sdate, 3 * (3 - num_parts));
      chop(edate, 3 * (3 - num_parts));
    } else {
      sdate += "T" + row[1];
      edate += "T" + row[3];
      if (num_parts < 6) {
        chop(sdate, 3 * (6 - num_parts));
        chop(edate, 3 * (6 - num_parts));
      }
    }
    ofs << "    \"temporalCoverage\": \"" << sdate << "/" << edate << "\"," <<
        endl;
  }
  double min_west_lon, min_south_lat, max_east_lon, max_north_lat;
  bool is_grid;
  my::map<Entry> unique_places_table;
  fill_geographic_extent_data(server, dsid, xdoc, min_west_lon, min_south_lat,
      max_east_lon, max_north_lat, is_grid, nullptr, &unique_places_table);
  if (is_grid) {
    ofs << "    \"spatialCoverage\": {" << endl;
    ofs << "      \"@type\": \"Place\"," << endl;
    ofs << "      \"geo\": {" << endl;
    if (min_west_lon < 9999.) {
      if (myequalf(min_west_lon, max_east_lon) && myequalf(min_south_lat,
          max_north_lat)) {
        ofs << "        \"@type\": \"GeoCoordinates\"," << endl;
        ofs << "        \"latitude\": " << min_south_lat << endl;
        ofs << "        \"longitude\": " << min_west_lon << endl;
      } else {
        ofs << "        \"@type\": \"GeoShape\"," << endl;
        ofs << "        \"box\": \"" << min_south_lat << "," << min_west_lon <<
            " " << max_north_lat << "," << max_east_lon << "\"" << endl;
      }
    }
    ofs << "      }" << endl;
    ofs << "    }," << endl;
  } else if (!unique_places_table.empty()) {
/*
    ife.key="__HAS_GEOGRAPHIC_IDENTIFIERS__";
    tokens.ifs.insert(ife);
    re.key="__GEOGRAPHIC_IDENTIFIER__";
    re.list.reset(new std::deque<string>);
    tokens.repeats.insert(re);
    for (const auto& key : unique_places_table.keys()) {
      re.list->emplace_back(key);
    }
*/
  }
  ofs << "    \"publisher\": {" << endl;
  ofs << "      \"@type\": \"Organization\"," << endl;
  ofs << "      \"name\": \"" << PUBLISHER << "\"" << endl;
  ofs << "    }," << endl;
  ofs << "    \"datePublished\": \"" << xdoc.element("dsOverview/"
      "publicationDate").content() << "\"" << endl;
  ofs << "  }" << endl;
  ofs << "</script>" << endl;
  return true;
}

} // end namespace metadataExport
