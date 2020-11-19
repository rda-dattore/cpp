#include <iostream>
#include <datetime.hpp>
#include <strutils.hpp>

namespace dateutils {

void decode_julian_day(int jul_day,size_t& year,size_t& month,size_t& day,size_t base_year)
{
  size_t days_per_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
// get the year for this julian day
  jul_day-=365;
  if ( (base_year % 4) == 0) jul_day--;
  while (jul_day > 0) {
    ++base_year;
    jul_day-=365;
    if ( (base_year % 4) == 0) {
	--jul_day;
    }
  }
  jul_day+=365;
  if ( (base_year % 4) == 0) {
    ++jul_day;
  }
  year=base_year;
// get the month for this julian day
  if ( (year % 4) == 0) {
    days_per_month[1]=29;
  }
  size_t n;
  for (n=0; jul_day > 0; ++n) {
    jul_day-=days_per_month[n];
  }
  jul_day+=days_per_month[n-1];
  month=n;
// get the day for this julian day
  day=jul_day;
}

size_t days_in_month(size_t year,size_t month,std::string calendar)
{
  if (month < 1 || month > 12) {
    return 0;
  }
  if (calendar == "360_day") {
    return 30;
  }
  else {
    if (month != 2 || is_leap_year(year,calendar)) {
	return DateTime::days_in_month_leap[month];
    }
    else {
	return DateTime::days_in_month_noleap[month];
    }
  }
}

int julian_day(size_t year,size_t month,size_t day,size_t base_year)
{
  size_t days_per_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  if (year < base_year) {
    std::cout << "julian_day(): error -- " << year << " is less than base year " << base_year << std::endl;
    return -1;
  }
  --month;
// sum the days up to the current year
  for (size_t n=base_year; n < year; ++n) {
    day+= ( (n % 4) == 0) ? 366 : 365;
  }
// sum the days up to the current month
  if ( (year % 4) == 0) {
    days_per_month[1]=29;
  }
  for (size_t n=0; n < month; ++n) {
    day+=days_per_month[n];
  }
  return day;
}

std::string string_date_to_ll_string(std::string date)
{
  strutils::replace_all(date,"-","");
  strutils::replace_all(date," ","");
  strutils::replace_all(date,":","");
  return date;
}

std::string string_ll_to_date_string(std::string ll_date)
{
  if (ll_date.length() > 4) {
    ll_date.insert(4,"-");
  }
  if (ll_date.length() > 7) {
    ll_date.insert(7,"-");
  }
  if (ll_date.length() > 10) {
    ll_date.insert(10," ");
    if (ll_date.length() > 13) {
        ll_date.insert(13,":");
        if (ll_date.length() > 16) {
          ll_date.insert(16,":");
        }
    }
  }
  return ll_date;
}

DateTime current_date_time()
{
  DateTime dt;
  dt.set_to_current();
  return dt;
}

DateTime from_unixtime(long long timestamp)
{
  return DateTime(1970,1,1,0,0).seconds_added(timestamp);
}

bool is_leap_year(size_t year,std::string calendar)
{
  if (calendar.empty() || calendar == "standard" || calendar == "gregorian" || calendar == "proleptic_gregorian") {
    if ( (year % 4) == 0 && ( (year % 100 != 0) || (year % 400) == 0)) {
	return true;
    }
    else {
	return false;
    }
  }
  else if (calendar == "366_day" || calendar == "all_leap") {
    return true;
  }
  else {
    return false;
  }
}

} // end namespace dateutils
