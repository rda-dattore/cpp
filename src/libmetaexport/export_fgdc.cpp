#include <fstream>
#include <metadata_export.hpp>
#include <metadata.hpp>
#include <citation.hpp>
#include <xml.hpp>
#include <MySQL.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <myerror.hpp>

namespace metadataExport {

bool export_to_fgdc(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  MySQL::Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"");
  std::string indent(indent_length,' ');
  ofs << indent << "<metadata xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << indent << "            xsi:schemaLocation=\"http://www.fgdc.gov/schemas/metadata" << std::endl;
  ofs << indent << "                                http://www.fgdc.gov/schemas/metadata/fgdc-std-001-1998.xsd\">" << std::endl;
  ofs << indent << "  <idinfo>" << std::endl;
  ofs << indent << "    <citation>" << std::endl;
  ofs << indent << "      <citeinfo>" << std::endl;
  ofs << indent << "        <origin>" << citation::list_authors(xdoc,server,3,"et al.") << "</origin>" << std::endl;
  auto e=xdoc.element("dsOverview/publicationDate");
  auto pub_date=e.content();
  if (pub_date.empty()) {
    MySQL::LocalQuery query("pub_date","search.datasets","dsid = '"+dsnum+"'");
    if (query.submit(server) < 0) {
	std::cout << "Content-type: text/plain" << std::endl << std::endl;
	std::cout << "Database error: " << query.error() << std::endl;
	exit(1);
    }
    MySQL::Row row;
    if (query.fetch_row(row)) {
	pub_date=row[0];
    }
  }
  ofs << indent << "        <pubdate>" << strutils::substitute(pub_date,"-","") << "</pubdate>" << std::endl;
  e=xdoc.element("dsOverview/title");
  ofs << indent << "        <title>" << e.content() << "</title>" << std::endl;
  ofs << indent << "        <pubinfo>" << std::endl;
  ofs << indent << "          <pubplace>Boulder, CO</pubplace>" << std::endl;
  ofs << indent << "          <publish>Research Data Archive at the National Center for Atmospheric Research, Computational and Information Systems Laboratory</publish>" << std::endl;
  ofs << indent << "        </pubinfo>" << std::endl;
  ofs << indent << "        <onlink>https://rda.ucar.edu/datasets/ds" << dsnum << "/</onlink>" << std::endl;
  MySQL::LocalQuery query("doi","dssdb.dsvrsn","dsid = 'ds"+dsnum+"' and isnull(end_date)");
  MySQL::Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << indent << "      <onlink>https://doi.org/" << row[0] << "</onlink>" << std::endl;
  }
  ofs << indent << "      </citeinfo>" << std::endl;
  ofs << indent << "    </citation>" << std::endl;
  ofs << indent << "    <descript>" << std::endl;
  e=xdoc.element("dsOverview/summary");
  ofs << indent << "      <abstract>" << std::endl;
  ofs << htmlutils::convert_html_summary_to_ascii(e.to_string(),120,indent_length+8) << std::endl;
  ofs << indent << "      </abstract>" << std::endl;
  ofs << indent << "      <purpose>.</purpose>" << std::endl;
  ofs << indent << "    </descript>" << std::endl;
  ofs << indent << "    <timeperd>" << std::endl;
  ofs << indent << "      <timeinfo>" << std::endl;
  ofs << indent << "        <rngdates>" << std::endl;
  query.set("min(date_start),max(date_end)","dssdb.dsperiod","dsid = 'ds"+dsnum+"' and date_start < '9998-01-01' and date_end < '9998-01-01'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    std::cout << "Database error: " << query.error() << std::endl;
    exit(1);
  }
  if (query.fetch_row(row)) {
    ofs << indent << "          <begdate>" << strutils::substitute(row[0],"-","") << "</begdate>" << std::endl;
    ofs << indent << "          <enddate>" << strutils::substitute(row[1],"-","") << "</enddate>" << std::endl;
  }
  ofs << indent << "        </rngdates>" << std::endl;
  ofs << indent << "      </timeinfo>" << std::endl;
  ofs << indent << "      <current>ground condition</current>" << std::endl;
  ofs << indent << "    </timeperd>" << std::endl;
  ofs << indent << "    <status>" << std::endl;
  e=xdoc.element("dsOverview/continuingUpdate");
  auto update=e.attribute_value("value");
  if (update == "no") {
    ofs << indent << "      <progress>Complete</progress>" << std::endl;
    ofs << indent << "      <update>None planned</update>" << std::endl;
  }
  else {
    ofs << indent << "      <progress>In work</progress>" << std::endl;
    ofs << indent << "      <update>" << strutils::to_capital(update) << "</update>" << std::endl;
  }
  ofs << indent << "    </status>" << std::endl;
  double min_west_lon,min_south_lat,max_east_lon,max_north_lat;
  bool is_grid;
  my::map<Entry> unique_places_table;
  fill_geographic_extent_data(server,dsnum,xdoc,min_west_lon,min_south_lat,max_east_lon,max_north_lat,is_grid,nullptr,&unique_places_table);
  if (max_north_lat > -999.) {
    ofs << indent << "    <spdom>" << std::endl;
    ofs << indent << "      <bounding>" << std::endl;
    if (min_west_lon < -180.) {
	min_west_lon+=360.;
    }
    ofs << indent << "        <westbc>" << min_west_lon << "</westbc>" << std::endl;
    if (max_east_lon > 180.) {
	max_east_lon-=360.;
    }
    ofs << indent << "        <eastbc>" << max_east_lon << "</eastbc>" << std::endl;
    ofs << indent << "        <northbc>" << max_north_lat << "</northbc>" << std::endl;
    ofs << indent << "        <southbc>" << min_south_lat << "</southbc>" << std::endl;
    ofs << indent << "      </bounding>" << std::endl;
    ofs << indent << "    </spdom>" << std::endl;
  }
  ofs << indent << "    <keywords>" << std::endl;
  query.set("select g.path from search.variables_new as v left join search.GCMD_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"+dsnum+"' and v.vocabulary = 'GCMD'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    std::cout << "Database error: " << query.error() << std::endl;
    exit(1);
  }
  while (query.fetch_row(row)) {
    ofs << indent << "      <theme>" << std::endl;
    ofs << indent << "        <themekt>GCMD</themekt>" << std::endl;
    ofs << indent << "        <themekey>" << row[0] << "</themekey>" << std::endl;
    ofs << indent << "      </theme>" << std::endl;
  }
  for (const auto& key : unique_places_table.keys()) {
    ofs << indent << "      <place>" << std::endl;
    ofs << indent << "        <placekt>GCMD</placekt>" << std::endl;
    ofs << indent << "        <placekey>" << key << "</placekey>" << std::endl;
    ofs << indent << "      </place>" << std::endl;
  }
  ofs << indent << "    </keywords>" << std::endl;
  e=xdoc.element("dsOverview/restrictions/access");
  if (e.name() == "access") {
    ofs << indent << "    <accconst>" << std::endl;
    ofs << htmlutils::convert_html_summary_to_ascii(e.to_string(),120,6) << std::endl;
    ofs << indent << "    </accconst>" << std::endl;
  }
  else
    ofs << indent << "    <accconst>None</accconst>" << std::endl;
  e=xdoc.element("dsOverview/restrictions/usage");
  if (e.name() == "usage") {
    ofs << indent << "    <useconst>" << std::endl;
    ofs << htmlutils::convert_html_summary_to_ascii(e.to_string(),120,6) << std::endl;
    ofs << indent << "    </useconst>" << std::endl;
  }
  else {
    ofs << indent << "    <useconst>None</useconst>" << std::endl;
  }
  auto elist=xdoc.element_list("dsOverview/contact");
  auto contact_parts=strutils::split(elist.front().content());
  query.set("logname,phoneno","dssdb.dssgrp","fstname = '"+contact_parts[0]+"' and lstname = '"+contact_parts[1]+"'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    std::cout << "Database error: " << query.error() << std::endl;
    exit(1);
  }
  query.fetch_row(row);
  ofs << indent << "    <ptcontac>" << std::endl;
  ofs << indent << "      <cntinfo>" << std::endl;
  ofs << indent << "        <cntperp>" << std::endl;
  ofs << indent << "          <cntper>" << elist.front().content() << "</cntper>" << std::endl;
  ofs << indent << "          <cntorg>UCAR/CISL Research Data Archive</cntorg>" << std::endl;
  ofs << indent << "        </cntperp>" << std::endl;
  ofs << indent << "        <cntaddr>" << std::endl;
  ofs << indent << "          <addrtype>physical</addrtype>" << std::endl;
  ofs << indent << "          <address>1850 Table Mesa Drive</address>" << std::endl;
  ofs << indent << "          <city>Boulder</city>" << std::endl;
  ofs << indent << "          <state>Colorado</state>" << std::endl;
  ofs << indent << "          <postal>80303</postal>" << std::endl;
  ofs << indent << "          <country>U.S.A.</country>" << std::endl;
  ofs << indent << "        </cntaddr>" << std::endl;
  ofs << indent << "        <cntaddr>" << std::endl;
  ofs << indent << "          <addrtype>mailing</addrtype>" << std::endl;
  ofs << indent << "          <address>P.O. Box 3000</address>" << std::endl;
  ofs << indent << "          <city>Boulder</city>" << std::endl;
  ofs << indent << "          <state>Colorado</state>" << std::endl;
  ofs << indent << "          <postal>80307-3000</postal>" << std::endl;
  ofs << indent << "          <country>U.S.A.</country>" << std::endl;
  ofs << indent << "        </cntaddr>" << std::endl;
  ofs << indent << "        <cntaddr>" << std::endl;
  ofs << indent << "          <addrtype>shipping</addrtype>" << std::endl;
  ofs << indent << "          <address>UCAR Shipping and Receiving</address>" << std::endl;
  ofs << indent << "          <address>3090 Center Green Drive</address>" << std::endl;
  ofs << indent << "          <city>Boulder</city>" << std::endl;
  ofs << indent << "          <state>Colorado</state>" << std::endl;
  ofs << indent << "          <postal>80301</postal>" << std::endl;
  ofs << indent << "          <country>U.S.A.</country>" << std::endl;
  ofs << indent << "        </cntaddr>" << std::endl;
  ofs << indent << "        <cntvoice>" << row[1] << "</cntvoice>" << std::endl;
  ofs << indent << "        <cntemail>" << row[0] << "@ucar.edu</cntemail>" << std::endl;
  ofs << indent << "      </cntinfo>" << std::endl;
  ofs << indent << "    </ptcontac>" << std::endl;
  ofs << indent << "  </idinfo>" << std::endl;
  ofs << indent << "  <metainfo>" << std::endl;
  e=xdoc.element("dsOverview/timeStamp");
  auto ts_parts=strutils::split(e.attribute_value("value"));
  ofs << indent << "    <metd>" << strutils::substitute(ts_parts[0],"-","") << "</metd>" << std::endl;
  ofs << indent << "    <metc>" << std::endl;
  ofs << indent << "      <cntinfo>" << std::endl;
  ofs << indent << "        <cntperp>" << std::endl;
  ofs << indent << "          <cntper>Bob Dattore</cntper>" << std::endl;
  ofs << indent << "          <cntorg>UCAR/CISL Research Data Archive</cntorg>" << std::endl;
  ofs << indent << "        </cntperp>" << std::endl;
  ofs << indent << "        <cntaddr>" << std::endl;
  ofs << indent << "          <addrtype>mailing</addrtype>" << std::endl;
  ofs << indent << "          <address>P.O. Box 3000</address>" << std::endl;
  ofs << indent << "          <city>Boulder</city>" << std::endl;
  ofs << indent << "          <state>Colorado</state>" << std::endl;
  ofs << indent << "          <postal>80307-3000</postal>" << std::endl;
  ofs << indent << "          <country>U.S.A.</country>" << std::endl;
  ofs << indent << "        </cntaddr>" << std::endl;
  ofs << indent << "        <cntvoice>303-497-1825</cntvoice>" << std::endl;
  ofs << indent << "        <cntemail>dattore@ucar.edu</cntemail>" << std::endl;
  ofs << indent << "      </cntinfo>" << std::endl;
  ofs << indent << "    </metc>" << std::endl;
  ofs << indent << "    <metstdn>FGDC Content Standard for Digital Geospatial Metadata</metstdn>" << std::endl;
  ofs << indent << "    <metstdv>FGDC-STD-001-1998</metstdv>" << std::endl;
  ofs << indent << "  </metainfo>" << std::endl;
  ofs << indent << "</metadata>" << std::endl;
  server.disconnect();
  return true;
}

} // end namespace metadataExport
