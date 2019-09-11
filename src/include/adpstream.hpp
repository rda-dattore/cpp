// FILE: adpstream.h

#ifndef ADPSTREAM_H
#define   ADPSTREAM_H

#include <iodstream.hpp>

class InputADPStream : public idstream
{
public:
  InputADPStream() : file_buf(nullptr),date{' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},file_buf_len(0),file_buf_pos(0),is_ascii(false) {}
  InputADPStream(const char *filename) : InputADPStream() { open(filename); }
//  InputADPStream(const InputADPStream& source) : InputADPStream() { *this=source; }
//  InputADPStream& operator=(const InputADPStream& source);
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);

private:
  int read_from_disk();

  std::unique_ptr<unsigned char[]> file_buf;
  unsigned char date[10];
  size_t file_buf_len,file_buf_pos;
  bool is_ascii;
};

class OutputADPStream: public odstream
{
};

#endif
