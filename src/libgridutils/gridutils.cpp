#include <iostream>
#include <string>
#include <regex>
#include <gridutils.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <xml.hpp>

namespace gridutils {

void cartesianll(Grid::GridDimensions dim,Grid::GridDefinition def,int pole_x,int pole_y,double **lat,double **lon)
{
  const double erad=31.204359052;
  const double erad2=erad*erad;
  const double pi=3.14159265;
  const double rad2deg=180./pi;
  int n,m;
  double x,y,r2,xaxis=def.olongitude+90.;

  if (xaxis > 360.) xaxis-=360.;
  for (n=0; n < dim.y; n++) {
    for (m=0; m < dim.x; m++) {
	x=m-pole_x;
	y=n-pole_y;
	if (!floatutils::myequalf(x,0.) || !floatutils::myequalf(y,0.))
	  lon[n][m]=rad2deg*atan2(y,x)+xaxis;
	r2=x*x+y*y;
	lat[n][m]=rad2deg*asin((erad2-r2)/(erad2+r2));
    }
  }
}

size_t find_grid_precision(double reference_value)
{
  int iv;
  size_t precision=1;

  reference_value=fabs(reference_value);
  iv=reference_value;
  reference_value-=iv;
  reference_value*=10.;
  while (precision < 10 && reference_value < 1.) {
    precision++;
    reference_value*=10.;
  }

  return precision;
}

bool fill_gaussian_latitudes(std::string path_to_gauslat_lists,my::map<Grid::GLatEntry>& gaus_lats,size_t num_circles,bool grid_scans_N_to_S)
{
  Grid::GLatEntry glat_entry;
  glat_entry.key=num_circles;
  if (!gaus_lats.found(glat_entry.key,glat_entry)) {
    std::ifstream ifs;
    ifs.open((path_to_gauslat_lists+"/gauslats."+strutils::itos(num_circles)).c_str());
    if (!ifs.is_open()) {
	return false;
    }
    glat_entry.lats=new float[num_circles*2];
    auto n=0;
    char line[256];
    ifs.getline(line,256);
    while (!ifs.eof()) {
//	chop(line);
	if (grid_scans_N_to_S) {
	  glat_entry.lats[n]=atof(line);
	}
	else {
	  glat_entry.lats[num_circles*2-n-1]=atof(line);
	}
	++n;
	ifs.getline(line,256);
    }
    ifs.close();
    gaus_lats.insert(glat_entry);
  }
  return true;
}

Grid::GridDefinition fix_grid_definition(const Grid::GridDefinition& grid_definition,const Grid::GridDimensions& grid_dimensions)
{
  Grid::GridDefinition fixed_definition=grid_definition;

  if (!floatutils::myequalf((grid_definition.elongitude-grid_definition.slongitude)/(grid_dimensions.x-1),grid_definition.loincrement,0.000001)) {
    if (floatutils::myequalf(grid_definition.elongitude+grid_definition.loincrement-grid_definition.slongitude,360.,0.002)) {
	fixed_definition.loincrement=360./grid_dimensions.x;
	fixed_definition.elongitude=grid_definition.slongitude+fixed_definition.loincrement*(grid_dimensions.x-1);
    }
  }
  return fixed_definition;
}

namespace gridSubset {

void decode_grid_subset_string(std::string rinfo,struct Args& args)
{
  std::deque<std::string> sp,sp2,sp3;
  size_t n,m,idx;
  Parameter param;

  args.subset_bounds.nlat=99.;
  args.subset_bounds.slat=-99.;
  args.subset_bounds.wlon=-999.;
  args.subset_bounds.elon=999.;
  sp=strutils::split(rinfo,";");
  for (n=0; n < sp.size(); n++) {
    sp2=strutils::split(sp[n],"=");
    if (sp2[0] == "dsnum") {
	args.dsnum=sp2[1];
    }
    else if (sp2[0] == "startdate") {
	args.startdate=sp2[1];
    }
    else if (sp2[0] == "enddate") {
	args.enddate=sp2[1];
    }
    else if (sp2[0] == "level") {
	sp3=strutils::split(sp2[1],",");
	for (m=0; m < sp3.size(); m++) {
	  args.level_codes.push_back(sp3[m]);
	}
    }
    else if (sp2[0] == "product") {
	sp3=strutils::split(sp2[1],",");
	for (m=0; m < sp3.size(); m++) {
	  args.product_codes.push_back(sp3[m]);
	}
    }
    else if (sp2[0] == "parameters") {
	sp3=strutils::split(sp2[1],",");
	for (m=0; m < sp3.size(); m++) {
	  if ( (idx=sp3[m].find("!")) != std::string::npos) {
	    param.key=sp3[m].substr(idx+1);
	    param.format_code.reset(new std::string);
	    *param.format_code=sp3[m].substr(0,idx);
	  }
	  else {
	    param.key=sp3[m];
	    param.format_code=nullptr;
	  }
	  args.parameters.insert(param);
	}
    }
    else if (sp2[0] == "grid_definition") {
	args.grid_definition_code=sp2[1];
    }
    else if (sp2[0] == "ens") {
	args.ensemble_id=sp2[1];
    }
    else if (sp2[0] == "tindex") {
	args.tindex=sp2[1];
    }
    else if (sp2[0] == "nlat" && sp2[1].length() > 0) {
	args.subset_bounds.nlat=std::stof(sp2[1]);
    }
    else if (sp2[0] == "slat" && sp2[1].length() > 0) {
	args.subset_bounds.slat=std::stof(sp2[1]);
    }
    else if (sp2[0] == "wlon" && sp2[1].length() > 0) {
	args.subset_bounds.wlon=std::stof(sp2[1]);
    }
    else if (sp2[0] == "elon" && sp2[1].length() > 0) {
	args.subset_bounds.elon=std::stof(sp2[1]);
    }
    else if (sp2[0] == "inittime") {
	args.inittime=sp2[1];
    }
  }
}

}; // end namespace gridSubset

std::string translate_grid_definition(std::string definition,bool getAbbreviation = false)
{
  if (definition == "gaussLatLon") {
    if (getAbbreviation) {
	return "GLL";
    }
    else {
	return "Longitude/Gaussian Latitude";
    }
  }
  else if (definition == "lambertConformal") {
    if (getAbbreviation) {
	return "LC";
    }
    else {
	return "Lambert Conformal";
    }
  }
  else if (std::regex_search(definition,std::regex("^latLon"))) {
    if (getAbbreviation) {
	return "LL";
    }
    else {
	std::string s="Longitude/Latitude";
	if (std::regex_search(definition,std::regex("Cell$"))) {
	  s+=" cells";
	}
	return s;
    }
  }
  else if (definition == "mercator") {
    if (getAbbreviation) {
	return "MERC";
    }
    else {
	return "Mercator";
    }
  }
  else if (definition == "polarStereographic") {
    if (getAbbreviation) {
	return "PS";
    }
    else {
	return "Polar Stereographic";
    }
  }
  else if (definition == "sphericalHarmonics") {
    if (getAbbreviation) {
	return "SPHARM";
    }
    else {
	return "Spherical Harmonics";
    }
  }
  else {
    return "";
  }
}

std::string convert_grid_definition(std::string d)
{
  std::string converted_grid_definition;
  auto def_parts=strutils::split(d,"<!>");
  auto grid_info=strutils::split(def_parts.back(),":");
  if (def_parts.front() == "gaussLatLon") {
    auto start_lat_s=grid_info[2].substr(0,grid_info[2].length()-1);
    auto end_lat_s=grid_info[4].substr(0,grid_info[4].length()-1);
    float yres;
    if ((grid_info[2].back() == 'N' && grid_info[4].back() == 'S') || (grid_info[2].back() == 'S' && grid_info[4].back() == 'N')) {
	yres=(std::stof(start_lat_s)+std::stof(end_lat_s))/(std::stof(grid_info[1])-1);
    }
    else {
	yres=fabs(std::stof(start_lat_s)-std::stof(end_lat_s))/(std::stof(grid_info[1])-1);
    }
    auto yres_s=strutils::ftos(yres,6,3,' ');
    strutils::trim(yres_s);
    converted_grid_definition=grid_info[6]+"&deg; x ~"+yres_s+"&deg; from "+grid_info[3]+" to "+grid_info[5]+" and "+grid_info[2]+" to "+grid_info[4]+" <small>(";
    if (grid_info[0] == "-1") {
	converted_grid_definition+="reduced n"+strutils::itos(std::stoi(grid_info[1])/2);
    }
    else {
	converted_grid_definition+=grid_info[0]+" x "+grid_info[1];
    }
    converted_grid_definition+=" "+translate_grid_definition(def_parts.front())+")</small>";
  }
  else if (def_parts.front() == "lambertConformal") {
    converted_grid_definition=grid_info[7]+"km x "+grid_info[8]+"km (at "+grid_info[4]+") oriented "+grid_info[5]+" <small>("+grid_info[0]+"x"+grid_info[1]+" "+translate_grid_definition(def_parts.front())+" starting at "+grid_info[2]+","+grid_info[3]+")</small>";
  }
  else if (std::regex_search(def_parts.front(),std::regex("^(latLon|mercator)"))) {
    converted_grid_definition=grid_info[6]+"&deg; x ";
    if (def_parts.front() == "mercator") {
	converted_grid_definition.push_back('~');
    }
    converted_grid_definition+=grid_info[7]+"&deg; from "+grid_info[3]+" to "+grid_info[5]+" and "+grid_info[2]+" to "+grid_info[4]+" <small>(";
    if (grid_info[0] == "-1") {
	converted_grid_definition+="reduced";
    }
    else {
	converted_grid_definition+=grid_info[0]+" x "+grid_info[1];
    }
    converted_grid_definition+=" "+translate_grid_definition(def_parts.front())+")</small>";
  }
  else if (def_parts.front() == "polarStereographic") {
    converted_grid_definition=grid_info[7]+"km x "+grid_info[8]+"km (at ";
    if (grid_info[4].length() > 0) {
	converted_grid_definition+=grid_info[4];
    }
    else {
	converted_grid_definition+="60"+grid_info[6];
    }
    std::string orient;
    if (grid_info[6] == "N") {
	orient="North";
    }
    else {
	orient="South";
    }
    converted_grid_definition+=") oriented "+grid_info[5]+" <small>("+grid_info[0]+"x"+grid_info[1]+" "+orient+" "+translate_grid_definition(def_parts.front())+")</small>";
  }
  else if (def_parts.front() == "sphericalHarmonics") {
    converted_grid_definition=translate_grid_definition(def_parts.front())+" at ";
    if (grid_info[2] == grid_info[0] && grid_info[0] == grid_info[1]) {
	converted_grid_definition.push_back('T');
    }
    else if (std::stoi(grid_info[1]) == (std::stoi(grid_info[1])+std::stoi(grid_info[2]))) {
	converted_grid_definition.push_back('R');
    }
    converted_grid_definition+=grid_info[1]+" spectral resolution";
  }
  return converted_grid_definition;
}

std::string convert_meta_grid_definition(std::string grid_definition,std::string definition_parameters,const char *separator)
{
  std::string s;
  auto def_params=strutils::split(definition_parameters,separator);
  if (def_params.size() > 0) {
    if (grid_definition == "latLon") {
	s="numX="+def_params[0]+" numY="+def_params[1]+" start=("+def_params[2]+","+def_params[3]+") end=("+def_params[4]+","+def_params[5]+") latResolution="+def_params[6]+"deg lonResolution="+def_params[7]+"deg";
    }

  }
  return s;
}

std::string convert_grid_definition(std::string d,XMLElement e)
{
  std::string s,start_lat,end_lat,num_y;

  s=d+"<!>";
  if (d == "gaussLatLon") {
    auto circles=e.attribute_value("circles");
    start_lat=e.attribute_value("startLat");
    end_lat=e.attribute_value("endLat");
    num_y=e.attribute_value("numY");
    if (circles.empty()) {
	if ((start_lat.back() == 'N' && end_lat.back() == 'S') || (start_lat.back() == 'S' && end_lat.back() == 'N')) {
	  circles=strutils::itos(std::stoi(num_y)/2);
	}
	else {
	  circles=num_y;
	}
    }
    s+=e.attribute_value("numX")+":"+num_y+":"+start_lat+":"+e.attribute_value("startLon")+":"+end_lat+":"+e.attribute_value("endLon")+":"+e.attribute_value("xRes")+":"+circles;
  }
  else if (d == "lambertConformal") {
    s+=e.attribute_value("numX")+":"+e.attribute_value("numY")+":"+e.attribute_value("startLat")+":"+e.attribute_value("startLon")+":"+e.attribute_value("resLat")+":"+e.attribute_value("projLon")+":"+e.attribute_value("pole")+":"+e.attribute_value("xRes")+":"+e.attribute_value("yRes")+":"+e.attribute_value("stdParallel1")+":"+e.attribute_value("stdParallel2");
  }
  else if (d == "latLon") {
    s+=e.attribute_value("numX")+":"+e.attribute_value("numY")+":"+e.attribute_value("startLat")+":"+e.attribute_value("startLon")+":"+e.attribute_value("endLat")+":"+e.attribute_value("endLon")+":"+e.attribute_value("xRes")+":"+e.attribute_value("yRes");
  }
  else if (d == "mercator") {
    s+=e.attribute_value("numX")+":"+e.attribute_value("numY")+":"+e.attribute_value("startLat")+":"+e.attribute_value("startLon")+":"+e.attribute_value("endLat")+":"+e.attribute_value("endLon")+":"+e.attribute_value("xRes")+":"+e.attribute_value("yRes")+":"+e.attribute_value("resLat");
  }
  else if (d == "polarStereographic") {
    s+=e.attribute_value("numX")+":"+e.attribute_value("numY")+":"+e.attribute_value("startLat")+":"+e.attribute_value("startLon")+":"+e.attribute_value("resLat")+":"+e.attribute_value("projLon")+":"+e.attribute_value("pole")+":"+e.attribute_value("xRes")+":"+e.attribute_value("yRes");
  }
  else if (d == "sphericalHarmonics") {
    s+=e.attribute_value("t1")+":"+e.attribute_value("t2")+":"+e.attribute_value("t3");
  }
  return convert_grid_definition(s);
}

bool fill_spatial_domain_from_grid_definition(std::string definition,std::string center,double& west_lon,double& south_lat,double& east_lon,double& north_lat)
{
  bool filled=false;
  west_lon=south_lat=999.;
  east_lon=north_lat=-999.;
  auto def_parts=strutils::split(definition,"<!>");
  if (std::regex_search(def_parts.front(),std::regex("^(latLon|gaussLatLon|mercator)(Cell){0,1}$"))) {
    auto grid_info=strutils::split(def_parts[1],":");
    auto start_lat_s=grid_info[2];
    strutils::chop(start_lat_s);
    auto start_lat=std::stod(start_lat_s);
    if (grid_info[2].back() == 'S') {
	start_lat=-start_lat;
    }
    if (start_lat < south_lat) {
	south_lat=start_lat;
    }
    if (start_lat > north_lat) {
	north_lat=start_lat;
    }
    auto start_lon_s=grid_info[3];
    strutils::chop(start_lon_s);
    auto start_elon=std::stod(start_lon_s);
    if (grid_info[3].back() == 'W') {
	start_elon=-start_elon+360.;
    }
    auto end_lat_s=grid_info[4];
    strutils::chop(end_lat_s);
    auto end_lat=std::stod(end_lat_s);
    if (grid_info[4].back() == 'S') {
	end_lat=-end_lat;
    }
    if (end_lat < south_lat) {
	south_lat=end_lat;
    }
    if (end_lat > north_lat) {
	north_lat=end_lat;
    }
    auto end_lon_s=grid_info[5];
    strutils::chop(end_lon_s);
    auto end_elon=std::stod(end_lon_s);
    if (grid_info[5].back() == 'W') {
	end_elon=-end_elon+360.;
    }
    auto xspace=std::stod(grid_info[0])-1.;
    double xres;
    if (std::regex_search(def_parts.front(),std::regex("^(latLon|gaussLatLon)"))) {
	xres=std::stod(grid_info[6]);
    }
    else {
	xres=fabs(end_elon-start_elon)/xspace;
    }
    if (xspace < 0.) {
// reduced grids
	xspace=(end_elon-start_elon)/xres;
    }
    auto scans_east=false;
    if (std::regex_search(def_parts.front(),std::regex("Cell$"))) {
	auto xres2=xres/2.;
	start_elon+=xres2;
	end_elon-=xres2;
    }
    if (fabs((end_elon-start_elon)/xspace-xres) < 0.01) {
	scans_east=true;
    }
    else if (fabs((end_elon+360.-start_elon)/xspace-xres) < 0.01) {
	end_elon+=360.;
	scans_east=true;
    }
    else if (fabs((start_elon-end_elon)/xspace-xres) < 0.01) {
	scans_east=false;
    }
    else if (fabs((start_elon+360.-end_elon)/xspace-xres) < 0.01) {
	start_elon+=360.;
	scans_east=false;
    }
    else {
	return false;
    }
// adjust global grids where longitude boundary is not repeated
    auto is_global_lon=false;
    if (!floatutils::myequalf(end_elon,start_elon)) {
	if ( fabs(fabs(end_elon-start_elon)-360.) < 0.001) {
	  is_global_lon=true;
	}
	else {
	  if (end_elon > start_elon) {
	    if ( fabs(fabs(end_elon+xres-start_elon)-360.) < 0.001) {
		is_global_lon=true;
	    }
	  }
	  else {
	     if ( fabs(fabs(end_elon-xres-start_elon)-360.) < 0.001) {
		is_global_lon=true;
	    }
	  }
	}
    }
    if ( fabs(fabs(north_lat-south_lat+std::stod(grid_info[7]))-180.) < 0.001) {
	south_lat=-90.;
	north_lat=90.;
    }
    if (is_global_lon) {
	if (center == "dateLine") {
	  west_lon=0.;
	  east_lon=360.;
	}
	else if (center == "primeMeridian") {
	  west_lon=-180.;
	  east_lon=180.;
	}
    }
    else if (center == "dateLine") {
	if (scans_east) {
	  west_lon=start_elon;
	  east_lon=end_elon;
	}
	else {
	  west_lon=end_elon;
	  east_lon=start_elon;
	}
    }
    else if (center == "primeMeridian") {
	if (scans_east) {
	  west_lon= (start_elon >= 180.) ? start_elon-360. : start_elon;
	  east_lon= (end_elon > 180.) ? end_elon-360. : end_elon;
	}
	else {
	  west_lon= (end_elon >= 180.) ? end_elon-360. : end_elon;
	  east_lon= (start_elon > 180.) ? start_elon-360. : start_elon;
	}
    }
    filled=true;
  }
  else if (def_parts.front() == "lambertConformal") {
    auto grid_info=strutils::split(def_parts[1],":");
    auto lat1_s=grid_info[2];
    strutils::chop(lat1_s);
    auto lat1=std::stod(lat1_s);
    if (grid_info[2].back() == 'S') {
	lat1=-lat1;
    }
    auto elon1_s=grid_info[3];
    strutils::chop(elon1_s);
    auto elon1=std::stod(elon1_s);
    if (grid_info[3].back() == 'W') {
	elon1=360.-elon1;
    }
    auto tanlat_s=grid_info[4];
    strutils::chop(tanlat_s);
    auto tanlat=std::stod(tanlat_s);
    if (grid_info[4].back() == 'S') {
	tanlat=-tanlat;
    }
    auto elonv_s=grid_info[5];
    strutils::chop(elonv_s);
    auto elonv=std::stod(elonv_s);
    if (grid_info[5].back() == 'W') {
	elonv=360.-elonv;
    }
    fill_spatial_domain_from_lambert_conformal_grid(std::stoi(grid_info[0]),std::stoi(grid_info[1]),lat1,elon1,std::stod(grid_info[7]),elonv,tanlat,west_lon,south_lat,east_lon,north_lat);
    if (center == "dateLine") {
	west_lon+=360.;
	east_lon+=360.;
    }
    filled=true;
  }
  else if (def_parts.front() == "polarStereographic") {
    auto grid_info=strutils::split(def_parts[1],":");
    auto elonv_s=grid_info[5];
    strutils::chop(elonv_s);
    auto elonv=std::stod(elonv_s);
    if (grid_info[5].back() == 'W') {
	elonv=360.-elonv;
    }
    auto tanlat_s=grid_info[4];
    strutils::chop(tanlat_s);
    auto tanlat=std::stod(tanlat_s);
/*
    if (strutils::has_ending(grid_info[4],"S"))
	tanlat=-tanlat;
*/
    auto start_lat_s=grid_info[2];
    if (start_lat_s.back() == 'S') {
	start_lat_s="-"+start_lat_s;
    }
    strutils::chop(start_lat_s);
    auto start_lat=std::stod(start_lat_s);
    auto start_lon_s=grid_info[3];
    if (start_lon_s.back() == 'W') {
	start_lon_s="-"+start_lon_s;
    }
    strutils::chop(start_lon_s);
    auto start_elon=std::stod(start_lon_s);
    if (start_elon < 0.) {
	start_elon+=360.;
    }
    fill_spatial_domain_from_polar_stereographic_grid2(std::stoi(grid_info[0]),std::stoi(grid_info[1]),start_lat,start_elon,std::stod(grid_info[7]),elonv,grid_info[6][0],tanlat,west_lon,south_lat,east_lon,north_lat);
    if (north_lat > -99. && south_lat < 99.) {
	if (center == "dateLine") {
	  west_lon=0.;
	  east_lon=360.;
	}
	filled=true;
    }
    else {
	filled=false;
    }
  }
  return filled;
}

} // end namespace gridutils
