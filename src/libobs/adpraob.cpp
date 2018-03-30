#include <iomanip>
#include <string>
#include <raob.hpp>
#include <bsort.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

const float ADPRaob::mand_pres[20]={1000.,850.,700.,500.,400.,300.,250.,200.,
                                     150.,100., 70., 50., 30., 20., 10.,  7.,
                                       5.,  3.,  2.,  1.};
const int ADPRaob::mand_hgt[15]={   370,  4780,  9880, 18280, 23560, 30050,
                                  33980, 38660, 44680, 53170, 60650, 67690,
                                  78390, 86880,101880};

size_t ADPRaob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length)
  const
{
return 0;
}

bool compare_ADP_heights(Raob::RaobLevel& left,Raob::RaobLevel& right)
{
/*
  if (left.type == 0)
    return -1;
  else if (right.type == 0)
    return 1;
*/
  if (left.pressure < 1600. && right.pressure < 1600.) {
    if (left.pressure >= right.pressure)
	return true;
    else
	return false;
  }
  else {
    if (left.height <= right.height)
	return true;
    else
	return false;
  }
}

/*
int ADPComparePressures(Raob::RaobLevel& left,Raob::RaobLevel& right)
{
  if (myequalf(left.pressure,0.))
    return 1;
  else
    return -1;
}
*/

void ADPRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  short yr,mo,dy,hr,obhr,type;
  char *buffer=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  int nsig,nwp;
  short month[]={0,31,28,31,30,31,30,31,31,30,31,30,31};
  int dum,rpt_len,off,code,clen,nclev,noff;
  int n;
  size_t m,l;
//  RaobLevel *temp=NULL;
  bool resort=false,keep_level;
  char bcd[18];
  DateTime ref_date_time(1963,12,16,120000,0);

  nlev=0;
// Office Note 29 format
  auto t=std::string(buffer,4);
  if (t == "0000" || t == "1200" || t == "0600" || t == "1800") {
    strutils::strget(buffer,hr,4);
    strutils::strget(buffer+4,yr,2);
    strutils::strget(buffer+6,mo,2);
    strutils::strget(buffer+8,dy,2);
    strutils::strget(buffer+26,obhr,4);
    strutils::strget(buffer+10,dum,5);
    location_.latitude=dum/100.;
    strutils::strget(buffer+15,dum,5);
    if (dum != 99999 && dum >= 0 && dum <= 36000) {
	location_.longitude= (dum == 0) ? 0. : 360.-(dum/100.);
    }
    else {
	location_.longitude=-999.9;
    }
    location_.ID.assign(&buffer[20],6);
    strutils::strget(buffer+40,location_.elevation,5);
    strutils::strget(buffer+47,rpt_len,3);
    rpt_len*=10;
    off=50;
    while (off < rpt_len) {
	strutils::strget(buffer+off,code,2);
	strutils::strget(buffer+off+2,clen,3);
	strutils::strget(buffer+off+5,nclev,2);
// patch for when nclev = '**'
	if (nclev < 0) {
	  nclev=0;
	  code=99;
	}
/*
	if ( (nlev+nclev) > static_cast<int>(raob.capacity)) {
	  if (levels != NULL) {
	    temp=new RaobLevel[nlev];
	    for (n=0; n < nlev; n++)
		temp[n]=levels[n];
	    delete[] levels;
	  }
	  levels=new RaobLevel[nlev+nclev];
	  if (temp != NULL) {
	    for (n=0; n < nlev; n++)
		levels[n]=temp[n];
	    delete[] temp;
	    temp=NULL;
	  }
	  raob.capacity=nlev+nclev;
	}
*/
if (static_cast<int>(levels.capacity()) < (nlev+nclev))
levels.resize(nlev+nclev);
	noff=off+10;
	switch (code) {
	  case 1:
	  {
	    for (n=0,l=0; n < nclev; n++,l++) {
		m=nlev+n;
		levels[m].pressure=mand_pres[l];
		strutils::strget(buffer+noff,levels[m].height,5);
		strutils::strget(buffer+noff+5,levels[m].temperature,4);
		levels[m].temperature/=10.;
		strutils::strget(buffer+noff+9,levels[m].moisture,3);
		levels[m].moisture/=10.;
		if (levels[m].moisture < 99.)
		  levels[m].moisture=levels[m].temperature-levels[m].moisture;
		strutils::strget(buffer+noff+12,levels[m].wind_direction,3);
		strutils::strget(buffer+noff+15,levels[m].wind_speed,3);
		levels[m].type=1;
		levels[m].height_quality=buffer[noff+18];
		levels[m].temperature_quality=buffer[noff+19];
		levels[m].moisture_quality=buffer[noff+20];
		levels[m].wind_quality=buffer[noff+21];
		levels[m].pressure_quality=' ';
		noff+=22;
// remove missing levels
		if (levels[m].height == 99999 && levels[m].temperature > 99. && levels[m].moisture > 99. && levels[m].wind_direction == 999 && levels[m].wind_speed > 990.) {
		  n--;
		  nclev--;
		}
	    }
	    nlev=nlev+nclev;
	    break;
	  }
	  case 2:
	  {
	    for (n=0; n < nclev; n++) {
		m=nlev+n;
		strutils::strget(buffer+noff,levels[m].pressure,5);
		levels[m].pressure*=0.1;
		strutils::strget(buffer+noff+5,levels[m].temperature,4);
		levels[m].temperature*=0.1;
		strutils::strget(buffer+noff+9,levels[m].moisture,3);
		levels[m].moisture*=0.1;
		if (levels[m].moisture < 99.)
		  levels[m].moisture=levels[m].temperature-levels[m].moisture;
		if (n == 0)
		  levels[m].type=0;
		else
		  levels[m].type=2;
		levels[m].pressure_quality=buffer[noff+12];
		levels[m].temperature_quality=buffer[noff+13];
		levels[m].moisture_quality=buffer[noff+14];
		levels[m].height=99999;
		levels[m].wind_direction=999;
		levels[m].wind_speed=999.;
		levels[m].height_quality=levels[m].wind_quality=' ';
		noff+=15;
	    }
	    nlev=nlev+nclev;
	    break;
	  }
	  case 3:
	  {
	    for (n=0; n < nclev; n++) {
		m=nlev+n;
		strutils::strget(buffer+noff,levels[m].pressure,5);
		levels[m].pressure*=0.1;
		strutils::strget(buffer+noff+5,levels[m].wind_direction,3);
		strutils::strget(buffer+noff+8,levels[m].wind_speed,3);
		if (n == 0)
		  levels[m].type=0;
		else
		  levels[m].type=2;
		levels[m].pressure_quality=buffer[noff+11];
		levels[m].wind_quality=buffer[noff+12];
		levels[m].height=99999;
		levels[m].temperature=levels[m].moisture=99.9;
		levels[m].height_quality=levels[m].temperature_quality=
              levels[m].moisture_quality=' ';
		noff+=13;
	    }
	    nlev=nlev+nclev;
	    break;
	  }
	  case 4:
	  {
	    for (n=0; n < nclev; n++) {
		m=nlev+n;
		strutils::strget(buffer+noff,levels[m].height,5);
		strutils::strget(buffer+noff+5,levels[m].wind_direction,3);
		strutils::strget(buffer+noff+8,levels[m].wind_speed,3);
		levels[m].height_quality=buffer[noff+11];
		levels[m].wind_quality=buffer[noff+12];
		levels[m].pressure=9999.9;
		levels[m].temperature=levels[m].moisture=99.9;
		if (n == 0)
		  levels[m].type=0;
		else
		  levels[m].type=2;
		levels[m].pressure_quality=levels[m].temperature_quality=levels[m].moisture_quality=' ';
		noff+=13;
	    }
	    nlev=nlev+nclev;
	    break;
	  }
	  case 5:
	  {
	    for (n=0; n < nclev; n++) {
		m=nlev+n;
		strutils::strget(buffer+noff,levels[m].pressure,5);
		levels[m].pressure*=0.1;
		strutils::strget(buffer+noff+5,levels[m].temperature,4);
		levels[m].temperature*=0.1;
		strutils::strget(buffer+noff+9,levels[m].moisture,3);
		levels[m].moisture*=0.1;
		if (levels[m].moisture < 99.)
		  levels[m].moisture=levels[m].temperature-levels[m].moisture;
		strutils::strget(buffer+noff+12,levels[m].wind_direction,3);
		strutils::strget(buffer+noff+15,levels[m].wind_speed,3);
		levels[m].type=4;
		levels[m].pressure_quality=buffer[noff+18];
		levels[m].temperature_quality=buffer[noff+19];
		levels[m].moisture_quality=buffer[noff+20];
		levels[m].wind_quality=buffer[noff+21];
		levels[m].height=99999;
		levels[m].height_quality=' ';
		noff+=22;
	    }
	    nlev=nlev+nclev;
	    break;
	  }
	  case 7:
	  case 8:
	  {
	    break;
	  }
	  default:
	  {
	    buffer[rpt_len]='\0';
	    std::cerr << "Error: unrecognized category code " << code << " in sounding:" << std::endl;
	    std::cerr << buffer << std::endl;
	    std::cerr << "Sounding truncated and may contain errors" << std::endl;
//	    exit(1);
	    clen=rpt_len;
	  }
	}
	clen*=10;
	off+=(clen-off);
    }
  }
// binary ADP
  else {
    bits::get(stream_buffer,yr,43,7);
    bits::get(stream_buffer,mo,50,4);
    bits::get(stream_buffer,dy,54,5);
    bits::get(stream_buffer,hr,59,5);
    bits::get(stream_buffer,obhr,137,5);
// these are missing codes, so set the ob hour to the synoptic hour
    if (obhr == 24 || obhr == 31)
	obhr=hr;
    obhr*=100;
    bits::get(stream_buffer,dum,100,18);
    if (dum > 0x1ffff) dum=0x20000-dum;
    location_.latitude=dum/10.;
    bits::get(stream_buffer,dum,118,18);
    location_.longitude=dum/10.;
    location_.longitude= (location_.longitude <= 180.) ? -location_.longitude : 360.-location_.longitude;
    bits::get(stream_buffer,type,142,6);
// block/station number
    if (type < 0x8 || type == 0x20 || type == 0x24) {
	bits::get(stream_buffer,dum,148,14);
	location_.ID=strutils::ftos(dum,2,0,'0');
	bits::get(stream_buffer,dum,162,10);
	location_.ID+=strutils::ftos(dum,3,0,'0');
    }
// call letters
    else {
	bits::get(stream_buffer,bcd,148,6,0,4);
	charconversions::ebcd_to_ascii(bcd,bcd,4);
	location_.ID.assign(bcd,4);
    }
    if (DateTime(1900+yr,mo,dy,hr*10000,0) < ref_date_time)
	location_.elevation=-999;
    else {
	bits::get(stream_buffer,location_.elevation,1795,15);
	if (location_.elevation == 0x3fff) {
	  location_.elevation=-999;
	}
	else if (location_.elevation > 0x3fff) {
	  location_.elevation=0x4000-location_.elevation;
	}
    }
// mandatory levels
/*
    if (raob.capacity < 15) {
	if (levels != NULL)
	  delete[] levels;
	raob.capacity=15;
	levels=new RaobLevel[raob.capacity];
    }
*/
if (levels.capacity() < 15)
levels.resize(15);
    off=172;
    for (n=0,l=0; n < 15; n++,l++) {
	levels[nlev].pressure=mand_pres[l];
	bits::get(stream_buffer,levels[nlev].height,off+18,18);
	if (levels[nlev].height == 0x1ffff)
	  levels[nlev].height=64000;
	else {
	  if (levels[nlev].height > 0x1ffff)
	    levels[nlev].height=0x20000-levels[nlev].height;
// convert D-val in feet to meters
	  levels[nlev].height=lround((mand_hgt[l]+levels[nlev].height*10)*0.3048);
	}
	bits::get(stream_buffer,dum,off+36,18);
	if (dum == 0x1ffff)
	  levels[nlev].temperature=99.9;
	else {
	  if (dum > 0x1ffff)
	    dum=0x20000-dum;
	  levels[nlev].temperature=dum/10.;
	}
	bits::get(stream_buffer,dum,off+54,18);
	if (dum == 0x1ffff)
	  levels[nlev].moisture=99.9;
	else {
	  if (dum > 0x1ffff)
	    dum=0x20000-dum;
	  levels[nlev].moisture=dum/10.;
	}
	bits::get(stream_buffer,levels[nlev].wind_direction,off+72,18);
	if (levels[nlev].wind_direction == 0x1ffff)
	  levels[nlev].wind_direction=500;
	else if (yr < 68)
	  levels[nlev].wind_direction*=10;
	bits::get(stream_buffer,dum,off+90,18);
	if (dum == 0x1ffff)
	  levels[nlev].wind_speed=250.;
	else
	  levels[nlev].wind_speed=dum;
// remove missing levels
	if (levels[nlev].height != 64000 || levels[nlev].temperature < 99.  || levels[nlev].moisture < 99. || (levels[nlev].wind_direction != 500 && levels[nlev].wind_speed < 250.)) {
	  levels[nlev].type=1;
	  levels[nlev].pressure_quality=levels[nlev].height_quality=levels[nlev].temperature_quality=levels[nlev].moisture_quality=levels[nlev].wind_quality=' ';
	  nlev++;
	}
	off+=108;
    }
// significant levels
    off=1864;
    bits::get(stream_buffer,nsig,1828,9);
    if (nsig > 0) {
/*
	if (static_cast<int>(raob.capacity) < nlev+nsig) {
	  if (levels != NULL) {
	    temp=new RaobLevel[nlev];
	    for (n=0; n < nlev; n++)
		temp[n]=levels[n];
	    delete[] levels;
	  }
	  levels=new RaobLevel[nlev+nsig];
	  if (temp != NULL) {
	    for (n=0; n < nlev; n++)
		levels[n]=temp[n];
	    delete[] temp;
	    temp=NULL;
	  }
	  raob.capacity=nlev+nsig;
	}
*/
if (static_cast<int>(levels.capacity()) < (nlev+nsig))
levels.resize(nlev+nsig);
	for (n=0; n < nsig; ++n) {
	  bits::get(stream_buffer,dum,off+1,11);
	  if (dum == 0x7ff) {
	    levels[nlev].pressure=1600.;
	  }
	  else {
	    levels[nlev].pressure=dum;
	  }
	  bits::get(stream_buffer,dum,off,1);
	  levels[nlev].height=64000;
	  if (yr < 68 && n == 0 && levels[nlev].pressure < 1600. && levels[nlev].pressure > 850.) {
	    levels[nlev].type=0;
	    if (location_.elevation != -999) {
		levels[nlev].height=location_.elevation;
	    }
	  }
	  else {
	    if (dum == 1) {
		levels[nlev].type=4;
	    }
	    else {
		levels[nlev].type=2;
	    }
	  }
	  bits::get(stream_buffer,dum,off+12,12);
	  if (dum == 0x7ff) {
	    levels[nlev].temperature=99.9;
	  }
	  else {
	    if (dum > 0x7ff) {
		dum=0x800-dum;
	    }
	    levels[nlev].temperature=dum/10.;
	  }
	  bits::get(stream_buffer,dum,off+24,12);
	  if (dum == 0x7ff)
	    levels[nlev].moisture=99.9;
	  else {
	    if (dum > 0x7ff)
		dum=0x800-dum;
	    levels[nlev].moisture=dum/10.;
	  }
	  levels[nlev].wind_direction=500;
	  levels[nlev].wind_speed=250.;
// remove missing levels
	  if (!floatutils::myequalf(levels[nlev].pressure,1600.) || levels[nlev].temperature < 99.  || levels[nlev].moisture < 99.) {
	    levels[nlev].pressure_quality=levels[nlev].height_quality=levels[nlev].temperature_quality=levels[nlev].moisture_quality=levels[nlev].wind_quality=' ';
	    nlev++;
	  }
	  off+=36;
	}
    }
// wind-by-pressure levels
    bits::get(stream_buffer,nwp,1837,9);
    if (nwp > 0) {
	if (yr < 68) {
/*
	  if (static_cast<int>(raob.capacity) < nlev+nwp) {
	    if (levels != NULL) {
		temp=new RaobLevel[nlev];
		for (n=0; n < nlev; n++)
		  temp[n]=levels[n];
		delete[] levels;
	    }
	    levels=new RaobLevel[nlev+nwp];
	    if (temp != NULL) {
		for (n=0; n < nlev; n++)
		  levels[n]=temp[n];
		delete[] temp;
		temp=NULL;
	    }
	    raob.capacity=nlev+nwp;
	  }
*/
if (static_cast<int>(levels.capacity()) < (nlev+nwp))
levels.resize(nlev+nwp);
	  for (n=0; n < nwp; n++) {
	    levels[nlev].type=2;
	    bits::get(stream_buffer,dum,off,12);
	    if (dum == 0xfff)
		levels[nlev].pressure=1600.;
	    else
		levels[nlev].pressure=dum;
	    bits::get(stream_buffer,levels[nlev].wind_direction,off+12,12);
	    if (levels[nlev].wind_direction == 0xfff)
		levels[nlev].wind_direction=500;
	    else
		levels[nlev].wind_direction*=10;
	    bits::get(stream_buffer,dum,off+24,12);
	    if (dum == 0xfff)
		levels[nlev].wind_speed=250.;
	    else
		levels[nlev].wind_speed=dum;
	    levels[nlev].height=64000;
	    levels[nlev].temperature=levels[nlev].moisture=99.9;
	    levels[nlev].pressure_quality=levels[nlev].height_quality=levels[nlev].temperature_quality=levels[nlev].moisture_quality=levels[nlev].wind_quality=' ';
	    nlev++;
	    off+=36;
	  }
	}
	else {
/*
	  if (static_cast<int>(raob.capacity) < nlev+nwp/3) {
	    if (levels != NULL) {
		temp=new RaobLevel[nlev];
		for (n=0; n < nlev; n++)
		  temp[n]=levels[n];
		delete[] levels;
	    }
	    levels=new RaobLevel[nlev+nwp/3];
	    if (temp != NULL) {
		for (n=0; n < nlev; n++)
		  levels[n]=temp[n];
		delete[] temp;
		temp=NULL;
	    }
	    raob.capacity=nlev+nwp/3;
	  }
*/
if (static_cast<int>(levels.capacity()) < (nlev+nwp/3))
levels.resize(nlev+nwp/3);
	  for (n=0; n < nwp/3; ++n) {
	    bits::get(stream_buffer,bcd,off,6,0,18);
	    charconversions::ebcd_to_ascii(bcd,bcd,18);
	    for (m=0; m < 18; m++) {
		if (bcd[m] == ':') {
		  bcd[m]='0';
		}
	    }
	    strutils::strget(&bcd[1],type,2);
	    if (!strutils::is_numeric(std::string(&bcd[3],3))) {
		levels[nlev].pressure=999;
	    }
	    else {
		strutils::strget(&bcd[3],levels[nlev].pressure,3);
	    }
	    if (!floatutils::myequalf(levels[nlev].pressure,999.) && !floatutils::myequalf(levels[nlev].pressure,0.)) {
		keep_level=false;
		levels[nlev].height=64000;
		levels[nlev].pressure_quality=levels[nlev].height_quality=levels[nlev].temperature_quality=levels[nlev].moisture_quality=levels[nlev].wind_quality=' ';
		switch (type) {
		  case 99:
		  {
// surface
		    if (levels[nlev].pressure < 500.) {
			levels[nlev].pressure+=1000.;
		    }
		    if (location_.elevation != -999) {
			levels[nlev].height=location_.elevation;
		    }
		    if (!strutils::is_numeric(std::string(&bcd[7],3))) {
			levels[nlev].temperature=99.9;
		    }
		    else {
			strutils::strget(&bcd[7],levels[nlev].temperature,3);
			if ((static_cast<int>(levels[nlev].temperature) % 2) == 1) {
			  levels[nlev].temperature=-levels[nlev].temperature;
			}
			levels[nlev].temperature/=10.;
			keep_level=true;
		    }
		    if (!strutils::is_numeric(std::string(&bcd[10],2))) {
			levels[nlev].moisture=99.9;
		    }
		    else {
			strutils::strget(&bcd[10],levels[nlev].moisture,2);
			levels[nlev].moisture=levels[nlev].temperature-levels[nlev].moisture/10.;
			keep_level=true;
		    }
		    if (!strutils::is_numeric(std::string(&bcd[13],2))) {
			levels[nlev].wind_direction=500;
		    }
		    else {
			strutils::strget(&bcd[13],levels[nlev].wind_direction,2);
			levels[nlev].wind_direction*=10;
			if (levels[nlev].wind_direction > 360) {
			  levels[nlev].wind_direction=500;
			}
			else {
			  keep_level=true;
			}
		    }
		    if (levels[nlev].wind_direction == 500 || !strutils::is_numeric(std::string(&bcd[15],3))) {
			levels[nlev].wind_speed=250.;
		    }
		    else {
			strutils::strget(&bcd[15],levels[nlev].wind_speed,3);
			if (levels[nlev].wind_speed > 500.) {
			  levels[nlev].wind_direction+=5;
			  levels[nlev].wind_speed-=500.;
			}
			keep_level=true;
		    }
		    levels[nlev].type=0;
		    if (keep_level) {
			++nlev;
		    }
		    break;
		  }
		  case 88:
		  {
// tropopause
		    if (bcd[0] == '2' && levels[nlev].pressure >= 100.)
			levels[nlev].pressure/=10.;
		    if (!strutils::is_numeric(std::string(&bcd[7],3)))
			levels[nlev].temperature=99.9;
		    else {
			strutils::strget(&bcd[7],levels[nlev].temperature,3);
			if ((static_cast<int>(levels[nlev].temperature) % 2) == 1)
			  levels[nlev].temperature=-levels[nlev].temperature;
			levels[nlev].temperature/=10.;
			keep_level=true;
		    }
		    if (!strutils::is_numeric(std::string(&bcd[10],2)))
			levels[nlev].moisture=99.9;
		    else {
			strutils::strget(&bcd[10],levels[nlev].moisture,2);
			levels[nlev].moisture=levels[nlev].temperature-levels[nlev].moisture/10.;
			keep_level=true;
		    }
		    if (!strutils::is_numeric(std::string(&bcd[13],2)))
			levels[nlev].wind_direction=500;
		    else {
			strutils::strget(&bcd[13],levels[nlev].wind_direction,2);
			levels[nlev].wind_direction*=10;
			if (levels[nlev].wind_direction > 360)
			  levels[nlev].wind_direction=500;
			else
			  keep_level=true;
		    }
		    if (levels[nlev].wind_direction == 500 || !strutils::is_numeric(std::string(&bcd[15],3)))
			levels[nlev].wind_speed=250.;
		    else {
			strutils::strget(&bcd[15],levels[nlev].wind_speed,3);
			if (levels[nlev].wind_speed > 500.) {
			  levels[nlev].wind_direction+=5;
			  levels[nlev].wind_speed-=500.;
			}
			keep_level=true;
		    }
		    levels[nlev].type=4;
		    if (keep_level)
			nlev++;
		    break;
		  }
		  case 77:
		  {
// max wind level
		    levels[nlev].temperature=99.9;
		    levels[nlev].moisture=99.9;
		    if (!strutils::is_numeric(std::string(&bcd[7],2)))
			levels[nlev].wind_direction=500;
		    else {
			strutils::strget(&bcd[7],levels[nlev].wind_direction,2);
			levels[nlev].wind_direction*=10;
			if (levels[nlev].wind_direction > 360)
			  levels[nlev].wind_direction=500;
			else
			  keep_level=true;
		    }
		    if (levels[nlev].wind_direction == 500 || !strutils::is_numeric(std::string(&bcd[9],3)))
			levels[nlev].wind_speed=250.;
		    else {
			strutils::strget(&bcd[9],levels[nlev].wind_speed,3);
			if (levels[nlev].wind_speed > 500.) {
			  levels[nlev].wind_direction+=5;
			  levels[nlev].wind_speed-=500.;
			}
			keep_level=true;
		    }
		    levels[nlev].type=5;
		    if (keep_level)
			nlev++;
		    break;
		  }
		  case 66:
		  {
		    if (bcd[0] == '2' && levels[nlev].pressure >= 100.)
			levels[nlev].pressure/=10.;
		    levels[nlev].temperature=99.9;
		    levels[nlev].moisture=99.9;
		    if (!strutils::is_numeric(std::string(&bcd[7],2)))
			levels[nlev].wind_direction=500;
		    else {
			strutils::strget(&bcd[7],levels[nlev].wind_direction,2);
			levels[nlev].wind_direction*=10;
			if (levels[nlev].wind_direction > 360)
			  levels[nlev].wind_direction=500;
			else
			  keep_level=true;
		    }
		    if (levels[nlev].wind_direction == 500 || !strutils::is_numeric(std::string(&bcd[9],3)))
			levels[nlev].wind_speed=250.;
		    else {
			strutils::strget(&bcd[9],levels[nlev].wind_speed,3);
			if (levels[nlev].wind_speed > 500.) {
			  levels[nlev].wind_direction+=5;
			  levels[nlev].wind_speed-=500.;
			}
			keep_level=true;
		    }
		    levels[nlev].type=2;
		    if (keep_level)
			nlev++;
		    break;
		  }
		}
	    }
	    off+=108;
	  }
	}
    }
  }
  if (hr == 0 && obhr >= 2100) {
    --dy;
    if (dy == 0) {
	if ( (yr % 4) == 0) month[2]=29;
	--mo;
	if (mo == 0) {
	  --yr;
	  mo=12;
	}
	dy=month[mo];
	month[2]=28;
    }
  }
  if (yr >= 62) {
    date_time_.set(1900+yr,mo,dy,obhr*100);
  }
  else {
    date_time_.set(2000+yr,mo,dy,obhr*100);
  }
// sort the levels in the sounding
  for (n=0; n < nlev; ++n) {
    if (levels[n].height >= 64000 && levels[n].pressure < 9999. && levels[n].pressure > 0.) {
	levels[n].height=metcalcs::stdp2z(levels[n].pressure);
	levels[n].height_quality='x';
    }
  }
//  binary_sort(levels,nlev,compare_ADP_heights);
binary_sort(levels,compare_ADP_heights,0,nlev);
  for (n=0; n < nlev; n++) {
    if (levels[n].height_quality == 'x') {
	levels[n].height=99999;
	levels[n].height_quality=' ';
    }
  }
// merge any duplicate levels
  nclev=nlev;
  for (n=1,m=0; n < nlev; n++,m++) {
    if ((levels[n].pressure < 9999. && floatutils::myequalf(levels[n].pressure,levels[m].pressure)) || (levels[n].type == 0 && levels[m].type == 0)) {
	levels[n].pressure= (levels[n].pressure > 9999.) ? levels[m].pressure : levels[n].pressure;
	levels[n].height= (levels[n].height == 99999) ? levels[m].height : levels[n].height;
	levels[n].temperature= (levels[n].temperature > 99.) ? levels[m].temperature : levels[n].temperature;
	levels[n].moisture= (levels[n].moisture > 99.) ? levels[m].moisture : levels[n].moisture;
	levels[n].wind_direction= (levels[n].wind_direction == 999) ? levels[m].wind_direction : levels[n].wind_direction;
	levels[n].wind_speed= (levels[n].wind_speed > 990.) ? levels[m].wind_speed : levels[n].wind_speed;
	levels[n].pressure_quality= (levels[n].pressure_quality == ' ') ? levels[m].pressure_quality : levels[n].pressure_quality;
	levels[n].height_quality= (levels[n].height_quality == ' ') ? levels[m].height_quality : levels[n].height_quality;
	levels[n].temperature_quality= (levels[n].temperature_quality == ' ') ? levels[m].temperature_quality : levels[n].temperature_quality;
	levels[n].moisture_quality= (levels[n].moisture_quality == ' ') ? levels[m].moisture_quality : levels[n].moisture_quality;
	levels[n].wind_quality= (levels[n].wind_quality == ' ') ? levels[m].wind_quality : levels[n].wind_quality;
	if (levels[n].type == 0 || levels[m].type == 0) {
	  levels[n].type=0;
	}
	else if (levels[n].type == 4 || levels[m].type == 4) {
	  levels[n].type=4;
	}
	else {
	  levels[n].type= (levels[n].type >= levels[m].type) ? levels[n].type : levels[m].type;
	}
	levels[m].pressure=0.;
	nclev--;
	resort=true;
    }
  }
  if (resort) {
//    binary_sort(levels,nlev,ADPComparePressures);
binary_sort(levels,
[](Raob::RaobLevel& left,Raob::RaobLevel& right) -> bool
{
if (floatutils::myequalf(left.pressure,0.)) {
return false;
}
else {
return true;
}
},
0,nlev);
  }
  nlev=nclev;
}

void ADPRaob::print(std::ostream& outs) const
{
  int n;

  print_header(outs,true);

  outs.precision(1);
  outs << "\n  LEV    PRES Q    HGT Q   TEMP Q   DEWP Q   DD    FF Q  TYPE" <<
    std::endl;
  for (n=0; n < nlev; n++) {
    outs << std::setw(5) << n+1 << std::setw(8) << levels[n].pressure << " " << levels[n].pressure_quality << std::setw(7) << levels[n].height << " " << levels[n].height_quality << std::setw(7) << levels[n].temperature << " " << levels[n].temperature_quality << std::setw(7) << levels[n].moisture << " " << levels[n].moisture_quality << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << " " << levels[n].wind_quality << std::setw(6) << levels[n].type <<  std::endl;
  }

  outs << std::endl;
}

void ADPRaob::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);

  if (verbose) {
    outs << "  Station: " << location_.ID << "  Date: " << date_time_.to_string() << "Z  Location: " << std::setw(6) << location_.latitude << std::setw(8) << location_.longitude << std::setw(6) << location_.elevation << std::endl;
  }
  else {
    outs << " Stn=" << location_.ID << " Date=" << std::setfill('0') << std::setw(4) << date_time_.year() << std::setw(2) << date_time_.month() << std::setw(2) << date_time_.day() << std::setw(4) << date_time_.time() << std::setfill(' ') << " Lat=" << location_.latitude << " Lon=" << location_.longitude << " Elev=" << location_.elevation << std::endl;
  }
}
