// FILE: tdlhourly.cpp

#include <surface.hpp>
#include <strutils.hpp>

int InputTDLHourlyObservationStream::ignore()
{
return -1;
}

int InputTDLHourlyObservationStream::peek()
{
return -1;
}

int InputTDLHourlyObservationStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;
  if (fs.is_open()) {
    char *buf=reinterpret_cast<char *>(const_cast<unsigned char *>(buffer));
    fs.getline(buf,buffer_length);
    if (fs.eof()) {
	bytes_read=bfstream::eof;
    }
    else if (fs.fail()) {
	bytes_read=bfstream::error;
    }
    else {
	bytes_read=fs.gcount()-1;
    }
  }
  else {
    bytes_read=bfstream::error;
  }
  if (bytes_read != bfstream::eof) {
    ++num_read;
    curr_offset=static_cast<int>(fs.tellg())-bytes_read;
  }
  return bytes_read;
}

void TDLHourlyObservation::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *buf=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  short yr,mo,dy,hr;
  int dum;
  if (buf[107] == '\0') {
    strutils::strget(&buf[0],yr,2);
    yr+=1900;
    strutils::strget(&buf[2],mo,2);
    strutils::strget(&buf[4],dy,2);
    strutils::strget(&buf[6],hr,2);
    date_time_.set(yr,mo,dy,hr*10000);
    location_.ID=std::string(&buf[8],4);
    strutils::strget(&buf[12],dum,5);
    location_.latitude=dum/100.;
    strutils::strget(&buf[17],dum,6);
    location_.longitude=dum/100.;
  }
  else if (buf[150] == '\0') {
    strutils::strget(&buf[0],yr,4);
    strutils::strget(&buf[4],mo,2);
    strutils::strget(&buf[6],dy,2);
    strutils::strget(&buf[8],hr,2);
    date_time_.set(yr,mo,dy,hr*10000);
    location_.ID=std::string(&buf[10],4);
    strutils::strget(&buf[14],stn_type,2);
    strutils::strget(&buf[16],dum,7);
    location_.latitude=dum/10000.;
    strutils::strget(&buf[23],dum,8);
    location_.longitude=dum/10000.;
  }
  else {
    std::cerr << "Error: does not appear to be a TDL Hourly observation" << std::endl;
    exit(1);
  }
}

void TDLHourlyObservation::print(std::ostream& outs) const
{ 
  print_header(outs,true);
}

void TDLHourlyObservation::print_header(std::ostream& outs, bool verbose) const
{
}
