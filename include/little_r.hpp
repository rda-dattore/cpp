// FILE: little_r.hpp

#ifndef LITTLE_R_H
#define LITTLE_R_H

#include <vector>
#include <iodstream.hpp>
#include <observation.hpp>

class InputLITTLE_RStream : public idstream
{
public:
  InputLITTLE_RStream() {}
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
};

class LITTLE_RObservation : public Observation
{
public:
  LITTLE_RObservation() : platform_code_(0),data_records_(),is_sounding_(false) {}
  const std::vector<std::tuple<float,int,float,int,float,int,float,int,float,int,float,int,float,int,float,int,float,int,float,int>>& data_records() const { return data_records_; }
  DateTime date_time() const;
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  ObservationLocation location() const;
  int num_data_records() const { return data_records_.size(); }
  short platform_code() const { return platform_code_; }
  bool is_sounding() const { return is_sounding_; }

protected:
  short platform_code_;
  std::vector<std::tuple<float,int,float,int,float,int,float,int,float,int,float,int,float,int,float,int,float,int,float,int>> data_records_;
  bool is_sounding_;
};

#endif
