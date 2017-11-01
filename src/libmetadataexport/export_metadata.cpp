#include <iostream>
#include <algorithm>
#include <regex>
#include <sys/stat.h>
#include <pthread.h>
#include <xml.hpp>
#include <MySQL.hpp>
#include <bsort.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <metadata.hpp>
#include <citation.hpp>
#include <hereDoc.hpp>
#include <tokendoc.hpp>

namespace metadataExport {

int compare_references(XMLElement& left,XMLElement& right)
{
  auto e=left.element("year");
  auto l=e.content();
  e=right.element("year");
  auto r=e.content();
  if (l > r) {
    return -1;
  }
  else {
    return 1;
  }
}

std::string resolution_key(double res,std::string units)
{
  std::string s;
  if (units == "degrees") {
    if (res < 0.09) {
	s="0";
    }
    else if (res < 0.5) {
	s="1";
    }
    else if (res < 1.) {
	s="2";
    }
    else if (res < 2.5) {
	s="3";
    }
    else if (res < 5.) {
	s="4";
    }
    else if (res < 10.) {
	s="5";
    }
    else {
	s="6";
    }
  }
  else if (units == "km") {
    if (res < 0.001) {
	s="7";
    }
    else if (res < 0.03) {
	s="8";
    }
    else if (res < 0.1) {
	s="9";
    }
    else if (res < 0.25) {
	s="10";
    }
    else if (res < 0.5) {
	s="11";
    }
    else if (res < 1.) {
	s="12";
    }
    else if (res < 10.) {
	s="0";
    }
    else if (res < 50.) {
	s="1";
    }
    else if (res < 100.) {
	s="2";
    }
    else if (res < 250.) {
	s="3";
    }
    else if (res < 500.) {
	s="4";
    }
    else if (res < 1000.) {
	s="5";
    }
    else {
	s="6";
    }
  }
  return (s+"<!>"+units);
}

void add_to_resolution_table(double lon_res,double lat_res,std::string units,my::map<Entry>& resolution_table)
{
  Entry e;
  e.key=resolution_key(lon_res,units);
  if (!resolution_table.found(e.key,e)) {
    e.min_lon=lon_res;
    e.min_lat=lat_res;
    resolution_table.insert(e);
  }
  else {
    if (lon_res < e.min_lon) {
	e.min_lon=lon_res;
	resolution_table.replace(e);
    }
  }
  e.key=resolution_key(lat_res,units);
  if (!resolution_table.found(e.key,e)) {
    e.min_lon=lon_res;
    e.min_lat=lat_res;
    resolution_table.insert(e);
  }
  else {
    if (lat_res < e.min_lat) {
	e.min_lat=lat_res;
	resolution_table.replace(e);
    }
  }
}

std::string primary_size(std::string dsnum,MySQL::Server& server)
{
  const char *vunits[]={"bytes","Kbytes","Mbytes","Gbytes","Tbytes","Pbytes"};
  MySQL::LocalQuery query("primary_size","dssdb.dataset","dsid = 'ds"+dsnum+"'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    std::cout << "Database error: " << query.error() << std::endl;
    exit(1);
  }
  std::string psize;
  auto n=0;
  MySQL::Row row;
  if (query.fetch_row(row)) {
    if (!row[0].empty()) {
	double vsize=std::stoll(row[0]);
	while (vsize > 999.999999) {
	  vsize/=1000.;
	  ++n;
	}
	psize=strutils::ftos(vsize,7,3,' ');
	strutils::trim(psize);
    }
  }
  psize+=std::string(" ")+vunits[n];
  return psize;
}

void convert_box_data(const std::string& row,const std::string& col,double& comp_lat,double& comp_lon,std::string comp_type)
{
  double lat,lon;
  convert_box_to_center_lat_lon(1,std::stoi(row),std::stoi(col),lat,lon);
  if (comp_type == "min") {
    if (lat >= -89.5) {
	lat-=0.5;
    }
    if (lat < comp_lat) {
	comp_lat=lat;
    }
    lon-=0.5;
    if (lon < comp_lon) {
	comp_lon=lon;
    }
  }
  else if (comp_type == "max") {
    if (lat <= 89.5) {
	lat+=0.5;
    }
    if (lat > comp_lat) {
	comp_lat=lat;
    }
    lon+=0.5;
    if (lon > comp_lon) {
	comp_lon=lon;
    }
  }
}

struct HResEntry {
  HResEntry() : res(0.),uom() {}

  double res;
  std::string uom;
};

void fill_geographic_extent_data(MySQL::Server& server,std::string dsnum,XMLDocument& xdoc,double& min_west_lon,double& min_south_lat,double& max_east_lon,double& max_north_lat,bool& is_grid,std::deque<HResEntry>& hres_list,my::map<Entry>& unique_places_table)
{
  min_west_lon=min_south_lat=9999.;
  max_east_lon=max_north_lat=-9999.;
  is_grid=false;
  MySQL::LocalQuery query("select definition,defParams from (select distinct gridDefinition_code from GrML.summary where dsid = '"+dsnum+"') as s left join GrML.gridDefinitions as d on d.code = s.gridDefinition_code");
  query.submit(server);
  if (query.num_rows() > 0) {
    is_grid=true;
    MySQL::Row row;
    while (query.fetch_row(row)) {
	double west_lon,south_lat,east_lon,north_lat;
	if (fill_spatial_domain_from_grid_definition(row[0]+"<!>"+row[1],"primeMeridian",west_lon,south_lat,east_lon,north_lat)) {
	  if (west_lon < min_west_lon) {
	    min_west_lon=west_lon;
	  }
	  if (east_lon > max_east_lon) {
	    max_east_lon=east_lon;
	  }
	  if (south_lat < min_south_lat) {
	    min_south_lat=south_lat;
	  }
	  if (north_lat > max_north_lat) {
	    max_north_lat=north_lat;
	  }
	  auto parts=strutils::split(row[1],":");
	  HResEntry he;
	  if (row[0] == "polarStereographic" || row[0] == "lambertConformal") {
	    he.res=std::stod(parts[7])*1000;
	    he.uom="m";
	  }
	  else {
	    he.res=std::stod(parts[6]);
	    he.uom="degree";
	  }
	  hres_list.emplace_back(he);
	}
    }
  }
  else {
    auto elist=xdoc.element_list("dsOverview/contentMetadata/geospatialCoverage/grid");
    if (elist.size() > 0) {
	is_grid=true;
	for (const auto& element : elist) {
	  auto gdef=element.attribute_value("definition");
	  std::string def_params;
	  if (gdef == "latLon" || gdef == "gaussLatLon") {
	    def_params=element.attribute_value("numX")+":"+element.attribute_value("numY")+":"+element.attribute_value("startLat")+":"+element.attribute_value("startLon")+":"+element.attribute_value("endLat")+":"+element.attribute_value("endLon")+":"+element.attribute_value("xRes")+":";
	    if (gdef == "latLon") {
		def_params+=element.attribute_value("yRes");
	    }
	    else {
		def_params+=strutils::itos(std::stoi(element.attribute_value("numY"))/2);
	    }
	  }
	  else if (gdef == "polarStereographic") {
	    def_params=element.attribute_value("numX")+":"+element.attribute_value("numY")+":"+element.attribute_value("startLat")+":"+element.attribute_value("startLon")+":60"+element.attribute_value("pole")+":"+element.attribute_value("projLon")+":"+element.attribute_value("pole")+":"+element.attribute_value("xRes")+":"+element.attribute_value("yRes");
	  }
	  else if (gdef == "mercator") {
	    def_params=element.attribute_value("numX")+":"+element.attribute_value("numY")+":"+element.attribute_value("startLat")+":"+element.attribute_value("startLon")+":"+element.attribute_value("endLat")+":"+element.attribute_value("endLon")+":"+element.attribute_value("xRes")+":"+element.attribute_value("yRes")+":"+element.attribute_value("resLat");
	  }
	  else if (gdef == "lambertConformal") {
	    def_params=element.attribute_value("numX")+":"+element.attribute_value("numY")+":"+element.attribute_value("startLat")+":"+element.attribute_value("startLon")+":"+element.attribute_value("resLat")+":"+element.attribute_value("projLon")+":"+element.attribute_value("pole")+":"+element.attribute_value("xRes")+":"+element.attribute_value("yRes")+":"+element.attribute_value("stdParallel1")+":"+element.attribute_value("stdParallel2");
	  }
	  double west_lon,south_lat,east_lon,north_lat;
	  if (fill_spatial_domain_from_grid_definition(element.attribute_value("definition")+"<!>"+def_params,"primeMeridian",west_lon,south_lat,east_lon,north_lat)) {
	    if (west_lon < min_west_lon) {
		min_west_lon=west_lon;
	    }
	    if (east_lon < 0. && east_lon < west_lon) {
		east_lon+=360.;
	    }
	    if (east_lon > max_east_lon) {
		max_east_lon=east_lon;
	    }
	    if (south_lat < min_south_lat) {
		min_south_lat=south_lat;
	    }
	    if (north_lat > max_north_lat) {
		max_north_lat=north_lat;
	    }
	  }
	}
    }
  }
  std::sort(hres_list.begin(),hres_list.end(),
  [](const HResEntry& left,const HResEntry& right) -> bool
  {
    if (left.res <= right.res) {
	return true;
    }
    else {
	return false;
    }
  });
  if (MySQL::table_exists(server,"ObML.ds"+strutils::substitute(dsnum,".","")+"_geobounds")) {
    query.set("select min(min_lat),min(min_lon),max(max_lat),max(max_lon) from ObML.ds"+strutils::substitute(dsnum,".","")+"_geobounds");
    if (query.submit(server) == 0) {
	MySQL::Row row;
	while (query.fetch_row(row)) {
	  if (row[0].empty() || row[1].empty() || row[2].empty() || row[3].empty()) {
	    std::cerr << "oai warning: " << dsnum << " geobounds: '" << row[0] << "','" << row[1] << "','" << row[2] << "','" << row[3] << "'" << std::endl;
	  }
	  else {
	    auto f=std::stof(row[0])/10000.;
	    if (f < min_south_lat) {
		min_south_lat=f;
	    }
	    f=std::stof(row[1])/10000.;
	    if (f < min_west_lon) {
		min_west_lon=f;
	    }
	    f=std::stof(row[2])/10000.;
	    if (f > max_north_lat) {
		max_north_lat=f;
	    }
	    f=std::stof(row[3])/10000.;
	    if (f > max_east_lon) {
		max_east_lon=f;
	    }
	  }
	}
    }
  }
  else {
    query.set("select l.keyword,d.box1d_row,d.box1d_bitmap_min,d.box1d_bitmap_max from search.locations as l left join search.location_data as d on d.keyword = l.keyword and d.vocabulary = l.vocabulary where l.dsid = '"+dsnum+"' and d.box1d_row >= 0");
    query.submit(server);
    if (query.num_rows() > 0) {
	MySQL::Row row;
	while (query.fetch_row(row)) {
	  Entry entry;
	  if (!unique_places_table.found(row[0],entry)) {
	    entry.key=row[0];
	    unique_places_table.insert(entry);
	  }
	  if (row[1] != "0" || row[2] != "359") {
	    convert_box_data(row[1],row[2],min_south_lat,min_west_lon,"min");
	    convert_box_data(row[1],row[3],max_north_lat,max_east_lon,"max");
	  }
	}
    }
    else {
	auto elist=xdoc.element_list("dsOverview/contentMetadata/geospatialCoverage/location");
	std::string where_conditions="";
	for (const auto& element : elist) {
	  if (!where_conditions.empty()) {
	    where_conditions+=" or ";
	  }
	  where_conditions+="keyword = '"+element.content()+"'";
	}
	query.set("select keyword,min(box1d_row),min(box1d_bitmap_min),max(box1d_row),max(box1d_bitmap_max) from search.location_data where "+where_conditions+" and box1d_row >= 0 group by keyword");
	query.submit(server);
	if (query.num_rows() > 0) {
	  MySQL::Row row;
	  while (query.fetch_row(row)) {
	    if (row[2] != "0" || row[4] != "359") {
		convert_box_data(row[1],row[2],min_south_lat,min_west_lon,"min");
		convert_box_data(row[3],row[4],max_north_lat,max_east_lon,"max");
	    }
	  }
	}
    }
  }
}

void add_replace_from_element_content(XMLDocument& xdoc,std::string xpath,std::string replace_token,TokenDocument& token_doc,std::string if_key = "")
{
  auto e=xdoc.element(xpath);
  if (!e.name().empty()) {
    if (!if_key.empty()) {
	token_doc.add_if(if_key);
    }
    token_doc.add_replacement(replace_token,e.content());
  }
}

void export_to_OAI_DC(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  std::string indent;
  for (size_t n=0; n < indent_length; ++n) {
    indent+=" ";
  }
  XMLElement e=xdoc.element("dsOverview/creator@vocabulary=GCMD");
  auto dss_centername=e.attribute_value("name");
  if (strutils::contains(dss_centername,"/SCD/")) {
    strutils::replace_all(dss_centername,"SCD","CISL");
  }
  ofs << indent << "<oai_dc:dc xmlns:oai_dc=\"http://www.openarchives.org/OAI/2.0/oai_dc/\"" << std::endl;
  ofs << indent << "           xmlns:dc=\"http://purl.org/dc/elements/1.1/\"" << std::endl;
  ofs << indent << "           xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << indent << "           xsi:schemaLocation=\"http://www.openarchives.org/OAI/2.0/oai_dc/" << std::endl;
  ofs << indent << "                               http://www.openarchives.org/OAI/2.0/oai_dc.xsd\">" << std::endl;
  e=xdoc.element("dsOverview/title");
  ofs << indent << "  <dc:title>" << e.content() << "</dc:title>" << std::endl;
  ofs << indent << "  <dc:creator>" << dss_centername << "</dc:creator>" << std::endl;
  ofs << indent << "  <dc:publisher>" << dss_centername << "</dc:publisher>" << std::endl;
  e=xdoc.element("dsOverview/topic@vocabulary=ISO");
  ofs << indent << "  <dc:subject>" << e.content() << "</dc:subject>" << std::endl;
  e=xdoc.element("dsOverview/summary");
  ofs << indent << "  <dc:description>" << std::endl;
  ofs << summary::convert_html_summary_to_ASCII(e.to_string(),80,indent_length+4) << std::endl;
  ofs << indent << "  </dc:description>" << std::endl;
  std::list<XMLElement> elist=xdoc.element_list("dsOverview/contributor@vocabulary=GCMD");
  for (const auto& element : elist) {
    ofs << indent << "  <dc:contributor>" << element.attribute_value("name") << "</dc:contributor>" << std::endl;
  }
  ofs << indent << "  <dc:type>Dataset</dc:type>" << std::endl;
  ofs << indent << "  <dc:identifier>ds" << dsnum << "</dc:identifier>" << std::endl;
  ofs << indent << "  <dc:language>english</dc:language>" << std::endl;
  elist=xdoc.element_list("dsOverview/relatedDataset");
  for (const auto& element : elist) {
    ofs << indent << "  <dc:relation>rda.ucar.edu:ds" << element.attribute_value("ID") << "</dc:relation>" << std::endl;
  }
  elist=xdoc.element_list("dsOverview/relatedResource");
  for (const auto& element : elist) {
    ofs << indent << "  <dc:relation>" << element.content() << " [" << element.attribute_value("url") << "]</dc:relation>" << std::endl;
  }
  e=xdoc.element("dsOverview/restrictions/access");
  if (e.name() == "access") {
    ofs << indent << "  <dc:rights>" << std::endl;
    ofs << summary::convert_html_summary_to_ASCII(e.to_string(),80,4) << std::endl;
    ofs << indent << "  </dc:rights>" << std::endl;
  }
  e=xdoc.element("dsOverview/restrictions/usage");
  if (e.name() == "usage") {
    ofs << indent << "  <dc:rights>" << std::endl;
    ofs << summary::convert_html_summary_to_ASCII(e.to_string(),80,4) << std::endl;
    ofs << indent << "  </dc:rights>" << std::endl;
  }
  ofs << indent << "</oai_dc:dc>" << std::endl;
}

XMLDocument *old_gcmd=nullptr;
void export_to_DIF(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  MySQL::LocalQuery query;
  MySQL::Row row;
  my::map<Entry> resolution_table,format_table;

  const char *res_keywords[]={
  "1 km - &lt; 10 km or approximately .01 degree - &lt; .09 degree",
  "10 km - &lt; 50 km or approximately .09 degree - &lt; .5 degree",
  "50 km - &lt; 100 km or approximately .5 degree - &lt; 1 degree",
  "100 km - &lt; 250 km or approximately 1 degree - &lt; 2.5 degrees",
  "250 km - &lt; 500 km or approximately 2.5 degrees - &lt; 5.0 degrees",
  "500 km - &lt; 1000 km or approximately 5 degrees - &lt; 10 degrees",
  "&gt;= 1000 km or &gt;= 10 degrees",
  "&lt; 1 meter",
  "1 meter - &lt; 30 meters",
  "30 meters - &lt; 100 meters",
  "100 meters - &lt; 250 meters",
  "250 meters - &lt; 500 meters",
  "500 meters - &lt; 1 km"
  };
  TempDir temp_dir;
  if (!temp_dir.create("/tmp")) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    std::cout << "Error creating temporary directory" << std::endl;
    exit(1);
  }
  MySQL::Server server;
  metautils::connect_to_metadata_server(server);
  std::string indent;
  for (size_t n=0; n < indent_length; ++n) {
    indent+=" ";
  }
  auto e=xdoc.element("dsOverview/creator@vocabulary=GCMD");
  auto dss_centername=e.attribute_value("name");
  if (strutils::contains(dss_centername,"/SCD/")) {
    strutils::replace_all(dss_centername,"SCD","CISL");
  }
  ofs << indent << "<DIF xmlns=\"http://gcmd.gsfc.nasa.gov/Aboutus/xml/dif/\"" << std::endl;
  ofs << indent << "     xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << indent << "     xsi:schemaLocation=\"http://gcmd.gsfc.nasa.gov/Aboutus/xml/dif/" << std::endl;
  ofs << indent << "                         http://gcmd.gsfc.nasa.gov/Aboutus/xml/dif/dif_v9.7.1.xsd\">" << std::endl;
  ofs << indent << "  <Entry_ID>NCAR_DS" << dsnum << "</Entry_ID>" << std::endl;
  e=xdoc.element("dsOverview/title");
  auto dstitle=e.content();
  ofs << indent << "  <Entry_Title>" << dstitle << "</Entry_Title>" << std::endl;
  ofs << indent << "  <Data_Set_Citation>" << std::endl;
  ofs << indent << "    <Dataset_Creator>" << dss_centername << "</Dataset_Creator>" << std::endl;
  ofs << indent << "    <Dataset_Title>" << dstitle << "</Dataset_Title>" << std::endl;
  ofs << indent << "    <Dataset_Publisher>" << dss_centername << "</Dataset_Publisher>" << std::endl;
  ofs << indent << "    <Online_Resource>https://rda.ucar.edu/datasets/ds" << dsnum << "/</Online_Resource>" << std::endl;
  ofs << indent << "  </Data_Set_Citation>" << std::endl;
  auto elist=xdoc.element_list("dsOverview/contact");
  for (const auto& element : elist) {
    ofs << indent << "  <Personnel>" << std::endl;
    ofs << indent << "    <Role>Technical Contact</Role>" << std::endl;
    auto contact_name=element.content();
    auto idx=contact_name.find(" ");
    auto fname=contact_name.substr(0,idx);
    auto lname=contact_name.substr(idx+1);
    query.set("logname,phoneno","dssdb.dssgrp","fstname = '"+fname+"' and lstname = '"+lname+"'");
    if (query.submit(server) < 0) {
        std::cout << "Content-type: text/plain" << std::endl << std::endl;
        std::cout << "Database error: " << query.error() << std::endl;
        exit(1);
    }
    query.fetch_row(row);
    ofs << indent << "    <First_Name>" << fname << "</First_Name>" << std::endl;
    ofs << indent << "    <Last_Name>" << lname << "</Last_Name>" << std::endl;
    ofs << indent << "    <Email>" << row[0] << "@ucar.edu</Email>" << std::endl;
    ofs << indent << "    <Phone>" << row[1] << "</Phone>" << std::endl;
    ofs << indent << "    <Fax>(303)-497-1291</Fax>" << std::endl;
    ofs << indent << "    <Contact_Address>" << std::endl;
    ofs << indent << "      <Address>National Center for Atmospheric Research</Address>" << std::endl;
    ofs << indent << "      <Address>CISL/DSS</Address>" << std::endl;
    ofs << indent << "      <Address>P.O. Box 3000</Address>" << std::endl;
    ofs << indent << "      <City>Boulder</City>" << std::endl;
    ofs << indent << "      <Province_or_State>CO</Province_or_State>" << std::endl;
    ofs << indent << "      <Postal_Code>80307</Postal_Code>" << std::endl;
    ofs << indent << "      <Country>U.S.A.</Country>" << std::endl;
    ofs << indent << "    </Contact_Address>" << std::endl;
    ofs << indent << "  </Personnel>" << std::endl;
  }
  ofs << indent << "  <Personnel>" << std::endl;
  ofs << indent << "    <Role>DIF Author</Role>" << std::endl;
  ofs << indent << "    <First_Name>Bob</First_Name>" << std::endl;
  ofs << indent << "    <Last_Name>Dattore</Last_Name>" << std::endl;
  ofs << indent << "    <Email>dattore@ucar.edu</Email>" << std::endl;
  ofs << indent << "    <Phone>(303)-497-1825</Phone>" << std::endl;
  ofs << indent << "    <Fax>(303)-497-1291</Fax>" << std::endl;
  ofs << indent << "    <Contact_Address>" << std::endl;
  ofs << indent << "      <Address>National Center for Atmospheric Research</Address>" << std::endl;
  ofs << indent << "      <Address>CISL/DSS</Address>" << std::endl;
  ofs << indent << "      <Address>P.O. Box 3000</Address>" << std::endl;
  ofs << indent << "      <City>Boulder</City>" << std::endl;
  ofs << indent << "      <Province_or_State>CO</Province_or_State>" << std::endl;
  ofs << indent << "      <Postal_Code>80307</Postal_Code>" << std::endl;
  ofs << indent << "      <Country>U.S.A.</Country>" << std::endl;
  ofs << indent << "    </Contact_Address>" << std::endl;
  ofs << indent << "  </Personnel>" << std::endl;
  query.set("select g.path from search.variables_new as v left join search.GCMD_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"+dsnum+"' and v.vocabulary = 'GCMD'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    std::cout << "Database error: " << query.error() << std::endl;
    exit(1);
  }
  while (query.fetch_row(row)) {
    auto parts=strutils::split(row[0]," > ");
    ofs << indent << "  <Parameters>" << std::endl;
    ofs << indent << "    <Category>" << parts[0] << "</Category>" << std::endl;
    ofs << indent << "    <Topic>" << parts[1] << "</Topic>" << std::endl;
    ofs << indent << "    <Term>" << parts[2] << "</Term>" << std::endl;
    ofs << indent << "    <Variable_Level_1>" << parts[3] << "</Variable_Level_1>" << std::endl;
    if (parts.size() > 4) {
	ofs << indent << "    <Variable_Level_2>" << parts[4] << "</Variable_Level_2>" << std::endl;
	if (parts.size() > 5) {
	  ofs << indent << "    <Variable_Level_3>" << parts[5] << "</Variable_Level_3>" << std::endl;
	  if (parts.size() > 6) {
	    ofs << indent << "    <Detailed_Variable>" << parts[6] << "</Detailed_Variable>" << std::endl;
	  }
	}
    }
    ofs << indent << "  </Parameters>" << std::endl;
  }
  e=xdoc.element("dsOverview/topic@vocabulary=ISO");
  ofs << indent << "  <ISO_Topic_Category>" << e.content() << "</ISO_Topic_Category>" << std::endl;
  query.set("select g.path from search.platforms_new as p left join search.GCMD_platforms as g on g.uuid = p.keyword where p.dsid = '"+dsnum+"' and p.vocabulary = 'GCMD'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    std::cout << "Database error: " << query.error() << std::endl;
    exit(1);
  }
  while (query.fetch_row(row)) {
    ofs << indent << "  <Source_Name>" << std::endl;
    auto index=row[0].find(" > ");
    if (index != std::string::npos) {
	ofs << indent << "   <Short_Name>" << row[0].substr(0,index) << "</Short_Name>" << std::endl;
	ofs << indent << "   <Long_Name>" << row[0].substr(index+3) << "</Long_Name>" << std::endl;
    }
    else {
	ofs << indent << "    <Short_Name>" << row[0] << "</Short_Name>" << std::endl;
    }
    ofs << indent << "  </Source_Name>" << std::endl;
  }
  query.set("min(date_start),max(date_end)","dssdb.dsperiod","dsid = 'ds"+dsnum+"' and date_start < '9998-01-01' and date_end < '9998-01-01'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    std::cout << "Database error: " << query.error() << std::endl;
    exit(1);
  }
  if (query.num_rows() > 0) {
    query.fetch_row(row);
    ofs << indent << "  <Temporal_Coverage>" << std::endl;
    ofs << indent << "    <Start_Date>" << row[0] << "</Start_Date>" << std::endl;
    ofs << indent << "    <Stop_Date>" << row[1] << "</Stop_Date>" << std::endl;
    ofs << indent << "  </Temporal_Coverage>" << std::endl;
  }
  e=xdoc.element("dsOverview/continuingUpdate");
  if (e.attribute_value("value") == "no") {
    ofs << indent << "  <Data_Set_Progress>Complete</Data_Set_Progress>" << std::endl;
  }
  else {
    ofs << indent << "  <Data_Set_Progress>In Work</Data_Set_Progress>" << std::endl;
  }
  query.set("select definition,defParams from (select distinct gridDefinition_code from GrML.summary where dsid = '"+dsnum+"') as s left join GrML.gridDefinitions as d on d.code = s.gridDefinition_code");
  query.submit(server);
  if (query.num_rows() > 0) {
    double min_west_lon=9999.,min_south_lat=9999.,max_east_lon=-9999.,max_north_lat=-9999.;
    while (query.fetch_row(row)) {
	double west_lon,south_lat,east_lon,north_lat;
	if (fill_spatial_domain_from_grid_definition(row[0]+"<!>"+row[1],"primeMeridian",west_lon,south_lat,east_lon,north_lat)) {
	  if (west_lon < min_west_lon) {
	    min_west_lon=west_lon;
	  }
	  if (east_lon > max_east_lon) {
	    max_east_lon=east_lon;
	  }
	  if (south_lat < min_south_lat) {
	    min_south_lat=south_lat;
	  }
	  if (north_lat > max_north_lat) {
	    max_north_lat=north_lat;
	  }
	}
	auto sdum=convert_grid_definition(row[0]+"<!>"+row[1]);
	if (strutils::contains(sdum,"&deg; x")) {
	  auto sp=strutils::split(sdum," x ");
	  sdum=sp[1];
	  sdum=sdum.substr(0,sdum.find("&deg;"));
	  strutils::replace_all(sdum,"~","");
	  add_to_resolution_table(std::stof(sdum),std::stof(strutils::substitute(sp[0],"&deg;","")),"degrees",resolution_table);
	}
	else if (strutils::contains(sdum,"km x")) {
	  auto sp=strutils::split(sdum," x ");
	  sdum=sp[1];
	  sdum=sdum.substr(0,sdum.find("km"));
	  add_to_resolution_table(std::stof(sdum),std::stof(strutils::substitute(sp[0],"km","")),"km",resolution_table);
	}
    }
    if (max_north_lat > -999.) {
	ofs << indent << "  <Spatial_Coverage>" << std::endl;
	ofs << indent << "    <Southernmost_Latitude>" << fabs(min_south_lat);
	if (min_south_lat < 0.) {
	  ofs << " South";
	}
	else {
	  ofs << " North";
	}
	ofs << "</Southernmost_Latitude>" << std::endl;
	ofs << indent << "    <Northernmost_Latitude>" << fabs(max_north_lat);
	if (max_north_lat < 0.) {
	  ofs << " South";
	}
	else {
	  ofs << " North";
	}
	ofs << "</Northernmost_Latitude>" << std::endl;
	if (min_west_lon < -180.) {
	  min_west_lon+=360.;
	}
	ofs << indent << "    <Westernmost_Longitude>" << fabs(min_west_lon);
	if (min_west_lon < 0.) {
	  ofs << " West";
	}
	else {
	  ofs << " East";
	}
	ofs << "</Westernmost_Longitude>" << std::endl;
	if (max_east_lon > 180.) {
	  max_east_lon-=360.;
	}
	ofs << indent << "    <Easternmost_Longitude>" << fabs(max_east_lon);
	if (max_east_lon < 0.) {
	  ofs << " West";
	}
	else {
	  ofs << " East";
	}
	ofs << "</Easternmost_Longitude>" << std::endl;
	ofs << indent << "  </Spatial_Coverage>" << std::endl;
    }
    for (const auto& key : resolution_table.keys()) {
	ofs << indent << "  <Data_Resolution>" << std::endl;
	Entry entry;
	resolution_table.found(key,entry);
	auto sp=strutils::split(entry.key,"<!>");
	ofs << indent << "    <Latitude_Resolution>" << entry.min_lat << " " << sp[1] << "</Latitude_Resolution>" << std::endl;
	ofs << indent << "    <Longitude_Resolution>" << entry.min_lon << " " << sp[1] << "</Longitude_Resolution>" << std::endl;
	ofs << indent << "    <Horizontal_Resolution_Range>" << res_keywords[std::stoi(sp[0])] << "</Horizontal_Resolution_Range>" << std::endl;
	ofs << indent << "  </Data_Resolution>" << std::endl;
    }
  }
/*
    query.set("keyword","search.time_resolutions","dsid = '"+dsnum+"' and vocabulary = 'GCMD'");
    if (query.submit(server) < 0) {
	std::cout << "Content-type: text/plain" << std::endl << std::endl;
	std::cout << "Database error: " << query.error() << std::endl;
	exit(1);
    }
    if (query.num_rows() > 0) {
	while (query.fetch_row(row)) {
	ofs << "  <Data_Resolution>" << std::endl;
	ofs << "  </Data_Resolution>" << std::endl;
    }
*/
  elist=xdoc.element_list("dsOverview/project@vocabulary=GCMD");
  for (const auto& element : elist) {
    ofs << indent << "  <Project>" << std::endl;
    auto project=element.content();
    if (strutils::contains(project," > ")) {
      ofs << indent << "   <Short_Name>" << project.substr(0,project.find(" > ")) << "</Short_Name>" << std::endl;
      ofs << indent << "   <Long_Name>" << project.substr(project.find(" > ")+3) << "</Long_Name>" << std::endl;
    }
    else {
      ofs << indent << "    <Short_Name>" << project << "</Short_Name>" << std::endl;
    }
    ofs << indent << "  </Project>" << std::endl;
  }
  e=xdoc.element("dsOverview/restrictions/access");
  if (e.name() == "access") {
    auto access_restrictions=e.to_string();
    ofs << indent << "  <Access_Constraints>" << std::endl;
    ofs << summary::convert_html_summary_to_ASCII(access_restrictions,80,indent_length+4) << std::endl;
    ofs << indent << "  </Access_Constraints>" << std::endl;
  }
  e=xdoc.element("dsOverview/restrictions/usage");
  if (e.name() == "usage") {
    auto usage_restrictions=e.to_string();
    ofs << indent << "  <Use_Constraints>" << std::endl;
    ofs << summary::convert_html_summary_to_ASCII(usage_restrictions,80,indent_length+4) << std::endl;
    ofs << indent << "  </Use_Constraints>" << std::endl;
  }
  ofs << indent << "  <Data_Set_Language>English</Data_Set_Language>" << std::endl;
  ofs << indent << "  <Data_Center>" << std::endl;
  ofs << indent << "    <Data_Center_Name>" << std::endl;
  ofs << indent << "      <Short_Name>" << dss_centername.substr(0,dss_centername.find(" > ")) << "</Short_Name>" << std::endl;
  ofs << indent << "      <Long_Name>" << dss_centername.substr(dss_centername.find(" > ")+3) << "</Long_Name>" << std::endl;
  ofs << indent << "    </Data_Center_Name>" << std::endl;
  ofs << indent << "    <Data_Center_URL>https://rda.ucar.edu/</Data_Center_URL>" << std::endl;
  ofs << indent << "    <Data_Set_ID>" << dsnum << "</Data_Set_ID>" << std::endl;
  ofs << indent << "    <Personnel>" << std::endl;
  ofs << indent << "      <Role>DATA CENTER CONTACT</Role>" << std::endl;
  ofs << indent << "      <Last_Name>DSS Help Desk</Last_Name>" << std::endl;
  ofs << indent << "      <Email>dssweb@ucar.edu</Email>" << std::endl;
  ofs << indent << "      <Phone>(303)-497-1231</Phone>" << std::endl;
  ofs << indent << "      <Fax>(303)-497-1291</Fax>" << std::endl;
  ofs << indent << "      <Contact_Address>" << std::endl;
  ofs << indent << "        <Address>National Center for Atmospheric Research</Address>" << std::endl;
  ofs << indent << "        <Address>CISL/DSS</Address>" << std::endl;
  ofs << indent << "        <Address>P.O. Box 3000</Address>" << std::endl;
  ofs << indent << "        <City>Boulder</City>" << std::endl;
  ofs << indent << "        <Province_or_State>CO</Province_or_State>" << std::endl;
  ofs << indent << "        <Postal_Code>80307</Postal_Code>" << std::endl;
  ofs << indent << "        <Country>U.S.A.</Country>" << std::endl;
  ofs << indent << "      </Contact_Address>" << std::endl;
  ofs << indent << "    </Personnel>" << std::endl;
  ofs << indent << "  </Data_Center>" << std::endl;
  auto distribution_size=primary_size(dsnum,server);
  if (!distribution_size.empty()) {
    ofs << indent << "  <Distribution>" << std::endl;
    ofs << indent << "    <Distribution_Size>" << distribution_size << "</Distribution_Size>" << std::endl;
    ofs << indent << "  </Distribution>" << std::endl;
  }
  query.set("keyword","search.formats","dsid = '"+dsnum+"'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    std::cout << "Database error: " << query.error() << std::endl;
    exit(1);
  }
  struct stat buf;
  std::string format_ref;
  if (stat((directives.server_root+"/web/metadata/FormatReferences.xml.new").c_str(),&buf) == 0) {
    format_ref=directives.server_root+"/web/metadata/FormatReferences.xml.new";
  }
  else {
    format_ref=remote_web_file("https://rda.ucar.edu/metadata/FormatReferences.xml.new",temp_dir.name());
  }
  XMLDocument fdoc(format_ref);
  while (query.fetch_row(row)) {
    auto format=row[0];
    strutils::replace_all(format,"proprietary_","");
    e=fdoc.element(("formatReferences/format@name="+format+"/DIF").c_str());
    if (e.name() != "DIF") {
	e=fdoc.element(("formatReferences/format@name="+format+"/type").c_str());
    }
    Entry entry;
    entry.key=e.content();
    if (!format_table.found(entry.key,entry)) {
	format_table.insert(entry);
    }
  }
  fdoc.close();
  for (const auto& key : format_table.keys()) {
    ofs << indent << "  <Distribution>" << std::endl;
    ofs << indent << "    <Distribution_Format>" << key << "</Distribution_Format>" << std::endl;
    ofs << indent << "  </Distribution>" << std::endl;
  }
  elist=xdoc.element_list("dsOverview/reference");
  if (elist.size() > 0) {
    std::vector<XMLElement> earray;
    earray.reserve(elist.size());
    for (const auto& element : elist) {
	earray.emplace_back(element);
    }
    binary_sort(earray,compare_references);
    ofs << indent << "  <Reference>" << std::endl;
    for (size_t n=0; n < elist.size(); ++n) {
	if (n > 0) {
	  ofs << std::endl;
	}
	auto reference="<reference><p>"+earray[n].element("authorList").content()+", "+earray[n].element("year").content()+": "+earray[n].element("title").content()+". ";
	auto ref_type=earray[n].attribute_value("type");
	if (ref_type == "journal") {
	  e=earray[n].element("periodical");
	  reference+=e.content()+", "+e.attribute_value("number")+", "+e.attribute_value("pages");
	}
	else if (ref_type == "preprint") {
	  e=earray[n].element("conference");
	  reference+=" Proceedings of the "+e.content()+", "+e.attribute_value("host")+", "+e.attribute_value("location")+", "+e.attribute_value("pages");
	}
	else if (ref_type == "technical_report") {
	  e=earray[n].element("organization");
	  reference+=e.attribute_value("reportID")+", "+e.content()+", "+e.attribute_value("pages")+" pp.";
	}
	else if (ref_type == "book") {
	  e=earray[n].element("publisher");
	  reference+=e.content()+", "+e.attribute_value("place");
	}
	auto doi=earray[n].element("doi").content();
	if (!doi.empty()) {
	  reference+=" (DOI: "+doi+")";
	}
	auto url=earray[n].element("url").content();
	if (!url.empty()) {
	  reference+=", URL: "+url;
	}
	reference+=".</p></reference>";
	ofs << summary::convert_html_summary_to_ASCII(reference,80,indent_length+4) << std::endl;
    }
    ofs << indent << "  </Reference>" << std::endl;
  }
  ofs << indent << "  <Summary>" << std::endl;
  e=xdoc.element("dsOverview/summary");
  ofs << summary::convert_html_summary_to_ASCII(e.to_string(),80,indent_length+4) << std::endl;
  ofs << indent << "  </Summary>" << std::endl;
  elist=xdoc.element_list("dsOverview/relatedDataset");
  for (const auto& element : elist) {
    ofs << indent << "  <Related_URL>" << std::endl;
    ofs << indent << "    <URL_Content_Type>" << std::endl;
    ofs << indent << "      <Type>VIEW RELATED INFORMATION</Type>" << std::endl;
    ofs << indent << "    </URL_Content_Type>" << std::endl;
    ofs << indent << "    <URL>https://rda.ucar.edu/datasets/ds" << element.attribute_value("ID") << "/</URL>" << std::endl;
    ofs << indent << "  </Related_URL>" << std::endl;
  }
  elist=xdoc.element_list("dsOverview/relatedResource");
  for (const auto& element : elist) {
    ofs << indent << "  <Related_URL>" << std::endl;
    ofs << indent << "    <URL_Content_Type>" << std::endl;
    ofs << indent << "      <Type>VIEW RELATED INFORMATION</Type>" << std::endl;
    ofs << indent << "    </URL_Content_Type>" << std::endl;
    ofs << indent << "    <URL>" << element.attribute_value("url") << "</URL>" << std::endl;
    ofs << indent << "    <Description>" << element.content() << "</Description>" << std::endl;
    ofs << indent << "  </Related_URL>" << std::endl;
  }
  ofs << indent << "  <IDN_Node>" << std::endl;
  ofs << indent << "    <Short_Name>USA/NCAR</Short_Name>" << std::endl;
  ofs << indent << "  </IDN_Node>" << std::endl;
  ofs << indent << "  <Metadata_Name>CEOS IDN DIF</Metadata_Name>" << std::endl;
  ofs << indent << "  <Metadata_Version>9.7</Metadata_Version>" << std::endl;
  ofs << indent << "</DIF>" << std::endl;                    
  server.disconnect();
}

void export_to_DataCite(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  MySQL::Server server;
  metautils::connect_to_metadata_server(server);
  std::string indent;
  for (size_t n=0; n < indent_length; ++n) {
    indent+=" ";
  }
  ofs << indent << "<resource xmlns=\"http://datacite.org/schema/kernel-2.2\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://datacite.org/schema/kernel-2.2 http://schema.datacite.org/meta/kernel-2.2/metadata.xsd\">" << std::endl;
  MySQL::LocalQuery query;
  query.set("doi","dssdb.dsvrsn","dsid = 'ds"+dsnum+"' and isnull(end_date)");
  MySQL::Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << indent << "  <identifier identifierType=\"DOI\">" << row[0] << "</identifier>" << std::endl;
  }
  XMLElement e=xdoc.element("dsOverview/title");
  ofs << indent << "  <creators>" << std::endl;
  ofs << indent << "    <creator>YYYYY, XXXXX</creator>" << std::endl;
  ofs << indent << "  </creators>" << std::endl;
  ofs << indent << "  <titles>" << std::endl;
  e=xdoc.element("dsOverview/title");
  ofs << indent << "    <title>" << e.content() << "</title>" << std::endl;
  ofs << indent << "  </titles>" << std::endl;
  ofs << indent << "  <publisher>University Corporation For Atmospheric Research (UCAR):National Center for Atmospheric Research (NCAR):Computational and Information Systems Laboratory (CISL):Operations and Services Division (OSD):Data Support Section (DSS)</publisher>" << std::endl;
  e=xdoc.element("dsOverview/publicationDate");
  ofs << indent << "  <publicationYear>" << e.content().substr(0,4) << "</publicationYear>" << std::endl;
  ofs << indent << "  <subjects>" << std::endl;
  query.set("select g.path from search.variables_new as v left join search.GCMD_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"+dsnum+"' and v.vocabulary = 'GCMD'");
  if (query.submit(server) == 0) {
    while (query.fetch_row(row)) {
	ofs << indent << "    <subject subjectScheme=\"GCMD\">" << row[0] << "</subject>" << std::endl;
    }
  }
  ofs << indent << "  </subjects>" << std::endl;
  ofs << indent << "  <contributors>" << std::endl;
  ofs << indent << "  </contributors>" << std::endl;
  query.set("select min(concat(date_start,' ',time_start)),min(start_flag),max(concat(date_end,' ',time_end)),min(end_flag),time_zone from dssdb.dsperiod where dsid = 'ds"+dsnum+"' and date_start > '0000-00-00' and date_start < '3000-01-01' and date_end > '0000-00-00' and date_end < '3000-01-01'");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << indent << "  <dates>" << std::endl;
    ofs << indent << "    <date dateType=\"Valid\">" << summarizeMetadata::set_date_time_string(row[0],row[1],row[4],"T") << " to " << summarizeMetadata::set_date_time_string(row[2],row[3],row[4],"T") << "</date>" << std::endl;
    ofs << indent << "  </dates>" << std::endl;
  }
  ofs << indent << "  <language>eng</language>" << std::endl;
  ofs << indent << "  <resourceType resourceTypeGeneral=\"Dataset\">XXXXX</resourceType>" << std::endl;
  ofs << indent << "  <alternateIdentifiers>" << std::endl;
  ofs << indent << "    <alternateIdentifier alternateIdentifierType=\"Local\">ds" << dsnum << "</alternateIdentifier>" << std::endl;
  ofs << indent << "  </alternateIdentifiers>" << std::endl;
  auto size=primary_size(dsnum,server);
  if (!size.empty()) {
    ofs << indent << "  <relatedIdentifiers>" << std::endl;
    ofs << indent << "  </relatedIdentifiers>" << std::endl;
    ofs << indent << "  <sizes>" << std::endl;
    ofs << indent << "    <size>" << size << "</size>" << std::endl;
    ofs << indent << "  </sizes>" << std::endl;
    query.set("keyword","search.formats","dsid = '"+dsnum+"'");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
	ofs << indent << "  <formats>" << std::endl;
	while (query.fetch_row(row)) {
	  ofs << indent << "    <format>" << strutils::substitute(row[0],"_"," ") << "</format>" << std::endl;
	}
	ofs << indent << "  </formats>" << std::endl;
    }
    ofs << indent << "  <version>XXXXX</version>" << std::endl;
    ofs << indent << "  <rights>XXXXX</rights>" << std::endl;
    ofs << indent << "  <descriptions>" << std::endl;
    e=xdoc.element("dsOverview/summary");
    auto summary=e.to_string();
    ofs << indent << "    <description descriptionType=\"Abstract\">" << std::endl;
    ofs << summary::convert_html_summary_to_ASCII(summary,120,indent_length+6) << std::endl;
    ofs << indent << "    </description>" << std::endl;
    ofs << indent << "  </descriptions>" << std::endl;
  }
  ofs << indent << "</resource>" << std::endl;
  server.disconnect();
}

void export_to_FGDC(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  MySQL::Server server;
  metautils::connect_to_metadata_server(server);
  std::string indent;
  for (size_t n=0; n < indent_length; ++n) {
    indent+=" ";
  }
  ofs << indent << "<metadata xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << indent << "            xsi:schemaLocation=\"http://www.fgdc.gov/schemas/metadata" << std::endl;
  ofs << indent << "                                http://www.fgdc.gov/schemas/metadata/fgdc-std-001-1998.xsd\">" << std::endl;
  ofs << indent << "  <idinfo>" << std::endl;
  ofs << indent << "    <citation>" << std::endl;
  ofs << indent << "      <citeinfo>" << std::endl;
  ofs << indent << "        <origin>" << citation::list_authors(xdoc,3,"et al.") << "</origin>" << std::endl;
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
  ofs << summary::convert_html_summary_to_ASCII(e.to_string(),120,indent_length+8) << std::endl;
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
  std::deque<HResEntry> hres_list;
  my::map<Entry> unique_places_table;
  fill_geographic_extent_data(server,dsnum,xdoc,min_west_lon,min_south_lat,max_east_lon,max_north_lat,is_grid,hres_list,unique_places_table);
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
    ofs << summary::convert_html_summary_to_ASCII(e.to_string(),120,6) << std::endl;
    ofs << indent << "    </accconst>" << std::endl;
  }
  else
    ofs << indent << "    <accconst>None</accconst>" << std::endl;
  e=xdoc.element("dsOverview/restrictions/usage");
  if (e.name() == "usage") {
    ofs << indent << "    <useconst>" << std::endl;
    ofs << summary::convert_html_summary_to_ASCII(e.to_string(),120,6) << std::endl;
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
}

void export_to_ISO19139(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  MySQL::Server server;
  MySQL::LocalQuery query;
  MySQL::Row row;
  hereDoc::Tokens tokens;
  hereDoc::IfEntry ife;
  hereDoc::RepeatEntry re;
  my::map<Entry> unique_places_table;

  metautils::connect_to_metadata_server(server);
  tokens.replaces.emplace_back("__DSNUM__<!>"+dsnum);
  XMLElement e=xdoc.element("dsOverview/timeStamp");
  auto mdate=e.attribute_value("value").substr(0,10);
  query.set("mssdate","dssdb.dataset","dsid = 'ds"+dsnum+"'");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    if (row[0] > mdate) {
	mdate=row[0]+"T00:00:00";
    }
    else {
	mdate="";
    }
  }
  if (mdate.empty()) {
    mdate=e.attribute_value("value");
    mdate[10]='T';
    mdate.erase(19);
  }
  tokens.replaces.emplace_back("__MDATE__<!>"+mdate);
  e=xdoc.element("dsOverview/title");
  tokens.replaces.emplace_back("__TITLE__<!>"+e.content());
  e=xdoc.element("dsOverview/publicationDate");
  auto pub_date=e.content();
  if (pub_date.empty()) {
    query.set("pub_date","search.datasets","dsid = '"+dsnum+"'");
    if (query.submit(server) == 0 && query.fetch_row(row)) {
	pub_date=row[0];
    }
  }
  if (!pub_date.empty()) {
    tokens.replaces.emplace_back("__PUBLICATION_DATE_XML__<!><gmd:date><gco:Date>"+pub_date+"</gco:Date></gmd:date>");
  }
  else {
    tokens.replaces.emplace_back("__PUBLICATION_DATE_XML__<!><gmd:date gco:nilReason=\"unknown\" />");
  }
  query.set("doi","dssdb.dsvrsn","dsid = 'ds"+dsnum+"' and isnull(end_date)");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ife.key="__HAS_DOI__";
    tokens.ifs.insert(ife);
    tokens.replaces.emplace_back("__DOI__<!>"+row[0]);
  }
  else {
    ife.key="__NO_DOI__";
    tokens.ifs.insert(ife);
  }
  std::list<XMLElement> elist=xdoc.element_list("dsOverview/author");
  if (elist.size() > 0) {
    ife.key="__HAS_AUTHOR_PERSONS__";
    tokens.ifs.insert(ife);
    re.key="__AUTHOR_PERSON__";
    re.list.reset(new std::deque<std::string>);
    tokens.repeats.insert(re);
    for (const auto& e : elist) {
	std::string author=e.attribute_value("lname")+", "+e.attribute_value("fname")+" "+e.attribute_value("mname");
	strutils::trim(author);
	re.list->emplace_back(author);
    }
  }
  else {
    query.set("select g.path from search.contributors_new as c left join search.GCMD_providers as g on g.uuid = c.keyword where c.dsid = '"+dsnum+"' and c.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
	ife.key="__HAS_AUTHOR_ORGS__";
	tokens.ifs.insert(ife);
	re.key="__AUTHOR_ORG__";
	re.list.reset(new std::deque<std::string>);
	tokens.repeats.insert(re);
	while (query.fetch_row(row)) {
	  re.list->emplace_back(strutils::substitute(row[0].substr(row[0].find(" > ")+3),"&","&amp;"));
	}
    }
  }
  e=xdoc.element("dsOverview/summary");
  tokens.replaces.emplace_back("__ABSTRACT__<!>"+summary::convert_html_summary_to_ASCII(e.to_string(),32768,0));
//tokens.replaces.emplace_back("__ABSTRACT__<!>"+e.content());
  e=xdoc.element("dsOverview/continuingUpdate");
  std::string frequency;
  if (e.attribute_value("value") == "yes") {
    tokens.replaces.emplace_back("__PROGRESS_STATUS__<!>onGoing");
    frequency=e.attribute_value("frequency");
    if (frequency == "bi-monthly") {
	frequency="monthly";
    }
    else if (frequency == "half-yearly") {
	frequency="biannually";
    }
    else if (frequency == "yearly") {
	frequency="annually";
    }
    else if (frequency == "irregularly") {
	frequency="irregular";
    }
  }
  else {
    tokens.replaces.emplace_back("__PROGRESS_STATUS__<!>completed");
    frequency="notPlanned";
  }
  tokens.replaces.emplace_back("__UPDATE_FREQUENCY__<!>"+frequency);
  e=xdoc.element("dsOverview/logo");
  if (e.name().length() > 0) {
    ife.key="__HAS_LOGO__";
    tokens.ifs.insert(ife);
    tokens.replaces.emplace_back("__LOGO_URL__<!>https://rda.ucar.edu/images/ds_logos/"+e.content());
    tokens.replaces.emplace_back("__LOGO_IMAGE_FORMAT__<!>"+strutils::to_upper(e.content().substr(e.content().rfind(".")+1)));
  }
  elist=xdoc.element_list("dsOverview/contact");
  tokens.replaces.emplace_back("__SPECIALIST_NAME__<!>"+elist.front().content());
  std::deque<std::string> sp=strutils::split(elist.front().content());
  query.set("logname,phoneno","dssdb.dssgrp","fstname = '"+sp[0]+"' and lstname = '"+sp[1]+"'");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    tokens.replaces.emplace_back("__SPECIALIST_PHONE__<!>"+row[1]);
    tokens.replaces.emplace_back("__SPECIALIST_EMAIL__<!>"+row[0]+"@ucar.edu");
  }
  query.set("keyword","search.formats","dsid = '"+dsnum+"'");
  if (query.submit(server) == 0 && query.num_rows() > 0) {
    ife.key="__HAS_FORMATS__";
    tokens.ifs.insert(ife);
    re.key="__FORMAT__";
    re.list.reset(new std::deque<std::string>);
    tokens.repeats.insert(re);
    while (query.fetch_row(row)) {
	re.list->emplace_back(strutils::to_capital(strutils::substitute(row[0],"proprietary_","")));
    }
  }
  query.set("select concept_scheme,version,revision_date from search.GCMD_versions");
  if (query.submit(server) == 0) {
    while (query.fetch_row(row)) {
	auto concept_scheme=strutils::to_upper(row[0]);
	tokens.replaces.emplace_back("__"+concept_scheme+"_VERSION__<!>"+row[1]);
	tokens.replaces.emplace_back("__"+concept_scheme+"_REVISION_DATE__<!>"+row[2]);
    }
    query.set("select g.path from search.platforms_new as p left join search.GCMD_platforms as g on g.uuid = p.keyword where p.dsid = '"+dsnum+"' and p.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
	ife.key="__HAS_PLATFORMS__";
	tokens.ifs.insert(ife);
	re.key="__PLATFORM__";
	re.list.reset(new std::deque<std::string>);
	tokens.repeats.insert(re);
	while (query.fetch_row(row)) {
	  re.list->emplace_back(strutils::substitute(row[0],"&","&amp;"));
	}
    }
    query.set("select g.path from search.instruments_new as i left join search.GCMD_instruments as g on g.uuid = i.keyword where i.dsid = '"+dsnum+"' and i.vocabulary = 'GCMD'");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
	ife.key="__HAS_INSTRUMENTS__";
	tokens.ifs.insert(ife);
	re.key="__INSTRUMENT__";
	re.list.reset(new std::deque<std::string>);
	tokens.repeats.insert(re);
	while (query.fetch_row(row)) {
	  re.list->emplace_back(row[0]);
	}
    }
//    query.set("select g.path from search.projects_new as p left join search.GCMD_projects as g on g.uuid = p.keyword where p.dsid = '"+dsnum+"' and p.vocabulary = 'GCMD'");
query.set("select distinct g.path from (select keyword from search.projects_new where dsid = '"+dsnum+"' and vocabulary = 'GCMD' union select keyword from search.supportedProjects_new where dsid = '"+dsnum+"' and vocabulary = 'GCMD') as p left join search.GCMD_projects as g on g.uuid = p.keyword");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
	ife.key="__HAS_PROJECTS__";
	tokens.ifs.insert(ife);
	re.key="__PROJECT__";
	re.list.reset(new std::deque<std::string>);
	tokens.repeats.insert(re);
	while (query.fetch_row(row)) {
	  re.list->emplace_back(row[0]);
	}
    }
    query.set("select g.path from search.variables_new as v left join search.GCMD_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"+dsnum+"' and v.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
	re.key="__SCIENCEKEYWORD__";
	re.list.reset(new std::deque<std::string>);
	tokens.repeats.insert(re);
	while (query.fetch_row(row)) {
	  re.list->emplace_back(row[0]);
	}
    }
  }
  e=xdoc.element("dsOverview/topic@vocabulary=ISO");
  tokens.replaces.emplace_back("__ISO_TOPIC__<!>"+e.content());
  double min_west_lon,min_south_lat,max_east_lon,max_north_lat;
  bool is_grid;
  std::deque<HResEntry> hres_list;
  fill_geographic_extent_data(server,dsnum,xdoc,min_west_lon,min_south_lat,max_east_lon,max_north_lat,is_grid,hres_list,unique_places_table);
  if (is_grid) {
    ife.key="__IS_GRID__";
    tokens.ifs.insert(ife);
  }
  if (min_west_lon < 9999.) {
    if (myequalf(min_west_lon,max_east_lon) && myequalf(min_south_lat,max_north_lat)) {
	ife.key="__HAS_POINT__";
	tokens.replaces.emplace_back("__LAT__<!>"+strutils::ftos(min_south_lat,10));
	tokens.replaces.emplace_back("__LON__<!>"+strutils::ftos(min_west_lon,10));
    }
    else {
	ife.key="__HAS_BOUNDING_BOX__";
	tokens.replaces.emplace_back("__WEST_LON__<!>"+strutils::ftos(min_west_lon,10));
	tokens.replaces.emplace_back("__EAST_LON__<!>"+strutils::ftos(max_east_lon,10));
	tokens.replaces.emplace_back("__SOUTH_LAT__<!>"+strutils::ftos(min_south_lat,10));
	tokens.replaces.emplace_back("__NORTH_LAT__<!>"+strutils::ftos(max_north_lat,10));
    }
    tokens.ifs.insert(ife);
  }
  if (unique_places_table.size() > 0) {
    ife.key="__HAS_GEOGRAPHIC_IDENTIFIERS__";
    tokens.ifs.insert(ife);
    re.key="__GEOGRAPHIC_IDENTIFIER__";
    re.list.reset(new std::deque<std::string>);
    tokens.repeats.insert(re);
    for (const auto& key : unique_places_table.keys()) {
	re.list->emplace_back(key);
    }
  }
  query.set("select min(date_start),min(time_start),max(date_end),max(time_end),min(start_flag),any_value(time_zone) from dssdb.dsperiod where dsid = 'ds"+dsnum+"' and date_start < '9998-01-01' and date_end < '9998-01-01' group by dsid");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    size_t max_parts=std::stoi(row[4]);
    auto parts=strutils::split(row[0],"-");
    std::string s;
    size_t m= (max_parts < 3) ? max_parts : 3;
    for (size_t n=0; n < m; ++n) {
	if (n > 0) {
	  s+="-";
	}
	s+=parts[n];
    }
    if (max_parts > 3) {
	s+="T";
	if (std::regex_search(row[1],std::regex("^31:"))) {
	  parts=strutils::split("00:00:00",":");
	}
	else {
	  parts=strutils::split(row[1],":");
	}
	auto m=max_parts-3;
	for (size_t n=0; n < 3; ++n) {
	  if (n > 0) {
	    s+=":";
	  }
	  if (n < m) {
	    s+=parts[n];
	  }
	  else {
	    s+="00";
	  }
	}
	if (row[5] == "+0000") {
	  s+="Z";
	}
	else {
	  s+=row[5].substr(0,3)+":"+row[5].substr(3);
	}
    }
    tokens.replaces.emplace_back("__BEGIN_DATE__<!>"+s);
    parts=strutils::split(row[2],"-");
    s="";
    m= (max_parts < 3) ? max_parts : 3;
    for (size_t n=0; n < m; ++n) {
	if (n > 0) {
	  s+="-";
	}
	s+=parts[n];
    }
    if (max_parts > 3) {
	s+="T";
	if (std::regex_search(row[3],std::regex("^31:"))) {
	  parts=strutils::split("23:59:59",":");
	}
	else {
	  parts=strutils::split(row[3],":");
	}
	auto m=max_parts-3;
	for (size_t n=0; n < 3; ++n) {
	  if (n > 0) {
	    s+=":";
	  }
	  if (n < m) {
	    s+=parts[n];
	  }
	  else {
	    s+="00";
	  }
	}
	if (row[5] == "+0000") {
	  s+="Z";
	}
	else {
	  s+=row[5].substr(0,3)+":"+row[5].substr(3);
	}
    }
    tokens.replaces.emplace_back("__END_DATE__<!>"+s);
  }
  e=xdoc.element("dsOverview/restrictions/usage");
  if (!e.name().empty()) {
    ife.key="__HAS_USAGE_RESTRICTIONS__";
    tokens.ifs.insert(ife);
    tokens.replaces.emplace_back("__USAGE_RESTRICTIONS__<!>"+summary::convert_html_summary_to_ASCII(e.to_string(),32768,0));
  }
  else {
    ife.key="__NO_USAGE_RESTRICTIONS__";
    tokens.ifs.insert(ife);
  }
  e=xdoc.element("dsOverview/restrictions/access");
  if (!e.name().empty()) {
    ife.key="__HAS_ACCESS_RESTRICTIONS__";
    tokens.ifs.insert(ife);
    tokens.replaces.emplace_back("__ACCESS_RESTRICTIONS__<!>"+summary::convert_html_summary_to_ASCII(e.to_string(),32768,0));
  }
  else {
    ife.key="__NO_ACCESS_RESTRICTIONS__";
    tokens.ifs.insert(ife);
  }
  hereDoc::print("/usr/local/www/server_root/web/html/oai/iso19139.xml",&tokens,ofs,indent_length);
}

void export_to_ISO19115_dash_3(std::unique_ptr<TokenDocument>& token_doc,std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  if (token_doc == nullptr) {
    token_doc.reset(new TokenDocument("/usr/local/www/server_root/web/html/oai/iso19115-3.xml"));
  }
  XMLElement e=xdoc.element("dsOverview/timeStamp");
  auto mdate=e.attribute_value("value").substr(0,10);
  MySQL::Server server;
  metautils::connect_to_metadata_server(server);
  MySQL::LocalQuery query;
  query.set("mssdate","dssdb.dataset","dsid = 'ds"+dsnum+"'");
  MySQL::Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    if (row[0] > mdate) {
	mdate=row[0];
    }
  }
  token_doc->add_replacement("__MDATE__",mdate);
  token_doc->add_replacement("__DSNUM__",dsnum);
  add_replace_from_element_content(xdoc,"dsOverview/title","__TITLE__",*token_doc);
  e=xdoc.element("dsOverview/publicationDate");
  auto pub_date=e.content();
  if (pub_date.empty()) {
    query.set("pub_date","search.datasets","dsid = '"+dsnum+"'");
    if (query.submit(server) == 0 && query.fetch_row(row)) {
	pub_date=row[0];
    }
  }
  if (!pub_date.empty()) {
    token_doc->add_if("__HAS_PUBLICATION_DATE__");
    token_doc->add_replacement("__PUBLICATION_DATE__",pub_date);
  }
  query.set("doi","dssdb.dsvrsn","dsid = 'ds"+dsnum+"' and isnull(end_date)");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    token_doc->add_if("__HAS_DOI__");
    token_doc->add_replacement("__DOI__",row[0]);
  }
  auto elist=xdoc.element_list("dsOverview/author");
  if (elist.size() > 0) {
    for (const auto& e : elist) {
	auto author=e.attribute_value("lname")+", "+e.attribute_value("fname")+" "+e.attribute_value("mname");
	strutils::trim(author);
	token_doc->add_repeat("__AUTHOR__",author);
    }
  }
  else {
    elist=xdoc.element_list("dsOverview/contributor@vocabulary=GCMD");
    for (const auto& e : elist) {
	if (e.attribute_value("citable").length() == 0 || e.attribute_value("citable") == "yes") {
	  auto author=e.attribute_value("name");
	  token_doc->add_repeat("__AUTHOR__",author.substr(author.find(" > ")+3));
	}
    }
  }
  add_replace_from_element_content(xdoc,"dsOverview/summary","__ABSTRACT__",*token_doc);
  e=xdoc.element("dsOverview/continuingUpdate");
  std::string frequency;
  if (e.attribute_value("value") == "yes") {
    token_doc->add_replacement("__PROGRESS_STATUS__","onGoing");
    frequency=e.attribute_value("frequency");
    if (frequency == "bi-monthly") {
	frequency="monthly";
    }
    else if (frequency == "half-yearly") {
	frequency="biannually";
    }
    else if (frequency == "yearly") {
	frequency="annually";
    }
    else if (frequency == "irregularly") {
	frequency="irregular";
    }
  }
  else {
    token_doc->add_replacement("__PROGRESS_STATUS__","completed");
    frequency="notPlanned";
  }
  token_doc->add_replacement("__UPDATE_FREQUENCY__",frequency);
  elist=xdoc.element_list("dsOverview/contact");
  token_doc->add_replacement("__SPECIALIST_NAME__",elist.front().content());
  auto specparts=strutils::split(elist.front().content());
  query.set("logname,phoneno,officeno","dssdb.dssgrp","fstname = '"+specparts[0]+"' and lstname = '"+specparts[1]+"'");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    token_doc->add_replacement("__SPECIALIST_PHONE__",row[1]);
    token_doc->add_replacement("__SPECIALIST_OFFICE__","ML-"+row[2]);
    token_doc->add_replacement("__SPECIALIST_EMAIL__",row[0]+"@ucar.edu");
  }
  double min_west_lon,min_south_lat,max_east_lon,max_north_lat;
  bool is_grid=false;
  std::deque<HResEntry> hres_list;
  my::map<Entry> unique_places_table;
  fill_geographic_extent_data(server,dsnum,xdoc,min_west_lon,min_south_lat,max_east_lon,max_north_lat,is_grid,hres_list,unique_places_table);
  if (is_grid) {
    token_doc->add_if("__IS_GRID__");
  }
  if (!hres_list.empty()) {
    token_doc->add_if("__HAS_SPATIAL_RESOLUTION__");
    for (const auto& h : hres_list) {
	token_doc->add_repeat("__SPATIAL_RESOLUTION__","UOM[!]"+h.uom+"<!>RES[!]"+strutils::dtos(h.res,10));
    }
  }
  auto has_any_extent=false;
  if (min_west_lon < 9999.) {
    token_doc->add_if("__HAS_BOUNDING_BOX__");
    token_doc->add_replacement("__WEST_LON__",strutils::dtos(min_west_lon,10));
    token_doc->add_replacement("__EAST_LON__",strutils::dtos(max_east_lon,10));
    token_doc->add_replacement("__SOUTH_LAT__",strutils::dtos(min_south_lat,10));
    token_doc->add_replacement("__NORTH_LAT__",strutils::dtos(max_north_lat,10));
    has_any_extent=true;
  }
  if (unique_places_table.size() > 0) {
    token_doc->add_if("__HAS_GEOGRAPHIC_IDENTIFIERS__");
    for (const auto& key : unique_places_table.keys()) {
	token_doc->add_repeat("__GEOGRAPHIC_IDENTIFIER__",key);
    }
    has_any_extent=true;
  }
  query.set("select min(date_start),min(time_start),max(date_end),max(time_end),min(start_flag),any_value(time_zone) from dssdb.dsperiod where dsid = 'ds"+dsnum+"' and date_start < '9998-01-01' and date_end < '9998-01-01' group by dsid");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    size_t max_parts=std::stoi(row[4]);
    size_t m= (max_parts < 3) ? max_parts : 3;
    for (size_t l=0; l < 2; ++l) {
	auto parts=strutils::split(row[l*2],"-");
	std::string s;
	for (size_t n=0; n < m; ++n) {
	  if (n > 0) {
	    s+="-";
	  }
	  s+=parts[n];
	}
	if (max_parts > 3) {
	  s+="T";
	  parts=strutils::split(row[l*2+1],":");
	  auto m=max_parts-3;
	  for (size_t n=0; n < 3; ++n) {
	    if (n > 0) {
		s+=":";
	    }
	    if (n < m) {
		s+=parts[n];
	    }
	    else {
		s+="00";
	    }
	  }
	  if (row[5] == "+0000") {
	    s+="Z";
	  }
	  else {
	    s+=row[5].substr(0,3)+":"+row[5].substr(3);
	  }
	}
	if (l == 0) {
	  token_doc->add_replacement("__BEGIN_DATE_TIME__",s);
	  token_doc->add_if("__HAS_TEMPORAL_EXTENT__");
	  has_any_extent=true;
	}
	else {
	  token_doc->add_replacement("__END_DATE_TIME__",s);
	}
    }
  }
  if (has_any_extent) {
    token_doc->add_if("__HAS_ANY_EXTENT__");
  }
  elist=xdoc.element_list("dsOverview/reference@type=journal");
  if (elist.size() > 0) {
    token_doc->add_if("__HAS_PUBLICATIONS__");
    for (const auto& e : elist) {
	auto alist=strutils::split(e.element("authorList").content(),",");
	auto p=e.element("periodical");
	auto rval="TITLE[!]"+e.element("title").content()+"<!>YEAR[!]"+e.element("year").content()+"<!>AUTHOR[!]"+alist[0];
	if (alist.size() > 1) {
	  rval+=", "+alist[1];
	}
	rval+="<!>NAME[!]"+p.content()+"<!>ISSUE[!]"+p.attribute_value("number")+"<!>PAGES[!]"+p.attribute_value("pages");
	for (size_t n=2; n < alist.size(); ++n) {
	  rval+="<!>COAUTHOR[!]"+alist[n];
	}
	token_doc->add_repeat("__PUBLICATION__",rval);
    }
  }
  e=xdoc.element("dsOverview/logo");
  if (e.name().length() > 0) {
    token_doc->add_if("__HAS_LOGO__");
    token_doc->add_replacement("__LOGO_URL__","https://rda.ucar.edu/images/ds_logos/"+e.content());
    token_doc->add_replacement("__LOGO_IMAGE_FORMAT__",strutils::to_upper(e.content().substr(e.content().rfind(".")+1)));
  }
  query.set("keyword","search.formats","dsid = '"+dsnum+"'");
  if (query.submit(server) == 0 && query.num_rows() > 0) {
    token_doc->add_if("__HAS_FORMATS__");
    while (query.fetch_row(row)) {
	token_doc->add_repeat("__FORMAT__",strutils::to_capital(strutils::substitute(row[0],"proprietary_","")));
    }
  }
  query.set("select concept_scheme,version,revision_date from search.GCMD_versions");
  if (query.submit(server) == 0) {
    while (query.fetch_row(row)) {
	auto concept_scheme=strutils::to_upper(row[0]);
	token_doc->add_replacement("__"+concept_scheme+"_VERSION__",row[1]);
//	token_doc->add_replacement("__"+concept_scheme+"_REVISION_DATE__",row[2]);
    }
    query.set("select g.path from search.variables_new as v left join search.GCMD_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"+dsnum+"' and v.vocabulary = 'GCMD'");
    if (query.submit(server) == 0) {
	while (query.fetch_row(row)) {
	  token_doc->add_repeat("__SCIENCEKEYWORD__",row[0]);
	}
    }
    query.set("select g.path from search.platforms_new as p left join search.GCMD_platforms as g on g.uuid = p.keyword where p.dsid = '"+dsnum+"' and p.vocabulary = 'GCMD'");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
	token_doc->add_if("__HAS_PLATFORMS__");
	while (query.fetch_row(row)) {
	  token_doc->add_repeat("__PLATFORM__",row[0]);
	}
    }
    query.set("select distinct g.path from (select keyword from search.projects_new where dsid = '"+dsnum+"' and vocabulary = 'GCMD' union select keyword from search.supportedProjects_new where dsid = '"+dsnum+"' and vocabulary = 'GCMD') as p left join search.GCMD_projects as g on g.uuid = p.keyword");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
	token_doc->add_if("__HAS_PROJECTS__");
	while (query.fetch_row(row)) {
	  token_doc->add_repeat("__PROJECT__",row[0]);
	}
    }
    query.set("select g.path from search.instruments_new as i left join search.GCMD_instruments as g on g.uuid = i.keyword where i.dsid = '"+dsnum+"' and i.vocabulary = 'GCMD'");
    if (query.submit(server) == 0 && query.num_rows() > 0) {
	token_doc->add_if("__HAS_INSTRUMENTS__");
	while (query.fetch_row(row)) {
	  token_doc->add_repeat("__INSTRUMENT__",row[0]);
	}
    }
  }
  add_replace_from_element_content(xdoc,"dsOverview/restrictions/access","__ACCESS_RESTRICTIONS__",*token_doc,"__HAS_ACCESS_RESTRICTIONS__");
  add_replace_from_element_content(xdoc,"dsOverview/restrictions/usage","__USAGE_RESTRICTIONS__",*token_doc,"__HAS_USAGE_RESTRICTIONS__");
  elist=xdoc.element_list("dsOverview/relatedResource");
  if (elist.size() > 0) {
    token_doc->add_if("__HAS_RELATED_RESOURCES__");
    for (const auto& e : elist) {
	token_doc->add_repeat("__RELATED_RESOURCE__","TITLE[!]"+e.content()+"<!>URL[!]"+e.attribute_value("url"));
    }
  }
  elist=xdoc.element_list("dsOverview/relatedDataset");
  if (elist.size() > 0) {
    token_doc->add_if("__HAS_RELATED_RESOURCES__");
    for (const auto& e : elist) {
	auto dsid=e.attribute_value("ID");
	query.set("title","search.datasets","dsid = '"+dsid+"'");
	if (query.submit(server) == 0 && query.fetch_row(row)) {
	  token_doc->add_repeat("__RELATED_RESOURCE__","TITLE[!]"+row[0]+"<!>URL[!]https://rda.ucar.edu/datasets/ds"+dsid+"/");
	}
    }
  }
  server.disconnect();
  ofs << *token_doc << std::endl;
}

void add_frequency(my::map<Entry>& frequency_table,size_t frequency,std::string unit)
{
  Entry e;

  if (frequency == 1) {
    e.key=strutils::itos(frequency)+"<!>"+unit;
  }
  else {
    if (unit == "minute") {
	e.key=strutils::itos(lround(60./frequency+0.49))+"<!>second";
    }
    else if (unit == "hour") {
	e.key=strutils::itos(lround(60./frequency+0.49))+"<!>minute";
    }
    else if (unit == "day") {
	e.key=strutils::itos(lround(24./frequency+0.49))+"<!>hour";
    }
    else if (unit == "week") {
	e.key=strutils::itos(lround(7./frequency+0.49))+"<!>day";
    }
    else if (unit == "month") {
	e.key=strutils::itos(lround(30./frequency+0.49))+"<!>day";
    }
  }
  if (e.key.length() > 0 && !frequency_table.found(e.key,e)) {
    frequency_table.insert(e);
  }
}

extern "C" void *run_query(void *ts)
{
  MySQL::Server tserver;
  PThreadStruct *t=(PThreadStruct *)ts;

  metautils::connect_to_metadata_server(tserver);
  t->query.submit(tserver);
  tserver.disconnect();
  return NULL;
}

void export_to_native(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  std::string indent;
  bool found_content_metadata=false;
  MySQL::Server server;
  MySQL::LocalQuery query;
  MySQL::Row row;
  std::string dsnum2=strutils::substitute(dsnum,".","");
  my::map<Entry> frequency_table;
  std::list<std::string> type_list,format_list;
  PThreadStruct pts,gfts,gfmts,ofts,ofmts,dts;
  TempDir temp_dir;
  struct stat buf;

  temp_dir.create("/tmp");
  metautils::connect_to_metadata_server(server);
  for (size_t n=0; n < indent_length; ++n) {
    indent+=" ";
  }
  XMLElement e=xdoc.element("dsOverview");
  ofs << indent << "<dsOverview xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << indent << "            xsi:schemaLocation=\"https://rda.ucar.edu/schemas" << std::endl;
  ofs << indent << "                                https://rda.ucar.edu/schemas/dsOverview.xsd\"" << std::endl;
  ofs << indent << "            ID=\"" << e.attribute_value("ID") << "\" type=\"" << e.attribute_value("type") << "\">" << std::endl;
  e=xdoc.element("dsOverview/timeStamp");
  ofs << indent << "  " << e.to_string() << std::endl;
  query.set("doi","dssdb.dsvrsn","dsid = 'ds"+dsnum+"' and isnull(end_date)");
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << indent << "  <doi>" << row[0] << "</doi>" << std::endl;
  }
  e=xdoc.element("dsOverview/continuingUpdate");
  ofs << indent << "  " << e.to_string() << std::endl;
  e=xdoc.element("dsOverview/access");
  ofs << indent << "  " << e.to_string() << std::endl;
  e=xdoc.element("dsOverview/title");
  ofs << indent << "  " << e.to_string() << std::endl;
  e=xdoc.element("dsOverview/summary");
  auto summary=e.to_string();
  strutils::replace_all(summary,"\n","\n"+indent+"  ");
  ofs << indent << "  " << summary << std::endl;
  e=xdoc.element("dsOverview/creator");
  auto creator=e.to_string();
  if (strutils::contains(creator,"/SCD/")) {
    strutils::replace_all(creator,"SCD","CISL");
  }
  ofs << indent << "  " << creator << std::endl;
  std::list<XMLElement> elist=xdoc.element_list("dsOverview/contributor");
  for (const auto& element : elist) {
    auto contributor=element.to_string();
    if (strutils::contains(contributor,"/SCD/")) {
	strutils::replace_all(contributor,"SCD","CISL");
    }
    ofs << indent << "  " << contributor << std::endl;
  }
  elist=xdoc.element_list("dsOverview/author");
  for (const auto& element : elist) {
    ofs << indent << "  <author>" << element.attribute_value("fname");
    auto author=element.attribute_value("mname");
    if (!author.empty()) {
	ofs << " " << author;
    }
    ofs << " " << element.attribute_value("lname") << "</author>" << std::endl;
  }
  e=xdoc.element("dsOverview/restrictions");
  if (e.name() == "restrictions") {
    ofs << indent << "  " << e.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/variable");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/contact");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/platform");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/project");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/supportsProject");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  e=xdoc.element("dsOverview/topic");
  ofs << indent << "  " << e.to_string() << std::endl;
  elist=xdoc.element_list("dsOverview/keyword");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/reference");
  for (const auto& element : elist) {
    auto reference=element.to_string();
    strutils::replace_all(reference,"\n","\n"+indent+"  ");
    ofs << indent << "  " << reference << std::endl;
  }
  elist=xdoc.element_list("dsOverview/referenceURL");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/relatedResource");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/relatedDataset");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  e=xdoc.element("dsOverview/publicationDate");
  if (e.name() == "publicationDate") {
    ofs << indent << "  " << e.to_string() << std::endl;
  }
  else {
    query.set("pub_date","search.datasets","dsid = "+dsnum);
    if (query.submit(server) == 0 && query.fetch_row(row)) {
	ofs << indent << "  <publicationDate>" << row[0] << "</publicationDate>" << std::endl;
    }
  }
  auto cmd_databases=metautils::CMD_databases("unknown","unknown","unknown");
  for (const auto& database : cmd_databases) {
    if (MySQL::table_exists(server,database+".ds"+dsnum2+"_primaries")) {
	found_content_metadata=true;
    }
  }
  if (found_content_metadata) {
    ofs << indent << "  <contentMetadata>" << std::endl;
    pts.query.set("select min(concat(date_start,' ',time_start)),min(start_flag),max(concat(date_end,' ',time_end)),min(end_flag),time_zone from dssdb.dsperiod where dsid = 'ds"+dsnum+"' group by dsid");
    pthread_create(&pts.tid,NULL,run_query,reinterpret_cast<void *>(&pts));
    gfts.query.set("nsteps_per,unit","GrML.frequencies","dsid = '"+dsnum+"'");
    pthread_create(&gfts.tid,NULL,run_query,reinterpret_cast<void *>(&gfts));
    gfmts.query.set("select distinct f.format from GrML.ds"+dsnum2+"_primaries as p left join GrML.formats as f on f.code = p.format_code");
    pthread_create(&gfmts.tid,NULL,run_query,reinterpret_cast<void *>(&gfmts));
    ofts.query.set("select min(min_obs_per),max(max_obs_per),unit from ObML.ds"+dsnum2+"_frequencies group by unit");
    pthread_create(&ofts.tid,NULL,run_query,reinterpret_cast<void *>(&ofts));
    ofmts.query.set("select distinct f.format from ObML.ds"+dsnum2+"_primaries as p left join ObML.formats as f on f.code = p.format_code");
    pthread_create(&ofmts.tid,NULL,run_query,reinterpret_cast<void *>(&ofmts));
    dts.query.set("select distinct d.definition,d.defParams from GrML.summary as s left join GrML.gridDefinitions as d on d.code = s.gridDefinition_code where s.dsid = '"+dsnum+"'");
    pthread_create(&dts.tid,NULL,run_query,reinterpret_cast<void *>(&dts));
    pthread_join(pts.tid,NULL);
    pthread_join(gfts.tid,NULL);
    pthread_join(gfmts.tid,NULL);
    pthread_join(ofts.tid,NULL);
    pthread_join(ofmts.tid,NULL);
    pthread_join(dts.tid,NULL);
    if (pts.query.num_rows() > 0) {
	while (pts.query.fetch_row(row)) {
	  ofs << indent << "    <temporal start=\"" << summarizeMetadata::set_date_time_string(row[0],row[1],row[4]) << "\" end=\"" << summarizeMetadata::set_date_time_string(row[2],row[3],row[4]) << "\" groupID=\"Entire Dataset\" />" << std::endl;
	}
    }
    for (const auto& database : cmd_databases) {
	if (database == "GrML") {
	  type_list.emplace_back("grid");
	  if (gfts.query.num_rows() > 0) {
	    while (gfts.query.fetch_row(row)) {
		add_frequency(frequency_table,std::stoi(row[0]),row[1]);
	    }
	  }
	  if (gfmts.query.num_rows() > 0) {
	    while (gfmts.query.fetch_row(row)) {
		format_list.emplace_back(row[0]);
	    }
	  }
	}
	else if (database == "ObML") {
	  type_list.emplace_back("platform_observation");
	  if (ofts.query.num_rows() > 0) {
	    while (ofts.query.fetch_row(row)) {
		add_frequency(frequency_table,std::stoi(row[0]),row[2]);
		add_frequency(frequency_table,std::stoi(row[1]),row[2]);
	    }
	  }
	  if (ofmts.query.num_rows() > 0) {
	    while (ofmts.query.fetch_row(row)) {
		format_list.emplace_back(row[0]);
	    }
	  }
	}
    }
    for (const auto& key : frequency_table.keys()) {
	size_t idx=key.find("<!>");
	ofs << indent << "    <temporalFrequency type=\"regular\" number=\"" << key.substr(0,idx) << "\" unit=\"" << key.substr(idx+3) << "\" />" << std::endl;
    }
    for (const auto& type : type_list) {
	ofs << indent << "    <dataType>" << type << "</dataType>" << std::endl;
    }
    std::string format_ref_doc;
    if (stat((directives.server_root+"/web/metadata/FormatReferences.xml.new").c_str(),&buf) == 0) {
	format_ref_doc=directives.server_root+"/web/metadata/FormatReferences.xml.new";
    }
    else {
	format_ref_doc=remote_web_file("https://rda.ucar.edu/metadata/FormatReferences.xml.new",temp_dir.name());
    }
    XMLDocument fdoc(format_ref_doc);
    for (const auto& format : format_list) {
	e=fdoc.element("formatReferences/format@name="+format+"/href");
	auto href=e.content();
	ofs << indent << "    <format";
	if (!href.empty()) {
	  ofs << " href=\"" << href << "\"";
	}
	ofs << ">" << format << "</format>" << std::endl;
    }
    fdoc.close();
    if (dts.query.num_rows() > 0) {
	while (dts.query.fetch_row(row)) {
	  ofs << indent << "    <geospatialCoverage>" << std::endl;
	  std::deque<std::string> sp=strutils::split(row[1],":");
	  ofs << indent << "      <grid definition=\"" << row[0] << "\" numX=\"" << sp[0] << "\" numY=\"" << sp[1] << "\"";
	  auto grid_def=row[0];
	  if (grid_def == "gaussLatLon") {
	    ofs << " startLon=\"" << sp[3] << "\" startLat=\"" << sp[2] << "\" endLon=\"" << sp[5] << "\" endLat=\"" << sp[4] << "\" xRes=\"" << sp[6] << "\"";
	  }
	  else if (grid_def == "lambertConformal") {
	    ofs << " startLon=\"" << sp[3] << "\" startLat=\"" << sp[2] << "\" projLon=\"" << sp[5] << "\" resLat=\"" << sp[4] << "\" xRes=\"" << sp[7] << "\" yRes=\"" << sp[8] << "\" pole=\"" << sp[6] << "\" stdParallel1=\"" << sp[9] << "\" stdParallel2=\"" << sp[10] << "\"";
	  }
	  else if (grid_def == "latLon") {
	    ofs << " startLon=\"" << sp[3] << "\" startLat=\"" << sp[2] << "\" endLon=\"" << sp[5] << "\" endLat=\"" << sp[4] << "\" xRes=\"" << sp[6] << "\" yRes=\"" << sp[7] << "\"";
	  }
	  else if (grid_def == "mercator") {
	    ofs << " startLon=\"" << sp[3] << "\" startLat=\"" << sp[2] << "\" endLon=\"" << sp[5] << "\" endLat=\"" << sp[4] << "\" xRes=\"" << sp[6] << "\" yRes=\"" << sp[7] << "\" resLat=\"" << sp[8] << "\"";
	  }
	  else if (grid_def == "polarStereographic") {
	    ofs << " startLon=\"" << sp[3] << "\" startLat=\"" << sp[2] << "\" projLon=\"" << sp[5] << "\" xRes=\"" << sp[7] << "\" yRes=\"" << sp[8] << "\" pole=\"" << sp[6] << "\"";
	  }
	  ofs << " />" << std::endl;
	  ofs << indent << "    </geospatialCoverage>" << std::endl;
	}
    }
    ofs << indent << "  </contentMetadata>" << std::endl;
  }
  else {
    e=xdoc.element("dsOverview/contentMetadata");
    auto content_metadata=e.to_string();
    strutils::replace_all(content_metadata,"\n","\n"+indent+"  ");
    ofs << indent << "  " << content_metadata << std::endl;
  }
  ofs << indent << "</dsOverview>" << std::endl;
  server.disconnect();
}

void export_to_THREDDS(std::ostream& ofs,std::string ident,XMLDocument& xdoc,size_t indent_length)
{
  MySQL::Server server;
  MySQL::LocalQuery query;
  MySQL::Row row;
  std::string indent,title;
  size_t n;

  for (n=0; n < indent_length; n++)
    indent+=" ";
  metautils::connect_to_metadata_server(server);
  ofs << indent << "<catalog xmlns=\"http://www.unidata.ucar.edu/namespaces/thredds/InvCatalog/v1.0\"" << std::endl;
  ofs << indent << "         xmlns:xlink=\"http://www.w3.org/1999/xlink\"" << std::endl;
  ofs << indent << "         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << indent << "         xsi:schemaLocation=\"http://www.unidata.ucar.edu/namespaces/thredds/InvCatalog/v1.0" << std::endl;
  ofs << indent << "                             http://www.unidata.ucar.edu/schemas/thredds/InvCatalog.1.0.xsd\"" << std::endl;
  ofs << indent << "         name=";
  if (ident.length() == 5 && ident[3] == '.' && strutils::is_numeric(strutils::substitute(ident,".",""))) {
    query.set("select title,summary from search.datasets where dsid = '"+ident+"'");
    if (query.submit(server) < 0) {
	std::cout << "Content-type: text/plain" << std::endl << std::endl;
	std::cout << "Database error: " << query.error() << std::endl;
	exit(1);
    }
    query.fetch_row(row);
    title=strutils::substitute(row[0],"\"","&quot;");
    ofs << "\"" << title << " (ds" << ident << ")\">" << std::endl;
    ofs << indent << "  <service base=\"https://rda.ucar.edu/\" name=\"RDADB Server\" serviceType=\"HTTPServer\" desc=\"UCAR/CISL Research Data Archive\" />" << std::endl;
    ofs << indent << "  <dataset name=\"" << title << " (ds" << ident << ")\" ID=\"edu.ucar.dss.ds" << ident << "\" harvest=\"true\">" << std::endl;
    ofs << indent << "    <metadata>" << std::endl;
    ofs << indent << "      <publisher>" << std::endl;
    ofs << indent << "        <name vocabulary=\"DIF\">UCAR/NCAR/CISL/DSS > Data Support Section, Computational and Information Systems Laboratory, National Center for Atmospheric Research, University Corporation for Atmospheric Research</name>" << std::endl;
    ofs << indent << "        <contact url=\"https://rda.ucar.edu/\" email=\"dssweb@ucar.edu\" />" << std::endl;
    ofs << indent << "      </publisher>" << std::endl;
    ofs << indent << "      <documentation type=\"summary\">" << std::endl;
    ofs << indent << "        " << summary::convert_html_summary_to_ASCII("<summary>"+row[1]+"</summary>",72-indent_length,8+indent_length) << std::endl;
    ofs << indent << "      </documentation>" << std::endl;
    ofs << indent << "    </metadata>" << std::endl;
    ofs << indent << "  </dataset>" << std::endl;
  }
  else if (strutils::has_beginning(ident,"range")) {
    query.set("select * from dssdb.dsranges where min = '"+ident.substr(5)+"'");
    if (query.submit(server) < 0) {
	std::cout << "Content-type: text/plain" << std::endl << std::endl;
	std::cout << "Database error: " << query.error() << std::endl;
	exit(1);
    }
    query.fetch_row(row);
    ofs << "\"Datasets "+row[1]+".0-"+row[2]+".9: "+row[0]+"\">" << std::endl;
    ofs << indent << "  <dataset name=\"Datasets "+row[1]+".0-"+row[2]+".9: "+row[0]+"\" ID=\"ucar.scd.dss.range"+row[1]+"\">" << std::endl;
    ofs << indent << "    <metadata>" << std::endl;
    ofs << indent << "      <publisher>" << std::endl;
    ofs << indent << "        <name vocabulary=\"DIF\">UCAR/NCAR/CISL/DSS > Data Support Section, Computational and Information Systems Laboratory, National Center for Atmospheric Research, University Corporation for Atmospheric Research</name>" << std::endl;
    ofs << indent << "      </publisher>" << std::endl;
    ofs << indent << "    </metadata>" << std::endl;
    query.set("select dsid,title from search.datasets where (type = 'P' or type = 'H') and dsid != '999.9' and dsid >= '"+row[1]+".0' and dsid <= '"+row[2]+".9'");
    if (query.submit(server) < 0) {
	std::cout << "Content-type: text/plain" << std::endl << std::endl;
	std::cout << "Database error: " << query.error() << std::endl;
	exit(1);
    }
    while (query.fetch_row(row)) {
	ofs << indent << "    <catalogRef xlink:href=\"https://rda.ucar.edu/metadata/thredds/ds_catalogs/ds"+row[0]+".thredds\" xlink:title=\"ds"+row[0]+":  "+row[1]+"\" />" << std::endl;
    }
    ofs << indent << "  </dataset>" << std::endl;
  }
  else if (ident == "main") {
    ofs << "\"DSS Dataset Holdings\">" << std::endl;
    ofs << indent << "  <dataset name=\"DSS Dataset Holdings\" ID=\"ucar.scd.dss\">" << std::endl;
    ofs << indent << "    <metadata>" << std::endl;
    ofs << indent << "      <publisher>" << std::endl;
    ofs << indent << "        <name vocabulary=\"DIF\">UCAR/NCAR/SCD/DSS > Data Support Section, Scientific Computing Division, National Center for Atmospheric Research, University Corporation for Atmospheric Research</name>" << std::endl;
    ofs << indent << "        <contact url=\"https://rda.ucar.edu/\" email=\"dssweb@ucar.edu\" />" << std::endl;
    ofs << indent << "      </publisher>" << std::endl;
    ofs << indent << "    </metadata>" << std::endl;
    ofs << indent << "    <documentation type=\"summary\">" << std::endl;
    ofs << indent << "      The Data Support Section (DSS) manages a large archive of meteorological, oceanographic, and related research data that have been assembled from various and numerous sources from around the world." << std::endl;
    ofs << indent << "    </documentation>" << std::endl;
    ofs << indent << "    <documentation xlink:href=\"https://rda.ucar.edu/\" xlink:title=\"RDA Home Page\" xlink:show=\"new\" />" << std::endl;
    query.set("select * from dssdb.dsranges");
    if (query.submit(server) < 0) {
	std::cout << "Content-type: text/plain" << std::endl << std::endl;
	std::cout << "Database error: " << query.error() << std::endl;
	exit(1);
    }
    while (query.fetch_row(row)) {
	if (!strutils::has_beginning(row[0],"Reserved"))
	  ofs << indent << "    <catalogRef xlink:href=\"https://rda.ucar.edu/datasets/ranges/range"+row[1]+".thredds\" xlink:title=\"Datasets "+row[1]+".0-"+row[2]+".9: "+row[0]+"\" />" << std::endl;
    }
    ofs << indent << "  </dataset>" << std::endl;
  }
  ofs << indent << "</catalog>" << std::endl;
  server.disconnect();
}

void export_metadata(std::string format,std::unique_ptr<TokenDocument>& token_doc,std::ostream& ofs,std::string ident,size_t initial_indent_length)
{
  XMLDocument xdoc;
  TempDir temp_dir;
  struct stat buf;

  temp_dir.create("/tmp");
  if (ident.length() == 5 && ident[3] == '.' && strutils::is_numeric(strutils::substitute(ident,".",""))) {
    std::string ds_overview;
    if (stat((directives.server_root+"/web/datasets/ds"+ident+"/metadata/dsOverview.xml").c_str(),&buf) == 0) {
	ds_overview=directives.server_root+"/web/datasets/ds"+ident+"/metadata/dsOverview.xml";
    }
    else {
	ds_overview=remote_web_file("https://rda.ucar.edu/datasets/ds"+ident+"/metadata/dsOverview.xml",temp_dir.name());
    }
    xdoc.open(ds_overview);
  }
  if (format == "oai_dc") {
    export_to_OAI_DC(ofs,ident,xdoc,initial_indent_length);
  }
  else if (format == "datacite") {
    export_to_DataCite(ofs,ident,xdoc,initial_indent_length);
  }
  else if (format == "dif") {
    export_to_DIF(ofs,ident,xdoc,initial_indent_length);
  }
  else if (format == "fgdc") {
    export_to_FGDC(ofs,ident,xdoc,initial_indent_length);
  }
  else if (format == "iso19139") {
    export_to_ISO19139(ofs,ident,xdoc,initial_indent_length);
  }
  else if (format == "iso19115-3") {
    export_to_ISO19115_dash_3(token_doc,ofs,ident,xdoc,initial_indent_length);
  }
  else if (format == "native") {
    export_to_native(ofs,ident,xdoc,initial_indent_length);
  }
  else if (format == "thredds") {
    export_to_THREDDS(ofs,ident,xdoc,initial_indent_length);
  }
  xdoc.close();
}

} // end namespace metadataExport
