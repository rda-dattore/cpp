#include <citation.hpp>
#include <strutils.hpp>
#include <tempfile.hpp>
#include <PostgreSQL.hpp>
#include <datetime.hpp>

using namespace PostgreSQL;
using std::stoll;
using std::string;
using strutils::ds_aliases;
using strutils::is_numeric;
using strutils::split;
using strutils::substitute;
using strutils::to_sql_tuple_string;

namespace citation {

string citation(string dsid, string style, string access_date, const Row& row,
    XMLSnippet& xsnip, Server& server, bool htmlized) {
  string citation;
  style=strutils::to_lower(style);
  if (style == "agu") {
    citation = agu_citation(dsid, access_date, xsnip, server, row, htmlized);
  } else if (style == "ams") {
    citation = ams_citation(dsid, access_date, xsnip, server, row, htmlized);
  } else if (style == "copernicus") {
    citation = copernicus_citation(dsid, access_date, xsnip, server, row,
        htmlized);
  } else if (style == "datacite") {
    citation = list_authors(xsnip, server, 32768, "", false) + " (" + row[1].
        substr(0, 4) + "): " + row[0] + ". " + DATA_CENTER + ". Dataset. ";
    if (!row[2].empty()) {
      citation += DOI_URL_BASE + "/" + row[2];
    } else {
      citation += "https://rda.ucar.edu/datasets/" + dsid + "/";
    }
    citation += ". " + formatted_access_date(access_date, false, htmlized);
  } else if (style == "esip") {
    citation = esip_citation(dsid, access_date, xsnip, server, row, htmlized);
  } else if (style == "gdj") {
    citation=list_authors(xsnip,server,32768,"",false) + " (" + row[1].substr(0,
        4) + "): " + row[0] + ". " + DATA_CENTER + ". ";
    if (!row[2].empty()) {
      citation += DOI_URL_BASE + "/" + row[2];
    } else {
      citation += "https://rda.ucar.edu/datasets/" + dsid + "/";
    }
    citation += ". " + formatted_access_date(access_date, false, htmlized);
  }
  if (htmlized && (access_date.empty() || strutils::occurs(access_date,"-") != 2 || !strutils::is_numeric(substitute(access_date,"-","")))) {
    citation+="<br />&nbsp;&nbsp;<small><font color=\"red\">&dagger;</font>Please fill in the \"Accessed\" date with the day, month, and year (e.g. - 5 Aug 2011) you last accessed the data from the RDA.</small>";
  }
  return citation;
}

string citation(string dsid, string style, string access_date, Server& server,
    bool htmlized) {
  string citation_;
  auto dsparts = split(dsid, "<!>");
  if (dsparts[0].find("ds") == 0) {
    dsparts[0] = dsparts[0].substr(2);
  }
  string doi;
  if (dsparts.size() > 1 && !dsparts[1].empty()) {
    doi = dsparts[1];
  }
  XMLDocument xdoc;
  if (open_ds_overview(xdoc, dsparts[0])) {
    auto ds_set = to_sql_tuple_string(ds_aliases(dsparts[0]));
    LocalQuery query;
    query.set("select s.title, s.pub_date, d.doi, d.sd, d.ed, d.status from ("
        "select doi, min(start_date) as sd, max(end_date) as ed, string_agg("
        "status, ',') as status from dssdb.dsvrsn where dsid in " + ds_set +
        " and status in ('A', 'H') group by doi) as d left join search."
        "datasets as s on s.dsid in " + ds_set + " order by d.sd desc, d.ed "
        "desc");
    if (query.submit(server) == 0) {
      if (query.num_rows() == 0) {

        // dataset does not have a DOI
        query.set("title, pub_date, ''", "search.datasets", "dsid in " +
            ds_set);
        query.submit(server);
      }
      if (query.num_rows() > 1) {
        if (htmlized) {
          citation_ += "<small>";
        }
        citation_ += "<img src=\"/images/alert.gif\" width=\"12\" height=\""
            "12\" />&nbsp;This dataset has more than one DOI. You are viewing "
            "the citation for data downloaded <form name=\"doi_select\" style="
            "\"display: inline\"><select name=\"doi\" onChange=\""
            "changeCitation()\">";
        string cm;
        auto is_first = true;
        for (const auto& row : query) {
          if (is_first && row[5].find("A") != string::npos) {
            citation_ += "<option value=\"" + row[2] + "\"";
            if (row[2] == doi) {
              citation_ += " selected";
            }
            citation_ += ">on or after " + row[3] + "</option>";
          } else {
            citation_ += "<option value=\"" + row[2] + "\"";
            if (row[2] == doi) {
              citation_ += " selected";
            }
            citation_ += ">between " + row[3] + " and " + DateTime(stoll(
                substitute(row[4], "-", "")) * 1000000).days_subtracted(1).
                to_string("%Y-%m-%d") + "</option>";
          }
          if (!doi.empty() && doi == row[2]) {
            cm=citation(dsparts[0], style, access_date, row, xdoc, server,
                htmlized);
          }
          is_first = false;
        }
        if (cm.empty()) {
          query.rewind();
          Row row;
          query.fetch_row(row);
          cm=citation(dsparts[0], style, access_date, row, xdoc, server,
              htmlized);
        }
        citation_ += "</select></form>";
        if (htmlized) {
          citation_ += "</small><br />";
        }
        citation_ += cm;
      } else {
        Row row;
        if (query.fetch_row(row)) {
          citation_ += citation(dsparts[0], style, access_date, row, xdoc,
              server, htmlized);
        }
      }
    }
    if (htmlized) {
      citation_ += add_citation_style_changer(style);
    }
    xdoc.close();
  }
  return citation_;
}

string temporary_citation(string dsid, string style, string access_date,
    Server& server, bool htmlized) {
  string s; // return value
  string xml;
  xml.reserve(30000);
  auto ds_set = to_sql_tuple_string(ds_aliases(dsid));
  LocalQuery query("select authors from metautil.metaman where dsid in " +
      ds_set);
  Row row;
  if (query.submit(server) == 0 && query.fetch_row(row) && !row[0].empty()) {
    xml = "<dsOverview ID=\"" + dsid + "\">";
    auto sp = split(row[0], "\n");
    for (const auto& a : sp) {
      auto sp2 = split(a, "[!]");
      if (sp2.size() > 1) {
          xml += "<author xsi:type=\"authorPerson\" fname=\"" + sp2[0] +
              "\" mname=\"" + sp2[1] + "\" lname=\"" + sp2[2] + "\" />";
      } else {
          xml += "<author xsi:type=\"authorCorporation\" name=\"" + sp2[0] +
              "\" />";
      }
    }
    xml += "</dsOverview>";
  } else {
    query.set("select contributors from metautil.metaman where dsid in " +
        ds_set);
    if (query.submit(server) == 0 && query.fetch_row(row) && !row[0].empty()) {
      xml="<dsOverview ID=\""+dsid+"\">";
      auto contributors=split(row[0],"\n");
      for (const auto& c : contributors) {
        auto cparts=split(c,"[!]");
        if (cparts.size() == 5 || cparts[3] == "Y") {
          xml+="<contributor name=\""+cparts[0]+"\" />";
        }
      }
      xml+="</dsOverview>";
    }
  }
  if (!xml.empty()) {
    query.set("select m.title,s.pub_date,d.doi from metautil.metaman as m left join search.datasets as s on s.dsid = m.dsid left join dssdb.dsvrsn as d on d.dsid = concat('ds',m.dsid) where m.dsid in "+ds_set);
    if (query.submit(server) == 0 && query.fetch_row(row)) {
      XMLSnippet xsnip(xml);
      s=citation(dsid,style,access_date,row,xsnip,server,htmlized);
      if (htmlized) {
        s+=add_citation_style_changer(style);
      }
    }
  }
  return s;
}

} // end namespace citation
