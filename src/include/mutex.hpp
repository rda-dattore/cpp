// FILE: mutex.hpp

#ifndef MUTEX_H
#define   MUTEX_H

class Mutex {
public:
  Mutex() : lock_flag(0) { }
  bool is_locked() const { return lock_flag == 1; }
  void lock();
  void unlock();

private:
  volatile int lock_flag;
};

#endif
