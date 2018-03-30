#include <string>
#include <strutils.hpp>
#include <grid.hpp>

namespace metatranslations {

void decode_grid_definition_parameters(std::string definition,std::string def_params,Grid::GridDimensions& grid_dim,Grid::GridDefinition& grid_def)
{
  auto def_parts=strutils::split(def_params,":");
  if (definition == "latLon") {
    grid_dim.x=std::stoi(def_parts[0]);
    grid_dim.y=std::stoi(def_parts[1]);
    grid_def.slatitude=std::stof(def_parts[2].substr(0,def_parts[2].length()-1));
    if (def_parts[2].back() == 'S') {
	grid_def.slatitude=-grid_def.slatitude;
    }
    grid_def.slongitude=std::stof(def_parts[3].substr(0,def_parts[3].length()-1));
    if (def_parts[3].back() == 'W') {
	grid_def.slongitude=-grid_def.slongitude;
    }
    grid_def.elatitude=std::stof(def_parts[4].substr(0,def_parts[4].length()-1));
    if (def_parts[4].back() == 'S') {
	grid_def.elatitude=-grid_def.elatitude;
    }
    grid_def.elongitude=std::stof(def_parts[5].substr(0,def_parts[5].length()-1));
    if (def_parts[5].back() == 'W') {
	grid_def.elongitude=-grid_def.elongitude;
    }
    grid_def.loincrement=std::stof(def_parts[6]);
    grid_def.laincrement=std::stof(def_parts[7]);
  }
  else if (definition == "gaussLatLon") {
    grid_dim.x=std::stoi(def_parts[0]);
    grid_dim.y=std::stoi(def_parts[1]);
    grid_def.slatitude=std::stof(def_parts[2].substr(0,def_parts[2].length()-1));
    if (def_parts[2].back() == 'S') {
	grid_def.slatitude=-grid_def.slatitude;
    }
    grid_def.slongitude=std::stof(def_parts[3].substr(0,def_parts[3].length()-1));
    if (def_parts[3].back() == 'W') {
	grid_def.slongitude=-grid_def.slongitude;
    }
    grid_def.elatitude=std::stof(def_parts[4].substr(0,def_parts[4].length()-1));
    if (def_parts[4].back() == 'S') {
	grid_def.elatitude=-grid_def.elatitude;
    }
    grid_def.elongitude=std::stof(def_parts[5].substr(0,def_parts[5].length()-1));
    if (def_parts[5].back() == 'W') {
	grid_def.elongitude=-grid_def.elongitude;
    }
    grid_def.loincrement=std::stof(def_parts[6]);
    grid_def.num_circles=std::stoi(def_parts[7]);
  }
  else if (definition == "polarStereographic") {
    grid_dim.x=std::stoi(def_parts[0]);
    grid_dim.y=std::stoi(def_parts[1]);
    grid_def.slatitude=std::stof(def_parts[2].substr(0,def_parts[2].length()-1));
    if (def_parts[2].back() == 'S') {
	grid_def.slatitude=-grid_def.slatitude;
    }
    grid_def.slongitude=std::stof(def_parts[3].substr(0,def_parts[3].length()-1));
    if (def_parts[3].back() == 'W') {
	grid_def.slongitude=-grid_def.slongitude;
    }
    grid_def.llatitude=std::stof(def_parts[4].substr(0,def_parts[4].length()-1));
    if (def_parts[4].back() == 'S') {
	grid_def.llatitude=-grid_def.llatitude;
    }
    grid_def.olongitude=std::stof(def_parts[5].substr(0,def_parts[5].length()-1));
    if (def_parts[5].back() == 'W') {
	grid_def.olongitude=-grid_def.olongitude;
    }
    grid_def.projection_flag= (def_parts[6] == "N") ? 0 : 1;
    grid_def.dx=std::stof(def_parts[7]);
    grid_def.dy=std::stof(def_parts[8]);
  }
  else {
    grid_dim.x=grid_dim.y=0;
    grid_def.slatitude=grid_def.slongitude=grid_def.elatitude=grid_def.elongitude=0;
    grid_def.loincrement=grid_def.laincrement=0.;
  }
}

} // end namespace metatranslations
