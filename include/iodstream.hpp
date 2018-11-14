// FILE: iodstream.h

#ifndef IODSTREAM_H
#define IODSTREAM_H

#include <iostream>
#include <bfstream.hpp>

class iods
{
public:
  enum {
    binary,cos,rptout,cos_rptout
  };
  iods() : file_name(),fs() {}
  virtual ~iods() {}
  virtual void close()=0;
  std::string name() const { return file_name; }
  virtual bool is_open() const { return (fs.is_open()); }

protected:
  std::string file_name;
  std::fstream fs;
};

class idstream;
class idstream_iterator
{
public:
  idstream_iterator(idstream& stream,bool is_end) : _stream(stream),_is_end(is_end),buffer(nullptr),buffer_capacity(0) {}
  bool operator !=(const idstream_iterator& source) { return (_is_end != source._is_end); }
  const idstream_iterator& operator++();
  unsigned char *operator*();

private:
  idstream &_stream;
  bool _is_end;
  std::unique_ptr<unsigned char[]> buffer;
  int buffer_capacity;
};

class idstream : public iods
{
public:
  idstream() : icosstream(nullptr),irptstream(nullptr),if77_stream(nullptr),ivstream(nullptr),num_read(0) {}

// pure virtual functions from iods
  virtual ~idstream() { close(); }
  virtual bool is_open() const { return (fs.is_open() || icosstream != nullptr || irptstream != nullptr || if77_stream != nullptr || ivstream != nullptr); }

// pure virtual functions making idstream an abstract class
  virtual int ignore()=0;
  virtual int peek()=0;
  virtual int read(unsigned char *buffer,size_t buffer_length)=0;

// local methods
  virtual void close();
  virtual off_t current_record_offset() const { return -1; }
  size_t number_read() const { return num_read; }
  virtual bool open(std::string filename);
  bool is_cos_blocked() const { return (icosstream != nullptr); }
  virtual void rewind();

// range-base support
  idstream_iterator begin();
  idstream_iterator end();

protected:
  std::unique_ptr<icstream> icosstream;
  std::unique_ptr<irstream> irptstream;
  std::unique_ptr<if77stream> if77_stream;
  std::unique_ptr<ivbsstream> ivstream;
  size_t num_read;
};

class odstream : public iods
{
public:
  odstream() : ocosstream(nullptr),orptstream(nullptr),ocrptstream(nullptr),num_written(0) {}
  virtual ~odstream() { close(); }
  virtual void close();
  virtual bool is_open() const { return (fs.is_open() || ocosstream != nullptr || orptstream != nullptr || ocrptstream != nullptr);
  }
  size_t number_written() const { return num_written; }
  virtual bool open(std::string filename,size_t blocking_flag=binary);
  virtual int write(const unsigned char *buffer,size_t num_bytes);

protected:
  std::unique_ptr<ocstream> ocosstream;
  std::unique_ptr<orstream> orptstream;
  std::unique_ptr<ocrstream> ocrptstream;
  size_t num_written;
};

#endif
