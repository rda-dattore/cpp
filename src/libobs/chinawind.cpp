// FILE: chinawind.cpp
//
#include <iomanip>
#include <pibal.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputChinaWindStream::ignore()
{
return -1;
}

int InputChinaWindStream::peek()
{
return -1;
}

int InputChinaWindStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;

  if (ics != NULL) {
    bytes_read=ics->read(buffer,buffer_length);
    if (bytes_read == craystream::eod)
	bytes_read=ics->read(buffer,buffer_length);
    if (bytes_read < 0)
	bytes_read=bfstream::eof;
  }
  else {
    std::cerr << "Error: no InputChinaWindStream opened" << std::endl;
    exit(1);
  }

  num_read++;
  return bytes_read;
}

size_t ChinaWind::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void ChinaWind::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  short yr,mo,dy,hr;
  int n,off,idum;

  bits::get(stream_buffer,yr,32,32);
  bits::get(stream_buffer,mo,64,32);
  bits::get(stream_buffer,dy,96,32);
  bits::get(stream_buffer,hr,128,32);
  date_time_.set(1900+yr,mo,dy,hr*10000);
  bits::get(stream_buffer,idum,160,32);
  location_.ID=strutils::itos(idum);
  location_.latitude=floatutils::ibmconv(stream_buffer,192);
  location_.longitude=floatutils::ibmconv(stream_buffer,224);
  location_.elevation=floatutils::ibmconv(stream_buffer,256);
  bits::get(stream_buffer,instrument_type,288,32);
  bits::get(stream_buffer,pibal.nlev,320,32);
  if (fill_header_only) {
    return;
  }
  if (static_cast<int>(pibal.capacity) < pibal.nlev) {
    pibal.capacity=pibal.nlev;
    levels.resize(pibal.capacity);
  }
  off=11;
  for (n=0; n < pibal.nlev; ++n) {
    levels[n].height=floatutils::ibmconv(stream_buffer,(off)*32)*1000;
    levels[n].wind_speed=floatutils::ibmconv(stream_buffer,(off+1)*32);
    levels[n].wind_direction=floatutils::ibmconv(stream_buffer,(off+2)*32);
    bits::get(stream_buffer,idum,(off+3)*32,32);
    levels[n].level_quality=static_cast<char>(idum+48);
    off+=4;
  }
}

void ChinaWind::print(std::ostream& outs) const
{
  print_header(outs,true);
  outs.precision(1);
  outs << "  LEVEL Q    HGT    DD      FF" << std::endl;
  for (short n=0; n < pibal.nlev; ++n) {
    outs << std::setw(7) << n+1 << std::setw(2) << levels[n].level_quality << std::setw(7) << levels[n].height << std::setw(6) << levels[n].wind_direction << std::setw(8) << levels[n].wind_speed << std::endl;
  }
  outs << std::endl;
}

void ChinaWind::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);
  if (verbose)
    outs << "  Station: " << location_.ID << "  Date: " << date_time_.to_string() << "Z  Location: " << std::setw(6) << location_.latitude << std::setw(8) << location_.longitude << std::setw(6) << location_.elevation << std::endl;
  else
    outs << "  Stn=" << location_.ID << " Date=" << std::setw(4) << date_time_.year() << std::setfill('0') << std::setw(2) << date_time_.month() << std::setw(2) << date_time_.day() << std::setw(4) << date_time_.time() << std::setfill(' ') << " Lat=" << location_.latitude << " Lon=" << location_.longitude << " Elev=" << location_.elevation << std::endl;
}
