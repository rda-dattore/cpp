// FILE: mutex.h

#ifndef MUTEX_H
#define   MUTEX_H

class Mutex {
public:
  Mutex() : lock_flag(0) {}
  void lock();
  void unlock();

private:
  volatile int lock_flag;
};

#endif
