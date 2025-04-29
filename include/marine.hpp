// FILE: marine.h

#ifndef MARINE_H
#define  MARINE_H

#include <iostream>
#include <iodstream.hpp>
#include <surface.hpp>

class MarineObservation : public SurfaceObservation
{
public:
  enum {imma_format=1};
  struct MarineReport {
    MarineReport() : sea_surface_temperature(0.),wave_direction(0.),wave_height(0.),wave_period(0.),swell_direction(0.),swell_height(0.),swell_period(0.) {}

    float sea_surface_temperature,wave_direction,wave_height,wave_period,swell_direction,swell_height,swell_period;
  };
  MarineObservation() : report() {}

protected:
  MarineReport report;
};

class InputIMMAObservationStream : public idstream
{
public:
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);

private:
};

class IMMAObservation : public MarineObservation
{
public:
  struct IMMAPlatformData {
    IMMAPlatformData() : atti(0),code(0) {}

    short atti,code;
  };
  IMMAObservation() : imma_version(-1),ID_type_(0),platform_type_(),attm_ID_list() {}
  std::list<short> attachment_ID_list() const { return attm_ID_list; }
  DateTime date_time() const { return date_time_; }
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  short ID_type() const { return ID_type_; }
  ObservationLocation location() const { return location_; }
  IMMAPlatformData platform_type() const { return platform_type_; }
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;

private:
  short imma_version,ID_type_;
  IMMAPlatformData platform_type_;
  std::list<short> attm_ID_list;
};

class InputNODCBTObservationStream : public idstream
{
public:
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);

private:
};

class NODCBTObservation : public Observation
{
public:
  struct Level {
    Level() : depth(0),t(0.) {}

    int depth;
    float t;
  };
  NODCBTObservation() : data_type_(),institution_code(),num_levels(0),levels() {}
  DateTime date_time() const { return date_time_; }
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  std::string data_type() const { return data_type_; }
  std::string institution() const { return institution_code; }
  Level level(short level_number);
  ObservationLocation location() const { return location_; }
  short number_of_levels() const { return num_levels; }
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;

private:
  std::string data_type_,institution_code;
  short num_levels;
  std::vector<Level> levels;
};

class InputNODCSeaLevelObservationStream : public idstream
{
public:
  static const size_t FILE_BUF_LEN;
  static const size_t RECORD_LEN;

  InputNODCSeaLevelObservationStream() : file_buf(nullptr),id(),lat(),lon(),utc_minutes_offset(),value_offset(0) {}
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);

private:
  std::unique_ptr<char[]> file_buf;
  std::string id,lat,lon,utc_minutes_offset;
  size_t value_offset;
};

class NODCSeaLevelObservation : public Observation
{
public:
  NODCSeaLevelObservation() : sea_level_height_(0),data_format_() {}
  DateTime date_time() const { return date_time_; }
  std::string data_format() const { return data_format_; }
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  ObservationLocation location() const { return location_; }
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;
  int sea_level_height() const { return sea_level_height_; }

private:
  int sea_level_height_;
  std::string data_format_;
};

#endif
