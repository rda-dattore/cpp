// FILE: bfstream.h
//

#ifndef BFSTREAM_H
#define BFSTREAM_H

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <datetime.hpp>

class bfstream {
public:
  enum {eof=-2,error};

  size_t block_count() const { return num_blocks; }
  virtual void close()=0;
  virtual bool is_open() const { return fs.is_open(); }
  size_t maximum_block_size() const { return max_block_len; }
  virtual void rewind()=0;

protected:
  bfstream() : fs(),file_buf(nullptr),file_buf_len(0),file_buf_pos(0),num_blocks(0),max_block_len(0),file_name() {}
  bfstream(const bfstream& source) : bfstream() { *this=source; }
  virtual ~bfstream();
  bfstream& operator=(const bfstream& source) { return *this; }

  std::fstream fs;
  std::unique_ptr<unsigned char[]> file_buf;
  size_t file_buf_len,file_buf_pos;
  size_t num_blocks,max_block_len;
  std::string file_name;
};

class ibfstream : virtual public bfstream
{
public:
  ibfstream() : num_read(0) {}
  virtual int ignore()=0;
  size_t number_read() const { return num_read; }
  virtual bool open(std::string filename);
  virtual int peek()=0;
  virtual int read(unsigned char *buffer,size_t buffer_length)=0;

protected:
  size_t num_read;
};

class obfstream : virtual public bfstream
{
public:
  obfstream() : num_written(0) {}
  size_t number_written() const { return num_written; }
  virtual bool open(std::string filename);
  virtual int write(const unsigned char *buffer,size_t num_bytes)=0;

protected:
  size_t num_written;
};

class craystream : virtual public bfstream
{
public:
  static const int eod;

protected:
  craystream();

  static const size_t cray_block_size,cray_word_size;
  static const short cw_bcw,cw_eor,cw_eof,cw_eod;
  short cw_type;
};

class icstream : public ibfstream, virtual public craystream
{
public:
  icstream() : at_eod(false) {}
  icstream(std::string filename) : icstream() { open(filename); at_eod=false; }
  icstream(const icstream& source) : icstream() { *this=source; }
  icstream& operator=(const icstream& source);
  void close();
  int ignore();
  bool open(std::string filename);
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  void rewind();

private:
  int read_from_disk();

  bool at_eod;
};

class ocstream : public obfstream, virtual public craystream
{
public:
  ocstream() : oc() {}
  ocstream(std::string filename) : ocstream() { open(filename); }
  virtual ~ocstream() { if (is_open()) close(); }
  void close();
  bool open(std::string filename);
  void rewind() {}
  int write(const unsigned char *buffer,size_t num_bytes);
  void write_eof();

private:
  int write_to_disk();

  struct OCData {
    OCData() : block_space(0),blocks_full(0),blocks_back(0),wrote_eof(false) {}

    size_t block_space,blocks_full,blocks_back;
    bool wrote_eof;
  } oc;
};

class iocstream : public icstream, public ocstream
{
public:
  iocstream() : stream_type(0) {}
  iocstream(std::string filename,const char *mode) : iocstream() { open(filename,mode); }
  virtual ~iocstream() { if (is_open()) close(); }
  void close();
  int ignore();
  bool is_input() { return (stream_type == 0); }
  bool is_output() { return (stream_type == 1); }
  bool open(std::string filename);
  bool open(std::string filename,std::string mode);
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  void rewind();
  int write(const unsigned char *buffer,size_t num_bytes);
  void write_eof();

private:
  short stream_type;
};

class rptoutstream : virtual public bfstream
{
public:
  size_t block_length() const { return block_len; }
  bool is_new_block() const { return new_block; }

protected:
  rptoutstream() : block_len(0),word_count(0),new_block(false) { file_buf.reset(new unsigned char[8000]); }

  size_t block_len,word_count;
  bool new_block;
};

class irstream : public ibfstream, virtual public rptoutstream
{
public:
  irstream() : icosstream(nullptr),_flag() {}
  irstream(std::string filename) : irstream() { open(filename); }
  irstream(const irstream& source) : irstream() { *this=source; }
  virtual ~irstream() { if (is_open()) close(); }
  irstream& operator=(const irstream& source);
  void close();
  int flag() const { return static_cast<int>(_flag); }
  int ignore();
  bool is_open() const { return (fs.is_open() || icosstream != nullptr); }
  bool open(std::string filename);
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  void rewind();
  size_t words_read() const { return word_count; }

protected:
  std::unique_ptr<icstream> icosstream;

private:
  unsigned char _flag;
};

class orstream : public obfstream, public rptoutstream
{
public:
  orstream() {}
  orstream(std::string filename) { open(filename); }
  virtual ~orstream() { if (is_open()) close(); }
  void close();
  int flush();
  bool open(std::string filename);
  void rewind() {}
  int write(const unsigned char *buffer,size_t num_bytes);
};

class ocrstream : public obfstream, virtual public rptoutstream
{
public:
  ocrstream() : ocosstream(nullptr) {}
  ocrstream(const ocrstream& source) : ocrstream() { *this=source; }
  ocrstream(std::string filename) : ocrstream() { open(filename); }
  virtual ~ocrstream() { if (is_open()) close(); }
  ocrstream& operator=(const ocrstream& source);
  void close();
  int flush();
  bool is_open() const { return (ocosstream != nullptr); }
  bool open(std::string filename);
  void rewind() {}
  size_t words_written() const { return word_count; }
  int write(const unsigned char *buffer,size_t num_bytes);
  void write_eof();

protected:
  std::unique_ptr<ocstream> ocosstream;
};

class iocrstream : public irstream, public ocrstream
{
public:
  iocrstream() : stream_type(0) {}
  iocrstream(std::string filename,const char *mode) : iocrstream() { open(filename,mode); }
  virtual ~iocrstream() { if (is_open()) close(); }
  void close();
  int flush();
  size_t block_length() const;
  int ignore();
  bool is_input() { return (stream_type == 0); }
  bool is_new_block() const;
  bool is_open() const { return (fs.is_open() || icosstream != NULL || ocosstream != NULL); }
  bool is_output() { return (stream_type == 1); }
  bool open(std::string filename);
  bool open(std::string filename,std::string mode);
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  void rewind();
  int write(const unsigned char *buffer,size_t num_bytes);
  void write_eof();

private:
  short stream_type;
};

class vbsstream
{
public:
protected:
  vbsstream() : capacity(0) {}
  virtual ~vbsstream() {}

  size_t capacity;
};

class ivbsstream : public ibfstream, public vbsstream
{
public:
  ivbsstream() {}
  ivbsstream(std::string filename) { open(filename); }
  ivbsstream(const ivbsstream& source) { *this=source; }
  ivbsstream& operator=(const ivbsstream& source);
  void close();
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  void rewind();

private:
  int read_from_disk();
};

class ovbsstream : public obfstream, public vbsstream
{
public:
  ovbsstream() : block_space(0),blocks_full(0),blocks_back(0) {}
  ovbsstream(std::string filename) : ovbsstream() { open(filename); }
  ovbsstream(const ovbsstream& source) : ovbsstream() { *this=source; }
  virtual ~ovbsstream() { if (is_open()) close(); }
  ovbsstream& operator=(const ovbsstream& source);
  void close();
  void rewind() {}
  int write(const unsigned char *buffer,size_t num_bytes);

private:
  void write_to_disk();

  size_t block_space,blocks_full,blocks_back;
};

class if77stream : public ibfstream
{
public:
  if77stream() {}
  if77stream(std::string filename) { open(filename); }
  if77stream(const if77stream& source) { *this=source; }
  if77stream& operator=(const if77stream& source);
  void close();
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  void rewind() { fs.seekg(0,std::ios::beg); }
};

class of77stream : public obfstream
{
public:
  of77stream() {}
  of77stream(std::string filename) { open(filename); }
  of77stream(const of77stream& source) { *this=source; }
  virtual ~of77stream() { if (is_open()) close(); }
  of77stream& operator=(const of77stream& source);
  void close();
  void rewind() {}
  int write(const unsigned char *buffer,size_t num_bytes);
};

class ivmsbackupstream : public ibfstream
{
public:
  enum {undefined_record=-2,xor_record,null_record,summary_record,volume_record,file_record,vbn_record,physical_volume_record,lbn_record,fid_record};

  ivmsbackupstream() : icosstream(nullptr),block_num(-1),rec_size(-1),rec_type(-1) {}
  ivmsbackupstream(std::string filename) : ivmsbackupstream() { open(filename); }
  ivmsbackupstream(const ivmsbackupstream& source) : ivmsbackupstream() { *this=source; }
  ivmsbackupstream& operator=(const ivmsbackupstream& source);
  int block_number() const { return block_num; }
  void close();
  int ignore();
  bool is_open() const { return (icosstream != nullptr); }
  bool open(std::string filename);
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  short record_size() const { return rec_size; }
  short record_type() const { return rec_type; }
  void rewind();

protected:
  std::shared_ptr<icstream> icosstream;
  int block_num;
  short rec_size,rec_type;
};

namespace vmsbackup {

struct SummaryData {
  SummaryData() : saveset_name(),backup_command(),username(),os_version(),node_name(),device(),backup_version(),user_id(-1),group_id(-1),os_code(-1),xor_group_size(-1),buffer_count(-1),vms_time(-1),saveset_datetime(),cpu_id(-1),block_size(-1) {}

  std::string saveset_name,backup_command,username,os_version,node_name,device,backup_version;
  short user_id,group_id,os_code,xor_group_size,buffer_count;
  long long vms_time;
  DateTime saveset_datetime;
  int cpu_id,block_size;
};
struct FileData {
  FileData() : file_name(),file_alloc(-1),user_id(-1),group_id(-1) {}

  std::string file_name;
  int file_alloc;
  short user_id,group_id;
};

extern void print_file_record_data(std::ostream& outs,FileData f);
extern FileData decode_file_record(unsigned char *buffer,size_t record_size);
extern void print_summary_record_data(std::ostream& outs,SummaryData s);
extern SummaryData decode_summary_record(unsigned char *buffer,size_t record_size);

} // end namespace vmsbackup

#endif
