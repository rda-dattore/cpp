#include <iostream>
#include <iomanip>
#include <sstream>
#include <regex>
#include <time.h>
#include <datetime.hpp>
#include <strutils.hpp>

using std::runtime_error;
using std::string;
using strutils::ftos;
using strutils::itos;
using strutils::replace_all;

const short DateTime::LOCAL_UTC_OFFSET_FROM_STANDARD_TIME_AS_HHMM = -700;
const size_t DateTime::DAYS_IN_MONTH_NOLEAP[13] = { 0, 31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31 };
const size_t DateTime::DAYS_IN_MONTH_LEAP[13] = { 0, 31, 29, 31, 30, 31, 30, 31,
    31, 30, 31, 30, 31 };
const size_t DateTime::DAYS_IN_MONTH_360_DAY[13] = { 0, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30 };

void DateTime::add(string units, size_t num_to_add, string calendar) {
  units = strutils::to_lower(units);
  if (units == "hours") {
    add_hours(num_to_add, calendar);
  } else if (units == "days") {
    add_days(num_to_add, calendar);
  } else if (units == "months") {
    add_months(num_to_add, calendar);
  } else if (units == "years") {
    add_years(num_to_add);
  } else if (units == "minutes") {
    add_minutes(num_to_add, calendar);
  } else if (units == "seconds") {
    add_seconds(num_to_add, calendar);
  } else {
    throw runtime_error("DateTime::add(): units " + units + " not recognized.");
  }
}

void DateTime::fadd(string units, double num_to_add, string calendar) {
  units = strutils::to_lower(units);
  if (units == "hours") {
    fadd_hours(num_to_add, calendar);
  } else if (units == "days") {
    fadd_days(num_to_add, calendar);
  } else if (units == "minutes") {
    fadd_minutes(num_to_add, calendar);
  } else if (units == "seconds") {
    add_seconds(num_to_add, calendar);
  } else {
    throw runtime_error("DateTime::fadd(): units " + units + " not recognized.");
  }
}

void DateTime::add_days(size_t days_to_add, string calendar) {
  size_t *d;
  if (calendar == "360_day") {
    d = const_cast<size_t *>(DAYS_IN_MONTH_360_DAY);
  } else if (calendar == "365_day" || calendar == "noleap") {
    d = const_cast<size_t *>(DAYS_IN_MONTH_NOLEAP);
  } else {
    if (dateutils::is_leap_year(m_year, calendar)) {
      d = const_cast<size_t *>(DAYS_IN_MONTH_LEAP);
    } else {
      d = const_cast<size_t *>(DAYS_IN_MONTH_NOLEAP);
    }
    calendar = "";
  }
  auto newdy = m_day + days_to_add;
  while (newdy > d[m_month]) {
    newdy -= d[m_month];
    ++m_month;
    if (m_month > 12) {
      ++m_year;
      if (calendar.empty()) {
        if (dateutils::is_leap_year(m_year, calendar)) {
          d = const_cast<size_t *>(DAYS_IN_MONTH_LEAP);
        } else {
          d = const_cast<size_t *>(DAYS_IN_MONTH_NOLEAP);
        }
      }
      m_month = 1;
    }
  }
  m_day = newdy;
  if (m_weekday >= 0) {
    m_weekday = (m_weekday + days_to_add) % 7;
  }
}

void DateTime::add_hours(size_t hours_to_add, string calendar) {
  auto d = hours_to_add / 24;
  auto h = hours_to_add % 24;
  m_hour += h;
  if (m_hour > 23) {
    m_hour -= 24;
    ++d;
  }
  add_days(d, calendar);
}

void DateTime::add_minutes(size_t minutes_to_add, string calendar) {
  auto h = minutes_to_add / 60;
  auto m = minutes_to_add % 60;
  m_minute += m;
  if (m_minute > 59) {
    m_minute -= 60;
    ++h;
  }
  add_hours(h, calendar);
}

void DateTime::add_months(size_t months_to_add, string calendar) {
  size_t *d = nullptr;
  auto is_leap = true;
  if (calendar == "360_day") {
    d = const_cast<size_t *>(DAYS_IN_MONTH_360_DAY);
    is_leap = false;
  } else if (calendar == "365_day" || calendar == "noleap") {
    d=const_cast<size_t *>(DAYS_IN_MONTH_NOLEAP);
    is_leap = false;
  }
  for (size_t n = 0; n < months_to_add; ++n) {
    if (is_leap) {
      if (dateutils::is_leap_year(m_year, calendar)) {
        d = const_cast<size_t *>(DAYS_IN_MONTH_LEAP);
      } else {
        d = const_cast<size_t *>(DAYS_IN_MONTH_NOLEAP);
      }
      calendar = "";
    }
    add_days(d[m_month], calendar);
  }
}

void DateTime::add_seconds(size_t seconds_to_add, string calendar) {
  auto m = seconds_to_add / 60;
  auto s = seconds_to_add % 60;
  m_second += s;
  if (m_second > 59) {
    m_second -= 60;
    ++m;
  }
  add_minutes(m, calendar);
}

void DateTime::add_time(size_t hhmmss, string calendar) {
  auto h=hhmmss/10000;
  auto m=hhmmss % 10000;
  auto s = m % 100;
  if (s > 59) {
    throw runtime_error("Error: bad value " + itos(hhmmss) + " for hhmmss.");
  }
  m /= 100;
  if (m > 59) {
    throw runtime_error("Error: bad value " + itos(hhmmss) + " for hhmmss.");
  }
  add_seconds(s, calendar);
  add_minutes(m, calendar);
  add_hours(h, calendar);
}

DateTime DateTime::added(string units, size_t num_to_add, string calendar)
    const {
  DateTime d = *this;
  d.add(units, num_to_add, calendar);
  return d;
}

DateTime DateTime::fadded(string units, double num_to_add, string calendar)
    const {
  DateTime d = *this;
  d.fadd(units, num_to_add, calendar);
  return d;
}

DateTime DateTime::days_added(size_t days_to_add, string calendar) const {
  DateTime d = *this;
  d.add_days(days_to_add, calendar);
  return d;
}

DateTime DateTime::days_subtracted(size_t days_to_subtract, string calendar)
    const {
  DateTime d = *this;
  d.subtract_days(days_to_subtract, calendar);
  return d;
}

int DateTime::days_since(const DateTime& reference, string calendar) const {
  if (*this < reference) {
    return -1;
  }
  if (*this == reference) {
    return 0;
  }
  int d;
  size_t *nd;
  if (calendar == "360_day") {
    d = 360 * this->years_since(reference);
    nd = const_cast<size_t *>(DAYS_IN_MONTH_360_DAY);
  } else {
    d = 365 * this->years_since(reference);
    nd = const_cast<size_t *>(DAYS_IN_MONTH_NOLEAP);
  }
  if (d > 0 && dateutils::is_leap_year(reference.m_year, calendar) &&
      reference.m_month <= 2) {
    ++d;
  }
  for (short n = reference.m_year + 1; n < this->m_year; ++n) {
    if (dateutils::is_leap_year(n, calendar)) {
      ++d;
    }
  }
  if (dateutils::is_leap_year(this->m_year, calendar) && this->m_month > 2 &&
      (reference.m_year < this->m_year || reference.m_month <= 2)) {
    ++d;
  }
  d += this->m_day - reference.m_day;
  if (this->months_since(reference) > 0) {
    if (this->m_month < reference.m_month || (this->m_month == reference.m_month
        && (this->m_day < reference.m_day || (this->m_day == reference.m_day &&
        this->time() < reference.time())))) {
      for (short n = reference.m_month; n <= 12; ++n) {
        d += nd[n];
      }
      for (short n = 1; n < this->m_month; ++n) {
        d += nd[n];
      }
    } else {
      for (short n = reference.m_month; n < this->m_month; ++n) {
        d += nd[n];
      }
    }
  } else if (this->m_day < reference.m_day || (d == 0 && this->m_month !=
      reference.m_month)) {
    d += nd[reference.m_month];
  }
  if (this->time() < reference.time()) {
    --d;
  }
  return d;
}

int DateTime::hours_since(const DateTime& reference, string calendar) const {
  if (*this < reference) {
    return -1;
  }
  if (*this == reference) {
    return 0;
  }
  int h = 24 * this->days_since(reference, calendar);
  h += this->m_hour-reference.m_hour;
  if (this->m_hour < reference.m_hour || (this->m_hour == reference.m_hour &&
      this->time() < reference.time())) {
    h += 24;
  }
  if (this->m_minute < reference.m_minute || (this->m_minute == reference.
      m_minute && this->m_second < reference.m_second)) {
    --h;
  }
  return h;
}

int DateTime::minutes_since(const DateTime& reference, string calendar) const {
  if (*this < reference) {
    return -1;
  }
  int m = 60 * this->hours_since(reference, calendar);
  m += this->m_minute-reference.m_minute;
  if (this->m_minute < reference.m_minute || (this->m_minute == reference.
     m_minute && this->m_second < reference.m_second)) {
    m += 60;
  }
  if (this->m_second < reference.m_second) {
    --m;
  }
  return m;
}

int DateTime::months_since(const DateTime& reference) const {
  if (*this < reference) {
    return -1;
  }
  int m = 12 * this->years_since(reference);
  m += this->m_month - reference.m_month;
  if (this->m_month < reference.m_month || (this->m_month == reference.m_month
      && (this->m_day < reference.m_day || (this->m_day == reference.m_day &&
      this->time() < reference.time())))) {
    m += 12;
  }
  if (this->m_day < reference.m_day || (this->m_day == reference.m_day && this->
      time() < reference.time())) {
    --m;
  }
  return m;
}

long long DateTime::seconds_since(const DateTime& reference, string calendar)
    const {
  if (*this < reference) {
    return -1;
  }
  long long s = 60 * static_cast<long long>(this->minutes_since(reference,
      calendar));
  s += this->m_second - reference.m_second;
  if (this->m_second < reference.m_second) {
    s += 60;
  }
  return s;
}

int DateTime::time_since(const DateTime& reference, string calendar) const {
  if (*this < reference) {
    return -1;
  }
  auto h = this->hours_since(reference, calendar);
  auto m = this->minutes_since(reference, calendar) % 60;
  auto s = this->seconds_since(reference, calendar) % 60;
  return h * 10000 + m * 100 + s;
}

int DateTime::years_since(const DateTime& reference) const {
  int y;
  if (*this < reference) {
    return -1;
  }
  y = this->m_year-reference.m_year;
  if (this->m_month < reference.m_month) {
    return --y;
  } else if (this->m_month > reference.m_month) {
    return y;
  }
  if (this->m_day < reference.m_day) {
    return --y;
  } else if (this->m_day > reference.m_day) {
    return y;
  }
  if (this->time() < reference.time()) {
    return --y;
  }
  return y;
}

DateTime DateTime::hours_added(size_t hours_to_add, string calendar) const {
  DateTime d = *this;
  d.add_hours(hours_to_add, calendar);
  return d;
}

DateTime DateTime::hours_subtracted(size_t hours_to_subtract, string calendar)
    const {
  DateTime d = *this;
  d.subtract_hours(hours_to_subtract, calendar);
  return d;
}

DateTime DateTime::minutes_added(size_t minutes_to_add, string calendar)
    const {
  DateTime d = *this;
  d.add_minutes(minutes_to_add, calendar);
  return d;
}

DateTime DateTime::minutes_subtracted(size_t minutes_to_subtract, string
    calendar) const {
  DateTime d = *this;
  d.subtract_minutes(minutes_to_subtract, calendar);
  return d;
}

DateTime DateTime::months_added(size_t months_to_add, string calendar) const {
  DateTime d = *this;
  d.add_months(months_to_add, calendar);
  return d;
}

DateTime DateTime::months_subtracted(size_t months_to_subtract, string
    calendar) const {
  DateTime d = *this;
  d.subtract_months(months_to_subtract, calendar);
  return d;
}

DateTime DateTime::seconds_added(size_t seconds_to_add, string calendar) const {
  DateTime d = *this;
  d.add_seconds(seconds_to_add, calendar);
  return d;
}

DateTime DateTime::seconds_subtracted(size_t seconds_to_subtract, string
    calendar) const {
  DateTime d = *this;
  d.subtract_seconds(seconds_to_subtract, calendar);
  return d;
}

void DateTime::set(long long YYYYMMDDHHMMSS) {
  size_t t = YYYYMMDDHHMMSS % 1000000;
  YYYYMMDDHHMMSS /= 1000000;
  short d;
  if (YYYYMMDDHHMMSS > 0) {
    d = YYYYMMDDHHMMSS % 100;
    YYYYMMDDHHMMSS /= 100;
  } else {
    d = 0;
  }
  short m;
  if (YYYYMMDDHHMMSS > 0) {
    m = YYYYMMDDHHMMSS % 100;
    YYYYMMDDHHMMSS /= 100;
  } else {
    m = 0;
  }
  short y;
  if (YYYYMMDDHHMMSS > 0) {
    y = YYYYMMDDHHMMSS;
  } else {
    y = 0;
  }
  set(y, m, d, t);
}

void DateTime::set(short year, short month, short day, size_t hhmmss, short
    utc_offset_as_hhmm) {
  DateTime b;
  b.m_year = 1970;
  b.m_month = 1;
  b.m_day = 4;
  m_year = year;
  m_month=month;
  m_day = day;
  if (*this == b) {
    m_weekday = 0;
  } else if (*this > b) {
    m_weekday = (days_since(b) % 7);
  } else {
    m_weekday = ((7 - (b.days_since(*this) % 7)) % 7);
  }
  set_time(hhmmss);
  set_utc_offset(utc_offset_as_hhmm);
}

void DateTime::set(size_t hours_to_add, const DateTime& reference, string
    calendar) {
  *this = reference;
  this->add_hours(hours_to_add, calendar);
}

void DateTime::set_to_current(bool set_to_utc) {
  auto tm = ::time(nullptr);
  auto t = localtime(&tm);
  set(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour * 10000 +
      t->tm_min * 100 + t->tm_sec);
  auto tz = LOCAL_UTC_OFFSET_FROM_STANDARD_TIME_AS_HHMM;
  if (t->tm_isdst > 0) {
    tz += 100;
  }
  set_utc_offset(tz);
  m_weekday = t->tm_wday;
  if (set_to_utc) {
    if (tz > 0) {
      subtract_hours(tz / 100);
    } else {
      add_hours(-tz / 100);
    }
    set_utc_offset(0);
  }
}

void DateTime::set_utc_offset(short utc_offset_as_hhmm) {
  if (utc_offset_as_hhmm < -2400 || (utc_offset_as_hhmm > -100 &&
      utc_offset_as_hhmm < 100 && utc_offset_as_hhmm != 0) ||
      utc_offset_as_hhmm > 2400) {
    throw runtime_error("Error: bad offset from UTC specified: " + itos(
        utc_offset_as_hhmm) + ".");
  }
  m_utc_offset = utc_offset_as_hhmm;
}

void DateTime::set_time(size_t hhmmss) {
  m_second = hhmmss % 100;
  hhmmss /= 100;
  m_minute = hhmmss % 100;
  hhmmss /= 100;
  m_hour = hhmmss;
}

void DateTime::subtract(string units, size_t num_to_subtract, string calendar) {
  units = strutils::to_lower(units);
  if (units == "hours") {
    subtract_hours(num_to_subtract, calendar);
  } else if (units == "days") {
    subtract_days(num_to_subtract, calendar);
  } else if (units == "months") {
    subtract_months(num_to_subtract, calendar);
  } else if (units == "years") {
    subtract_years(num_to_subtract);
  } else if (units == "minutes") {
    subtract_minutes(num_to_subtract, calendar);
  } else if (units == "seconds") {
    subtract_seconds(num_to_subtract, calendar);
  } else {
    throw runtime_error("Error: units " + units + " not recognized.");
  }
}

void DateTime::subtract_days(size_t days_to_subtract, string calendar) {
  size_t days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (dateutils::is_leap_year(m_year, calendar)) {
    days[2] = 29;
  }
  int newdy = m_day - days_to_subtract;
  while (newdy <= 0) {
    --m_month;
    if (m_month < 1) {
      --m_year;
      days[2] = dateutils::is_leap_year(m_year, calendar) ? 29 : 28;
      m_month = 12;
    }
    newdy += days[m_month];
  }
  m_day = newdy;
}

void DateTime::subtract_hours(size_t hours_to_subtract, string calendar) {
  auto dd = hours_to_subtract / 24;
  auto hh = hours_to_subtract % 24;
  m_hour -= hh;
  if (m_hour < 0) {
    m_hour += 24;
    ++dd;
  }
  subtract_days(dd, calendar);
}

void DateTime::subtract_minutes(size_t minutes_to_subtract, string calendar) {
  auto hh = minutes_to_subtract / 60;
  auto mm = minutes_to_subtract % 60;
  m_minute -= mm;
  if (m_minute < 0) {
    m_minute += 60;
    ++hh;
  }
  subtract_hours(hh, calendar);
}

void DateTime::subtract_months(size_t months_to_subtract, string calendar) {
  for (size_t n = 0; n < months_to_subtract; ++n) {
    --m_month;
    if (m_month == 0) {
      --m_year;
      m_month = 12;
    }
  }
  size_t *d;
  if (dateutils::is_leap_year(m_year, calendar)) {
    d = const_cast<size_t *>(DAYS_IN_MONTH_LEAP);
  } else {
    d = const_cast<size_t *>(DAYS_IN_MONTH_NOLEAP);
  }
  if (m_day > static_cast<int>(d[m_month])) {
    m_day = d[m_month];
  }
}

void DateTime::subtract_seconds(size_t seconds_to_subtract, string calendar) {
  auto m = seconds_to_subtract / 60;
  auto s = seconds_to_subtract % 60;
  m_second -= s;
  if (m_second < 0) {
    m_second += 60;
    ++m;
  }
  subtract_minutes(m, calendar);
}

DateTime DateTime::subtracted(string units, size_t num_to_subtract, string
    calendar) const {
  DateTime d = *this;
  d.subtract(units, num_to_subtract, calendar);
  return d;
}

string DateTime::to_string() const {
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(4) << m_year << "-" << std::setw(2) <<
      m_month << "-" << std::setw(2) << m_day << " " << std::setw(2) << m_hour
      << ":" << std::setw(2) << m_minute << ":" << std::setw(2) << m_second <<
      " ";
  if (m_utc_offset < 0) {
    if (m_utc_offset > -2400) {
      ss << "-" << std::setw(4) << abs(m_utc_offset);
    } else if (m_utc_offset == -2400) {
      ss << "LST  ";
    }
  } else {
    if (m_utc_offset < 2400) {
      ss << "+" << std::setw(4) << abs(m_utc_offset);
    } else if (m_utc_offset == 2400) {
      ss << "LT   ";
    }
  }
  return ss.str();
}

string DateTime::to_string(const char *format) const {

// uses formats as in cftime, with additions:
//  %mm - month [1-12] with no leading zero for single digits
//  %dd - day [1-31] with no leading zero for single digits
//  %HH - hour (24-hour clock) [0-23] with no leading zero for single digits
//  %hh - full month name
//  %MM - minutes with leading zero for single digits
//  %SS - seconds with leading zero for single digits
//  %ZZ - time zone with ':' between hour and minute

  static const char *WEEKDAYS[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri",
      "Sat" };
  static const char *HMONTHS[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  static const char *HHMONTHS[] = { "", "January", "February", "March", "April",
      "May", "June", "July", "August", "September", "October", "November",
      "December" };
  string d = format;
  replace_all(d, "%ISO8601", "%Y-%m-%dT%H:%MM:%SS%ZZ");

  // double-letter formats MUST come before single-letter formats or else the
  //  replacement will not work correctly (i.e. - %HH before %H)
  replace_all(d, "%dd", itos(m_day));
  replace_all(d, "%d", ftos(m_day, 2, 0, '0'));
  replace_all(d, "%HH", itos(m_hour));
  replace_all(d, "%H", ftos(m_hour, 2, 0, '0'));
  replace_all(d, "%hh", HHMONTHS[m_month]);
  replace_all(d, "%h", HMONTHS[m_month]);
  replace_all(d, "%MM", ftos(m_minute, 2, 0, '0'));
  replace_all(d, "%M", itos(m_minute));
  replace_all(d, "%mm", itos(m_month));
  replace_all(d, "%m", ftos(m_month, 2, 0, '0'));
  replace_all(d, "%R", ftos(m_hour, 2, 0, '0') + ":" + ftos(m_minute, 2,
      0, '0'));
  replace_all(d, "%SS", ftos(m_second, 2, 0, '0'));
  replace_all(d, "%S", itos(m_second));
  replace_all(d, "%T", ftos(m_hour, 2, 0, '0') + ":" + ftos(m_minute, 2,
      0, '0') + ":" + ftos(m_second, 2, 0, '0'));
  replace_all(d, "%Y", ftos(m_year, 4, 0, '0'));
  if (std::regex_search(d, std::regex("%ZZ"))) {
    if (m_utc_offset > -2400 && m_utc_offset < 2400) {
      auto tz = ftos(abs(m_utc_offset), 4, 0, '0');
      tz.insert(2, ":");
      if (m_utc_offset < 0) {
        tz.insert(0, "-");
      } else {
        tz.insert(0, "+");
      }
      replace_all(d, "%ZZ", tz);
    } else {
      replace_all(d, "%ZZ", "%Z");
    }
  }
  if (std::regex_search(d, std::regex("%Z"))) {
    if (m_utc_offset > -2400 && m_utc_offset < 2400) {
      auto tz = ftos(abs(m_utc_offset), 4, 0, '0');
      if (m_utc_offset < 0) {
        tz.insert(0, "-");
      } else {
        tz.insert(0, "+");
      }
      replace_all(d, "%Z", tz);
    } else if (m_utc_offset == -2400) {
      replace_all(d, "%Z", "LST");
    } else if (m_utc_offset == 2400) {
      replace_all(d, "%Z", "LT");
    }
  }
  if (m_weekday >= 0) {
    replace_all(d, "%a", WEEKDAYS[m_weekday]);
  }
  return d;
}

DateTime DateTime::time_added(size_t hhmmss, string calendar) const {
  DateTime d = *this;
  d.add_time(hhmmss, calendar);
  return d;
}

DateTime DateTime::years_added(size_t years_to_add) const {
  DateTime d = *this;
  d.add_years(years_to_add);
  return d;
}

DateTime DateTime::years_subtracted(size_t years_to_subtract) const {
  DateTime d = *this;
  d.subtract_years(years_to_subtract);
  return d;
}

bool operator!=(const DateTime& source1, const DateTime& source2) {
  if (source1.m_year != source2.m_year) {
    return true;
  }
  if (source1.m_month != source2.m_month) {
    return true;
  }
  if (source1.m_day != source2.m_day) {
    return true;
  }
  if (source1.m_hour != source2.m_hour) {
    return true;
  }
  if (source1.m_minute != source2.m_minute) {
    return true;
  }
  if (source1.m_second != source2.m_second) {
    return true;
  }
  return false;
}

bool operator<(const DateTime& source1, const DateTime& source2) {
  if (source1.m_year < source2.m_year) {
    return true;
  } else if (source1.m_year == source2.m_year) {
    if (source1.m_month < source2.m_month) {
      return true;
    } else if (source1.m_month == source2.m_month) {
      if (source1.m_day < source2.m_day) {
        return true;
      } else if (source1.m_day == source2.m_day) {
        if (source1.m_hour < source2.m_hour) {
          return true;
        } else if (source1.m_hour == source2.m_hour) {
          if (source1.m_minute < source2.m_minute) {
            return true;
          } else if (source1.m_minute == source2.m_minute) {
            if (source1.m_second < source2.m_second) {
              return true;
            }
          }
        }
      }
    }
  }
  return false;
}

bool operator<=(const DateTime& source1, const DateTime& source2) {
  if (source1 < source2 || source1 == source2) {
    return true;
  }
  return false;
}

bool operator>(const DateTime& source1, const DateTime& source2) {
  if (source1.m_year > source2.m_year) {
    return true;
  } else if (source1.m_year == source2.m_year) {
    if (source1.m_month > source2.m_month) {
      return true;
    } else if (source1.m_month == source2.m_month) {
      if (source1.m_day > source2.m_day) {
        return true;
      } else if (source1.m_day == source2.m_day) {
        if (source1.m_hour > source2.m_hour) {
          return true;
        } else if (source1.m_hour == source2.m_hour) {
          if (source1.m_minute > source2.m_minute) {
            return true;
          } else if (source1.m_minute == source2.m_minute) {
            if (source1.m_second > source2.m_second) {
              return true;
            }
          }
        }
      }
    }
  }
  return false;
}

bool operator>=(const DateTime& source1, const DateTime& source2)
{
  if (source1 == source2 || source1 > source2) {
    return true;
  }
  return false;
}
