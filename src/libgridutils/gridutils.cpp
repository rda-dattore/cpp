#include <iostream>
#include <string>
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
  if (definition == "latLon") {
    if (getAbbreviation) {
	return "LL";
    }
    else {
	return "Longitude/Latitude";
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
  std::string s,sdum,sdum2,sdum3;
  std::deque<std::string> sp,spx;
  float yres;

  sp=strutils::split(d,"<!>");
  spx=strutils::split(sp[1],":");
  if (sp[0] == "gaussLatLon") {
    sdum2=spx[2].substr(0,spx[2].length()-1);
    sdum3=spx[4].substr(0,spx[4].length()-1);
    if ((strutils::has_ending(spx[2],"N") && strutils::has_ending(spx[4],"S")) || (strutils::has_ending(spx[2],"S") && strutils::has_ending(spx[4],"N"))) {
	yres=(std::stof(sdum2)+std::stof(sdum3))/(std::stof(spx[1])-1);
    }
    else {
	yres=fabs(std::stof(sdum2)-std::stof(sdum3))/(std::stof(spx[1])-1);
    }
    sdum=strutils::ftos(yres,6,3,' ');
    strutils::trim(sdum);
    s=spx[6]+"&deg; x ~"+sdum+"&deg; from "+spx[3]+" to "+spx[5]+" and "+spx[2]+" to "+spx[4]+" <small>(";
    if (spx[0] == "-1") {
	s+="reduced n"+strutils::itos(std::stoi(spx[1])/2);
    }
    else {
	s+=spx[0]+" x "+spx[1];
    }
    s+=" "+translate_grid_definition(sp[0])+")</small>";
  }
  else if (sp[0] == "lambertConformal")
    s=spx[7]+"km x "+spx[8]+"km (at "+spx[4]+") oriented "+spx[5]+" <small>("+spx[0]+"x"+spx[1]+" "+translate_grid_definition(sp[0])+" starting at "+spx[2]+","+spx[3]+")</small>";
  else if (sp[0] == "latLon") {
    s=spx[6]+"&deg; x "+spx[7]+"&deg; from "+spx[3]+" to "+spx[5]+" and "+spx[2]+" to "+spx[4]+" <small>(";
    if (spx[0] == "-1") {
	s+="reduced";
    }
    else {
	s+=spx[0]+" x "+spx[1];
    }
    s+=" "+translate_grid_definition(sp[0])+")</small>";
  }
  else if (sp[0] == "mercator")
    s=spx[6]+"km x "+spx[7]+"km (at "+spx[8]+") from "+spx[3]+" to "+spx[5]+" and "+spx[2]+" to "+spx[4]+" <small>("+spx[0]+" x "+spx[1]+" "+translate_grid_definition(sp[0])+")</small>";
  else if (sp[0] == "polarStereographic") {
    s=spx[7]+"km x "+spx[8]+"km (at ";
    if (spx[4].length() > 0) {
	s+=spx[4];
    }
    else {
	s+="60"+spx[6];
    }
    if (spx[6] == "N") {
	sdum="North";
    }
    else {
	sdum="South";
    }
    s+=") oriented "+spx[5]+" <small>("+spx[0]+"x"+spx[1]+" "+sdum+" "+translate_grid_definition(sp[0])+")</small>";
  }
  else if (sp[0] == "sphericalHarmonics") {
    s=translate_grid_definition(sp[0])+" at ";
    if (spx[2] == spx[0] && spx[0] == spx[1]) {
	s+="T";
    }
    else if (std::stoi(spx[1]) == (std::stoi(spx[1])+std::stoi(spx[2]))) {
	s+="R";
    }
    s+=spx[1]+" spectral resolution";
  }
  return s;
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
  std::deque<std::string> sp,spx;
  std::string sdum;
  double ddum;
  double start_lat,start_elon,end_elon,xres,xspace;
  bool scansEast=false,isGlobalLon=false;
  double lat1,elon1,tanlat,elonv;
  bool filled=false;

  west_lon=south_lat=999.;
  east_lon=north_lat=-999.;
  sp=strutils::split(definition,"<!>");
  if (sp[0] == "latLon" || sp[0] == "gaussLatLon" || sp[0] == "mercator") {
    spx=strutils::split(sp[1],":");
    sdum=spx[2];
    strutils::chop(sdum);
    ddum=std::stof(sdum);
    if (strutils::has_ending(spx[2],"S")) {
	ddum=-ddum;
    }
    if (ddum < south_lat) {
	south_lat=ddum;
    }
    if (ddum > north_lat) {
	north_lat=ddum;
    }
    sdum=spx[3];
    strutils::chop(sdum);
    start_elon=std::stof(sdum);
    if (strutils::has_ending(spx[3],"W")) {
	start_elon=-start_elon+360.;
    }
    sdum=spx[4];
    strutils::chop(sdum);
    ddum=std::stof(sdum);
    if (strutils::has_ending(spx[4],"S")) {
	ddum=-ddum;
    }
    if (ddum < south_lat) {
	south_lat=ddum;
    }
    if (ddum > north_lat) {
	north_lat=ddum;
    }
    sdum=spx[5];
    strutils::chop(sdum);
    end_elon=std::stof(sdum);
    if (strutils::has_ending(spx[5],"W")) {
	end_elon=-end_elon+360.;
    }
    xspace=std::stof(spx[0])-1.;
    if (sp[0] == "latLon" || sp[0] == "gaussLatLon") {
	xres=std::stof(spx[6]);
    }
    else {
	xres=fabs(end_elon-start_elon)/xspace;
    }
    if (xspace < 0.) {
// reduced grids
	xspace=(end_elon-start_elon)/xres;
    }
    if (fabs((end_elon-start_elon)/xspace-xres) < 0.01) {
	scansEast=true;
    }
    else if (fabs((end_elon+360.-start_elon)/xspace-xres) < 0.01) {
	end_elon+=360.;
	scansEast=true;
    }
    else if (fabs((start_elon-end_elon)/xspace-xres) < 0.01) {
	scansEast=false;
    }
    else if (fabs((start_elon+360.-end_elon)/xspace-xres) < 0.01) {
	start_elon+=360.;
	scansEast=false;
    }
    else {
	return false;
    }
// adjust global grids where longitude boundary is not repeated
    if (!floatutils::myequalf(end_elon,start_elon)) {
	if ( fabs(fabs(end_elon-start_elon)-360.) < 0.001)
	  isGlobalLon=true;
	else {
	  if (end_elon > start_elon) {
	    if ( fabs(fabs(end_elon+xres-start_elon)-360.) < 0.001)
		isGlobalLon=true;
	  }
	  else {
	     if ( fabs(fabs(end_elon-xres-start_elon)-360.) < 0.001)
		isGlobalLon=true;
	  }
	}
    }
    if ( fabs(fabs(north_lat-south_lat+std::stof(spx[7]))-180.) < 0.001) {
	south_lat=-90.;
	north_lat=90.;
    }
    if (isGlobalLon) {
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
	if (scansEast) {
	  west_lon=start_elon;
	  east_lon=end_elon;
	}
	else {
	  west_lon=end_elon;
	  east_lon=start_elon;
	}
    }
    else if (center == "primeMeridian") {
	if (scansEast) {
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
  else if (sp[0] == "lambertConformal") {
    spx=strutils::split(sp[1],":");
    sdum=spx[2];
    strutils::chop(sdum);
    lat1=std::stof(sdum);
    if (strutils::has_ending(spx[2],"S")) {
	lat1=-lat1;
    }
    sdum=spx[3];
    strutils::chop(sdum);
    elon1=std::stof(sdum);
    if (strutils::has_ending(spx[3],"W")) {
	elon1=360.-elon1;
    }
    sdum=spx[4];
    strutils::chop(sdum);
    tanlat=std::stof(sdum);
    if (strutils::has_ending(spx[4],"S")) {
	tanlat=-tanlat;
    }
    sdum=spx[5];
    strutils::chop(sdum);
    elonv=std::stof(sdum);
    if (strutils::has_ending(spx[5],"W")) {
	elonv=360.-elonv;
    }
    fill_spatial_domain_from_lambert_conformal_grid(std::stoi(spx[0]),std::stoi(spx[1]),lat1,elon1,std::stof(spx[7]),elonv,tanlat,west_lon,south_lat,east_lon,north_lat);
    if (center == "dateLine") {
	west_lon+=360.;
	east_lon+=360.;
    }
    filled=true;
  }
  else if (sp[0] == "polarStereographic") {
    spx=strutils::split(sp[1],":");
    sdum=spx[5];
    strutils::chop(sdum);
    elonv=std::stof(sdum);
    if (strutils::has_ending(spx[5],"W")) {
	elonv=360.-elonv;
    }
    sdum=spx[4];
    strutils::chop(sdum);
    tanlat=std::stof(sdum);
/*
    if (strutils::has_ending(spx[4],"S"))
	tanlat=-tanlat;
*/
    sdum=spx[2];
    if (strutils::has_ending(sdum,"S")) {
	sdum="-"+sdum;
    }
    strutils::chop(sdum);
    start_lat=std::stof(sdum);
    sdum=spx[3];
    if (strutils::has_ending(sdum,"W")) {
	sdum="-"+sdum;
    }
    strutils::chop(sdum);
    start_elon=std::stof(sdum);
    if (start_elon < 0.) {
	start_elon+=360.;
    }
    fill_spatial_domain_from_polar_stereographic_grid2(std::stoi(spx[0]),std::stoi(spx[1]),start_lat,start_elon,std::stof(spx[7]),elonv,spx[6][0],tanlat,west_lon,south_lat,east_lon,north_lat);
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
