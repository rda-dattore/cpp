// FILE: aircraft.h

#ifndef AIRCRAFT_H
#define   AIRCRAFT_H

#include <iodstream.hpp>
#include <datetime.hpp>
#include <observation.hpp>

class AircraftReport : public Observation
{
public:
  static const bool header_only,full_report;
  enum {adp_format=1,nz_format,navyspot_format,datsav_format};
  enum class MoistureType {_NULL=0,DEW_POINT_TEMPERATURE,DEW_POINT_DEPRESSION,RELATIVE_HUMIDITY,SPECIFIC_HUMIDITY,WET_BULB_DEPRESSION};

  AircraftReport() : synoptic_date_time_(),data() {}
  virtual ~AircraftReport() {}
  DateTime date_time() const { return date_time_; }
  virtual void fill(const unsigned char *stream_buffer,bool fill_header_only) =0;
  ObservationLocation location() const { return location_; }
  float moisture() const { return data.moisture; }
  float pressure() const { return data.pressure; }
  DateTime synoptic_date_time() const { return synoptic_date_time_; }
  float temperature() const { return data.temperature; }
  size_t wind_direction() const { return data.wind_direction; }
  float wind_speed() const { return data.wind_speed; }
  virtual void print() const =0;
  virtual void print_header() const =0;

protected:
  DateTime synoptic_date_time_;
  struct Data {
    Data() : pressure(0.),height(0.),temperature(0.),moisture(0.),wind_speed(0.),wind_direction(0),moisture_indicator(MoistureType::_NULL) {}

    float pressure,height,temperature,moisture,wind_speed;
    size_t wind_direction;
    MoistureType moisture_indicator;
  } data;
};

class ADPAircraftReport : public AircraftReport
{
public:
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print() const;
  void print_header() const;
};

class NewZealandAircraftReport : public AircraftReport
{
public:
  NewZealandAircraftReport() : qual(0) {}
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print() const;
  void print_header() const;

private:
  short qual;
};

class NavySpotAircraftReport : public AircraftReport
{
public:
  NavySpotAircraftReport() : navy() {}
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print() const;
  void print_header() const;

  friend bool operator==(const NavySpotAircraftReport& a,const NavySpotAircraftReport& b);
  friend bool operator!=(const NavySpotAircraftReport& a,const NavySpotAircraftReport& b) { return !(a == b); }

private:
  struct Navy {
    Navy() : type(0) {}

    size_t type;
  } navy;
};

class InputDATSAVAircraftReportStream : public idstream
{
public:
  InputDATSAVAircraftReportStream() : file_buf(new unsigned char[9994]),file_buf_pos(0),file_buf_len(0) {}
  void close();
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);

private:
  std::unique_ptr<unsigned char[]> file_buf;
  int file_buf_pos,file_buf_len;
};

class OutputDATSAVAircraftReportStream : public odstream
{
public:
  OutputDATSAVAircraftReportStream() : file_buf(new unsigned char[9994]),file_buf_pos(4) {}
  void close();
  int write(const unsigned char *buffer,size_t num_bytes);

private:
  std::unique_ptr<unsigned char[]> file_buf;
  size_t file_buf_pos;
};

class DATSAVAircraftReport : public AircraftReport
{
public:
  DATSAVAircraftReport() : datsav() {}
  int altitude_dvalue() const { return datsav.alt_dval; }
  int altitude_of_mandatory_pressure_level() const { return datsav.alt_pl; }
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print() const;
  void print_header() const;

private:
  struct DATSAV {
    DATSAV() : rpt_type(),ob_type(),alt_dval(0),alt_pl(0),turbulence(),sfc_p(0),altim(0),hdg(0),sfc_dd(0),sfc_ff(0) {}

    short rpt_type,ob_type;
    int alt_dval,alt_pl;
    struct {
	short frequency,intensity,type;
    } turbulence;
    size_t sfc_p,altim,hdg,sfc_dd,sfc_ff;
  } datsav;
};

class InputTDF57AircraftReportStream : public idstream
{
public:
  InputTDF57AircraftReportStream() {}
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);

private:
};

class TDF57AircraftReport : public AircraftReport
{
public:
  TDF57AircraftReport() {}
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print() const;
  void print_header() const;

private:
};

#endif
