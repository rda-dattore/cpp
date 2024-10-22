#include <grid.hpp>
#include <strutils.hpp>
#include <myexception.hpp>

using std::make_tuple;
using std::move;
using std::string;
using std::tie;
using std::to_string;
using std::tuple;
using std::vector;

#ifndef __cosutils

namespace grib_utils {

static const char *GRIB1_TIME_UNIT[] = { "minute", "hour", "day", "month",
    "year", "year", "year", "year", "", "", "hour", "hour", "hour" };
/* not used anywhere?
static const int GRIB1_PER_DAY[] = { 1440, 24, 1, 0, 0, 0, 0, 0, 0, 0, 24, 24,
    24 };
*/
static const int GRIB1_UNIT_MULT[] = { 1, 1, 1, 1, 1, 10, 30, 100, 1, 1, 3, 6,
    12 };
static const char *GRIB2_TIME_UNIT[] = { "minute", "hour", "day", "month",
    "year", "year", "year", "year", "", "", "hour", "hour", "hour", "second" };
/* not used anywhere?
static const int GRIB2_PER_DAY[] = { 1440, 24, 1, 0, 0, 0, 0, 0, 0, 0, 24, 24,
    24, 86400 };
*/
static const int GRIB2_UNIT_MULT[] = { 1, 1, 1, 1, 1, 10, 30, 100, 1, 1, 3, 6,
    12, 1 };

tuple<string, DateTime> interval_24_hour_forecast_averages(string f, const
    DateTime& v1, short t, short p1, short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, l>
//        p is a string containing the product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  char **u = nullptr;
  if (f == "grib") {
    u = const_cast<char **>(GRIB1_TIME_UNIT);
  } else if (f == "grib2") {
    u = const_cast<char **>(GRIB2_TIME_UNIT);
  }
  if (t == 11) {
    p1 *= 6;
    p2 *= 6;
    t = 1;
  }
  if (t >= 1 && t <= 2) {
    if (n == dateutils::days_in_month(v1.year(), v1.month())) {
      p = "Monthly Mean (1 per day) of ";
    } else {
      p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2 - p1) +
          "-" + u[t] + " intervals of ";
    }
  } else {
    p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2 - p1) + "-"
        + u[t] + " intervals of ";
  }
  p += to_string(p2 - p1) + "-" + u[t] + " Average (initial+" + to_string(p1) +
      " to initial+" + to_string(p2) + ")";
  switch (t) {
    case 1: {
      v2 = v1.hours_added((n - 1) * 24 + (p2 - p1));
      break;
    }
    case 2: {
      v2 = v1.days_added((n - 1) + (p2 - p1));
      break;
    }
    default: {
      throw my::BadOperation_Error("unable to compute last valid date/time "
          "from time units " + to_string(t));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> successive_forecast_averages(string f, const DateTime&
    v1, short t, short p1, short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  char **u = nullptr;
  if (f == "grib") {
    u = const_cast<char **>(GRIB1_TIME_UNIT);
  } else if (f == "grib2") {
    u = const_cast<char **>(GRIB2_TIME_UNIT);
  }
  if (t >= 1 && t <= 2) {
    size_t x = (t == 2) ? 1 : 24 / p2;
    if ( (dateutils::days_in_month(v1.year(), v1.month()) - n / x) < 3) {
      p = "Monthly Mean (" + to_string(x) + " per day) of Forecasts of ";
    } else {
      p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2 - p1) +
          "-" + u[t] + " intervals of ";
    }
  } else {
    p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2 - p1) + "-"
        + u[t] + " intervals of ";
  }
  p += to_string(p2 - p1) + "-" + u[t] + " Average (initial+" + to_string(p1) +
      " to initial+" + to_string(p2) + ")";
  switch (t) {
    case 1: {
      v2 = v1.hours_added(n * (p2 - p1));
      break;
    }
    case 2: {
      v2 = v1.days_added(n * (p2 - p1));
      break;
    }
    case 11: {
      v2 = v1.hours_added(n * (p2 - p1) * 6);
      break;
    }
    default: {
      throw my::BadOperation_Error("unable to compute last valid date/time "
          "from time units " + to_string(t) +
          " (successive_forecast_averages)");
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_2(string f, const DateTime& v1, short t,
    short p1, short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2 = v1; // return value
  char **u = nullptr;
  if (f == "grib") {
    u = const_cast<char **>(GRIB1_TIME_UNIT);
  } else if (f == "grib2") {
    u = const_cast<char **>(GRIB2_TIME_UNIT);
  }
  if (n > 0) {
    size_t x = dateutils::days_in_month(v1.year(), v1.month());
    if ( (n % x) == 0) {
      p = "Monthly Mean (" + to_string(n / x) + " per day) of ";
      v2.add_hours((n - 1) * 24 * x / n);
    } else {
      throw my::BadOperation_Error("mean with time range indicator 2 does not "
          "make sense");
    }
  }
  if (p1 == 0) {
    if (p2 == 0) {
      p += "Analysis";
    } else {
      p += to_string(p2) + "-" + u[t] + " Product";
      if (p2 > 0) {
        p += " (initial+0 to initial+" + to_string(p2) + ")";
      }
    }
  } else {
    p += to_string(p2 - p1) + "-" + u[t] + " Product";
    if (p1 > 0 || p2 > 0) {
      p += " (initial+" + to_string(p1) + " to initial+" + to_string(p2) + ")";
    }
  }
  switch (t) {
    case 1: {
      v2.add_hours(p2 - p1);
      break;
    }
    case 2: {
      v2.add_hours((p2 - p1) * 24);
      break;
    }
    default: {
      throw my::BadOperation_Error("unable to compute last valid date/time "
          "from time units " + to_string(t));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_3(const DateTime& v1, short t, short p1,
    short p2) {
// inputs:
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  if (p1 == 0 && t == 2 && static_cast<size_t>(p2) == dateutils::days_in_month(
      v1.year(), v1.month())) {
    p = "Monthly Average";
  } else {
    p = to_string(p2 - p1) + "-" + GRIB1_TIME_UNIT[t] + " Average (initial+" +
        to_string(p1) + " to initial+" + to_string(p2) + ")";
  }
  switch (t) {
    case 1: {
      v2 = v1.hours_added(p2 - p1);
      break;
    }
    case 2: {
      v2 = v1.days_added(p2 - p1);
      break;
    }
    case 3: {
      v2 = v1.months_added(p2 - p1);
      break;
    }
    default: {
      throw my::BadOperation_Error("unable to compute valid date/time from "
          "time units " + to_string(t));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime, DateTime> time_range_4_5(const DateTime& r, short n,
    short t, short p1, short p2) {
// inputs:
//    r is the reference date/time
//    n is the GRIB time range indicator code
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
// return value:
//    tuple<p, f, v>
//        p is a string containing the GRIB product description
//        f is the computed forecast date/time
//        v is the computed valid date/time

  string p; // return value;
  DateTime f, v; // return values
  p = to_string(std::abs(p2 - p1)) + "-" + GRIB1_TIME_UNIT[t] + " ";
  if (n == 4) {
    p += "Accumulation";
  } else {
    p += "Difference";
  }
  p += " (initial";
  if (p1 < 0) {
    p += "-";
  } else {
    p += "+";
  }
  p += to_string(p1) + " to initial";
  if (p2 < 0) {
    p += "-";
  } else {
    p += "+";
  }
  p += to_string(p2) + ")";
  f = v = r;
  switch (t) {
    case 1: {
      if (p2 < 0) {
        v.subtract_hours(-p2);
      } else {
        v.add_hours(p2);
      }
      if (p1 < 0) {
        f.subtract_hours(-p1);
      }
      break;
    }
    case 2: {
      if (p2 < 0) {
        v.subtract_days(-p2);
      } else {
        v.add_days(p2);
      }
      if (p1 < 0) {
        f.subtract_days(-p1);
      }
      break;
    }
    case 3: {
      if (p2 < 0) {
        v.subtract_months(-p2);
      } else {
        v.add_months(p2);
      }
      if (p1 < 0) {
        f.subtract_months(-p1);
      }
      break;
    }
    default: {
      throw my::BadOperation_Error("unable to compute valid date/time from "
          "time units " + to_string(t));
    }
  }
  return move(make_tuple(p, f, v));
}

tuple<string, DateTime, DateTime> time_range_7(const DateTime& r, short c, short
    t, short p1, short p2) {
// inputs:
//    r is the reference date/time
//    c is the center identification code
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
// return value:
//    tuple<p, f, v>
//        p is a string containing the GRIB product description
//        f is the computed forecast date/time
//        v is the computed valid date/time

  string p; // return value;
  DateTime f, v; // return values
  if (c == 7) {
    if (p2 == 0) {
      f = v = r;
      switch (t) {
        case 2: {
          f.subtract_days(p1);
          size_t n = p1 + 1;
          if (dateutils::days_in_month(f.year(), f.month()) == n) {
            p = "Monthly Mean";
          } else if (n == 5) {
            p = "Pentad Mean";
          } else if (n == 7) {
            p = "Weekly Mean";
          } else {
            p = to_string(n) + "-" + GRIB1_TIME_UNIT[t] + " Average";
          }
          break;
        }
        default: {
          throw my::BadSpecification_Error("time unit " + to_string(t) + " not "
              "recognized for time range indicator 7 and P2 == 0");
        }
      }
    } else {
      throw my::BadSpecification_Error("time unit " + to_string(t) + " not "
          "recognized for time range indicator 7 and P2 != 0");
    }
  } else {
    throw my::BadSpecification_Error("time range 7 not defined for center " +
        to_string(c));
  }
  return move(make_tuple(p, f, v));
}

tuple<string, DateTime> time_range_51(const DateTime& r, short t, short p1,
    short p2, size_t n) {
// inputs:
//    r is the reference date/time
//    t is the GRIB time unit code
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v>
//        p is a string containing the GRIB product description
//        v is the computed valid date/time

  string p; // return value
  DateTime v; // return value
  switch (t) {
    case 1: { // hour
      if (p2 == 24) {
        p = "Daily Means";
        v = r.years_added(n -1).days_added(1);
      }
      break;
    }
    case 3: { // month
      p = "Monthly Means";
      v = r.years_added(n - 1).months_added(1);
      break;
    }
  }
  if (p.empty()) {
    throw my::BadSpecification_Error("unexpected time range: P1 - " + to_string(
        p1) + ", P2 - " + to_string(p2) + ", Unit - " + to_string(t) + ", N: " +
        to_string(n));
  }
  p = to_string(n) + "-year Climatology of " + p;
  return move(make_tuple(p, v));
}

tuple<string, DateTime> time_range_113(string f, const DateTime& v1, short t,
    short p1, short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  char **u = nullptr;
  if (f == "grib") {
    u = const_cast<char **>(GRIB1_TIME_UNIT);
  } else if (f == "grib2") {
    u = const_cast<char **>(GRIB2_TIME_UNIT);
  } else {
    throw my::BadOperation_Error("time_range_113(): no time units defined for "
        "GRIB format " + f);
  }
  if (t >= 1 && t <= 2) {
    size_t x = (t == 2) ? 1 : 24 / p2;
    if ( (dateutils::days_in_month(v1.year(), v1.month()) - n / x) < 3) {
      if (p1 == 0) {
        p = "Monthly Mean (" + to_string(x) + " per day) of Analyses";
      } else {
        p = "Monthly Mean (" + to_string(x) + " per day) of " + to_string(p1) +
            "-" + u[t] + " Forecasts";
      }
    } else {
      p = "Mean of " + to_string(n) + " " + to_string(p1) + "-" + u[t] +
          " Forecasts at " + to_string(p2) + "-" + u[t] + " intervals";
    }
  } else {
    p = "Mean of " + to_string(n) + " " + to_string(p1) + "-" + u[t] +
        " Forecasts at " + to_string(p2) + "-" + u[t] + " intervals";
  }
  switch (t) {
    case 1: {
      v2 = v1.hours_added((n - 1) * p2);
      break;
    }
    case 2: {
      v2 = v1.hours_added((n - 1) * p2 * 24);
      break;
    }
    default: {
      throw my::BadOperation_Error("unable to compute last valid date/time "
          "from time units " + to_string(t));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_118(const DateTime& v1, short t, short p1,
    short p2, size_t n) {
// inputs:
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the latest valid date/time

  string p; // return value
  DateTime v2; // return value
  if (t >= 1 && t <= 2) {
    size_t x = (t == 2) ? 1 : 24 / p2;
    if ( (dateutils::days_in_month(v1.year(), v1.month()) - n / x) < 3) {
      if (p1 == 0) {
        p = "Monthly Variance/Covariance (" + to_string(x) + " per day) of "
            "Analyses";
      } else {
        p = "Monthly Variance/Covariance (" + to_string(x) + " per day) of " +
            to_string(p1) + "-" + GRIB1_TIME_UNIT[t] + " Forecasts";
      }
    } else {
      p = "Variance/Covariance of " + to_string(n) + " " + to_string(p1) + "-" +
          GRIB1_TIME_UNIT[t] + " Forecasts at " + to_string(p2) + "-" +
          GRIB1_TIME_UNIT[t] + " intervals";
    }
  } else {
    p = "Variance/Covariance of " + to_string(n) + " " + to_string(p1) + "-" +
        GRIB1_TIME_UNIT[t] + " Forecasts at " + to_string(p2) + "-" +
        GRIB1_TIME_UNIT[t] + " intervals";
  }
  switch (t) {
    case 1: {
      v2 = v1.hours_added((n - 1) * p2);
      break;
    }
    case 2: {
      v2 = v1.hours_added((n - 1) * p2 * 24);
      break;
    }
    default: {
      throw my::BadOperation_Error("unable to compute last valid date/time "
          "from time units " + to_string(t) + " (time_range_118)");
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_120(string f, const DateTime& v1, short t,
    short p1, short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  char **u = nullptr;
  if (f == "grib") {
    u = const_cast<char **>(GRIB1_TIME_UNIT);
  } else if (f == "grib2") {
    u = const_cast<char **>(GRIB2_TIME_UNIT);
  }
  if (t == 1) {
    size_t x = 24 / (p2 - p1);
    if ( (dateutils::days_in_month(v1.year(),
        v1.month()) - n / x) < 3) {
      p = "Monthly Mean (" + to_string(x) + " per day) of Forecasts of ";
    } else {
      p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2 - p1) +
          "-" + u[t] + " intervals of ";
    }
  } else {
    p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2 - p1) + "-"
        + u[t] + " intervals of ";
  }
  p += to_string(p2 - p1) + "-" + u[t] + " Accumulation (initial+" + to_string(
      p1) + " to initial+" + to_string(p2) + ")";
  switch (t) {
    case 1: {
      v2 = v1.hours_added(n * (p2 - p1));
      break;
    }
    default: {
      throw my::BadOperation_Error("unable to compute last valid date/time "
          "from time units " + to_string(t) + " (time_range_120)");
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_123(string f, const DateTime& v1, short t,
    short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  char **u = nullptr;
  if (f == "grib") {
    u = const_cast<char **>(GRIB1_TIME_UNIT);
  } else if (f == "grib2") {
    u = const_cast<char **>(GRIB2_TIME_UNIT);
  }
  if (t >= 1 && t <= 2) {
    size_t x = (t == 2) ? 1 : 24 / p2;
    if ( (dateutils::days_in_month(v1.year(), v1.month()) - n / x) < 3) {
      p = "Monthly Mean (" + to_string(x) + " per day) of Analyses";
    } else {
      p = "Mean of " + to_string(n) + " Analyses at " + to_string(p2) + "-" + u[
          t] + " intervals";
    }
  } else {
    p = "Mean of " + to_string(n) + " Analyses at " + to_string(p2) + "-" + u[t]
        + " intervals";
  }
  switch (t) {
    case 1: {
      v2 = v1.hours_added((n - 1) * p2);
      break;
    }
    case 2: {
      v2 = v1.hours_added((n - 1) * p2 * 24);
      break;
    }
    default: {
      throw my::BadOperation_Error("unable to compute last valid date/time "
          "from time units " + to_string(t));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_128(string f, short c, short s, const
    DateTime& v1, short t, short p1, short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    c is the center identification code
//    s is the subcenter identification code
//    v1 is the earliest valid date/time
//    t is the GRIB time unit value
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  char **u = nullptr;
  if (f == "grib") {
    u = const_cast<char **>(GRIB1_TIME_UNIT);
  } else if (f == "grib2") {
    u = const_cast<char **>(GRIB2_TIME_UNIT);
  }
  switch (c) {
    case 7:
    case 60: {
      if (t >= 1 && t <= 2) {
        if (n == dateutils::days_in_month(v1.year(), v1.month())) {
          p = "Monthly Mean (1 per day) of ";
        } else {
          p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2) + "-"
              + u[t] + " intervals of ";
        }
      } else {
        p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2) + "-" +
            u[t] + " intervals of ";
      }
      p += to_string(p2 - p1) + "-" + u[t] + " Accumulation (initial+" +
          to_string(p1) + " to initial+" + to_string(p2) + ")";
      switch (t) {
        case 1: {
          v2 = v1.hours_added((n - 1) * 24 + (p2 - p1));
          break;
        }
        case 2: {
          v2 = v1.days_added((n - 1) + (p2 - p1));
          break;
        }
        default: {
          throw my::BadOperation_Error("unable to compute last valid date/time "
              "from time units " + to_string(t));
        }
      }
      break;
    }
    case 34: {
      switch (s) {
        case 241: {
          tie(p, v2) = interval_24_hour_forecast_averages(f, v1, t, p1, p2, n);
          break;
        }
        default: {
          throw my::BadSpecification_Error("time range code 128 is not defined "
              "for center 34, subcenter " + to_string(s));
        }
      }
      break;
    }
    default: {
      throw my::BadSpecification_Error("time range code 128 is not defined for "
          "center " + to_string(c));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_129(string f, short c, short s, const
    DateTime& v1, short t, short p1, short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    c is the center identification code
//    s is the subcenter identification code
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  char **u = nullptr;
  if (f == "grib") {
    u = const_cast<char **>(GRIB1_TIME_UNIT);
  } else if (f == "grib2") {
    u = const_cast<char **>(GRIB2_TIME_UNIT);
  }
  switch (c) {
    case 7: {
      if (t >= 1 && t <= 2) {
        size_t x = (t == 2) ? 1 : 24 / p2;
        if ( (dateutils::days_in_month(v1.year(), v1.month()) - n / x) < 3) {
          p = "Monthly Mean (" + to_string(x) + " per day) of Forecasts of ";
        } else {
          p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2 - p1)
              + "-" + u[t] + " intervals of ";
        }
      } else {
        p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2 - p1) +
            "-" + u[t] + " intervals of ";
      }
      p += to_string(p2 - p1) + "-" + u[t] + " Accumulation (initial+" +
          to_string(p1) + " to initial+" + to_string(p2) + ")";
      switch (t) {
        case 1: {
          v2 = v1.hours_added(n * (p2 - p1));
          break;
        }
        case 2: {
          v2 = v1.days_added(n * (p2 - p1));
          break;
        }
        default: {
          throw my::BadOperation_Error("unable to compute last valid date/time "
              "for center 7 from time units " + to_string(t) +
              " (time_range_129)");
        }
      }
      break;
    }
    case 34: {
      switch (s) {
        case 241: {
          if (t == 1) {
            if ( (dateutils::days_in_month(v1.year(), v1.month()) - n) < 3) {
              p = "Monthly Variance/Covariance (1 per day) of Forecasts of ";
            } else {
              p = "Variance/Covariance of " + to_string(n) + " Forecasts at "
                  "24-hour intervals of ";
            }
          } else {
            p = "Variance/Covariance of " + to_string(n) + " Forecasts at "
                "24-hour intervals of ";
          }
          p += to_string(p2 - p1) + "-" + u[t] + " Average (initial+" +
              to_string(p1) + " to initial+" + to_string(p2) + ")";
          switch (t) {
            case 1: {
              v2 = v1.hours_added((n - 1) * 24 + p2);
              break;
            }
            default: {
              throw my::BadOperation_Error("unable to compute last valid "
                  "date/time for center 34, subcenter 241 from time units " +
                  to_string(t) + " (time_range_129)");
            }
          }
          break;
        }
        default: {
          throw my::BadSpecification_Error("time range code 129 is not defined "
              "for center 34, subcenter " + to_string(c));
        }
      }
      break;
    }
    default: {
      throw my::BadSpecification_Error("time range code 129 is not defined for "
          "center " + to_string(c));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_130(string f, short c, short s, const
    DateTime& v1, short t, short p1, short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    c is the center identification code
//    s is the subcenter identification code
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  switch (c) {

    // NCEP
    case 7: {
      tie(p, v2) = interval_24_hour_forecast_averages(f, v1, t, p1, p2, n);
      break;
    }
    case 34: {
      switch (s) {
        case 241: {
          tie(p, v2) = successive_forecast_averages(f, v1, t, p1, p2, n);
          break;
        }
        default: {
          throw my::BadSpecification_Error("time range code 130 is not defined "
              "for center 34, subcenter " + to_string(s));
        }
      }
      break;
    }

    // ECMWF
    case 98: {
      if (t == 1) {
        v2 = v1.hours_added(n);
        if (n == dateutils::days_in_month(v2.year(), v2.month()) * 24) {
          p = "Monthly Mean (24 per day) of Forecasts of 1-hour Accumulation";
        } else {
          throw my::BadSpecification_Error("ECMWF time range code 130 does not "
              "define a monthly mean");
        }
      } else {
        throw my::BadSpecification_Error("ECMWF time range code 130 is not "
            "defined for time units " + to_string(t));
      }
      break;
    }
    default: {
      throw my::BadSpecification_Error("time range code 130 is not defined for "
          "center " + to_string(c));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_131(string f, short c, short s, const
    DateTime& v1, short t, short p1, short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    c is the center identification code
//    s is the subcenter identification code
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  char **u = nullptr;
  if (f == "grib") {
    u = const_cast<char **>(GRIB1_TIME_UNIT);
  } else if (f == "grib2") {
    u = const_cast<char **>(GRIB2_TIME_UNIT);
  }
  switch (c) {
    case 7: {
      tie(p, v2) = successive_forecast_averages(f, v1, t, p1, p2, n);
      break;
    }
    case 34: {
      switch (s) {
        case 241: {
          if (t == 1) {
            size_t x = 24 / p2;
            if (dateutils::days_in_month(v1.year(), v1.month()) - n / x < 3) {
              p = "Monthly Variance/Covariance (" + to_string(x) + " per day) "
                  "of Forecasts of ";
            } else {
              p = "Variance/Covariance of " + to_string(n) + " Forecasts at " +
                  to_string(p2 - p1) + "-" + u[t] + " intervals of ";
            }
          } else {
            p = "Variance/Covariance of " + to_string(n) + " Forecasts at " +
                to_string(p2 - p1) + "-" + u[t] + " intervals of ";
          }
          p += to_string(p2 - p1) + "-" + u[t] + " Average (initial+" +
              to_string(p1) + " to initial+" + to_string(p2) + ")";
          switch (t) {
            case 1: {
              v2 = v1.hours_added(n * (p2 - p1));
              break;
            }
            default: {
              throw my::BadOperation_Error("unable to compute last valid "
                  "date/time for center 34, subcenter 241 from time units " +
                  to_string(t) + " (time_range_131)");
            }
          }
          break;
        }
        default: {
          throw my::BadSpecification_Error("time range code 131 is not defined "
              "for center 34, subcenter " + to_string(s));
        }
      }
      break;
    }
    default: {
      throw my::BadSpecification_Error("time range code 131 is not defined for "
          "center " + to_string(c));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_132(short c, short s, const DateTime& v1,
    short t, short p1, short p2, size_t n) {
// inputs:
//    c is the center identification code
//    s is the subcenter identification code
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  switch (c) {
    case 34: {
      switch (s) {
        case 241: {
          tie(p, v2) = time_range_118(v1, t, p1, p2, n);
          break;
        }
        default: {
          throw my::BadSpecification_Error("time range code 132 is not defined "
              "for center 34, subcenter " + to_string(s));
        }
      }
      break;
    }
    default: {
      throw my::BadSpecification_Error("time range code 132 is not defined for "
          "center " + to_string(c));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_133(short c, const DateTime& v1, short t,
    short p1, short p2, size_t n) {
// inputs:
//    c is the center identification code
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  switch (c) {

    // ECMWF
    case 98: {
      if (t == 1) {
        auto x = 24 / (p1 % 24); // number per day
        auto r = 24 / x; // temporal resolution
        v2 = v1.hours_added(n * r);
        if (n == dateutils::days_in_month(v2.year(), v2.month()) * x) {
          p = "Monthly Mean (" + to_string(x) + " per day) of " + to_string(r) +
              "-hour Forecasts";
        } else {
          throw my::BadSpecification_Error("ECMWF time range code 133 does not "
              "define a monthly mean");
        }
      } else {
        throw my::BadSpecification_Error("ECMWF time range code 133 is not "
            "defined for time units " + to_string(t));
      }
      break;
    }
    default: {
      throw my::BadSpecification_Error("time range code 133 is not defined for "
          "center " + to_string(c));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_137(string f, short c, const DateTime& v1,
    short t, short p1, short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    c is the center identification code
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  char **u = nullptr;
  if (f == "grib") {
    u = const_cast<char **>(GRIB1_TIME_UNIT);
  } else if (f == "grib2") {
    u = const_cast<char **>(GRIB2_TIME_UNIT);
  }
  switch (c) {
    case 7:
    case 60: {
      if (t >= 1 && t <= 2) {
        if (n / 4 == dateutils::days_in_month(v1.year(), v1.month())) {
          p = "Monthly Mean (4 per day) of ";
        } else if (n == dateutils::days_in_month(v1.year(), v1.month())) {
          p = "Monthly Mean (1 per day) of ";
        } else {
          p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2) + "-"
              + u[t] + " intervals of ";
        }
      } else {
        p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2) + "-" +
            u[t] + " intervals of ";
      }
      p += to_string(p2 - p1) + "-" + u[t] + " Accumulation (initial+" +
          to_string(p1) + " to initial+" + to_string(p2) + ")";
      switch (t) {
        case 1: {
          v2 = v1.hours_added((n - 1) * 6 + (p2 - p1));
          break;
        }
        default: {
          throw my::BadOperation_Error("unable to compute last valid date/time "
              "from time units " + to_string(t));
        }
      }
      break;
    }
    default: {
      throw my::BadSpecification_Error("time range code 137 is not defined for "
          "center " + to_string(c));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<string, DateTime> time_range_138(string f, short c, const DateTime& v1,
    short t, short p1, short p2, size_t n) {
// inputs:
//    f is a string containing the data format
//    c is the center identification code
//    v1 is the earliest valid date/time
//    t is the GRIB time unit code
//    p1 is the GRIB P1 value
//    p2 is the GRIB P2 value
//    n is the number in the average
// return value:
//    tuple<p, v2>
//        p is a string containing the GRIB product description
//        v2 is the computed latest valid date/time

  string p; // return value
  DateTime v2; // return value
  char **u = nullptr;
  int hours_to_last = 0;
  if (f == "grib") {
    u = const_cast<char **>(GRIB1_TIME_UNIT);
  } else if (f == "grib2") {
    u = const_cast<char **>(GRIB2_TIME_UNIT);
  }
  switch (c) {
    case 7:
    case 60: {
      if (t == 11) {
        p1 *= 6;
        p2 *= 6;
        t = 1;
      }
      if (t >= 1 && t <= 2) {
        if (n / 4 == dateutils::days_in_month(v1.year(), v1.month())) {
          p = "Monthly Mean (4 per day) of ";
          hours_to_last = (n - 1) * 6;
        } else if (n == dateutils::days_in_month(v1.year(), v1.month())) {
          p = "Monthly Mean (1 per day) of ";
          hours_to_last = (n - 1) * 24;
        } else {
          p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2 - p1)
              + "-" + u[t] + " intervals of ";
        }
      } else {
        p = "Mean of " + to_string(n) + " Forecasts at " + to_string(p2 - p1) +
            "-" + u[t] + " intervals of ";
      }
      p += to_string(p2 - p1) + "-" + u[t] + " Average (initial+" + to_string(
          p1) + " to " "initial+" + to_string(p2) + ")";
      v2 = v1;
      switch (t) {
        case 1: {
          v2.add_hours(hours_to_last + p2 - p1);
          break;
        }
        default: {
          throw my::BadSpecification_Error("unable to compute last valid "
              "date/time from time units " + to_string(t));
        }
      }
      break;
    }
    default: {
      throw my::BadSpecification_Error("time range code 138 is not defined for "
          "center " + to_string(c));
    }
  }
  return move(make_tuple(p, v2));
}

tuple<DateTime, size_t> forecast_time_data(size_t p1, short t, const DateTime&
    r) {
// inputs:
//    p1 is the GRIB P1 value
//    t is the GRIB time unit code
//    r is the reference date/time
// return value:
//    tuple<f, h>
//        f is the computed forecast date/time
//        h is the computed forecast time value in GRIB time units

  DateTime f; // return value
  size_t h; // return value
  switch (t) {
    case 0: {
      f = r.minutes_added(p1);
      h = p1 * 100;
      break;
    }
    case 1: {
      f = r.hours_added(p1);
      h = p1 * 10000;
      break;
    }
    case 2: {
      f = r.days_added(p1);
      h = p1 * 240000;
      break;
    }
    case 3: {
      f = r.months_added(p1);
      h = f.time_since(r);
      break;
    }
    case 4: {
      f = r.years_added(p1);
      h = f.time_since(r);
      break;
    }
    case 12: {
      f = r.hours_added(p1 * 12);
      h = p1 * 12 * 10000;
      break;
    }
    default: {
      throw my::BadOperation_Error("unable to convert forecast hour for time "
          "units " + to_string(t));
    }
  }
  return move(make_tuple(f, h));
}

// input:
//    grid is a pointer to a GRIBGrid
// return value:
//    tuple<p, f, v, h>
//        p is a string containing the GRIB product description
//        f is the forecast date/time
//        v is the valid date/time
//        h is the forecast time
tuple<string, DateTime, DateTime, size_t> grib_product(GRIBGrid *grid) {
  string p; // return value
  DateTime f, v; // return values
  size_t h = 0; // return value
  auto t = grid->time_unit();
  auto p1 = grid->P1() * GRIB1_UNIT_MULT[t];
  auto p2 = grid->P2() * GRIB1_UNIT_MULT[t];
  switch (grid->time_range()) {
    case 0:
    case 10: {
      if (p1 == 0) {
        p = "Analysis";
        h = 0;
        f = grid->reference_date_time();
      } else {
        p = to_string(p1) + "-" + GRIB1_TIME_UNIT[t] + " Forecast";
        tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      }
      v = f;
      break;
    }
    case 1: {
      p = to_string(p1) + "-" + GRIB1_TIME_UNIT[t] + " Forecast";
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      v = f;
      break;
    }
    case 2: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_2("grib", f, t, p1, p2, grid->number_averaged());
      break;
    }
    case 3: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_3(f, t, p1, p2);
      break;
    }
    case 4:
    case 5: {
      tie(p, f, v) = time_range_4_5(grid->reference_date_time(), grid->
          time_range(), t, p1, p2);
      break;
    }
    case 7: {
      tie(p, f, v) = time_range_7(grid->reference_date_time(), grid->source(),
          t, p1, p2);
      break;
    }
    case 51: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_51(grid->reference_date_time(), t, p1, p2, grid->
          number_averaged());
      break;
    }
    case 113: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_113("grib", f, t, p1, p2, grid->number_averaged());
      break;
    }
    case 118: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_118(f, t, p1, p2, grid->number_averaged());
      break;
    }
    case 120: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_120("grib", f, t, p1, p2, grid->number_averaged());
      break;
    }
    case 123: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_123("grib", f, t, p2, grid->number_averaged());
      break;
    }
    case 128: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_128("grib", grid->source(), grid->sub_center_id(),
          f, t, p1, p2, grid->number_averaged());
      break;
    }
    case 129: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_129("grib", grid->source(), grid->sub_center_id(),
          f, t, p1, p2, grid->number_averaged());
      break;
    }
    case 130: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_130("grib", grid->source(), grid->sub_center_id(),
          f, t, p1, p2, grid->number_averaged());
      break;
    }
    case 131: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_131("grib", grid->source(), grid->sub_center_id(),
          f, t, p1, p2, grid->number_averaged());
      break;
    }
    case 132: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_132(grid->source(), grid->sub_center_id(), f, t,
          p1, p2, grid->number_averaged());
      break;
    }
    case 133: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_133(grid->source(), f, t, p1, p2, grid->
          number_averaged());
      break;
    }
    case 137: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_137("grib", grid->source(), f, t, p1, p2, grid->
          number_averaged());
      break;
    }
    case 138: {
      tie(f, h) = forecast_time_data(p1, t, grid->reference_date_time());
      tie(p, v) = time_range_138("grib", grid->source(), f, t, p1, p2, grid->
          number_averaged());
      break;
    }
    default: {
      throw my::BadSpecification_Error("GRIB time range indicator " + to_string(
          grid->time_range()) + " not recognized");
    }
  }
  return move(make_tuple(p, f, v, h));
}

tuple<string, DateTime> local_use_grib2_product(short src, short sub_ctr,
    vector<GRIB2Grid::StatisticalProcessRange>& spranges, const DateTime&
    forecast_date_time, short time_unit) {
  switch (src) {
    case 7:
    case 60: {
      switch (spranges[0].type) {

        // average of analyses
        case 193: {
          return time_range_113("grib2", forecast_date_time, time_unit,
              spranges[0].period_time_increment.value - spranges[1].
              period_length.value, spranges[0].period_time_increment.value,
              spranges[0].period_length.value);
        }
        case 194: {
          return time_range_123("grib2", forecast_date_time, time_unit,
              spranges[0].period_time_increment.value, spranges[0].
              period_length.value);
        }

        // average of foreast accumulations at 24-hour intervals
        case 195: {
          return time_range_128("grib2", src, sub_ctr, forecast_date_time,
              time_unit, spranges[0].period_time_increment.value - spranges[1].
              period_length.value, spranges[0].period_time_increment.value,
              spranges[0].period_length.value);
        }

        // average of foreast averages at 24-hour intervals
        case 197: {
          return time_range_130("grib2", src, sub_ctr, forecast_date_time,
              time_unit, spranges[0].period_time_increment.value - spranges[1].
              period_length.value, spranges[0].period_time_increment.value,
              spranges[0].period_length.value);
        }

        // average of foreast accumulations at 6-hour intervals
        case 204: {
          return time_range_137("grib2", src, forecast_date_time, time_unit,
              spranges[0].period_time_increment.value - spranges[1].
              period_length.value, spranges[0].period_time_increment.value,
              spranges[0].period_length.value);
        }

        // average of foreast averages at 6-hour intervals
        case 205: {
          return time_range_138("grib2", src, forecast_date_time, time_unit,
              spranges[0].period_time_increment.value - spranges[1].
              period_length.value, spranges[0].period_time_increment.value,
              spranges[0].period_length.value);
        }
        case 255: {
          return time_range_2("grib2", forecast_date_time, time_unit, spranges[
              0].period_time_increment.value - spranges[1].period_length.value,
              spranges[0].period_time_increment.value, spranges[0].
              period_length.value);
        }
        default: {
          throw my::BadOperation_Error("unable to decode NCEP local-use "
              "statistical process type " + to_string(spranges[0].type));
        }
      }
      break;
    }
    default: {
      throw my::BadOperation_Error("unable to decode local-use statistical "
          "processing for center " + to_string(src));
    }
  }
}

void decode_negative_period_process(const GRIB2Grid::StatisticalProcessRange&
    sprange, string& product, const DateTime& valid_date_time, DateTime&
    forecast_date_time, const DateTime& reference_date_time, bool
    is_outermost) {
  switch (sprange.type) {
    case 1: {
      product += to_string(-sprange.period_length.value * GRIB2_UNIT_MULT[
          sprange.period_length.unit]) + "-" + GRIB2_TIME_UNIT[sprange.
          period_length.unit] + " Accumulation (initial";
      forecast_date_time = valid_date_time.subtracted(GRIB2_TIME_UNIT[sprange.
          period_length.unit] + string("s"), -sprange.period_length.value *
          GRIB2_UNIT_MULT[sprange.period_length.unit]);
      if (forecast_date_time < reference_date_time) {
        product += "-" + to_string(reference_date_time.hours_since(
            forecast_date_time));
      } else {
        product += "+" + to_string(forecast_date_time.hours_since(
            reference_date_time));
      }
      product +=" to initial";
      if (valid_date_time < reference_date_time) {
        product += "-" + to_string(reference_date_time.hours_since(
            valid_date_time));
      } else {
        product += "+" + to_string(valid_date_time.hours_since(
            reference_date_time));
      }
      product += ")";
      break;
    }
    default: {
      throw my::BadOperation_Error("no decoding for negative period and "
          "statistical process type " + to_string(sprange.type) + ", length: " +
          to_string(sprange.period_length.value) + "; date - " +
          forecast_date_time.to_string());
    }
  }
}

void decode_positive_period_process(const GRIB2Grid::StatisticalProcessRange&
    sprange, string& product, DateTime& valid_date_time, const DateTime&
    forecast_date_time, int forecast_hour, bool is_outermost) {
  string p;
  switch (sprange.type) {

    // average (0)
    // accumulation (1)
    // maximum (2)
    // minimum (3)
    // unspecified (255) - call it a period
    case 0:
    case 1:
    case 2:
    case 3:
    case 6:
    case 255: {
      switch (sprange.type) {
        case 0: {
          p = "Average";
          break;
        }
        case 1: {
          p = "Accumulation";
          break;
        }
        case 2: {
          p = "Maximum";
          break;
        }
        case 3: {
          p = "Minimum";
          break;
        }
        case 6: {
          p = "Standard Deviation";
          break;
        }
        case 255: {
          p = "Period";
          break;
        }
      }
      p = to_string(sprange.period_length.value * GRIB2_UNIT_MULT[sprange.
          period_length.unit]) + "-" + GRIB2_TIME_UNIT[sprange.period_length.
          unit] + " " + p;
      if (sprange.period_length.value > 0 && sprange.period_time_increment.value
          == 0) {

        // non-zero length continuous process
        p += " (initial+" + to_string(forecast_hour) + " to initial+" +
            to_string(forecast_hour + sprange.period_length.value *
            GRIB2_UNIT_MULT[sprange.period_length.unit]) + ")";
      }
      product += p;
      if (is_outermost) {
        valid_date_time = forecast_date_time.added(GRIB2_TIME_UNIT[sprange.
            period_length.unit] + string("s"), sprange.period_length.value *
            GRIB2_UNIT_MULT[sprange.period_length.unit]);
      }
      break;
    }
    default: {
      throw my::BadOperation_Error("no decoding for positive period and "
          "statistical process type " + to_string(sprange.type));
    }
  }
}

void decode_process(const GRIB2Grid::StatisticalProcessRange&
    sprange, string& product, DateTime& valid_date_time, DateTime&
    forecast_date_time, const DateTime& reference_date_time, int
    forecast_hour, bool is_outermost) {
  if (sprange.period_length.value < 0) {
    decode_negative_period_process(sprange, product, valid_date_time,
        forecast_date_time, reference_date_time, is_outermost);
  } else {
    decode_positive_period_process(sprange, product, valid_date_time,
        forecast_date_time, forecast_hour, is_outermost);
  }
}

// input:
//    grid is a pointer to a GRIB2Grid
// return value:
//    tuple<p, f, v>
//        p is a string containing the GRIB product description
//        f is the computed forecast date/time
//        v is the computed valid date/time
tuple<string, DateTime, DateTime> grib2_product(GRIB2Grid *grid) {
  string p; // return value
  DateTime f = grid->forecast_date_time(); // return value
  DateTime v = grid->valid_date_time(); // return value
  short u = grid->time_unit();
  int h = grid->forecast_time() / 10000;
  switch (grid->product_type()) {
    case 0:
    case 1:
    case 2: {
      if (h == 0) {
        p = "Analysis";
      } else {
        p = to_string(h) + "-" + GRIB2_TIME_UNIT[u] + " Forecast";
      }
      break;
    }
    case 8:
    case 11:
    case 12:
    case 61: {
      vector<GRIB2Grid::StatisticalProcessRange> spranges = grid->
          statistical_process_ranges();
      if (spranges.front().type >= 192 && spranges.size() == 2) {
        string s;
        tie(s, v) = local_use_grib2_product(grid->source(), grid->
            sub_center_id(), spranges, f, u);
        return move(make_tuple(s, f, v));
      }
      p = "";
      for (size_t n = 0; n < spranges.size(); ++n) {
        if (!p.empty()) {
          p += " of ";
        }
        decode_process(spranges[n], p, v, f, grid->reference_date_time(), h,
            n == 0);
      }
      break;
    }
    default: {
      throw my::BadSpecification_Error("product type " + to_string(grid->
          product_type()) + " not recognized");
    }
  }
  return move(make_tuple(p, f, v));
}

short p2_from_statistical_end_time(const GRIB2Grid& grid) {
  auto r = grid.reference_date_time();
  auto e = grid.statistical_process_end_date_time();
  switch (grid.time_unit()) {
    case 0: {
      return (e.time() / 100 % 100) - (r.time() / 100 % 100);
    }
    case 1: {
       return e.time() / 10000 - r.time() / 10000;
    }
    case 2: {
      return e.day() - r.day();
    }
    case 3: {
      return e.month() - r.month();
    }
    case 4: {
      return e.year() - r.year();
    }
    default: {
      throw my::BadOperation_Error("unable to map end time with units " +
          to_string(grid.time_unit()) + " to GRIB1");
    }
  }
}

} // end namespace grib_utils

#endif
