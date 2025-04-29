#include <limits>
#include <timer.hpp>

double Timer::current() const
{
  if (is_running) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC,&t);
    return elapsed_time(start_,t);
  }
  else {
    return std::numeric_limits<double>::quiet_NaN();
  }
}

void Timer::reset()
{
  if (is_running) {
    return;
  }
  elapsed_time_=0.;
}

void Timer::restart()
{
  if (is_running) {
    return;
  }
  clock_gettime(CLOCK_MONOTONIC,&start_);
  is_running=true;
}

void Timer::start()
{
  if (is_running) {
    return;
  }
  elapsed_time_=0.;
  clock_gettime(CLOCK_MONOTONIC,&start_);
  is_running=true;
}

void Timer::stop()
{
  if (!is_running) {
    return;
  }
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC,&t);
  elapsed_time_+=elapsed_time(start_,t);
  is_running=false;
}

double Timer::elapsed_time(const timespec& t1,const timespec& t2) const
{
  return (t2.tv_sec+t2.tv_nsec/1000000000.)-(t1.tv_sec+t1.tv_nsec/1000000000.);
}
