#include <iostream>
#include <navyspotstream.hpp>
#include <bits.hpp>
#include <utils.hpp>

InputNavySpotStream::~InputNavySpotStream()
{
  if (file_buf != nullptr) {
    file_buf.release();
  }
  if (header != nullptr) {
    header.release();
  }
}

/*
InputNavySpotStream& InputNavySpotStream::operator=(const InputNavySpotStream& source)
{
  if (this == &source) {
    return *this;
  }
  if (ics != nullptr && source.ics != nullptr) {
    file_buf_len=source.file_buf_len;
    offset=source.offset;
    if (file_buf == nullptr && source.file_buf != nullptr) {
	file_buf=new unsigned char[50000];
	std::copy(source.file_buf,source.file_buf+file_buf_len,file_buf);
    }
    else if (file_buf != nullptr && source.file_buf == nullptr) {
	delete[] file_buf;
	file_buf=nullptr;
    }
    else if (file_buf != nullptr && source.file_buf != nullptr) {
	std::copy(source.file_buf,source.file_buf+file_buf_len,file_buf);
    }
    if (header == nullptr && source.header != nullptr) {
	header=new char[10];
	std::copy(source.header,source.header+10,header);
    }
    else if (header != nullptr && source.header == nullptr) {
	delete[] header;
	header=nullptr;
    }
    else if (header != nullptr && source.header != nullptr) {
	std::copy(source.header,source.header+10,header);
    }
    ics=source.ics;
  }
  else if (irs != nullptr && source.irs != nullptr) {
    irs=source.irs;
  }
  return *this;
}
*/


int InputNavySpotStream::ignore()
{
  if (ics != NULL) {
    return bfstream::error;
  }
  else if (irs != NULL) {
    int bytes_read=irs->ignore();
    if (bytes_read != bfstream::eof) {
      ++num_read;
    }
    return bytes_read;
  }
  else {
    return bfstream::error;
  }
}

bool InputNavySpotStream::open(std::string filename)
{
  if (is_open()) {
    std::cerr << "Error: currently connected to another file stream" << std::endl;
    exit(1);
  }
  num_read=0;
  idstream::open(filename);
  if (ics != NULL) {
    file_buf.reset(new unsigned char[50000]);
    offset=0;
    header.reset(new char[10]);
    header[0]='\0';
  }
  return true;
}

int InputNavySpotStream::peek()
{
  if (!is_open()) {
    std::cerr << "Error: no InputNavySpotStream has been opened" << std::endl;
    exit(1);
  }
  if (ics != nullptr) {
    return bfstream::error;
  }
  else if (irs != nullptr) {
    return irs->peek();
  }
  else {
    return bfstream::error;
  }
}

int InputNavySpotStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (!is_open()) {
    std::cerr << "Error: no InputNavySpotStream has been opened" << std::endl;
    exit(1);
  }
  int bytes_read;
  bool get_new_block=false;
  if (ics != nullptr) {
    short nmand=0,ntrop=0,nwind=0,nsig=0,nmax=0;
// check for a good station id word and dictionary word
    if (header[0] == '\0') {
	get_new_block=true;
    }
    else {
	long long check;
	bits::get(file_buf.get(),check,offset,60);
	if (check != 0) {
	  if (header[9] == '2' || header[9] == '6') {
	    bits::get(file_buf.get(),check,offset+60,42);
	    while (check != 0) {
		offset+=60;
		bits::get(file_buf.get(),check,offset+60,42);
	    }
	    bits::get(file_buf.get(),nwind,offset+102,6);
	    bits::get(file_buf.get(),ntrop,offset+108,6);
	    bits::get(file_buf.get(),nmand,offset+114,6);
	    if (nmand == 0 && ntrop == 0 && nwind == 0) {
		get_new_block=true;
	    }
	  }
	  else if (header[9] == '5') {
	    bits::get(file_buf.get(),nmax,offset+288,6);
	    bits::get(file_buf.get(),nsig,offset+294,6);
	  }
	}
	else {
	  get_new_block=true;
	}
    }
// read another block from the disk file
    if (get_new_block || static_cast<int>(offset+180) > file_buf_len) {
	if ( (file_buf_len=ics->read(file_buf.get(),50000)) == bfstream::eof) {
	  return file_buf_len;
	}
// if bytes_read equals the buffer length, exit as a fatal error because the
// entire raob may not have been read
	if (file_buf_len >= 50000) {
	  std::cerr << "Error: buffer overflow" << std::endl;
	  exit(1);
	}
	bits::get(file_buf.get(),header.get(),0,6,0,10);
	charconversions::ebcd_to_ascii(header.get(),header.get(),10);
	auto bad_header=false;
	for (size_t n=0; n < 8; ++n) {
	  if (header[n] < '0' || header[n] > '9') {
	    bad_header=true;
	    n=8;
	  }
	}
	while (bad_header || header[9] < '1' || header[9] > '6') {
	  if ( (file_buf_len=ics->read(file_buf.get(),50000)) == bfstream::eof) {
	    return file_buf_len;
	  }
	  if (file_buf_len >= 50000) {
	    std::cerr << "Error: buffer overflow" << std::endl;
	    exit(1);
	  }
	  bits::get(file_buf.get(),header.get(),0,6,0,10);
	  charconversions::ebcd_to_ascii(header.get(),header.get(),10);
	  bad_header=false;
	  for (size_t n=0; n < 8; ++n) {
	    if (header[n] < '0' || header[n] > '9') {
		bad_header=true;
		n=8;
	    }
	  }
	}
	file_buf_len*=8;
	offset=60;
	if (header[9] == '2' || header[9] == '6') {
	  bits::get(file_buf.get(),nwind,offset+102,6);
	  bits::get(file_buf.get(),ntrop,offset+108,6);
	  bits::get(file_buf.get(),nmand,offset+114,6);
	}
	else if (header[9] == '5') {
	  bits::get(file_buf.get(),nmax,offset+288,6);
	  bits::get(file_buf.get(),nsig,offset+294,6);
	}
    }
// copy the current raob into the user buffer
    std::copy(&header[0],&header[10],buffer);
    size_t off;
    if (header[9] == '2' || header[9] == '6') {
	off=(2+nmand+ntrop+(nwind+1)/2)*60;
    }
    else if (header[9] == '5') {
	off=(5+(nmax+nsig+1)/2)*60;
    }
    else if (header[9] == '4') {
	off=120;
    }
    else {
	std::cerr << "Error: unable to get offset for sounding type '" << header[9] << "'" << std::endl;
	exit(1);
    }
    bytes_read=(80+off+7)/8;
    bits::get(file_buf.get(),&buffer[10],offset,8,0,bytes_read);
    offset+=off;
  }
  else if (irs != NULL) {
    bytes_read=irs->read(buffer,buffer_length);
    if (bytes_read != bfstream::eof) {
	++num_read;
    }
    return bytes_read;
  }
  else {
    bytes_read=bfstream::error;
  }
  ++num_read;
  return bytes_read;
}
