#include <little_r.hpp>
#include <strutils.hpp>

int InputLITTLE_RStream::ignore()
{
return -1;
}

int InputLITTLE_RStream::peek()
{
  static std::unique_ptr<char []> line_buffer;
  if (line_buffer == nullptr) {
    line_buffer.reset(new char[2048]);
  }
  auto start_off=fs.tellg();
// read to the end record
  fs.getline(line_buffer.get(),2048);
  while (std::string(line_buffer.get(),13) != "-777777.00000") {
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (fs.bad()) {
	return bfstream::error;
    }
    fs.getline(line_buffer.get(),2048);
  }
// read the tail integers
  fs.getline(line_buffer.get(),2048);
  if (fs.eof()) {
    return bfstream::eof;
  }
  else if (fs.bad()) {
    return bfstream::error;
  }
  auto curr_off=fs.tellg();
  fs.seekg(start_off,std::ios_base::beg);
  return (curr_off-start_off+3);
}

int InputLITTLE_RStream::read(unsigned char *buffer,size_t buffer_length)
{
  static std::unique_ptr<char []> line_buffer;
  if (line_buffer == nullptr) {
    line_buffer.reset(new char[2048]);
  }
  auto last_off=fs.tellg();
// read to the end record
  fs.getline(line_buffer.get(),2048);
  int boff=4;
  while (1) {
    std::string check;
    check.assign(line_buffer.get(),13);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (fs.bad()) {
	return bfstream::error;
    }
    auto curr_off=fs.tellg();
    size_t len=curr_off-last_off-1;
    last_off=curr_off;
    if ( (boff+len) > buffer_length) {
	return bfstream::error;
    }
    std::copy(&line_buffer[0],&line_buffer[len],&buffer[boff]);
    boff+=len;
    buffer[boff++]='\n';
    fs.getline(line_buffer.get(),2048);
    if (check == "-777777.00000") {
	break;
    }
  }
  if (fs.eof()) {
    return bfstream::eof;
  }
  else if (fs.bad()) {
    return bfstream::error;
  }
  auto curr_off=fs.tellg();
  size_t len=curr_off-last_off-1;
  if ( (boff+len) > buffer_length) {
    return bfstream::error;
  }
  std::copy(&line_buffer[0],&line_buffer[len],&buffer[boff]);
  boff+=len;
  buffer[0]=(boff >> 24);
  buffer[1]=((boff >> 16) & 0xff);
  buffer[2]=((boff >> 8) & 0xff);
  buffer[3]=(boff  & 0xff);
  ++num_read;
  return boff;
}

void LITTLE_RObservation::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int rec_len=(stream_buffer[0] << 24)+(stream_buffer[1] << 16)+(stream_buffer[2] << 8)+stream_buffer[3];
  auto records=strutils::split(std::string(&(reinterpret_cast<const char *>(stream_buffer))[4],rec_len),"\n");
  location_.latitude=std::stof(records.front().substr(0,20));
  location_.longitude=std::stof(records.front().substr(20,20));
  location_.ID=records.front().substr(40,40);
  if (location_.longitude > 180.) {
    location_.longitude-=360.;
  }
  platform_code_=std::stoi(records.front().substr(123,3));
  if (records.front()[279] == 'T') {
    is_sounding_=true;
  }
  else {
    is_sounding_=false;
  }
  auto yr=std::stoi(records.front().substr(326,4));
  auto mo=std::stoi(records.front().substr(330,2));
  auto dy=std::stoi(records.front().substr(332,2));
  auto hhmmss=std::stoi(records.front().substr(334,6));
  date_time_.set(yr,mo,dy,hhmmss);
  if (!fill_header_only) {
    records.pop_front();
    records.pop_back();
    records.pop_back();
    for (const auto& r : records) {
	data_records_.emplace_back(std::make_tuple(std::stof(r.substr(0,13)),std::stoi(r.substr(13,7)),std::stof(r.substr(20,13)),std::stoi(r.substr(33,7)),std::stof(r.substr(40,13)),std::stoi(r.substr(53,7)),std::stof(r.substr(60,13)),std::stoi(r.substr(73,7)),std::stof(r.substr(80,13)),std::stoi(r.substr(93,7)),std::stof(r.substr(100,13)),std::stoi(r.substr(113,7)),std::stof(r.substr(120,13)),std::stoi(r.substr(133,7)),std::stof(r.substr(140,13)),std::stoi(r.substr(153,7)),std::stof(r.substr(160,13)),std::stoi(r.substr(173,7)),std::stof(r.substr(180,13)),std::stoi(r.substr(193,7))));
    }
  }
}

DateTime LITTLE_RObservation::date_time() const
{
  return date_time_;
}

Observation::ObservationLocation LITTLE_RObservation::location() const
{
  return location_;
}
