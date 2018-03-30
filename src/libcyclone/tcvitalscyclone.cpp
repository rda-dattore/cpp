#include <cyclone.hpp>
#include <strutils.hpp>

int InputTCVitalsCycloneStream::peek()
{
return -1;
}

int InputTCVitalsCycloneStream::read(unsigned char *buffer,size_t buffer_length)
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
  if (sbuf.length() < 95 || sbuf.length() > 155) {
    return bfstream::error;
  }
  std::copy(sbuf.begin(),sbuf.end(),buffer);
  buffer[sbuf.length()]='\0';
  ++num_read;
  return sbuf.length();
}

void TCVitalsCyclone::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  std::string sbuf=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  storm_number_=std::stoi(sbuf.substr(5,2));
  basin_ID_=sbuf[7];
  ID_=sbuf.substr(9,9);
  strutils::trim(ID_);
  fix_data_.resize(1);
  fix_data_[0].classification="tropical";
  fix_data_[0].datetime.set(std::stoll(sbuf.substr(19,8))*1000000+std::stoll(sbuf.substr(28,4))*100);
  fix_data_[0].latitude=std::stoi(sbuf.substr(33,3))/10.;
  if (sbuf[36] == 'S') {
    fix_data_[0].latitude=-fix_data_[0].latitude;
  }
  fix_data_[0].longitude=std::stoi(sbuf.substr(38,4))/10.;
  if (sbuf[42] == 'W') {
    fix_data_[0].longitude=-fix_data_[0].longitude;
  }
  fix_data_[0].min_pres=std::stoi(sbuf.substr(52,4));
  fix_data_[0].max_wind=std::stoi(sbuf.substr(67,2));
}

void TCVitalsCyclone::print(std::ostream& outs) const
{
}

void TCVitalsCyclone::print_header(std::ostream& outs,bool verbose) const
{
  std::cout.setf(std::ios::fixed);
  std::cout.precision(1);
  std::cout << "ID: " << ID_ << "  Date: " << fix_data_[0].datetime.to_string("%Y-%m-%d %H:%MM") << "  Classification: " << fix_data_[0].classification << "  Lat: " << fix_data_[0].latitude << "  Lon: " << fix_data_[0].longitude << "  Central Press: " << fix_data_[0].min_pres << "mb  Max Wind: " << fix_data_[0].max_wind << "m/sec" << std::endl;
}
