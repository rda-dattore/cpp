#include <sys/stat.h>
#include <PostgreSQL.hpp>
#include <xml.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <tempfile.hpp>
#include <datetime.hpp>

using namespace PostgreSQL;
using std::string;
using std::vector;
using strutils::capitalize;
using strutils::is_numeric;
using strutils::ng_gdex_id;
using strutils::occurs;
using strutils::split;
using strutils::substitute;

namespace citation {

string add_citation_style_changer(string style) {
  string s; // return value
  s = "<br /><small>Bibliographic citation shown in <form name=\"citation\" "
      "style=\"display: inline\"><select name=\"style\" onChange=\""
      "changeCitation()\">";
  s += "<option value=\"agu\"";
  if (style == "agu") {
    s += " selected";
  }
  s += ">American Geophysical Union (AGU)</option>";
  s += "<option value=\"ams\"";
  if (style == "ams") {
    s += " selected";
  }
  s += ">American Meteorological Society (AMS)</option>";
  s += "<option value=\"copernicus\"";
  if (style == "copernicus") {
    s += " selected";
  }
  s += ">Copernicus Publications</option>";
  s += "<option value=\"datacite\"";
  if (style == "datacite") {
    s += " selected";
  }
  s += ">DataCite (DC)</option>";
  s += "<option value=\"esip\"";
  if (style == "esip") {
    s += " selected";
  }
  s += ">Federation of Earth Science Information Partners (ESIP)</option>";
  s += "<option value=\"gdj\"";
  if (style == "gdj") {
    s += " selected";
  }
  s += ">Geoscience Data Journal</option>";
  s += "</select></form> style</small>";
  return s;
}

string formatted_access_date(string access_date, bool embedded, bool htmlized) {
  string s = "Accessed"; // return value
  if (embedded) {
    s.front() = 'a';
  }
  if (!access_date.empty() && occurs(access_date, "-") == 2 && is_numeric(
      substitute(access_date, "-", ""))) {
    auto sp = split(access_date, "-");
    DateTime dt(stoi(sp[0]), stoi(sp[1]), stoi(sp[2]), 0, 0);
    s += " " + dt.to_string("%dd %h %Y");
    if (!embedded) {
      s += ".";
    }
  } else {
    if (htmlized) {
      s += "<font color=\"red\">&dagger;</font>";
    }
    s += " dd mmm yyyy";
    if (!embedded) {
      s += ".";
    }
  }
  return s;
}

string list_authors(XMLSnippet& xsnip, Server& server, size_t max_num_authors,
    string et_al_string, bool last_first_middle_on_first_author_only) {
  string authlist; // return value
  LocalQuery query("select given_name, middle_name, family_name, type from "
          "search.dataset_authors where dsid = '" + ng_gdex_id(
          xsnip.element("dsOverview").attribute_value("ID")) + "' order by "
          "sequence");
  if (query.submit(server) == 0 && query.num_rows() > 0) {
    if (query.num_rows() > max_num_authors) {
      Row row;
      query.fetch_row(row);
      if (row[3] == "Person") {
        authlist = row[2] + ", " + row[0].substr(0,1) + ".";
        if (!row[1].empty()) {
          if (strutils::occurs(row[1], ".") > 0) {
            authlist += " " + row[1];
          } else {
            authlist += " " + row[1].substr(0, 1) + ".";
          }
        }
      } else {
        authlist = row[0];
      }
      authlist += ", " + et_al_string;
    } else {
      size_t n = 0;
      for (const auto& row : query) {
        if (row[3] == "Person") {
          if (last_first_middle_on_first_author_only && n > 0) {
            authlist += ", ";
            if (n > 0 && (n + 1) == query.num_rows()) {
              authlist += "and ";
            }
            authlist += row[0].substr(0, 1) + ".";
            if (!row[1].empty()) {
              if (strutils::occurs(row[1], ".") > 0) {
                authlist += " " + row[1];
              } else {
                authlist += " " + row[1].substr(0, 1) + ".";
              }
            }
            authlist += " " + row[2];
          } else {
            if (!authlist.empty()) {
              authlist += ", ";
              if (n > 0 && (n + 1) == query.num_rows()) {
                authlist += "and ";
              }
            }
            authlist += row[2] + ", " + row[0].substr(0, 1) + ".";
            if (!row[1].empty()) {
              if (strutils::occurs(row[1], ".") > 0) {
                authlist += " " + row[1];
              } else {
                authlist += " " + row[1].substr(0, 1) + ".";
              }
            }
          }
        } else {
          if (!authlist.empty()) {
            authlist+=", ";
            if (n > 0 && (n + 1) == query.num_rows()) {
              authlist += "and ";
            }
          }
          authlist += row[0];
        }
        ++n;
      }
    }
  } else {
    LocalQuery query("select g.path, c.contact from search.contributors_new as "
        "c left join search.gcmd_providers as g on g.uuid = c.keyword where c."
        "dsid = '" + ng_gdex_id(xsnip.element("dsOverview").attribute_value(
        "ID")) + "' and c.vocabulary = 'GCMD' and c.citable = 'Y' order by c."
        "disp_order");
    if (query.submit(server) == 0) {
      vector<string> c_list;
      for (const auto& row : query) {
        c_list.emplace_back(row[0] + "<!>" + row[1]);
      }
      if (c_list.size() > max_num_authors) {
        auto sp = split(c_list.front(), " > ");
        authlist += substitute(sp.back(), ", ", "/") + ", " + et_al_string;
      } else {
        size_t n = 0;
        for (const auto& c : c_list) {
          auto sp = split(c, "<!>");
          if (!authlist.empty()) {
            authlist += ", ";
          }
          if (n > 0 && (n + 1) == c_list.size()) {
            authlist += "and ";
          }
          auto sp2 = split(sp.front(), " > ");
          if (sp2.back() == "UNAFFILIATED INDIVIDUAL") {
            auto sp3 = split(sp.back(), ",");
            if (!sp3.empty()) {
              sp2.back() = capitalize(sp2.back()) + ", " + sp3.front();
            } else {
              sp2.back() = "";
            }
          }
          if (!sp2.back().empty()) {
            authlist += substitute(sp2.back(), ", ", "/");
          }
          ++n;
        }
      }
    }
  }
  return authlist;
}

bool open_ds_overview(XMLDocument& xdoc, string dsid) {
  TempDir temp_dir;
  string fname;
  struct stat buf;
  if (stat(("/usr/local/www/server_root/web/datasets/" + dsid + "/metadata/"
      "dsOverview.xml").c_str(), &buf) == 0) {
    fname = "/usr/local/www/server_root/web/datasets/" + dsid + "/metadata/"
        "dsOverview.xml";
  } else {
    if (!temp_dir.create("/data/ptmp")) {
      return false;
    }
/*
    fname = unixutils::remote_web_file("https://gdex.k8s.ucar.edu/datasets/" +
        dsid + "/native/", temp_dir.name());
*/
    fname = temp_dir.name() + "/" + dsid + ".xml";
    std::stringstream oss, ess;
    unixutils::mysystem2("/usr/bin/curl -s -o " + fname + " https://gdex.k8s.ucar.edu/datasets/" + dsid + "/native/", oss, ess);
  }
  if (xdoc.open(fname)) {
    return true;
  }
  return false;
}

} // end namespace citation
