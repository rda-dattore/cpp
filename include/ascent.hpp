// FILE: ascent.h

#ifndef ASCENT_H
#define  ASCENT_H

#include <iodstream.hpp>
#include <observation.hpp>

class InputASCENTStream : public bfstream {
public:
  InputASCENTStream() { }
  void close();
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
  void rewind();

private:
};

class ASCENTData : public Observation {
public:
  ASCENTData() { }
  DateTime date_time() const { return date_time_; }
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  ObservationLocation location() const { return location_; }

private:
};

#endif
