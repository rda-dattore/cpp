// FILE: cyclone.h

#ifndef CYCLONE_H
#define   CYCLONE_H

#include <vector>
#include <iodstream.hpp>
#include <datetime.hpp>

class Cyclone
{
public:
  static const bool header_only,full_report;
  struct FixData {
    FixData() : classification(),latitude(0.),longitude(0.),datetime(),max_wind(0.),min_pres(0.) {}

    std::string classification;
    float latitude,longitude;
    DateTime datetime;
    float max_wind,min_pres;
  };
  Cyclone() : ID_(),fix_data_() {}
  virtual ~Cyclone() {}
  virtual void fill(const unsigned char *stream_buffer,bool fill_header_only) =0;
  const std::vector<FixData>& fix_data() const { return fix_data_; }
  std::string ID() const { return ID_; }
  virtual void print(std::ostream& outs) const =0;
  virtual void print_header(std::ostream& outs,bool verbose) const =0;

protected:
  std::string ID_;
  std::vector<FixData> fix_data_;
};

class InputHURDATCycloneStream : public idstream
{
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
};

class HURDATCyclone : public Cyclone
{
public:
  HURDATCyclone() : type_(),sss(0),coast_cross(-1),landfall1(),landfall2(),lat_h(),lon_h() {}
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  char latitude_hemisphere() const { return lat_h; }
  char longitude_hemisphere() const { return lon_h; }
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;
  std::string type() const { return type_; }

private:
  std::string type_;
  short sss,coast_cross;
  DateTime landfall1,landfall2;
  char lat_h,lon_h;
};

class InputTCVitalsCycloneStream : public idstream
{
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
};

class TCVitalsCyclone : public Cyclone
{
public:
  TCVitalsCyclone() : storm_number_(0),basin_ID_(' ') {}
  char basin_ID() const { return basin_ID_; }
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;
  size_t storm_number() const { return storm_number_; }

private:
  size_t storm_number_;
  char basin_ID_;
};

#endif
