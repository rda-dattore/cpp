#include <complex>
#include <vector>
#include <utils.hpp>

void fill_lat_lons_from_lambert_conformal_gridpoints(size_t num_x,size_t num_y,double lat1,double elon1,double dx,double elonv,double tanlat,std::vector<float>& lats,std::vector<float>& elons)
{
  int hemi= (tanlat > 0.) ? 1 : -1;
  const double PI=3.14159;
  const double RAD=PI/180.;
  tanlat*=RAD;
  double an=hemi*sin(tanlat);
  lat1*=RAD;
  elon1*=RAD;
// radius to lower left corner of grid
  const double R_EARTH=6.3712e+6;
  dx*=1000.;
  double rmll=R_EARTH/dx*pow(cos(tanlat),1.-an)*pow(1.+an,an)*pow(cos(lat1)/(1.+hemi*sin(lat1)),an)/an;
// find pole point
  double arg=an*(elon1-elonv*RAD);
  double polei=1.-hemi*rmll*sin(arg);
  double polej=1.+rmll*cos(arg);
  lats.clear();
  elons.clear();
  for (size_t n=0; n < num_y; ++n) {
    double y=n+1-polej;
    for (size_t m=0; m < num_x; ++m) {
	double x=m+1-polei;
// radius to the i,j point in grid units
	double r2=pow(x,2.)+pow(y,2.);
// check that i and j are not out of bounds
	double theta=PI*(1.-an);
	double beta=fabs(atan2(x,y));
	if (beta < theta) {
	  lats.emplace_back(-99.);
	  elons.emplace_back(-999.);
	}
	else {
	  if (myequalf(r2,0.)) {
	    lats.emplace_back(hemi*90.);
	    elons.emplace_back(elonv);
	  }
	  else {
	    double elon=elonv+(1/RAD*atan2(hemi*x,-y)/an);
	    while (elon > 360.) {
		elon-=360.;
	    }
	    elons.emplace_back(elon);
	    double aninv=1./an;
	    double z=pow(an/(R_EARTH/dx),aninv)/(pow(cos(tanlat),(1.-an)*aninv)*(1.+an));
	    lats.emplace_back(hemi*(PI/2.-2.*atan(z*pow(r2,aninv/2.)))*1/RAD);
	  }
	}
    }
  }
}

void fill_lat_lon_from_lambert_conformal_gridpoint(size_t i,size_t j,double lat1,double elon1,double dx,double elonv,double tanlat,double& lat,double& elon)
{
  int hemi= (tanlat > 0.) ? 1 : -1;
  const double PI=3.14159;
  const double RAD=PI/180.;
  tanlat*=RAD;
  double an=hemi*sin(tanlat);
  lat1*=RAD;
  elon1*=RAD;
// radius in meters to lower left corner of grid
  dx*=1000.;
  const double R_EARTH=6.3712e+6;
  double rmll=R_EARTH/dx*pow(cos(tanlat),1.-an)*pow(1.+an,an)*pow(cos(lat1)/(1.+hemi*sin(lat1)),an)/an;
// find pole point
  double arg=an*(elon1-elonv*RAD);
  double polei=1.-hemi*rmll*sin(arg);
  double polej=1.+rmll*cos(arg);
// radius to the i,j point in grid units
  double x=i+1-polei;
  double y=j+1-polej;
  double r2=pow(x,2.)+pow(y,2.);
// check that the requested i and j are not out of bounds
  double theta=PI*(1.-an);
  double beta=fabs(atan2(x,y));
  if (beta < theta) {
    lat=-99.;
    elon=-999.;
    return;
  }
  if (myequalf(r2,0.)) {
    lat=hemi*90.;
    elon=elonv;
  }
  else {
    elon=elonv+(1/RAD*atan2(hemi*x,-y)/an);
    while (elon > 360.) {
	elon-=360.;
    }
    double aninv=1./an;
    double z=pow(an/(R_EARTH/dx),aninv)/(pow(cos(tanlat),(1.-an)*aninv)*(1.+an));
    lat=hemi*(PI/2.-2.*atan(z*pow(r2,aninv/2.)))*1/RAD;
  }
}

void fill_spatial_domain_from_lambert_conformal_grid(size_t num_i,size_t num_j,double lat1,double elon1,double dx,double elonv,double tanlat,double& west_lon,double& south_lat,double& east_lon,double& north_lat)
{
  north_lat=-9999.;
  south_lat=9999.;
  west_lon=9999.;
  east_lon=-9999.;
  for (size_t n=0; n < num_j; ++n) {
    auto straddlesPrimeMeridian=false;
    double last_elon=-1.;
    for (size_t m=0; m < num_i; ++m) {
	double lat,elon;
	fill_lat_lon_from_lambert_conformal_gridpoint(m,n,lat1,elon1,dx,elonv,tanlat,lat,elon);
	if (last_elon < 0.) {
	  last_elon=elon;
	}
	if (elon < last_elon) {
	  straddlesPrimeMeridian=true;
	}
	if (straddlesPrimeMeridian) {
	  elon+=360;
	}
	if (lat < south_lat) {
	  south_lat=lat;
	}
	if (lat > north_lat) {
	  north_lat=lat;
	}
	if (elon < west_lon) {
	  west_lon=elon;
	}
	if (elon > east_lon) {
	  east_lon=elon;
	}
	last_elon=elon;
    }
  }
  west_lon-=360.;
  east_lon-=360.;
}

void fill_lat_lon_from_polar_stereographic_gridpoint(size_t i,size_t j,size_t num_i,size_t num_j,double dx,double elonv,char projection,double dx_lat,double& lat,double& elon)
{
  double x=i+1.-(num_i+1.)/2.;
  if (projection == 'S') {
    x=-x;
  }
  double y=j+1.-(num_j+1.)/2.;
  double lon;
  if (myequalf(x,0.) && myequalf(y,0.)) {
    lat=90.;
    lon=0.;
  }
  else {
    const double PI=3.14159;
    const double RAD=PI/180.;
    const double RADINV=180./PI;
    lon=RADINV*atan2(y,x)+90.-(360.-elonv);
    double r2=x*x+y*y;
    const double R_EARTH=6.3712e+6;
    double re2=pow((1.+sin(dx_lat*RAD))*R_EARTH/(dx*1000.),2.);
    lat=RADINV*asin((re2-r2)/(re2+r2));
  }
  if (projection == 'S') {
    lat=-lat;
  }
  elon=lon+360.;
  while (elon < 0.) {
    elon+=360.;
  }
}

void fill_spatial_domain_from_polar_stereographic_grid(size_t num_i,size_t num_j,double dx,double elonv,char projection,double dx_lat,double& west_lon,double& south_lat,double& east_lon,double& north_lat)
{
  size_t n,m;
  double lat,elon;

  west_lon=-180.;
  east_lon=180.;
  if (projection == 'N') {
    north_lat=90.;
    south_lat=9999.;
  }
  else {
    south_lat=-90.;
    north_lat=-9999.;
  }
  for (n=0; n < num_j; n++) {
    for (m=0; m < num_i; m++) {
	fill_lat_lon_from_polar_stereographic_gridpoint(m,n,num_i,num_j,dx,elonv,projection,dx_lat,lat,elon);
	if (projection == 'N') {
	  if (lat < south_lat)
	    south_lat=lat;
	}
	else {
	  if (lat > north_lat)
	    north_lat=lat;
	}
    }
  }
}

void fill_spatial_domain_from_polar_stereographic_grid2(size_t num_i,size_t num_j,double start_lat,double start_elon,double dx,double elonv,char projection,double dx_lat,double& west_lon,double& south_lat,double& east_lon,double& north_lat)
{
  const double PI=3.14159;
  const double RAD=PI/180.;
  size_t n,m;
  double lat,elon,yoverx;
  int pole_x,pole_y;
  time_t start_time,curr_time;

  fill_lat_lon_from_polar_stereographic_gridpoint(0,0,num_i,num_j,dx,elonv,projection,dx_lat,lat,elon);
  if (fabs(lat-start_lat) > 0.5 || fabs(elon-start_elon) > 0.5) {
    yoverx=tan(RAD*(start_elon-90.+(360.-elonv)));
    if (projection == 'S') {
	yoverx=-yoverx;
    }
    pole_x=1;
    start_time=curr_time=time(NULL);
    while (1) {
	pole_y=lround(1-yoverx*(1-pole_x));
	fill_lat_lon_from_polar_stereographic_gridpoint(0,0,pole_x*2-1,pole_y*2-1,dx,elonv,projection,dx_lat,lat,elon);
	if (fabs(lat-start_lat) < 0.5 && fabs(elon-start_elon) < 0.5) {
	  break;
	}
	++pole_x;
	curr_time=time(NULL);
	if ( (curr_time-start_time) > 30) {
	  break;
	}
    }
    north_lat=-99.;
    south_lat=99.;
    west_lon=999.;
    east_lon=-999.;
    if ( (curr_time-start_time) > 30) {
	return;
    }
    for (n=0; n < num_j; ++n) {
	for (m=0; m < num_i; ++m) {
	  fill_lat_lon_from_polar_stereographic_gridpoint(m,n,pole_x*2-1,pole_y*2-1,dx,elonv,projection,dx_lat,lat,elon);
	  if (lat > north_lat) {
	    north_lat=lat;
	  }
	  if (lat < south_lat) {
	    south_lat=lat;
	  }
	  if (elon > east_lon) {
	    east_lon=elon;
	  }
	  if (elon < west_lon) {
	    west_lon=elon;
	  }
	  if (fabs(elon-360.) < 0.1 && west_lon > 0.) {
	    west_lon=0.;
	  }
	}
    }
    if (myequalf(west_lon,0.) && east_lon > 359.9) {
	west_lon=-180.;
	east_lon=180.;
    }
    else {
	if (west_lon > 180.) {
	  west_lon-=360;
	}
	if (east_lon > 180.) {
	  east_lon-=360;
	}
    }
  }
  else {
    west_lon=-180.;
    east_lon=180.;
    if (projection == 'N') {
	north_lat=90.;
	south_lat=9999.;
    }
    else {
	south_lat=-90.;
	north_lat=-9999.;
    }
    for (n=0; n < num_j; ++n) {
	for (m=0; m < num_i; ++m) {
	  fill_lat_lon_from_polar_stereographic_gridpoint(m,n,num_i,num_j,dx,elonv,projection,dx_lat,lat,elon);
	  if (projection == 'N') {
	    if (lat < south_lat) {
		south_lat=lat;
	    }
	  }
	  else {
	    if (lat > north_lat) {
		north_lat=lat;
	    }
	  }
	}
    }
  }
}
