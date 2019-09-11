// FILE: navyspotstream.h

#ifndef NAVYSPOTSTREAM_H
#define   NAVYSPOTSTREAM_H

#include <iodstream.hpp>

class InputNavySpotStream : public idstream
{
public:
  InputNavySpotStream() : file_buf(nullptr),file_buf_len(0),offset(0),header(nullptr) {}
  InputNavySpotStream(const char *filename) : InputNavySpotStream() { open(filename); }
//  InputNavySpotStream(const InputNavySpotStream& source) : InputNavySpotStream() { *this=source; }
  ~InputNavySpotStream();
//  InputNavySpotStream& operator=(const InputNavySpotStream& source);
  int ignore();
  bool open(const char *filename);
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
private:
  std::unique_ptr<unsigned char[]> file_buf;
  int file_buf_len;
  size_t offset;
  std::unique_ptr<char[]> header;
};

class OutputNavySpotStream: public odstream
{
};

#endif
