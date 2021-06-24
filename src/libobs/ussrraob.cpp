#include <iomanip>
#include <raob.hpp>
#include <bsort.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputUSSRRaobStream::ignore()
{
  int bytes_read;

  if (!is_open()) {
    std::cerr << "Error: no InputUSSRRaobStream has been opened" << std::endl;
    exit(1);
  }
  if (ics != NULL)
    bytes_read=ics->ignore();
  else
    bytes_read=bfstream::error;
  if (bytes_read != bfstream::eof)
    num_read++;
  return bytes_read;
}

int InputUSSRRaobStream::peek()
{
  if (!is_open()) {
    std::cerr << "Error: no InputUSSRRaobStream has been opened" << std::endl;
    exit(1);
  }
  if (ics != NULL)
    return ics->peek();
  else
    return bfstream::error;
}

int InputUSSRRaobStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;

  if (!is_open()) {
    std::cerr << "Error: no InputUSSRRaobStream has been opened" << std::endl;
    exit(1);
  }
  if (ics != NULL)
    bytes_read=ics->read(buffer,buffer_length);
  else
    bytes_read=bfstream::error;
  if (bytes_read != bfstream::eof)
    num_read++;
  return bytes_read;
}

bool OutputUSSRRaobStream::open(const char *filename,iods::Blocking blocking_flag)
{
  if (blocking_flag == iods::Blocking::cos) {
    return odstream::open(filename,blocking_flag);
  }
  else {
    std::cerr << "Error in OutputUSSRRaobStream::open: only COS-blocked output is "
      << "currently supported" << std::endl;
    exit(1);
  }
  return false;
}

int OutputUSSRRaobStream::write(const unsigned char *buffer,size_t num_bytes)
{
  int status;

  if (!is_open()) {
    std::cerr << "Error: no OutputUSSRRaobStream has been opened" << std::endl;
    exit(1);
  }
  if (ocs != NULL) {
    status=ocs->write(buffer,num_bytes);
  }
  else {
    status=bfstream::error;
  }
  ++num_written;
  if (status == static_cast<int>(num_bytes)) {
    return num_bytes;
  }
  else {
    return status;
  }
}

size_t USSRRaob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length)
const
{
  size_t off;
  int n,dum;

  if (buffer_length < 4) {
    std::cerr << "Warning: buffer too small" << std::endl;
    return 0;
  }
  bits::set(output_buffer,std::stoi(location_.ID),0,32);
  if (uraob.mois_wind_type == 0) {
    if (buffer_length < 14) {
	std::cerr << "Warning: buffer too small" << std::endl;
	return 0;
    }
// pack the header
    bits::set(output_buffer,date_time_.year()-1900,32,16);
    bits::set(output_buffer,date_time_.month(),48,16);
    bits::set(output_buffer,date_time_.day(),64,16);
    bits::set(output_buffer,lroundf(date_time_.time()/10000.),80,16);
    bits::set(output_buffer,nlev,96,16);
// pack the level data
    off=112;
    for (n=0; n < nlev; ++n) {
	if (static_cast<int>(buffer_length) < (14+n*24)) {
	  std::cerr << "Warning: buffer too small" << std::endl;
	  return 0;
	}
	bits::set(output_buffer,levels[n].type,off,16);
	if (levels[n].pressure > 9999.)
	  dum=0x8000;
	else
	  dum=levels[n].pressure;
	bits::set(output_buffer,dum,off+16,16);
	if (levels[n].height < -9999)
	  bits::set(output_buffer,0x8000,off+32,16);
	else
	  bits::set(output_buffer,lroundf(levels[n].height/10.),off+32,16);
	bits::set(output_buffer,static_cast<int>(levels[n].height_quality),off+48,16);
	if (levels[n].temperature < -99.)
	  dum=0x8000;
	else {
	  dum=lround(levels[n].temperature*10.);
	  if (dum < 0)
	    dum+=0x10000;
	}
	bits::set(output_buffer,dum,off+64,16);
	bits::set(output_buffer,static_cast<int>(levels[n].temperature_quality),off+80,16);
	if (levels[n].moisture < -99.)
	  dum=0x8000;
	else
	  dum=lround(levels[n].moisture*100.);
	bits::set(output_buffer,dum,off+96,16);
	bits::set(output_buffer,static_cast<int>(levels[n].moisture_quality),off+112,16);
	if (levels[n].ucomp < -999.)
	  dum=0x8000;
	else {
	  dum=lround(levels[n].ucomp*10.);
	  if (dum < 0)
	    dum+=0x10000;
	}
	bits::set(output_buffer,dum,off+128,16);
	bits::set(output_buffer,static_cast<int>(levels[n].wind_quality),off+144,16);
	if (levels[n].vcomp < -999.)
	  dum=0x8000;
	else {
	  dum=lround(levels[n].vcomp*10.);
	  if (dum < 0)
	    dum+=0x10000;
	}
	bits::set(output_buffer,dum,off+160,16);
	bits::set(output_buffer,static_cast<int>(levels[n].wind_quality),off+176,16);
	off+=192;
    }
  }
  else {
    if (buffer_length < 31) {
	std::cerr << "Warning: buffer too small" << std::endl;
	return 0;
    }
// pack the header
    bits::set(output_buffer,date_time_.year()-1900,32,8);
    bits::set(output_buffer,date_time_.month(),40,8);
    bits::set(output_buffer,date_time_.day(),48,8);
    bits::set(output_buffer,uraob.syn_hour,56,8);
    if (location_.latitude < -99.)
	dum=0x8000;
    else {
	dum=lround(location_.latitude*100.);
	if (dum < 0)
	  dum+=0x10000;
    }
    bits::set(output_buffer,dum,64,16);
    if (location_.longitude < -999.)
	dum=0x8000;
    else {
	dum=lround((location_.longitude+475.36)*100.);
	if (dum < 0)
	  dum+=0x10000;
    }
    bits::set(output_buffer,dum,80,16);
    bits::set(output_buffer,uraob.mrdsq,96,16);
    bits::set(output_buffer,uraob.quad,112,8);
    bits::set(output_buffer,uraob.sond_type,120,8);
    bits::set(output_buffer,nlev,128,8);
    bits::set(output_buffer,lroundf(date_time_.time()/10000.),136,8);
    if (location_.elevation == -999) {
	dum=0x8000;
    }
    else {
	dum=location_.elevation;
	if (dum < 0) {
	  dum+=0x10000;
	}
    }
    bits::set(output_buffer,dum,144,16);
    bits::set(output_buffer,uraob.instru_type,160,8);
    off=168;
    for (n=0; n < 5; n++) {
	bits::set(output_buffer,uraob.cloud[0][n],off,8);
	bits::set(output_buffer,uraob.cloud[1][n],off+8,8);
	off+=16;
    }
// pack the level data
    off=248;
    for (n=0; n < nlev; ++n) {
	if (static_cast<int>(buffer_length) < (31+n*18)) {
	  std::cerr << "Warning: buffer too small" << std::endl;
	  return 0;
	}
	bits::set(output_buffer,levels[n].type,off,8);
	if (levels[n].pressure > 9999.) {
	  dum=0x8000;
	}
	else {
	  dum=lround(levels[n].pressure*10.);
	}
	bits::set(output_buffer,dum,off+8,16);
	if (levels[n].height < -9999) {
	  dum=0x8000;
	}
	else {
	  dum=levels[n].height-10000;
	  if (dum < 0) {
	    dum+=0x10000;
	  }
	}
	bits::set(output_buffer,dum,off+24,16);
	bits::set(output_buffer,static_cast<int>(levels[n].height_quality),off+40,8);
	if (levels[n].temperature < -99.) {
	  dum=0x8000;
	  bits::set(output_buffer,dum,off+48,16);
	  bits::set(output_buffer,dum,off+72,16);
	}
	else {
	  dum=lround(levels[n].temperature*10.);
	  if (dum < 0) {
	    dum+=0x10000;
	  }
	  bits::set(output_buffer,dum,off+48,16);
	  dum=lround((levels[n].temperature-levels[n].moisture)*10.);
	  bits::set(output_buffer,dum,off+72,16);
	}
	bits::set(output_buffer,static_cast<int>(levels[n].temperature_quality),off+64,8);
	bits::set(output_buffer,static_cast<int>(levels[n].moisture_quality),off+88,8);
	if (levels[n].wind_direction == 999) {
	  bits::set(output_buffer,0x8000,off+96,16);
	}
	else {
	  bits::set(output_buffer,levels[n].wind_direction,off+96,16);
	}
	bits::set(output_buffer,static_cast<int>(levels[n].wind_quality),off+112,8);
	if (levels[n].wind_speed > 990.) {
	  bits::set(output_buffer,0x8000,off+120,16);
	}
	else {
	  bits::set(output_buffer,static_cast<int>(levels[n].wind_speed),off+120,16);
	}
	bits::set(output_buffer,static_cast<int>(levels[n].wind_quality),off+136,8);
	off+=144;
    }
  }
  return off/8;
}

int USSRRaobCompare(Raob::RaobLevel& left,Raob::RaobLevel& right)
{
  if (left.pressure >= right.pressure) {
    return -1;
  }
  else {
    return 1;
  }
}

void USSRRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  short yr,mo,dy,time;
  size_t off,nl;
  int n,dum;
  bool unsorted=false;

  bits::get(stream_buffer,dum,0,32);
  location_.ID=strutils::ftos(dum,5,0,'0');
  if (stream_buffer[4] == 0) {
// unpack the header
    bits::get(stream_buffer,yr,32,16);
    bits::get(stream_buffer,mo,48,16);
    bits::get(stream_buffer,dy,64,16);
    bits::get(stream_buffer,time,80,16);
    date_time_.set(1900+yr,mo,dy,time*10000);
    bits::get(stream_buffer,nlev,96,16);
    location_.latitude=-99.9;
    location_.longitude=-999.9;
    location_.elevation=-999;
    uraob.mois_wind_type=0;
    uraob.sond_type=11;
    if (fill_header_only)
	return;
// unpack the level data
    if (static_cast<int>(levels.capacity()) < nlev) {
	levels.resize(nlev);
    }
    nl=nlev;
    off=112;
    for (n=0; n < nlev; ++n) {
	bits::get(stream_buffer,levels[n].type,off,16);
	bits::get(stream_buffer,dum,off+16,16);
	if (dum == 0x8000) {
	  levels[n].pressure=9999.9;
	}
	else {
	  levels[n].pressure=dum;
	}
	levels[n].pressure_quality=1;
	bits::get(stream_buffer,levels[n].height,off+32,16);
	if (levels[n].height == 0x8000) {
	  levels[n].height=-99999;
	}
	else {
	  levels[n].height*=10;
	}
	bits::get(stream_buffer,dum,off+48,16);
	levels[n].height_quality=dum;
	bits::get(stream_buffer,dum,off+64,16);
	if (dum == 0x8000) {
	  levels[n].temperature=-99.9;
	}
	else {
	  if (dum > 0x8000) {
	    dum-=0x10000;
	  }
	  levels[n].temperature=dum/10.;
	}
	bits::get(stream_buffer,dum,off+80,16);
	levels[n].temperature_quality=dum;
	bits::get(stream_buffer,dum,off+96,16);
	if (dum == 0x8000) {
	  levels[n].moisture=-99.99;
	}
	else {
	  levels[n].moisture=dum/100.;
	}
	bits::get(stream_buffer,dum,off+112,16);
	levels[n].moisture_quality=dum;
	bits::get(stream_buffer,dum,off+128,16);
	if (dum == 0x8000) {
	  levels[n].ucomp=-999.9;
	}
	else {
	  if (dum > 0x8000) {
	    dum-=0x10000;
	  }
	  levels[n].ucomp=dum/10.;
	}
	bits::get(stream_buffer,dum,off+144,16);
	levels[n].wind_quality=dum;
	bits::get(stream_buffer,dum,off+160,16);
	if (dum == 0x8000) {
	  levels[n].vcomp=-999.9;
	}
	else {
	  if (dum > 0x8000) {
	    dum-=0x10000;
	  }
	  levels[n].vcomp=dum/10.;
	}
	bits::get(stream_buffer,dum,off+176,16);
	if (dum > levels[n].wind_quality) {
	  levels[n].wind_quality=dum;
	}
	off+=192;
// remove any missing levels
	if (levels[n].pressure > 9999. && levels[n].height < -9999) {
	  levels[n].pressure=0.;
	  unsorted=true;
	  nl--;
	}
	else {
	  if ((levels[n].pressure > 9999. || levels[n].height < -9999) &&
            (levels[n].temperature < -99. && levels[n].moisture < -99. &&
            (levels[n].ucomp < -999. || levels[n].vcomp < -999.))) {
	    levels[n].pressure=0.;
	    unsorted=true;
	    nl--;
	  }
	  else {
	    if (n > 0 && levels[n].pressure < 9999. && levels[n-1].pressure < 9999. && levels[n].pressure > levels[n-1].pressure) {
		unsorted=true;
	    }
	  }
	}
    }
  }
  else {
// unpack the header
    bits::get(stream_buffer,yr,32,8);
    bits::get(stream_buffer,mo,40,8);
    bits::get(stream_buffer,dy,48,8);
    bits::get(stream_buffer,uraob.syn_hour,56,8);
    bits::get(stream_buffer,dum,64,16);
    if (dum == 0x8000)
	location_.latitude=-99.99;
    else {
	if (dum > 0x8000)
	  dum-=0x10000;
	location_.latitude=dum/100.;
    }
    bits::get(stream_buffer,dum,80,16);
    if (dum == 0x8000)
	location_.longitude=-99.99;
    else
// longitudes are biased by 475.36, for some unknown reason
	location_.longitude=(dum-47536)/100.;
    bits::get(stream_buffer,uraob.sond_type,120,8);
    bits::get(stream_buffer,uraob.mrdsq,96,16);
    bits::get(stream_buffer,uraob.quad,112,8);
    bits::get(stream_buffer,uraob.sond_type,120,8);
    bits::get(stream_buffer,nlev,128,8);
    bits::get(stream_buffer,time,136,8);
    date_time_.set(1900+yr,mo,dy,time*10000);
    bits::get(stream_buffer,location_.elevation,144,16);
    if (location_.elevation == 0x8000) {
	location_.elevation=-999;
    }
    else if (location_.elevation > 0x8000) {
	location_.elevation-=0x10000;
    }
    bits::get(stream_buffer,uraob.instru_type,160,8);
    off=168;
    for (n=0; n < 5; ++n) {
	bits::get(stream_buffer,uraob.cloud[0][n],off,8);
	bits::get(stream_buffer,uraob.cloud[1][n],off+8,8);
	off+=16;
    }
    uraob.mois_wind_type=1;
    if (fill_header_only) {
	return;
    }
// unpack the level data
    if (static_cast<int>(levels.capacity()) < nlev) {
	levels.resize(nlev);
    }
    nl=nlev;
    off=248;
    for (n=0; n < nlev; ++n) {
	bits::get(stream_buffer,levels[n].type,off,8);
	bits::get(stream_buffer,dum,off+8,16);
	if (dum == 0x8000) {
	  levels[n].pressure=9999.9;
	}
	else {
	  levels[n].pressure=dum/10.;
	}
	bits::get(stream_buffer,levels[n].height,off+24,16);
	if (levels[n].height == 0x8000) {
	  levels[n].height=-99999;
	}
	else {
	  if (levels[n].height > 0x8000) {
//	    if (levels[n].pressure > 10. || (levels[n].pressure <= 10. &&
//              levels[n].height > 50000))
	    if (levels[n].pressure > 10.) {
		levels[n].height-=0x10000;
	    }
	  }
//	  if (levels[n].pressure <= 10. && levels[n].height < 20000)
//	    levels[n].height+=32768;
	  levels[n].height+=10000;
	}
	bits::get(stream_buffer,dum,off+40,8);
	levels[n].height_quality=dum;
	bits::get(stream_buffer,dum,off+48,16);
	if (dum == 0x8000) {
	  levels[n].temperature=levels[n].moisture=-99.9;
	}
	else {
	  if (dum > 0x8000) {
	    dum-=0x10000;
	  }
	  levels[n].temperature=dum/10.;
	  bits::get(stream_buffer,dum,off+72,16);
	  if (dum == 0x8000) {
	    levels[n].moisture=-99.9;
	  }
	  else {
	    levels[n].moisture=levels[n].temperature-dum/10.;
	  }
	}
	bits::get(stream_buffer,dum,off+64,8);
	levels[n].temperature_quality=dum;
	bits::get(stream_buffer,dum,off+88,8);
	levels[n].moisture_quality=dum;
	bits::get(stream_buffer,levels[n].wind_direction,off+96,16);
	if (levels[n].wind_direction == 0x8000) {
	  levels[n].wind_direction=999;
	}
	bits::get(stream_buffer,dum,off+112,8);
	levels[n].wind_quality=dum;
	bits::get(stream_buffer,dum,off+120,16);
	if (dum == 0x8000) {
	  levels[n].wind_speed=999.;
	}
	else {
	  levels[n].wind_speed=dum;
	}
	bits::get(stream_buffer,dum,off+136,8);
	if (dum > levels[n].wind_quality) {
	  levels[n].wind_quality=dum;
	}
	off+=144;
// remove any missing levels
	if (levels[n].pressure > 9999. && levels[n].height < -9999) {
	  levels[n].pressure=0.;
	  unsorted=true;
	  nl--;
	}
	else {
	  if ((levels[n].pressure > 9999. || levels[n].height < -9999) && (levels[n].temperature < -99. && levels[n].moisture < -99. && (levels[n].wind_direction == 999 || levels[n].wind_speed > 990.))) {
	    levels[n].pressure=0.;
	    unsorted=true;
	    nl--;
	  }
	  else {
	    if (n > 0 && levels[n].pressure < 9999. && levels[n-1].pressure < 9999. && levels[n].pressure > levels[n-1].pressure)
		unsorted=true;
	  }
	}
    }
  }
// sort the sounding, if necessary
  if (unsorted) {
    binary_sort(levels,
    [](Raob::RaobLevel& left,Raob::RaobLevel& right) -> bool
    {
	if (left.pressure >= right.pressure) {
	  return true;
	}
	else {
	  return false;
	}
    },0,nl);
    nlev=nl;
  }
}

void USSRRaob::print(std::ostream& outs) const
{
  int n;

  outs.setf(std::ios::fixed);
  outs.precision(1);
  print_header(outs,true);
  if (nlev > 0) {
    switch (uraob.mois_wind_type) {
	case 0:
	  std::cout << "\n   LEV    PRES    HGT Q   TEMP Q  SPEC HUM Q  U-COMP  V-COMP"
          << " Q  TYPE" << std::endl;
	  for (n=0; n < nlev; n++) {
	    outs << std::setw(6) << n+1 << std::setw(8) << levels[n].pressure << std::setw(7) << levels[n].height << std::setw(2) << static_cast<int>(levels[n].height_quality) << std::setw(7) << levels[n].temperature << std::setw(2) << static_cast<int>(levels[n].temperature_quality);
	    outs.precision(2);
	    outs << std::setw(10) << levels[n].moisture;
	    outs.precision(1);
          outs << std::setw(2) << static_cast<int>(levels[n].moisture_quality) << std::setw(8) << levels[n].ucomp << std::setw(8) << levels[n].vcomp << std::setw(2) << static_cast<int>(levels[n].wind_quality) << std::setw(6) << levels[n].type << std::endl;
	  }
	  break;
	case 1:
	  std::cout << "\n   LEV    PRES    HGT Q   TEMP Q   DEWP Q   DD    FF Q  TYPE"
          << std::endl;
	  for (n=0; n < nlev; ++n) {
	    outs << std::setw(6) << n+1 << std::setw(8) << levels[n].pressure << std::setw(7) << levels[n].height << std::setw(2) << static_cast<int>(levels[n].height_quality) << std::setw(7) << levels[n].temperature << std::setw(2) << static_cast<int>(levels[n].temperature_quality) << std::setw(7) << levels[n].moisture << std::setw(2) << static_cast<int>(levels[n].moisture_quality) << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << std::setw(2) << static_cast<int>(levels[n].wind_quality) << std::setw(6) << levels[n].type << std::endl;
	  }
	  break;
    }
  }
}

void USSRRaob::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);
  if (verbose) {
    outs << "Station: " << location_.ID << "  Date: " << date_time_.to_string() << "Z  Location: ";
    outs.precision(2);
    outs << std::setw(7) << location_.latitude << std::setw(9) << location_.longitude;
    outs.precision(1);
    outs << std::setw(6) << location_.elevation << "  Sounding Type: " << std::setw(2) << uraob.sond_type << "  NumLevels: " << std::setw(3) << nlev << std::endl;
  }
  else {
    outs << "Stn=" << location_.ID << " Date=" << std::setw(4) << date_time_.year() << std::setw(2) << date_time_.month() << std::setw(2) << date_time_.day() << std::setw(4) << date_time_.time() << std::setfill(' ') << " Lat=";
    outs.precision(2);
    outs << location_.latitude << " Lon=" << location_.longitude;
    outs.precision(1);
    outs << " Elev=" << location_.elevation << " Type=" << uraob.sond_type << " Nlev=" << nlev << std::endl;
  }
}

bool operator==(const USSRRaob& source1,const USSRRaob& source2)
{
  if (source1.uraob.sond_type != source2.uraob.sond_type) {
    return false;
  }
  if (source1.uraob.mois_wind_type != source2.uraob.mois_wind_type) {
    return false;
  }
  if (source1.nlev != source2.nlev) {
    return false;
  }
  if (source1 < source2 || source1 > source2) {
    return false;
  }
  for (int n=0; n < source1.nlev; ++n) {
    if (!floatutils::myequalf(source1.levels[n].pressure,source2.levels[n].pressure)) {
	return false;
    }
    if (!floatutils::myequalf(source1.levels[n].height,source2.levels[n].height)) {
	return false;
    }
    if (!floatutils::myequalf(source1.levels[n].temperature,source2.levels[n].temperature)) {
	return false;
    }
    if (!floatutils::myequalf(source1.levels[n].moisture,source2.levels[n].moisture)) {
	return false;
    }
    if (source1.uraob.mois_wind_type == 0) {
	if (!floatutils::myequalf(source1.levels[n].ucomp,source2.levels[n].ucomp)) {
	  return false;
	}
	if (!floatutils::myequalf(source1.levels[n].vcomp,source2.levels[n].vcomp)) {
	  return false;
	}
    }
    else {
	if (source1.levels[n].wind_direction != source2.levels[n].wind_direction) {
	  return false;
	}
	if (!floatutils::myequalf(source1.levels[n].wind_speed,source2.levels[n].wind_speed)) {
	  return false;
	}
    }
  }
  return true;
}

bool operator<(const USSRRaob& source1,const USSRRaob& source2)
{
  if (source1.location_.ID < source2.location_.ID) {
    return true;
  }
  if (source1.location_.ID > source2.location_.ID) {
    return false;
  }
// otherwise, station IDs are the same
  if (source1.date_time_ < source2.date_time_) {
    return true;
  }
  else if (source1.date_time_ > source2.date_time_) {
    return false;
  }
// otherwise, date_times are the same
  if (source1.uraob.mois_wind_type < source2.uraob.mois_wind_type) {
    return true;
  }
  else {
    return false;
  }
}

bool operator>(const USSRRaob& source1,const USSRRaob& source2)
{
  if (source1.location_.ID > source2.location_.ID) {
    return true;
  }
  if (source1.location_.ID < source2.location_.ID) {
    return false;
  }
// otherwise, station IDs are the same
  if (source1.date_time_ > source2.date_time_) {
    return true;
  }
  else if (source1.date_time_ < source2.date_time_) {
    return false;
  }
// otherwise, date_times are the same
  if (source1.uraob.mois_wind_type > source2.uraob.mois_wind_type) {
    return true;
  }
  else {
    return false;
  }
}
