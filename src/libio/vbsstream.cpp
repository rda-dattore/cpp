// FILE: vbsstream.cpp

#include <bfstream.hpp>
#include <bits.hpp>

inline int ivbsstream::read_from_disk()
{
  if (file_buf == nullptr) {
    capacity=8;
    file_buf.reset(new unsigned char[capacity]);
  }
  fs.read(reinterpret_cast<char *>(file_buf.get()),8);
  if (fs.gcount() != 8) {
    return eof;
  }
  int chkbits;
  bits::get(file_buf.get(),chkbits,16,16);
  if (chkbits != 0) {
    return error;
  }
  bits::get(file_buf.get(),chkbits,56,8);
  if (chkbits != 0) {
    return error;
  }
  bits::get(file_buf.get(),file_buf_len,0,16);
  if (file_buf_len == 0) {
    return error;
  }
  size_t llen;
  bits::get(file_buf.get(),llen,32,16);
  if (llen > file_buf_len) {
    return error;
  }
  if (file_buf_len > capacity) {
    auto *temp=new unsigned char[8];
    std::copy(&file_buf[0],&file_buf[8],temp);
    capacity=file_buf_len;
    file_buf.reset(new unsigned char[capacity]);
    std::copy(&temp[0],&temp[8],file_buf.get());
    delete[] temp;
    max_block_len=capacity;
  }
  int read_len=file_buf_len-8;
  fs.read(reinterpret_cast<char *>(&file_buf[8]),read_len);
  int status=0;
  if (fs.gcount() != read_len) {
    status=error;
  }
  file_buf_pos=4;
  num_blocks++;
  return status;
}

ivbsstream& ivbsstream::operator=(const ivbsstream& source)
{
  return *this;
}

void ivbsstream::close()
{
  if (!is_open()) {
    return;
  }
  fs.close();
}

int ivbsstream::ignore()
{
  if (file_buf_pos == file_buf_len) {
    int status;
    if ( (status=read_from_disk()) != 0) {
	return status;
    }
  }
  int lrec_len;
  bits::get(&file_buf[file_buf_pos],lrec_len,0,16);
  if (lrec_len == 0) {
    return -1;
  }
  file_buf_pos+=lrec_len;
  lrec_len-=4;
  ++num_read;
  return lrec_len;
}

int ivbsstream::peek()
{
  int llen;
  if (file_buf_pos == file_buf_len || num_read == 0) {
    unsigned char test[6];
    fs.read(reinterpret_cast<char *>(test),6);
    if (fs.gcount() != 6) {
	return eof;
    }
    bits::get(test,llen,32,16);
    fs.seekg(-6,std::ios_base::cur);
    return llen;
  }
  else {
    bits::get(&file_buf[file_buf_pos],llen,0,16);
    return llen;
  }
}

int ivbsstream::read(unsigned char *buffer,size_t num_bytes)
{
  if (file_buf_pos == file_buf_len) {
    int status;
    if ( (status=read_from_disk()) != 0) {
	if (status == error) {
	  ++num_read;
	}
	return status;
    }
  }
  size_t lrec_len;
  bits::get(&file_buf[file_buf_pos],lrec_len,0,16);
  int segment_control_char;
  bits::get(&file_buf[file_buf_pos],segment_control_char,16,8);
  if (lrec_len == 0) {
    return -1;
  }
  lrec_len-=4;
  int bytes_copied;
  if (num_bytes > lrec_len) {
    bytes_copied=lrec_len;
  }
  else {
    bytes_copied=num_bytes;
  }
  file_buf_pos+=4;
  if (bytes_copied > 0) {
    std::copy(&file_buf[file_buf_pos],&file_buf[file_buf_pos+bytes_copied],buffer);
  }
  file_buf_pos+=lrec_len;
  switch (segment_control_char) {
    case 1:
    case 3:
    {
	bytes_copied+=read(&buffer[bytes_copied],num_bytes-bytes_copied);
	break;
    }
  }
  ++num_read;
  return bytes_copied;
}

void ivbsstream::rewind()
{
  fs.seekg(std::ios_base::beg);
  file_buf_len=file_buf_pos=0;
  num_read=num_blocks=0;
}

ovbsstream& ovbsstream::operator=(const ovbsstream& source)
{
  return *this;
}

void ovbsstream::close()
{
  if (!is_open()) {
    return;
  }
  fs.close();
  file_name="";
}

int ovbsstream::write(const unsigned char *buffer,size_t num_bytes)
{
return error;
}
