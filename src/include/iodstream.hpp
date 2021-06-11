// FILE: iodstream.h

#ifndef IODSTREAM_H
#define IODSTREAM_H

#include <iostream>
#include <bfstream.hpp>
#include <s3.hpp>

class iods {
public:
  enum class Blocking{ binary = 0, cos, rptout, cos_rptout };

  iods() : file_name(), fs() { }
  virtual ~iods() { }
  virtual void close() = 0;
  std::string name() const { return file_name; }
  virtual bool is_open() const { return (fs.is_open()); }

protected:
  std::string file_name;
  std::fstream fs;
};

class idstream;
class idstream_iterator {
public:
  idstream_iterator(idstream& stream, bool is_end) : stream(stream), is_end(
      is_end), buffer(nullptr), buffer_capacity(0) { }
  bool operator !=(const idstream_iterator& source) { return is_end !=
      source.is_end; }
  const idstream_iterator& operator++();
  unsigned char *operator*();

private:
  idstream& stream;
  bool is_end;
  std::unique_ptr<unsigned char[]> buffer;
  int buffer_capacity;
};

class idstream : public iods {
public:
  idstream() : ics(nullptr), irs(nullptr), if77s(nullptr), ivs(nullptr), is3s(
      nullptr), num_read(0) { }

// pure virtual functions from iods
  virtual ~idstream() { close(); }
  virtual bool is_open() const {
    return (fs.is_open() || ics != nullptr || irs != nullptr || if77s != nullptr
        || ivs != nullptr || is3s != nullptr);
  }

// pure virtual functions making idstream an abstract class
  virtual int ignore() = 0;
  virtual int peek() = 0;
  virtual int read(unsigned char *buffer, size_t buffer_length) = 0;

// local methods
  virtual void close();
  virtual off_t current_record_offset() const { return -1; }
  size_t number_read() const { return num_read; }
  virtual bool open(std::string filename);
  bool is_cos_blocked() const { return ics != nullptr; }
  virtual void rewind();

// range-base support
  idstream_iterator begin();
  idstream_iterator end();

protected:
  std::unique_ptr<icstream> ics;
  std::unique_ptr<irstream> irs;
  std::unique_ptr<if77stream> if77s;
  std::unique_ptr<ivbsstream> ivs;
  std::unique_ptr<is3stream> is3s;
  size_t num_read;
};

class odstream : public iods {
public:
  odstream() : ocs(nullptr), ors(nullptr), ocrs(nullptr), num_written(0) { }
  virtual ~odstream() { close(); }
  virtual void close();
  virtual bool is_open() const {
    return (fs.is_open() || ocs != nullptr || ors != nullptr || ocrs !=
        nullptr);
  }
  size_t number_written() const { return num_written; }
  virtual bool open(std::string filename, Blocking block_flag = Blocking::
      binary);
  virtual int write(const unsigned char *buffer, size_t num_bytes);

protected:
  std::unique_ptr<ocstream> ocs;
  std::unique_ptr<orstream> ors;
  std::unique_ptr<ocrstream> ocrs;
  size_t num_written;
};

#endif
