#include <pibal.hpp>

Pibal::PibalLevel Pibal::level(short level_number) const
{
  if (level_number < pibal.nlev) {
    return levels[level_number];
  }
  else {
    PibalLevel blank;
    blank.height=-9999;
    blank.wind_direction=-999;
    blank.wind_speed=-999.9;
    return blank;
  }
}
