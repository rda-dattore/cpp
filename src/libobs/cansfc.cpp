// FILE: cansfc.cpp

#include <iomanip>
#include <surface.hpp>
#include <strutils.hpp>
#include <bits.hpp>

int InputDSSCanadianSurfaceHourlyObservationStream::ignore()
{
return 0;
}

int InputDSSCanadianSurfaceHourlyObservationStream::peek()
{
return 0;
}

int InputDSSCanadianSurfaceHourlyObservationStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;

  if (irptstream != NULL)
    bytes_read=irptstream->read(buffer,buffer_length);
  else
    bytes_read=bfstream::error;
  if (bytes_read != bfstream::eof)
    num_read++;
  return bytes_read;
}

void DSSCanadianSurfaceHourlyObservation::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int dum;
  short yr,mo,dy,hr,tz;

  bits::get(stream_buffer,dum,16,24);
  location_.ID=strutils::ftos(dum,7,0,'0');
  bits::get(stream_buffer,yr,40,12);
  bits::get(stream_buffer,mo,52,4);
  bits::get(stream_buffer,dy,56,5);
  bits::get(stream_buffer,hr,61,5);
  bits::get(stream_buffer,tz,66,4);
  tz-=8;
  date_time_.set(yr,mo,dy,(hr-tz)*10000);
  bits::get(stream_buffer,dum,70,14);
  location_.latitude=dum/100.;
  bits::get(stream_buffer,dum,84,16);
  location_.longitude=(dum-18000)/100.;
  bits::get(stream_buffer,location_.elevation,100,14);
}

void DSSCanadianSurfaceHourlyObservation::print(std::ostream& outs) const
{
  print_header(outs,true);
}

void DSSCanadianSurfaceHourlyObservation::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);

  if (verbose)
    outs << "ID: " << location_.ID << "  Date: " << date_time_.to_string() << "Z  Location: " << std::setw(6) << location_.latitude << std::setw(8) << location_.longitude << std::setw(6) << location_.elevation << std::endl;
  else {
  }
}
