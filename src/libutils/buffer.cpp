#include <iostream>
#include <buffer.hpp>

Buffer& Buffer::operator=(const Buffer& source) {
  if (this == &source) {
    return *this;
  }
  capacity=source.capacity;
  buffer.reset(new unsigned char[capacity]);
  std::copy(source.buffer.get(),source.buffer.get()+capacity,buffer.get());
  return *this;
}

unsigned char& Buffer::operator[](size_t index)
{
  if (index < capacity) {
    return buffer.get()[index];
  }
  else {
    return empty;
  }
}

void Buffer::allocate(size_t length)
{
  if (length > capacity) {
    capacity=length;
    buffer.reset(new unsigned char[capacity]);
  }
}
