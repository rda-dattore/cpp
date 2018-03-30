// FILE: rptoutstream.cpp

#include <iostream>
#include <cmath>
#include <bfstream.hpp>
#include <utils.hpp>
#include <bits.hpp>

irstream& irstream::operator=(const irstream& source)
{
  return *this;
}

void irstream::close()
{
  if (!is_open()) {
    return;
  }
  if (icosstream != nullptr) {
    icosstream.reset(nullptr);
  }
  else if (fs.is_open()) {
    fs.close();
  }
}

int irstream::ignore()
{
  if (file_buf_pos == file_buf_len) {
    if (icosstream != nullptr) {
	if (icosstream->read(file_buf.get(),8000) == eof) {
	  return eof;
	}
	bits::get(file_buf.get(),_flag,0,4);
	bits::get(file_buf.get(),block_len,28+_flag*4,32);
	file_buf_len=(block_len-1)*(60+_flag*4);
    }
    else {
	if (fs.is_open()) {
	  std::cerr << "Error: no irstream has been opened" << std::endl;
	  exit(1);
	}
	fs.read(reinterpret_cast<char *>(file_buf.get()),8);
	if (fs.gcount() > 0) {
	  bits::get(file_buf.get(),_flag,0,4);
	  if (_flag != 1) {
	    return error;
	  }
	  bits::get(file_buf.get(),block_len,28+_flag*4,32);
	  file_buf_len=(block_len-1)*(60+_flag*4);
	  fs.read(reinterpret_cast<char *>(&file_buf[8]),file_buf_len/8);
	}
	else {
	  return eof;
	}
    }
    ++num_blocks;
    new_block=true;
    long long sum;
    if (checksum(file_buf.get(),block_len,60+_flag*4,sum) != 0) {
	std::cerr << "Warning: checksum error on block number " << num_blocks << std::endl;
    }
    file_buf_pos=60+_flag*4;
  }
  ++num_read;
  int rptlen;
  bits::get(file_buf.get(),rptlen,file_buf_pos,12);
  rptlen*=(60+_flag*4);
  file_buf_pos+=rptlen;
  return lroundf(static_cast<float>(rptlen)/8);
}

bool irstream::open(std::string filename)
{
  if (is_open()) {
    std::cerr << "Error: currently connected to another file stream" << std::endl;
    exit(1);
  }
// test for COS-blocked file
  icosstream.reset(new icstream);
  if (!icosstream->open(filename)) {
    icosstream.reset(nullptr);
    return false;
  }
  unsigned char test_buffer;
  if (icosstream->read(&test_buffer,1) < 0) {
    icosstream->close();
    icosstream.reset(nullptr);
    fs.open(filename.c_str(),std::ios::in);
    if (!fs.is_open()) {
	return false;
    }
  }
  else {
    icosstream->rewind();
  }
  file_name=filename;
  file_buf_len=file_buf_pos=0;
  num_read=num_blocks=0;
  word_count=0;
  new_block=false;
  return true;
}

int irstream::peek()
{
  if (file_buf_pos == file_buf_len) {
    if (icosstream != nullptr) {
	auto status=icosstream->read(file_buf.get(),8000);
	if (status == eof || status == craystream::eod) {
	  return status;
	}
	bits::get(file_buf.get(),_flag,0,4);
	bits::get(file_buf.get(),block_len,28+_flag*4,32);
	if (block_len <= 0 || block_len > 1000) {
	  return error;
	}
	file_buf_len=(block_len-1)*(60+_flag*4);
    }
    else {
	if (!fs.is_open()) {
	  std::cerr << "Error: no irstream has been opened" << std::endl;
	  exit(1);
	}
	fs.read(reinterpret_cast<char *>(file_buf.get()),8);
	if (fs.gcount() > 0) {
	  bits::get(file_buf.get(),_flag,0,4);
	  if (_flag != 1) {
	    return error;
	  }
	  bits::get(file_buf.get(),block_len,28+_flag*4,32);
	  if (block_len > 8000) {
	    return error;
	  }
	  file_buf_len=(block_len-1)*8;
	  fs.read(reinterpret_cast<char *>(&file_buf[8]),file_buf_len);
	  file_buf_len=(block_len-1)*(60+_flag*4);
	}
	else {
	  return eof;
	}
    }
    ++num_blocks;
    new_block=true;
    word_count+=block_len;
    long long sum;
    if (checksum(file_buf.get(),block_len,60+_flag*4,sum) != 0) {
	std::cerr << "Warning: checksum error on block number " << num_blocks << std::endl;
    }
    file_buf_pos=60+_flag*4;
  }
  else {
    new_block=false;
  }
  int rptlen;
  bits::get(file_buf.get(),rptlen,file_buf_pos,12);
  rptlen*=(60+_flag*4);
  return lroundf(static_cast<float>(rptlen)/8);
}

int irstream::read(unsigned char *buffer,size_t num_bytes)
{
  auto rptlen=peek();
  if (rptlen == eof || rptlen == error || rptlen == craystream::eod) {
    return rptlen;
  }
  ++num_read;
  if (rptlen <= static_cast<int>(num_bytes)) {
    bits::get(file_buf.get(),buffer,file_buf_pos,8,0,rptlen);
    int irptlen;
    bits::get(file_buf.get(),irptlen,file_buf_pos,12);
    file_buf_pos+=(irptlen*(60+_flag*4));
    return rptlen;
  }
  else {
    if (num_bytes > 0) {
	std::copy(&file_buf[file_buf_pos],&file_buf[file_buf_pos+num_bytes],buffer);
    }
    file_buf_pos+=rptlen;
    return num_bytes;
  }
}

void irstream::rewind()
{
  if (icosstream != nullptr) {
    icosstream->rewind();
  }
  else {
    fs.clear();
    fs.seekg(std::ios_base::beg);
  }
  file_buf_len=file_buf_pos=0;
  num_read=num_blocks=0;
}

void orstream::close()
{
  if (!is_open()) {
    return;
  }
  orstream::flush();
  fs.close();
  file_name="";
}

int orstream::flush()
{
  if (file_buf_pos > 8) {
    auto num_to_write=file_buf_pos+8;
    block_len=num_to_write/8;
    bits::set(file_buf.get(),1,0,4);
    bits::set(file_buf.get(),block_len,32,32);
    word_count+=block_len;
    long long sum;
    checksum(file_buf.get(),block_len,64,sum);
    bits::set(&file_buf[file_buf_pos],sum,0,64);
    size_t num_out=fs.tellp();
    fs.write(reinterpret_cast<char *>(file_buf.get()),num_to_write);
    num_out=static_cast<size_t>(fs.tellp())-num_out;
    if (num_out != num_to_write) {
	return error;
    }
    ++num_blocks;
    file_buf_len=7984;
    file_buf_pos=8;
    new_block=true;
    return 0;
  }
  else {
    return 1;
  }
}

bool orstream::open(std::string filename)
{
  if (is_open()) {
    std::cerr << "Error: currently connected to another file stream" << std::endl;
    exit(1);
  }
  fs.open(filename.c_str(),std::ios::out);
  if (!fs.is_open()) {
    return false;
  }
  file_name=filename;
  file_buf_len=7984;
  file_buf_pos=8;
  num_written=num_blocks=0;
  word_count=0;
  new_block=false;
  return true;
}

int orstream::write(const unsigned char *buffer,size_t num_bytes)
{
  if (!fs.is_open()) {
    std::cerr << "Error: no orstream has been opened" << std::endl;
    exit(1);
  }
  if (num_bytes > file_buf_len) {
    if (orstream::flush() == error) {
	return error;
    }
  }
  else {
    new_block=false;
  }
  ++num_written;
  auto num_words=(num_bytes+7)/8;
  auto num_chars=num_words*8;
  std::copy(&buffer[0],&buffer[num_chars],&file_buf[file_buf_pos]);
  bits::set(&file_buf[file_buf_pos],num_words,0,12);
  file_buf_len-=num_chars;
  file_buf_pos+=num_chars;
  return num_bytes;
}

ocrstream& ocrstream::operator=(const ocrstream& source)
{
  if (this == &source) {
    return *this;
  }
  *ocosstream=*source.ocosstream;
  return *this;
}

void ocrstream::close()
{
  if (!is_open()) {
    return;
  }
  ocrstream::flush();
  ocosstream->close();
  ocosstream.reset(nullptr);
}

int ocrstream::flush()
{
  if (file_buf_pos > 8) {
    size_t num_out=file_buf_pos+8;
    block_len=num_out/8;
    bits::set(file_buf.get(),1,0,4);
    bits::set(file_buf.get(),block_len,32,32);
    word_count+=block_len;
    long long sum;
    checksum(file_buf.get(),block_len,64,sum);
    bits::set(&file_buf[file_buf_pos],sum,0,64);
    if (ocosstream->write(file_buf.get(),num_out) <= 0) {
	return error;
    }
    ++num_blocks;
    file_buf_len=7984;
    file_buf_pos=8;
    new_block=true;
    return 0;
  }
  else {
    return 1;
  }
}

bool ocrstream::open(std::string filename)
{
  if (is_open()) {
    std::cerr << "Error: currently connected to another file stream" << std::endl;
    exit(1);
  }
  ocosstream.reset(new ocstream);
  if (!ocosstream->open(filename)) {
    ocosstream.reset(nullptr);
    return false;
  }
  file_name=filename;
  file_buf_len=7984;
  file_buf_pos=8;
  num_written=num_blocks=0;
  word_count=0;
  new_block=false;
  return true;
}

int ocrstream::write(const unsigned char *buffer,size_t num_bytes)
{
  if (ocosstream == nullptr) {
    std::cerr << "Error: no ocrstream has been opened" << std::endl;
    exit(1);
  }
  if (num_bytes > file_buf_len) {
    if (ocrstream::flush() == error) {
	return error;
    }
  }
  else {
    new_block=false;
  }
  ++num_written;
  auto num_words=(num_bytes+7)/8;
  auto num_chars=num_words*8;
  std::copy(&buffer[0],&buffer[num_chars],&file_buf[file_buf_pos]);
  bits::set(&file_buf[file_buf_pos],num_words,0,12);
  file_buf_len-=num_chars;
  file_buf_pos+=num_chars;
  return num_bytes;
}

void ocrstream::write_eof()
{
  if (ocrstream::flush() == error) {
    std::cerr << "Warning: error while flushing output rptoutstream" << std::endl;
  }
  ocosstream->write_eof();
}

void iocrstream::close()
{
  switch (stream_type) {
    case 0:
    {
      irstream::close();
      break;
    }
    case 1:
    {
      ocrstream::close();
      break;
    }
  }
}

int iocrstream::flush()
{
  switch(stream_type) {
    case 1:
    {
	return ocrstream::flush();
    }
  }
  return 2;
}

size_t iocrstream::block_length() const 
{
  switch (stream_type) {
    case 0:
    {
	return irstream::block_len;
    }
    case 1:
    {
	return ocrstream::block_len;
    }
  }
  return 0;
}

int iocrstream::ignore()
{
  switch (stream_type) {
    case 0:
    {
      return irstream::ignore();
    }
    case 1:
    {
      std::cerr << "Warning: ignore should not be called on a stream opened for writing" << std::endl;
    }
  }
  return bfstream::error;
}

bool iocrstream::is_new_block() const
{
  switch (stream_type) {
    case 0:
    {
	return irstream::new_block;
    }
    case 1:
    {
	return ocrstream::new_block;
    }
  }
  return false;
}

bool iocrstream::open(std::string filename)
{
  std::cerr << "Error: no mode given" << std::endl;
  return false;
}

bool iocrstream::open(std::string filename,std::string mode)
{
  if (mode == "r") {
    stream_type=0;
    return irstream::open(filename);
  }
  else if (mode == "w") {
    stream_type=1;
    return ocrstream::open(filename);
  }
  else {
    std::cerr << "Error: bad mode " << mode << std::endl;
    return false;
  }
}

int iocrstream::peek()
{
  switch (stream_type) {
    case 0:
    {
      return irstream::peek();
    }
    case 1:
    {
      std::cerr << "Warning: peek should not be called on a stream opened for writing" << std::endl;
      exit(1);
    }
  }
  return bfstream::error;
}

int iocrstream::read(unsigned char *buffer,size_t buffer_length)
{
  switch (stream_type) {
    case 0:
    {
      return irstream::read(buffer,buffer_length);
    }
    case 1:
    {
      std::cerr << "Warning: read should not be called on a stream opened for writing" << std::endl;
      exit(1);
    }
  }
  return bfstream::error;
}

void iocrstream::rewind()
{
  switch (stream_type) {
    case 0:
    {
      irstream::rewind();
      break;
    }
    case 1:
    {
      ocrstream::rewind();
      break;
    }
  }
}

int iocrstream::write(const unsigned char *buffer,size_t num_bytes)
{
  switch (stream_type) {
    case 0:
    {
      std::cerr << "Warning: write should not be called on a stream opened for reading" << std::endl;
      exit(1);
    }
    case 1:
    {
      return ocrstream::write(buffer,num_bytes);
    }
  }
  return bfstream::error;
}

void iocrstream::write_eof()
{
  switch (stream_type) {
    case 0:
    {
	std::cerr << "Warning: write_eof should not be called on a stream opened for reading" << std::endl;
	exit(1);
    }
    case 1:
    {
	ocrstream::write_eof();
	break;
    }
  }
}
