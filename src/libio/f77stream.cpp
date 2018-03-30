// FILE: f77stream.cpp

#include <bfstream.hpp>
#include <bits.hpp>
#include <utils.hpp>

if77stream& if77stream::operator=(const if77stream& source)
{
  return *this;
}

void if77stream::close()
{
  if (!is_open()) {
    return;
  }
  fs.close();
  file_name="";
}

int if77stream::ignore()
{
  unsigned char buffer[4];
  fs.read(reinterpret_cast<char *>(buffer),4);
  if (fs.gcount() != 4) {
    return eof;
  }
  size_t rec_len;
  bits::get(buffer,rec_len,0,32);
  fs.seekg(rec_len,std::ios_base::cur);
  if (!fs) {
    rec_len=error;
  }
  fs.read(reinterpret_cast<char *>(buffer),4);
  if (fs.gcount() != 4) {
    rec_len=error;
  }
  else {
    size_t check_len;
    bits::get(buffer,check_len,0,32);
    if (check_len != rec_len) {
	rec_len=error;
    }
  }
  ++num_read;
  ++num_blocks;
  return rec_len;
}

int if77stream::peek()
{
  auto loc=fs.tellg();
  size_t nr=num_read,nb=num_blocks;
  auto rec_len=ignore();
  fs.seekg(loc,std::ios_base::beg);
  num_read=nr;
  num_blocks=nb;
  return rec_len;
}

int if77stream::read(unsigned char *buffer,size_t num_bytes)
{
  unsigned char fbuffer[4];
  fs.read(reinterpret_cast<char *>(fbuffer),4);
  if (fs.gcount() != 4) {
    return eof;
  }
  size_t rec_len;
  bits::get(fbuffer,rec_len,0,32);
  if (rec_len == 0) {
    return error;
  }
  size_t num_copied;
  if (rec_len > num_bytes) {
    num_copied=num_bytes;
    if (num_copied > 0) {
	fs.read(reinterpret_cast<char *>(buffer),num_copied);
	if (static_cast<size_t>(fs.gcount()) != num_copied) {
	  num_copied=error;
	}
    }
    fs.seekg(rec_len-num_bytes,std::ios_base::cur);
    if (!fs) {
	num_copied=error;
    }
  }
  else {
    num_copied=rec_len;
    fs.read(reinterpret_cast<char *>(buffer),num_copied);
    if (static_cast<size_t>(fs.gcount()) != num_copied) {
	num_copied=error;
    }
  }
  fs.read(reinterpret_cast<char *>(fbuffer),4);
  if (fs.gcount() != 4) {
    num_copied=error;
  }
  else {
    size_t check_len;
    bits::get(fbuffer,check_len,0,32);
    if (check_len != rec_len) {
	num_copied=error;
    }
  }
  ++num_read;
  ++num_blocks;
  return num_copied;
}

of77stream& of77stream::operator=(const of77stream& source)
{
  return *this;
}

void of77stream::close()
{
  if (!is_open()) {
    return;
  }
  fs.close();
  file_name="";
}

int of77stream::write(const unsigned char *buffer,size_t num_bytes)
{
  static bool *is_little_endian=nullptr;
  char buf[4];

  if (is_little_endian == nullptr) {
    is_little_endian=new bool;
    *is_little_endian=!unixutils::system_is_big_endian();
  }
  size_t num_out=fs.tellp();
  if (*is_little_endian) {
    fs.write(reinterpret_cast<char *>(&num_bytes),4);
  }
  else {
    bits::set(buf,num_bytes,0,32);
    fs.write(buf,4);
  }
  num_out=static_cast<size_t>(fs.tellp())-num_out;
  if (num_out != 4) {
    return error;
  }
  num_out=fs.tellp();
  fs.write(reinterpret_cast<char *>(const_cast<unsigned char *>(buffer)),num_bytes);
  num_out=static_cast<size_t>(fs.tellp())-num_out;
  if (num_out != num_bytes) {
    return error;
  }
  num_out=fs.tellp();
  if (*is_little_endian) {
    bits::set(buf,num_bytes,0,32);
    fs.write(buf,4);
  }
  else {
    fs.write(reinterpret_cast<char *>(&num_bytes),4);
  }
  num_out=static_cast<size_t>(fs.tellp())-num_out;
  if (num_out != 4) {
    return error;
  }
  return num_bytes;
}
