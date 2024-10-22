#include <regex>
#include <grid.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>
#include <myerror.hpp>

int InputGPCPGridStream::ignore()
{
return bfstream::error;
}

void InputGPCPGridStream::read_header()
{
  if (num_read == 0 && hdr_size == 0) {
    char buf[80];
    fs.read(reinterpret_cast<char *>(buf),80);
    if (fs.gcount() != 80) {
	myerror="header read error (1)";
	exit(1);
    }
    std::string test;
    test.assign(buf,80);
    size_t idx;
    if (!std::regex_search(test,std::regex("^size=\\(char\\*")) || (idx=test.find(")")) == std::string::npos || idx < 12) {
	myerror="invalid or missing header";
	exit(1);
    }
    hdr_size=std::stoi(test.substr(11,idx-11));
    if ( (idx=test.find("(real*4)x")) == std::string::npos) {
	myerror="header read error (2)";
	exit(1);
    }
    size_t idx2;
    if ( (idx2=test.find(" data file")) == std::string::npos || idx2 < idx) {
	myerror="header read error (3)";
	exit(1);
    }
    auto sp=strutils::split(test.substr(idx+9,idx2-idx+1),"x");
    if (sp.size() != 3) {
	myerror="header read error (4)";
	exit(1);
    }
    rec_size=std::stoi(sp[0])*std::stoi(sp[1])*4;
    header.reset(new unsigned char[hdr_size]);
    std::copy(&buf[0],&buf[80],header.get());
    auto s=strutils::itos(hdr_size);
    s.insert(0,6-s.length(),' ');
    std::copy(s.begin(),s.end(),&header[0]);
    sp[0].insert(0,6-sp[0].length(),' ');
    std::copy(sp[0].begin(),sp[0].end(),&header[6]);
    sp[1].insert(0,6-sp[1].length(),' ');
    std::copy(sp[1].begin(),sp[1].end(),&header[12]);
    fs.read(reinterpret_cast<char *>(&header[80]),hdr_size-80);
    if (static_cast<size_t>(fs.gcount()) != (hdr_size-80)) {
	myerror="header read error (5)";
	exit(1);
    }
  }
}

int InputGPCPGridStream::peek()
{
  if (eof_offset == 0) {
    auto curr_offset=fs.tellg();
    fs.seekg(std::ios_base::end);
    eof_offset=fs.tellg();
    fs.seekg(curr_offset,std::ios_base::beg);
  }
  if (num_read == 0 && hdr_size == 0) {
    read_header();
  }
  size_t curr_offset=fs.tellg();
  if (curr_offset == eof_offset) {
    return bfstream::eof;
  }
  else if ( (curr_offset+rec_size) <= eof_offset) {
    return hdr_size+rec_size;
  }
  else {
    return bfstream::error;
  }
}

int InputGPCPGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (!fs.is_open()) {
    myerror="no open file stream";
    exit(1);
  }
  if (num_read == 0 && hdr_size == 0) {
    read_header();
  }
  if (buffer_length < (hdr_size+rec_size)) {
    myerror="buffer overflow";
    exit(1);
  }
  fs.read(reinterpret_cast<char *>(&buffer[hdr_size]),rec_size);
  size_t bytes_read=fs.gcount();
  if (bytes_read == rec_size) {
    ++num_read;
    auto s=strutils::itos(num_read);
    s.insert(0,6-s.length(),' ');
    std::copy(s.begin(),s.end(),&header[18]);
    std::copy(&header[0],&header[hdr_size],&buffer[0]);
    bytes_read+=hdr_size;
    return bytes_read;
  }
  else if (bytes_read == 0) {
    return bfstream::eof;
  }
  else {
    return bfstream::error;
  }
}

size_t GPCPGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void GPCPGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  size_t idx,len;

  is_empty=true;
  std::string s;
  s.assign(reinterpret_cast<const char *>(stream_buffer),24);
  auto hdr_size=std::stoi(s.substr(0,6));
  dim.x=std::stoi(s.substr(6,6));
  dim.y=std::stoi(s.substr(12,6));
  dim.size=dim.x*dim.y;
  auto rec_num=std::stoi(s.substr(18,6));
  std::string header;
  header.assign(reinterpret_cast<const char *>(stream_buffer),hdr_size);
  idx=header.find("grid=")+5;
  len=header.substr(idx).find(" ");
  auto sp=strutils::split(header.substr(idx,len),"x");
  def.dx=std::stof(sp[0]);
  def.dy=std::stof(sp[1]);
  idx=header.find("1st_box_center=(")+16;
  len=header.substr(idx).find(")");
  sp=strutils::split(header.substr(idx,len),",");
  def.slatitude=std::stof(sp[0]);
  if (sp[0].back() == 'S') {
    def.slatitude=-def.slatitude;
  }
  def.slongitude=std::stof(sp[1]);
  if (sp[0].back() == 'W') {
    def.slongitude=-def.slongitude;
  }
  idx=header.find("last_box_center=(")+17;
  len=header.substr(idx).find(")");
  sp=strutils::split(header.substr(idx,len),",");
  def.elatitude=std::stof(sp[0]);
  if (sp[0].back() == 'S') {
    def.elatitude=-def.elatitude;
  }
  def.elongitude=std::stof(sp[1]);
  if (sp[0].back() == 'W') {
    def.elongitude=-def.elongitude;
  }
  if (std::regex_search(header,std::regex("lon/lat 1st_box_center"))) {
    def.type=Grid::Type::latitudeLongitude;
  }
  idx=header.find("year=");
  auto yr=std::stoi(header.substr(idx+5,4));
  if (std::regex_search(header,std::regex("days="))) {
    idx=header.find("month=");
    auto mo=std::stoi(header.substr(idx+6,2));
    m_reference_date_time.set(yr,mo,rec_num,0);
    m_valid_date_time=m_reference_date_time.days_added(1);
  }
  else {
    m_reference_date_time.set(yr,rec_num,1,0);
    m_valid_date_time=m_reference_date_time.months_added(1);
  }
  idx=header.find("variable=");
  param_name=header.substr(idx+9,header.substr(idx+9).find(" technique="));
  idx=header.find("units=");
  param_units=header.substr(idx+6,header.substr(idx+6).find(" year="));
  if (fill_header_only) {
    return;
  }
  union {
    int *idum;
    float *fdum;
  };
  idum=new int[dim.size];
  if (x_capacity < dim.x || y_capacity < dim.y) {
    if (m_gridpoints != nullptr) {
	for (int n=0; n < y_capacity; ++n) {
	  delete[] m_gridpoints[n];
	}
	delete[] m_gridpoints;
    }
    x_capacity=dim.x;
    y_capacity=dim.y;
    m_gridpoints=new double *[y_capacity];
    for (int n=0; n < y_capacity; ++n) {
	m_gridpoints[n]=new double[x_capacity];
    }
  }
  bits::get(stream_buffer,&idum[0],hdr_size*8,32,0,dim.size);
  size_t cnt=0;
  for (int n=0; n < dim.y; ++n) {
    for (int m=0; m < dim.x; ++m) {
	m_gridpoints[n][m]=fdum[cnt++];
	if (is_empty && !floatutils::myequalf(m_gridpoints[n][m],-99999.)) {
	  is_empty=false;
	}
    }
  }
  delete[] idum;
}

void GPCPGrid::print(std::ostream& outs) const
{
}

void GPCPGrid::print(std::ostream& outs,float start_latitude,float end_latitude,float start_longitude,float end_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void GPCPGrid::print_ascii(std::ostream& outs) const
{
}

void GPCPGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
}
