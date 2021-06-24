// FILE: mitraob.cpp
//
#include <iomanip>
#include <raob.hpp>
#include <bsort.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputMITRaobStream::ignore()
{
return -1;
}

int InputMITRaobStream::peek()
{
return -1;
}

int InputMITRaobStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;

  if (ics != NULL) {
    bytes_read=ics->read(buffer,buffer_length);
    if (bytes_read == craystream::eod)
	bytes_read=ics->read(buffer,buffer_length);
    if (bytes_read < 0)
	bytes_read=bfstream::eof;
  }
  else if (irs != NULL)
    bytes_read=irs->read(buffer,buffer_length);
  else {
    std::cerr << "Error: no InputMITRaobStream opened" << std::endl;
    exit(1);
  }

  num_read++;
  return bytes_read;
}

size_t MITRaob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

int compareMITLevels(Raob::RaobLevel& left,Raob::RaobLevel& right)
{
  if (left.pressure >= right.pressure)
    return -1;
  else
    return 1;
}
void MITRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  size_t check;
  size_t yr,mo,dy,time;
  char s[8];
  int dum,n;
  size_t off;
  size_t cnt=0;
  bool missingLevel,unsorted;
  float plist[]={0.,1000.,950.,900.,850.,800.,750.,700.,650.,600.,550.,500.,450.,400.,350.,300.,250.,200.,150.,100.,70.,50.,30.,20.,15.,10.,7.};

  nlev=999;
  bits::get(stream_buffer,check,0,16);
// IBM-word format
  if (check == 0) {
    bits::get(stream_buffer,yr,0,32);
    bits::get(stream_buffer,mo,32,32);
    bits::get(stream_buffer,dy,64,32);
    bits::get(stream_buffer,time,96,32);
    bits::get(stream_buffer,s,160,8,0,8);
    charconversions::ebcdic_to_ascii(s,s,8);
    location_.ID.assign(s,8);
    location_.latitude=floatutils::ibmconv(stream_buffer,224);
    location_.longitude=floatutils::ibmconv(stream_buffer,256);
    location_.elevation=lround(floatutils::ibmconv(stream_buffer,288));
    if (!fill_header_only) {
/*
	if (raob.capacity < 16) {
	  if (raob.capacity > 0)
	    delete[] levels;
	  raob.capacity=16;
	  levels=new RaobLevel[raob.capacity];
	}
*/
if (levels.capacity() < 16)
levels.resize(16);
	off=352;
	unsorted=false;
//	for (n=0; n < static_cast<int>(raob.capacity); n++) {
for (n=0; n < 16; n++) {
	  missingLevel=true;
	  levels[cnt].pressure=floatutils::ibmconv(stream_buffer,off);
	  bits::get(stream_buffer,levels[cnt].pressure_quality,off+40,8);
	  levels[cnt].ucomp=floatutils::ibmconv(stream_buffer,off+48);
	  if (levels[cnt].ucomp > -999.)
	    missingLevel=false;
	  bits::get(stream_buffer,levels[cnt].wind_quality,off+88,8);
	  levels[cnt].vcomp=floatutils::ibmconv(stream_buffer,off+96);
	  if (levels[cnt].vcomp > -999.)
	    missingLevel=false;
// use time_quality to hold the quality for the v-component
	  bits::get(stream_buffer,levels[cnt].time_quality,off+136,8);
	  levels[cnt].height=lround(floatutils::ibmconv(stream_buffer,off+144));
	  bits::get(stream_buffer,levels[cnt].height_quality,off+184,8);
	  levels[cnt].temperature=floatutils::ibmconv(stream_buffer,off+192);
	  if (levels[cnt].temperature > -999.)
	    missingLevel=false;
	  bits::get(stream_buffer,levels[cnt].temperature_quality,off+232,8);
	  levels[cnt].moisture=floatutils::ibmconv(stream_buffer,off+240);
	  if (levels[cnt].moisture > -999.)
	    missingLevel=false;
	  bits::get(stream_buffer,levels[cnt].moisture_quality,off+280,8);
	  if (missingLevel && levels[cnt].pressure > -999. && levels[cnt].height > -999)
	    missingLevel=false;
	  if (!missingLevel) {
	    if (cnt > 0 && levels[cnt].pressure > levels[cnt-1].pressure)
		unsorted=true;
	    cnt++;
	  }
	  off+=288;
	}
	nlev=cnt;
	if (unsorted)
//	  binary_sort(levels,nlev,
binary_sort(levels,
	  [](Raob::RaobLevel& left,Raob::RaobLevel& right) -> bool
	  {
	    if (left.pressure >= right.pressure) {
		return true;
	    }
	    else {
		return false;
	    }
	  },0,nlev);
    }
  }
// rptout format
  else {
    bits::get(stream_buffer,dum,16,10);
    location_.ID=strutils::ftos(dum,3,0,'0');
    bits::get(stream_buffer,yr,26,7);
    bits::get(stream_buffer,mo,33,4);
    bits::get(stream_buffer,dy,37,5);
    bits::get(stream_buffer,time,42,5);
    location_.latitude=-99.9;
    location_.longitude=-199.9;
    location_.elevation=-999;
    bits::get(stream_buffer,nlev,53,6);
    if (!fill_header_only) {
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
	off=64;
	for (n=0; n < nlev; n++) {
	  levels[n].pressure=plist[n];
	  levels[n].pressure_quality=0;
	  bits::get(stream_buffer,levels[n].height,off,16);
	  if (levels[n].height == 0xffff)
	    levels[n].height=-9999;
	  else
	    levels[n].height-=1000;
	  bits::get(stream_buffer,dum,off+16,13);
	  if (dum == 0x1fff)
	    levels[n].ucomp=-999.;
	  else
	    levels[n].ucomp=(dum-0x1000)/10.;
	  bits::get(stream_buffer,dum,off+29,13);
	  if (dum == 0x1fff)
	    levels[n].vcomp=-999.;
	  else
	    levels[n].vcomp=(dum-0x1000)/10.;
	  bits::get(stream_buffer,dum,off+42,11);
	  if (dum == 0x7ff)
	    levels[n].temperature=-999.;
	  else
	    levels[n].temperature=(dum+1500)/10.-273.2;
	  bits::get(stream_buffer,dum,off+53,10);
	  if (dum == 0x3ff)
	    levels[n].moisture=-999.;
	  else
	    levels[n].moisture=dum/10.;
	  bits::get(stream_buffer,levels[n].height_quality,off+63,2);
	  bits::get(stream_buffer,levels[n].wind_quality,off+65,2);
	  levels[n].time_quality=levels[n].wind_quality;
	  bits::get(stream_buffer,levels[n].temperature_quality,off+67,2);
	  bits::get(stream_buffer,levels[n].moisture_quality,off+69,2);
	  off+=71;
	}
	levels[0].pressure=levels[0].height+1000;
	levels[0].height=-9999;
    }
  }
  date_time_.set(1900+yr,mo,dy,time*10000);
}

void MITRaob::print(std::ostream& outs) const
{
  int n;

  print_header(outs,true);
  outs.precision(1);
  outs << "  LEVEL    PRES Q   HGT Q    TEMP Q    MOIS Q   UCOMP Q   VCOMP Q" << std::endl;
  for (n=0; n < nlev; ++n) {
    outs << std::setw(7) << n+1 << std::setw(8) << levels[n].pressure << std::setw(2) << static_cast<int>(levels[n].pressure_quality) << std::setw(6) << levels[n].height << std::setw(2) << static_cast<int>(levels[n].height_quality) << std::setw(8) << levels[n].temperature << std::setw(2) << static_cast<int>(levels[n].temperature_quality) << std::setw(8) << levels[n].moisture << std::setw(2) << static_cast<int>(levels[n].moisture_quality) << std::setw(8) << levels[n].ucomp << std::setw(2) << static_cast<int>(levels[n].wind_quality) << std::setw(8) << levels[n].vcomp << std::setw(2) << static_cast<int>(levels[n].time_quality) << std::endl;
  }
  outs << std::endl;
}

void MITRaob::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);
  if (verbose) {
    outs << "  Station: " << location_.ID << "  Date: " << date_time_.to_string() << "Z  Location: " << std::setw(6) << location_.latitude << std::setw(8) << location_.longitude << std::setw(6) << location_.elevation << "  Number of Levels: " << nlev << std::endl;
  }
  else {
    outs << "  Stn=" << location_.ID << " Date=" << std::setw(4) << date_time_.year() << std::setfill('0') << std::setw(2) << date_time_.month() << std::setw(2) << date_time_.day() << std::setw(4) << date_time_.time() << std::setfill(' ') << " Lat=" << location_.latitude << " Lon=" << location_.longitude << " Elev=" << location_.elevation << std::endl;
  }
}
