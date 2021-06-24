// FILE: adpstream.cpp

#include <iostream>
#include <adpstream.hpp>
#include <strutils.hpp>
#include <bits.hpp>

int InputADPStream::read_from_disk()
{
  auto num_bytes=0;
  if (ics != nullptr) {
    num_bytes=ics->read(file_buf.get(),file_buf_len);
    if (num_read == 0) {
	while (num_bytes > 0 && std::string(reinterpret_cast<char *>(&file_buf[20]),10) != "WASHINGTON") {
	  num_bytes=ics->read(file_buf.get(),file_buf_len);
	}
    }
    if (num_bytes == bfstream::error || num_bytes == bfstream::eof) {
	return num_bytes;
    }
  }
  else {
    fs.getline(reinterpret_cast<char *>(file_buf.get()),file_buf_len+2);
    if (num_read == 0) {
	while (fs.good() && std::string(reinterpret_cast<char *>(&file_buf[20]),10) != "WASHINGTON") {
	  fs.getline(reinterpret_cast<char *>(file_buf.get()),file_buf_len+2);
	}
    }
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (!fs.good()) {
	return bfstream::error;
    }
    num_bytes=file_buf_len+1;
    while (num_bytes > 0 && file_buf[num_bytes] != 0xa) {
	--num_bytes;
    }
    if (num_bytes == 0) {
	std::string s(&(reinterpret_cast<char *>(file_buf.get())[file_buf_len-8]),8);
	std::string test(8,file_buf[file_buf_len-8]);
	if (s == test || s == "NCAR DSS") {
	  fs.seekg(-1,std::ios_base::cur);
	  num_bytes=file_buf_len;
	}
	else if (file_buf_len == 6440) {
// check for 5120-byte records
	  s.assign(&(reinterpret_cast<char *>(file_buf.get())[file_buf_len-1328]),8);
	  test.assign(file_buf[file_buf_len-1328],8);
	  if (s == test) {
	    fs.seekg(-1321,std::ios_base::cur);
	    file_buf_len=5120;
	  }
	  else {
	    long long l;
	    bits::get(&file_buf[file_buf_len-8],l,0,64);
	    if (l == 0) {
		fs.seekg(-1,std::ios_base::cur);
		num_bytes=file_buf_len;
	    }
	    else {
		std::cerr << "Error: missing newline (1)" << std::endl;
		return bfstream::error;
	    }
	  }
	}
	else {
	  std::cerr << "Error: missing newline (2)" << std::endl;
	  return bfstream::error;
	}
    }
  }
  std::string s(reinterpret_cast<char *>(file_buf.get()),10);
  if (s == "ENDOF FILE" || s == "END RECORD") {
    num_bytes=read_from_disk();
  }
  else if (std::string(&(reinterpret_cast<char *>(file_buf.get())[20]),10) == "WASHINGTON") {
    if (std::string(reinterpret_cast<char *>(file_buf.get()),3) == "ADP") {
	std::copy(&file_buf[14],&file_buf[16],date);
	date[2]='0';
	date[3]='0';
	std::copy(&file_buf[8],&file_buf[14],&date[4]);
    }
    else {
	std::copy(&file_buf[0],&file_buf[10],date);
    }
    ++num_read;
    num_bytes=read_from_disk();
    --num_read;
  }
  file_buf_pos=0;
  return num_bytes;
}

int InputADPStream::ignore()
{
return bfstream::error;
}

int InputADPStream::peek()
{
return bfstream::error;
}

int InputADPStream::read(unsigned char *buffer,size_t buffer_length)
{
  size_t rptlen;
  int num_copied,num_bytes;

  if (!is_open()) {
    std::cerr << "Error: no open file stream" << std::endl;
    exit(1);
  }
  if (file_buf == nullptr) {
    if (irs != nullptr) {
	is_ascii=false;
    }
    else {
	is_ascii=true;
	file_buf_len=file_buf_pos=6440;
	file_buf.reset(new unsigned char[file_buf_len+2]);
    }
  }
  if (is_ascii) {
    if (file_buf_pos == file_buf_len || file_buf[file_buf_pos+37] == 0x20 || file_buf[file_buf_pos+38] == 0x20 || file_buf[file_buf_pos+39] == 0x20) {
	num_copied=read_from_disk();
	if (num_copied < 0) {
	  return num_copied;
	}
    }
    strutils::strget(reinterpret_cast<char *>(&file_buf[file_buf_pos+37]),rptlen,3);
    rptlen*=10;
    if (buffer_length >= rptlen+10) {
	num_copied=rptlen+10;
    }
    else {
	num_copied=buffer_length;
    }
//std::cerr << ftell(fp) << " " << rptlen << std::endl;
    if ((file_buf_pos+rptlen-10) > file_buf_len || std::string(&(reinterpret_cast<char *>(file_buf.get())[file_buf_pos+rptlen-10]),10) != "END REPORT") {
	file_buf_pos=file_buf_len;
//std::cerr << "C " << ftell(fp) << std::endl;
	return bfstream::error;
    }
    std::copy(date,date+10,buffer);
    std::copy(&file_buf[file_buf_pos],&file_buf[file_buf_pos+num_copied-10],&buffer[10]);
    file_buf_pos+=rptlen;
    if (std::string(reinterpret_cast<char *>(&file_buf[file_buf_pos]),10) == "END RECORD") {
	file_buf_pos=file_buf_len;
    }
    ++num_read;
    return num_copied;
  }
  else {
    num_bytes=irs->read(buffer,buffer_length);
    if (num_bytes > 0) {
	++num_read;
    }
    return num_bytes;
  }
}
