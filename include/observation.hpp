// FILE: observation.h

#ifndef OBSERVATION_H
#define   OBSERVATION_H

#include <datetime.hpp>

class Observation {
public:
  static const bool header_only,full_report;
  struct ObservationLocation {
    ObservationLocation() : ID(),latitude(0.),longitude(0.),elevation(0) {}

    std::string ID;
    float latitude,longitude;
    union {
	int elevation;
	int altitude;
    };
  };
  Observation() : date_time_(),location_() {}
  virtual ~Observation() {}
  virtual void fill(const unsigned char *stream_buffer,bool fill_header_only) =0;
  virtual DateTime date_time() const =0;
  virtual ObservationLocation location() const =0;

protected:
  DateTime date_time_;
  ObservationLocation location_;
};

extern bool operator==(const Observation::ObservationLocation& loc1,const Observation::ObservationLocation& loc2);
extern bool operator!=(const Observation::ObservationLocation& loc1,const Observation::ObservationLocation& loc2);

#endif
