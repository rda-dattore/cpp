#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>

using std::cerr;
using std::endl;

const double Grid::MISSING_VALUE = 3.4e38;
const double Grid::BAD_VALUE = -3.4e38;
const bool Grid::HEADER_ONLY = true, Grid::FULL_GRID = false;

Grid::~Grid() {
  if (m_gridpoints != nullptr) {
    for (int n = 0; n < dim.y; ++n) {
      delete[] m_gridpoints[n];
    }
    delete[] m_gridpoints;
    m_gridpoints = nullptr;
  }
  if (num_in_sum != nullptr) {
    for (int n = 0; n < dim.y; ++n) {
      delete[] num_in_sum[n];
    }
    delete[] num_in_sum;
    num_in_sum = nullptr;
  }
}

void Grid::add(float a) {
  if (!grid.filled) {
    return;
  }
  grid.pole += a;
  for (int n = 0; n < dim.y; ++n) {
    for (int m = 0; m < dim.x; ++m) {
      if (!floatutils::myequalf(m_gridpoints[n][m], MISSING_VALUE)) {
        m_gridpoints[n][m] += a;
      }
    }
  }
  stats.max_val += a;
  stats.min_val += a;
}

void Grid::subtract(float sub) {
  if (!grid.filled) {
    return;
  }
  grid.pole -= sub;
  for (int n = 0; n < dim.y; ++n) {
    for (int m = 0; m < dim.x; ++m) {
      if (!floatutils::myequalf(m_gridpoints[n][m], MISSING_VALUE)) {
        m_gridpoints[n][m] -= sub;
      }
    }
  }
  stats.max_val -= sub;
  stats.min_val -= sub;
}

void Grid::multiply_by(float mult) {
  if (!grid.filled) {
    return;
  }
  grid.pole *= mult;
  for (int n = 0; n < dim.y; ++n) {
    for (int m = 0; m < dim.x; ++m) {
      if (!floatutils::myequalf(m_gridpoints[n][m], MISSING_VALUE)) {
        m_gridpoints[n][m] *= mult;
      }
    }
  }
  stats.max_val *= mult;
  stats.min_val *= mult;
}

void Grid::divide_by(double div) {
  if (!grid.filled) {
    return;
  }
  grid.pole /= div;
  for (int n = 0; n < dim.y; ++n) {
    for (int m = 0; m < dim.x; ++m) {
      if (!floatutils::myequalf(m_gridpoints[n][m], MISSING_VALUE)) {
        m_gridpoints[n][m] /= div;
      }
    }
  }
  stats.max_val /= div;
  stats.min_val /= div;
}

void Grid::compute_mean() {
  if (num_in_sum == nullptr) {
    cerr << "Error: trying to compute the mean of a non-summed grid" << endl;
    exit(1);
  }
  if (!grid.filled) {
    return;
  }
  if (grid.num_in_pole_sum >= 10) {
    grid.pole /= static_cast<double>(grid.num_in_pole_sum);
  } else {
    grid.pole = MISSING_VALUE;
  }
  grid.num_in_pole_sum = 0;
  stats.max_val = -MISSING_VALUE;
  stats.min_val = MISSING_VALUE;
  stats.avg_val = 0.;
  auto avg_cnt = 0;
  for (int n = 0; n < dim.y; ++n) {
    for (int m = 0; m < dim.x; ++m) {
      if (num_in_sum[n][m] >= 10) {
        m_gridpoints[n][m] /= static_cast<double>(num_in_sum[n][m]);
        if (m_gridpoints[n][m] > stats.max_val) {
          stats.max_val = m_gridpoints[n][m];
          stats.max_i = m + 1;
          stats.max_j = n + 1;
        } else if (m_gridpoints[n][m] < stats.min_val) {
          stats.min_val = m_gridpoints[n][m];
          stats.min_i = m + 1;
          stats.min_j = n + 1;
        }
        stats.avg_val += m_gridpoints[n][m];
        ++avg_cnt;
      } else {
        m_gridpoints[n][m] = MISSING_VALUE;
      }
      num_in_sum[n][m] = 0;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val /= static_cast<double>(avg_cnt);
  } else {
    stats.max_val = MISSING_VALUE;
    stats.avg_val = MISSING_VALUE;
  }
}

double Grid::gridpoint(size_t x_location, size_t y_location) const {
  return (m_gridpoints != nullptr) ? m_gridpoints[y_location][x_location] :
      MISSING_VALUE;
}

double Grid::gridpoint_at(float latitude, float longitude) const {
  if (m_gridpoints == nullptr) {
    return Grid::BAD_VALUE;
  }
  int n, m;
  if (def.type == Type::polarStereographic) {
return -999.9;
  } else {
    n = latitude_index_of(latitude, nullptr);
    if (n < 0) {
      return Grid::BAD_VALUE;
    }
  }
  m = longitude_index_of(longitude);
  if (m < 0) {
    return Grid::BAD_VALUE;
  }
  if (n > dim.y || m > dim.x) {
    return Grid::BAD_VALUE;
  } else {
    return gridpoint(m, n);
  }
}

int Grid::latitude_index_of(float latitude, my::map<GLatEntry> *gaus_lats)
    const {
  float x = (latitude - def.slatitude) / def.laincrement;
  if (x < 0.) {
    if (latitude < def.elatitude) {
      return -1;
    } else {
      x = -x;
    }
  }
  int n = lroundf(x);
  if (!floatutils::myequalf(n, x, 0.01)) {
    return -1;
  } else {
    return n;
  }
}

int Grid::latitude_index_north_of(float latitude, my::map<GLatEntry> *gaus_lats)
    const {
  float x = (latitude - def.slatitude) / def.laincrement;
  if (x < 0.) {
    return -x;
  } else {
    return (x + 1);
  }
}

int Grid::latitude_index_south_of(float latitude, my::map<GLatEntry> *gaus_lats)
    const {
  float x = (latitude - def.slatitude) / def.laincrement;
  if (x < 0.) {
    return lroundf(-x);
  } else {
    return x;
  }
}

int Grid::longitude_index_of(float longitude) const {
  if (longitude < 0.) {
    if (def.elongitude > 180. || def.elongitude < def.slongitude) {
      longitude += 360.;
    }
  }
  double x = (longitude - def.slongitude) / def.loincrement;
  int m = lroundf(x);
  if (!floatutils::myequalf(m, x, 0.01)) {
    return -1;
  } else {
    return m;
  }
}

int Grid::longitude_index_east_of(float longitude) const {
  if (def.elongitude > 180. && longitude < 0.) {
    longitude += 360.;
  }
  int x = (longitude - def.slongitude) / def.loincrement;
  return (x + 1);
}

int Grid::longitude_index_west_of(float longitude) const {
  if (def.elongitude > 180. && longitude < 0.) {
    longitude += 360.;
  }
  return (longitude - def.slongitude) / def.loincrement;
}

bool operator==(const Grid::GridDefinition& def1, const Grid::GridDefinition&
    def2) {
  if (def1.type != def2.type) {
    cerr << "grid types don't match" << endl;
    return false;
  }
  if (!floatutils::myequalf(def1.slatitude,def2.slatitude)) {
    cerr << "start latitude difference: " << fabs(def1.slatitude -
        def2.slatitude) << endl;
    return false;
  }
  if (!floatutils::myequalf(def1.slongitude,def2.slongitude)) {
    cerr << "start longitude difference: " << fabs(def1.slongitude -
        def2.slongitude) << endl;
    return false;
  }
  if (!floatutils::myequalf(def1.elatitude,def2.elatitude)) {
    cerr << "end latitude difference: " << fabs(def1.elatitude -
        def2.elatitude) << endl;
    return false;
  }
  if (!floatutils::myequalf(def1.elongitude,def2.elongitude)) {
    cerr << "end longitude difference: " << fabs(def1.elongitude -
        def2.elongitude) << endl;
    return false;
  }
  if (!floatutils::myequalf(def1.loincrement,def2.loincrement)) {
    cerr << "longitude increment difference: " << fabs(def1.loincrement -
        def2.loincrement) << endl;
    return false;
  }
  if (!floatutils::myequalf(def1.laincrement,def2.laincrement)) {
    cerr << "latitude increment difference: " << fabs(def1.laincrement -
        def2.laincrement) << endl;
    return false;
  }
  if (def1.projection_flag != def2.projection_flag) {
    cerr << "projection flags don't match" << endl;
    return false;
  }
  if (def1.num_centers != def2.num_centers) {
    cerr << "number of centers don't match" << endl;
    return false;
  }
  if (!floatutils::myequalf(def1.stdparallel1,def2.stdparallel1)) {
    cerr << "stdparallel1 difference: " << fabs(def1.stdparallel1 -
        def2.stdparallel1) << endl;
    return false;
  }
  if (!floatutils::myequalf(def1.stdparallel2,def2.stdparallel2)) {
    cerr << "stdparallel2 difference: " << fabs(def1.stdparallel2 -
        def2.stdparallel2) << endl;
    return false;
  }
  return true;
}

bool operator!=(const Grid::GridDefinition& def1, const Grid::GridDefinition&
    def2) {
  return !(def1 == def2);
}

bool operator==(const Grid::GridDimensions& dim1, const Grid::GridDimensions&
    dim2) {
  if (dim1.x != dim2.x) {
    return false;
  }
  if (dim1.y != dim2.y) {
    return false;
  }
  if (dim1.size != dim2.size) {
    return false;
  }
  return true;
}

bool operator!=(const Grid::GridDimensions& dim1, const Grid::GridDimensions&
    dim2) {
  return !(dim1 == dim2);
}

namespace gridConversions {

void convert_grid_parameter_string(Grid::Format format, const char *string,
    std::vector<short>& params) {
  switch (format) {
    case (Grid::Format::octagonal):
    case (Grid::Format::tropical):
    case (Grid::Format::latlon): {
      std::string s = string;
      if (s == "z") {
        params[params.size()] = 1;
      } else if (s == "vvel") {
        params[params.size()] = 5;
      } else if (s == "slp") {
        params[params.size()] = 6;
        params[params.size()] = 28;
      } else if (s == "t") {
        params[params.size()] = 10;
        params[params.size()] = 11;
      } else if (s == "u") {
        params[params.size()] = 30;
      } else if (s == "v") {
        params[params.size()] = 31;
      } else if (s == "wind") {
        params[params.size()] = 30;
        params[params.size()] = 31;
      } else if (s == "rh") {
        params[params.size()] = 44;
      } else if (s == "sst") {
        params[params.size()] = 47;
        params[params.size()] = 57;
      }
      break;
    }
    default: {
      cerr << "Error: can't convert parameters for format " <<
          static_cast<int>(format) << endl;
      exit(1);
    }
  }
}

} // end namespace gridConversions
