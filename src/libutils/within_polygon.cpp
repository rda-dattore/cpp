#include <strutils.hpp>
#include <utils.hpp>

bool within_polygon(std::string polygon,std::string point)
{
  strutils::replace_all(polygon,"POLYGON","");
  strutils::replace_all(polygon,"(","");
  strutils::replace_all(polygon,")","");
  strutils::replace_all(point,"POINT","");
  strutils::replace_all(point,"(","");
  strutils::replace_all(point,")","");
// make an array of points of the polygon
  auto points=strutils::split(polygon,",");
// get the x and y coordinate of the point
  auto coords=strutils::split(point);
  auto point_x=std::stof(coords[0]);
  auto point_y=std::stof(coords[1]);
  auto poly1=points[0];
  auto cnt=0;
  for (size_t n=1; n <= points.size(); ++n) {
    coords=strutils::split(poly1);
    auto poly1_x=std::stof(coords[0]);
    auto poly1_y=std::stof(coords[1]);
    auto poly2=points[n % points.size()];
    coords=strutils::split(poly2);
    auto poly2_x=std::stof(coords[0]);
    auto poly2_y=std::stof(coords[1]);
    if (point_y > std::min(poly1_y,poly2_y)) {
	if (point_y <= std::max(poly1_y,poly2_y)) {
	  if (point_x <= std::max(poly1_x,poly2_x)) {
	    if (!floatutils::myequalf(poly1_y,poly2_y)) {
		auto xinters=(point_y-poly1_y)*(poly2_x-poly1_x)/(poly2_y-poly1_y)+poly1_x;
		if (floatutils::myequalf(poly1_x,poly2_x) || point_x <= xinters) {
		  ++cnt;
		}
	    }
	  }
	}
    }
    poly1=poly2;
  }
  if ( (cnt % 2) == 0) {
// outside
    return false;
  }
  else {
// inside
    return true;
  }
}
