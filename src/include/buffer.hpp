#include <iostream>
#include <stdlib.h>
#include <memory>

#ifndef BUFFER_H
#define  BUFFER_H

class Buffer {
public:
  Buffer() : capacity(0),buffer(nullptr),empty() {}
  Buffer(const Buffer& source) : Buffer() { *this=source; }
  Buffer(size_t length) : Buffer() { allocate(length); }
  ~Buffer() { buffer.reset(); }
  Buffer& operator=(const Buffer& source);
  unsigned char& operator[](size_t index);
  void allocate(size_t length);

private:
  size_t capacity;
  std::unique_ptr<unsigned char[]> buffer;
  unsigned char empty;
};

#endif
