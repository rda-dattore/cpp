#include <stdexcept>
#include <complex>
#include <strutils.hpp>

namespace geoutils {

void convert_lat_lon_to_box(size_t box_size,double lat,double lon,size_t& lat_index,size_t& lon_index)
{
  if (lat < -90. || lat > 90.) {
    throw std::range_error("latitude '"+strutils::ftos(lat,4)+"' not in [-90,90]");
  }
  if (lon < -180. || lon > 180.) {
    throw std::range_error("longitude '"+strutils::ftos(lon,4)+"' not in [-180,180]");
  }
  if (fabs(lat+90.) < 0.00001) {
    lat_index=0;
    lon_index=0;
  }
  else if (fabs(lat-90.) < 0.00001) {
//    lat_index=181/box_size;
lat_index=(180+box_size)/box_size;
    lon_index=0;
  }
  else {
    if (lat <= 0.) {
//	lat_index=((int)lat+90)/box_size;
lat_index=(static_cast<int>(lat)+89+box_size)/box_size;
    }
    else {
//	lat_index=((int)(lat+0.99999999999999)+90)/box_size;
lat_index=(static_cast<int>(lat+0.99999999999999)+89+box_size)/box_size;
    }
    if (fabs(lon+180.) < 0.00001) {
	lon=180.;
    }
    if (lon <= 0.) {
	lon_index=(static_cast<int>(lon)+179)/box_size;
    }
    else {
	lon_index=(static_cast<int>(lon+0.9999999999999)+179)/box_size;
    }
  }
}

int convert_box_to_center_lat_lon(size_t box_size,size_t lat_index,size_t lon_index,double& lat,double& lon)
{
  if (lat_index > (180/box_size)+1 || lon_index > (360/box_size)-1) {
    return -1;
  }
  if (lat_index == 0) {
    lat=-90.;
  }
  else if (lat_index == (180/box_size)+1) {
    lat=90.;
  }
  else {
    lat=lat_index*box_size-90.-box_size/2.;
  }
  lon=lon_index*box_size-180.+box_size/2.;
  return 0;
}

void lon_index_bounds_from_bitmap(const char *bitmap,size_t& min_lon_index,size_t& max_lon_index)
{
  struct bitmap_gap {
    int length,start,end;
  } biggest,current;
  current.start=-1;
  current.end=biggest.start=biggest.end=0;
  current.length=biggest.length=0;
  size_t n,m;
  for (n=0,m=0; n < 720; ++n,++m) {
    if (m == 360) m=0;
    if (bitmap[m] == '0') {
	if (current.start < 0) {
	  current.start=n;
	}
	current.end=n;
	++current.length;
    }
    else {
	if (current.length > biggest.length) {
	  biggest=current;
	}
	current.start=-1;
	current.length=0;
    }
  }
  if (biggest.length == 0) {
    min_lon_index=0;
    max_lon_index=359;
  }
  else {
    --biggest.start;
    if (biggest.start < 0) {
	biggest.start+=360;
    }
    else if (biggest.start > 359) {
	biggest.start-=360;
    }
    ++biggest.end;
    if (biggest.end < 0) {
	biggest.end+=360;
    }
    else if (biggest.end > 359) {
	biggest.end-=360;
    }
    min_lon_index=biggest.end;
    max_lon_index=biggest.start;
  }
}

double arc_distance(double lat1,double lon1,double lat2,double lon2)
{
  const double rad=3.141592654/180.;
  const double km=111.125;

  auto avg_lat=(lat1+lat2)/2.;
  auto a=(lat1-lat2)*km;
  a*=a;
  auto b=(lon1-lon2)*cos(avg_lat*rad)*km;
  b*=b;
  return sqrt(a+b);
}

double gc_distance(double lat1,double lon1,double lat2,double lon2)
{
  const double rad=3.141592654/180.;
  const double R=6372.795477598; /* quadratic mean from wikipedia */
  double a;

// Haversine formula
  a=pow(sin(fabs(lat2-lat1)/2.*rad),2.)+cos(lat1*rad)*cos(lat2*rad)*pow(sin(fabs(lon2-lon1)/2.*rad),2.);
  return 2.*atan2(sqrt(a),sqrt(1.-a))*R;
}

} // end namespace geoutils
