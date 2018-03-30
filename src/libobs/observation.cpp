#include <observation.hpp>

const bool Observation::header_only=true,
           Observation::full_report=false;

bool operator==(const Observation::ObservationLocation& loc1,const Observation::ObservationLocation& loc2)
{
  if (loc1.ID != loc2.ID) {
    return false;
  }
  if (loc1.latitude != loc2.latitude) {
    return false;
  }
  if (loc1.longitude != loc2.longitude) {
    return false;
  }
  if (loc1.elevation != loc2.elevation) {
    return false;
  }
  return true;
}

bool operator!=(const Observation::ObservationLocation& loc1,const Observation::ObservationLocation& loc2)
{
  return !(loc1 == loc2);
}
