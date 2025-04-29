// FILE: pibal.h
//

#ifndef PIBAL_H
#define   PIBAL_H

#include <iostream>
#include <vector>
#include <iodstream.hpp>
#include <bfstream.hpp>
#include <observation.hpp>

class Pibal : public Observation
{
public:
  enum {china_format=1001};
  struct PibalLocation {
    PibalLocation() : station(0),latitude(0.),longitude(0.),elevation(0) {}

    size_t station;
    float latitude,longitude;
    int elevation;
  };
  struct PibalLevel {
    PibalLevel() : pressure(0.),wind_speed(0.),height(0),wind_direction(0),type(0),ucomp(0.),vcomp(0.),level_quality(' '),time_quality(' '),pressure_quality(' '),height_quality(' '),wind_quality(' ') {}

    float pressure,wind_speed;
    int height,wind_direction;
    size_t type;
    float ucomp,vcomp;
    char level_quality,time_quality,pressure_quality,height_quality,wind_quality;
  };
  virtual size_t copy_to_buffer(unsigned char *output_buffer,size_t buffer_length) const =0;
  DateTime date_time() const { return date_time_; }
  virtual void fill(const unsigned char *stream_buffer,bool fill_header_only)=0;
  PibalLevel level(short level_number) const;
  ObservationLocation location() const { return location_; }
  short number_of_levels() const { return pibal.nlev; }
  virtual void print(std::ostream& outs) const =0;
  virtual void print_header(std::ostream& outs,bool verbose) const =0;

protected:
  struct {
    size_t capacity;
    short nlev;
  } pibal;
  std::vector<PibalLevel> levels;
};

class InputChinaWindStream : public idstream
{
public:
  InputChinaWindStream() {}
  InputChinaWindStream(const char *filename) { open(filename); }
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
};

class ChinaWind : public Pibal 
{
public:
  size_t copy_to_buffer(unsigned char *output_buffer,size_t buffer_length) const;
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;

private:
  size_t instrument_type;
};

#endif
