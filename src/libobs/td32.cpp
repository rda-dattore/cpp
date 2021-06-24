#include <fstream>
#include <td32.hpp>
#include <strutils.hpp>
#include <utils.hpp>

my::map<TD32Data::LibEntry> TD32Data::COOP_table,TD32Data::WBAN_table;

void InputTD32Stream::close()
{
  idstream::close();
  block_len=0;
  off=0;
}

int InputTD32Stream::ignore()
{
return -1;
}

int InputTD32Stream::peek()
{
return -1;
}

int InputTD32Stream::read(unsigned char *buffer,size_t buffer_length)
{
  if ((off+34) > block_len) {
    if (ics != nullptr) {
	curr_offset=-1;
	block_len=ics->read(block,20000);
	if (block_len < 0) {
	  return block_len;
	}
    }
    else if (fs.is_open()) {
	curr_offset=fs.tellg();
	fs.getline(reinterpret_cast<char *>(block),20000);
	if (fs.eof()) {
	  return bfstream::eof;
	}
	else if (fs.fail()) {
	  return bfstream::error;
	}
	block_len=fs.gcount()-1;
	if (block_len < 35) {
	  return bfstream::error;
	}
    }
    else {
	return bfstream::error;
    }
    off=0;
  }
  int num_bytes;
  strutils::strget(&((reinterpret_cast<char *>(block))[off]),num_bytes,4);
  if (num_bytes > static_cast<int>(buffer_length)) {
    return bfstream::error;
  }
  std::copy(&block[off],&block[off+num_bytes],buffer);
  off+=num_bytes;
  ++num_read;
  return num_bytes;
}

void TD32Data::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *cbuf=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  short yr,mo,dy;
  size_t n,off;
  int dum,dum2;
  LibEntry le;
  DateTime check_date_time;
  std::list<DateTime>::iterator it_start,it_end,end_it;
  std::list<float>::iterator it_lat,it_lon;

  header_.type.assign(&cbuf[4],3);
  location_.ID.assign(&cbuf[7],8);
  location_.latitude=-99.;
  location_.longitude=-999.;
  header_.elem.assign(&cbuf[15],4);
  header_.unit.assign(&cbuf[19],2);
  strutils::strget(&cbuf[21],yr,4);
  if (header_.type != "MLY") {
    strutils::strget(&cbuf[25],mo,2);
  }
  else {
    mo=0;
  }
  if (header_.type != "MLY" && header_.type != "DLY") {
    if (header_.type == "HLY") {
	strutils::strget(&cbuf[29],dy,2);
    }
    else {
	strutils::strget(&cbuf[27],dy,4);
    }
  }
  else {
    dy=0;
  }
  date_time_.set(yr,mo,dy,0);
  strutils::strget(&cbuf[31],num_vals,3);
  if (fill_header_only)
    return;
// WBAN number
  if (strutils::has_beginning(location_.ID,"000")) {
    if (WBAN_table.size() == 0) {
	fill_WBAN_library();
    }
    le.key=location_.ID.substr(3,5);
    if (!WBAN_table.found(le.key,le)) {
	le.data=nullptr;
    }
  }
// COOP number
  else {
    if (COOP_table.size() == 0) {
	fill_COOP_library();
    }
    le.key=location_.ID.substr(0,6);
    if (!COOP_table.found(le.key,le)) {
	le.data=NULL;
    }
  }
  reports_.resize(num_vals);
  off=34;
  for (n=0; n < num_vals; n++) {
    reports_[n].loc=location_;
    reports_[n].date_time=date_time_;
    if (header_.type == "DLY") {
	strutils::strget(&cbuf[off],dum,2);
	strutils::strget(&cbuf[off+2],dum2,2);
	reports_[n].date_time.set_day(dum);
	reports_[n].date_time.set_time(dum2*100);
    }
    else if (header_.type == "MLY") {
	strutils::strget(&cbuf[off],dum,2);
	reports_[n].date_time.set_month(dum);
    }
    else if (header_.type == "HLY" || header_.type == "HPD" || header_.type == "15M") {
	strutils::strget(&cbuf[off],dum,4);
	reports_[n].date_time.set_time(dum);
    }
    reports_[n].loc.latitude=-99.;
    reports_[n].loc.longitude=-999.;
    if (le.data != nullptr) {
	it_start=le.data->start.begin();
	it_end=le.data->end.begin();
	end_it=le.data->start.end();
	it_lat=le.data->lat.begin();
	it_lon=le.data->lon.begin();
	check_date_time=reports_[n].date_time;
	if (check_date_time.time() > 2400) {
	  check_date_time.set_time(2400);
	}
	for (; it_start != end_it; ++it_start,++it_end,++it_lat,++it_lon) {
	  if (check_date_time < *it_start || (check_date_time >= *it_start && check_date_time <= *it_end)) {
	    break;
	  }
	}
	if (it_lat != le.data->lat.end()) {
	  reports_[n].loc.latitude=*it_lat;
	  reports_[n].loc.longitude=*it_lon;
	}
    }
    strutils::strget(&cbuf[off+4],dum,6);
    reports_[n].value=dum;
    if (header_.unit[0] == 'T') {
	reports_[n].value/=10.;
    }
    else if (header_.unit[0] == 'H') {
	reports_[n].value/=100.;
    }
    reports_[n].flag1=cbuf[off+10];
    reports_[n].flag2=cbuf[off+11];
    off+=12;
  }
}

void TD32Data::fill_COOP_library()
{
  TD32Data::LibEntry le;
  DateTime start,end;
  short yr,mo,dy;
  float lat,lon;
  int deg,min,sec;
  std::string error;

  auto coop_library=unixutils::remote_web_file("http://rda.ucar.edu/datasets/ds901.0/station_libraries/COOP.TXT.latest","/tmp");
  std::ifstream ifs(coop_library);
  if (!ifs.is_open()) {
    std::cerr << "Error opening COOP.TXT.latest" << std::endl;
    exit(1);
  }
  char line[32768];
  ifs.getline(line,32768);
  while (!ifs.eof()) {
    le.key.assign(line,6);
    if (strutils::is_numeric(le.key)) {
	strutils::strget(&line[130],yr,4);
	strutils::strget(&line[134],mo,2);
	strutils::strget(&line[136],dy,2);
	start.set(yr,mo,dy,0);
	strutils::strget(&line[139],yr,4);
	strutils::strget(&line[143],mo,2);
	strutils::strget(&line[145],dy,2);
	end.set(yr,mo,dy,2400);
	strutils::strget(&line[149],deg,2);
	strutils::strget(&line[152],min,2);
	strutils::strget(&line[155],sec,2);
	lat=deg+min/60.+sec/3600.;
	if (line[148] == '-')
	  lat=-lat;
	strutils::strget(&line[159],deg,3);
	strutils::strget(&line[163],min,2);
	strutils::strget(&line[166],sec,2);
	lon=deg+min/60.+sec/3600.;
	if (line[158] == '-')
	  lon=-lon;
	if (!COOP_table.found(le.key,le)) {
	  le.data.reset(new LibEntry::Data);
	  le.data->start.emplace_back(start);
	  le.data->end.emplace_back(end);
	  le.data->lat.emplace_back(lat);
	  le.data->lon.emplace_back(lon);
	  COOP_table.insert(le);
	}
	else {
	  le.data->start.emplace_back(start);
	  le.data->end.emplace_back(end);
	  le.data->lat.emplace_back(lat);
	  le.data->lon.emplace_back(lon);
	  COOP_table.replace(le);
	}
    }
    ifs.getline(line,32768);
  }
  ifs.close();
  remove("/tmp/COOP.TXT.latest");
}

void TD32Data::fill_WBAN_library()
{
  auto wban_library=unixutils::remote_web_file("http://rda.ucar.edu/datasets/ds900.1/station_libraries/WBAN.TXT-2006jan","/tmp");
  std::ifstream ifs(wban_library);
  if (!ifs.is_open()) {
    std::cerr << "Error opening WBAN.TXT-2006jan" << std::endl;
    exit(1);
  }
  char line[32768];
  ifs.getline(line,32768);
  while (!ifs.eof()) {
    TD32Data::LibEntry le;
    le.key.assign(&line[10],5);
    if (strutils::is_numeric(le.key)) {
	short yr,mo,dy;
	strutils::strget(&line[161],yr,4);
	strutils::strget(&line[165],mo,2);
	strutils::strget(&line[167],dy,2);
	DateTime start(yr,mo,dy,0,0);
	strutils::strget(&line[170],yr,4);
	strutils::strget(&line[174],mo,2);
	strutils::strget(&line[176],dy,2);
	DateTime end(yr,mo,dy,2400,0);
	int deg,min,sec;
	strutils::strget(&line[180],deg,2);
	strutils::strget(&line[183],min,2);
	strutils::strget(&line[186],sec,2);
	auto lat=deg+min/60.+sec/3600.;
	if (line[179] == '-') {
	  lat=-lat;
	}
	strutils::strget(&line[190],deg,3);
	strutils::strget(&line[194],min,2);
	strutils::strget(&line[197],sec,2);
	auto lon=deg+min/60.+sec/3600.;
	if (line[189] == '-') {
	  lon=-lon;
	}
	if (!floatutils::myequalf(lat,0.) && !floatutils::myequalf(lon,0.)) {
	  if (!WBAN_table.found(le.key,le)) {
	    le.data.reset(new LibEntry::Data);
	    le.data->start.emplace_back(start);
	    le.data->end.emplace_back(end);
	    le.data->lat.emplace_back(lat);
	    le.data->lon.emplace_back(lon);
	    WBAN_table.insert(le);
	  }
	  else {
	    le.data->start.emplace_back(start);
	    le.data->end.emplace_back(end);
	    le.data->lat.emplace_back(lat);
	    le.data->lon.emplace_back(lon);
	  }
	}
    }
    ifs.getline(line,32768);
  }
  ifs.close();
  remove("/tmp/WBAN.TXT-2006jan");
}
