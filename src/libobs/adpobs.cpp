// FILE: adpobs.cpp

#include <strings.h>
#include <adpobs.hpp>
#include <strutils.hpp>

void ADPObservation::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *buffer=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  int units;
  strutils::strget(buffer,units,2);
  short hund,yr,mo,dy;
  strutils::strget(&buffer[2],hund,2);
  strutils::strget(&buffer[4],yr,2);
  strutils::strget(&buffer[6],mo,2);
  strutils::strget(&buffer[8],dy,2);
  if (mo != 0 && dy != 0) {
    if (yr < 70) {
	yr+=2000;
    }
    else {
	yr+=1900;
    }
  }
  synoptic_date_time_.set(yr,mo,dy,units*10000+lround(hund*60./100.)*100);
  location_.ID.assign(&buffer[20],6);
  strutils::trim(location_.ID);
  strutils::strget(&buffer[26],units,2);
  strutils::strget(&buffer[28],hund,2);
  if (synoptic_date_time_.time() == 0 && units > 18) {
    date_time_=date_time_.hours_subtracted(24);
    date_time_.set_time(units*10000+lround(hund*60./100.)*100);
  }
  else {
    date_time_.set(yr,mo,dy,units*10000+lround(hund*60./100.)*100);
  }
  strutils::strget(&buffer[30],units,2);
  strutils::strget(&buffer[32],hund,2);
  if (synoptic_date_time_.time() == 0 && units > 18) {
    receipt_date_time_=date_time_.hours_subtracted(24);
    receipt_date_time_.set_time(units*10000+lround(hund*60./100.)*100);
  }
  else if (synoptic_date_time_.time() == 1800 && units < 15) {
    receipt_date_time_=date_time_.hours_added(24);
    receipt_date_time_.set_time(units*10000+lround(hund*60./100.)*100);
  }
  else {
    receipt_date_time_.set(yr,mo,dy,units*10000+lround(hund*60./100.)*100);
  }
  strutils::strget(&buffer[37],platform_type_,3);
  strutils::strget(&buffer[10],units,5);
  if (units != 99999 && units <= 9000 and units >= -9000) {
    location_.latitude=units/100.;
  }
  else {
    location_.latitude=-99.9;
  }
  strutils::strget(&buffer[15],units,5);
  if (units != 99999 && units >= 0 && units <= 36000) {
    location_.longitude= (units == 0) ? 0. : 360.-(units/100.);
  }
  else {
    location_.longitude=-999.9;
  }
  strutils::strget(&buffer[40],location_.elevation,5);
  categories_.clear();
  if (fill_header_only) {
    return;
  }
  int rpt_len;
  strutils::strget(&buffer[47],rpt_len,3);
  rpt_len*=10;
  auto off=50;
  while (off < rpt_len) {
    std::string s;
    s.assign(&buffer[off],2);
    if (strutils::is_numeric(s)) {
	if (s != "00" && s != "09" && s != "99") {
	  categories_.emplace_back(std::stoi(s));
	}
	else {
	  break;
	}
    }
    else {
	break;
    }
    s.assign(&buffer[off+2],3);
    if (strutils::is_numeric(s)) {
	off=std::stoi(s)*10;
    }
    else {
	break;
    }
  }
}
