#ifndef DATETIME_HPP
#define   DATETIME_HPP

#include <string>

class DateTime
{
public:
  static const short local_utc_offset_from_standard_time_as_hhmm;
  static const size_t days_in_month_noleap[13];
  static const size_t days_in_month_leap[13];
  static const size_t days_in_month_360_day[13];
  DateTime() : year_(0),month_(0),day_(0),hour_(0),minute_(0),second_(0),utc_offset_(0),weekday_(-1) {}
  DateTime(short year,short month,short day,size_t hhmmss,short utc_offset_as_hhmm) : DateTime() {
    set(year,month,day,hhmmss,utc_offset_as_hhmm);
  }
  DateTime(long long YYYYMMDDHHMMSS) : DateTime() {
    set(YYYYMMDDHHMMSS);
  }
  void add(std::string units,size_t num_to_add,std::string calendar = "");
  void add_days(size_t days_to_add,std::string calendar = "");
  void add_hours(size_t hours_to_add,std::string calendar = "");
  void add_minutes(size_t minutes_to_add,std::string calendar = "");
  void add_months(size_t months_to_add,std::string calendar = "");
  void add_seconds(size_t seconds_to_add,std::string calendar = "");
  void add_time(size_t hhmmss,std::string calendar = "");
  void add_years(size_t years_to_add);
  DateTime added(std::string units,size_t num_to_add,std::string calendar = "") const;
  DateTime days_added(size_t days_to_add,std::string calendar = "") const;
  int days_since(const DateTime& reference,std::string calendar = "") const;
  DateTime days_subtracted(size_t days_to_subtract,std::string calendar = "") const;
  short day() const { return day_; }
  DateTime hours_added(size_t hours_to_add,std::string calendar = "") const;
  int hours_since(const DateTime& reference,std::string calendar = "") const;
  DateTime hours_subtracted(size_t hours_to_subtract,std::string calendar = "") const;
  DateTime minutes_added(size_t minutes_to_add,std::string calendar = "") const;
  int minutes_since(const DateTime& reference,std::string calendar = "") const;
  DateTime minutes_subtracted(size_t minutes_to_subtract,std::string calendar = "") const;
  DateTime months_added(size_t months_to_add,std::string calendar = "") const;
  int months_since(const DateTime& reference) const;
  DateTime months_subtracted(size_t months_to_subtract,std::string calendar = "") const;
  short month() const { return month_; }
  DateTime seconds_added(size_t seconds_to_add,std::string calendar = "") const;
  long long seconds_since(const DateTime& reference,std::string calendar = "") const;
  DateTime seconds_subtracted(size_t seconds_to_subtract,std::string calendar = "") const;
  void set(long long YYYYMMDDHHMMSS);
  void set(short year,short month = 0,short day = 0,size_t hhmmss = 0,short utc_offset_as_hhmm = 0);
  void set(size_t hours_to_add,const DateTime& reference,std::string calendar = "");
  void set_day(short day) { day_=day; }
  void set_month(short month) { month_=month; }
  void set_time(size_t hhmmss);
  void set_to_current();
  void set_utc_offset(short utc_offset_as_hhmm);
  void set_weekday(short weekday) { weekday_=weekday; }
  void set_year(short year) { year_=year; }
  void subtract(std::string units,size_t num_to_subtract,std::string calendar = "");
  void subtract_days(size_t days_to_subtract,std::string calendar = "");
  void subtract_hours(size_t hours_to_subtract,std::string calendar = "");
  void subtract_minutes(size_t minutes_to_subtract,std::string calendar = "");
  void subtract_months(size_t months_to_subtract,std::string calendar = "");
  void subtract_seconds(size_t seconds_to_subtract,std::string calendar = "");
  void subtract_years(size_t years_to_subtract);
  DateTime subtracted(std::string units,size_t num_to_subtract,std::string calendar = "") const;
  int time_since(const DateTime& reference,std::string calendar = "") const;
  size_t time() const { return hour_*10000+minute_*100+second_; }
  std::string to_string() const;
  std::string to_string(const char *format) const;
  DateTime time_added(size_t hhmmss,std::string calendar = "") const;
  long long unixtime() const { return seconds_since(DateTime(1970,1,1,0,0)); }
  short utc_offset() const { return utc_offset_; }
  short weekday() const { return weekday_; }
  int years_since(const DateTime& reference) const;
  short year() const { return year_; }
  DateTime years_added(size_t years_to_add) const;
  DateTime years_subtracted(size_t years_to_subtract) const;
  friend bool operator==(const DateTime& source1,const DateTime& source2)
  { return !(source1 != source2); }
  friend bool operator!=(const DateTime& source1,const DateTime& source2);
  friend bool operator<(const DateTime& source1,const DateTime& source2);
  friend bool operator<=(const DateTime& source1,const DateTime& source2);
  friend bool operator>(const DateTime& source1,const DateTime& source2);
  friend bool operator>=(const DateTime& source1,const DateTime& source2);

private:
  size_t *month_lengths(std::string calendar = "");

  short year_,month_,day_,hour_,minute_,second_,utc_offset_,weekday_;
};

namespace dateutils {

extern void decode_julian_day(int jul_day,size_t& year,size_t& month,size_t& day,size_t base_year = 1900);

extern size_t days_in_month(size_t year,size_t month,std::string calendar = "");

extern int julian_day(size_t year,size_t month,size_t day,size_t base_year = 1900);

extern std::string string_date_to_ll_string(std::string date);
extern std::string string_ll_to_date_string(std::string ll_date);

extern DateTime current_date_time();
extern DateTime from_unixtime(long long unixtime);

extern bool is_leap_year(size_t year,std::string calendar = "");

} // end namespace dateutils

#endif
