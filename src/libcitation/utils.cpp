#include <sys/stat.h>
#include <MySQL.hpp>
#include <xml.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <tempfile.hpp>
#include <datetime.hpp>

using std::string;
using std::vector;
using strutils::capitalize;
using strutils::is_numeric;
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

string list_authors(XMLSnippet& xsnip, MySQL::Server& server, size_t
    max_num_authors, string et_al_string, bool
    last_first_middle_on_first_author_only) {
  string authlist; // return value
  auto elist = xsnip.element_list("dsOverview/author");
  if (!elist.empty()) {
    if (elist.size() > max_num_authors) {
      auto type = elist.front().attribute_value("xsi:type");
      if (type == "authorPerson" || type.empty()) {
        auto f = elist.front().attribute_value("fname");
        auto m = elist.front().attribute_value("mname");
        auto l = elist.front().attribute_value("lname");
        authlist = l + ", " + f.substr(0,1) + ".";
        if (!m.empty()) {
          if (strutils::occurs(m, ".") > 0) {
            authlist += " " + m;
          } else {
            authlist += " " + m.substr(0, 1) + ".";
          }
        }
      } else {
        authlist = elist.front().attribute_value("name");
      }
      authlist += ", " + et_al_string;
    } else {
      size_t n = 0;
      for (const auto& e : elist) {
        auto type = e.attribute_value("xsi:type");
        if (type == "authorPerson" || type.empty()) {
          auto f = e.attribute_value("fname");
          auto m = e.attribute_value("mname");
          auto l = e.attribute_value("lname");
          if (last_first_middle_on_first_author_only && n > 0) {
            authlist += ", ";
            if (n > 0 && (n + 1) == elist.size()) {
              authlist += "and ";
            }
            authlist += f.substr(0, 1) + ".";
            if (!m.empty()) {
              if (strutils::occurs(m, ".") > 0) {
                authlist += " " + m;
              } else {
                authlist += " " + m.substr(0, 1) + ".";
              }
            }
            authlist += " " + l;
          } else {
            if (!authlist.empty()) {
              authlist += ", ";
              if (n > 0 && (n + 1) == elist.size()) {
                authlist += "and ";
              }
            }
            authlist += l + ", " + f.substr(0, 1) + ".";
            if (!m.empty()) {
              if (strutils::occurs(m, ".") > 0) {
                authlist += " " + m;
              } else {
                authlist += " " + m.substr(0, 1) + ".";
              }
            }
          }
        } else {
          if (!authlist.empty()) {
            authlist+=", ";
            if (n > 0 && (n + 1) == elist.size()) {
              authlist += "and ";
            }
          }
          authlist += e.attribute_value("name");
        }
        ++n;
      }
    }
  } else {
    MySQL::LocalQuery query("select g.path, c.contact from search."
        "contributors_new as c left join search.gcmd_providers as g on g.uuid "
        "= c.keyword where c.dsid = '" + xsnip.element("dsOverview").
        attribute_value("ID") + "' and c.vocabulary = 'GCMD' and c.citable = "
        "'Y' order by c.disp_order");
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

bool open_ds_overview(XMLDocument& xdoc, string dsnum) {
  string fname;
  struct stat buf;
  if (stat(("/usr/local/www/server_root/web/datasets/ds" + dsnum + "/metadata/"
      "dsOverview.xml").c_str(), &buf) == 0) {
    fname = "/usr/local/www/server_root/web/datasets/ds" + dsnum + "/metadata/"
        "dsOverview.xml";
  } else {
    TempDir temp_dir;
    if (!temp_dir.create("/tmp")) {
      return false;
    }
    fname = unixutils::remote_web_file("https://rda.ucar.edu/datasets/ds" +
        dsnum + "/metadata/dsOverview.xml", temp_dir.name());
  }
  if (xdoc.open(fname)) {
    return true;
  }
  return false;
}

} // end namespace citation
