#include <iomanip>
#include <raob.hpp>
#include <bsort.hpp>
#include <strutils.hpp>
#include <utils.hpp>

int InputDATSAVRaobStream::ignore()
{
return 0;
}

int InputDATSAVRaobStream::peek()
{
  const size_t BUF_LEN=10000;
  char buffer[BUF_LEN];
  int num_bytes;

  if (!is_open()) {
    std::cerr << "Error: no InputDATSAVRaobStream has been opened" << std::endl;
    exit(1);
  }
  fs.getline(buffer,BUF_LEN);
  if (fs.eof()) {
    return bfstream::eof;
  }
  else if (!fs.good()) {
    ++num_read;
    return bfstream::error;
  }
  num_bytes=std::string(buffer).length();
  fs.seekg(-num_bytes,std::ios_base::cur);
  return num_bytes;
}

int InputDATSAVRaobStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (!is_open()) {
    std::cerr << "Error: no InputDATSAVRaobStream has been opened" << std::endl;
    exit(1);
  }
  char *buf=reinterpret_cast<char *>(const_cast<unsigned char *>(buffer));
  fs.getline(buf,buffer_length);
  if (fs.eof()) {
    return bfstream::eof;
  }
  else if (!fs.good()) {
    ++num_read;
    return bfstream::error;
  }
  ++num_read;
  return std::string(buf).length();
}

int OutputDATSAVRaobStream::write(const unsigned char *buffer,size_t num_bytes)
{
  if (!is_open()) {
    std::cerr << "Error: no OutputDATSAVRaobStream has been opened" << std::endl;
    exit(1);
  }
  fs.write(reinterpret_cast<char *>(const_cast<unsigned char *>(buffer)),num_bytes);
  ++num_written;
  if (static_cast<size_t>(fs.gcount()) == num_bytes) {
    return num_bytes;
  }
  else {
    return bfstream::error;
  }
}

size_t DATSAVRaob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
  size_t length=72+nlev*40+draob.num_addl+draob.num_rmks+draob.num_reject+1;
  int n,off;
  char *buf;

  if (length > buffer_length) {
    return 0;
  }
  buf=reinterpret_cast<char *>(const_cast<unsigned char *>(output_buffer));
// mandatory section
  strutils::strput(buf,length,4,'0');
  strutils::strput(&buf[4],nlev,3,'0');
  strutils::strput(&buf[7],draob.num_addl,3,'0');
  strutils::strput(&buf[10],draob.num_rmks,3,'0');
  strutils::strput(&buf[13],draob.num_reject,3,'0');
  std::copy(location_.ID.c_str(),location_.ID.c_str()+6,&buf[16]);
  strutils::strput(&buf[22],date_time_.year(),4,'0');
  strutils::strput(&buf[26],date_time_.month(),2,'0');
  strutils::strput(&buf[28],date_time_.day(),2,'0');
  strutils::strput(&buf[30],date_time_.time()/100,4,'0');
  strutils::strput(&buf[34],lround(location_.latitude*1000.),6,'0',true);
  strutils::strput(&buf[40],lround(location_.longitude*1000.),7,'0',true);
  std::copy(draob.type.c_str(),draob.type.c_str()+5,&buf[47]);
  strutils::strput(&buf[52],location_.elevation,5,'0',true);
  std::copy(draob.call.c_str(),draob.call.c_str()+5,&buf[57]);
  strutils::strput(&buf[62],draob.wind_code,1);
  strutils::strput(&buf[63],draob.qual_rate,3,'0');
  strutils::strput(&buf[66],draob.source,1);
  strutils::strput(&buf[67],draob.thermo_code,2,'0');
  buf[69]='L';
  buf[70]='E';
  buf[71]='V';
  off=72;
  for (n=0; n < nlev; ++n) {
    buf[off]='L';
    strutils::strput(&buf[off+1],n+1,3,'0');
    switch(levels[n].type) {
	case 0:
	  buf[off+4]='S';
	  break;
	case 4:
	  buf[off+4]='T';
	  break;
	case 5:
	  buf[off+4]='M';
	  break;
	default:
	  buf[off+4]='9';
    }
    strutils::strput(&buf[off+5],lroundf(levels[n].pressure*10),5,'0');
    buf[off+10]=levels[n].pressure_quality;
    strutils::strput(&buf[off+11],levels[n].height,6,'0',true);
    buf[off+17]=levels[n].height_quality;
    strutils::strput(&buf[off+18],lroundf(levels[n].temperature*10),5,'0',true);
    buf[off+23]=levels[n].temperature_quality;
    strutils::strput(&buf[off+24],lroundf(levels[n].moisture*10),5,'0',true);
    buf[off+29]=levels[n].moisture_quality;
    strutils::strput(&buf[off+30],levels[n].wind_direction,3,'0');
    buf[off+33]=draob.wind_type;
    buf[off+34]=levels[n].wind_quality;
    strutils::strput(&buf[off+35],static_cast<int>(levels[n].wind_speed*10.),4,'0');
    buf[off+39]=levels[n].wind_quality;
    off+=40;
  }
  if (draob.num_addl > 0) {
    std::copy(&addl_data[0],&addl_data[0]+draob.num_addl,&buf[off]);
    off+=draob.num_addl;
  }
  if (draob.num_rmks > 0) {
    std::copy(&remarks[0],&remarks[0]+draob.num_rmks,&buf[off]);
    off+=draob.num_rmks;
  }
  if (draob.num_reject > 0) {
    std::copy(&quality[0],&quality[0]+draob.num_reject,&buf[off]);
    off+=draob.num_reject;
  }
  buf[off]=0xa;
  buf[length]='\0';
  return length;
}

bool compare_DATSAV_pressures(Raob::RaobLevel& left,Raob::RaobLevel& right)
{
  if (left.pressure >= right.pressure) {
    return true;
  }
  else {
    return false;
  }
}

bool compare_DATSAV_heights(Raob::RaobLevel& left,Raob::RaobLevel& right)
{
  if (left.height <= right.height) {
    return true;
  }
  else {
    return false;
  }
}

void DATSAVRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *buf=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  short yr,mo,dy,time;
  int dum,n;
  size_t m,off,nl;
  bool unsorted=false,is_raob;

// mandatory section
  strutils::strget(&buf[4],nlev,3);
  strutils::strget(&buf[7],draob.num_addl,3);
  strutils::strget(&buf[10],draob.num_rmks,3);
  strutils::strget(&buf[13],draob.num_reject,3);
  location_.ID.assign(&buf[16],6);
  strutils::strget(&buf[22],yr,4);
  strutils::strget(&buf[26],mo,2);
  strutils::strget(&buf[28],dy,2);
  strutils::strget(&buf[30],time,4);
  date_time_.set(yr,mo,dy,time*100);
  strutils::strget(&buf[34],dum,6);
  location_.latitude=dum/1000.;
  strutils::strget(&buf[40],dum,7);
  location_.longitude=dum/1000.;
  draob.type.assign(&buf[47],5);
  draob.type[5]='\0';
  if (draob.type == "UPTMP") {
    is_raob=true;
  }
  else {
    is_raob=false;
  }
  strutils::strget(&buf[52],location_.elevation,5);
  draob.call.assign(&buf[57],5);
  draob.call[5]='\0';
  strutils::strget(&buf[62],draob.wind_code,1);
  strutils::strget(&buf[63],draob.qual_rate,3);
  strutils::strget(&buf[66],draob.source,1);
  strutils::strget(&buf[67],draob.thermo_code,2);
// level data section
  if (fill_header_only || std::string(&buf[69],3) != "LEV") {
    return;
  }
/*
  if (static_cast<int>(raob.capacity) < nlev) {
    if (raob.capacity > 0)
	delete[] levels;
    raob.capacity=nlev;
    levels=new RaobLevel[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev)
levels.resize(nlev);
  off=72;
  nl=nlev;
  for (n=0; n < nlev; ++n) {
    switch(buf[off+4]) {
	case 'S':
	  levels[n].type=0;
	  break;
	case 'T':
	case 'B':
	  levels[n].type=4;
	  break;
	case 'M':
	  levels[n].type=5;
	  break;
	default:
	  levels[n].type=9;
    }
    strutils::strget(&buf[off+5],dum,5);
    levels[n].pressure=dum/10.;
    levels[n].pressure_quality=buf[off+10];
    strutils::strget(&buf[off+11],levels[n].height,6);
    levels[n].height_quality=buf[off+17];
    strutils::strget(&buf[off+18],dum,5);
    levels[n].temperature=dum/10.;
    levels[n].temperature_quality=buf[off+23];
    strutils::strget(&buf[off+24],dum,5);
    levels[n].moisture=dum/10.;
    levels[n].moisture_quality=buf[off+29];
    strutils::strget(&buf[off+30],levels[n].wind_direction,3);
    draob.wind_type=buf[off+33];
    strutils::strget(&buf[off+35],dum,4);
    levels[n].wind_speed=dum/10.;
    if (buf[off+34] > buf[off+39]) {
	levels[n].wind_quality=buf[off+34];
    }
    else {
	levels[n].wind_quality=buf[off+39];
    }
    if (levels[n].pressure > 9999. || floatutils::myequalf(levels[n].pressure,0.)) {
// remove missing levels
	if (levels[n].height == 99999 || floatutils::myequalf(levels[n].pressure,0.)) {
	  levels[n].pressure=0.;
	  unsorted=true;
	  --nl;
	}
// preserve the location in the stack of levels w/missing pressure
	else {
	  if (n > 0) {
	    if (!floatutils::myequalf(levels[n-1].pressure,0.)) {
		levels[n].pressure=levels[n-1].pressure-0.1;
	    }
	    else if (n > 1) {
		m=n-2;
		while (m > 0 && floatutils::myequalf(levels[m].pressure,0.)) {
		  --m;
		}
		if (!floatutils::myequalf(levels[m].pressure,0.)) {
		  levels[n].pressure=levels[m].pressure-0.1;
		}
	    }
	    levels[n].pressure_quality='9';
	  }
	}
    }
    if (n > 0) {
	if (is_raob) {
	  if (levels[n].pressure > levels[n-1].pressure) unsorted=true;
	}
	else {
	  if (levels[n].height < levels[n-1].height) unsorted=true;
	}
    }
    off+=40;
  }
// sort the stack, if necessary
  if (unsorted) {
    if (is_raob) {
	binary_sort(levels,compare_DATSAV_pressures,0,nlev);
    }
    else {
	binary_sort(levels,compare_DATSAV_heights,0,nlev);
    }
    nlev=nl;
  }
// merge duplicate levels
  nl=nlev;
  unsorted=false;
  for (n=1; n < nlev; ++n) {
    if (floatutils::myequalf(levels[n].pressure,levels[n-1].pressure) || ((levels[n].pressure_quality == '9' || levels[n-1].pressure_quality == '9') && levels[n].height == levels[n-1].height)) {
	if ((levels[n].height == levels[n-1].height || levels[n].height == 99999 || levels[n-1].height == 99999) && (floatutils::myequalf(levels[n].temperature,levels[n-1].temperature) || levels[n].temperature > 999. || levels[n-1].temperature > 999.) && (floatutils::myequalf(levels[n].moisture,levels[n-1].moisture) || levels[n].moisture > 999. || levels[n-1].moisture > 999.) && (levels[n].wind_direction == levels[n-1].wind_direction || levels[n].wind_direction == 999 || levels[n-1].wind_direction == 999) && (floatutils::myequalf(levels[n].wind_speed,levels[n-1].wind_speed) || levels[n].wind_speed < 9990. || levels[n-1].wind_speed < 9990.)) {
	  if (levels[n].pressure_quality == '9') {
	    levels[n].pressure=levels[n-1].pressure;
	    levels[n].pressure_quality=levels[n-1].pressure_quality;
	  }
	  if (levels[n].height == 99999) levels[n].height=levels[n-1].height;
	  if (levels[n].height_quality == '9')
	    levels[n].height_quality=levels[n-1].height_quality;
	  if (levels[n].temperature > 999.)
	    levels[n].temperature=levels[n-1].temperature;
	  if (levels[n].temperature_quality == '9')
	    levels[n].temperature_quality=levels[n-1].temperature_quality;
	  if (levels[n].moisture > 999.) levels[n].moisture=levels[n-1].moisture;
	  if (levels[n].moisture_quality == '9')
	    levels[n].moisture_quality=levels[n-1].moisture_quality;
	  if (levels[n].wind_direction == 999)
	    levels[n].wind_direction=levels[n-1].wind_direction;
	  if (levels[n].wind_speed < 9990.)
	    levels[n].wind_speed=levels[n-1].wind_speed;
	  if (levels[n].wind_quality == '9')
	    levels[n].wind_quality=levels[n-1].wind_quality;
	  if (levels[n].type == 9) levels[n].type=levels[n-1].type;
	  levels[n-1].pressure=0.;
	  levels[n-1].height=0;
	  --nl;
	  unsorted=true;
	}
    }
  }
// sort the stack, if necessary
  if (unsorted) {
    if (is_raob) {
	binary_sort(levels,compare_DATSAV_pressures,0,nlev);
    }
    else {
	binary_sort(levels,compare_DATSAV_heights,0,nlev);
    }
    nlev=nl;
  }
// restore any missing pressures that were changed to preserve the level
// location in the stack
  for (n=0; n < nlev; ++n) {
    if (levels[n].pressure_quality == '9' && levels[n].pressure < 9999.)
	levels[n].pressure=9999.9;
  }
  if (draob.num_addl > 0) {
    addl_data.reset(new char[draob.num_addl]);
    std::copy(&buf[off],&buf[off]+draob.num_addl,addl_data.get());
    off+=draob.num_addl;
  }
  if (draob.num_rmks > 0) {
    remarks.reset(new char[draob.num_rmks]);
    std::copy(&buf[off],&buf[off]+draob.num_rmks,remarks.get());
    off+=draob.num_rmks;
  }
  if (draob.num_reject > 0) {
    quality.reset(new char[draob.num_reject]);
    std::copy(&buf[off],&buf[off]+draob.num_reject,quality.get());
  }
}

void DATSAVRaob::print(std::ostream& outs) const
{
  int n;

  print_header(outs,true);
  outs.setf(std::ios::fixed);
  outs.precision(1);
  if (nlev > 0) {
    outs << "\n   LEV    PRES Q    HGT Q   TEMP Q   MOIS Q   DD    FF Q  TYPE"
      << std::endl;
    for (n=0; n < nlev; ++n) {
	outs << std::setw(6) << n+1 << std::setw(8) << levels[n].pressure << " " << levels[n].pressure_quality << std::setw(7) << levels[n].height << " " << levels[n].height_quality << std::setw(7) << levels[n].temperature << " " << levels[n].temperature_quality << std::setw(7) << levels[n].moisture << " " << levels[n].moisture_quality << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << " " << levels[n].wind_quality << std::setw(6) << levels[n].type << std::endl;
    }
    outs << std::endl;
  }
}

void DATSAVRaob::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(3);

  if (verbose) {
    outs << "Station: " << location_.ID << "  Type: " << draob.type << "  Date: " << date_time_.to_string() << "Z  Location: " << std::setw(7) << location_.latitude << std::setw(9) << location_.longitude << std::setw(6) << location_.elevation << "  NumLevels: " << std::setw(3) << nlev << std::endl;
  }
  else {
    outs << "Stn=" << location_.ID << " Type=" << draob.type << " Date=" << date_time_.to_string("%Y%m%d") << std::setfill('0') << std::setw(4) << date_time_.time() << std::setfill(' ') << " Lat=" << location_.latitude << " Lon=" << location_.longitude << " Elev=" << location_.elevation << " Nlev=" << nlev << std::endl;
  }
}
