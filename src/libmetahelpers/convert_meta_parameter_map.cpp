#include <string>
#include <strutils.hpp>

namespace metatranslations {

std::string convert_meta_parameter_map(std::string format,std::string map,bool verboseOutput)
{
  std::string s;
  if (format == "WMO_GRIB1") {
    if (verboseOutput) {
	s="Center "+strutils::itos(std::stoi(map.substr(0,map.length()-6)))+"-"+strutils::itos(std::stoi(map.substr(map.length()-6,3)))+", Table "+strutils::itos(std::stoi(map.substr(map.length()-3)));
    }
    else {
	s=strutils::itos(std::stoi(map.substr(0,map.length()-6)))+"-"+strutils::itos(std::stoi(map.substr(map.length()-6,3)))+"."+strutils::itos(std::stoi(map.substr(map.length()-3)));
    }
  }
  return s;
}

} // end namespace metatranslations
