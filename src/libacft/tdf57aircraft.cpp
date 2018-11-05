#include <iomanip>
#include <aircraft.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <myerror.hpp>

int InputTDF57AircraftReportStream::ignore()
{
  if (icosstream != nullptr) {
    int num_bytes;
    if ( (num_bytes=icosstream->ignore()) > 0) {
	++num_read;
    }
    return num_bytes;
  }
  else {
    if (!fs.is_open()) {
	myerror="no open input stream";
	return bfstream::error;
    }
    char c='\0';
    int num_bytes=0;
    while (c != '\n') {
	fs.read(&c,1);
	if (!fs.good()) {
	  if (fs.eof()) {
	    return bfstream::eof;
	  }
	  else {
	    return bfstream::error;
	  }
	}
	num_bytes+=fs.gcount();
    }
    ++num_read;
    return num_bytes;
  }
}

int InputTDF57AircraftReportStream::peek()
{
  if (icosstream != nullptr) {
    return icosstream->peek();
  }
  else {
    if (!fs.is_open()) {
	myerror="no open input stream";
	return bfstream::error;
    }
    char c='\0';
    int num_bytes=0;
    while (c != '\n') {
	fs.read(&c,1);
	if (!fs.good()) {
	  if (fs.eof()) {
	    return bfstream::eof;
	  }
	  else {
	    return bfstream::error;
	  }
	}
	num_bytes+=fs.gcount();
    }
    fs.seekg(-num_bytes,std::ios::cur);
    return num_bytes;
  }
}

int InputTDF57AircraftReportStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (icosstream != nullptr) {
    int num_bytes;
    if ( (num_bytes=icosstream->read(buffer,buffer_length)) > 0) {
	++num_read;
    }
    return num_bytes;
  }
  else {
    if (!fs.is_open()) {
	myerror="no open input stream";
	return bfstream::error;
    }
    fs.getline(reinterpret_cast<char *>(buffer),buffer_length);
    if (!fs.good()) {
	if (fs.eof()) {
	  return bfstream::eof;
	}
	else {
	  return bfstream::error;
	}
    }
    ++num_read;
    return fs.gcount();
  }
}

void TDF57AircraftReport::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  auto *buf=reinterpret_cast<const char *>(stream_buffer);
  location_.ID.assign(&buf[4],4);
  short yr,mo,dy,time;
  strutils::strget(&buf[12],yr,2);
  strutils::strget(&buf[14],mo,2);
  strutils::strget(&buf[16],dy,2);
  strutils::strget(&buf[18],time,4);
  date_time_.set(1900+yr,mo,dy,time*100,0);
  if (buf[22] == ' ' || buf[22] == '*') {
    location_.latitude=-99.9;
    location_.longitude=-999.9;
  }
  else {
    std::string lat_s(&buf[23],3);
    if (lat_s == "   " || lat_s == "  *") {
	location_.latitude=-99.9;
    }
    else {
	location_.latitude=std::stof(lat_s)/10.;
	if (buf[22] >= '5' && buf[22] <= '8') {
	  location_.latitude=-location_.latitude;
	}
    }
    std::string lon_s(&buf[26],3);
    if (lon_s == "   " || lon_s == "  *") {
	location_.longitude=-999.9;
    }
    else {
	auto lon=std::stoi(lon_s);
	if (lon <= 800 && (buf[22] == '1' || buf[22] == '2' || buf[22] == '6' || buf[22] == '7')) {
	  lon+=1000;
	}
	location_.longitude=lon/10.;
	if (buf[22] == '0' || buf[22] == '1' || buf[22] == '5' || buf[22] == '6') {
	  location_.longitude=-location_.longitude;
	}
    }
  }
  std::string alt_s(&buf[29],3);
  if (alt_s == "   " || alt_s == "  *") {
    location_.altitude=-99.9;
  }
  else {
    location_.altitude=std::stoi(alt_s)*100;
  }
  std::string data_s;
  data_s.assign(&buf[41],2);
  if (data_s == "  " || data_s == " *") {
    data.wind_direction=999;
  }
  else {
    data.wind_direction=std::stoi(data_s)*10;
  }
  data_s.assign(&buf[43],3);
  if (data_s == "   " || data_s == "  *") {
    data.wind_speed=-99.9;
  }
  else {
    data.wind_speed=std::stoi(data_s);
  }
  data_s.assign(&buf[98],2);
  if (data_s == "  " || data_s == " *") {
    data.temperature=-99.9;
  }
  else {
    data.temperature=charconversions::decode_overpunch(&buf[98],2,1);
  }
  data_s.assign(&buf[100],2);
  if (data_s == "  " || data_s == " *") {
    data_s.assign(&buf[102],2);
    if (data_s == "  " || data_s == " *") {
	data_s.assign(&buf[104],2);
	if (data_s == "  " || data_s == " *") {
	  data_s.assign(&buf[106],2);
	  if (data_s == "  " || data_s == " *") {
	  }
	  else {
	    data.moisture_indicator=MoistureType::RELATIVE_HUMIDITY;
	  }
	}
	else {
	  data.moisture_indicator=MoistureType::DEW_POINT_DEPRESSION;
	}
    }
    else {
	data.moisture_indicator=MoistureType::WET_BULB_DEPRESSION;
    }
  }
  else {
    data.moisture_indicator=MoistureType::DEW_POINT_TEMPERATURE;
  }
}

void TDF57AircraftReport::print() const
{
  print_header();
  std::cout.precision(1);
  std::cout << "  DD: " << std::setw(3) << data.wind_direction << " FF: " << std::setw(5) << data.wind_speed << "  Temp: " << data.temperature << std::endl;
}

void TDF57AircraftReport::print_header() const
{
  std::cout.setf(std::ios::fixed);
  std::cout.precision(2);
  std::cout << "  Date/Time: " << date_time_.to_string() << "  ID: " << location_.ID << "  Lat: " << location_.latitude << "  Lon: " << location_.longitude << "  Alt: " << location_.altitude << std::endl;
}
