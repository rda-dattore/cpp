#include <raob.hpp>
#include <strutils.hpp>
#include <myerror.hpp>

int InputUADBRaobStream::ignore()
{
return bfstream::error;
}

int InputUADBRaobStream::peek()
{
return bfstream::error;
}

int InputUADBRaobStream::read(unsigned char *buffer,size_t buffer_length)
{
  char *cbuf=reinterpret_cast<char *>(const_cast<unsigned char *>(buffer));
  fs.getline(cbuf,104);
  if (fs.eof()) {
    return bfstream::eof;
  }
  else if (fs.gcount() != 103 || cbuf[0] != 'H') {
    return bfstream::error;
  }
  size_t nlev;
  strutils::strget(&cbuf[89],nlev,4);
  int off=103;
  for (size_t n=0; n < nlev; ++n) {
    fs.getline(&cbuf[off],52);
    if (fs.gcount() != 51) {
	myerror="bad line length on level "+strutils::itos(n+1);
	return bfstream::error;
    }
    off+=51;
  }
  cbuf[--off]='\0';
  ++num_read;
  return off;
}

size_t UADBRaob::copy_to_buffer(unsigned char *output_buffer,size_t buffer_length) const
{
return 0;
}

void UADBRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  const char *cbuf=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  std::deque<std::string> sp;
  int n;
  int yr,mo,dy,hr,min,idum;

  sp=strutils::split(cbuf,"\n");
  location_.ID.assign(&cbuf[15],6);
  strutils::trim(location_.ID);
  strutils::strget(&cbuf[22],id_type,2);
  strutils::strget(&cbuf[38],yr,4);
  strutils::strget(&cbuf[43],mo,2);
  strutils::strget(&cbuf[46],dy,2);
  strutils::strget(&cbuf[49],hr,2);
  strutils::strget(&cbuf[51],min,2);
  min=lround(min*60./100.);
  date_time_.set(yr,mo,dy,hr*10000+min*100);
  strutils::strget(&cbuf[57],location_.latitude,5);
  strutils::strget(&cbuf[63],idum,4);
  location_.latitude+=idum/10000.;
  strutils::strget(&cbuf[68],location_.longitude,5);
  strutils::strget(&cbuf[74],idum,4);
  strutils::strget(&cbuf[79],location_.elevation,6);
  location_.longitude+=idum/10000.;
  strutils::strget(&cbuf[86],obs_type,2);
  if (!fill_header_only) {
    for (n=1; n < static_cast<int>(sp.size()); ++n) {
    }
  }
}

void UADBRaob::print(std::ostream& outs) const
{
  print_header(outs,true);
}

void UADBRaob::print_header(std::ostream& outs,bool verbose) const
{
  if (verbose) {
  }
  else {
    outs << " Stn=" << location_.ID << " Type=" << id_type << " Date=" << date_time_.to_string("%Y%m%d%H%MM") << " Lat=" << location_.latitude << " Lon=" << location_.longitude << " Elev=" << location_.elevation << std::endl;
  }
}
