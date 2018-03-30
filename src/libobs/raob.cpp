#include <raob.hpp>

Raob::RaobLevel Raob::level(short level_number) const
{
  if (level_number < nlev) {
    return levels[level_number];
  }
  else {
    RaobLevel blank;
    blank.pressure=-999.9;
    blank.height=-9999;
    return blank;
  }
}
