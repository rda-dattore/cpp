#include <iostream>
#include <iomanip>
#include <sstream>
#include <regex>
#include <time.h>
#include <datetime.hpp>
#include <strutils.hpp>

const short DateTime::local_utc_offset_from_standard_time_as_hhmm=-700;
const size_t DateTime::days_in_month_noleap[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};
const size_t DateTime::days_in_month_leap[13]={0,31,29,31,30,31,30,31,31,30,31,30,31};
const size_t DateTime::days_in_month_360_day[13]={0,30,30,30,30,30,30,30,30,30,30,30,30};

void DateTime::add(std::string units,size_t num_to_add,std::string calendar)
{
  units=strutils::to_lower(units);
  if (units == "hours") {
    add_hours(num_to_add,calendar);
  }
  else if (units == "days") {
    add_days(num_to_add,calendar);
  }
  else if (units == "months") {
    add_months(num_to_add,calendar);
  }
  else if (units == "years") {
    add_years(num_to_add);
  }
  else if (units == "minutes") {
    add_minutes(num_to_add,calendar);
  }
  else if (units == "seconds") {
    add_seconds(num_to_add,calendar);
  }
  else {
    std::cerr << "Error: units " << units << " not recognized" << std::endl;
    exit(1);
  }
}

void DateTime::add_days(size_t days_to_add,std::string calendar)
{
  size_t *days;
  if (calendar == "360_day") {
    days=const_cast<size_t *>(days_in_month_360_day);
  }
  else if (calendar == "365_day" || calendar == "noleap") {
    days=const_cast<size_t *>(days_in_month_noleap);
  }
  else {
    if (dateutils::is_leap_year(year_,calendar)) {
	days=const_cast<size_t *>(days_in_month_leap);
    }
    else {
	days=const_cast<size_t *>(days_in_month_noleap);
    }
    calendar="";
  }
  auto newdy=day_+days_to_add;
  while (newdy > days[month_]) {
    newdy-=days[month_];
    ++month_;
    if (month_ > 12) {
	++year_;
	if (calendar.empty()) {
	  if (dateutils::is_leap_year(year_,calendar)) {
	    days=const_cast<size_t *>(days_in_month_leap);
	  }
	  else {
	    days=const_cast<size_t *>(days_in_month_noleap);
	  }
	}
	month_=1;
    }
  }
  day_=newdy;
  if (weekday_ >= 0) {
    weekday_=(weekday_+days_to_add) % 7;
  }
}

void DateTime::add_hours(size_t hours_to_add,std::string calendar)
{
  size_t hh,dd;

  dd=hours_to_add/24;
  hh=hours_to_add % 24;
  hour_+=hh;
  if (hour_ > 23) {
    hour_-=24;
    dd++;
  }
  add_days(dd,calendar);
}

void DateTime::add_minutes(size_t minutes_to_add,std::string calendar)
{
  auto hh=minutes_to_add/60;
  auto mm=minutes_to_add % 60;
  minute_+=mm;
  if (minute_ > 59) {
    minute_-=60;
    ++hh;
  }
  add_hours(hh,calendar);
}

void DateTime::add_months(size_t months_to_add,std::string calendar)
{
  size_t *days=nullptr;
  auto check_for_leap=true;
  if (calendar == "360_day") {
    days=const_cast<size_t *>(days_in_month_360_day);
    check_for_leap=false;
  }
  else if (calendar == "365_day" || calendar == "noleap") {
    days=const_cast<size_t *>(days_in_month_noleap);
    check_for_leap=false;
  }
  for (size_t n=0; n < months_to_add; ++n) {
    if (check_for_leap) {
	if (dateutils::is_leap_year(year_,calendar)) {
	  days=const_cast<size_t *>(days_in_month_leap);
	}
	else {
	  days=const_cast<size_t *>(days_in_month_noleap);
	}
	calendar="";
    }
    add_days(days[month_],calendar);
  }
}

void DateTime::add_seconds(size_t seconds_to_add,std::string calendar)
{
  auto mm=seconds_to_add/60;
  auto ss=seconds_to_add % 60;
  second_+=ss;
  if (second_ > 59) {
    second_-=60;
    ++mm;
  }
  add_minutes(mm,calendar);
}

void DateTime::add_time(size_t hhmmss,std::string calendar)
{
  auto hh=hhmmss/10000;
  auto mm=hhmmss % 10000;
  auto ss=mm % 100;
  if (ss > 59) {
    std::cerr << "Error: bad value " << hhmmss << " for hhmmss" << std::endl;
    exit(1);
  }
  mm/=100;
  if (mm > 59) {
    std::cerr << "Error: bad value " << hhmmss << " for hhmmss" << std::endl;
    exit(1);
  }
  add_seconds(ss,calendar);
  add_minutes(mm,calendar);
  add_hours(hh,calendar);
}

void DateTime::add_years(size_t years_to_add)
{
  year_+=years_to_add;
}

DateTime DateTime::added(std::string units,size_t num_to_add,std::string calendar) const
{
  DateTime new_dt=*this;

  new_dt.add(units,num_to_add,calendar);
  return new_dt;
}

DateTime DateTime::days_added(size_t days_to_add,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.add_days(days_to_add,calendar);
  return new_dt;
}

DateTime DateTime::days_subtracted(size_t days_to_subtract,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.subtract_days(days_to_subtract,calendar);
  return new_dt;
}

int DateTime::days_since(const DateTime& reference,std::string calendar) const
{
  if (*this < reference) {
    return -1;
  }
  if (*this == reference) {
    return 0;
  }
  size_t days,*num_days;
  if (calendar == "360_day") {
    days=360*this->years_since(reference);
    num_days=const_cast<size_t *>(days_in_month_360_day);
  }
  else {
    days=365*this->years_since(reference);
    num_days=const_cast<size_t *>(days_in_month_noleap);
  }
  if (days > 0 && dateutils::is_leap_year(reference.year_,calendar) && reference.month_ <= 2) {
    ++days;
  }
  for (short n=reference.year_+1; n < this->year_; ++n) {
    if (dateutils::is_leap_year(n,calendar)) {
	++days;
    }
  }
  if (dateutils::is_leap_year(this->year_,calendar) && this->month_ > 2 && (reference.year_ < this->year_ || reference.month_ <= 2)) {
    ++days;
  }
  days+=(this->day_-reference.day_);
  if (this->months_since(reference) > 0) {
    if (this->month_ < reference.month_ || (this->month_ == reference.month_ && (this->day_ < reference.day_ || (this->day_ == reference.day_ && this->time() < reference.time())))) {
	for (short n=reference.month_; n <= 12; ++n) {
	  days+=num_days[n];
	}
	for (short n=1; n < this->month_; ++n) {
	  days+=num_days[n];
	}
    }
    else {
	for (short n=reference.month_; n < this->month_; ++n) {
	  days+=num_days[n];
	}
    }
  }
  else if (this->day_ < reference.day_ || (days == 0 && this->month_ != reference.month_)) {
    days+=num_days[reference.month_];
  }
  if (this->time() < reference.time()) {
    --days;
  }
  return days;
}

int DateTime::hours_since(const DateTime& reference,std::string calendar) const
{
  if (*this < reference) {
    return -1;
  }
  if (*this == reference) {
    return 0;
  }
  int hours=24*this->days_since(reference,calendar);
  hours+=(this->hour_-reference.hour_);
  if (this->hour_ < reference.hour_ || (this->hour_ == reference.hour_ && this->time() < reference.time())) {
    hours+=24;
  }
  if (this->minute_ < reference.minute_ || (this->minute_ == reference.minute_ && this->second_ < reference.second_)) {
    --hours;
  }
  return hours;
}

int DateTime::minutes_since(const DateTime& reference,std::string calendar) const
{
  if (*this < reference) {
    return -1;
  }
  int minutes=60*this->hours_since(reference,calendar);
  minutes+=(this->minute_-reference.minute_);
  if (this->minute_ < reference.minute_ || (this->minute_ == reference.minute_ && this->second_ < reference.second_)) {
    minutes+=60;
  }
  if (this->second_ < reference.second_) {
    --minutes;
  }
  return minutes;
}

int DateTime::months_since(const DateTime& reference) const
{
  if (*this < reference) {
    return -1;
  }
  int months=12*this->years_since(reference);
  months+=(this->month_-reference.month_);
  if (this->month_ < reference.month_ || (this->month_ == reference.month_ && (this->day_ < reference.day_ || (this->day_ == reference.day_ && this->time() < reference.time())))) {
    months+=12;
  }
  if (this->day_ < reference.day_ || (this->day_ == reference.day_ && this->time() < reference.time())) {
    --months;
  }
  return months;
}

long long DateTime::seconds_since(const DateTime& reference,std::string calendar) const
{
  if (*this < reference) {
    return -1;
  }
  long long seconds=60*static_cast<long long>(this->minutes_since(reference,calendar));
  seconds+=(this->second_-reference.second_);
  if (this->second_ < reference.second_) {
    seconds+=60;
  }
  return seconds;
}

int DateTime::time_since(const DateTime& reference,std::string calendar) const
{
  if (*this < reference) {
    return -1;
  }
  auto hh=this->hours_since(reference,calendar);
  auto mm=this->minutes_since(reference,calendar) % 60;
  auto ss=this->seconds_since(reference,calendar) % 60;
  int hhmmss=hh*10000+mm*100+ss;
  return hhmmss;
}

int DateTime::years_since(const DateTime& reference) const
{
  int years;

  if (*this < reference) {
    return -1;
  }
  years=this->year_-reference.year_;
  if (this->month_ < reference.month_) {
    --years;
  }
  else if (this->month_ == reference.month_) {
    if (this->day_ < reference.day_ || (this->day_ == reference.day_ && this->time() < reference.time())) {
	years--;
    }
  }
  return years;
}

DateTime DateTime::hours_added(size_t hours_to_add,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.add_hours(hours_to_add,calendar);
  return new_dt;
}

DateTime DateTime::hours_subtracted(size_t hours_to_subtract,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.subtract_hours(hours_to_subtract,calendar);
  return new_dt;
}

DateTime DateTime::minutes_added(size_t minutes_to_add,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.add_minutes(minutes_to_add,calendar);
  return new_dt;
}

DateTime DateTime::minutes_subtracted(size_t minutes_to_subtract,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.subtract_minutes(minutes_to_subtract,calendar);
  return new_dt;
}

DateTime DateTime::months_added(size_t months_to_add,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.add_months(months_to_add,calendar);
  return new_dt;
}

DateTime DateTime::months_subtracted(size_t months_to_subtract,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.subtract_months(months_to_subtract,calendar);
  return new_dt;
}

DateTime DateTime::seconds_added(size_t seconds_to_add,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.add_seconds(seconds_to_add,calendar);
  return new_dt;
}

DateTime DateTime::seconds_subtracted(size_t seconds_to_subtract,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.subtract_seconds(seconds_to_subtract,calendar);
  return new_dt;
}

void DateTime::set(long long YYYYMMDDHHMMSS)
{
  size_t time=YYYYMMDDHHMMSS % 1000000;
  YYYYMMDDHHMMSS/=1000000;
  short day;
  if (YYYYMMDDHHMMSS > 0) {
    day=YYYYMMDDHHMMSS % 100;
    YYYYMMDDHHMMSS/=100;
  }
  else {
    day=0;
  }
  short month;
  if (YYYYMMDDHHMMSS > 0) {
    month=YYYYMMDDHHMMSS % 100;
    YYYYMMDDHHMMSS/=100;
  }
  else {
    month=0;
  }
  short year;
  if (YYYYMMDDHHMMSS > 0) {
    year=YYYYMMDDHHMMSS;
  }
  else {
    year=0;
  }
  set(year,month,day,time);
}

void DateTime::set(short year,short month,short day,size_t hhmmss,short utc_offset_as_hhmm)
{
  DateTime base;
  base.year_=1970;
  base.month_=1;
  base.day_=4;
  year_=year;
  month_=month;
  day_=day;
  if (*this == base) {
    weekday_=0;
  }
  else if (*this > base) {
    weekday_=(days_since(base) % 7);
  }
  else {
    weekday_=((7-(base.days_since(*this) % 7)) % 7);
  }
  set_time(hhmmss);
  set_utc_offset(utc_offset_as_hhmm);
}

void DateTime::set(size_t hours_to_add,const DateTime& reference,std::string calendar)
{
  *this=reference;
  this->add_hours(hours_to_add,calendar);
}

void DateTime::set_to_current()
{
  auto tm=::time(nullptr);
  auto t=localtime(&tm);
  set(t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour*10000+t->tm_min*100+t->tm_sec);
  auto tz=local_utc_offset_from_standard_time_as_hhmm;
  if (t->tm_isdst > 0) {
    tz+=100;
  }
  set_utc_offset(tz);
  weekday_=t->tm_wday;
}

void DateTime::set_utc_offset(short utc_offset_as_hhmm)
{
  if (utc_offset_as_hhmm < -2400 || (utc_offset_as_hhmm > -100 && utc_offset_as_hhmm < 100 && utc_offset_as_hhmm != 0) || utc_offset_as_hhmm > 2400) {
    std::cerr << "Error: bad offset from UTC specified: " << utc_offset_as_hhmm << std::endl;
    exit(1);
  }
  utc_offset_=utc_offset_as_hhmm;
}

void DateTime::set_time(size_t hhmmss)
{
  second_=hhmmss % 100;
  hhmmss/=100;
  minute_=hhmmss % 100;
  hhmmss/=100;
  hour_=hhmmss;
}

void DateTime::subtract(std::string units,size_t num_to_subtract,std::string calendar)
{
  units=strutils::to_lower(units);
  if (units == "hours") {
    subtract_hours(num_to_subtract,calendar);
  }
  else if (units == "days") {
    subtract_days(num_to_subtract,calendar);
  }
  else if (units == "months") {
    subtract_months(num_to_subtract,calendar);
  }
  else if (units == "years") {
    subtract_years(num_to_subtract);
  }
  else if (units == "minutes") {
    subtract_minutes(num_to_subtract,calendar);
  }
  else if (units == "seconds") {
    subtract_seconds(num_to_subtract,calendar);
  }
  else {
    std::cerr << "Error: units " << units << " not recognized" << std::endl;
    exit(1);
  }
}

void DateTime::subtract_days(size_t days_to_subtract,std::string calendar)
{
  size_t days[]={0,31,28,31,30,31,30,31,31,30,31,30,31};
  if (dateutils::is_leap_year(year_,calendar)) {
    days[2]=29;
  }
  int newdy=day_-days_to_subtract;
  while (newdy <= 0) {
    --month_;
    if (month_ < 1) {
	--year_;
	days[2]= (dateutils::is_leap_year(year_,calendar)) ? 29 : 28;
	month_=12;
    }
    newdy+=days[month_];
  }
  day_=newdy;
}

void DateTime::subtract_hours(size_t hours_to_subtract,std::string calendar)
{
  auto dd=hours_to_subtract/24;
  auto hh=hours_to_subtract % 24;
  hour_-=hh;
  if (hour_ < 0) {
    hour_+=24;
    ++dd;
  }
  subtract_days(dd,calendar);
}

void DateTime::subtract_minutes(size_t minutes_to_subtract,std::string calendar)
{
  auto hh=minutes_to_subtract/60;
  auto mm=minutes_to_subtract % 60;
  minute_-=mm;
  if (minute_ < 0) {
    minute_+=60;
    ++hh;
  }
  subtract_hours(hh,calendar);
}

void DateTime::subtract_months(size_t months_to_subtract,std::string calendar)
{
  for (size_t n=0; n < months_to_subtract; ++n) {
    --month_;
    if (month_ == 0) {
	--year_;
	month_=12;
    }
  }
  size_t *days;
  if (dateutils::is_leap_year(year_,calendar)) {
    days=const_cast<size_t *>(days_in_month_leap);
  }
  else {
    days=const_cast<size_t *>(days_in_month_noleap);
  }
  if (day_ > static_cast<int>(days[month_])) {
    day_=days[month_];
  }
}

void DateTime::subtract_seconds(size_t seconds_to_subtract,std::string calendar)
{
  auto mm=seconds_to_subtract/60;
  auto ss=seconds_to_subtract % 60;
  second_-=ss;
  if (second_ < 0) {
    second_+=60;
    ++mm;
  }
  subtract_minutes(mm,calendar);
}

void DateTime::subtract_years(size_t years_to_subtract)
{
  year_-=years_to_subtract;
}

DateTime DateTime::subtracted(std::string units,size_t num_to_subtract,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.subtract(units,num_to_subtract,calendar);
  return new_dt;
}

std::string DateTime::to_string() const
{
  std::stringstream dt_str;
  dt_str << std::setfill('0') << std::setw(4) << year_ << "-" << std::setw(2) << month_ << "-" << std::setw(2) << day_ << " " << std::setw(2) << hour_ << ":" << std::setw(2) << minute_ << ":" << std::setw(2) << second_ << " ";
  if (utc_offset_ < 0) {
    if (utc_offset_ > -2400) {
	dt_str << "-" << std::setw(4) << abs(utc_offset_);
    }
    else if (utc_offset_ == -2400) {
	dt_str << "LST  ";
    }
  }
  else {
    if (utc_offset_ < 2400) {
	dt_str << "+" << std::setw(4) << abs(utc_offset_);
    }
    else if (utc_offset_ == 2400) {
	dt_str << "LT   ";
    }
  }
  return dt_str.str();
}

std::string DateTime::to_string(const char *format) const
{
// uses formats as in cftime, with additions:
//  %mm - month [1-12] with no leading zero for single digits
//  %dd - day [1-31] with no leading zero for single digits
//  %HH - hour (24-hour clock) [0-23] with no leading zero for single digits
//  %hh - full month name
//  %MM - minutes with leading zero for single digits
//  %SS - seconds with leading zero for single digits
//  %ZZ - time zone with ':' between hour and minute
  const char *weekdays[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  const char *hmonths[]={"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  const char *hhmonths[]={"","January","February","March","April","May","June","July","August","September","October","November","December"};

  std::string date_string=format;
  strutils::replace_all(date_string,"%ISO8601","%Y-%m-%dT%H:%MM:%SS%ZZ");
// double-letter formats MUST come before single-letter formats or else the
//  replacement will not work correctly (i.e. - %HH before %H)
  strutils::replace_all(date_string,"%dd",strutils::itos(day_));
  strutils::replace_all(date_string,"%d",strutils::ftos(day_,2,0,'0'));
  strutils::replace_all(date_string,"%HH",strutils::itos(hour_));
  strutils::replace_all(date_string,"%H",strutils::ftos(hour_,2,0,'0'));
  strutils::replace_all(date_string,"%hh",hhmonths[month_]);
  strutils::replace_all(date_string,"%h",hmonths[month_]);
  strutils::replace_all(date_string,"%MM",strutils::ftos(minute_,2,0,'0'));
  strutils::replace_all(date_string,"%M",strutils::itos(minute_));
  strutils::replace_all(date_string,"%mm",strutils::itos(month_));
  strutils::replace_all(date_string,"%m",strutils::ftos(month_,2,0,'0'));
  strutils::replace_all(date_string,"%R",strutils::ftos(hour_,2,0,'0')+":"+strutils::ftos(minute_,2,0,'0'));
  strutils::replace_all(date_string,"%SS",strutils::ftos(second_,2,0,'0'));
  strutils::replace_all(date_string,"%S",strutils::itos(second_));
  strutils::replace_all(date_string,"%T",strutils::ftos(hour_,2,0,'0')+":"+strutils::ftos(minute_,2,0,'0')+":"+strutils::ftos(second_,2,0,'0'));
  strutils::replace_all(date_string,"%Y",strutils::ftos(year_,4,0,'0'));
  if (std::regex_search(date_string,std::regex("%ZZ"))) {
    if (utc_offset_ > -2400 && utc_offset_ < 2400) {
	auto tz=strutils::ftos(abs(utc_offset_),4,0,'0');
	tz.insert(2,":");
	if (utc_offset_ < 0) {
	  tz.insert(0,"-");
	}
	else {
	  tz.insert(0,"+");
	}
	strutils::replace_all(date_string,"%ZZ",tz);
    }
    else {
	strutils::replace_all(date_string,"%ZZ","%Z");
    }
  }
  if (std::regex_search(date_string,std::regex("%Z"))) {
    if (utc_offset_ > -2400 && utc_offset_ < 2400) {
	auto tz=strutils::ftos(abs(utc_offset_),4,0,'0');
	if (utc_offset_ < 0) {
	  tz.insert(0,"-");
	}
	else {
	  tz.insert(0,"+");
	}
	strutils::replace_all(date_string,"%Z",tz);
    }
    else if (utc_offset_ == -2400) {
	strutils::replace_all(date_string,"%Z","LST");
    }
    else if (utc_offset_ == 2400) {
	strutils::replace_all(date_string,"%Z","LT");
    }
  }
  if (weekday_ >= 0) {
    strutils::replace_all(date_string,"%a",weekdays[weekday_]);
  }
  return date_string;
}

DateTime DateTime::time_added(size_t hhmmss,std::string calendar) const
{
  DateTime new_dt=*this;
  new_dt.add_time(hhmmss,calendar);
  return new_dt;
}

DateTime DateTime::years_added(size_t years_to_add) const
{
  DateTime new_dt=*this;
  new_dt.add_years(years_to_add);
  return new_dt;
}

DateTime DateTime::years_subtracted(size_t years_to_subtract) const
{
  DateTime new_dt=*this;
  new_dt.subtract_years(years_to_subtract);
  return new_dt;
}

bool operator!=(const DateTime& source1,const DateTime& source2)
{
  if (source1.year_ != source2.year_) {
    return true;
  }
  if (source1.month_ != source2.month_) {
    return true;
  }
  if (source1.day_ != source2.day_) {
    return true;
  }
  if (source1.hour_ != source2.hour_) {
    return true;
  }
  if (source1.minute_ != source2.minute_) {
    return true;
  }
  if (source1.second_ != source2.second_) {
    return true;
  }
  return false;
}

bool operator<(const DateTime& source1,const DateTime& source2)
{
  if (source1.year_ < source2.year_) {
    return true;
  }
  else if (source1.year_ == source2.year_) {
    if (source1.month_ < source2.month_) {
	return true;
    }
    else if (source1.month_ == source2.month_) {
	if (source1.day_ < source2.day_) {
	  return true;
	}
	else if (source1.day_ == source2.day_) {
	  if (source1.hour_ < source2.hour_) {
	    return true;
	  }
	  else if (source1.hour_ == source2.hour_) {
	    if (source1.minute_ < source2.minute_) {
		return true;
	    }
	    else if (source1.minute_ == source2.minute_) {
		if (source1.second_ < source2.second_) {
		  return true;
		}
	    }
	  }
	}
    }
  }
  return false;
}

bool operator<=(const DateTime& source1,const DateTime& source2)
{
  if (source1 < source2 || source1 == source2)
    return true;
  return false;
}

bool operator>(const DateTime& source1,const DateTime& source2)
{
  if (source1.year_ > source2.year_) {
    return true;
  }
  else if (source1.year_ == source2.year_) {
    if (source1.month_ > source2.month_) {
	return true;
    }
    else if (source1.month_ == source2.month_) {
	if (source1.day_ > source2.day_) {
	  return true;
	}
	else if (source1.day_ == source2.day_) {
	  if (source1.hour_ > source2.hour_) {
	    return true;
	  }
	  else if (source1.hour_ == source2.hour_) {
	    if (source1.minute_ > source2.minute_) {
		return true;
	    }
	    else if (source1.minute_ == source2.minute_) {
		if (source1.second_ > source2.second_) {
		  return true;
		}
	    }
	  }
	}
    }
  }
  return false;
}

bool operator>=(const DateTime& source1,const DateTime& source2)
{
  if (source1 == source2 || source1 > source2) {
    return true;
  }
  return false;
}
