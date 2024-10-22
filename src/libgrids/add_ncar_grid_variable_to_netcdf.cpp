#include <netcdf.hpp>
#include <grid.hpp>
#include <strutils.hpp>
#include <utils.hpp>

namespace grid_to_netcdf {

void add_ncar_grid_variable_to_netcdf(OutputNetCDFStream *ostream, const
    Grid *source_grid, size_t num_dims, size_t *dim_ids)
{
  std::string var_name, attr_string;
  switch (source_grid->parameter()) {
    case 1: {
      var_name="z_" + strutils::itos(source_grid->first_level_value());
      ostream->add_variable(var_name, NetCDF::DataType::FLOAT, num_dims,
          dim_ids);
      ostream->add_variable_attribute(var_name, "standard_name", std::string(
          "geopotential_height"));
      attr_string="geopotential height at " + strutils::itos(source_grid->
          first_level_value()) + "mb";
      ostream->add_variable_attribute(var_name, "long_name", attr_string);
      ostream->add_variable_attribute(var_name, "units", std::string("meter"));
      break;
    }
    case 5: {
      var_name="w_" + strutils::itos(source_grid->first_level_value());
      ostream->add_variable(var_name, NetCDF::DataType::FLOAT, num_dims,
          dim_ids);
      ostream->add_variable_attribute(var_name, "standard_name", std::string(
          "lagrangian_tendency_of_air_pressure"));
      attr_string += "omega at " + strutils::itos(source_grid->first_level_value()) + "mb";
      ostream->add_variable_attribute(var_name, "long_name", attr_string);
      ostream->add_variable_attribute(var_name, "units", std::string(
          "hPa second-1"));
      break;
    }
    case 6:
    case 28: {
      ostream->add_variable("slp", NetCDF::DataType::FLOAT, num_dims, dim_ids);
      ostream->add_variable_attribute("slp", "standard_name", std::string(
          "air_pressure_at_sea_level"));
      ostream->add_variable_attribute("slp", "long_name", std::string(
          "sea level pressure"));
      ostream->add_variable_attribute("slp", "units", std::string("hPa"));
      ostream->add_variable_attribute("slp", "missing_value",
          static_cast<float>(Grid::MISSING_VALUE));
      break;
    }
    case 10: {
      var_name="t_" + strutils::itos(source_grid->first_level_value());
      ostream->add_variable(var_name, NetCDF::DataType::FLOAT, num_dims,
          dim_ids);
      ostream->add_variable_attribute(var_name, "standard_name", std::string(
          "air_temperature"));
      attr_string += "temperature at " + strutils::itos(source_grid->
          first_level_value()) + "mb";
      ostream->add_variable_attribute(var_name, "long_name", attr_string);
      ostream->add_variable_attribute(var_name, "units", std::string(
          "degreeC"));
      break;
    }
    case 11: {
      var_name="tv_" + strutils::itos(source_grid->first_level_value());
      ostream->add_variable(var_name, NetCDF::DataType::FLOAT, num_dims,
          dim_ids);
      ostream->add_variable_attribute(var_name, "name", var_name);
      attr_string += "virtual temperature at " + strutils::itos(source_grid->
          first_level_value()) + "mb";
      ostream->add_variable_attribute(var_name, "long_name", attr_string);
      ostream->add_variable_attribute(var_name, "units", std::string(
          "degreeC"));
      break;
    }
    case 30: {
      var_name="u_" + strutils::itos(source_grid->first_level_value());
      ostream->add_variable(var_name, NetCDF::DataType::FLOAT, num_dims,
          dim_ids);
      ostream->add_variable_attribute(var_name, "standard_name", std::string(
          "eastward_wind"));
      attr_string += "u wind component at " + strutils::itos(source_grid->
          first_level_value()) + "mb";
      ostream->add_variable_attribute(var_name, "long_name", attr_string);
      ostream->add_variable_attribute(var_name, "units", std::string(
          "meter second-1"));
      break;
    }
    case 31: {
      var_name="v_";
      var_name="v_" + strutils::itos(source_grid->first_level_value());
      ostream->add_variable(var_name, NetCDF::DataType::FLOAT, num_dims,
          dim_ids);
      ostream->add_variable_attribute(var_name, "standard_name", std::string(
          "northward_wind"));
      attr_string += "v wind component at " + strutils::itos(source_grid->
          first_level_value()) + "mb";
      ostream->add_variable_attribute(var_name, "long_name", attr_string);
      ostream->add_variable_attribute(var_name, "units", std::string(
          "meter second-1"));
      break;
    }
    case 44: {
      var_name="rh_";
      var_name="rh_" + strutils::itos(source_grid->first_level_value()) + "_" +
          strutils::itos(source_grid->second_level_value());
      ostream->add_variable(var_name, NetCDF::DataType::FLOAT, num_dims,
          dim_ids);
      ostream->add_variable_attribute(var_name, "standard_name", std::string(
          "relative_humidity"));
      attr_string += "relative humidity in the layer between " + strutils::itos(
          source_grid->first_level_value()) + "mb and " + strutils::itos(
          source_grid->second_level_value()) + "mb";
      ostream->add_variable_attribute(var_name, "long_name", attr_string);
      ostream->add_variable_attribute(var_name, "units", std::string(
          "percent"));
      break;
    }
  }
}

} // end namespace grid_to_netcdf
