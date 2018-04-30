// FILE: craystream.cpp
#include <iostream>
#include <bfstream.hpp>
#include <bits.hpp>

const int craystream::eod=-5;
const size_t craystream::cray_block_size=4096;
const size_t craystream::cray_word_size=8;
const short craystream::cw_bcw=0;
const short craystream::cw_eor=0x8;
const short craystream::cw_eof=0xe;
const short craystream::cw_eod=0xf;

craystream::craystream() : cw_type(0)
{
  file_buf_len=cray_block_size;
  file_buf.reset(new unsigned char[file_buf_len]);
}

inline int icstream::read_from_disk()
{
  if (at_eod) {
    cw_type=cw_eod;
    return eod;
  }
  fs.read(reinterpret_cast<char *>(file_buf.get()),file_buf_len);
  size_t bytes_read=fs.gcount();
  if (num_blocks == 0 && bytes_read < file_buf_len) {
    cw_type=-1;
    return error;
  }
  file_buf_pos=0;
  bits::get(file_buf.get(),cw_type,0,4);
  if (cw_type == cw_eod) {
    at_eod=true;
  }
  if (num_blocks == 0) {
    size_t unused[2];
    bits::get(file_buf.get(),unused[0],4,7);
    bits::get(file_buf.get(),unused[1],12,19);
    size_t previous_block;
    bits::get(file_buf.get(),previous_block,31,24);
    if (cw_type != cw_bcw || unused[0] != 0 || unused[1] != 0 || previous_block != 0) {
	cw_type=cw_eod+1;
    }
  }
  ++num_blocks;
  return 0;
}

icstream& icstream::operator=(const icstream& source)
{
  return *this;
}

void icstream::close()
{
  if (!is_open()) {
    return;
  }
  fs.close();
  file_name="";
}

int icstream::ignore()
{
  return read(nullptr,0);
}

bool icstream::open(std::string filename)
{
  if (!ibfstream::open(filename)) {
    return false;
  }
  file_buf_pos=file_buf_len;
  return true;
}

int icstream::peek()
{
  static unsigned char *fb=new unsigned char[file_buf_len];

  auto loc=fs.tellg();
  std::copy(&file_buf[0],&file_buf[file_buf_len],fb);
  short cwt=cw_type;
  size_t fbp=file_buf_pos;
  size_t nr=num_read;
  size_t nb=num_blocks;
  auto rec_len=ignore();
  if (loc >= 0) {
    fs.clear();
    fs.seekg(loc,std::ios_base::beg);
  }
  std::copy(&fb[0],&fb[file_buf_len],file_buf.get());
  file_buf_pos=fbp;
  cw_type=cwt;
  num_read=nr;
  num_blocks=nb;
  return rec_len;
}

int icstream::read(unsigned char *buffer,size_t buffer_length)
{
  static int total_len=0,iterator=0;

  ++iterator;
// if the current position in the file buffer is at the end of the buffer, read
// a new block of data from the disk file
  if (file_buf_pos == file_buf_len) {
    read_from_disk();
    if (cw_type == cw_eof) {
	return eof;
    }
  }
  int num_copied=0;
  switch (cw_type) {
    case cw_bcw:
    case cw_eor:
    case cw_eof:
    {
// get the length of the current block of data
	size_t len;
	bits::get(&file_buf[file_buf_pos],len,55,9);
// move the buffer pointer to the next control word
	auto copy_start=file_buf_pos+cray_word_size;
	file_buf_pos+=(len+1)*cray_word_size;
	size_t ub;
	if (file_buf_pos < file_buf_len) {
// if the file buffer position is inside the file buffer, get the number of
// unused bits in the end of the current block of data
	  bits::get(&file_buf[file_buf_pos],ub,4,6);
	  ub/=8;
	}
	else {
	  ub=0;
	}
// copy the data to the user-specified buffer
	num_copied=len*cray_word_size-ub;
	total_len+=num_copied;
	if (buffer != nullptr) {
	  if (num_copied > static_cast<int>(buffer_length)) {
	    num_copied=buffer_length;
	  }
	  if (num_copied > 0) {
	    std::copy(&file_buf[copy_start],&file_buf[copy_start+num_copied],buffer);
	  }
	}
	if (file_buf_pos < file_buf_len) {
// if the file buffer position is inside the file buffer, get the control word
	  bits::get(&file_buf[file_buf_pos],cw_type,0,4);
	  if (cw_type == cw_bcw) {
	    return craystream::error;
	  }
	}
	else {
// otherwise, must be at the end of a Cray block, so next control word assumed
// to be a BCW
	  cw_type=cw_bcw;
	}
	break;
    }
  }
  switch (cw_type) {
    case cw_bcw:
    {
	if (buffer != nullptr) {
	  num_copied+=read(buffer+num_copied,buffer_length-num_copied);
	}
	else {
	  num_copied+=read(nullptr,0);
	}
	--iterator;
	if (iterator == 0) {
	  if (num_copied > total_len) {
	    num_copied=total_len;
	  }
	  total_len=0;
	}
	return num_copied;
    }
    case cw_eor:
    {
	++num_read;
	--iterator;
	if (iterator == 0) {
	  if (num_copied > total_len) {
	    num_copied=total_len;
	  }
	  total_len=0;
	}
	return num_copied;
    }
    case cw_eof:
    {
	return eof;
    }
    case cw_eod:
    {
	return craystream::eod;
    }
    default:
    {
	return error;
    }
  }
}

void icstream::rewind()
{
  fs.clear();
  fs.seekg(std::ios_base::beg);
  num_read=num_blocks=0;
  read_from_disk();
}

inline int ocstream::write_to_disk()
{
  size_t num_out=fs.tellp();
  fs.write(reinterpret_cast<char *>(file_buf.get()),file_buf_len);
  num_out=static_cast<size_t>(fs.tellp())-num_out;
  if (num_out != file_buf_len) {
    return -1;
  }
  file_buf_pos=0;
  for (size_t n=0; n < cray_word_size; ++n) {
    file_buf[n]=0;
  }
  oc.block_space=511;
  ++num_blocks;
  return num_out;
}

void ocstream::close()
{
  if (!is_open()) {
    return;
  }
  if (!oc.wrote_eof) {
// write the EOF
    write_eof();
  }
// write the EOD
  if (oc.block_space == 0) {
    write_to_disk();
    ++oc.blocks_full;
    ++oc.blocks_back;
    bits::set(&file_buf[file_buf_pos],oc.blocks_full,31,24);
    bits::set(&file_buf[file_buf_pos],0,55,9);
  }
  file_buf_pos+=cray_word_size;
  for (size_t n=file_buf_pos; n < file_buf_pos+8; ++n) {
    file_buf[n]=0;
  }
  bits::set(&file_buf[file_buf_pos],0xf,0,4);
  for (size_t n=file_buf_pos+8; n < file_buf_len; ++n) {
    file_buf[n]=0;
  }
  write_to_disk();
  fs.close();
  file_name="";
}

bool ocstream::open(std::string filename)
{
  if (!obfstream::open(filename)) {
    return false;
  }
  file_buf_pos=0;
  for (size_t n=0; n < 8; ++n) {
    file_buf[n]=0;
  }
  oc.block_space=511;
  oc.blocks_full=oc.blocks_back=0;
  oc.wrote_eof=false;
  return true;
}

int ocstream::write(const unsigned char *buffer,size_t num_bytes)
{
  size_t num_words=(num_bytes+7)/8;
  if (num_words > oc.block_space) {
    bits::set(&file_buf[file_buf_pos],oc.block_space,55,9);
    file_buf_pos+=cray_word_size;
    auto new_front=oc.block_space*cray_word_size;
    std::copy(buffer,buffer+new_front,&file_buf[file_buf_pos]);
    if (write_to_disk() == error) {
	return error;
    }
    ++oc.blocks_full;
    ++oc.blocks_back;
    bits::set(&file_buf[file_buf_pos],oc.blocks_full,31,24);
    write(&buffer[new_front],num_bytes-new_front);
  }
  else {
    bits::set(&file_buf[file_buf_pos],num_words,55,9);
    file_buf_pos+=cray_word_size;
    std::copy(buffer,buffer+num_bytes,&file_buf[file_buf_pos]);
    oc.block_space-=num_words;
// filled the block to the end
    if (oc.block_space == 0) {
	if (write_to_disk() == error) {
	  return error;
	}
	++oc.blocks_full;
	++oc.blocks_back;
	bits::set(&file_buf[file_buf_pos],oc.blocks_full,31,24);
	file_buf_pos+=cray_word_size;
    }
    else {
      file_buf_pos+=num_words*cray_word_size;
    }
    for (size_t n=file_buf_pos; n < file_buf_pos+cray_word_size; ++n) {
	file_buf[n]=0;
    }
    bits::set(&file_buf[file_buf_pos],0x8,0,4);
    size_t unused_bits=(num_words*8-num_bytes)*8;
    bits::set(&file_buf[file_buf_pos],unused_bits,4,6);
    bits::set(&file_buf[file_buf_pos],oc.blocks_full,20,20);
    bits::set(&file_buf[file_buf_pos],oc.blocks_back,40,15);
    oc.blocks_back=0;
    --oc.block_space;
    ++num_written;
  }
  return num_bytes;
}

void ocstream::write_eof()
{
  if (oc.block_space == 0) {
    write_to_disk();
    ++oc.blocks_full;
    ++oc.blocks_back;
    bits::set(&file_buf[file_buf_pos],oc.blocks_full,31,24);
    bits::set(&file_buf[file_buf_pos],0,55,9);
  }
  file_buf_pos+=cray_word_size;
  for (size_t n=file_buf_pos; n < file_buf_pos+8; ++n) {
    file_buf[n]=0;
  }
  bits::set(&file_buf[file_buf_pos],0xe,0,4);
  bits::set(&file_buf[file_buf_pos],oc.blocks_full,20,20);
  --oc.block_space;
  oc.wrote_eof=true;
}

void iocstream::close()
{
  switch (stream_type) {
    case 0:
    {
	icstream::close();
	break;
    }
    case 1:
    {
	ocstream::close();
	break;
    }
  }
}

int iocstream::ignore()
{
  switch (stream_type) {
    case 0:
    {
	return icstream::ignore();
    }
    case 1:
    {
	std::cerr << "Warning: ignore should not be called on a stream opened for writing" << std::endl;
    }
  }
  return bfstream::error;
}

bool iocstream::open(std::string filename)
{
  std::cerr << "Error: no mode given" << std::endl;
  return false;
}

bool iocstream::open(std::string filename,std::string mode)
{
  if (mode == "r") {
    stream_type=0;
    return icstream::open(filename);
  }
  else if (mode == "w") {
    stream_type=1;
    return ocstream::open(filename);
  }
  else {
    std::cerr << "Error: bad mode " << mode << std::endl;
    return false;
  }
}

int iocstream::peek()
{
  switch (stream_type) {
    case 0:
    {
	return icstream::peek();
    }
    case 1:
    {
	std::cerr << "Warning: peek should not be called on a stream opened for writing" << std::endl;
    }
  }
  return bfstream::error;
}

int iocstream::read(unsigned char *buffer,size_t buffer_length)
{
  switch (stream_type) {
    case 0:
    {
	return icstream::read(buffer,buffer_length);
    }
    case 1:
    {
	std::cerr << "Warning: read should not be called on a stream opened for writing" << std::endl;
    }
  }
  return bfstream::error;
}

void iocstream::rewind()
{
  switch (stream_type) {
    case 0:
    {
	icstream::rewind();
	break;
    }
    case 1:
    {
	ocstream::rewind();
	break;
    }
  }
}

int iocstream::write(const unsigned char *buffer,size_t num_bytes)
{
  switch (stream_type) {
    case 0:
    {
	std::cerr << "Warning: write should not be called on a stream opened for reading" << std::endl;
    }
    case 1:
    {
	return ocstream::write(buffer,num_bytes);
    }
  }
  return bfstream::error;
}

void iocstream::write_eof()
{
  switch (stream_type) {
    case 0:
    {
	std::cerr << "Warning: write_eof should not be called on a stream opened for reading" << std::endl;
    }
    case 1:
    {
	ocstream::write_eof();
	break;
    }
  }
}
