#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>

const double Grid::missing_value=3.4e38;
const double Grid::bad_value=-3.4e38;
const bool Grid::header_only=true,
           Grid::full_grid=false;

Grid::~Grid()
{
  int n;

  if (gridpoints_ != nullptr) {
    for (n=0; n < dim.y; n++)
	delete[] gridpoints_[n];
    delete[] gridpoints_;
    gridpoints_=nullptr;
  }
  if (num_in_sum != nullptr) {
    for (n=0; n < dim.y; n++)
	delete[] num_in_sum[n];
    delete[] num_in_sum;
    num_in_sum=nullptr;
  }
}

void Grid::add(float a)
{
  int n,m;

  if (!grid.filled)
    return;

  grid.pole+=a;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  gridpoints_[n][m]+=a;
	}
    }
  }
  stats.max_val+=a;
  stats.min_val+=a;
}

void Grid::subtract(float sub)
{
  int n,m;

  if (!grid.filled)
    return;

  grid.pole-=sub;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  gridpoints_[n][m]-=sub;
	}
    }
  }
  stats.max_val-=sub;
  stats.min_val-=sub;
}

void Grid::multiply_by(float mult)
{
  int n,m;

  if (!grid.filled)
    return;

  grid.pole*=mult;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  gridpoints_[n][m]*=mult;
	}
    }
  }
  stats.max_val*=mult;
  stats.min_val*=mult;
}

void Grid::divide_by(double div)
{
  int n,m;

  if (!grid.filled) {
    return;
  }
  grid.pole/=div;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  gridpoints_[n][m]/=div;
	}
    }
  }
  stats.max_val/=div;
  stats.min_val/=div;
}

void Grid::compute_mean()
{
  int n,m,avg_cnt=0;

  if (num_in_sum == nullptr) {
    std::cerr << "Error: trying to compute the mean of a non-summed grid" << std::endl;
    exit(1);
  }
  if (!grid.filled) {
    return;
  }
  if (grid.num_in_pole_sum >= 10) {
    grid.pole/=static_cast<double>(grid.num_in_pole_sum);
  }
  else {
    grid.pole=Grid::missing_value;
  }
  grid.num_in_pole_sum=0;
  stats.max_val=-Grid::missing_value;
  stats.min_val=Grid::missing_value;
  stats.avg_val=0.;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	if (num_in_sum[n][m] >= 10) {
	  gridpoints_[n][m]/=static_cast<double>(num_in_sum[n][m]);
	  if (gridpoints_[n][m] > stats.max_val) {
	    stats.max_val=gridpoints_[n][m];
	    stats.max_i=m+1;
	    stats.max_j=n+1;
	  }
	  else if (gridpoints_[n][m] < stats.min_val) {
	    stats.min_val=gridpoints_[n][m];
	    stats.min_i=m+1;
	    stats.min_j=n+1;
	  }
	  stats.avg_val+=gridpoints_[n][m];
	  avg_cnt++;
	}
	else {
	  gridpoints_[n][m]=Grid::missing_value;
	}
	num_in_sum[n][m]=0;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  else {
    stats.max_val=Grid::missing_value;
    stats.avg_val=Grid::missing_value;
  }
}

double Grid::gridpoint(size_t x_location,size_t y_location) const
{
  return (gridpoints_ != nullptr) ? gridpoints_[y_location][x_location] : Grid::missing_value;
}

double Grid::gridpoint_at(float latitude,float longitude) const
{
  int n,m;

  if (gridpoints_ == nullptr) {
    return Grid::bad_value;
  }
  if (def.type == polarStereographicType) {
return -999.9;
  }
  else {
    n=latitude_index_of(latitude,nullptr);
    if (n < 0) {
	return Grid::bad_value;
    }
  }
  m=longitude_index_of(longitude);
  if (m < 0) {
    return Grid::bad_value;
  }
  if (n > dim.y || m > dim.x) {
    return Grid::bad_value;
  }
  else {
    return gridpoint(m,n);
  }
}

int Grid::latitude_index_of(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  float x=(latitude-def.slatitude)/def.laincrement;
  if (x < 0.) {
    if (latitude < def.elatitude) {
	return -1;
    }
    else {
	x=-x;
    }
  }
  int n=lroundf(x);
  if (!floatutils::myequalf(n,x,0.01)) {
    return -1;
  }
  else {
    return n;
  }
}

int Grid::latitude_index_north_of(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  float x;

  x=(latitude-def.slatitude)/def.laincrement;
  if (x < 0.) {
    return -x;
  }
  else {
    return (x+1);
  }
}

int Grid::latitude_index_south_of(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  float x;

  x=(latitude-def.slatitude)/def.laincrement;
  if (x < 0.) {
    return lroundf(-x);
  }
  else {
    return x;
  }
}

int Grid::longitude_index_of(float longitude) const
{
  if (def.elongitude > 180. && longitude < 0.) {
    longitude+=360.;
  }
  double x=(longitude-def.slongitude)/def.loincrement;
  int m=lroundf(x);
  if (!floatutils::myequalf(m,x,0.01)) {
    return -1;
  }
  else {
    return m;
  }
}

int Grid::longitude_index_east_of(float longitude) const
{
  int x;

  if (def.elongitude > 180. && longitude < 0.) {
    longitude+=360.;
  }
  x=(longitude-def.slongitude)/def.loincrement;
  return (x+1);
}

int Grid::longitude_index_west_of(float longitude) const
{
  if (def.elongitude > 180. && longitude < 0.) {
    longitude+=360.;
  }
  return (longitude-def.slongitude)/def.loincrement;
}

bool operator==(const Grid::GridDefinition& def1,const Grid::GridDefinition& def2)
{
  if (def1.type != def2.type) {
    std::cerr << "grid types don't match" << std::endl;
    return false;
  }
  if (!floatutils::myequalf(def1.slatitude,def2.slatitude)) {
    std::cerr << "start latitude difference: " << fabs(def1.slatitude-def2.slatitude) << std::endl;
    return false;
  }
  if (!floatutils::myequalf(def1.slongitude,def2.slongitude)) {
    std::cerr << "start longitude difference: " << fabs(def1.slongitude-def2.slongitude) << std::endl;
    return false;
  }
  if (!floatutils::myequalf(def1.elatitude,def2.elatitude)) {
    std::cerr << "end latitude difference: " << fabs(def1.elatitude-def2.elatitude) << std::endl;
    return false;
  }
  if (!floatutils::myequalf(def1.elongitude,def2.elongitude)) {
    std::cerr << "end longitude difference: " << fabs(def1.elongitude-def2.elongitude) << std::endl;
    return false;
  }
  if (!floatutils::myequalf(def1.loincrement,def2.loincrement)) {
    std::cerr << "longitude increment difference: " << fabs(def1.loincrement-def2.loincrement) << std::endl;
    return false;
  }
  if (!floatutils::myequalf(def1.laincrement,def2.laincrement)) {
    std::cerr << "latitude increment difference: " << fabs(def1.laincrement-def2.laincrement) << std::endl;
    return false;
  }
  if (def1.projection_flag != def2.projection_flag) {
    std::cerr << "projection flags don't match" << std::endl;
    return false;
  }
  if (def1.num_centers != def2.num_centers) {
    std::cerr << "number of centers don't match" << std::endl;
    return false;
  }
  if (!floatutils::myequalf(def1.stdparallel1,def2.stdparallel1)) {
    std::cerr << "stdparallel1 difference: " << fabs(def1.stdparallel1-def2.stdparallel1) << std::endl;
    return false;
  }
  if (!floatutils::myequalf(def1.stdparallel2,def2.stdparallel2)) {
    std::cerr << "stdparallel2 difference: " << fabs(def1.stdparallel2-def2.stdparallel2) << std::endl;
    return false;
  }
  return true;
}

bool operator!=(const Grid::GridDefinition& def1,const Grid::GridDefinition& def2)
{
  return !(def1 == def2);
}

bool operator==(const Grid::GridDimensions& dim1,const Grid::GridDimensions& dim2)
{
  if (dim1.x != dim2.x)
    return false;
  if (dim1.y != dim2.y)
    return false;
  if (dim1.size != dim2.size)
    return false;
  return true;
}

bool operator!=(const Grid::GridDimensions& dim1,const Grid::GridDimensions& dim2)
{
  return !(dim1 == dim2);
}

namespace gridConversions {

void convert_grid_parameter_string(size_t format,const char *string,std::vector<short>& params)
{
  if (format == 0) {
    std::cerr << "Error: can't convert parameter when format is not known" << std::endl;
    exit(1);
  }

  switch (format) {
    case (Grid::octagonalFormat):
    case (Grid::tropicalFormat):
    case (Grid::latlonFormat):
    {
	std::string s=string;
	if (s == "z") {
	  params[params.size()]=1;
	}
	else if (s == "vvel") {
	  params[params.size()]=5;
	}
	else if (s == "slp") {
	  params[params.size()]=6;
	  params[params.size()]=28;
	}
	else if (s == "t") {
	  params[params.size()]=10;
	  params[params.size()]=11;
	}
	else if (s == "u") {
	  params[params.size()]=30;
	}
	else if (s == "v") {
	  params[params.size()]=31;
	}
	else if (s == "wind") {
	  params[params.size()]=30;
	  params[params.size()]=31;
	}
	else if (s == "rh") {
	  params[params.size()]=44;
	}
	else if (s == "sst") {
	  params[params.size()]=47;
	  params[params.size()]=57;
	}
	break;
    }
    default:
    {
	std::cerr << "Error: can't convert parameters for format " << format << std::endl;
	exit(1);
    }
  }
}

} // end namespace gridConversions
