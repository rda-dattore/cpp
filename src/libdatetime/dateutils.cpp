#include <iostream>
#include <datetime.hpp>
#include <strutils.hpp>

using std::string;
using strutils::replace_all;

namespace dateutils {

/* deprecated
void decode_julian_day(int jul_day, size_t& year, size_t& month, size_t& day,
    size_t base_year) {
  size_t *d;
  if (is_leap_year(year, "")) {
    d = const_cast<size_t *>(DateTime::DAYS_IN_MONTH_LEAP);
  } else {
    d = const_cast<size_t *>(DateTime::DAYS_IN_MONTH_NOLEAP);
  }

  // get the year for this julian day
  jul_day -= 365;
  if ( (base_year % 4) == 0) {
    --jul_day;
  }
  while (jul_day > 0) {
    ++base_year;
    jul_day -= 365;
    if ( (base_year % 4) == 0) {
      --jul_day;
    }
  }
  jul_day += 365;
  if ( (base_year % 4) == 0) {
    ++jul_day;
  }
  year = base_year;

  // get the month for this julian day
  size_t n;
  for (n = 0; jul_day > 0; ++n) {
    jul_day -= d[n];
  }
  jul_day += d[n - 1];
  month = n;

  // get the day for this julian day
  day = jul_day;
}
*/

size_t days_in_month(size_t year, size_t month, string calendar) {
  if (month < 1 || month > 12) {
    return 0;
  }
  if (calendar == "360_day") {
    return 30;
  }
  if (month != 2 || is_leap_year(year, calendar)) {
    return DateTime::DAYS_IN_MONTH_LEAP[month];
  }
  return DateTime::DAYS_IN_MONTH_NOLEAP[month];
}

/* deprecated
int julian_day(size_t year, size_t month, size_t day, size_t base_year) {
  size_t *d;
  if (is_leap_year(year, "")) {
    d = const_cast<size_t *>(DateTime::DAYS_IN_MONTH_LEAP);
  } else {
    d = const_cast<size_t *>(DateTime::DAYS_IN_MONTH_NOLEAP);
  }
  if (year < base_year) {
    std::cerr << "julian_day(): error -- " << year << " is less than base "
        "year " << base_year << std::endl;
    return -1;
  }

  // sum the days up to the current year
  for (size_t n = base_year; n < year; ++n) {
    day += is_leap_year(year, "") == 0 ? 366 : 365;
  }

  // sum the days up to the current month
  for (size_t n = 0; n < month; ++n) {
    day += d[n];
  }
  return day;
}
*/

string string_date_to_ll_string(string date) {
  replace_all(date, "-", "");
  replace_all(date, " ", "");
  replace_all(date, ":", "");
  return date;
}

string string_ll_to_date_string(string ll_date) {
  if (ll_date.length() > 4) {
    ll_date.insert(4, "-");
  }
  if (ll_date.length() > 7) {
    ll_date.insert(7, "-");
  }
  if (ll_date.length() > 10) {
    ll_date.insert(10, " ");
    if (ll_date.length() > 13) {
        ll_date.insert(13, ":");
        if (ll_date.length() > 16) {
          ll_date.insert(16, ":");
        }
    }
  }
  return ll_date;
}

DateTime current_date_time() {
  DateTime d;
  d.set_to_current();
  return d;
}

DateTime from_unixtime(long long timestamp) {
  return DateTime(1970, 1, 1, 0, 0).seconds_added(timestamp);
}

bool is_leap_year(size_t year, string calendar) {
  if (calendar.empty() || calendar == "standard" || calendar == "gregorian" ||
      calendar == "proleptic_gregorian") {
    if ( (year % 4) == 0 && ( (year % 100 != 0) || (year % 400) == 0)) {
      return true;
    } else {
      return false;
    }
  }
  if (calendar == "366_day" || calendar == "all_leap") {
    return true;
  }
  return false;
}

} // end namespace dateutils
