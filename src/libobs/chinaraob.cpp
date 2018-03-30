// FILE: chinaraob.cpp
//
#include <iomanip>
#include <raob.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputChinaRaobStream::ignore()
{
return -1;
}

int InputChinaRaobStream::peek()
{
return -1;
}

int InputChinaRaobStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;

  if (icosstream != NULL) {
    bytes_read=icosstream->read(buffer,buffer_length);
    if (bytes_read == craystream::eod)
	bytes_read=icosstream->read(buffer,buffer_length);
    if (bytes_read < 0)
	bytes_read=bfstream::eof;
  }
  else {
    std::cerr << "Error: no InputChinaRaobStream opened" << std::endl;
    exit(1);
  }

  num_read++;
  return bytes_read;
}

size_t ChinaRaob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void ChinaRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int n,off,idum;
  short yr,mo,dy,hr;
  size_t fmt;

  bits::get(stream_buffer,idum,32,32);
  if (idum >= 58 && idum <= 62)
    fmt=1;
  else
    fmt=0;

// 1954-1957 format
  if (fmt == 0) {
    bits::get(stream_buffer,idum,0,32);
    location_.ID=strutils::itos(idum);
    location_.latitude=floatutils::ibmconv(stream_buffer,32);
    location_.longitude=floatutils::ibmconv(stream_buffer,64);
    location_.elevation=floatutils::ibmconv(stream_buffer,96);
    bits::get(stream_buffer,instrument_type,128,32);
    bits::get(stream_buffer,yr,160,32);
    bits::get(stream_buffer,mo,192,32);
    bits::get(stream_buffer,dy,224,32);
    bits::get(stream_buffer,hr,256,32);
    date_time_.set(yr,mo,dy,hr*10000);
    nlev=16;
  }
// 1958-1962 format
  else {
    bits::get(stream_buffer,yr,32,32);
    bits::get(stream_buffer,mo,64,32);
    bits::get(stream_buffer,dy,96,32);
    bits::get(stream_buffer,hr,128,32);
    date_time_.set(yr,mo,dy,hr*10000);
    bits::get(stream_buffer,idum,160,32);
    location_.ID=strutils::itos(idum);
    location_.latitude=floatutils::ibmconv(stream_buffer,192);
    location_.longitude=floatutils::ibmconv(stream_buffer,224);
    location_.elevation=floatutils::ibmconv(stream_buffer,256);
    bits::get(stream_buffer,instrument_type,288,32);
    bits::get(stream_buffer,nlev,320,32);
  }
  if (fill_header_only)
    return;

/*
  if (static_cast<int>(raob.capacity) < nlev) {
    if (raob.capacity > 0)
	delete[] levels;
    raob.capacity=nlev;
    levels=new RaobLevel[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev)
levels.resize(nlev);
  if (fmt == 0) {
    off=288;
    for (n=0; n < nlev; n++) {
	levels[n].pressure=floatutils::ibmconv(stream_buffer,off);
	bits::get(stream_buffer,idum,off+32,16);
	levels[n].pressure_quality=static_cast<char>(idum+48);
	levels[n].height=floatutils::ibmconv(stream_buffer,off+48);
	bits::get(stream_buffer,idum,off+80,16);
	levels[n].height_quality=static_cast<char>(idum+48);
	levels[n].temperature=floatutils::ibmconv(stream_buffer,off+96);
	bits::get(stream_buffer,idum,off+128,16);
	levels[n].temperature_quality=static_cast<char>(idum+48);
	levels[n].moisture=floatutils::ibmconv(stream_buffer,off+144);
	bits::get(stream_buffer,idum,off+176,16);
	levels[n].moisture_quality=static_cast<char>(idum+48);
	levels[n].level_quality='9';
	off+=192;
    }
  }
  else {
    off=11;
    for (n=0; n < nlev; n++) {
	bits::get(stream_buffer,idum,off*32,32);
	levels[n].pressure=idum;
	levels[n].height=floatutils::ibmconv(stream_buffer,(off+1)*32);
	levels[n].temperature=floatutils::ibmconv(stream_buffer,(off+2)*32);
	levels[n].moisture=floatutils::ibmconv(stream_buffer,(off+3)*32);
	bits::get(stream_buffer,idum,(off+4)*32,32);
	levels[n].level_quality=static_cast<char>(idum+48);
	off+=5;
    }
  }
}

void ChinaRaob::print(std::ostream& outs) const
{
  print_header(outs,true);
  outs.precision(1);
  if (levels[0].level_quality == '9') {
    outs << "  LEVEL    PRES Q   HGT Q    TEMP Q    MOIS Q" << std::endl;
  }
  else {
    outs << "  LEVEL Q    PRES   HGT    TEMP    MOIS" << std::endl;
  }
  for (short n=0; n < nlev; ++n) {
    if (levels[0].level_quality == '9')
	outs << std::setw(7) << n+1 << std::setw(8) << levels[n].pressure << std::setw(2) << levels[n].pressure_quality << std::setw(6) << levels[n].height << std::setw(2) << levels[n].height_quality << std::setw(8) << levels[n].temperature << std::setw(2) << levels[n].temperature_quality << std::setw(8) << levels[n].moisture << std::setw(2) << levels[n].moisture_quality << std::endl;
    else
	outs << std::setw(7) << n+1 << std::setw(2) << levels[n].level_quality << std::setw(8) << levels[n].pressure << std::setw(6) << levels[n].height << std::setw(8) << levels[n].temperature << std::setw(8) << levels[n].moisture << std::endl;
  }
  outs << std::endl;
}

void ChinaRaob::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);
  if (verbose)
    outs << "  Station: " << location_.ID << "  Date: " << date_time_.to_string() << "Z  Location: " << std::setw(6) << location_.latitude << std::setw(8) << location_.longitude << std::setw(6) << location_.elevation << std::endl;
  else
    outs << "  Stn=" << location_.ID << " Date=" << std::setw(4) << date_time_.year() << std::setfill('0') << std::setw(2) << date_time_.month() << std::setw(2) << date_time_.day() << std::setw(4) << date_time_.time() << std::setfill(' ') << " Lat=" << location_.latitude << " Lon=" << location_.longitude << " Elev=" << location_.elevation << std::endl;
}
