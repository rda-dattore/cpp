#include <iomanip>
#include <regex>
#include <marine.hpp>
#include <strutils.hpp>
#include <myerror.hpp>

const size_t InputNODCSeaLevelObservationStream::FILE_BUF_LEN=81;
const size_t InputNODCSeaLevelObservationStream::RECORD_LEN=39;

int InputNODCSeaLevelObservationStream::ignore()
{
return -1;
}

int InputNODCSeaLevelObservationStream::peek()
{
return -1;
}

int InputNODCSeaLevelObservationStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (buffer_length < RECORD_LEN) {
    myerror="buffer overflow";
    return bfstream::error;
  }
  if (file_buf == nullptr) {
    file_buf.reset(new char[FILE_BUF_LEN]);
  }
  if (value_offset == 0) {
    fs.getline(file_buf.get(),FILE_BUF_LEN);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (fs.fail()) {
	myerror="file read error";
	return bfstream::error;
    }
    std::string file_type(&file_buf[0],3);
    if (!std::regex_search(file_type,std::regex("^18[456]$"))) {
	myerror="not a sea level data record";
	return bfstream::error;
    }
    auto data_type=file_buf[2];
    while (!fs.eof() && file_buf[9] != data_type) {
	switch (file_buf[9]) {
	  case '1':
	  {
	    id.assign(&file_buf[10],8);
	    lat.assign(&file_buf[48],5);
	    lon.assign(&file_buf[54],6);
	    utc_minutes_offset.assign(&file_buf[70],4);
	    break;
	  }
	  case '2':
	  {
	    break;
	  }
	  case '3':
	  {
	    break;
	  }
	  default:
	  {
	    myerror="invalid sea level data record type";
	    return bfstream::error;
	  }
	}
	fs.getline(file_buf.get(),FILE_BUF_LEN);
	if (fs.eof()) {
	  return bfstream::eof;
	}
	else if (fs.fail()) {
	  myerror="file read error";
	  return bfstream::error;
	}
    }
  }
  std::copy(id.begin(),id.end(),&buffer[0]);
  std::copy(lat.begin(),lat.end(),&buffer[8]);
  std::copy(lon.begin(),lon.end(),&buffer[13]);
  std::copy(utc_minutes_offset.begin(),utc_minutes_offset.end(),&buffer[30]);
  buffer[19]=file_buf[9];
  switch (buffer[19]) {
    case '4':
    {
	std::copy(&file_buf[11],&file_buf[19],&buffer[20]);
	short ampm=(file_buf[19]-49)*12;
	auto hr=strutils::ftos(ampm+value_offset,2,0,'0');
	buffer[28]=hr[0];
	buffer[29]=hr[1];
	auto off=20+value_offset*5;
	std::copy(&file_buf[off],&file_buf[off+5],&buffer[34]);
	if (++value_offset == 12) {
	  value_offset=0;
	}
	break;
    }
    case '5':
    {
	short year;
	strutils::strget(&file_buf[11],year,4);
	DateTime dt(year,1,0,0,0);
	short ndays;
	strutils::strget(&file_buf[16],ndays,3);
	auto date=dt.days_added(ndays+value_offset).to_string("%Y%m%d%H");
	std::copy(date.begin(),date.end(),&buffer[20]);
	auto off=20+value_offset*5;
	std::copy(&file_buf[off],&file_buf[off+5],&buffer[34]);
	if (++value_offset == 12) {
	  value_offset=0;
	}
	break;
    }
    case '6':
    {
	short year;
	strutils::strget(&file_buf[11],year,4);
	DateTime dt(year,1,1,0,0);
	short smonth=(file_buf[15]-49)*6;
	auto date=dt.months_added(smonth+value_offset).to_string("%Y%m%d%H");
	std::copy(date.begin(),date.end(),&buffer[20]);
	auto off=17+value_offset*8;
	std::copy(&file_buf[off],&file_buf[off+5],&buffer[34]);
	if (++value_offset == 6) {
	  value_offset=0;
	}
	break;
    }
  }
  ++num_read;
  return RECORD_LEN;
}

void NODCSeaLevelObservation::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *buf=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  location_.ID.assign(&buf[0],8);
  short dd,mm;
  strutils::strget(&buf[8],dd,2);
  strutils::strget(&buf[10],mm,2);
  location_.latitude=dd+static_cast<float>(mm)/60.;
  if (buf[12] == 'S') {
    location_.latitude=-location_.latitude;
  }
  strutils::strget(&buf[13],dd,3);
  strutils::strget(&buf[16],mm,2);
  location_.longitude=dd+static_cast<float>(mm)/60.;
  if (buf[18] == 'S') {
    location_.longitude=-location_.longitude;
  }
  short year,month;
  strutils::strget(&buf[20],year,4);
  strutils::strget(&buf[24],month,2);
  data_format_="F18";
  data_format_.push_back(buf[19]);
  short day,hour;
  switch (buf[19]) {
    case '4':
    {
	strutils::strget(&buf[26],day,2);
	strutils::strget(&buf[28],hour,2);
	break;
    }
    case '5':
    {
	strutils::strget(&buf[26],day,2);
	hour=0;
	break;
    }
    case '6':
    {
	day=dateutils::days_in_month(year,month);
	hour=0;
    }
  }
  short utc_off;
  strutils::strget(&buf[30],utc_off,4);
  auto hh=utc_off/10.;
  auto min=(utc_off % 10)*6;
  date_time_.set(year,month,day,hour*10000,hh*100+min);
  strutils::strget(&buf[34],sea_level_height_,5);
}

void NODCSeaLevelObservation::print(std::ostream& outs) const
{
  outs << "ID: " << location_.ID << " Lat: " << location_.latitude << " Lon: " << location_.longitude << " Date: " << date_time_.to_string("%Y-%m-%d %H:%MM %Z") << " Sea Level Height: " << sea_level_height_ << std::endl;
}

void NODCSeaLevelObservation::print_header(std::ostream& outs,bool verbose) const
{
}
