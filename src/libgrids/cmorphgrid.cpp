#include <bfstream.hpp>
#include <grid.hpp>
#include <utils.hpp>
#include <myerror.hpp>

bool InputCMORPH025GridStream::open(std::string filename)
{
  size_t idx;
  if ( (idx=filename.find("_3hr-025deg_cpc+comb")) != std::string::npos) {
    m_date_time.set(std::stoll(filename.substr(idx-8,8))*1000000);
    type=1;
  }
  else if ( (idx=filename.find("CMORPH+MWCOMB_3HRLY-025DEG_")) != std::string::npos) {
    m_date_time.set(std::stoll(filename.substr(idx+27,8))*1000000);
    type=1;
  }
  else if ( (idx=filename.find("_RAW_0.25deg-3HLY_")) != std::string::npos || (idx=filename.find("_ADJ_0.25deg-3HLY_")) != std::string::npos) {
    m_date_time.set(std::stoll(filename.substr(idx+18,8))*1000000);
    type=2;
  }
  else {
    myerror="filename '"+filename+"' is not recognized";
    exit(1);
  }
  return idstream::open(filename);
}

int InputCMORPH025GridStream::peek()
{
  unsigned char *buffer=new unsigned char[2764800];
  int bytes_read=read(buffer,2764800);
  if (bytes_read > 0) {
    fs.seekg(-bytes_read,std::ios_base::cur);
    --num_read;
  }
  delete[] buffer;
  return bytes_read;
}

int InputCMORPH025GridStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (!fs.is_open()) {
    myerror="no open file stream";
    exit(1);
  }
  if (buffer_length < 2764800) {
    myerror="buffer overflow";
    exit(1);
  }
  fs.read(reinterpret_cast<char *>(buffer),2764800);
  int bytes_read=fs.gcount();
  if (bytes_read == 2764800) {
    switch (type) {
	case 1:
	{
	  m_date_time.set_time((num_read/2)*30000);
	  if ( (num_read % 2) == 0) {
	    parameter.code="MMP";
	    parameter.description="Merged microwave precipitation rate";
	    parameter.units="mm/hr";
	  }
	  else {
	    parameter.code="CMORPH";
	    parameter.description="CMORPH precipitation rate estimate";
	    parameter.units="mm/hr";
	  }
	  latitude.start=59.875;
	  latitude.end=-59.875;
	  break;
	}
	case 2:
	{
	  m_date_time.set_time(num_read*30000);
	  parameter.code="CMORPHA";
	  parameter.description="CMORPH accumulated precipitation estimate";
	  parameter.units="mm";
	  latitude.start=-59.875;
	  latitude.end=59.875;
	  break;
	}
    }
    ++num_read;
    return bytes_read;
  }
  else if (bytes_read == 0) {
    return bfstream::eof;
  }
  else {
    return bfstream::error;
  }
}

CMORPH025Grid::CMORPH025Grid()
{
  dim.x=1440;
  dim.y=480;
  dim.size=dim.x*dim.y;
  def.type=Grid::Type::latitudeLongitude;
  def.slatitude=def.elatitude=0.;
  def.laincrement=0.25;
  def.slongitude=0.125;
  def.elongitude=359.875;
  def.loincrement=0.25;
}

size_t CMORPH025Grid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void CMORPH025Grid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  if (def.slatitude == 0 && def.elatitude == 0) {
    myerror="start and end latitudes not set";
    exit(1);
  }
  if (!fill_header_only) {
    if (m_gridpoints == nullptr) {
	m_gridpoints=new double *[dim.y];
	for (short n=0; n < dim.y; ++n) {
	  m_gridpoints[n]=new double[dim.x];
	}
    }
    auto l=0;
    float *f=reinterpret_cast<float *>(const_cast<unsigned char *>(stream_buffer));
    for (short n=0; n < dim.y; ++n) {
	for (short m=0; m < dim.x; ++m) {
	  m_gridpoints[n][m]=f[l++];
	}
    }
  }
}

bool CMORPH025Grid::is_averaged_grid() const
{
return false;
}

void CMORPH025Grid::print(std::ostream& outs) const
{
}

void CMORPH025Grid::print(std::ostream& outs,float start_latitude,float end_latitude,float start_longitude,float end_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void CMORPH025Grid::print_ascii(std::ostream& outs) const
{
}

void CMORPH025Grid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  if (verbose) {
  }
  else {
    outs << "|VTime=" << m_valid_date_time.to_string("%Y%m%d%H%MM") << std::endl;
  }
}

void CMORPH025Grid::set_latitudes(float start_latitude,float end_latitude)
{
  def.slatitude=start_latitude;
  def.elatitude=end_latitude;
}

const size_t InputCMORPH8kmGridStream::RECORD_SIZE=32637008;

bool InputCMORPH8kmGridStream::open(std::string filename)
{
  size_t idx;
  if ( (idx=filename.find("_8km-30min_")) != std::string::npos) {
    m_date_time.set(std::stoll(filename.substr(idx+11,10))*10000);
  }
  else {
    myerror="filename '"+filename+"' is not recognized";
    exit(1);
  }
  return idstream::open(filename);
}

int InputCMORPH8kmGridStream::peek()
{
  fs.seekg(RECORD_SIZE-1,std::ios_base::cur);
  char c;
  fs.read(&c,1);
  if (fs.good()) {
    fs.seekg(-RECORD_SIZE,std::ios_base::cur);
    return RECORD_SIZE+12;
  }
  else {
    if (fs.eof()) {
	return bfstream::eof;
    }
    else {
	return bfstream::error;
    }
  }
}

int InputCMORPH8kmGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (!fs.is_open()) {
    myerror="no open file stream";
    exit(1);
  }
  if (buffer_length < (RECORD_SIZE+12)) {
    myerror="buffer overflow";
    exit(1);
  }
  fs.read(&(reinterpret_cast<char *>(buffer))[12],RECORD_SIZE);
  int bytes_read=fs.gcount();
  if (bytes_read == static_cast<int>(RECORD_SIZE)) {
    ++num_read;
    if ( (num_read % 2) == 0) {
	m_date_time.add_minutes(30);
    }
    auto date_str=m_date_time.to_string("%Y%m%d%H%MM");
    std::copy(date_str.begin(),date_str.end(),buffer);
    return bytes_read+12;
  }
  else if (bytes_read == 0) {
    return bfstream::eof;
  }
  else {
    return bfstream::error;
  }
}

CMORPH8kmGrid::CMORPH8kmGrid()
{
  dim.x=4948;
  dim.y=1649;
  dim.size=dim.x*dim.y;
  def.type=Grid::Type::latitudeLongitude;
  def.slatitude=-59.963614;
  def.elatitude=59.963614;
  def.laincrement=(def.elatitude-def.slatitude)/(dim.y-1);
  def.slongitude=0.036378335;
  def.elongitude=359.963621665;
  def.loincrement=(def.elongitude-def.slongitude)/(dim.x-1);
}


size_t CMORPH8kmGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void CMORPH8kmGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  m_reference_date_time.set(std::stoll(std::string(reinterpret_cast<const char *>(stream_buffer),12))*100);
  m_valid_date_time=m_reference_date_time.minutes_added(30);
  if (!fill_header_only) {
    if (m_gridpoints == nullptr) {
	m_gridpoints=new double *[dim.y];
	for (short n=0; n < dim.y; n++) {
	  m_gridpoints[n]=new double[dim.x];
	}
    }
    auto l=0;
    float *f=reinterpret_cast<float *>(const_cast<unsigned char *>(stream_buffer));
    for (short n=0; n < dim.y; ++n) {
	for (short m=0; m < dim.x; ++m) {
	  m_gridpoints[n][m]=f[l++];
	}
    }
  }
}

bool CMORPH8kmGrid::is_averaged_grid() const
{
  return true;
}

void CMORPH8kmGrid::print(std::ostream& outs) const
{
}

void CMORPH8kmGrid::print(std::ostream& outs,float start_latitude,float end_latitude,float start_longitude,float end_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void CMORPH8kmGrid::print_ascii(std::ostream& outs) const
{
}

void CMORPH8kmGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  if (verbose) {
  }
  else {
    outs << "|Rtime=" << m_reference_date_time.to_string("%Y%m%d%H%MM") << "|VTime=" << m_valid_date_time.to_string("%Y%m%d%H%MM") << std::endl;
  }
}
