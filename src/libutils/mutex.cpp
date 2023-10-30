#include <mutex.hpp>

void Mutex::lock() {
  while (lock_flag == 1) { }
  lock_flag = 1;
}

void Mutex::unlock() {
  lock_flag = 0;
}
