#include <iostream>
#include <regex>
#include <PostgreSQL.hpp>
#include <citation_pg.hpp>
#include <strutils.hpp>
#include <xmlutils.hpp>
#include <xml.hpp>

using namespace PostgreSQL;
using std::endl;
using std::regex;
using std::regex_search;
using std::string;
using strutils::is_numeric;
using strutils::replace_all;
using strutils::split;

namespace citation {

void export_to_bibtex(std::ostream& ofs, string dsid, string access_date,
    Server& server) {
  XMLDocument xdoc;
  if (open_ds_overview(xdoc, dsid)) {
    ofs << "@misc{cisl_rda_ds" << dsid << "," << endl;
    ofs << " author = \"";
    auto elist = xdoc.element_list("dsOverview/author");
    auto n = 0;
    if (!elist.empty()) {
      for (const auto& e : elist) {
        if (n > 0) {
          ofs << " and ";
        }
        auto type = e.attribute_value("xsi:type");
        if (type == "authorPerson" || type.empty()) {
          ofs << e.attribute_value("fname");
          auto mname = e.attribute_value("mname");
          if (!mname.empty()) {
            ofs << " " << mname;
          }
          ofs << " {" << e.attribute_value("lname") << "}";
        } else {
          ofs << "{" << e.attribute_value("name") << "}";
        }
        ++n;
      }
    } else {
      LocalQuery query("select g.path from search.contributors_new as c left "
          "join search.gcmd_providers as g on g.uuid = c.keyword where c.dsid "
          "= '" + dsid + "' and c.vocabulary = 'GCMD' and c.citable = 'Y' "
          "order by c.disp_order");
      if (query.submit(server) == 0) {
        for (const auto& row : query) {
          if (n > 0) {
            ofs << " and ";
          }
          auto sp = split(row[0], " > ");
          ofs << "{" << sp.back() << "}";
          ++n;
        }
      }
    }
    ofs << "\"," << endl;
    LocalQuery query("select s.title, s.summary, s.pub_date,d.doi from search."
        "datasets as s left join dssdb.dsvrsn as d on d.dsid = concat('ds', s."
        "dsid) where s.dsid = '" + dsid + "'");
    Row row;
    if (query.submit(server) == 0 && query.fetch_row(row)) {
      ofs << " title = \"" << row[0] << "\"," << endl;
      ofs << " publisher  = \"" << DATA_CENTER << "\"," << endl;
      ofs << " address = {Boulder CO}," << endl;
      ofs << " year  = \"" << row[2].substr(0, 4) << "\"," << endl;
      if (!row[3].empty()) {
        ofs << " url = \"" << DOI_URL_BASE << "/" << row[3] << "\"" << endl;
      } else {
        ofs << " url = \"https://rda.ucar.edu/datasets/" << dsid << "/\"" <<
            endl;
      }
    }
    ofs << "}" << endl;
  }
}

void export_to_ris(std::ostream& ofs, string dsid, string access_date, Server&
    server) {
  XMLDocument xdoc;
  if (open_ds_overview(xdoc, dsid)) {
    ofs << "Provider: " << DATA_CENTER << "\r" << endl;
    ofs << "Tagformat: ris\r" << endl;
    ofs << "\r" << endl;
    ofs << "TY  - DATA\r" << endl;
    auto elist = xdoc.element_list("dsOverview/author");
    if (!elist.empty()) {
      for (const auto& e : elist) {
        ofs << "AU  - ";
        auto type = e.attribute_value("xsi:type");
        if (type == "authorPerson" || type.empty()) {
          ofs << e.attribute_value("lname") << ", " << e.attribute_value(
              "fname");
          auto mname = e.attribute_value("mname");
          if (!mname.empty()) {
            ofs << " " << mname;
          }
        } else {
          ofs << e.attribute_value("name");
        }
        ofs << "\r" << endl;
      }
    } else {
      elist = xdoc.element_list("dsOverview/contributor");
      for (const auto& e : elist) {
        auto sp = split(e.attribute_value("name"), " > ");
        ofs << "AU  - " << sp[1] << "\r" << endl;
      }
    }
    LocalQuery query("select s.title, s.summary, s.pub_date, d.doi from search."
        "datasets as s left join dssdb.dsvrsn as d on d.dsid = concat('ds', s."
        "dsid) where s.dsid = '" + dsid + "'");
    Row row;
    if (query.submit(server) == 0 && query.fetch_row(row)) {
      ofs << "T1  - " << row[0] << "\r" << endl;
      auto s = row[1];
      replace_all(s, "\n", " ");
      regex r = regex("  ");
      while (regex_search(s, r)) {
        replace_all(s, "  ", " ");
      }
      XMLSnippet xs("<summary>" + s + "</summary>");
      auto e = xs.element("summary");
      auto abstract = xmlutils::to_plain_text(e, 80, 6).substr(6);
      replace_all(abstract, "\n", "\r\n");
      ofs << "AB  - " << abstract << "\r" << endl;
      ofs << "PY  - " << row[2].substr(0, 4) << "\r" << endl;
      if (!row[3].empty()) {
        ofs << "DO  - " << row[3] << "\r" << endl;
        ofs << "UR  - " << DOI_URL_BASE << "/" << row[3] << "\r" << endl;
      } else {
        ofs << "UR  - https://rda.ucar.edu/datasets/" << dsid << "/\r" <<
            endl;
      }
    }
    ofs << "PB  - " << DATA_CENTER << "\r" << endl;
    ofs << "CY  - Boulder, CO\r" << endl;
    ofs << "LA  - English\r" << endl;
    if (!access_date.empty() && strutils::occurs(access_date, "-") == 2 &&
        is_numeric(strutils::substitute(access_date, "-", ""))) {
      auto sp = split(access_date, "-");
      ofs << "Y2  - " << sp[0] << "/" << sp[1] << "/" << sp[2] << "\r" << endl;
    }
    ofs << "ER  - \r" << endl;
  }
}

} // end namespace citation
