// FILE: rushipraob.h

#include <iomanip>
#include <raob.hpp>
#include <bits.hpp>
#include <utils.hpp>

int InputRussianShipRaobStream::ignore()
{
return 0;
}

int InputRussianShipRaobStream::peek()
{
return 0;
}

int InputRussianShipRaobStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;

  if (!is_open()) {
    std::cerr << "Error: no InputRussianShipRaobStream has been opened" << std::endl;
    exit(1);
  }
  if (ics != nullptr) {
    bytes_read=ics->read(buffer,buffer_length);
    if (bytes_read == craystream::eod) {
	bytes_read=bfstream::eof;
    }
  }
  else if (ivs != nullptr) {
    bytes_read=ivs->read(buffer,buffer_length);
  }
  else {
    bytes_read=bfstream::error;
  }
  if (bytes_read != bfstream::eof) {
    ++num_read;
  }
  return bytes_read;
}

size_t RussianShipRaob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void RussianShipRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int n,off;
  short yr,mo,dy,hr;
  int dum;
  char cdum[4];

  bits::get(stream_buffer,yr,40,16);
  bits::get(stream_buffer,mo,56,8);
  bits::get(stream_buffer,dy,64,8);
  bits::get(stream_buffer,hr,72,8);
  date_time_.set(yr,mo,dy,hr*10000);
  bits::get(stream_buffer,octant,80,8);
  bits::get(stream_buffer,dum,88,16);
  if (dum > 900) {
    location_.latitude=-99.9;
  }
  else {
    location_.latitude=dum/10.;
    if (octant > 4) {
	location_.latitude=-location_.latitude;
    }
  }
  bits::get(stream_buffer,dum,104,16);
  if (dum > 1800) {
    location_.longitude=-199.9;
  }
  else {
    location_.longitude=dum/10.;
    if ( (octant % 5) < 2) {
	location_.longitude=-location_.longitude;
    }
  }
  location_.elevation=-999;
  bits::get(stream_buffer,cdum,120,8,0,4);
  charconversions::ebcdic_to_ascii(cdum,cdum,4);
  bits::set(&call_sign,reinterpret_cast<unsigned char *>(cdum),0,8,0,4);
  for (n=0; n < 4; n++) {
    if (cdum[n] < 48 || cdum[n] > 90)
	cdum[n]=47;
  }
  location_.ID.assign(cdum,4);
  bits::get(stream_buffer,nlev,168,8);
  if (fill_header_only)
    return;
/*
  if (nlev > static_cast<int>(raob.capacity)) {
    if (levels != NULL)
	delete[] levels;
    if (wind_quality2 != NULL)
	delete[] wind_quality2;
    levels=new RaobLevel[nlev];
    wind_quality2=new char[nlev];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
wind_quality2.resize(nlev);
}
  off=256;
  for (n=0; n < nlev; ++n) {
    bits::get(stream_buffer,levels[n].type,off,8);
    if (levels[n].type == 0xff)
	levels[n].type=99;
    bits::get(stream_buffer,dum,off+8,16);
    if (dum == 0x8000)
	levels[n].pressure=999.9;
    else
	levels[n].pressure=dum/10.;
    bits::get(stream_buffer,levels[n].pressure_quality,off+24,8);
    bits::get(stream_buffer,levels[n].height,off+32,24);
    if (levels[n].height == 0x800000)
	levels[n].height=99999;
    else if (levels[n].height > 0x800000)
	levels[n].height=levels[n].height-0x1000000;
    bits::get(stream_buffer,levels[n].height_quality,off+56,8);
    bits::get(stream_buffer,dum,off+64,16);
    if (dum == 0x8000)
	levels[n].temperature=-99.9;
    else {
	if (dum > 0x8000)
	  dum=dum-0x10000;
	levels[n].temperature=dum/10.;
    }
    bits::get(stream_buffer,levels[n].temperature_quality,off+80,8);
    bits::get(stream_buffer,dum,off+88,16);
    if (dum == 0x8000 || levels[n].temperature < -99.)
	levels[n].moisture=-99.9;
    else {
	if (dum > 0x8000)
	  dum=dum-0x10000;
	levels[n].moisture=levels[n].temperature-dum/10.;
    }
    bits::get(stream_buffer,levels[n].moisture_quality,off+104,8);
    bits::get(stream_buffer,levels[n].wind_direction,off+112,16);
    if (levels[n].wind_direction == 0x8000)
	levels[n].wind_direction=999;
    bits::get(stream_buffer,levels[n].wind_quality,off+128,8);
    bits::get(stream_buffer,dum,off+136,8);
    if (dum == 0x80)
	levels[n].wind_speed=999.;
    else
	levels[n].wind_speed=dum;
    bits::get(stream_buffer,wind_quality2[n],off+144,8);
    off+=152;
  }
}

void RussianShipRaob::print(std::ostream& outs) const
{
  int n;

  print_header(outs,true);
  outs << "                                                        ------- Wind -------" << std::endl;
  outs << "Level  Pressure Q  Height Q  Temperature Q  Dewpoint Q  Direction Q   Speed Q" << std::endl;
  for (n=0; n < nlev; ++n) {
    outs << std::setw(5) << n << std::setw(10) << levels[n].pressure << std::setw(2) << static_cast<int>(levels[n].pressure_quality) << std::setw(8) << levels[n].height << std::setw(2) << static_cast<int>(levels[n].height_quality) << std::setw(13) << levels[n].temperature << std::setw(2) << static_cast<int>(levels[n].temperature_quality) << std::setw(10) << levels[n].moisture << std::setw(2) << static_cast<int>(levels[n].moisture_quality) << std::setw(11) << levels[n].wind_direction << std::setw(2) << static_cast<int>(levels[n].wind_quality) << std::setw(8) << levels[n].wind_speed << std::setw(2) << static_cast<int>(wind_quality2[n]) << std::endl;
  }
}

void RussianShipRaob::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (verbose)
    outs << "ID: " << location_.ID << "(" << std::setw(11) << call_sign << ")  Date: " << date_time_.to_string() << "Z  Location: " << std::setw(2) << octant << std::setw(6) << location_.latitude << std::setw(8) << location_.longitude << std::setw(6) << location_.elevation << "  Number of Levels: " << std::setw(3) << nlev << std::endl;
  else {
  }
}
