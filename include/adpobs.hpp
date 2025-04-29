// FILE: adpobs.h

#ifndef ADPOBS_H
#define  ADPOBS_H

#include <list>
#include <observation.hpp>
#include <datetime.hpp>

class ADPObservation : public Observation
{
public:
  ADPObservation() : synoptic_date_time_(),receipt_date_time_(),platform_type_(-1),categories_() {}
  std::list<size_t> categories() const { return categories_; }
  DateTime date_time() const { return date_time_; }
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  ObservationLocation location() const { return location_; };
  short platform_type() const { return platform_type_; };
  DateTime receipt_date_time() const { return receipt_date_time_; }
  DateTime synoptic_date_time() const { return synoptic_date_time_; }

private:
  DateTime synoptic_date_time_,receipt_date_time_;
  short platform_type_;
  std::list<size_t> categories_;
};

#endif
