// FILE: bfstream.cpp

#include <iostream>
#include <bfstream.hpp>

bfstream::~bfstream()
{
  if (file_buf != nullptr) {
    file_buf.reset(nullptr);
  }
  if (fs.is_open()) {
    fs.close();
  }
}

bool ibfstream::open(std::string filename)
{
// opening a stream while another is open is a fatal error
  if (is_open()) {
    std::cerr << "Error: an open stream already exists" << std::endl;
    exit(1);
  }
  file_name=filename;
  fs.open(file_name.c_str(),std::ios::in);
  if (!fs.is_open()) {
    return false;
  }
  num_read=num_blocks=max_block_len=0;
  return true;
}

bool obfstream::open(std::string filename)
{
// opening a stream while another is open is a fatal error
  if (is_open()) {
    std::cerr << "Error: an open stream already exists" << std::endl;
    exit(1);
  }
  file_name=filename;
  fs.open(file_name.c_str(),std::ios::out);
  if (!fs.is_open()) {
    return false;
  }
  num_written=num_blocks=max_block_len=0;
  return true;
}
