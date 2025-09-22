#include <citation_pg.hpp>
#include <regex>
#include <PostgreSQL.hpp>
#include <xml.hpp>

using namespace PostgreSQL;
using std::regex;
using std::regex_replace;
using std::string;

namespace citation {

string agu_citation(string dsid, string access_date, XMLSnippet& xsnip, Server&
    server, const Row& row, bool htmlized) {
  string s; // return value
  s = list_authors(xsnip, server, 9, "et al.");
  if (s.back() != '.') {
    s += ".";
  }
  s += " (" + row[1].substr(0, 4) +
      "). " + row[0];
  auto e=xsnip.element("dsOverview/continuingUpdate");
  if (e.attribute_value("value") == "yes") {
    s += " (Updated " + e.attribute_value("frequency") + ")";
  }
  s += " [Dataset]. " + DATA_CENTER + ". ";
  if (!row[2].empty()) {
    s += DOI_URL_BASE + "/" + row[2];
  } else {
    s += "https://rda.ucar.edu/datasets/" + dsid + "/";
  }
  s += ". " + formatted_access_date(access_date, false, htmlized);
  return s;
}

string ams_citation(string dsid, string access_date, XMLSnippet& xsnip, Server&
    server, const Row& row, bool htmlized) {
  string s; // return value
  s = list_authors(xsnip, server, 8, "and Coauthors") + ", " + row[1].substr(0,
      4) + ": " + row[0] + ". " + DATA_CENTER + ", " + formatted_access_date(
      access_date, true, htmlized) + ", ";
  if (!row[2].empty()) {
    s += DOI_URL_BASE + "/" + row[2];
  } else {
    s += "https://rda.ucar.edu/datasets/" + dsid + "/";
  }
  s += ".";
  return s;
}

string copernicus_citation(string dsid, string access_date, XMLSnippet& xsnip,
    Server& server, const Row& row, bool htmlized) {
  string s; // return value
  s = list_authors(xsnip, server, 32768, "", false) + ": " + row[0] + ", " +
      DATA_CENTER + ", ";
  if (!row[2].empty()) {
    s += DOI_URL_BASE + "/" + row[2];
  } else {
    s += "https://rda.ucar.edu/datasets/" + dsid + "/";
  }
  s += ", " + row[1].substr(0, 4) + ". " + formatted_access_date(access_date,
      false, htmlized);
  return s;
}

string esip_citation(string dsid, string access_date, XMLSnippet& xsnip,
    Server& server, const Row& row, bool htmlized) {
  string s; // return value
  s = list_authors(xsnip, server, 10, "et al.");
  if (htmlized) {
    s = regex_replace(s, regex("\\\\u([0-9a-zA-Z]{4})"), "&#x$1;");
  }
  if (s.back() != '.') {
    s += ".";
  }
  s += " " + row[1].substr(0, 4);
  auto e = xsnip.element("dsOverview/continuingUpdate");
  if (e.attribute_value("value") == "yes" && e.attribute_value("frequency") !=
      "irregularly") {
    s += ", updated " + e.attribute_value("frequency");
  }
  s += ". ";
  if (htmlized) {
    s += "<i>";
  }
  s += row[0];
  if (htmlized) {
    s += "</i>";
  }
  s += ". " + DATA_CENTER + ". ";
  if (!row[2].empty()) {
    s += DOI_URL_BASE + "/" + row[2];
  } else {
    s += "https://rda.ucar.edu/datasets/" + dsid + "/";
  }
  s += ". " + formatted_access_date(access_date, false, htmlized);
  return s;
}

} // end namespace citation
