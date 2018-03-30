#include <cyclone.hpp>
#include <strutils.hpp>

int InputHURDATCycloneStream::peek()
{
return -1;
}

int InputHURDATCycloneStream::read(unsigned char *buffer,size_t buffer_length)
{
  char line[256];

  fs.getline(line,256);
  if (fs.eof()) {
    return bfstream::eof;
  }
  else if (!fs.good()) {
    return bfstream::error;
  }
  std::string sbuf=line;
  size_t nhist=std::stoi(sbuf.substr(19,2));
  for (size_t n=0; n <= nhist; ++n) {
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (!fs.good()) {
	return bfstream::error;
    }
    sbuf+=line;
  }
  strutils::trim(sbuf);
  std::copy(sbuf.begin(),sbuf.end(),buffer);
  buffer[sbuf.length()]='\0';
  return sbuf.length();
}

void HURDATCyclone::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  std::string sbuf=reinterpret_cast<const char *>(stream_buffer);
  auto buf_parts=strutils::split(sbuf,"\n");
  auto yr=std::stoi(buf_parts[0].substr(12,4));
  ID_=buf_parts[0].substr(35,11);
  strutils::trim(ID_);
  if (ID_ == "NOT NAMED" || ID_ == "UNKNOWN") {
    auto s=buf_parts[0].substr(22,2);
    strutils::replace_all(s," ","0");
    ID_+="-"+s;
  }
  if (buf_parts[0].substr(47,4) == "XING") {
    coast_cross=buf_parts[0][52]+48;
  }
  else {
    coast_cross=-1;
  }
  if (buf_parts[0].substr(54,3) == "SSS") {
    sss=buf_parts[0][58]+48;
  }
  else {
    sss=-1;
  }
  lat_h=buf_parts[0][64];
  float lat;
  if (lat_h == 'N') {
    lat=1.;
  }
  else {
    lat=-1.;
  }
  lon_h=buf_parts[0][70];
  float lon;
  if (lon_h == 'W') {
    lon=-1.;
  }
  else {
    lon=1.;
  }
  type_=buf_parts.back().substr(6,2);
  fix_data_.clear();
  short lastmo=0;
  for (size_t n=1; n < buf_parts.size()-1; n++) {
    auto mo=std::stoi(buf_parts[n].substr(6,2));
    if (mo < lastmo) {
	++yr;
    }
    lastmo=mo;
    auto dy=std::stoi(buf_parts[n].substr(9,2));
    auto off=11;
    for (size_t m=0; m < 4; m++) {
	if (buf_parts[n][off] != ' ') {
	  FixData fd;
	  if (buf_parts[n][off] == '*' || buf_parts[n][off] == 'X') {
	    fd.classification="tropical";
	  }
	  else if (buf_parts[n][off] == 'E') {
	    fd.classification="extratropical";
	  }
	  else if (buf_parts[n][off] == 'G') {
	    fd.classification="extratropical_gale";
	  }
	  else if (buf_parts[n][off] == 'L') {
	    fd.classification="remnant_low";
	  }
	  else if (buf_parts[n][off] == 'P') {
	    fd.classification="post_tropical";
	  }
	  else if (buf_parts[n][off] == 'S') {
	    fd.classification="subtropical";
	  }
	  else if (buf_parts[n][off] == 'W') {
	    fd.classification="open_wave";
	  }
	  else {
	    fd.classification="unknown-"+buf_parts[n].substr(off,1);
	  }
	  fd.datetime.set(yr,mo,dy,m*60000);
	  fd.latitude=lat*std::stof(buf_parts[n].substr(off+1,3))/10.;
	  fd.longitude=lon*std::stof(buf_parts[n].substr(off+4,4))/10.;
	  if (fd.longitude < -180.) {
	    fd.longitude+=360.;
	  }
	  else if (fd.longitude > 180.) {
	    fd.longitude-=360.;
	  }
	  auto max_wind=buf_parts[n].substr(off+8,4);
	  if (max_wind != "    ") {
	    fd.max_wind=std::stof(max_wind);
	  }
	  else {
	    fd.max_wind=-999;
	  }
	  auto min_pres=buf_parts[n].substr(off+12,5);
	  if (min_pres != "     ") {
	    fd.min_pres=std::stof(min_pres);
	  }
	  else {
	    fd.min_pres=-9999;
	  }
	  fix_data_.emplace_back(fd);
	}
	off+=17;
    }
  }
}

void HURDATCyclone::print(std::ostream& outs) const
{
}

void HURDATCyclone::print_header(std::ostream& outs,bool verbose) const
{
}
