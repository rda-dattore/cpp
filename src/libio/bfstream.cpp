// FILE: bfstream.cpp

#include <iostream>
#include <bfstream.hpp>

using std::cerr;
using std::endl;
using std::ios;
using std::string;

bfstream::~bfstream() {
  if (file_buf != nullptr) {
    file_buf.reset(nullptr);
  }
  if (fs.is_open()) {
    fs.close();
  }
}

bool ibfstream::open(string filename) {
  if (is_open()) {
    // opening a stream while another is open is a fatal error
    cerr << "Error: an open stream already exists" << endl;
    exit(1);
  }
  file_name = filename;
  fs.open(file_name.c_str(), ios::in);
  if (!fs.is_open()) {
    return false;
  }
  num_read = 0;
  num_blocks = 0;
  max_block_len = 0;
  return true;
}

bool obfstream::open(string filename) {
  if (is_open()) {
    // opening a stream while another is open is a fatal error
    cerr << "Error: an open stream already exists" << endl;
    exit(1);
  }
  file_name = filename;
  fs.open(file_name.c_str(), ios::out);
  if (!fs.is_open()) {
    return false;
  }
  num_written = 0;
  num_blocks = 0;
  max_block_len = 0;
  return true;
}
