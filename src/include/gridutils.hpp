// FILE: gridutils.h

#ifndef GRIDUTILS_H
#define   GRIDUTILS_H

#include <unordered_map>
#include <xml.hpp>
#include <grid.hpp>

namespace gridutils {

extern std::string convert_grid_definition(std::string d);
extern std::string convert_grid_definition(std::string d, XMLElement e);

extern size_t find_grid_precision(double reference_value);

extern bool fill_gaussian_latitudes(std::string path_to_gauslat_lists, my::map<
    Grid::GLatEntry>& gaus_lats, size_t num_circles, bool grid_scans_N_to_S);
extern bool fill_spatial_domain_from_grid_definition(std::string definition,
    std::string center, double& west_lon, double& south_lat, double& east_lon,
    double& north_lat);

extern void cartesianll(Grid::GridDimensions dim, Grid::GridDefinition def, int
    pole_x, int pole_y, double **lat, double **lon);
extern void fill_lat_lons_from_lambert_conformal_gridpoints(size_t num_x, size_t
    num_y, double lat1, double elon1, double dx, double elonv, double tanlat,
    std::vector<float>& lats, std::vector<float>& elons);
extern void fill_lat_lon_from_lambert_conformal_gridpoint(size_t i, size_t j,
    double lat1, double elon1, double dx, double elonv, double tanlat, double&
    lat, double& elon);
extern void fill_lat_lon_from_polar_stereographic_gridpoint(size_t i, size_t j,
    size_t num_i, size_t num_j, double dx, double elonv, char projection, double
    dx_lat, double& lat, double& elon);
extern void fill_spatial_domain_from_lambert_conformal_grid(size_t num_i, size_t
    num_j, double lat1, double elon1, double dx, double elonv, double tanlat,
    double& west_lon, double& south_lat, double& east_lon, double& north_lat);
extern void fill_spatial_domain_from_polar_stereographic_grid(size_t num_x,
    size_t num_y, double dx, double elonv, char projection, double dx_lat,
    double& west_lon, double& south_lat, double& east_lon, double& north_lat);
extern void fill_spatial_domain_from_polar_stereographic_grid2(size_t num_x,
    size_t num_y, double start_lat, double start_elon, double dx, double elonv,
    char projection, double dx_lat, double& west_lon, double& south_lat, double&
    east_lon, double& north_lat);

extern Grid::GridDefinition fix_grid_definition(const Grid::GridDefinition&
    grid_definition, const Grid::GridDimensions& grid_dimensions);

namespace gridSubset {

struct SubsetBounds {
  SubsetBounds() : nlat(99.), slat(-99.), wlon(-999.), elon(999.) { }

  float nlat,slat;
  float wlon,elon;
};

struct Args {
  Args() : dsnum(), startdate(), enddate(), grid_definition_code(),
      ensemble_id(), tindex(), inittime(), level_codes(), product_codes(),
      parameters(), subset_bounds(), dates_are_init(false) { }

  std::string dsnum, startdate, enddate, grid_definition_code, ensemble_id,
      tindex, inittime;
  std::list<std::string> level_codes, product_codes;
  std::unordered_map<std::string, std::string> parameters;
  SubsetBounds subset_bounds;
  bool dates_are_init;
};

extern void decode_grid_subset_string(std::string rinfo, struct Args& args);

} // end namespace gridSubset

} // end namespace gridutils

#endif
