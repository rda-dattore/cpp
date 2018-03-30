#include <iomanip>
#include <marine.hpp>
#include <strutils.hpp>
#include <myerror.hpp>

int InputIMMAObservationStream::ignore()
{
return -1;
}

int InputIMMAObservationStream::peek()
{
return -1;
}

int InputIMMAObservationStream::read(unsigned char *buffer,size_t buffer_length)
{
  int numBytes;
  char *b=reinterpret_cast<char *>(const_cast<unsigned char *>(buffer));

  if (fs.is_open()) {
    fs.getline(b,buffer_length);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (fs.fail()) {
	return bfstream::error;
    }
    numBytes=fs.gcount()-1;
  }
  else {
    myerror="NULL file pointer";
    return bfstream::error;
  }
  if (numBytes < 108) {
    myerror="short record - "+strutils::itos(numBytes)+" bytes";
    return bfstream::error;
  }
  else {
    ++num_read;
    return numBytes;
  }
}

void IMMAObservation::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *buffer=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  short yr,mo,dy,hr,min;
  std::string sdum;
  size_t off,len;
  short attm_ID,attm_len;

  platform_type_.atti=0;
  attm_ID_list.clear();
  strutils::strget(buffer,yr,4);
  strutils::strget(&buffer[4],mo,2);
  strutils::strget(&buffer[6],dy,2);
  sdum.assign(&buffer[8],4);
  if (sdum == "    ") {
    hr=min=99;
  }
  else {
    strutils::strget(&buffer[8],hr,2);
    strutils::strget(&buffer[10],min,2);
    min=lround(min*0.6);
  }
  date_time_.set(yr,mo,dy,hr*10000+min*100);
  strutils::strget(&buffer[12],location_.latitude,5);
  location_.latitude/=100.;
  strutils::strget(&buffer[17],location_.longitude,6);
  location_.longitude/=100.;
  if (location_.longitude >= 180. && location_.longitude < 360.) {
    location_.longitude-=360.;
  }
  strutils::strget(&buffer[23],imma_version,2);
  strutils::strget(&buffer[32],ID_type_,2);
  location_.ID.assign(&buffer[34],9);
  strutils::trim(location_.ID);
  if (fill_header_only) {
    return;
  }
  off=108;
  len=std::string(buffer).length();
  while (off < len) {
    strutils::strget(&buffer[off],attm_ID,2);
    attm_ID_list.push_back(attm_ID);
    strutils::strget(&buffer[off+2],attm_len,2);
    switch (attm_ID) {
	case 1:
	{
	  if (platform_type_.atti == 0 || (platform_type_.atti == 2 && platform_type_.code == 0)) {
	    platform_type_.atti=1;
	    strutils::strget(&buffer[off+16],platform_type_.code,2);
	  }
	  break;
	}
	case 2:
	{
	  if (platform_type_.atti == 0) {
	    platform_type_.atti=2;
	    strutils::strget(&buffer[off+5],platform_type_.code,1);
	  }
	  break;
	}
	default:
	{
	  off=len;
	  break;
	}
    }
    off+=attm_len;
  }
}

void IMMAObservation::print(std::ostream& outs) const
{
  print_header(outs,true);
  outs.setf(std::ios::fixed);
  outs.precision(2);
}

void IMMAObservation::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);
  outs << "Date=" << date_time_.to_string("%Y-%m-%dT%H:%MM") << " IDTyp=" << ID_type_ << " ID=" << location_.ID;
  if (platform_type_.atti > 0) {
    outs << " PlatTyp=" << platform_type_.atti << "/" << platform_type_.code;
  }
  outs << " Lat=" << location_.latitude << " Lon=" << location_.longitude;
  if (attm_ID_list.size() > 0) {
    outs << " Attms=" << attm_ID_list.size();
  }
  outs << std::endl;
}
