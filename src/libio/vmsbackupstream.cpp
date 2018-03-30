// FILE: vmsbackupstream.cpp

#include <iostream>
#include <bfstream.hpp>
#include <utils.hpp>
#include <myerror.hpp>

ivmsbackupstream& ivmsbackupstream::operator=(const ivmsbackupstream& source)
{
  return *this;
}

void ivmsbackupstream::close()
{
  if (!is_open()) {
    return;
  }
  if (icosstream != nullptr) {
    icosstream.reset();
    icosstream=nullptr;
  }
}

int ivmsbackupstream::ignore()
{
  int num_bytes=0;
  if (file_buf_pos == file_buf_len) {
    if (icosstream != nullptr) {
	if ( (num_bytes=icosstream->read(file_buf.get(),32768)) == eof) {
	  return eof;
	}
    }
    else {
	return error;
    }
    ++num_blocks;
  }
  ++num_read;
  return num_bytes;
}

bool ivmsbackupstream::open(std::string filename)
{
  if (is_open()) {
    std::cerr << "Error: currently connected to another file stream" << std::endl;
    exit(1);
  }
  icosstream.reset(new icstream);
  if (!icosstream->open(filename)) {
    icosstream.reset();
    icosstream=nullptr;
    return false;
  }
  unsigned char test_buffer;
  if (icosstream->read(&test_buffer,1) < 0) {
    icosstream->close();
    icosstream.reset();
    icosstream=nullptr;
    return false;
  }
  else {
    icosstream->rewind();
  }
  file_buf_len=file_buf_pos=0;
  num_read=num_blocks=0;
  return true;
}

int ivmsbackupstream::peek()
{
return 0;
}

int ivmsbackupstream::read(unsigned char *buffer,size_t num_bytes)
{
  int nbytes=0;
  if (!is_open() || icosstream == nullptr) {
    std::cerr << "Error: no open file stream" << std::endl;
    exit(1);
  }
  rec_type=undefined_record;
  if (file_buf_pos == file_buf_len) {
    int bsize=icosstream->peek();
    if (bsize < 0) {
	return bsize;
    }
    if (file_buf_len < static_cast<size_t>(bsize)) {
	file_buf_len=bsize;
	file_buf.reset(new unsigned char[file_buf_len]);
    }
    if ( (nbytes=icosstream->read(file_buf.get(),file_buf_len)) < 0) {
	return nbytes;
    }
    file_buf_pos=0;
  }
  if (file_buf_pos == 0) {
// block header
    ++num_blocks;
    short block_type=unixutils::little_endian(&file_buf[6],2);
    block_num=unixutils::little_endian(&file_buf[8],4);
    switch (block_type) {
	case 1:
	{
	  if (num_bytes < 256) {
	    myerror="buffer overflow";
	    exit(1);
	  }
	  file_buf_pos+=256;
	  break;
	}
	case 2:
	{
// XOR block/record
	  if (num_bytes < file_buf_len) {
	    myerror="buffer overflow";
	    exit(1);
	  }
	  std::copy(&file_buf[272],&file_buf[file_buf_len],&buffer[0]);
	  file_buf_pos=file_buf_len;
	  rec_size=file_buf_len-272;
	  rec_type=xor_record;
	  ++num_read;
	  return (file_buf_len-272);
	}
	default:
	{
	  myerror="invalid block type";
	  exit(1);
	}
    }
  }
// record header
  rec_size=unixutils::little_endian(&file_buf[file_buf_pos],2);
  rec_type=unixutils::little_endian(&file_buf[file_buf_pos+2],2);
  file_buf_pos+=16;
  std::copy(&file_buf[file_buf_pos],&file_buf[file_buf_pos+rec_size],&buffer[0]);
  file_buf_pos+=rec_size;
  ++num_read;
  return rec_size;
}

void ivmsbackupstream::rewind()
{
  if (icosstream != nullptr) {
    icosstream->rewind();
  }
  num_read=num_blocks=0;
}

namespace vmsbackup {

SummaryData decode_summary_record(unsigned char *buffer,size_t record_size)
{
  SummaryData s;
  char *cbuf=reinterpret_cast<char *>(buffer);
// check for a valid header
  if (buffer[0] != 1 || buffer[1] != 1) {
    return s;
  }
  size_t off=2;
  while (off < record_size) {
    short data_size=unixutils::little_endian(&buffer[off],2);
    short data_type=unixutils::little_endian(&buffer[off+2],2);
    switch (data_type) {
	case 0:
	{
// padding at end of summary
	  off=record_size;
	  break;
	}
	case 1:
	{
	  s.saveset_name.assign(&cbuf[off+4],data_size);
	  break;
	}
	case 2:
	{
	  s.backup_command.assign(&cbuf[off+4],data_size);
	  break;
	}
	case 4:
	{
	  s.username.assign(&cbuf[off+4],data_size);
	  break;
	}
	case 5:
	{
	  if (data_size == 4) {
	    s.user_id=unixutils::little_endian(&buffer[off+4],2);
	    s.group_id=unixutils::little_endian(&buffer[off+6],2);
	  }
	  break;
	}
	case 6:
	{
	  if (data_size == 8) {
	    s.vms_time=unixutils::little_endian(&buffer[off+4],8);
	    s.saveset_datetime=DateTime(1858,11,17,0,0).seconds_added(s.vms_time/10000000);
	  }
	  break;
	}
	case 7:
	{
	  if (data_size == 2) {
	    s.os_code=unixutils::little_endian(&buffer[off+4],2);
	  }
	  break;
	}
	case 8:
	{
	  s.os_version.assign(&cbuf[off+4],data_size);
	  break;
	}
	case 9:
	{
	  s.node_name.assign(&cbuf[off+4],data_size);
	  break;
	}
	case 10:
	{
	  if (data_size == 4) {
	    s.cpu_id=unixutils::little_endian(&buffer[off+4],4);
	  }
	  break;
	}
	case 11:
	{
	  s.device.assign(&cbuf[off+4],data_size);
	  break;
	}
	case 12:
	{
	  s.backup_version.assign(&cbuf[off+4],data_size);
	  break;
	}
	case 13:
	{
	  if (data_size == 4) {
	    s.block_size=unixutils::little_endian(&buffer[off+4],4);
	  }
	  break;
	}
	case 14:
	{
	  if (data_size == 2) {
	    s.xor_group_size=unixutils::little_endian(&buffer[off+4],2);
	  }
	  break;
	}
	case 15:
	{
	  if (data_size == 2) {
	    s.buffer_count=unixutils::little_endian(&buffer[off+4],2);
	  }
	  break;
	}
    }
    off+=(4+data_size);
  }
  return s;
}

void print_summary_record_data(std::ostream& outs,SummaryData s)
{
  outs << "Saveset name: " << s.saveset_name << std::endl;
}

FileData decode_file_record(unsigned char *buffer,size_t record_size)
{
  FileData f;
  char *cbuf=reinterpret_cast<char *>(buffer);
// check for a valid header
  if (buffer[0] != 1 || buffer[1] != 1) {
    return f;
  }
  size_t off=2;
  while (off < record_size) {
    short data_size=unixutils::little_endian(&buffer[off],2);
    short data_type=unixutils::little_endian(&buffer[off+2],2);
    switch (data_type) {
	case 0x2a:
	{
	  f.file_name.assign(&cbuf[off+4],data_size);
	  break;
	}
	case 0x2e:
	{
	  if (data_size == 4) {
	    f.file_alloc=unixutils::little_endian(&buffer[off+4],4);
	  }
	  break;
	}
	case 0x2f:
	{
	  if (data_size == 4) {
	    f.user_id=unixutils::little_endian(&buffer[off+4],2);
	    f.group_id=unixutils::little_endian(&buffer[off+6],2);
	  }
	  break;
	}
    }
    off+=(4+data_size);
  }
  return f;
}

void print_file_record_data(std::ostream& outs,FileData f)
{
  outs << "File name: " << f.file_name << std::endl;
  outs << "File allocation (512-byte blocks): " << f.file_alloc << std::endl;
}

} // end namespace vmsbackup
