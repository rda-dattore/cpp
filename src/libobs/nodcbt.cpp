#include <iomanip>
#include <complex>
#include <marine.hpp>
#include <strutils.hpp>

int InputNODCBTObservationStream::ignore()
{
return -1;
}

int InputNODCBTObservationStream::peek()
{
return -1;
}

int InputNODCBTObservationStream::read(unsigned char *buffer,size_t buffer_length)
{
  int num_bytes;
  if (icosstream != nullptr) {
    if ( (num_bytes=icosstream->read(buffer,buffer_length)) < 0) {
	return num_bytes;
    }
  }
  else {
    char *b=reinterpret_cast<char *>(const_cast<unsigned char *>(buffer));
    fs.getline(b,buffer_length);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (fs.fail()) {
	return bfstream::error;
    }
    num_bytes=fs.gcount()-1;
  }
  ++num_read;
  return num_bytes;
}

void NODCBTObservation::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *buffer=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  short deg,min;
  short yr,mo,dy,time;
  int n,dum;
  size_t off;

  data_type_.assign(buffer,2);
  strutils::strget(&buffer[3],deg,2);
  strutils::strget(&buffer[5],min,3);
  location_.latitude=deg+min/600.;
  location_.latitude=lroundf(location_.latitude*1000.)/1000.;
  if (buffer[8] == 'S') {
    location_.latitude=-location_.latitude;
  }
  strutils::strget(&buffer[10],deg,3);
  strutils::strget(&buffer[13],min,3);
  location_.longitude=deg+min/600.;
  location_.longitude=lroundf(location_.longitude*1000.)/1000.;
  if (buffer[16] == 'W') {
    location_.longitude=-location_.longitude;
  }
  strutils::strget(&buffer[18],yr,2);
  if (yr < 30) {
    yr+=2000;
  }
  else {
    yr+=1900;
  }
  strutils::strget(&buffer[20],mo,2);
  strutils::strget(&buffer[22],dy,2);
  strutils::strget(&buffer[24],time,4);
  date_time_.set(yr,mo,dy,time*100);
  location_.ID=std::string(&buffer[44],2)+std::string(&buffer[48],2);
  institution_code=std::string(&buffer[46],2);
  strutils::strget(&buffer[95],num_levels,4);
  levels.resize(num_levels);
  off=100;
  for (n=0; n < num_levels; ++n) {
    strutils::strget(&buffer[off],levels[n].depth,4);
    strutils::strget(&buffer[off+4],dum,4);
    levels[n].t=dum/100.;
    off+=8;
  }
}

NODCBTObservation::Level NODCBTObservation::level(short level_number)
{
  if (static_cast<int>(level_number) >= num_levels) {
    NODCBTObservation::Level empty;
    return empty;
  }
  else {
    return levels[level_number];
  }
}

void NODCBTObservation::print(std::ostream& outs) const
{
  int n;

  print_header(outs,true);
  outs.setf(std::ios::fixed);
  outs.precision(2);
  outs << "  LEVEL  DEPTH (M)  TEMP (C)" << std::endl;
  for (n=0; n < num_levels; ++n) {
    outs << std::setw(7) << n+1 << std::setw(7) << levels[n].depth << std::setw(10) << levels[n].t << std::endl;
  }
}

void NODCBTObservation::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(3);
  outs << date_time_.to_string() << " " << location_.ID << " " << location_.latitude << " " << location_.longitude << std::endl;
}
