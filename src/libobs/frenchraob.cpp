#include <iomanip>
#include <bfstream.hpp>
#include <raob.hpp>
#include <strutils.hpp>
#include <utils.hpp>

void InputFrenchRaobStream::read_from_disk()
{
  if (ics != NULL) {
    file_buf_len=ics->read(file_buf.get(),block_size);
  }
  else {
    fs.read(reinterpret_cast<char *>(file_buf.get()),block_size);
    if (fs.eof()) {
	file_buf_len=bfstream::eof;
    }
    else {
	file_buf_len=fs.gcount();
    }
  }
  if (file_buf_len > 0) {
    charconversions::ebcdic_to_ascii(reinterpret_cast<char *>(file_buf.get()),reinterpret_cast<char *>(file_buf.get()),file_buf_len);
  }
  if (file_buf_len == bfstream::eof) {
    file_buf_pos=file_buf_len;
  }
  else {
    file_buf_pos=0;
  }
}

void InputFrenchRaobStream::close()
{
  idstream::close();
  file_buf=nullptr;
}

int InputFrenchRaobStream::ignore()
{
return bfstream::error;
}

bool InputFrenchRaobStream::open(const char *filename)
{
  if (!idstream::open(filename)) {
    return false;
  }
// check for the type of observation, as this determines the block size
  unsigned char test;
  if (ics != NULL) {
    ics->read(&test,1);
  }
  else {
    fs.read(reinterpret_cast<char *>(&test),1);
  }
// rawindsonde is 'A' - EBCDIC 0xc1
  if (test == 0xc1) {
    block_size=3668;
    file_buf.reset(new unsigned char[block_size]);
    idstream::rewind();
    read_from_disk();
    if (file_buf_len < 0) {
	return false;
    }
  }
// radiosonde is 'C' - EBCDIC 0xc3
  else if (test == 0xc3) {
    block_size=3630;
    idstream::rewind();
  }
  else {
    std::cerr << "Error: unrecognized observation type " << std::hex << static_cast<int>(test) << std::dec << std::endl;
    exit(1);
  }
  return true;
}

int InputFrenchRaobStream::peek()
{
return bfstream::error;
}

int InputFrenchRaobStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;
  size_t num_copied,len,num_copied2;

  if (!is_open()) {
    std::cerr << "Error: no InputFrenchRaobStream has been opened" << std::endl;
    exit(1);
  }
  switch (block_size) {
    case 3630:
	if (buffer_length < 3630) {
	  num_copied=buffer_length;
	}
	else {
	  num_copied=3630;
	}
	if (ics != NULL) {
	  bytes_read=ics->read(buffer,num_copied);
	}
	else {
	  fs.read(reinterpret_cast<char *>(buffer),num_copied);
	  bytes_read=fs.gcount();
	  if (num_copied != 3630)
	    fs.seekg(3630-num_copied,std::ios_base::cur);
	}
	charconversions::ebcdic_to_ascii(reinterpret_cast<char *>(buffer),reinterpret_cast<char *>(buffer),bytes_read);
	break;
    case 3668:
	if (static_cast<int>(file_buf_pos) == file_buf_len) {
	  read_from_disk();
	  if (file_buf_len < 0) {
	    if (file_buf_len == bfstream::error) {
		++num_read;
	    }
	    return file_buf_len;
	  }
	}
	len=(file_buf[file_buf_pos+18]-48)*131;
	if (static_cast<int>(file_buf_pos+len) > file_buf_len) {
	  num_copied=file_buf_len-file_buf_pos;
	  if (num_copied > buffer_length)
	    num_copied=buffer_length;
	  std::copy(&file_buf[file_buf_pos],&file_buf[file_buf_pos]+num_copied,buffer);
	  read_from_disk();
	  if ( (len-num_copied) > buffer_length) {
	    num_copied2=buffer_length-num_copied;
	  }
	  else {
	    num_copied2=len-num_copied;
	  }
	  std::copy(&file_buf[file_buf_pos],&file_buf[file_buf_pos]+num_copied2,&buffer[num_copied]);
	  file_buf_pos+=(len-num_copied);
	  num_copied+=num_copied2;
	}
	else {
	  if (len > buffer_length) {
	    num_copied=buffer_length;
	  }
	  else {
	    num_copied=len;
	  }
	  std::copy(&file_buf[file_buf_pos],&file_buf[file_buf_pos]+num_copied,buffer);
	  file_buf_pos+=len;
	}
	bytes_read=num_copied;
	break;
    default:
	return -1;
  }
  ++num_read;
  return bytes_read;
}

size_t FrenchRaob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void FrenchRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *cbuf=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  short card_num,num_cards;
  int n;
  size_t m,off;
  RaobLevel max_wind,final;
  int temp;
  size_t dum;

  strutils::strget(&cbuf[1],dum,2);
  date_time_.set_year(1900+dum);
  strutils::strget(&cbuf[3],dum,2);
  date_time_.set_month(dum);
  strutils::strget(&cbuf[5],dum,2);
  date_time_.set_day(dum);
  strutils::strget(&cbuf[7],dum,2);
  date_time_.set_time(dum*10000);
  strutils::strget(&cbuf[9],fraob.flags.time_deviation,1);
  location_.ID.assign(&cbuf[10],3);
  strutils::strget(&cbuf[15],fraob.flags.measurement_method,2);
  strutils::strget(&cbuf[18],num_cards,1);
  strutils::strget(&cbuf[21],fraob.flags.sounding_type,1);
  if (fill_header_only) {
    nlev=0;
    return;
  }
  nlev=num_cards*12-1;
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
  strutils::strget(cbuf+17,card_num,1);
  if (card_num != 1) {
    std::cerr << "Error: bad card" << std::endl;
    std::cout.write(cbuf,131);
    std::cout << std::endl;
    exit(1);
  }
// get the maximum wind level
  if (fraob.flags.sounding_type == 1) {
    strutils::strget(cbuf+113,max_wind.pressure,4);
    strutils::strget(cbuf+117,max_wind.wind_direction,2);
    max_wind.wind_direction*=10;
    strutils::strget(cbuf+119,max_wind.wind_speed,3);
    if (max_wind.wind_direction == 880 || max_wind.wind_speed > 880.)
	max_wind.pressure=9999;
    fraob.max_wind_height=max_wind.pressure;
  }
  else {
    strutils::strget(cbuf+113,max_wind.height,4);
    if (max_wind.height != 9999)
	max_wind.height*=10;
    strutils::strget(cbuf+117,max_wind.wind_direction,2);
    max_wind.wind_direction*=10;
    strutils::strget(cbuf+119,max_wind.wind_speed,3);
    if (max_wind.wind_direction == 880 || max_wind.wind_speed > 880.)
	max_wind.height=9999;
    fraob.max_wind_height=max_wind.height;
  }
  nlev=0;
  off=23;
  for (m=0; m < 10; m++) {
    if (fraob.flags.sounding_type == 1) {
	strutils::strget(cbuf+off,levels[nlev].pressure,4);
	if (!floatutils::myequalf(levels[nlev].pressure,9999.)) {
// insert the maximum wind level
	  if (!floatutils::myequalf(max_wind.pressure,9999.) && levels[nlev].pressure < max_wind.pressure) {
	    if (!floatutils::myequalf(max_wind.pressure,levels[nlev-1].pressure)) {
		temp=levels[nlev].pressure;
		levels[nlev]=max_wind;
		levels[nlev].type=5;
		nlev++;
		levels[nlev].pressure=temp;
	    }
	    max_wind.pressure=0;
	  }
	  strutils::strget(cbuf+off+4,levels[nlev].wind_direction,2);
	  levels[nlev].wind_direction*=10;
	  strutils::strget(cbuf+off+6,levels[nlev].wind_speed,3);
	  if (levels[nlev].wind_direction < 880 && levels[nlev].wind_speed < 880.) {
	    if (m == 0)
		levels[nlev].type=0;
	    else
		levels[nlev].type=9;
	    nlev++;
	  }
	}
    }
    else {
	strutils::strget(cbuf+off,levels[nlev].height,4);
	if (levels[nlev].height != 9999) {
	  levels[nlev].height*=10;
// insert the maximum wind level
	  if (max_wind.height != 9999 && levels[nlev].height >
            max_wind.height) {
	    if (max_wind.height != levels[nlev-1].height) {
		temp=levels[nlev].height;
		levels[nlev]=max_wind;
		levels[nlev].type=5;
		nlev++;
		levels[nlev].height=temp;
	    }
	    max_wind.height=9999;
	  }
	  strutils::strget(cbuf+off+4,levels[nlev].wind_direction,2);
	  levels[nlev].wind_direction*=10;
	  strutils::strget(cbuf+off+6,levels[nlev].wind_speed,3);
	  if (levels[nlev].wind_direction < 880 && levels[nlev].wind_speed < 880.) {
	    if (m == 0)
		levels[nlev].type=0;
	    else
		levels[nlev].type=9;
	    nlev++;
	  }
	}
    }
    off+=9;
  }
  off+=9;
// get the final level
  if (fraob.flags.sounding_type == 1) {
    strutils::strget(cbuf+off,final.pressure,4);
    strutils::strget(cbuf+off+4,final.wind_direction,2);
    final.wind_direction*=10;
    strutils::strget(cbuf+off+6,final.wind_speed,3);
    if (final.wind_direction == 880 || final.wind_speed < 880.)
	final.pressure=9999;
  }
  else {
    strutils::strget(cbuf+off,final.height,4);
    if (final.height != 9999)
	final.height*=10;
    strutils::strget(cbuf+off+4,final.wind_direction,2);
    final.wind_direction*=10;
    strutils::strget(cbuf+off+6,final.wind_speed,3);
    if (final.wind_direction == 880 || final.wind_speed < 880.)
	final.height=9999;
  }
  off+=9;
  for (n=1; n < num_cards; n++) {
    strutils::strget(cbuf+off+17,card_num,1);
    if (card_num-1 != n) {
	std::cerr << "Error: bad card" << std::endl;
	std::cout.write(cbuf+off,131);
	std::cout << std::endl;
	exit(1);
    }

    off+=23;
    for (m=0; m < 12; m++) {
	if (fraob.flags.sounding_type == 1) {
	  strutils::strget(cbuf+off,levels[nlev].pressure,4);
	  if (!floatutils::myequalf(levels[nlev].pressure,9999.)) {
// insert the maximum wind level
	    if (!floatutils::myequalf(max_wind.pressure,9999.) && levels[nlev].pressure < max_wind.pressure) {
		if (!floatutils::myequalf(max_wind.pressure,levels[nlev-1].pressure)) {
		  temp=levels[nlev].pressure;
		  levels[nlev]=max_wind;
		  levels[nlev].type=5;
		  nlev++;
		  levels[nlev].pressure=temp;
		}
		else
		  levels[nlev-1].type=5;
		max_wind.pressure=0;
	    }
	    strutils::strget(cbuf+off+4,levels[nlev].wind_direction,2);
	    levels[nlev].wind_direction*=10;
	    strutils::strget(cbuf+off+6,levels[nlev].wind_speed,3);
	    if (levels[nlev].wind_direction < 880 && levels[nlev].wind_speed < 880.) {
		levels[nlev].type=9;
		nlev++;
	    }
	  }
	}
	else {
	  strutils::strget(cbuf+off,levels[nlev].height,4);
	  if (levels[nlev].height != 9999) {
	    levels[nlev].height*=10;
// insert the maximum wind level
	    if (max_wind.height != 9999 && levels[nlev].height >
              max_wind.height) {
		if (max_wind.height != levels[nlev-1].height) {
		  temp=levels[nlev].height;
		  levels[nlev]=max_wind;
		  levels[nlev].type=5;
		  nlev++;
		  levels[nlev].height=temp;
		}
		else
		  levels[nlev-1].type=5;
		max_wind.height=9999;
	    }
	    strutils::strget(cbuf+off+4,levels[nlev].wind_direction,2);
	    levels[nlev].wind_direction*=10;
	    strutils::strget(cbuf+off+6,levels[nlev].wind_speed,3);
	    if (levels[nlev].wind_direction < 880 && levels[nlev].wind_speed < 880.) {
		levels[nlev].type=9;
		nlev++;
	    }
	  }
	}
	off+=9;
    }
  }
// add the max wind level, if not done already, and the final level
  if (fraob.flags.sounding_type == 1) {
    if (!floatutils::myequalf(max_wind.pressure,9999.) && !floatutils::myequalf(max_wind.pressure,0.)) {
	levels[nlev]=max_wind;
	levels[nlev].type=5;
	nlev++;
    }
    if (!floatutils::myequalf(final.pressure,9999.) && !floatutils::myequalf(final.pressure,levels[nlev-1].pressure)) {
	if (final.pressure > levels[nlev-1].pressure) {
	  if (final.pressure > levels[nlev-2].pressure) {
	    std::cerr << "Warning: bad top level on " << date_time_.to_string() << " type: "
            << fraob.flags.sounding_type << std::endl;
	    levels[nlev]=final;
	    if (floatutils::myequalf(final.pressure,max_wind.pressure))
		levels[nlev].type=5;
	    else
		levels[nlev].type=9;
	    nlev++;
	  }
	  else {
	    levels[nlev]=levels[nlev-1];
	    levels[nlev-1]=final;
	    if (floatutils::myequalf(final.pressure,max_wind.pressure))
		levels[nlev-1].type=5;
	    else
		levels[nlev-1].type=9;
	    nlev++;
	  }
	}
	else {
	  levels[nlev]=final;
	  if (floatutils::myequalf(final.pressure,max_wind.pressure))
	    levels[nlev].type=5;
	  else
	    levels[nlev].type=9;
	  nlev++;
	}
    }
  }
  else {
    if (max_wind.height != 9999) {
	levels[nlev]=max_wind;
	levels[nlev].type=5;
	nlev++;
    }
    if (final.height != 9999 && final.height != levels[nlev-1].height) {
	if (final.height < levels[nlev-1].height) {
	  if (final.height < levels[nlev-2].height) {
	    std::cerr << "Warning: bad top level on " << date_time_.to_string() << " type: "
            << fraob.flags.sounding_type << std::endl;
	    levels[nlev]=final;
	    if (final.height == max_wind.height)
		levels[nlev].type=5;
	    else
		levels[nlev].type=9;
	    nlev++;
	  }
	  else {
	    levels[nlev]=levels[nlev-1];
	    levels[nlev-1]=final;
	    if (final.height == max_wind.height)
		levels[nlev-1].type=5;
	    else
		levels[nlev-1].type=9;
	    nlev++;
	  }
	}
	else {
	  levels[nlev]=final;
	  if (final.height == max_wind.height)
	    levels[nlev].type=5;
	  else
	    levels[nlev].type=9;
	  nlev++;
	}
    }
  }
}

void FrenchRaob::print(std::ostream& outs) const
{
  int n;

  print_header(outs,true);
  if (nlev > 0) {
    switch (fraob.flags.sounding_type) {
	case 1:
	  outs << "\n   LEV   PRES   DD    FF" << std::endl;
	  for (n=0; n < nlev; n++)
	    outs << std::setw(6) << n+1 << std::setw(7) << levels[n].pressure << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << std::endl;
	  break;
	case 2:
	  outs << "\n   LEV    HGT   DD    FF" << std::endl;
	  for (n=0; n < nlev; n++)
	    outs << std::setw(6) << n+1 << std::setw(7) << levels[n].height << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << std::endl;
	  break;
    }
  }
  outs << std::endl;
}

void FrenchRaob::print_header(std::ostream& outs,bool verbose) const
{
  if (verbose) {
    outs << "Type: " << std::setw(2) << fraob.flags.sounding_type << "  Station: " << location_.ID << "  Date: " << date_time_.to_string() << "Z  Time " << "Deviation Code: " << std::setw(2) << fraob.flags.time_deviation << "  Method Code: " << std::setw(2) << fraob.flags.measurement_method;
    if (nlev > 0) {
	outs << "  Max Wind Level: " << std::setw(5) << fraob.max_wind_height;
	if (fraob.flags.sounding_type == 1)
	  outs << "mb";
	else
	  outs << "m";
	outs << "  NumLevels: " << std::setw(3) << nlev << std::endl;
    }
    else
	outs << std::endl;
  }
  else {
  }
}
