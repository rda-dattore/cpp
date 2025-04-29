// FILE: td32.h

#ifndef TD32_H
#define  TD32_H

#include <list>
#include <iodstream.hpp>
#include <observation.hpp>
#include <mymap.hpp>

class InputTD32Stream : public idstream
{
public:
  InputTD32Stream() : block(),block_len(0),off(0),curr_offset(0) {}
  void close();
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  off_t current_record_offset() const { return curr_offset; }

private:
  unsigned char block[20000];
  int block_len,off;
  off_t curr_offset;
};

class TD32Data : public Observation
{
public:
  struct Header {
    Header() : type(),elem(),unit() {}

    std::string type,elem,unit;
  };
  struct Report {
    Report() : date_time(),loc(),value(0.),flag1(' '),flag2(' ') {}

    DateTime date_time;
    ObservationLocation loc;
    float value;
    char flag1,flag2;
  };
  struct LibEntry {
    struct Data {
	Data() : start(),end(),lat(),lon() {}

	std::list<DateTime> start,end;
	std::list<float> lat,lon;
    };
    LibEntry() : key(),data(nullptr) {}

    std::string key;
    std::shared_ptr<Data> data;
  };
  TD32Data() : header_(),num_vals(0),reports_() {}
  DateTime date_time() const { return date_time_; }
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  Header header() const { return header_; }
  ObservationLocation location() const { return location_; }
  size_t number_of_values() const { return num_vals; }
  const std::vector<Report>& reports() const { return reports_; }

private:
  static my::map<LibEntry> COOP_table,WBAN_table;
  Header header_;
  size_t num_vals;
  std::vector<Report> reports_;
  void fill_COOP_library();
  void fill_WBAN_library();
};

#endif
