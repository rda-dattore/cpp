// FILE: bsort.h

#ifndef BSORT_H
#define   BSORT_H

#include <memory>
#include <vector>
#include <functional>

template <class Item,class Compare>
void merge(std::vector<Item>& array,int left_index,int left_length,int right_length,Compare compare)
{
  std::unique_ptr<Item[]> temp(new Item[left_length+right_length]);
  int left=0,right=0;
  size_t temp_count=0;
  auto right_index=left_index+left_length;
  auto unsorted=false;
  while (left < left_length && right < right_length) {
    if (compare(array[left_index+left],array[right_index+right])) {
	temp[temp_count++]=array[left_index+left];
	++left;
    }
    else {
	temp[temp_count++]=array[right_index+right];
	++right;
	unsorted=true;
    }
  }
  if (left < left_length) {
    while (left < left_length) {
	temp[temp_count++]=array[left_index+left];
	++left;
    }
  }
  else {
    while (right < right_length) {
	temp[temp_count++]=array[right_index+right];
	++right;
    }
  }
// copy the merged temporary array into the two sub-arrays
  if (unsorted) {
    temp_count=0;
    left=0;
    while (left < left_length) {
	array[left_index+left]=temp[temp_count++];
	++left;
    }
    right=0;
    while (right < right_length) {
	array[right_index+right]=temp[temp_count++];
	++right;
    }
  }
}

template <class Item,class Compare>
void binary_sort(std::vector<Item>& array,Compare compare,bool do_merge = true,size_t offset = 0,size_t num = 0)
{
  if (num == 0) {
    num=array.size();
  }
  else if ((offset+num) > array.size()) {
    num=array.size()-offset;
  }
  auto left_length=num/2;
  auto right_length=num-left_length;
  if (left_length > 1) {
    binary_sort(array,compare,do_merge,offset,left_length);
  }
  if (right_length > 1) {
    binary_sort(array,compare,do_merge,offset+left_length,right_length);
  }
  if (do_merge) {
    merge(array,offset,left_length,right_length,compare);
  }
}

#endif
