#include <iomanip>
#include <aircraft.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

void InputDATSAVAircraftReportStream::close()
{
  idstream::close();
  file_buf_pos=0;
  file_buf_len=0;
}

int InputDATSAVAircraftReportStream::ignore()
{
  auto rec_len=peek();
  if (rec_len == bfstream::error || rec_len == bfstream::eof) {
    return rec_len;
  }
  file_buf_pos+=rec_len;
  ++num_read;
  return rec_len;
}

int InputDATSAVAircraftReportStream::peek()
{
  if (file_buf_pos == file_buf_len) {
    if (icosstream != nullptr) {
	return icosstream->peek();
    }
    else {
	char buf[2];
	fs.read(buf,2);
	if (!fs.good()) {
	  if (fs.eof()) {
	    return bfstream::eof;
	  }
	  else {
	    return bfstream::error;
	  }
	}
	int len;
	bits::get(buf,len,0,16);
	fs.seekg(-2,std::ios::cur);
	return len;
    }
  }
  else {
    int rec_len;
    bits::get(&file_buf[file_buf_pos],rec_len,0,16);
    return rec_len;
  }
}

int InputDATSAVAircraftReportStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (file_buf_pos == file_buf_len) {
    if (icosstream != nullptr) {
	file_buf_len=icosstream->read(file_buf.get(),9994);
	if (file_buf_len == bfstream::error || file_buf_len == bfstream::eof) {
	  return file_buf_len;
	}
    }
    else {
	fs.read(reinterpret_cast<char *>(file_buf.get()),2);
	if (!fs.good()) {
	  if (fs.eof()) {
	    return bfstream::eof;
	  }
	  else {
	    return bfstream::error;
	  }
	}
	bits::get(file_buf.get(),file_buf_len,0,16);
	fs.read(reinterpret_cast<char *>(&file_buf[2]),file_buf_len-2);
    }
    file_buf_pos=4;
  }
  int rec_len;
  bits::get(&file_buf[file_buf_pos],rec_len,0,16);
  int num_copied;
  if (rec_len > static_cast<int>(buffer_length)) {
    num_copied=buffer_length;
  }
  else {
    num_copied=rec_len;
  }
  std::copy(&file_buf[file_buf_pos],&file_buf[file_buf_pos+num_copied],buffer);
  file_buf_pos+=rec_len;
  ++num_read;
  return num_copied;
}

void OutputDATSAVAircraftReportStream::close()
{
  bits::set(file_buf.get(),file_buf_pos,0,16);
  int status=0;
  if (ocosstream != nullptr) {
    status=ocosstream->write(file_buf.get(),file_buf_pos);
  }
  else if (orptstream != nullptr) {
    status=orptstream->write(file_buf.get(),file_buf_pos);
  }
  else if (ocrptstream != nullptr) {
    status=ocrptstream->write(file_buf.get(),file_buf_pos);
  }
  else {
    if (!fs.is_open()) {
	std::cerr << "Error: no open output stream" << std::endl;
	exit(1);
    }
    fs.write(reinterpret_cast<char *>(file_buf.get()),file_buf_pos);
    if (!fs.good()) {
	status=bfstream::error;
    }
  }
  if (status == bfstream::error) {
    std::cerr << "Error closing output stream" << std::endl;
    exit(1);
  }
  odstream::close();
  file_buf_pos=4;
}

int OutputDATSAVAircraftReportStream::write(const unsigned char *buffer,size_t buffer_length)
{
// if the block is full, write it to disk before adding the next report
  if ( (file_buf_pos+buffer_length) > 9994) {
    bits::set(file_buf.get(),file_buf_pos,0,16);
    int status=0;
    if (ocosstream != nullptr) {
	status=ocosstream->write(file_buf.get(),file_buf_pos);
    }
    else if (orptstream != nullptr) {
	status=orptstream->write(file_buf.get(),file_buf_pos);
    }
    else if (ocrptstream != nullptr) {
	status=ocrptstream->write(file_buf.get(),file_buf_pos);
    }
    else {
	if (!fs.is_open()) {
	  std::cerr << "Error: no open output stream" << std::endl;
	  exit(1);
	}
	fs.write(reinterpret_cast<char *>(file_buf.get()),file_buf_pos);
	if (!fs.good()) {
	  status=bfstream::error;
	}
    }
    if (status == bfstream::error) {
	return status;
    }
    file_buf_pos=4;
  }
  std::copy(&buffer[0],&buffer[buffer_length],&file_buf[file_buf_pos]);
  file_buf_pos+=buffer_length;
  ++num_written;
  return buffer_length;
}

void DATSAVAircraftReport::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
// unpack mandatory section
  size_t naddl;
  bits::get(stream_buffer,naddl,32,16);
  naddl*=8;
  short yr,mo,dy,hr,min;
  bits::get(stream_buffer,yr,96,16);
  bits::get(stream_buffer,mo,112,16);
  bits::get(stream_buffer,dy,128,16);
  bits::get(stream_buffer,hr,144,16);
  bits::get(stream_buffer,min,160,16);
  date_time_.set(1900+yr,mo,dy,hr*10000+min*100);
  synoptic_date_time_=date_time_;
  if (date_time_.time() <= 30000 || date_time_.time() > 210000) {
    synoptic_date_time_.set_time(0);
    if (date_time_.time() > 210000) {
	synoptic_date_time_.add_days(1);
    }
  }
  if (date_time_.time() > 30000 && date_time_.time() <= 90000) {
    synoptic_date_time_.set_time(60000);
  }
  else if (date_time_.time() > 90000 && date_time_.time() <= 150000) {
    synoptic_date_time_.set_time(120000);
  }
  else if (date_time_.time() > 150000 && date_time_.time() <= 210000) {
    synoptic_date_time_.set_time(180000);
  }
  bits::get(stream_buffer,datsav.rpt_type,176,16);
  bits::get(stream_buffer,datsav.ob_type,192,16);
  short lat;
  bits::get(stream_buffer,lat,208,16);
  if (lat == static_cast<short>(0xffff)) {
    location_.latitude=-99.99;
  }
  else {
    if (lat < 0) {
	++lat;
    }
    auto deg=lat/100;
    auto min=lat % 100;
    location_.latitude=deg+static_cast<float>(min)/60.;
  }
  int lon;
  bits::get(stream_buffer,lon,224,32);
  if (static_cast<size_t>(lon) == 0xffffffff) {
    location_.longitude=-999.99;
  }
  else {
    if (lon < 0) {
	++lon;
    }
    auto deg=lon/100;
    auto min=lon % 100;
    location_.longitude=(deg+static_cast<float>(min)/60.);
// convert from positive west longitude to normal east longitude convention
    location_.longitude=-location_.longitude;
  }
  bits::get(stream_buffer,location_.altitude,256,16);
  char id[6];
  bits::get(stream_buffer,id,288,8,0,6);
  charconversions::ebcdic_to_ascii(id,id,6);
  location_.ID.assign(id,6);
  strutils::trim(location_.ID);
// mandatory section
  bits::get(stream_buffer,data.wind_direction,448,16);
  if (data.wind_direction == 0xffff) {
    data.wind_direction=999;
  }
  short sval,sval_miss=0xffff;
  bits::get(stream_buffer,sval,464,16);
  if (sval == sval_miss) {
    data.wind_speed=-99.9;
  }
  else {
    data.wind_speed=sval/10.;
  }
  bits::get(stream_buffer,sval,480,16);
  if (sval == sval_miss) {
    data.pressure=9999;
  }
  else {
    data.pressure=sval;
  }
  short sign;
  bits::get(stream_buffer,sign,512,1);
  bits::get(stream_buffer,datsav.alt_dval,513,15);
  if (datsav.alt_dval == 0x7fff) {
    datsav.alt_dval=-9999;
  }
  else if (sign == 1) {
    datsav.alt_dval=-datsav.alt_dval;
  }
  bits::get(stream_buffer,sval,528,16);
  if (sval == sval_miss) {
    data.temperature=-99.9;
  }
  else {
    data.temperature=sval/10.;
  }
  bits::get(stream_buffer,sval,544,16);
  if (sval == sval_miss) {
    data.moisture=-99.9;
  }
  else {
    data.moisture=sval/10.;
  }
  bits::get(stream_buffer,sign,560,1);
  bits::get(stream_buffer,datsav.alt_pl,561,15);
  if (datsav.alt_pl == 0x7fff) {
    datsav.alt_pl=-9999;
  }
  else if (sign == 1) {
    datsav.alt_pl=-datsav.alt_pl;
  }
  bits::get(stream_buffer,datsav.turbulence.frequency,576,16);
  if (datsav.turbulence.frequency == sval_miss) {
    datsav.turbulence.frequency=-9;
  }
  bits::get(stream_buffer,datsav.turbulence.intensity,592,16);
  if (datsav.turbulence.intensity == sval_miss) {
    datsav.turbulence.intensity=-9;
  }
  bits::get(stream_buffer,datsav.turbulence.type,608,16);
  if (datsav.turbulence.type == sval_miss) {
    datsav.turbulence.type=-9;
  }
// unpack additional data sections
  size_t off=0;
  datsav.sfc_dd=datsav.sfc_p=0xffff;
  while (off < naddl) {
    size_t ind;
    bits::get(stream_buffer,ind,640+off,16);
    switch (ind) {
	case 5:
	{
	  bits::get(stream_buffer,datsav.sfc_dd,640+off+48,16);
	  if (datsav.sfc_dd == 0xffff) {
	    datsav.sfc_dd=999;
	  }
	  else {
	    datsav.sfc_dd*=10;
	  }
	  bits::get(stream_buffer,datsav.sfc_ff,640+off+64,16);
	  if (datsav.sfc_ff == 0xffff) {
	    datsav.sfc_ff=999;
	  }
	  break;
	}
	case 7:
	{
	  bits::get(stream_buffer,datsav.sfc_p,640+off+16,16);
	  if (datsav.sfc_p == 0xffff) {
	    datsav.sfc_p=999;
	  }
	  bits::get(stream_buffer,datsav.altim,640+off+32,16);
	  if (datsav.altim == 0xffff) {
	    datsav.altim=999;
	  }
	  bits::get(stream_buffer,datsav.hdg,640+off+48,16);
	  if (datsav.hdg == 0xffff) {
	    datsav.hdg=999;
	  }
	  else {
	    datsav.hdg*=10;
	  }
	  break;
	}
    }
    off+=128;
  }
}

void DATSAVAircraftReport::print() const
{
  print_header();
  std::cout.precision(1);
  std::cout << "  DD: " << std::setw(3) << data.wind_direction << " FF: " << std::setw(5) << data.wind_speed << "  Pres: " << data.pressure << "  Alt. D-val: " << datsav.alt_dval << "  Temp: " << std::setw(5) << data.temperature << "  DPD: " << std::setw(4) << data.moisture << "  PL Alt: " << datsav.alt_pl << "  Turbulence (freq/intens/typ): " << datsav.turbulence.frequency << "/" << datsav.turbulence.intensity << "/" << datsav.turbulence.type << std::endl;
  if (datsav.sfc_dd != 0xffff) {
    std::cout << "  Surface DD: " << std::setw(3) << datsav.sfc_dd << " Surface FF: " << std::setw(5) << datsav.sfc_ff << std::endl;
  }
  if (datsav.sfc_p != 0xffff) {
    std::cout << "  Surface Pressure: " << std::setw(5) << datsav.sfc_p << "  Altimeter: " << std::setw(5) << datsav.altim << "  Aircraft Heading: " << std::setw(3) << datsav.hdg << std::endl;
  }
}

void DATSAVAircraftReport::print_header() const
{
  std::cout.setf(std::ios::fixed);
  std::cout.precision(2);
  std::cout << " Type (rpt/ob): " << datsav.rpt_type << "/" << datsav.ob_type << " Synoptic Date: " << synoptic_date_time_.to_string() << "  Date/Time: " << date_time_.to_string() << "  ID: " << location_.ID << "  Lat: " << std::setw(6) << location_.latitude << " Lon: " << std::setw(7) << location_.longitude;
  std::cout.precision(1);
  std::cout << " Alt: " << std::setw(7) << location_.altitude << std::endl;
}
