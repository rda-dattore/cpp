// FILE: ascent.h

#ifndef ASCENT_H
#define  ASCENT_H

#include <iodstream.hpp>
#include <observation.hpp>

class InputASCENTStream : public ibfstream {
public:
  InputASCENTStream() : header(), header_len(0) { }
  void close();
  int ignore();
  bool open(std::string filename);
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
  void rewind();

private:
  std::string header;
  size_t header_len;
};

class ASCENTObservation : public Observation {
public:
  ASCENTObservation() : instrument_(), qc_outcome_() { }
  DateTime date_time() const { return date_time_; }
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  std::string instrument() const { return instrument_; }
  ObservationLocation location() const { return location_; }
  std::string qc_outcome() const { return qc_outcome_; }

private:
  std::string instrument_, qc_outcome_;
};

#endif
