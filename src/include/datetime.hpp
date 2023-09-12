#ifndef DATETIME_HPP
#define   DATETIME_HPP

#include <string>

class DateTime
{
public:
  static const short LOCAL_UTC_OFFSET_FROM_STANDARD_TIME_AS_HHMM;
  static const size_t DAYS_IN_MONTH_NOLEAP[13];
  static const size_t DAYS_IN_MONTH_LEAP[13];
  static const size_t DAYS_IN_MONTH_360_DAY[13];
  DateTime() : m_year(0), m_month(0), m_day(0), m_hour(0), m_minute(0),
      m_second(0), m_utc_offset(0), m_weekday(-1) { }
  DateTime(short year, short month, short day, size_t hhmmss, short
      utc_offset_as_hhmm) : DateTime() {
    set(year, month, day, hhmmss, utc_offset_as_hhmm); }
  DateTime(long long YYYYMMDDHHMMSS) : DateTime() {
    set(YYYYMMDDHHMMSS); }
  void add(std::string units, size_t num_to_add, std::string calendar = "");
  void fadd(std::string units, double num_to_add, std::string calendar = "");
  void add_days(size_t days_to_add, std::string calendar = "");
  void fadd_days(double days_to_add, std::string calendar = "") {
      add_seconds(days_to_add * 86400., calendar); }
  void add_hours(size_t hours_to_add, std::string calendar = "");
  void fadd_hours(double hours_to_add, std::string calendar = "") {
      add_seconds(hours_to_add * 3600., calendar); }
  void add_minutes(size_t minutes_to_add, std::string calendar = "");
  void fadd_minutes(double minutes_to_add, std::string calendar = "") {
      add_seconds(minutes_to_add * 60., calendar); }
  void add_months(size_t months_to_add, std::string calendar = "");
  void add_seconds(size_t seconds_to_add, std::string calendar = "");
  void add_time(size_t hhmmss, std::string calendar = "");
  void add_years(size_t years_to_add) {
      m_year += years_to_add; }
  DateTime added(std::string units, size_t num_to_add, std::string calendar =
      "") const;
  DateTime fadded(std::string units, double num_to_add, std::string calendar =
      "") const;
  DateTime days_added(size_t days_to_add, std::string calendar = "") const;
  int days_since(const DateTime& reference, std::string calendar = "") const;
  DateTime days_subtracted(size_t days_to_subtract, std::string calendar = "")
      const;
  short day() const { return m_day; }
  DateTime hours_added(size_t hours_to_add, std::string calendar = "") const;
  int hours_since(const DateTime& reference, std::string calendar = "") const;
  DateTime hours_subtracted(size_t hours_to_subtract, std::string calendar = "")
      const;
  DateTime minutes_added(size_t minutes_to_add, std::string calendar = "")
      const;
  int minutes_since(const DateTime& reference, std::string calendar = "") const;
  DateTime minutes_subtracted(size_t minutes_to_subtract, std::string calendar =
      "") const;
  DateTime months_added(size_t months_to_add, std::string calendar = "") const;
  int months_since(const DateTime& reference) const;
  DateTime months_subtracted(size_t months_to_subtract, std::string calendar =
      "") const;
  short month() const { return m_month; }
  DateTime seconds_added(size_t seconds_to_add, std::string calendar = "")
      const;
  long long seconds_since(const DateTime& reference, std::string calendar = "")
      const;
  DateTime seconds_subtracted(size_t seconds_to_subtract, std::string calendar =
      "") const;
  void set(long long YYYYMMDDHHMMSS);
  void set(short year, short month = 0, short day = 0, size_t hhmmss = 0, short
      utc_offset_as_hhmm = 0);
  void set(size_t hours_to_add, const DateTime& reference, std::string calendar
      = "");
  void set_day(short day) { m_day=day; }
  void set_month(short month) { m_month=month; }
  void set_time(size_t hhmmss);
  void set_to_current();
  void set_utc_offset(short utc_offset_as_hhmm);
  void set_weekday(short weekday) { m_weekday=weekday; }
  void set_year(short year) { m_year=year; }
  void subtract(std::string units, size_t num_to_subtract, std::string calendar
      = "");
  void subtract_days(size_t days_to_subtract, std::string calendar = "");
  void subtract_hours(size_t hours_to_subtract, std::string calendar = "");
  void subtract_minutes(size_t minutes_to_subtract, std::string calendar = "");
  void subtract_months(size_t months_to_subtract, std::string calendar = "");
  void subtract_seconds(size_t seconds_to_subtract, std::string calendar = "");
  void subtract_years(size_t years_to_subtract) {
      m_year -= years_to_subtract; }
  DateTime subtracted(std::string units, size_t num_to_subtract, std::string
      calendar = "") const;
  int time_since(const DateTime& reference, std::string calendar = "") const;
  size_t time() const { return m_hour * 10000 + m_minute * 100 + m_second; }
  std::string to_string() const;
  std::string to_string(const char *format) const;
  DateTime time_added(size_t hhmmss, std::string calendar = "") const;
  long long unixtime() const { return seconds_since(DateTime(1970, 1, 1, 0,
      0)); }
  short utc_offset() const { return m_utc_offset; }
  short weekday() const { return m_weekday; }
  int years_since(const DateTime& reference) const;
  short year() const { return m_year; }
  DateTime years_added(size_t years_to_add) const;
  DateTime years_subtracted(size_t years_to_subtract) const;
  friend bool operator==(const DateTime& source1, const DateTime& source2) {
      return !(source1 != source2); }
  friend bool operator!=(const DateTime& source1, const DateTime& source2);
  friend bool operator<(const DateTime& source1, const DateTime& source2);
  friend bool operator<=(const DateTime& source1, const DateTime& source2);
  friend bool operator>(const DateTime& source1, const DateTime& source2);
  friend bool operator>=(const DateTime& source1, const DateTime& source2);

private:
  short m_year, m_month, m_day, m_hour, m_minute, m_second, m_utc_offset,
      m_weekday;
};

namespace dateutils {

/* deprecated
extern void decode_julian_day(int jul_day, size_t& year, size_t& month, size_t&
    day, size_t base_year = 1900);
*/

extern size_t days_in_month(size_t year, size_t month, std::string calendar =
    "");

/* deprecated
extern int julian_day(size_t year, size_t month, size_t day, size_t base_year =
    1900);
*/

extern std::string string_date_to_ll_string(std::string date);
extern std::string string_ll_to_date_string(std::string ll_date);

extern DateTime current_date_time();
extern DateTime from_unixtime(long long unixtime);

extern bool is_leap_year(size_t year, std::string calendar = "");

} // end namespace dateutils

#endif
