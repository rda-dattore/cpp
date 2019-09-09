// FILE: reanalhdr.h

#ifndef REANALHDR_H
#define   REANALHDR_H

#include <iostream>
#include <aircraft.hpp>
#include <raob.hpp>
#include <datetime.hpp>

class ReanalysisHeader
{
public:
  static const short unknown_id,iwmo_id,iwban_id,icoop_id,iwmo6_id,iwmoext_id,iother_id,acall_id,awmo_id,awban_id,acoop_id,aname_id,aother_id;
  static const char *gateids[46];

  ReanalysisHeader() : stn(0),cid(),fill_bits(0),h_type(0),num_bytes(0),fmt(0),d_type(0),src(0),summ(0),dt_type(0),l_type(0),p_type(0),length(0),date_time(),lon(0.),lat(0.),elev(0) {}
  void fill(const unsigned char *buffer);
  short getDataType() const { return d_type; }
  DateTime getDateTime() const { return date_time; }
  short getDateTimeType() const { return dt_type; }
  int getElevation() const { return elev; }
  short getFormat() const { return fmt; }
  float getLatitude() const { return lat; }
  short getLength() const { return length; }
  short getLocationType() const { return l_type; }
  float getLongitude() const { return lon; }
  short getPlatformIDType() const { return p_type; }
  short getSource() const { return src; }
  long long getNumericStationID() const { return stn; }
  std::string getASCIIStationID() const { return cid; }
  void set(const NavySpotAircraftReport& source);
  void set(const NewZealandAircraftReport& source);
  void set(const DATSAVAircraftReport& source);
  void set(const Tsraob& source);
  void setLocation(float latitude,float longitude,int elevation,short location_type=0);
  void setPlatformIDType(short platform_id) { p_type=platform_id; }
  void setStationID(long long station_id) { stn=station_id; }
  void copyToBuffer(unsigned char *output_buffer,size_t buffer_length) const;
  friend std::ostream& operator<<(std::ostream& out_stream,const ReanalysisHeader& source);

private:
  long long stn;
  std::string cid;
  short fill_bits,h_type,num_bytes,fmt,d_type,src,summ,dt_type,l_type,p_type;
  short length;
  DateTime date_time;
  float lon,lat;
  int elev;
};

#endif
