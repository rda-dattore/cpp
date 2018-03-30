// FILE: galapagosua.cpp
//
#include <iomanip>
#include <raob.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bsort.hpp>

int InputGalapagosSoundingStream::ignore()
{
return -1;
}

int InputGalapagosSoundingStream::peek()
{
return -1;
}

void InputGalapagosSoundingStream::set_date(DateTime& date_time_reference)
{
  short yr,mo,dy,time;

  yr=std::stoi(sline.substr(14,4));
  mo=std::stoi(sline.substr(11,2));
  dy=std::stoi(sline.substr(8,2));
  time=std::stoi(sline.substr(19,4));
  date_time_reference.set(yr,mo,dy,time*100);
}

int InputGalapagosSoundingStream::read(unsigned char *buffer,size_t buffer_length)
{
  char line[32768];
  DateTime curr_date_time;

  if (!fs.is_open()) {
    std::cerr << "Error: no InputGalapagosSoundingStream has been opened" << std::endl;
    exit(1);
  }
  if (date_time.year() == 0) {
    fs.getline(line,32768);
    sline=line;
    set_date(date_time);
  }
  std::string buffer_contents;
  do {
    if (sline.length() > 0) {
	buffer_contents+=sline;
    }
    fs.getline(line,32768);
    if (!fs.eof()) {
	sline=line;
	set_date(curr_date_time);
    }
    else {
	sline="";
    }
  } while (!fs.eof() && curr_date_time == date_time);
  date_time=curr_date_time;
  if ( (buffer_contents.length()+1) > buffer_length) {
    std::cerr << "Error: " << (buffer_contents.length()+1) << "-byte sounding overflows buffer" << std::endl;
    exit(1);
  }
  else {
    if (!buffer_contents.empty()) {
	std::copy(buffer_contents.c_str(),buffer_contents.c_str()+buffer_contents.length(),buffer);
	buffer[buffer_contents.length()]='\0';
	num_read++;
	return (buffer_contents.length()+1);
    }
    else {
	return bfstream::eof;
    }
  }
}

size_t GalapagosSounding::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

bool sort_galapagos_by_pressure(Raob::RaobLevel& left,Raob::RaobLevel& right)
{
  if (left.pressure >= right.pressure) {
    return true;
  }
  else {
    return false;
  }
}
void GalapagosSounding::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  std::deque<std::string> sp;
  std::string dum;
  size_t n;
  int m;
  short yr,mo,dy,time;
  RaobLevel hires_level;
  bool non_missing,found_duplicate=false,found_surface=false;

  sp=strutils::split(std::string(reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer))),"\n");
  nlev=0;
  for (n=0; n < sp.size()-1; n++) {
    if (strutils::has_beginning(sp[n],"1")) {
	location_.ID=sp[n].substr(2,5);
	dy=std::stoi(sp[n].substr(8,2));
	mo=std::stoi(sp[n].substr(11,2));
	yr=std::stoi(sp[n].substr(14,4));
	time=std::stoi(sp[n].substr(19,4));
	date_time_.set(yr,mo,dy,time*100);
// need to add for possibility of ~ in degrees and hundredths
	location_.latitude=-(std::stof(sp[n].substr(26,2))+std::stof(sp[n].substr(29,2))/100.);
	location_.longitude=-(std::stof(sp[n].substr(32,2))+std::stof(sp[n].substr(35,2))/100.);
	location_.elevation=std::stoi(sp[n].substr(38,2));
    }
    else if (strutils::has_beginning(sp[n],"3")) {
	++nlev;
    }
    else if (strutils::has_beginning(sp[n],"4")) {
	++nlev;
    }
    else if (strutils::has_beginning(sp[n],"5")) {
	++nlev;
    }
  }
// add one to regular stack for surface level in hi-res data
  ++nlev;
  if (fill_header_only) {
    return;
  }
/*
  if (nlev > static_cast<int>(raob.capacity)) {
    if (raob.capacity > 0)
      delete[] levels;
    raob.capacity=nlev;
    levels=new RaobLevel[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev)
levels.resize(nlev);
  m=0;
  for (n=0; n < sp.size()-1; n++) {
    switch (static_cast<int>(sp[n][0])) {
	case 49:
	{
// '1' record - station identification
	  break;
	}
	case 50:
	{
// '2' records - hi-resolution sounding
	  if (!found_surface) {
	    non_missing=false;
	    dum=sp[n].substr(24,3);
	    strutils::replace_all(dum,"~","");
	    while (strutils::contains(dum," "))
		strutils::replace_all(dum," ","");
	    if (dum.length() == 0)
		hires_level.time=-99.;
	    else {
		hires_level.time=std::stof(dum);
		dum=sp[n].substr(28,3);
		hires_level.time+=std::stof(dum)/60.;
	    }
	    dum=sp[n].substr(31,5);
	    strutils::replace_all(dum,"~","");
	    while (strutils::contains(dum," "))
		strutils::replace_all(dum," ","");
	    if (dum.length() == 0)
		hires_level.height=99999;
	    else
		hires_level.height=std::stoi(dum);
	    dum=sp[n].substr(37,5);
	    strutils::replace_all(dum,"~","");
	    while (strutils::contains(dum," "))
		strutils::replace_all(dum," ","");
	    if (dum.length() == 0)
		hires_level.pressure=-999.9;
	    else
		hires_level.pressure=std::stof(dum)/10.;
	    dum=sp[n].substr(43,5);
	    strutils::replace_all(dum,"~","");
	    while (strutils::contains(dum," "))
		strutils::replace_all(dum," ","");
	    if (dum.length() == 0)
		hires_level.temperature=-999.9;
	    else {
		hires_level.temperature=std::stof(dum)/10.;
		non_missing=true;
	    }
	    dum=sp[n].substr(53,5);
	    strutils::replace_all(dum,"~","");
	    while (strutils::contains(dum," "))
		strutils::replace_all(dum," ","");
	    if (dum.length() == 0)
		hires_level.moisture=-999.9;
	    else {
		hires_level.moisture=std::stof(dum)/10.;
		non_missing=true;
	    }
	    dum=sp[n].substr(59,3);
	    strutils::replace_all(dum,"~","");
	    while (strutils::contains(dum," "))
		strutils::replace_all(dum," ","");
	    if (dum.length() == 0)
		hires_level.wind_direction=999;
	    else {
		hires_level.wind_direction=std::stoi(dum);
		non_missing=true;
	    }
	    dum=sp[n].substr(63,3);
	    strutils::replace_all(dum,"~","");
	    while (strutils::contains(dum," "))
		strutils::replace_all(dum," ","");
	    if (dum.length() == 0)
		hires_level.wind_speed=-999.;
	    else {
		hires_level.wind_speed=std::stof(dum)/10.;
		non_missing=true;
	    }
	    if (!non_missing && hires_level.pressure > -999. && hires_level.height < 99999)
		non_missing=true;
	    if (non_missing) {
		if (hires_level.height == location_.elevation) {
		  levels[m]=hires_level;
		  levels[m].type=0;
		  found_surface=true;
		  m++;
		}
	    }
	  }
	  break;
	}
	case 51:
	{
// '3' records - significant t/dp levels
	  non_missing=false;
	  levels[m].type=3;
	  dum=sp[n].substr(31,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].height=99999;
	  else
	    levels[m].height=std::stoi(dum);
	  dum=sp[n].substr(37,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].pressure=-999.9;
	  else
	    levels[m].pressure=std::stof(dum)/10.;
	  dum=sp[n].substr(43,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].temperature=-999.9;
	  else {
	    levels[m].temperature=std::stof(dum)/10.;
	    non_missing=true;
	  }
	  dum=sp[n].substr(53,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].moisture=-999.9;
	  else {
	    levels[m].moisture=std::stof(dum)/10.;
	    non_missing=true;
	  }
	  levels[m].wind_direction=999;
	  levels[m].wind_speed=999.;
	  if (!non_missing && levels[m].pressure > -999. && levels[m].height < 99999)
	    non_missing=true;
	  if (non_missing)
	    m++;
	  break;
	}
	case 52:
	{
// '4' records - significant wind levels
	  non_missing=false;
	  levels[m].type=4;
	  dum=sp[n].substr(31,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].height=99999;
	  else
	    levels[m].height=std::stoi(dum);
	  dum=sp[n].substr(37,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].pressure=-999.9;
	  else
	    levels[m].pressure=std::stof(dum)/10.;
	  levels[m].temperature=-999.9;
	  levels[m].moisture=-999.9;
	  dum=sp[n].substr(43,3);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].wind_direction=999;
	  else {
	    levels[m].wind_direction=std::stoi(dum);
	    non_missing=true;
	  }
	  dum=sp[n].substr(47,3);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," ")) {
	    strutils::replace_all(dum," ","");
	  }
	  if (dum.empty()) {
	    levels[m].wind_speed=999.;
	  }
	  else {
	    levels[m].wind_speed=std::stof(dum)/10.;
	    non_missing=true;
	  }
	  if (!non_missing && levels[m].pressure > -999. && levels[m].height < 99999) {
	    non_missing=true;
	  }
	  if (non_missing) {
	    m++;
	  }
	  break;
	}
	case 53:
	{
// '5' records - mandatory levels
	  non_missing=false;
	  levels[m].type=5;
	  dum=sp[n].substr(24,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," ")) {
	    strutils::replace_all(dum," ","");
	  }
	  if (dum.empty()) {
	    levels[m].pressure=-999.9;
	  }
	  else {
	    levels[m].pressure=std::stof(dum)/10.;
	  }
	  dum=sp[n].substr(30,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," ")) {
	    strutils::replace_all(dum," ","");
	  }
	  if (dum.empty()) {
	    levels[m].height=99999;
	  }
	  else {
	    levels[m].height=std::stoi(dum);
	  }
	  dum=sp[n].substr(36,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," ")) {
	    strutils::replace_all(dum," ","");
	  }
	  if (dum.empty()) {
	    levels[m].temperature=-999.9;
	  }
	  else {
	    levels[m].temperature=std::stof(dum)/10.;
	    non_missing=true;
	  }
	  dum=sp[n].substr(46,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," ")) {
	    strutils::replace_all(dum," ","");
	  }
	  if (dum.empty()) {
	    levels[m].moisture=-999.9;
	  }
	  else {
	    levels[m].moisture=std::stof(dum)/10.;
	    non_missing=true;
	  }
	  dum=sp[n].substr(52,3);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," ")) {
	    strutils::replace_all(dum," ","");
	  }
	  if (dum.empty()) {
	    levels[m].wind_direction=999;
	  }
	  else {
	    levels[m].wind_direction=std::stoi(dum);
	    non_missing=true;
	  }
	  dum=sp[n].substr(56,3);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," ")) {
	    strutils::replace_all(dum," ","");
	  }
	  if (dum.empty()) {
	    levels[m].wind_speed=999.;
	  }
	  else {
	    levels[m].wind_speed=std::stof(dum)/10.;
	    non_missing=true;
	  }
	  if (!non_missing && levels[m].pressure > -999. && levels[m].height < 99999) {
	    non_missing=true;
	  }
	  if (non_missing) {
	    ++m;
	  }
	  break;
	}
	default:
	{
	  std::cerr << "Error: record beginning with " << sp[n][0] << " not defined" << std::endl;
	  exit(1);
	}
    }
  }
  nlev=m;
  binary_sort(levels,sort_galapagos_by_pressure,0,nlev);
  for (n=1; n < static_cast<size_t>(nlev); n++) {
    if (floatutils::myequalf(levels[n].pressure,levels[n-1].pressure)) {
	found_duplicate=true;
	if (levels[n].height > 90000)
	  levels[n].height=levels[n-1].height;
	else if (levels[n-1].height < 90000 && levels[n].height != levels[n-1].height) {
	  levels[n].height= (levels[n].type == 5) ? levels[n-1].height : levels[n].height;
}
	if (levels[n].temperature < -999.)
	  levels[n].temperature=levels[n-1].temperature;
	else if (levels[n-1].temperature > -999. && !floatutils::myequalf(levels[n].temperature,levels[n-1].temperature))
	  levels[n].temperature= (levels[n].type == 5) ? levels[n-1].temperature : levels[n].temperature;
	if (levels[n].moisture < -999.)
	  levels[n].moisture=levels[n-1].moisture;
	else if (levels[n-1].moisture > -999. && !floatutils::myequalf(levels[n].moisture,levels[n-1].moisture))
	  levels[n].moisture= (levels[n].type == 5) ? levels[n-1].moisture : levels[n].moisture;
	if (levels[n].wind_direction == 999)
	  levels[n].wind_direction=levels[n-1].wind_direction;
	else if (levels[n-1].wind_direction < 999 && levels[n].wind_direction != levels[n-1].wind_direction)
	  levels[n].wind_direction= (levels[n].type == 5) ? levels[n-1].wind_direction : levels[n].wind_direction;
	if (floatutils::myequalf(levels[n].wind_speed,999.))
	  levels[n].wind_speed=levels[n-1].wind_speed;
	else if (levels[n-1].wind_speed < 999. && !floatutils::myequalf(levels[n].wind_speed,levels[n-1].wind_speed))
	  levels[n].wind_speed= (levels[n].type == 5) ? levels[n-1].wind_speed : levels[n].wind_speed;
	levels[n].type= (levels[n].type == 0 || levels [n].type == 5) ? levels[n].type : levels[n-1].type;
	levels[n-1].pressure=0.;
    }
  }
  if (found_duplicate) {
    binary_sort(levels,sort_galapagos_by_pressure,0,nlev);
    for (n=0,m=0; n < static_cast<size_t>(nlev); n++) {
	if (levels[n].pressure > 0.)
	  m++;
    }
    nlev=m;
  }
}

void GalapagosSounding::print(std::ostream& outs) const
{
  int n;

  print_header(outs,true);
  outs.precision(1);
  outs << "  LEV   PRES    HGT   TEMP  DEWPT   DD    FF  TYPE" << std::endl;
  for (n=0; n < nlev; n++)
    outs << std::setw(5) << n+1 << std::setw(7) << levels[n].pressure << std::setw(7) << levels[n].height << std::setw(7) << levels[n].temperature << std::setw(7) << levels[n].moisture << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << std::setw(6) << levels[n].type << std::endl;
}

void GalapagosSounding::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);

  if (verbose) {
    outs << "  Station: " << location_.ID << " Date: " << date_time_.to_string() << "  Location: " << std::setw(7) << location_.latitude << std::setw(8) << location_.longitude << std::setw(5) << location_.elevation << "  Number of Levels: " << std::setw(3) << nlev << std::endl;
  }
  else
    outs << " Stn=" << location_.ID << " Date=" << date_time_.to_string("%Y%m%d%H%MM") << " Lat=" << location_.latitude << " Lon=" << location_.longitude << " Elev=" << location_.elevation << std::endl;
}

size_t GalapagosHighResolutionSounding::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void GalapagosHighResolutionSounding::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  std::deque<std::string> sp;
  std::string dum;
  size_t n;
  int m;
  short yr,mo,dy,time;
  bool non_missing,found_duplicate=false;

  sp=strutils::split(std::string(reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer))),"\n");
  nlev=0;
  for (n=0; n < sp.size()-1; n++) {
    if (strutils::has_beginning(sp[n],"1")) {
	location_.ID=sp[n].substr(2,5);
	dy=std::stoi(sp[n].substr(8,2));
	mo=std::stoi(sp[n].substr(11,2));
	yr=std::stoi(sp[n].substr(14,4));
	time=std::stoi(sp[n].substr(19,4));
	date_time_.set(yr,mo,dy,time*100);
// need to add for possibility of ~ in degrees and hundredths
	location_.latitude=-(std::stof(sp[n].substr(26,2))+std::stof(sp[n].substr(29,2))/100.);
	location_.longitude=-(std::stof(sp[n].substr(32,2))+std::stof(sp[n].substr(35,2))/100.);
	location_.elevation=std::stoi(sp[n].substr(38,2));
    }
    else if (strutils::has_beginning(sp[n],"2"))
	nlev++;
  }
  if (fill_header_only)
    return;
/*
  if (nlev > static_cast<int>(raob.capacity)) {
    if (raob.capacity > 0)
      delete[] levels;
    raob.capacity=nlev;
    levels=new RaobLevel[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev)
levels.resize(nlev);
  m=0;
  for (n=0; n < sp.size()-1; n++) {
    switch (static_cast<int>(sp[n][0])) {
// '1' record - station identification
	case 49:
	  break;
// '2' records - hi-resolution sounding
	case 50:
	  non_missing=false;
	  levels[m].type=2;
	  dum=sp[n].substr(24,3);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].time=-99.;
	  else {
	    levels[m].time=std::stof(dum);
	    dum=sp[n].substr(28,3);
	    levels[m].time+=std::stof(dum)/60.;
	  }
	  dum=sp[n].substr(31,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].height=99999;
	  else
	    levels[m].height=std::stoi(dum);
	  dum=sp[n].substr(37,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].pressure=-999.9;
	  else
	    levels[m].pressure=std::stof(dum)/10.;
	  dum=sp[n].substr(43,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].temperature=-999.9;
	  else {
	    levels[m].temperature=std::stof(dum)/10.;
	    non_missing=true;
	  }
	  dum=sp[n].substr(53,5);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].moisture=-999.9;
	  else {
	    levels[m].moisture=std::stof(dum)/10.;
	    non_missing=true;
	  }
	  dum=sp[n].substr(59,3);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].wind_direction=999;
	  else {
	    levels[m].wind_direction=std::stoi(dum);
	    non_missing=true;
	  }
	  dum=sp[n].substr(63,3);
	  strutils::replace_all(dum,"~","");
	  while (strutils::contains(dum," "))
	    strutils::replace_all(dum," ","");
	  if (dum.length() == 0)
	    levels[m].wind_speed=-999.;
	  else {
	    levels[m].wind_speed=std::stof(dum)/10.;
	    non_missing=true;
	  }
	  if (!non_missing && levels[m].pressure > -999. && levels[m].height < 99999)
	    non_missing=true;
	  if (non_missing) {
	    if (levels[m].height == location_.elevation)
		levels[m].type=0;
	    m++;
	  }
	  break;
// '3' records - significant t/dp levels
	case 51:
// '4' records - significant wind levels
	case 52:
// '5' records - mandatory levels
	case 53:
	  break;
	default:
	  std::cerr << "Error: record beginning with " << sp[n][0] << " not defined" << std::endl;
	  exit(1);
    }
  }
  nlev=m;
  binary_sort(levels,sort_galapagos_by_pressure,0,nlev);
  for (n=1; n < static_cast<size_t>(nlev); n++) {
    if (floatutils::myequalf(levels[n].pressure,levels[n-1].pressure)) {
	found_duplicate=true;
	if (levels[n].height > 90000)
	  levels[n].height=levels[n-1].height;
	else if (levels[n-1].height < 90000 && levels[n].height != levels[n-1].height) {
	  levels[n].height= (levels[n].type == 5) ? levels[n-1].height : levels[n].height;
}
	if (levels[n].temperature < -999.)
	  levels[n].temperature=levels[n-1].temperature;
	else if (levels[n-1].temperature > -999. && !floatutils::myequalf(levels[n].temperature,levels[n-1].temperature))
	  levels[n].temperature= (levels[n].type == 5) ? levels[n-1].temperature : levels[n].temperature;
	if (levels[n].moisture < -999.)
	  levels[n].moisture=levels[n-1].moisture;
	else if (levels[n-1].moisture > -999. && !floatutils::myequalf(levels[n].moisture,levels[n-1].moisture))
	  levels[n].moisture= (levels[n].type == 5) ? levels[n-1].moisture : levels[n].moisture;
	if (levels[n].wind_direction == 999)
	  levels[n].wind_direction=levels[n-1].wind_direction;
	else if (levels[n-1].wind_direction < 999 && levels[n].wind_direction != levels[n-1].wind_direction)
	  levels[n].wind_direction= (levels[n].type == 5) ? levels[n-1].wind_direction : levels[n].wind_direction;
	if (floatutils::myequalf(levels[n].wind_speed,999.))
	  levels[n].wind_speed=levels[n-1].wind_speed;
	else if (levels[n-1].wind_speed < 999. && !floatutils::myequalf(levels[n].wind_speed,levels[n-1].wind_speed))
	  levels[n].wind_speed= (levels[n].type == 5) ? levels[n-1].wind_speed : levels[n].wind_speed;
	levels[n].type= (levels[n].type == 0 || levels [n].type == 5) ? levels[n].type : levels[n-1].type;
	levels[n-1].pressure=0.;
    }
  }
  if (found_duplicate) {
    binary_sort(levels,sort_galapagos_by_pressure,0,nlev);
    for (n=0,m=0; n < static_cast<size_t>(nlev); ++n) {
	if (levels[n].pressure > 0.) {
	  ++m;
	}
    }
    nlev=m;
  }
}

void GalapagosHighResolutionSounding::print(std::ostream& outs) const
{
  print_header(outs,true);
  outs.precision(1);
  outs << "  LEV   TIME   PRES    HGT   TEMP  DEWPT   DD    FF  TYPE" << std::endl;
  for (short n=0; n < nlev; ++n) {
    outs << std::setw(5) << n << std::setw(7) << levels[n].time << std::setw(7) << levels[n].pressure << std::setw(7) << levels[n].height << std::setw(7) << levels[n].temperature << std::setw(7) << levels[n].moisture << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << std::setw(6) << levels[n].type << std::endl;
  }
}

void GalapagosHighResolutionSounding::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);

  if (verbose) {
    outs << "  Station: " << location_.ID << " Date: " << date_time_.to_string() << "  Location: " << std::setw(7) << location_.latitude << std::setw(8) << location_.longitude << std::setw(5) << location_.elevation << "  Number of Levels: " << std::setw(3) << nlev << std::endl;
  }
  else {
    outs << " Stn=" << location_.ID << " Date=" << date_time_.to_string("%Y%m%d%H%MM") << " Lat=" << location_.latitude << " Lon=" << location_.longitude << " Elev=" << location_.elevation << std::endl;
  }
}
