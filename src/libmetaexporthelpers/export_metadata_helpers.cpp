#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <PostgreSQL.hpp>
#include <xml.hpp>
#include <mymap.hpp>
#include <metadata_export_pg.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <gridutils.hpp>

using namespace PostgreSQL;
using std::cerr;
using std::cout;
using std::endl;
using std::stod;
using std::stof;
using std::stoi;
using std::stoll;
using std::string;
using std::to_string;
using std::unordered_set;
using std::vector;
using strutils::ftos;
using strutils::split;
using strutils::substitute;
using strutils::trim;

namespace metadataExport {

bool compare_references(XMLElement& left,XMLElement& right)
{
  auto e=left.element("year");
  auto l=e.content();
  e=right.element("year");
  auto r=e.content();
  if (l > r) {
    return true;
  }
  else {
    return false;
  }
}

string resolution_key(double res,string units)
{
  string s;
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

void add_to_resolution_table(double lon_res,double lat_res,string units,my::map<Entry>& resolution_table)
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

string primary_size(string dsnum,Server& server)
{
  const char *vunits[]={"bytes","Kbytes","Mbytes","Gbytes","Tbytes","Pbytes"};
  LocalQuery query("primary_size","dssdb.dataset","dsid = 'ds"+dsnum+"'");
  if (query.submit(server) < 0) {
    cout << "Content-type: text/plain" << endl << endl;
    cout << "Database error: " << query.error() << endl;
    exit(1);
  }
  string psize;
  auto n=0;
  Row row;
  if (query.fetch_row(row)) {
    if (!row[0].empty()) {
	double vsize=stoll(row[0]);
	while (vsize > 999.999999) {
	  vsize/=1000.;
	  ++n;
	}
	psize=ftos(vsize,7,3,' ');
	trim(psize);
    }
  }
  psize+=string(" ")+vunits[n];
  return psize;
}

void convert_box_data(const string& row,const string& col,double& comp_lat,double& comp_lon,string comp_type)
{
  double lat,lon;
  geoutils::convert_box_to_center_lat_lon(1,stoi(row),stoi(col),lat,lon);
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

void fill_geographic_extent_data(Server& server,string dsnum,XMLDocument& dataset_overview,double& min_west_lon,double& min_south_lat,double& max_east_lon,double& max_north_lat,bool& is_grid,vector<HorizontalResolutionEntry> *hres_list,my::map<Entry> *unique_places_table)
{
  min_west_lon=min_south_lat=9999.;
  max_east_lon=max_north_lat=-9999.;
  is_grid=false;
  LocalQuery query("select definition,defParams from (select distinct gridDefinition_code from GrML.summary where dsid = '"+dsnum+"') as s left join GrML.gridDefinitions as d on d.code = s.gridDefinition_code");
  query.submit(server);
  if (query.num_rows() > 0) {
    is_grid=true;
    unordered_set<string> unique_hres_set;
    Row row;
    while (query.fetch_row(row)) {
	double west_lon,south_lat,east_lon,north_lat;
	if (gridutils::fill_spatial_domain_from_grid_definition(row[0]+"<!>"+row[1],"primeMeridian",west_lon,south_lat,east_lon,north_lat)) {
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
	  auto parts=split(row[1],":");
	  HorizontalResolutionEntry hre;
	  if (row[0] == "polarStereographic" || row[0] == "lambertConformal") {
	    hre.resolution=stod(parts[7])*1000;
	    hre.uom="m";
	    hre.key=parts[7]+hre.uom;
	  }
	  else {
	    hre.resolution=stod(parts[6]);
	    hre.uom="degree";
	    hre.key=parts[6]+hre.uom;
	  }
	  if (hres_list != nullptr && unique_hres_set.find(hre.key) == unique_hres_set.end()) {
	    hres_list->emplace_back(hre);
	    unique_hres_set.emplace(hre.key);
	  }
	}
    }
  }
  else {
    auto elist=dataset_overview.element_list("dsOverview/contentMetadata/geospatialCoverage/grid");
    if (elist.size() > 0) {
	is_grid=true;
	for (const auto& element : elist) {
	  auto gdef=element.attribute_value("definition");
	  string def_params;
	  if (gdef == "latLon" || gdef == "gaussLatLon") {
	    def_params=element.attribute_value("numX")+":"+element.attribute_value("numY")+":"+element.attribute_value("startLat")+":"+element.attribute_value("startLon")+":"+element.attribute_value("endLat")+":"+element.attribute_value("endLon")+":"+element.attribute_value("xRes")+":";
	    if (gdef == "latLon") {
		def_params+=element.attribute_value("yRes");
	    }
	    else {
		def_params+=to_string(stoi(element.attribute_value("numY"))/2);
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
	  if (element.attribute_value("isCell") == "true") {
	    gdef+="Cell";
	  }
	  double west_lon,south_lat,east_lon,north_lat;
	  if (gridutils::fill_spatial_domain_from_grid_definition(gdef+"<!>"+def_params,"primeMeridian",west_lon,south_lat,east_lon,north_lat)) {
	    if (west_lon < min_west_lon) {
		min_west_lon=west_lon;
	    }
	    if (east_lon < 0. && east_lon < west_lon) {
		east_lon+=360.;
	    }
	    if (east_lon > max_east_lon) {
		max_east_lon=east_lon;
	    }
	    if (max_east_lon > 180.) {
		max_east_lon-=360.;
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
  if (hres_list != nullptr) {
    std::sort(hres_list->begin(),hres_list->end(),
    [](const HorizontalResolutionEntry& left,const HorizontalResolutionEntry& right) -> bool
    {
	if (left.resolution < right.resolution) {
	  return true;
	}
	else if (left.resolution > right.resolution) {
	  return false;
	}
	else {
	  if (left.uom <= right.uom) {
	    return true;
	  }
	  else {
	    return false;
	  }
	}
    });
  }
  if (table_exists(server,"WObML.ds"+substitute(dsnum,".","")+"_geobounds")) {
    query.set("select min(min_lat),min(min_lon),max(max_lat),max(max_lon) from WObML.ds"+substitute(dsnum,".","")+"_geobounds where min_lat >= -900000 and min_lon >= -1800000 and max_lat <= 900000 and max_lon <= 1800000");
    if (query.submit(server) == 0) {
	Row row;
	while (query.fetch_row(row)) {
	  if (row[0].empty() || row[1].empty() || row[2].empty() || row[3].empty()) {
	    cerr << "oai warning: " << dsnum << " geobounds: '" << row[0] << "','" << row[1] << "','" << row[2] << "','" << row[3] << "'" << endl;
	  }
	  else {
	    auto f=stof(row[0])/10000.;
	    if (f < min_south_lat) {
		min_south_lat=f;
	    }
	    f=stof(row[1])/10000.;
	    if (f < min_west_lon) {
		min_west_lon=f;
	    }
	    f=stof(row[2])/10000.;
	    if (f > max_north_lat) {
		max_north_lat=f;
	    }
	    f=stof(row[3])/10000.;
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
	Row row;
	while (query.fetch_row(row)) {
	  Entry entry;
	  if (unique_places_table != nullptr && !unique_places_table->found(row[0],entry)) {
	    entry.key=row[0];
	    unique_places_table->insert(entry);
	  }
	  if (row[1] != "0" || row[2] != "359") {
	    convert_box_data(row[1],row[2],min_south_lat,min_west_lon,"min");
	    convert_box_data(row[1],row[3],max_north_lat,max_east_lon,"max");
	  }
	}
    }
    else {
	auto elist=dataset_overview.element_list("dsOverview/contentMetadata/geospatialCoverage/location");
	string where_conditions="";
	for (const auto& element : elist) {
	  if (!where_conditions.empty()) {
	    where_conditions+=" or ";
	  }
	  where_conditions+="keyword = '"+element.content()+"'";
	}
	query.set("select keyword,min(box1d_row),min(box1d_bitmap_min),max(box1d_row),max(box1d_bitmap_max) from search.location_data where "+where_conditions+" and box1d_row >= 0 group by keyword");
	query.submit(server);
	if (query.num_rows() > 0) {
	  Row row;
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

void add_replace_from_element_content(XMLDocument& xdoc,string xpath,string replace_token,TokenDocument& token_doc,string if_key)
{
  auto e=xdoc.element(xpath);
  if (!e.name().empty()) {
    if (!if_key.empty()) {
	token_doc.add_if(if_key);
    }
    token_doc.add_replacement(replace_token,e.content());
  }
}

} // end namespace metadataExport
