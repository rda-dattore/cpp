#include <string>

namespace metatranslations {

std::string translate_platform(std::string definition)
{
  if (definition == "aircraft") {
    return "Aircraft";
  }
  if (definition == "bogus") {
    return "Bogus";
  }
  else if (definition == "CMAN") {
    return "Coastal Marine Automated Network (CMAN) Station";
  }
  else if (definition == "driftingBuoy") {
    return "Drifting Buoy";
  }
  else if (definition == "landStation") {
    return "Land Station";
  }
  else if (definition == "mooredBuoy") {
    return "Moored Buoy";
  }
  else if (definition == "rovingShip") {
    return "Roving Ship";
  }
  else if (definition == "satellite") {
    return "Satellite";
  }
  else {
    return "";
  }
}

} // end namespace metatranslations
