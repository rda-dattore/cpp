#ifndef TIMER_HPP
#define   TIMER_HPP

#include <time.h>

class Timer
{
public:
  Timer() : start_(),elapsed_time_(0.),is_running(false) {}
  double current() const;
  double elapsed_time() const { return elapsed_time_; }
  void reset();
  void restart();
  void start();
  void stop();

private:
  double elapsed_time(const timespec& t1,const timespec& t2) const;

  struct timespec start_;
  double elapsed_time_;
  bool is_running;
};

#endif
