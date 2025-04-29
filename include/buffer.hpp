#include <iostream>
#include <stdlib.h>
#include <memory>
#include <sstream>

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

template <class Type>
class Buffer2 {
public:
  Buffer2() : capacity(0),size_(0),buffer(nullptr) {}
  Buffer2(const Buffer2& source) : Buffer2() { *this=source; }
  Buffer2(size_t size) : Buffer2() { resize(size); }
  ~Buffer2() { buffer.reset(nullptr); }
  Buffer2& operator=(const Buffer2& source);
  Type& operator[](size_t index);
  Type *get() { return buffer.get(); }
  void resize(size_t size);
  size_t size() const { return size_; }

private:
  size_t capacity,size_;
  std::unique_ptr<Type[]> buffer;
};

template <class Type>
Buffer2<Type>& Buffer2<Type>::operator=(const Buffer2<Type>& source) {
}

template <class Type>
Type& Buffer2<Type>::operator[](size_t index)
{
  if (index < size_) {
    return buffer.get()[index];
  }
  else {
    std::stringstream error;
    error << "index " << index << " is out of range [0:" << (size_-1) << "]";
    throw std::out_of_range(error.str());
  }
}

template <class Type>
void Buffer2<Type>::resize(size_t size)
{
  size_=size;
  if (size_ > capacity) {
    capacity=size_;
    buffer.reset(new Type[capacity]);
  }
}

#endif
