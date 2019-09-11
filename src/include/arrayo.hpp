#include <iostream>
#include <stdlib.h>

#ifndef ARRAYO_H
#define  ARRAYO_H

template <class Item>
class Array {
public:
  Array() : ptr_to_array(nullptr),alength(0),capacity(0),increment(1) {}
  Array(size_t size) : alength(0),increment(1) {
    capacity=size;
    ptr_to_array=new Item[capacity];
  }
  Array(const Array& source) : ptr_to_array(nullptr),alength(0),capacity(0),increment(1) { *this=source; }
  ~Array() { clear(); }
  Array<Item>& operator=(const Array<Item>& source);
//  Item *operator[](size_t index);
  Item &operator[](size_t index);
  Item operator()(size_t index) const;
  void clear();
  Item *getArrayAddress() const { return ptr_to_array; }
  size_t getCapacity() const { return capacity; }
  size_t length() const { return alength; };
  void setIncreaseIncrement(size_t incrementValue) { increment=incrementValue; }

  template <class Item1> friend std::ostream& operator<<(std::ostream& out_stream,const Array<Item1>& source);

private:
  Item *ptr_to_array;
  size_t alength,capacity,increment;
};

template <class Item>
void Array<Item>::clear()
{
  if (ptr_to_array != NULL) {
    delete[] ptr_to_array;
    ptr_to_array=NULL;
    alength=0;
    capacity=0;
  }
}

template <class Item>
Array<Item>& Array<Item>::operator=(const Array<Item>& source)
{
  if (this == &source) {
    return *this;
  }
  if (ptr_to_array != NULL) {
    delete[] ptr_to_array;
    ptr_to_array=NULL;
  }
  alength=source.alength;
  capacity=source.capacity;
  increment=source.increment;
  ptr_to_array=new Item[capacity];
  for (size_t n=0; n < alength; ++n) {
    ptr_to_array[n]=source.ptr_to_array[n];
  }
  return *this;
}

/*
template <class Item>
Item *Array<Item>::operator[](size_t index)
{
  Item *temp=NULL;
  size_t n;

  if (index > length)
    return NULL;
  else if (index == length) {
    if (index == capacity) {
	if (length > 0) {
	  temp=new Item[length];
	  for (n=0; n < length; n++)
	    temp[n]=ptr_to_array[n];
	  delete[] ptr_to_array;
	}
	capacity+=increment;
	ptr_to_array=new Item[capacity];
	for (n=0; n < length; n++)
	  ptr_to_array[n]=temp[n];
	if (temp != NULL)
	  delete[] temp;
    }
    length++;
  }
  return ptr_to_array+index;
}
*/

template <class Item>
Item &Array<Item>::operator[](size_t index)
{
  Item *temp=NULL;
  size_t n;

  if (index > alength) {
    std::cerr << "Error: array index out of bounds (reference)" << std::endl;
    exit(1);
  }
  else if (index == alength) {
    if (index == capacity) {
	if (alength > 0) {
	  temp=new Item[alength];
	  for (n=0; n < alength; n++)
	    temp[n]=ptr_to_array[n];
	  delete[] ptr_to_array;
	}
	capacity+=increment;
	ptr_to_array=new Item[capacity];
	for (n=0; n < alength; n++)
	  ptr_to_array[n]=temp[n];
	if (temp != NULL)
	  delete[] temp;
    }
    alength++;
  }
  return ptr_to_array[index];
}

template <class Item>
Item Array<Item>::operator()(size_t index) const
{
  if (index > alength) {
    std::cerr << "Error: array index out of bounds (value)" << std::endl;
    exit(1);
  }

  return ptr_to_array[index];
}

template <class Item>
std::ostream& operator<<(std::ostream& out_stream,const Array<Item>& source)
{
  for (size_t n=0; n < source.alength; n++) {
    if (n > 0)
	out_stream << ",";
    out_stream << "\"" << source.ptr_to_array[n] << "\"";
  }
  return out_stream;
}

#endif
