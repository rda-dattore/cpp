#include <iomanip>
#include <complex>
#include <observation.hpp>
#include <raob.hpp>
#include <pibal.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

const float Tsraob::mois_scale[4]={1.,1.,0.1,0.01};
const int Tsraob::mois_bias[4]={1000,1000,1000,0};
const size_t Tsraob::mois_precision[4]={1,1,1,2};

int InputTsraobStream::ignore()
{
  if (!is_open()) {
    std::cerr << "Error: no InputTsraobStream has been opened" << std::endl;
    exit(1);
  }
// read the next raob from the stream
  int bytes_read;
  if (irptstream != nullptr) {
    bytes_read=irptstream->ignore();
  }
  else if (icosstream != nullptr) {
return bfstream::error;
  }
  else {
return bfstream::error;
  }
  ++num_read;
  return bytes_read;
}

int InputTsraobStream::peek()
{
  int bytes_read;

  if (!is_open()) {
    std::cerr << "Error: no InputTsraobStream has been opened" << std::endl;
    exit(1);
  }

// read the next raob from the stream
  if (irptstream != NULL)
    bytes_read=irptstream->peek();
  else if (icosstream != NULL)
return bfstream::error;
  else
return bfstream::error;

  return bytes_read;
}

int InputTsraobStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;

  if (!is_open()) {
    std::cerr << "Error: no InputTsraobStream has been opened" << std::endl;
    exit(1);
  }

// read the next raob from the stream
  if (irptstream != NULL) {
    bytes_read=irptstream->read(buffer,buffer_length);

// if bytes_read equals the buffer length, exit as a fatal error because the
// entire raob may not have been read
    if (bytes_read == static_cast<int>(buffer_length)) {
	std::cerr << "Error: buffer overflow" << std::endl;
	exit(1);
    }
  }
  else if (icosstream != nullptr) {
bytes_read=bfstream::error;
  }
  else {
bytes_read=bfstream::error;
  }
  if (bytes_read != bfstream::eof) {
    ++num_read;
  }
  return bytes_read;
}

int OutputTsraobStream::write(const unsigned char *buffer,size_t num_bytes)
{
  int num_write=0;
  if (!is_open()) {
    std::cerr << "Error: no InputTsraobStream has been opened" << std::endl;
    exit(1);
  }
  if (orptstream != nullptr) {
    num_write=orptstream->write(buffer,num_bytes);
  }
  else if (ocrptstream != nullptr) {
    num_write=ocrptstream->write(buffer,num_bytes);
  }
  else if (ocosstream != nullptr) {
    std::cerr << "Error writing to COS-blocked file" << std::endl;
    exit(1);
  }
  else if (!fs.is_open()) {
    std::cerr << "Error writing to plain-binary file" << std::endl;
    exit(1);
  }
  ++num_written;
  if (num_write <= 0) {
    return bfstream::error;
  }
  else {
    return num_write;
  }
}

void Tsraob::operator+=(const Tsraob& source)
{
  int total_nlev=nlev+source.nlev;
//  RaobLevel *temp_levels=NULL;
//  size_t *temp_rcb=NULL;
  int n,m,start,end;

// reallocate memory if the total raob is larger
/*
  if (static_cast<int>(raob.capacity) < total_nlev) {
    if (raob.capacity > 0) {
	temp_levels=new RaobLevel[nlev];
	temp_rcb=new size_t[nlev];
	for (n=0; n < nlev; n++) {
	  temp_levels[n]=levels[n];
	  temp_rcb[n]=rcb[n];
	}
	delete[] levels;
	delete[] rcb;
    }
    raob.capacity=total_nlev;
    levels=new RaobLevel[raob.capacity];
    rcb=new size_t[raob.capacity];
    if (temp_levels != NULL) {
	for (n=0; n < nlev; n++) {
	  levels[n]=temp_levels[n];
	  rcb[n]=temp_rcb[n];
	}
	delete[] temp_levels;
	delete[] temp_rcb;
    }
  }
*/
if (static_cast<int>(levels.capacity()) < total_nlev) {
levels.resize(total_nlev);
rcb.resize(total_nlev);
}
// add the levels from the sounding to be added
  for (n=nlev,m=0; n < total_nlev; n++,m++) {
    levels[n]=source.levels[m];
    rcb[n]=source.rcb[m];
  }
  nlev=total_nlev;
  tsr.trel[1]=source.tsr.trel[1];
  for (n=2; n < 9; n++) {
    if (tsr.ind[n] != source.tsr.ind[n]) {
	if (tsr.ind[n] == 0) {
	  start=0;
	  end=127;
	}
	else {
	  start=127;
	  end=nlev;
	}
	for (m=start; m < end; m++) {
	  switch (n) {
	    case 2:
		if (tsr.qual_is_ascii)
		  levels[m].level_quality='0';
		else
		  levels[m].level_quality=0;
		break;
	    case 3:
		if (tsr.qual_is_ascii)
		  levels[m].time_quality='0';
		else
		  levels[m].time_quality=0;
		break;
	    case 4:
		if (tsr.qual_is_ascii)
		  levels[m].pressure_quality='0';
		else
		  levels[m].pressure_quality=0;
		break;
	    case 5:
		if (tsr.qual_is_ascii)
		  levels[m].height_quality='0';
		else
		  levels[m].height_quality=0;
		break;
	    case 6:
		if (tsr.qual_is_ascii)
		  levels[m].temperature_quality='0';
		else
		  levels[m].temperature_quality=0;
		break;
	    case 7:
		if (tsr.qual_is_ascii)
		  levels[m].moisture_quality='0';
		else
		  levels[m].moisture_quality=0;
		break;
	    case 8:
		if (tsr.qual_is_ascii)
		  levels[m].wind_quality='0';
		else
		  levels[m].wind_quality=0;
		break;
	  }
	}
	tsr.ind[n]=1;
    }
  }
}

size_t Tsraob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length)
  const
{
  size_t plat,plon,pelev,addl_flag;
  size_t off,length;
  int n,nl;

// pack the header
  bits::set(output_buffer,tsr.flags.ubits,12,4);
  bits::set(output_buffer,tsr.flags.format,16,6);
  bits::set(output_buffer,std::stoi(location_.ID),22,17);
  bits::set(output_buffer,date_time_.year()-1900,39,7);
  bits::set(output_buffer,date_time_.month(),46,4);
  bits::set(output_buffer,date_time_.day(),50,5);
  bits::set(output_buffer,static_cast<int>(date_time_.time()/10000.),55,5);
  plat=lround(location_.latitude*10.)+1000;
  bits::set(output_buffer,plat,60,11);
  plon=lround(location_.longitude*10.);
  if (location_.longitude > -199.)
    plon=-plon;
  bits::set(output_buffer,plon+2000,71,12);
  pelev=location_.elevation+1000;
  bits::set(output_buffer,pelev,83,14);
  bits::set(output_buffer,tsr.flags.source,97,7);
  bits::set(output_buffer,tsr.flags.height_temperature_status,104,4);
  bits::set(output_buffer,tsr.flags.wind_status,108,2);
  bits::set(output_buffer,tsr.flags.surface_level,110,3);
  nl=nlev;
  if (tsr.flags.format >= 9 && tsr.flags.format <= 14)
    nl-=127; 
  bits::set(output_buffer,nl,113,7);
  bits::set(output_buffer,tsr.flags.wind_units,120,1);
  bits::set(output_buffer,tsr.flags.moisture_units,121,2);
  addl_flag= (tsr.flags.additional_data) ? 1 : 0;
  bits::set(output_buffer,addl_flag,123,1);

  off=124;
// pack the raob levels
  switch (tsr.flags.format) {
    case 1:
    case 3:
    case 6:
    case 9:
    case 11:
    case 14:
	length=(off+77*nlev+7)/8;
	if (length > buffer_length)
	  return 0;
	for (n=0; n < nlev; ++n) {
	  bits::set(output_buffer,rcb[n],off,8);
	  bits::set(output_buffer,lroundf(levels[n].pressure*10.),off+8,14);
	  bits::set(output_buffer,levels[n].height+1000,off+22,16);
	  bits::set(output_buffer,lroundf(levels[n].temperature*10.)+1000,off+38,11);
	  bits::set(output_buffer,lroundf(levels[n].moisture/mois_scale[tsr.flags.moisture_units])+mois_bias[tsr.flags.moisture_units],off+49,11);
	  bits::set(output_buffer,levels[n].wind_direction,off+60,9);
	  bits::set(output_buffer,lroundf(levels[n].wind_speed),off+69,8);
	  off+=77;
	}
	break;
    case 2:
    case 10:
    case 22:
	length=(off+36*nlev+7)/8;
	if (length > buffer_length)
	  return 0;
	for (n=0; n < nlev; ++n) {
	  bits::set(output_buffer,rcb[n],off,3);
	  bits::set(output_buffer,levels[n].height+1000,off+3,16);
	  bits::set(output_buffer,levels[n].wind_direction,off+19,9);
	  bits::set(output_buffer,lroundf(levels[n].wind_speed),off+28,8);
	  off+=36;
	}
	break;
    case 5:
    case 13:
	length=(off+36*nlev+7)/8;
	if (length > buffer_length)
	  return 0;
	for (n=0; n < nlev; ++n) {
	  bits::set(output_buffer,rcb[n],off,3);
	  bits::set(output_buffer,static_cast<int>(levels[n].pressure*10.),off+3,16);
	  bits::set(output_buffer,levels[n].wind_direction,off+19,9);
	  bits::set(output_buffer,lroundf(levels[n].wind_speed),off+28,8);
	  off+=36;
	}
	break;
    default:
	std::cerr << "Error: unrecognized format " << tsr.flags.format << std::endl;
	exit(1);
  }

// pack additional data, if it exists
  if (tsr.flags.additional_data) {
    length=(off+33+(4+tsr.nind*8)*nlev+7)/8;
    if (length > buffer_length)
	return 0;
    bits::set(output_buffer,tsr.ind,off,1,0,9);
    bits::set(output_buffer,(date_time_.time()%10000)*10,off+9,12);
    bits::set(output_buffer,static_cast<int>(tsr.trel[1]*10.),off+21,12);
    off+=33;
    for (n=0; n < nlev; ++n) {
	bits::set(output_buffer,levels[n].type,off,4);
	off+=4;
	if (tsr.nind > 0) {
// level quality
	  if (tsr.ind[2] == 1) {
	    bits::set(output_buffer,levels[n].level_quality,off,8);
	    off+=8;
	  }
// level time quality
	  if (tsr.ind[3] == 1) {
	    bits::set(output_buffer,levels[n].time_quality,off,8);
	    off+=8;
	  }
// pressure quality
	  if (tsr.ind[4] == 1) {
	    bits::set(output_buffer,levels[n].pressure_quality,off,8);
	    off+=8;
	  }
// height quality
	  if (tsr.ind[5] == 1) {
	    bits::set(output_buffer,levels[n].height_quality,off,8);
	    off+=8;
	  }
// temperature quality
	  if (tsr.ind[6] == 1) {
	    bits::set(output_buffer,levels[n].temperature_quality,off,8);
	    off+=8;
	  }
// moisture quality
	  if (tsr.ind[7] == 1) {
	    bits::set(output_buffer,levels[n].moisture_quality,off,8);
	    off+=8;
	  }
// wind quality
	  if (tsr.ind[8] == 1) {
	    bits::set(output_buffer,levels[n].wind_quality,off,8);
	    off+=8;
	  }
	}
    }
  }

// pack the level times if there are individual times
  if (tsr.ind[0] == 0 || tsr.ind[0] == 16) {
    for (n=0; n < nlev; ++n) {
	bits::set(output_buffer,lroundf(levels[n].time*10.),off,12);
	off+=12;
    }
  }

  return (off+7)/8;
}

bool Tsraob::equals(const Tsraob& source)
{
  int n;

  if (nlev != source.nlev)
    return false;

  for (n=0; n < nlev; ++n) {
    if (!floatutils::myequalf(levels[n].pressure,source.levels[n].pressure))
	return false;
    if (!floatutils::myequalf(levels[n].height,source.levels[n].height))
	return false;
    if (!floatutils::myequalf(levels[n].temperature,source.levels[n].temperature))
	return false;
    if (!floatutils::myequalf(levels[n].moisture,source.levels[n].moisture))
	return false;
    if (levels[n].wind_direction != source.levels[n].wind_direction)
	return false;
    if (!floatutils::myequalf(levels[n].wind_speed,source.levels[n].wind_speed))
	return false;
  }

  return true;
}

void Tsraob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int n;
  size_t addl_flag;
  int lat,lon,dum;
  size_t mins;
  int off,len;

  bits::get(stream_buffer,len,0,12);
  len*=64;
// unpack the header
  off=12;
  if (tsr.format_flag > 0) {
    bits::get(stream_buffer,tsr.flags.ubits,off,4);
    off+=4;
  }
  bits::get(stream_buffer,tsr.flags.format,off,6);
  bits::get(stream_buffer,dum,off+6,17);
  location_.ID=strutils::ftos(dum,6,0,'0');
  bits::get(stream_buffer,dum,off+23,7);
  date_time_.set_year(1900+dum);
  bits::get(stream_buffer,dum,off+30,4);
  date_time_.set_month(dum);
  bits::get(stream_buffer,dum,off+34,5);
  date_time_.set_day(dum);
  bits::get(stream_buffer,dum,off+39,5);
  date_time_.set_time(dum*10000);
  bits::get(stream_buffer,lat,off+44,11);
  location_.latitude=(lat-1000.)/10.;
  bits::get(stream_buffer,lon,off+55,12);
  location_.longitude=(lon-2000.)/10.;
  if (location_.longitude > -199.) {
    location_.longitude=-location_.longitude;
  }
  bits::get(stream_buffer,location_.elevation,off+67,14);
  location_.elevation-=1000;
  bits::get(stream_buffer,tsr.flags.source,off+81,7);
  if (tsr.flags.source == 47) {
    location_.ID=strutils::ftos(std::stoi(location_.ID)*10+tsr.flags.ubits,6,0,'0');
  }
  bits::get(stream_buffer,tsr.flags.height_temperature_status,off+88,4);
  bits::get(stream_buffer,tsr.flags.wind_status,off+92,2);
  bits::get(stream_buffer,tsr.flags.surface_level,off+94,3);
  bits::get(stream_buffer,nlev,off+97,7);
  if (tsr.flags.format >= 9 && tsr.flags.format <= 14) {
    nlev+=127;
  }
  bits::get(stream_buffer,tsr.flags.wind_units,off+104,1);
  bits::get(stream_buffer,tsr.flags.moisture_units,off+105,2);
  bits::get(stream_buffer,addl_flag,off+107,1);
  tsr.flags.additional_data= (addl_flag == 1) ? true : false;
  if (tsr.flags.source == 41 || tsr.flags.source == 45) {
    tsr.deck=tsr.flags.height_temperature_status*16+tsr.flags.wind_status*4+tsr.flags.moisture_units;
    if (tsr.flags.source == 41)
	tsr.deck+=5200;
    else
	tsr.deck+=5300;
    tsr.flags.height_temperature_status=tsr.flags.wind_status=0;
    tsr.flags.moisture_units=0;
  }
  else
    tsr.deck=-1;

  if (fill_header_only)
    return;
// unpack the levels in the sounding
/*
  if (static_cast<int>(raob.capacity) < nlev) {
    if (raob.capacity > 0) {
	delete[] rcb;
	delete[] levels;
    }
    raob.capacity=nlev;
    rcb=new size_t[raob.capacity];
    levels=new RaobLevel[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
// unpack the sounding data
  off=124;
  switch (tsr.flags.format) {
    case 1:
    case 9:
    case 3:
    case 11:
    case 4:
    case 12:
    case 6:
	for (n=0; n < nlev; ++n) {
	  bits::get(stream_buffer,rcb[n],off,8);
	  bits::get(stream_buffer,dum,off+8,14);
	  levels[n].pressure=dum/10.;
	  bits::get(stream_buffer,levels[n].height,off+22,16);
	  levels[n].height-=1000;
	  bits::get(stream_buffer,dum,off+38,11);
	  levels[n].temperature=(dum-1000)/10.;
	  bits::get(stream_buffer,dum,off+49,11);
	  levels[n].moisture=(dum-mois_bias[tsr.flags.moisture_units])*mois_scale[tsr.flags.moisture_units];
	  bits::get(stream_buffer,levels[n].wind_direction,off+60,9);
	  bits::get(stream_buffer,dum,off+69,8);
	  levels[n].wind_speed=dum;
	  off+=77;
	}
	break;
    case 2:
    case 10:
    case 22:
	for (n=0; n < nlev; ++n) {
	  bits::get(stream_buffer,rcb[n],off,3);
	  bits::get(stream_buffer,levels[n].height,off+3,16);
	  levels[n].height-=1000;
	  bits::get(stream_buffer,levels[n].wind_direction,off+19,9);
	  bits::get(stream_buffer,dum,off+28,8);
	  levels[n].wind_speed=dum;
	  off+=36;
	}
	if (tsr.flags.source == 14 && tsr.flags.additional_data) {
	  for (n=0; n < nlev; ++n) {
	    bits::get(stream_buffer,dum,off,2);
	    levels[n].time=dum;
	    off+=2;
	  }
	}
	break;
    case 5:
    case 13:
	for (n=0; n < nlev; ++n) {
	  bits::get(stream_buffer,rcb[n],off,3);
	  bits::get(stream_buffer,dum,off+3,16);
	  levels[n].pressure=dum/10.;
	  bits::get(stream_buffer,levels[n].wind_direction,off+19,9);
	  bits::get(stream_buffer,dum,off+28,8);
	  levels[n].wind_speed=dum;
	  off+=36;
	}
	if (tsr.flags.source == 14 && tsr.flags.additional_data) {
	  for (n=0; n < nlev; ++n) {
	    bits::get(stream_buffer,dum,off,2);
	    levels[n].time=dum;
	    off+=2;
	  }
	}
	break;
    default:
	std::cerr << "Error: unrecognized format " << tsr.flags.format << std::endl;
	exit(1);
  }
  if (tsr.flags.additional_data && (tsr.flags.source != 14 || (tsr.flags.format != 2 && tsr.flags.format != 5))) {
// unpack additional data, if it exists
    bits::get(stream_buffer,tsr.ind,off,1,0,9);
    if (tsr.ind[0] == 0)
	tsr.qual_is_ascii=false;
    else
	tsr.qual_is_ascii=true;
    tsr.nind=0;
    for (n=2; n < 9; n++)
	tsr.nind=tsr.nind+tsr.ind[n];
    bits::get(stream_buffer,dum,off+9,12);
    tsr.trel[0]=dum/10.;
    if (tsr.trel[0] < 409.5) {
	mins=lroundf(tsr.trel[0]);
	date_time_.set_time(date_time_.time()+mins/60*10000);
	date_time_.set_time(date_time_.time()+(mins % 6000));
    }
    bits::get(stream_buffer,dum,off+21,12);
    tsr.trel[1]=dum/10.;
    off+=33;
    for (n=0; n < nlev; ++n) {
	bits::get(stream_buffer,levels[n].type,off,4);
	off+=4;
// '!' for character flags and 33 for integer flags indicates that the flag was
//    never set
	if (tsr.nind > 0) {
// level quality
	  if (tsr.ind[2] == 1) {
	    bits::get(stream_buffer,levels[n].level_quality,off,8);
	    off+=8;
	  }
	  else
	    levels[n].level_quality='!';
// level time quality
	  if (tsr.ind[3] == 1) {
	    bits::get(stream_buffer,levels[n].time_quality,off,8);
	    off+=8;
	  }
	  else
	    levels[n].time_quality='!';
// pressure quality
	  if (tsr.ind[4] == 1) {
	    bits::get(stream_buffer,levels[n].pressure_quality,off,8);
	    off+=8;
	  }
	  else
	    levels[n].pressure_quality='!';
// height quality
	  if (tsr.ind[5] == 1) {
	    bits::get(stream_buffer,levels[n].height_quality,off,8);
	    off+=8;
	  }
	  else
	    levels[n].height_quality='!';
// temperature quality
	  if (tsr.ind[6] == 1) {
	    bits::get(stream_buffer,levels[n].temperature_quality,off,8);
	    off+=8;
	  }
	  else
	    levels[n].temperature_quality='!';
// moisture quality
	  if (tsr.ind[7] == 1) {
	    bits::get(stream_buffer,levels[n].moisture_quality,off,8);
	    off+=8;
	  }
	  else
	    levels[n].moisture_quality='!';
// wind quality
	  if (tsr.ind[8] == 1) {
	    bits::get(stream_buffer,levels[n].wind_quality,off,8);
	    off+=8;
	  }
	  else
	    levels[n].wind_quality='!';
	}
	else {
	  levels[n].level_quality=' ';
	  levels[n].time_quality=' ';
	  levels[n].pressure_quality=' ';
	  levels[n].height_quality=' ';
	  levels[n].temperature_quality=' ';
	  levels[n].moisture_quality=' ';
	  levels[n].wind_quality=' ';
	}
    }
    if (tsr.ind[0] == 0) {
	switch (tsr.flags.source) {
	  case 14:
// unpack check bits
	    switch (tsr.flags.format) {
		case 1:
		  tsr.ind[0]+=32;
		  for (n=0; n < nlev; ++n) {
		    bits::get(stream_buffer,dum,off,18);
		    levels[n].time=dum;
		    off+=18;
		  }
		  break;
		case 2:
		  tsr.ind[0]+=32;
		  break;
	    }
	    break;
	  default:
// unpack individual level times
	    tsr.ind[0]+=16;
	    for (n=0; n < nlev; ++n) {
		bits::get(stream_buffer,dum,off,12);
		levels[n].time=dum/10.;
		off+=12;
	    }
	}
    }
  }
  else {
// for no additional data, label every level as "other", except for the surface
// level, if it exists
    for (n=0; n < nlev; ++n) {
	levels[n].type=9;
	if (tsr.flags.surface_level == n+1) levels[n].type=0;
	levels[n].level_quality=' ';
	levels[n].time_quality=' ';
	levels[n].pressure_quality=' ';
	levels[n].height_quality=' ';
	levels[n].temperature_quality=' ';
	levels[n].moisture_quality=' ';
	levels[n].wind_quality=' ';
    }
  }
}

void Tsraob::initialize(size_t num_levels)
{
  size_t n;

  tsr.trel[0]=tsr.trel[1]=0;
  tsr.ind[0]=tsr.ind[1]=1;
  for (n=2; n < 9; n++)
    tsr.ind[n]=0;
  tsr.nind=0;
  nlev=num_levels;
/*
  if (static_cast<int>(raob.capacity) < nlev) {
    if (raob.capacity > 0) {
	delete[] rcb;
	delete[] levels;
    }
    raob.capacity=nlev;
    rcb=new size_t[raob.capacity];
    levels=new RaobLevel[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
}

float Tsraob::computeVirtualTemperature(float p,float t,float m)
{
  float tv,e=-999.,w,d;
  float x;

  if (t < 99. && m < 99.) {
    switch (tsr.flags.moisture_units) {
// RH
	case 0:
	  if (m < 0)
	    m=-m;
	  if (m > 900.)
	    e=-999.;
	  else {
	    m/=100;
	    x=(7.5*t)/(237.3+t);
	    e=6.1078*m*pow(10.,x);
	  }
	  break;
// mixing ratio
	case 1:
	  if (m > 90.)
	    e=-999.;
	  else {
	    w=m/10000;
	    e=p*(w/(w+0.62197));
	  }
	  break;
// dewpoint
	case 2:
	  if (m > 90.)
	    e=-999.;
	  else {
	    x=(7.5*m)/(237.3+m);
	    e=6.1078*pow(10.,x);
	  }
	  break;
// specific humidity
	case 3:
	  std::cerr << "Error: no virtual temperature computation available for specific humidity" << std::endl;
	  exit(1);
	  break;
    }
    tv=t+273.15;
    if (e > -900.) {
	d=1.-0.379*e/p;
	if (d >= 0.1)
	  tv/=d;
    }
  }
  else {
    if (t < 99.)
	tv=t+273.15;
    else
	tv=-999.;
  }

  return tv;
}

void Tsraob::run_quality_check(size_t dz_tolerance)
{
  int n;
  size_t numSfc=0;
  int *computedHeights,*dz_difference,lastz=64000;
  float lastp=0.,tv,lasttv=0.,dz;
  int bottom=-1,adz,cdz;
  bool passed=true;

  computedHeights=new int[nlev];
  dz_difference=new int[nlev];
  for (n=0; n < nlev; ++n) {
// surface level; check to see if flagged in header or if multiple surface
//  levels exist
    if (levels[n].type == 0) {
	numSfc++;
	if (numSfc > 1) {
	  std::cerr << "Warning: multiple surface levels found - ";
	  print_header(std::cerr,false);
	}
	if (tsr.flags.surface_level == 0) {
	  tsr.flags.surface_level=n+1;
	  std::cerr << "Warning: surface level found and header changed - ";
	  print_header(std::cerr,false);
	}
    }
// compute heights
    if (tsr.flags.height_temperature_status == 0) {
	if (lastz == 64000) {
	  if (levels[n].height < 64000 && levels[n].pressure < 1600. && levels[n].temperature < 99.) {
	    computedHeights[n]=levels[n].height;
	    lastz=computedHeights[n];
	    lastp=levels[n].pressure;
	    lasttv=computeVirtualTemperature(levels[n].pressure,levels[n].temperature,levels[n].moisture);
	  }
	  else
	    computedHeights[n]=64000;
	}
	else {
	  tv=computeVirtualTemperature(levels[n].pressure,levels[n].temperature,levels[n].moisture);
	  dz=(tv+lasttv)*log(lastp/levels[n].pressure)*14.63563358;
	  computedHeights[n]=lastz+lroundf(dz);
	  lastz=computedHeights[n];
	  lastp=levels[n].pressure;
	  lasttv=tv;
	}
    }
  }
// run a hydrostatic check
  for (n=0; n < nlev; ++n) {
    if (levels[n].height < 64000 && computedHeights[n] < 64000) {
	if (bottom >= 0) {
	  adz=levels[n].height-levels[bottom].height;
	  cdz=computedHeights[n]-computedHeights[bottom];
	  dz_difference[n]=adz-cdz;
	  if (abs(dz_difference[n]) > static_cast<int>(dz_tolerance))
	    passed=false;
	}
	else
	  dz_difference[n]=0;
	bottom=n;
    }
    else
	dz_difference[n]=-99999;
  }
  if (!passed) {
    tsr.flags.height_temperature_status=1;
    std::cerr.setf(std::ios::fixed);
    std::cerr.precision(1);
    std::cerr << "\nFAILED sounding: ";
    print_header(std::cerr,false);
    for (n=0; n < nlev; ++n) {
	if (dz_difference[n] != -99999 && abs(dz_difference[n]) > static_cast<int>(dz_tolerance))
	  std::cerr << "--> ";
	else
	  std::cerr << "    ";
	std::cerr << std::setw(3) << n << std::setw(7) << levels[n].pressure << std::setw(6) << levels[n].height << std::setw(6) << computedHeights[n] << std::setw(6) << dz_difference[n] << std::setw(6) << levels[n].temperature << std::setw(6) << levels[n].moisture << std::endl;
    }
  }
  else
    tsr.flags.height_temperature_status=3;

  delete[] computedHeights;
}

void Tsraob::print(std::ostream& outs) const
{
  int n;

  outs.setf(std::ios::fixed);
  outs.precision(1);

  print_header(outs,true);
  if (nlev > 0) {
    switch (tsr.flags.format) {
	case 1:
	case 3:
	case 4:
	case 9:
	case 11:
	case 12:
	  if (tsr.flags.additional_data) {
	    if (tsr.nind > 0) {
		if (tsr.ind[0] >= 0x20)
		  outs << "\n   LEV  Q    PRES  Q    HGT  Q   TEMP  Q   MOIS  Q   DD" << "    FF  Q RCB TYP CHECKBITS" << std::endl;
		else if (tsr.ind[0] >= 0x10)
		  outs << "\n   LEV  Q    PRES  Q    HGT  Q   TEMP  Q   MOIS  Q   DD" << "    FF  Q RCB TYP   TIME" << std::endl;
		else
		  outs << "\n   LEV  Q    PRES  Q    HGT  Q   TEMP  Q   MOIS  Q   DD" << "    FF  Q RCB TYP" << std::endl;
	    }
	    else {
		if (tsr.ind[0] >= 0x20)
		  outs << "\n   LEV    PRES    HGT   TEMP   MOIS   DD    FF RCB TYP CHECKBITS" << std::endl;
		else if (tsr.ind[0] >= 0x10)
		  outs << "\n   LEV    PRES    HGT   TEMP   MOIS   DD    FF RCB TYP   TIME" << std::endl;
		else
		  outs << "\n   LEV    PRES    HGT   TEMP   MOIS   DD    FF RCB TYP" << std::endl;
	    }
	  }
	  else
	    outs << "\n   LEV    PRES    HGT   TEMP   MOIS   DD    FF RCB" << std::endl;
	  if (!tsr.flags.additional_data || tsr.nind == 0) {
	    for (n=0; n < nlev; ++n) {
		outs << std::setw(6) << n+1 << std::setw(8) << levels[n].pressure << std::setw(7) << levels[n].height << std::setw(7) << levels[n].temperature << std::setw(7);
		outs.precision(mois_precision[tsr.flags.moisture_units]);
		outs << levels[n].moisture;
		outs.precision(1);
		outs << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << "  " << std::setw(2) << std::hex << std::setfill('0') << rcb[n] << std::setfill(' ') << std::dec;
		if (tsr.flags.additional_data) {
		  outs << std::setw(4) << levels[n].type;
		  if (tsr.ind[0] >= 0x20)
		    outs << "    " << std::setw(6) << std::setfill('0') << std::oct << static_cast<size_t>(levels[n].time) << std::setfill(' ') << std::dec;
		  else if (tsr.ind[0] >= 0x10)
		    outs << std::setw(7) << levels[n].time;
		}
		outs << std::endl;
	    }
	  }
	  else {
	    for (n=0; n < nlev; ++n) {
		if (tsr.qual_is_ascii) {
		  outs << std::setw(6) << n+1 << "  " << levels[n].level_quality << std::setw(8) << levels[n].pressure << "  " << levels[n].pressure_quality << std::setw(7) << levels[n].height << "  " << levels[n].height_quality << std::setw(7) << levels[n].temperature << "  " << levels[n].temperature_quality << std::setw(7);
		  outs.precision(mois_precision[tsr.flags.moisture_units]);
		  outs  << levels[n].moisture;
		  outs.precision(1);
		  outs << "  " << levels[n].moisture_quality << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << "  " << levels[n].wind_quality << "  " << std::setw(2) << std::hex << std::setfill('0') << rcb[n] << std::setfill(' ') << std::dec << std::setw(4) << levels[n].type;
		  if (tsr.ind[0] >= 0x20)
		    outs << "    " << std::setw(6) << std::setfill('0') << std::oct << static_cast<size_t>(levels[n].time) << std::setfill(' ') << std::dec;
		  else if (tsr.ind[0] >= 0x10)
		    outs << std::setw(7) << levels[n].time;
 		  outs << std::endl;
		}
		else {
		  outs << std::setw(6) << n+1 << std::setw(3) << static_cast<int>(levels[n].level_quality) << std::setw(8) << levels[n].pressure << std::setw(3) << static_cast<int>(levels[n].pressure_quality) << std::setw(7) << levels[n].height << std::setw(3) << static_cast<int>(levels[n].height_quality) << std::setw(7) << levels[n].temperature << std::setw(3) << static_cast<int>(levels[n].temperature_quality) << std::setw(7);
		  outs.precision(mois_precision[tsr.flags.moisture_units]);
		  outs << levels[n].moisture;
		  outs.precision(1);
		  outs << std::setw(3) << static_cast<int>(levels[n].moisture_quality) << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << std::setw(3) << static_cast<int>(levels[n].wind_quality) << "  " << std::setw(2) << std::hex << std::setfill('0') << rcb[n] << std::setfill(' ') << std::dec << std::setw(4) << levels[n].type;
		  if (tsr.ind[0] >= 0x20)
		    outs << "    " << std::setw(6) << std::setfill('0') << std::oct << static_cast<size_t>(levels[n].time) << std::setfill(' ') << std::dec;
		  else if (tsr.ind[0] >= 0x10)
		    outs << std::setw(7) << levels[n].time;
		  outs << std::endl;
		}
	    }
	  }
	  break;
	case 2:
	case 10:
	case 22:
	  if (tsr.flags.additional_data) {
	    if (tsr.nind > 0) {
		if (tsr.ind[0] >= 0x20)
		  outs << "\n   LEV Q    HGT Q   DD    FF Q RCB TYP CHECKBITS" << std::endl;
		else if (tsr.ind[0] >= 0x10)
		  outs << "\n   LEV Q    HGT Q   DD    FF Q RCB TYP   TIME" << std::endl;
		else
		  outs << "\n   LEV Q    HGT Q   DD    FF Q RCB TYP" << std::endl;
	    }
	    else {
		if (tsr.ind[0] >= 0x10)
		  outs << "\n   LEV    HGT   DD    FF RCB TYP CHECKBITS" << std::endl;
		else if (tsr.ind[0] >= 0x10)
		  outs << "\n   LEV    HGT   DD    FF RCB TYP   TIME" << std::endl;
		else
		  outs << "\n   LEV    HGT   DD    FF RCB TYP" << std::endl;
	    }
	  }
	  else
	    outs << "\n   LEV    HGT   DD    FF RCB" << std::endl;
	  if (!tsr.flags.additional_data || tsr.nind == 0) {
	    for (n=0; n < nlev; ++n) {
		outs << std::setw(6) << n+1 << std::setw(7) << levels[n].height << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << " " << std::setw(3) << std::setfill('0') << rcb[n] << std::setfill(' ') << std::dec;
		if (tsr.flags.additional_data) {
		  outs << std::setw(4) << levels[n].type;
		  if (tsr.ind[0] >= 0x20)
		    outs << "         " << std::setw(1) << std::setfill('0') << std::oct << static_cast<size_t>(levels[n].time) << std::setfill(' ') << std::dec;
		  else if (tsr.ind[0] >= 0x10)
		    outs << std::setw(7) << levels[n].time;
		}
		outs << std::endl;
	    }
	  }
	  else {
	    for (n=0; n < nlev; ++n) {
		outs << std::setw(6) << n+1 << " " << levels[n].level_quality << std::setw(7) << levels[n].height << " " << levels[n].height_quality << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << " " << levels[n].wind_quality << " " << std::setw(3) << std::setfill('0') << rcb[n] << std::setfill(' ') << std::dec << std::setw(4) << levels[n].type;
		if (tsr.ind[0] >= 0x20)
		  outs << "         " << std::setw(1) << std::setfill('0') << std::oct << static_cast<size_t>(levels[n].time) << std::setfill(' ') << std::dec;
		else if (tsr.ind[0] >= 0x10)
		  outs << std::setw(7) << levels[n].time;
		outs << std::endl;
	    }
	  }
	  break;
	case 5:
	case 13:
	  if (tsr.flags.additional_data) {
	    if (tsr.nind > 0) {
		if (tsr.ind[0] >= 0x10)
		  outs << "\n   LEV Q   PRES Q   DD    FF Q RCB TYP   TIME" << std::endl;
		else
		  outs << "\n   LEV Q   PRES Q   DD    FF Q RCB TYP" << std::endl;
	    }
	    else {
		if (tsr.ind[0] >= 0x10)
		  outs << "\n   LEV   PRES   DD    FF RCB TYP   TIME" << std::endl;
		else
		  outs << "\n   LEV   PRES   DD    FF RCB TYP" << std::endl;
	    }
	  }
	  else
	    outs << "\n   LEV   PRES   DD    FF RCB" << std::endl;
	  if (!tsr.flags.additional_data || tsr.nind == 0) {
	    for (n=0; n < nlev; ++n) {
		outs << std::setw(6) << n+1 << std::setw(7) << levels[n].pressure << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << " " << std::setw(3) << std::setfill('0') << rcb[n] << std::setfill(' ') << std::dec;
		if (tsr.flags.additional_data) {
		  outs << std::setw(4) << levels[n].type;
		  if (tsr.ind[0] >= 0x10)
		    outs << std::setw(7) << levels[n].time;
		}
		outs << std::endl;
	    }
	  }
	  else {
	    for (n=0; n < nlev; ++n) {
		outs << std::setw(6) << n+1 << " " << levels[n].level_quality << std::setw(7) << levels[n].pressure << " " << levels[n].pressure_quality << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << " " << levels[n].wind_quality << " " << std::setw(3) << std::setfill('0') << rcb[n] << std::setfill(' ') << std::dec << std::setw(4) << levels[n].type;
		if (tsr.ind[0] >= 0x10)
		  outs << std::setw(7) << levels[n].time;
		outs << std::endl;
	    }
	  }
	  break;
    }
  }
  outs << std::endl;
}

void Tsraob::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (verbose) {
    if (location_.ID < "100000" && tsr.flags.source != 47) {
	outs << "Format: " << std::setw(2) << tsr.flags.format << "  Station: " << location_.ID.substr(1);
    }
    else {
	outs << "Format: " << std::setw(2) << tsr.flags.format << "  Station: " << location_.ID;
    }
    outs << "  Date: " << date_time_.to_string() << "Z  Location: " << std::setw(5) << location_.latitude << std::setw(7) << location_.longitude << std::setw(6) << location_.elevation << "  Source: " << std::setw(2) << tsr.flags.source << "  NumLevels: " << std::setw(3) << nlev << std::endl;
    outs << "  Flags --  HSTAT: " << std::setw(1) << tsr.flags.height_temperature_status << "  WSTAT: " << std::setw(1) << tsr.flags.wind_status << "  SFC: " << std::setw(2) << tsr.flags.surface_level << "  UWIND: " << std::setw(1) << tsr.flags.wind_units << "  UMOIS: " << std::setw(1) << tsr.flags.moisture_units << "  ADDL DATA: " << tsr.flags.additional_data;
    if (tsr.flags.source == 41 || tsr.flags.source == 45) {
	outs << "  DECK: " << std::setw(4) << tsr.deck;
    }
    if (tsr.flags.additional_data == 1) {
	outs << "  TM REL: " << std::setw(5) << tsr.trel[0] << ", " << std::setw(5) << tsr.trel[1];
    }
    outs << std::endl;
  }
  else {
    if (location_.ID < "100000" && tsr.flags.source != 47) {
	outs << "  Fmt=" << tsr.flags.format << " Stn=" << location_.ID.substr(1);
    }
    else {
	outs << "  Fmt=" << tsr.flags.format << " Stn=" << location_.ID;
    }
    outs << " Date=" << std::setfill('0') << std::setw(4) << date_time_.year() << std::setw(2) << date_time_.month() << std::setw(2) << date_time_.day() << std::setw(4) << date_time_.time() << std::setfill(' ') << " Lat=" << location_.latitude << " Lon=" << location_.longitude << " Elev=" << location_.elevation << " Src=" << tsr.flags.source << " Nlev=" << nlev << std::endl;
  }
}

void Tsraob::set_quality_indicators(float release_time[2],short quality_indicators[9])
{
  size_t n;

  tsr.trel[0]=release_time[0];
  tsr.trel[1]=release_time[1];
  tsr.ind[0]=tsr.ind[1]=1;
  tsr.nind=0;
  for (n=0; n < 9; n++) {
    tsr.ind[n]=quality_indicators[n];
    if (tsr.ind[n] == 1)
	tsr.nind++;
  }
}
 
Tsraob& Tsraob::operator=(const CardsRaob& source)
{
  int n;

  tsr.flags.ubits=0;
  tsr.flags.format=1;
  date_time_=source.date_time();
  location_=source.location();
  tsr.flags.source=25;
  nlev=source.number_of_levels();
  tsr.flags.height_temperature_status=tsr.flags.wind_status=tsr.flags.surface_level=tsr.flags.moisture_units=0;
  tsr.flags.additional_data=true;
  tsr.trel[0]=tsr.trel[1]=0.;
  tsr.ind[0]=tsr.ind[1]=1;
  for (n=2; n < 9; n++)
    tsr.ind[n]=0;
  tsr.nind=0;

/*
  if (static_cast<int>(raob.capacity) < nlev) {
    if (raob.capacity > 0) {
	delete[] levels;
	delete[] rcb;
    }
    raob.capacity=nlev;
    levels=new RaobLevel[raob.capacity];
    rcb=new size_t[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
  for (n=0; n < nlev; ++n) {
    levels[n]=source.level(n);
    if (levels[n].type == 0)
	tsr.flags.surface_level=n+1;
    if (levels[n].height == -9999)
	levels[n].height=64000;
    if (levels[n].temperature < -99.)
	levels[n].temperature=99.;
    if (levels[n].moisture < -98.)
	levels[n].moisture=99.;
    if (levels[n].wind_direction == 999)
	levels[n].wind_direction=500;
    if (floatutils::myequalf(levels[n].wind_speed,999.))
	levels[n].wind_speed=250.;
    rcb[n]=0;
  }
  return *this;
}

Tsraob& Tsraob::operator=(const USSRRaob& source)
{
  static size_t type[27]={9,0,2,4,2,5,2,9,9,9,1,0,2,4,2,5,9,9,9,9,9,9,9,9,9,9,
                          9};
  static size_t uqual[10]={57,48,49,50,51,51,32,32,32,57};
  double ws;

  tsr.flags.ubits=0;
  tsr.flags.format=1;
  date_time_=source.date_time();
  location_=source.location();
  if (location_.longitude < -999.) {
    location_.longitude=-199.9;
  }
  tsr.flags.source=39;
  nlev=source.number_of_levels();
  tsr.flags.height_temperature_status=tsr.flags.wind_status=tsr.flags.surface_level=0;
  tsr.flags.wind_units=0;
  if (source.moisture_and_wind_type() == 0) {
    tsr.flags.moisture_units=3;
  }
  else {
    tsr.flags.moisture_units=2;
  }
  tsr.flags.additional_data=true;
  tsr.trel[0]=tsr.trel[1]=0.;
  tsr.nind=6;
  tsr.ind[0]=tsr.ind[1]=tsr.ind[5]=tsr.ind[6]=tsr.ind[7]=tsr.ind[8]=1;
  tsr.ind[2]=tsr.ind[3]=tsr.ind[4]=0;

/*
  if (static_cast<int>(raob.capacity) < nlev) {
    if (raob.capacity > 0) {
	delete[] levels;
	delete[] rcb;
    }
    raob.capacity=nlev;
    levels=new RaobLevel[raob.capacity];
    rcb=new size_t[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
  for (int n=0; n < nlev; ++n) {
    levels[n]=source.level(n);
    if (levels[n].type < 27)
	levels[n].type=type[levels[n].type];
    else
	levels[n].type=9;
    if (levels[n].pressure > 9999.) levels[n].pressure=1600.;
    if (levels[n].height < -9999) levels[n].height=64000;
    if (levels[n].temperature < -99.) levels[n].temperature=99.;
    if (levels[n].moisture < -99.)
	levels[n].moisture=990.*mois_scale[tsr.flags.moisture_units];
    else if (tsr.flags.moisture_units == 3)
	levels[n].moisture*=100.;
    if (source.moisture_and_wind_type() == 0) {
	if (source.level(n).ucomp > -999. && source.level(n).vcomp > -999.) {
	  metcalcs::uv2wind(source.level(n).ucomp,source.level(n).vcomp,levels[n].wind_direction,ws);
	  levels[n].wind_speed=ws;
	}
	else {
	  levels[n].wind_direction=500;
	  levels[n].wind_speed=250.;
	}
    }
    else {
	if (levels[n].wind_direction == 999) levels[n].wind_direction=500;
	if (floatutils::myequalf(levels[n].wind_speed,999.)) levels[n].wind_speed=250.;
    }
    if (levels[n].type == 0) tsr.flags.surface_level=n+1;
    levels[n].pressure_quality=32;
    levels[n].height_quality=uqual[static_cast<int>(levels[n].height_quality)];
    levels[n].temperature_quality=uqual[static_cast<int>(levels[n].temperature_quality)];
    levels[n].moisture_quality=uqual[static_cast<int>(levels[n].moisture_quality)];
    levels[n].wind_quality=uqual[static_cast<int>(levels[n].wind_quality)];
    rcb[n]=0;
  }
  return *this;
}

Tsraob& Tsraob::operator=(const FrenchRaob& source)
{
  int n;

  tsr.flags.ubits=0;
  date_time_=source.date_time();
  if (source.location().ID == "203") {
    location_.ID="007510";
    location_.latitude=44.83;
    location_.longitude=-0.7;
    location_.elevation=47;
  }
  else if (source.location().ID == "210") {
    location_.ID="007480";
    location_.latitude=45.73;
    location_.longitude=4.92;
    location_.elevation=199;
  }
  else if (source.location().ID == "212") {
    location_.ID="007645";
    location_.latitude=43.86;
    location_.longitude=4.41;
    location_.elevation=59;
  }
  else if (source.location().ID == "219") {
    location_.ID="007190";
    location_.latitude=48.55;
    location_.longitude=7.64;
    location_.elevation=151;
  }
  else if (source.location().ID == "220") {
    location_.ID="007110";
    location_.latitude=48.45;
    location_.longitude=-4.42;
    location_.elevation=98;
  }
  else if (source.location().ID == "226") {
    location_.ID="007761";
    location_.latitude=41.92;
    location_.longitude=8.8;
    location_.elevation=5;
  }
  else if (source.location().ID == "237") {
    location_.ID="007180";
    location_.latitude=48.58;
    location_.longitude=5.96;
    location_.elevation=212;
  }
  else if (source.location().ID == "260") {
    location_.ID="007145";
    location_.latitude=48.77;
    location_.longitude=2.01;
    location_.elevation=167;
  }
  else if (source.location().ID == "848") {
    location_.ID="099011";
    location_.latitude=45.;
    location_.longitude=-16.;
    location_.elevation=8;
  }
  else if (source.location().ID == "879") {
    location_.ID="007481";
    location_.latitude=45.71;
    location_.longitude=4.95;
    location_.elevation=235;
  }
  else {
    std::cerr << "Error: unknown station " << source.location().ID << std::endl;
    exit(1);
  }
  tsr.flags.source=32;
  tsr.flags.height_temperature_status=tsr.flags.wind_status=0;
  tsr.flags.surface_level=0;
  tsr.flags.wind_units=0;
  tsr.flags.moisture_units=0;
  tsr.flags.additional_data=true;
  tsr.trel[0]=tsr.trel[1]=0.;
  tsr.ind[0]=tsr.ind[1]=1;
  for (n=2; n < 9; n++)
    tsr.ind[n]=0;
  tsr.nind=0;
/*
  if (static_cast<int>(raob.capacity) < source.number_of_levels()) {
    delete[] levels;
    delete[] rcb;
    raob.capacity=source.number_of_levels();
    levels=new RaobLevel[raob.capacity];
    rcb=new size_t[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < source.number_of_levels()) {
levels.resize(source.number_of_levels());
rcb.resize(source.number_of_levels());
}
  switch (source.flags().sounding_type) {
    case 1:
	tsr.flags.format=5;
	nlev=0;
	for (n=0; n < source.number_of_levels(); n++) {
	  if (source.level(n).wind_direction < 880 && source.level(n).wind_speed < 888.) {
	    levels[nlev]=source.level(n);
	    if (levels[nlev].type == 0)
		tsr.flags.surface_level=n+1;
	    rcb[nlev]=0;
	    ++nlev;
	  }
	}
	break;
    case 2:
	tsr.flags.format=2;
	nlev=0;
	for (n=0; n < source.number_of_levels(); n++) {
	  if (source.level(n).wind_direction < 880 && source.level(n).wind_speed < 888.) {
	    levels[nlev]=source.level(n);
	    if (levels[nlev].type == 0) {
		tsr.flags.surface_level=n+1;
	    }
	    rcb[nlev]=0;
	    ++nlev;
	  }
	}
	break;
    default:
	std::cerr << "Error: unknown sounding type " << 
        source.flags().sounding_type << std::endl;
	exit(1);
  }
  return *this;
}

Tsraob& Tsraob::operator=(const ADPRaob& source)
{
  int n;

  tsr.flags.ubits=0;
  tsr.flags.format=1;
  date_time_=source.date_time();
  location_=source.location();
  tsr.flags.source=46;
  tsr.flags.height_temperature_status=tsr.flags.wind_status=0;
  tsr.flags.surface_level=0;
  nlev=source.number_of_levels();
  tsr.flags.wind_units=1;
  tsr.flags.moisture_units=2;
  tsr.flags.additional_data=true;
  tsr.trel[0]=tsr.trel[1]=0.;
  tsr.ind[0]=tsr.ind[1]=1;
  tsr.ind[2]=tsr.ind[3]=0;
  for (n=4; n < 9; n++)
    tsr.ind[n]=1;
  tsr.nind=5;
/*
  if (static_cast<int>(raob.capacity) < nlev) {
    if (raob.capacity > 0) {
	delete[] levels;
	delete[] rcb;
    }
    raob.capacity=nlev;
    levels=new RaobLevel[raob.capacity];
    rcb=new size_t[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
  for (n=0; n < nlev; ++n) {
    levels[n]=source.level(n);
    if (levels[n].pressure > 9999.) levels[n].pressure=1600.;
    if (levels[n].height == 99999) levels[n].height=64000;
    if (levels[n].temperature > 99.) levels[n].temperature=99.;
    if (levels[n].moisture > 99.) levels[n].moisture=99.;
    if (levels[n].wind_direction == 999) levels[n].wind_direction=500;
    if (floatutils::myequalf(levels[n].wind_speed,999.) || levels[n].wind_direction == 500)
	levels[n].wind_speed=250.;
    levels[n].level_quality=levels[n].time_quality=' ';
    if (levels[n].type == 0) tsr.flags.surface_level=n+1;
    rcb[n]=0;
  }
  return *this;
}

char getQuality(char qflag)
{
  switch (qflag) {
    case '1':
      return '0';
    case '2':
      return '3';
    case '3':
      return '5';
    case '9':
      return '1';
    case '0':
    default:
      return '9';
  }
}

Tsraob& Tsraob::operator=(const DATSAVRaob& source)
{
  if (source.report_type() == "UPTMP") {
    tsr.flags.format=1;
  }
  else if (source.report_type() == "UPWN") {
    if (source.report_type()[4] == 'D') {
	tsr.flags.format=2;
    }
    else if (source.report_type()[4] == 'd') {
	tsr.flags.format=22;
    }
    else {
	std::cerr << "Error: report type " << source.report_type() << " not recognized" << std::endl;
	exit(1);
    }
  }
  else {
    std::cerr << "Error: report type " << source.report_type() << " not recognized" << std::endl;
    exit(1);
  }
  date_time_=source.date_time();
  location_=source.location();
  if (location_.ID[6] != '0') {
    tsr.flags.ubits=std::stoi(location_.ID.substr(6));
  }
  else {
    tsr.flags.ubits=0;
  }
  location_.ID=location_.ID.substr(0,5);
  if (location_.elevation == 9999) location_.elevation=-999;
  tsr.flags.source=47;
  nlev=source.number_of_levels();
  tsr.flags.height_temperature_status=tsr.flags.wind_status=0;
  tsr.flags.surface_level=0;
  tsr.flags.wind_units=0;
  tsr.flags.moisture_units=2;
  tsr.flags.additional_data=true;
  tsr.trel[0]=tsr.trel[1]=0.;
  tsr.ind[0]=tsr.ind[1]=1;
  tsr.ind[2]=tsr.ind[3]=0;
  for (size_t n=4; n < 9; ++n) {
    tsr.ind[n]=1;
  }
  tsr.nind=5;
/*
  if (static_cast<int>(raob.capacity) < nlev) {
    if (raob.capacity > 0) {
	delete[] levels;
	delete[] rcb;
    }
    raob.capacity=nlev;
    levels=new RaobLevel[raob.capacity];
    rcb=new size_t[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
  for (size_t n=0; n < static_cast<size_t>(nlev); ++n) {
    levels[n]=source.level(n);
    if (levels[n].pressure > 9999.) {
	levels[n].pressure=1600.;
    }
    if (levels[n].height == 99999) {
	levels[n].height=64000;
    }
    if (levels[n].temperature > 999. || levels[n].temperature < -99.9) {
      levels[n].temperature=99.;
    }
    if (levels[n].moisture > 999. || levels[n].temperature < -99.9) {
      levels[n].moisture=99.;
    }
    if (levels[n].wind_direction == 999) {
	levels[n].wind_direction=500;
    }
    if (floatutils::myequalf(levels[n].wind_speed,9999.) || levels[n].wind_direction == 500) {
	levels[n].wind_speed=250.;
    }
    levels[n].level_quality=levels[n].time_quality=' ';
    levels[n].pressure_quality=getQuality(levels[n].pressure_quality);
    levels[n].height_quality=getQuality(levels[n].height_quality);
    levels[n].temperature_quality=getQuality(levels[n].temperature_quality);
    levels[n].moisture_quality=getQuality(levels[n].moisture_quality);
    levels[n].wind_quality=getQuality(levels[n].wind_quality);
    if (levels[n].type == 0) {
	tsr.flags.surface_level=n+1;
    }
    rcb[n]=0;
  }
  return *this;
}

Tsraob& Tsraob::operator=(const NavySpotRaob& source)
{
  int n;

  switch (source.sounding_type()) {
    case 2:
    case 6:
	tsr.flags.format=1;
	break;
    case 5:
	tsr.flags.format=3;
	break;
  }
  date_time_=source.date_time();
  location_=source.location();
  if (location_.ID < "01001" || location_.ID > "98999") {
    nlev=0;
    return *this;
  }
  tsr.flags.ubits=0;
  tsr.flags.source=55;
  nlev=source.number_of_levels();
  tsr.flags.height_temperature_status=tsr.flags.wind_status=0;
  tsr.flags.surface_level=0;
  if (source.location().ID == "02160" || (source.location().ID >= "03170" && source.location().ID <= "03496") || source.location().ID == "03743" || source.location().ID == "03774" || source.location().ID == "03808" || source.location().ID == "06181" || source.location().ID == "08302" || source.location().ID == "10184" || source.location().ID == "10393" || source.location().ID == "10486" || source.location().ID == "10548" || source.location().ID == "11520" || source.location().ID == "11934" || (source.location().ID >= "12330" && source.location().ID <= "12982") || (source.location().ID >= "15120" && source.location().ID <= "15614") || (source.location().ID >= "20000" && source.location().ID <= "39999") || (source.location().ID >= "44231" && source.location().ID <= "44373") || source.location().ID == "47058" || (source.location().ID >= "50000" && source.location().ID <= "59999" && source.location().ID != "58965" && source.location().ID != "58968" && source.location().ID != "59345" && source.location().ID != "59553") || source.location().ID == "89542" || source.location().ID == "89592") {
    tsr.flags.wind_units=0;
  }
  else {
    tsr.flags.wind_units=1-source.wind_units();
  }
  tsr.flags.moisture_units=2;
  tsr.flags.additional_data=true;
  tsr.trel[0]=tsr.trel[1]=0.;
  tsr.ind[0]=tsr.ind[1]=1;
  for (n=2; n < 9; n++)
    tsr.ind[n]=0;
  tsr.nind=0;
/*
  if (static_cast<int>(raob.capacity) < nlev) {
    if (raob.capacity > 0) {
	delete[] levels;
	delete[] rcb;
    }
    raob.capacity=nlev;
    levels=new RaobLevel[raob.capacity];
    rcb=new size_t[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
  for (n=0; n < nlev; ++n) {
    rcb[n]=0;
    levels[n]=source.level(n);
    if (levels[n].pressure > 9999.) levels[n].pressure=1600.;
    if (levels[n].height == 99999) levels[n].height=64000;
    if (levels[n].temperature > 999.) {
      levels[n].temperature=99.;
      levels[n].moisture=99.;
    }
    else if (levels[n].temperature > 0. && levels[n].temperature < 0.2 && floatutils::myequalf(levels[n].moisture,0.)) {
      levels[n].temperature=99.;
      levels[n].moisture=99.;
	rcb[n]|=0xc;
    }
    else {
	if (levels[n].moisture > 999.)
	  levels[n].moisture=99.;
	else
	  levels[n].moisture=levels[n].temperature-levels[n].moisture;
    }
    if (levels[n].wind_direction == 99999) levels[n].wind_direction=500;
    if (floatutils::myequalf(levels[n].wind_speed,99999.) || levels[n].wind_direction == 500)
	levels[n].wind_speed=250.;
    levels[n].level_quality=levels[n].time_quality=levels[n].pressure_quality=levels[n].height_quality=levels[n].moisture_quality=levels[n].wind_quality=' ';
  }
  return *this;
}

Tsraob& Tsraob::operator=(const KuniaRaob& source)
{
  int n,dd;
  double ws;

  tsr.flags.format=1;
  date_time_=source.date_time();
  location_=source.location();
// fix these locations known to be bad
  if ((fabs(location_.latitude+80.0) < 0.1 && fabs(location_.longitude-119.5) < 0.1) ||
      (fabs(location_.latitude+43.9) < 0.1 && fabs(location_.longitude-176.5) < 0.1) ||
      (fabs(location_.latitude+29.2) < 0.1 && fabs(location_.longitude-177.9) < 0.1) ||
      (fabs(location_.latitude+14.3) < 0.1 && fabs(location_.longitude-170.7) < 0.1) ||
      (fabs(location_.latitude-5.2) < 0.1 && fabs(location_.longitude-52.7) < 0.1) ||
      (fabs(location_.latitude-5.2) < 0.1 && fabs(location_.longitude-52.8) < 0.1) ||
      (fabs(location_.latitude-16.2) < 0.1 && fabs(location_.longitude-169.2) < 0.1) ||
      (fabs(location_.latitude-16.6) < 0.1 && fabs(location_.longitude-169.4) < 0.1) ||
      (fabs(location_.latitude-16.6) < 0.1 && fabs(location_.longitude-169.5) < 0.1) ||
      (fabs(location_.latitude-16.7) < 0.1 && fabs(location_.longitude-169.5) < 0.1) ||
      (fabs(location_.latitude-19.2) < 0.1 && fabs(location_.longitude-155.0) < 0.1) ||
      (fabs(location_.latitude-19.6) < 0.1 && fabs(location_.longitude-155.1) < 0.1) ||
      (fabs(location_.latitude-19.7) < 0.1 && fabs(location_.longitude-154.9) < 0.1) ||
      (fabs(location_.latitude-19.7) < 0.1 && fabs(location_.longitude-155.0) < 0.1) ||
      (fabs(location_.latitude-21.3) < 0.1 && fabs(location_.longitude-159.1) < 0.1) ||
      (fabs(location_.latitude-21.8) < 0.1 && fabs(location_.longitude-159.2) < 0.1) ||
      (fabs(location_.latitude-21.9) < 0.1 && fabs(location_.longitude-159.3) < 0.1) ||
      (fabs(location_.latitude-21.9) < 0.1 && fabs(location_.longitude-159.4) < 0.1) ||
      (fabs(location_.latitude-23.8) < 0.1 && fabs(location_.longitude-125.0) < 0.1) ||
      (fabs(location_.latitude-27.9) < 0.1 && fabs(location_.longitude-177.0) < 0.1) ||
      (fabs(location_.latitude-28.1) < 0.1 && fabs(location_.longitude-177.3) < 0.1) ||
      (fabs(location_.latitude-28.2) < 0.1 && fabs(location_.longitude-177.2) < 0.1) ||
      (fabs(location_.latitude-28.2) < 0.1 && fabs(location_.longitude-177.3) < 0.1) ||
      (fabs(location_.latitude-29.8) < 0.1 && fabs(location_.longitude-139.7) < 0.1) ||
      (fabs(location_.latitude-29.9) < 0.1 && fabs(location_.longitude-140.0) < 0.1) ||
      (fabs(location_.latitude-30.0) < 0.1 && fabs(location_.longitude-140.0) < 0.1) ||
      (fabs(location_.latitude-34.6) < 0.1 && fabs(location_.longitude-120.5) < 0.1) ||
      (fabs(location_.latitude-34.6) < 0.1 && fabs(location_.longitude-120.6) < 0.1) ||
      (fabs(location_.latitude-34.7) < 0.1 && fabs(location_.longitude-120.6) < 0.1) ||
      (fabs(location_.latitude-34.9) < 0.1 && fabs(location_.longitude-101.3) < 0.1) ||
      (fabs(location_.latitude-34.9) < 0.1 && fabs(location_.longitude-106.3) < 0.1) ||
      (fabs(location_.latitude-36.3) < 0.1 && fabs(location_.longitude-116.0) < 0.1) ||
      (fabs(location_.latitude-39.2) < 0.1 && fabs(location_.longitude-104.4) < 0.1) ||
      (fabs(location_.latitude-40.3) < 0.1 && fabs(location_.longitude-111.4) < 0.1) ||
      (fabs(location_.latitude-42.1) < 0.1 && fabs(location_.longitude-122.4) < 0.1) ||
      (fabs(location_.latitude-42.8) < 0.1 && fabs(location_.longitude-108.7) < 0.1) ||
      (fabs(location_.latitude-43.1) < 0.1 && fabs(location_.longitude-116.1) < 0.1) ||
      (fabs(location_.latitude-44.0) < 0.1 && fabs(location_.longitude-103.0) < 0.1) ||
      (fabs(location_.latitude-44.0) < 0.1 && fabs(location_.longitude-103.1) < 0.1) ||
      (fabs(location_.latitude-44.3) < 0.1 && fabs(location_.longitude-123.0) < 0.1) ||
      (fabs(location_.latitude-44.9) < 0.1 && fabs(location_.longitude-122.9) < 0.1) ||
      (fabs(location_.latitude-44.9) < 0.1 && fabs(location_.longitude-123.0) < 0.1) ||
      (fabs(location_.latitude-47.1) < 0.1 && fabs(location_.longitude-111.2) < 0.1) ||
      (fabs(location_.latitude-47.2) < 0.1 && fabs(location_.longitude-117.3) < 0.1) ||
      (fabs(location_.latitude-47.4) < 0.1 && fabs(location_.longitude-111.2) < 0.1) ||
      (fabs(location_.latitude-47.4) < 0.1 && fabs(location_.longitude-111.4) < 0.1) ||
      (fabs(location_.latitude-47.5) < 0.1 && fabs(location_.longitude-111.3) < 0.1) ||
      (fabs(location_.latitude-47.5) < 0.1 && fabs(location_.longitude-111.4) < 0.1) ||
      (fabs(location_.latitude-47.6) < 0.1 && fabs(location_.longitude-117.5) < 0.1) ||
      (fabs(location_.latitude-48.0) < 0.1 && fabs(location_.longitude-106.3) < 0.1) ||
      (fabs(location_.latitude-48.0) < 0.1 && fabs(location_.longitude-124.6) < 0.1) ||
      (fabs(location_.latitude-49.8) < 0.1 && fabs(location_.longitude-144.7) < 0.1) ||
      (fabs(location_.latitude-50.0) < 0.1 && fabs(location_.longitude-145.0) < 0.1) ||
      (fabs(location_.latitude-50.2) < 0.1 && fabs(location_.longitude-127.2) < 0.1) ||
      (fabs(location_.latitude-51.4) < 0.1 && fabs(location_.longitude-176.1) < 0.1) ||
      (fabs(location_.latitude-51.7) < 0.1 && fabs(location_.longitude-176.5) < 0.1) ||
      (fabs(location_.latitude-51.8) < 0.1 && fabs(location_.longitude-176.6) < 0.1) ||
      (fabs(location_.latitude-51.9) < 0.1 && fabs(location_.longitude-176.6) < 0.1) ||
      (fabs(location_.latitude-53.2) < 0.1 && fabs(location_.longitude-114.1) < 0.1) ||
      (fabs(location_.latitude-53.9) < 0.1 && fabs(location_.longitude-101.0) < 0.1) ||
      (fabs(location_.latitude-54.0) < 0.1 && fabs(location_.longitude-101.0) < 0.1) ||
      (fabs(location_.latitude-55.0) < 0.1 && fabs(location_.longitude-131.2) < 0.1) ||
      (fabs(location_.latitude-55.0) < 0.1 && fabs(location_.longitude-131.5) < 0.1) ||
      (fabs(location_.latitude-55.0) < 0.1 && fabs(location_.longitude-162.3) < 0.1) ||
      (fabs(location_.latitude-55.1) < 0.1 && fabs(location_.longitude-162.6) < 0.1) ||
      (fabs(location_.latitude-55.1) < 0.1 && fabs(location_.longitude-162.7) < 0.1) ||
      (fabs(location_.latitude-57.0) < 0.1 && fabs(location_.longitude-169.9) < 0.1) ||
      (fabs(location_.latitude-57.0) < 0.1 && fabs(location_.longitude-170.1) < 0.1) ||
      (fabs(location_.latitude-57.1) < 0.1 && fabs(location_.longitude-170.1) < 0.1) ||
      (fabs(location_.latitude-57.1) < 0.1 && fabs(location_.longitude-170.2) < 0.1) ||
      (fabs(location_.latitude-57.2) < 0.1 && fabs(location_.longitude-170.1) < 0.1) ||
      (fabs(location_.latitude-57.3) < 0.1 && fabs(location_.longitude-152.1) < 0.1) ||
      (fabs(location_.latitude-57.7) < 0.1 && fabs(location_.longitude-152.5) < 0.1) ||
      (fabs(location_.latitude-57.8) < 0.1 && fabs(location_.longitude-152.5) < 0.1) ||
      (fabs(location_.latitude-58.2) < 0.1 && fabs(location_.longitude-156.1) < 0.1) ||
      (fabs(location_.latitude-58.3) < 0.1 && fabs(location_.longitude-122.3) < 0.1) ||
      (fabs(location_.latitude-58.6) < 0.1 && fabs(location_.longitude-156.7) < 0.1) ||
      (fabs(location_.latitude-58.7) < 0.1 && fabs(location_.longitude-122.5) < 0.1) ||
      (fabs(location_.latitude-58.7) < 0.1 && fabs(location_.longitude-156.6) < 0.1) ||
      (fabs(location_.latitude-58.8) < 0.1 && fabs(location_.longitude-122.5) < 0.1) ||
      (fabs(location_.latitude-58.8) < 0.1 && fabs(location_.longitude-122.6) < 0.1) ||
      (fabs(location_.latitude-58.9) < 0.1 && fabs(location_.longitude-122.6) < 0.1) ||
      (fabs(location_.latitude-59.1) < 0.1 && fabs(location_.longitude-139.2) < 0.1) ||
      (fabs(location_.latitude-59.4) < 0.1 && fabs(location_.longitude-139.6) < 0.1) ||
      (fabs(location_.latitude-59.5) < 0.1 && fabs(location_.longitude-139.6) < 0.1) ||
      (fabs(location_.latitude-59.9) < 0.1 && fabs(location_.longitude-111.4) < 0.1) ||
      (fabs(location_.latitude-60.3) < 0.1 && fabs(location_.longitude-161.3) < 0.1) ||
      (fabs(location_.latitude-60.6) < 0.1 && fabs(location_.longitude-161.8) < 0.1) ||
      (fabs(location_.latitude-60.7) < 0.1 && fabs(location_.longitude-161.8) < 0.1) ||
      (fabs(location_.latitude-60.8) < 0.1 && fabs(location_.longitude-161.8) < 0.1) ||
      (fabs(location_.latitude-61.0) < 0.1 && fabs(location_.longitude-149.4) < 0.1) ||
      (fabs(location_.latitude-61.1) < 0.1 && fabs(location_.longitude-150.0) < 0.1) ||
      (fabs(location_.latitude-61.2) < 0.1 && fabs(location_.longitude-150.0) < 0.1) ||
      (fabs(location_.latitude-62.4) < 0.1 && fabs(location_.longitude-155.1) < 0.1) ||
      (fabs(location_.latitude-63.0) < 0.1 && fabs(location_.longitude-155.6) < 0.1) ||
      (fabs(location_.latitude-64.0) < 0.1 && fabs(location_.longitude-145.6) < 0.1) ||
      (fabs(location_.latitude-64.0) < 0.1 && fabs(location_.longitude-145.7) < 0.1) ||
      (fabs(location_.latitude-64.2) < 0.1 && fabs(location_.longitude-164.9) < 0.1) ||
      (fabs(location_.latitude-64.2) < 0.1 && fabs(location_.longitude-172.9) < 0.1) ||
      (fabs(location_.latitude-64.3) < 0.1 && fabs(location_.longitude-147.2) < 0.1) ||
      (fabs(location_.latitude-64.4) < 0.1 && fabs(location_.longitude-173.1) < 0.1) ||
      (fabs(location_.latitude-64.4) < 0.1 && fabs(location_.longitude-173.2) < 0.1) ||
      (fabs(location_.latitude-64.5) < 0.1 && fabs(location_.longitude-165.3) < 0.1) ||
      (fabs(location_.latitude-64.5) < 0.1 && fabs(location_.longitude-165.4) < 0.1) ||
      (fabs(location_.latitude-64.7) < 0.1 && fabs(location_.longitude-147.9) < 0.1) ||
      (fabs(location_.latitude-64.8) < 0.1 && fabs(location_.longitude-147.7) < 0.1) ||
      (fabs(location_.latitude-64.8) < 0.1 && fabs(location_.longitude-147.8) < 0.1) ||
      (fabs(location_.latitude-64.8) < 0.1 && fabs(location_.longitude-147.9) < 0.1) ||
      (fabs(location_.latitude-65.1) < 0.1 && fabs(location_.longitude-126.4) < 0.1) ||
      (fabs(location_.latitude-65.1) < 0.1 && fabs(location_.longitude-126.7) < 0.1) ||
      (fabs(location_.latitude-65.2) < 0.1 && fabs(location_.longitude-126.8) < 0.1) ||
      (fabs(location_.latitude-65.3) < 0.1 && fabs(location_.longitude-126.8) < 0.1) ||
      (fabs(location_.latitude-66.0) < 0.1 && fabs(location_.longitude-169.1) < 0.1) ||
      (fabs(location_.latitude-66.0) < 0.1 && fabs(location_.longitude-169.8) < 0.1) ||
      (fabs(location_.latitude-66.1) < 0.1 && fabs(location_.longitude-169.8) < 0.1) ||
      (fabs(location_.latitude-66.2) < 0.1 && fabs(location_.longitude-169.8) < 0.1) ||
      (fabs(location_.latitude-66.4) < 0.1 && fabs(location_.longitude-162.3) < 0.1) ||
      (fabs(location_.latitude-66.7) < 0.1 && fabs(location_.longitude-162.5) < 0.1) ||
      (fabs(location_.latitude-66.8) < 0.1 && fabs(location_.longitude-162.5) < 0.1) ||
      (fabs(location_.latitude-66.9) < 0.1 && fabs(location_.longitude-162.5) < 0.1) ||
      (fabs(location_.latitude-67.3) < 0.1 && fabs(location_.longitude-115.0) < 0.1) ||
      (fabs(location_.latitude-67.7) < 0.1 && fabs(location_.longitude-115.0) < 0.1) ||
      (fabs(location_.latitude-67.8) < 0.1 && fabs(location_.longitude-115.0) < 0.1) ||
      (fabs(location_.latitude-68.1) < 0.1 && fabs(location_.longitude-133.1) < 0.1) ||
      (fabs(location_.latitude-68.4) < 0.1 && fabs(location_.longitude-178.9) < 0.1) ||
      (fabs(location_.latitude-68.8) < 0.1 && fabs(location_.longitude-179.4) < 0.1) ||
      (fabs(location_.latitude-68.9) < 0.1 && fabs(location_.longitude-179.2) < 0.1) ||
      (fabs(location_.latitude-68.9) < 0.1 && fabs(location_.longitude-179.4) < 0.1) ||
      (fabs(location_.latitude-70.0) < 0.1 && fabs(location_.longitude-143.2) < 0.1) ||
      (fabs(location_.latitude-70.0) < 0.1 && fabs(location_.longitude-143.5) < 0.1) ||
      (fabs(location_.latitude-70.4) < 0.1 && fabs(location_.longitude-177.8) < 0.1) ||
      (fabs(location_.latitude-70.9) < 0.1 && fabs(location_.longitude-178.3) < 0.1) ||
      (fabs(location_.latitude-70.9) < 0.1 && fabs(location_.longitude-178.4) < 0.1) ||
      (fabs(location_.latitude-70.9) < 0.1 && fabs(location_.longitude-178.5) < 0.1) ||
      (fabs(location_.latitude-71.0) < 0.1 && fabs(location_.longitude-156.1) < 0.1) ||
      (fabs(location_.latitude-71.0) < 0.1 && fabs(location_.longitude-178.4) < 0.1) ||
      (fabs(location_.latitude-71.2) < 0.1 && fabs(location_.longitude-156.6) < 0.1) ||
      (fabs(location_.latitude-71.3) < 0.1 && fabs(location_.longitude-124.5) < 0.1) ||
      (fabs(location_.latitude-71.3) < 0.1 && fabs(location_.longitude-156.8) < 0.1) ||
      (fabs(location_.latitude-71.8) < 0.1 && fabs(location_.longitude-124.7) < 0.1) ||
      (fabs(location_.latitude-71.9) < 0.1 && fabs(location_.longitude-124.7) < 0.1) ||
      (fabs(location_.latitude-72.0) < 0.1 && fabs(location_.longitude-124.6) < 0.1) ||
      (fabs(location_.latitude-76.0) < 0.1 && fabs(location_.longitude-119.0) < 0.1) ||
      (fabs(location_.latitude-76.1) < 0.1 && fabs(location_.longitude-119.0) < 0.1) ||
      (fabs(location_.latitude-76.1) < 0.1 && fabs(location_.longitude-119.3) < 0.1) ||
      (fabs(location_.latitude-76.2) < 0.1 && fabs(location_.longitude-119.2) < 0.1) ||
      (fabs(location_.latitude-76.2) < 0.1 && fabs(location_.longitude-119.3) < 0.1) ||
      (fabs(location_.latitude-78.2) < 0.1 && fabs(location_.longitude-103.5) < 0.1)) {
    location_.longitude=-location_.longitude;
  }
  else if (fabs(location_.latitude-10.0) < 0.1 && fabs(location_.longitude-100.0) < 0.1) {
    location_.latitude=36.1;
    location_.longitude=-80.;
  }
  else if (fabs(location_.latitude-53.5) < 0.1 && fabs(location_.longitude-0.) < 0.1 && date_time_ == DateTime(1966,11,27,0,0)) {
    location_.latitude=51.4;
    location_.longitude=-10.2;
  }
  else if (fabs(location_.latitude-54.5) < 0.1 && fabs(location_.longitude-0.) < 0.1) {
    location_.latitude=53.5;
    location_.longitude=27.1;
  }
  else if (fabs(location_.latitude-52.9) < 0.1 && fabs(location_.longitude-0.) < 0.1) {
    location_.latitude=49.5;
    location_.longitude=36.1;
  }
  else if (fabs(location_.latitude-53.8) < 0.1 && fabs(location_.longitude-0.) < 0.1) {
    location_.latitude=54.5;
    location_.longitude=9.7;
  }
  else if (fabs(location_.latitude-9.9) < 0.1 && fabs(location_.longitude-120.0) < 0.1) {
    location_.latitude=9.2;
    location_.longitude=138.2;
  }
  else if (fabs(location_.latitude-9.9) < 0.1 && fabs(location_.longitude-120.1) < 0.1) {
    location_.latitude=55.1;
    location_.longitude=165.7;
  }
  else if (fabs(location_.latitude+77.9) < 0.1 && fabs(location_.longitude-116.7) < 0.1) {
    location_.longitude=166.7;
  }
  else if (fabs(location_.latitude+5.0) < 0.1 && fabs(location_.longitude+52.8) < 0.1) {
    location_.latitude=-location_.latitude;
  }
  else if ((fabs(location_.latitude-65.6) < 0.1 && fabs(location_.longitude+31.6) < 0.1) || (fabs(location_.latitude-65.6) < 0.1 && fabs(location_.longitude+31.7) < 0.1)) {
    location_.longitude-=6.;
  }
  else if (fabs(location_.latitude-52.7) < 0.1 && fabs(location_.longitude+0.5) < 0.1) {
    location_.longitude=-35.5;
  }
  else if (fabs(location_.latitude-64.0) < 0.1 && fabs(location_.longitude-26.2) < 0.1) {
    location_.latitude=67.2;
  }
  else if (fabs(location_.latitude-62.1) < 0.1 && fabs(location_.longitude-146.2) < 0.1) {
    location_.longitude+=2.;
  }
  tsr.flags.ubits=0;
  tsr.flags.source=56;
  nlev=source.number_of_levels();
  tsr.flags.height_temperature_status=tsr.flags.wind_status=0;
  tsr.flags.surface_level=0;
  tsr.flags.wind_units=0;
  tsr.flags.moisture_units=2;
  tsr.flags.additional_data=false;
/*
  if (static_cast<int>(raob.capacity) < nlev) {
    if (raob.capacity > 0) {
	delete[] levels;
	delete[] rcb;
    }
    raob.capacity=nlev;
    levels=new RaobLevel[raob.capacity];
    rcb=new size_t[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
  for (n=0; n < nlev; ++n) {
    rcb[n]=0;
    levels[n]=source.level(n);
    if (levels[n].height == 99999) levels[n].height=64000;
    if (levels[n].temperature < -99.) levels[n].temperature=99.;
    if (levels[n].moisture < -99.) levels[n].moisture=99.;
    if (levels[n].ucomp > -999. && levels[n].vcomp > -999.) {
	metcalcs::uv2wind(levels[n].ucomp,levels[n].vcomp,levels[n].wind_direction,ws);
	levels[n].wind_direction=ws;
	dd=levels[n].wind_direction;
// rotate the wind direction from grid-relative to lat/lon relative
	if (dd != 0) {
	  if (location_.longitude < 0.) {
	    dd=lround(dd+location_.longitude+80.);
	  }
	  else {
	    dd=lround(dd+location_.longitude-280.);
	  }
	}
	if (dd > 360) {
	  levels[n].wind_direction=dd-360;
	}
	else if (dd < 0) {
	  levels[n].wind_direction=dd+360;
	}
	else {
	  levels[n].wind_direction=dd;
	}
    }
    else {
	levels[n].wind_direction=500;
	levels[n].wind_speed=250.;
    }
  }
  if (date_time_.year() < 1966 || date_time_.year() > 1970) {
    nlev=0;
  }
  return *this;
}

Tsraob& Tsraob::operator=(const ArgentinaRaob& source)
{
  int n;

  date_time_=source.date_time();
  location_=source.location();
  tsr.flags.format=1;
  tsr.flags.ubits=0;
  tsr.flags.source=37;
  nlev=source.number_of_levels();
  tsr.flags.height_temperature_status=tsr.flags.wind_status=0;
  tsr.flags.surface_level=0;
  tsr.flags.wind_units=1;
  tsr.flags.moisture_units=2;
  tsr.flags.additional_data=false;
/*
  if (nlev > static_cast<int>(raob.capacity)) {
    if (levels != NULL)
	delete[] levels;
    if (rcb != NULL)
	delete[] rcb;
    raob.capacity=nlev;
    rcb=new size_t[raob.capacity];
    levels=new RaobLevel[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
  for (n=0; n < nlev; ++n) {
    rcb[n]=0;
    levels[n]=source.level(n);
    if (levels[n].height < -9999)
	levels[n].height=64000;
    if (levels[n].temperature < -99.)
	levels[n].temperature=99.;
    if (levels[n].moisture < -99.)
	levels[n].moisture=99.;
    if (levels[n].type == 0)
	tsr.flags.surface_level=n+1;
  }
  return *this;
}

Tsraob& Tsraob::operator=(const ChinaRaob& source)
{
  return *this;
}

Tsraob& Tsraob::operator=(const ChinaWind& source)
{
  int n,nl,nmiss;
  Pibal::PibalLevel pibal_level;

  date_time_=source.date_time();
  location_=source.location();
  tsr.flags.format=2;
  tsr.flags.ubits=0;
  tsr.flags.source=40;
  nlev=source.number_of_levels();
  tsr.flags.height_temperature_status=tsr.flags.wind_status=0;
  tsr.flags.surface_level=0;
  tsr.flags.wind_units=0;
  tsr.flags.moisture_units=0;
  tsr.flags.additional_data=false;
/*
  if (nlev > static_cast<int>(raob.capacity)) {
    if (levels != NULL)
      delete[] levels;
    if (rcb != NULL)
      delete[] rcb;
    raob.capacity=nlev;
    rcb=new size_t[raob.capacity];
    levels=new RaobLevel[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
  nl=0;
  for (n=0; n < nlev; ++n) {
    rcb[nl]=0;
    nmiss=0;
    pibal_level=source.level(n);
    levels[nl].height=pibal_level.height;
    if (levels[nl].height < -999) {
	levels[nl].height=64000;
	nmiss=2;
    }
    levels[nl].wind_direction=pibal_level.wind_direction;
    if (levels[nl].wind_direction < -99) {
	levels[nl].wind_direction=500;
	nmiss++;
    }
    levels[nl].wind_speed=pibal_level.wind_speed;
    if (levels[nl].wind_speed < -99) {
	levels[nl].wind_speed=250;
	nmiss++;
    }
    if (nmiss < 2) {
	levels[nl].level_quality=pibal_level.level_quality;
	levels[nl].type=9;
	if (levels[nl].level_quality > '0')
	  tsr.flags.additional_data=true;
	++nl;
    }
  }
  nlev=nl;
  if (tsr.flags.additional_data) {
    tsr.trel[0]=tsr.trel[1]=0.;
    tsr.ind[0]=tsr.ind[1]=tsr.ind[2]=1;
    for (n=3; n < 9; n++)
	tsr.ind[n]=0;
    tsr.nind=1;
  }
  return *this;
}

Tsraob& Tsraob::operator=(const GalapagosSounding& source)
{
  int n;

  date_time_=source.date_time();
  location_=source.location();
  tsr.flags.format=1;
  tsr.flags.ubits=0;
  tsr.flags.source=15;
  nlev=source.number_of_levels();
  if (nlev > 127) {
    if (nlev > 254) {
	std::cerr << "Error: too many levels - " << nlev << std::endl;
	exit(1);
    }
    else {
	tsr.flags.format+=8;
    }
  }
  tsr.flags.height_temperature_status=tsr.flags.wind_status=0;
  tsr.flags.surface_level=0;
  tsr.flags.wind_units=0;
  tsr.flags.moisture_units=2;
  tsr.flags.additional_data=true;
  tsr.trel[0]=tsr.trel[1]=0.;
  tsr.ind[0]=1;
  tsr.ind[1]=1;
  for (n=2; n < 9; n++)
    tsr.ind[n]=0;
  tsr.nind=0;
/*
  if (nlev > static_cast<int>(raob.capacity)) {
    if (levels != NULL)
      delete[] levels;
    if (rcb != NULL)
      delete[] rcb;
    raob.capacity=nlev;
    rcb=new size_t[raob.capacity];
    levels=new RaobLevel[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
  for (n=0; n < nlev; ++n) {
    rcb[n]=0;
    levels[n]=source.level(n);
    if (levels[n].height == 99999) {
	levels[n].height=64000;
    }
    if (levels[n].temperature < -99.) {
	levels[n].temperature=99.;
    }
    if (levels[n].moisture < -99.) {
	levels[n].moisture=99.;
    }
    if (levels[n].wind_direction > 900) {
	levels[n].wind_direction=500;
    }
    if (levels[n].wind_speed > 900) {
	levels[n].wind_speed=250;
    }
    if (levels[n].type == 0) {
	tsr.flags.surface_level=n+1;
    }
    else if (levels[n].type == 5) {
	levels[n].type=1;
    }
    else {
	levels[n].type=2;
    }
  }
  return *this;
}

Tsraob& Tsraob::operator=(const GalapagosHighResolutionSounding& source)
{
  int n;

  date_time_=source.date_time();
  location_=source.location();
  tsr.flags.format=1;
  tsr.flags.ubits=0;
  tsr.flags.source=15;
  nlev=source.number_of_levels();
  if (nlev > 127) {
    if (nlev > 254) {
	std::cerr << "Error: too many levels - " << nlev << std::endl;
	exit(1);
    }
    else {
	tsr.flags.format+=8;
    }
  }
  tsr.flags.height_temperature_status=tsr.flags.wind_status=0;
  tsr.flags.surface_level=0;
  tsr.flags.wind_units=0;
  tsr.flags.moisture_units=2;
  tsr.flags.additional_data=true;
  tsr.trel[0]=tsr.trel[1]=0.;
  tsr.ind[0]=0;
  tsr.ind[1]=1;
  for (n=2; n < 9; ++n) {
    tsr.ind[n]=0;
  }
  tsr.nind=0;
/*
  if (nlev > static_cast<int>(raob.capacity)) {
    if (levels != NULL)
      delete[] levels;
    if (rcb != NULL)
      delete[] rcb;
    raob.capacity=nlev;
    rcb=new size_t[raob.capacity];
    levels=new RaobLevel[raob.capacity];
  }
*/
if (static_cast<int>(levels.capacity()) < nlev) {
levels.resize(nlev);
rcb.resize(nlev);
}
  for (n=0; n < nlev; n++) {
    rcb[n]=0;
    levels[n]=source.level(n);
    if (levels[n].height == 99999) {
	levels[n].height=64000;
    }
    if (levels[n].temperature < -99.) {
	levels[n].temperature=99.;
    }
    if (levels[n].moisture < -99.) {
	levels[n].moisture=99.;
    }
    if (levels[n].wind_direction > 900) {
	levels[n].wind_direction=500;
    }
    if (levels[n].wind_speed > 900) {
	levels[n].wind_speed=250;
    }
    if (levels[n].type == 0) {
	tsr.flags.surface_level=n+1;
    }
    else {
	levels[n].type=9;
    }
  }
  return *this;
}

bool operator==(const Tsraob& source1,const Tsraob& source2)
{
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
    if (source1.levels[n].height != source2.levels[n].height) {
	return false;
    }
    if (!floatutils::myequalf(source1.levels[n].temperature,source2.levels[n].temperature)) {
	return false;
    }
    if (!floatutils::myequalf(source1.levels[n].moisture,source2.levels[n].moisture)) {
	return false;
    }
    if (source1.levels[n].wind_direction != source2.levels[n].wind_direction) {
	return false;
    }
    if (!floatutils::myequalf(source1.levels[n].wind_speed,source2.levels[n].wind_speed)) {
	return false;
    }
  }
  return true;
}

bool operator<(const Tsraob& source1,const Tsraob& source2)
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
  else {
    return false;
  }
}

bool operator>(const Tsraob& source1,const Tsraob& source2)
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
  else {
    return false;
  }
}
