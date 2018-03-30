#include <iomanip>
#include <raob.hpp>
#include <bsort.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

const size_t NavySpotRaob::ua_mand_pres[10]={1000,850,700,500,400,300,200,150,100,250};
const size_t NavySpotRaob::pibal_mand_pres[12]={850,700,500,400,300,250,200,150,100, 70, 50, 30};
const int NavySpotRaob::std_z[10]={  111, 1457, 3012, 5574, 7185, 9164,11784,13608,16180,10364};

size_t NavySpotRaob::merge()
{
  int n,nl;

  nl=nlev;
  for (n=1; n < nlev; ++n) {
    if (floatutils::myequalf(levels[n].pressure,levels[n-1].pressure)) {
	if ((levels[n].height == levels[n-1].height || levels[n].height == 99999 || levels[n-1].height == 99999) && (floatutils::myequalf(levels[n].temperature,levels[n-1].temperature) || levels[n].temperature > 999. || levels[n-1].temperature > 999.) && (floatutils::myequalf(levels[n].moisture,levels[n-1].moisture) || levels[n].moisture > 999. || levels[n-1].moisture > 999.) && (levels[n].wind_direction == levels[n-1].wind_direction || levels[n].wind_direction == 99999 || levels[n-1].wind_direction == 99999) && (floatutils::myequalf(levels[n].wind_speed,levels[n-1].wind_speed) || levels[n].wind_speed < 99990. || levels[n-1].wind_speed < 99990.)) {
	  if (levels[n].height == 99999) {
	    levels[n].height=levels[n-1].height;
	  }
	  if (levels[n].temperature > 999.) {
	    levels[n].temperature=levels[n-1].temperature;
	  }
	  if (levels[n].moisture > 999.) {
	    levels[n].moisture=levels[n-1].moisture;
	  }
	  if (levels[n].wind_direction == 99999) {
	    levels[n].wind_direction=levels[n-1].wind_direction;
	  }
	  if (levels[n].wind_speed < 99990.) {
	    levels[n].wind_speed=levels[n-1].wind_speed;
	  }
	  levels[n-1].pressure=0.;
	  levels[n-1].height=0;
	  --nl;
	}
    }
  }
  return nl;
}

size_t NavySpotRaob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

bool navy_compare(Raob::RaobLevel& left,Raob::RaobLevel& right)
{
  if (left.pressure >= right.pressure) {
    return true;
  }
  else {
    return false;
  }
}

void NavySpotRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *cbuf=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  size_t off=0,sig_type,m;
  int n,nl=0;
  short yr,mo,dy,hr;
  short ind,nmand,ntrop,nwind;
  short nmax=0,nsig=0,x;
  int dum;
  bool missing;
  char cid[5];

  strutils::strget(cbuf,yr,2);
  strutils::strget(&cbuf[2],mo,2);
  strutils::strget(&cbuf[4],dy,2);
  strutils::strget(&cbuf[6],hr,2);
  date_time_.set(1900+yr,mo,dy,hr*10000);
  strutils::strget(&cbuf[9],navyraob.type,1);
  bits::get(stream_buffer,ind,off+80,1);
  bits::get(stream_buffer,dum,off+81,11);
  if (dum == 0x3ff) {
    location_.latitude=-99.9;
  }
  else {
    if (dum > 0x3ff) {
	dum-=0x7ff;
    }
    location_.latitude=dum/10.;
  }
  bits::get(stream_buffer,dum,off+92,12);
  if (dum == 0x7ff) {
    location_.longitude=-199.9;
  }
  else {
    if (dum > 0x7ff) {
	dum-=0xfff;
    }
    location_.longitude=dum/10.;
  }
  switch (ind) {
    case 0:
    {
	if (navyraob.type == 5) {
	  navyraob.wind_ind=1;
	}
	else {
	  bits::get(stream_buffer,navyraob.wind_ind,off+104,1);
	}
	bits::get(stream_buffer,location_.elevation,off+110,13);
	if (location_.elevation == 0xfff) {
	  location_.elevation=-999;
	}
	else if (location_.elevation > 0xfff) {
	  location_.elevation-=0x1fff;
	}
	bits::get(stream_buffer,dum,off+123,17);
	location_.ID=strutils::ftos(dum,5,0,'0');
	break;
    }
    case 1:
    {
      bits::get(stream_buffer,cid,off+110,6,0,5);
      charconversions::ebcd_to_ascii(cid,cid,5);
	location_.ID.assign(cid,5);
	location_.elevation=-999;
	if (navyraob.type == 5) {
	  navyraob.wind_ind=1;
	}
	else {
	  navyraob.wind_ind=0;
	}
      break;
    }
    default:
    {
      std::cout << "bad indicator" << std::endl;
      exit(1);
    }
  }
  nlev=0;
  if (!fill_header_only) {
    off=140;
    switch (navyraob.type) {
	case 2:
	{
	  bits::get(stream_buffer,nmand,off+54,6);
	  bits::get(stream_buffer,nwind,off+42,6);
	  bits::get(stream_buffer,ntrop,off+48,6);
	  off+=60;
	  nl=nmand+ntrop+nwind*2;
	  break;
	}
	case 6:
	{
	  bits::get(stream_buffer,nmand,off+54,6);
	  off+=60;
	  nl=nmand*2;
	  break;
	}
	case 5:
	{
	  bits::get(stream_buffer,nmax,off+228,6);
	  bits::get(stream_buffer,nsig,off+234,6);
	  nl=12+nmax+nsig;
	  break;
	}
    }
/*
    if (static_cast<int>(raob.capacity) < nl) {
	if (raob.capacity > 0)
	  delete[] levels;
	raob.capacity=nl;
	levels=new RaobLevel[raob.capacity];
    }
*/
if (static_cast<int>(levels.capacity()) < nl)
levels.resize(nl);
    switch (navyraob.type) {
	case 2:
	{
// upper-air mandatory levels
	  for (n=0; n < nmand; ++n) {
	    bits::get(stream_buffer,levels[nlev].height,off,13);
	    if (levels[nlev].height == 0xfff) {
		levels[nlev].height=99999;
	    }
	    else if (levels[nlev].height > 0xfff) {
		levels[nlev].height-=0x1fff;
	    }
	    bits::get(stream_buffer,dum,off+13,11);
	    if (dum == 0x3ff) {
		levels[nlev].temperature=9999.9;
	    }
	    else {
		if (dum > 0x3ff) {
		  dum-=0x7ff;
		}
		levels[nlev].temperature=dum/10.;
	    }
	    bits::get(stream_buffer,dum,off+24,10);
	    if (dum == 0x1ff) {
		levels[nlev].moisture=9999.9;
	    }
	    else {
		levels[nlev].moisture=dum/10.;
	    }
	    bits::get(stream_buffer,levels[nlev].wind_direction,off+34,10);
	    if (levels[nlev].wind_direction == 0x1ff || levels[nlev].wind_direction == 0x3ff) {
		levels[nlev].wind_direction=99999;
	    }
	    bits::get(stream_buffer,dum,off+44,10);
	    if (dum == 0x1ff || dum == 0x3ff) {
		levels[nlev].wind_speed=99999.;
	    }
	    else {
		levels[nlev].wind_speed=dum;
	    }
	    bits::get(stream_buffer,dum,off+54,6);
	    if (dum < 10) {
		if (levels[nlev].height != 99999)
		  levels[nlev].height+=std_z[dum];
		levels[nlev].pressure=ua_mand_pres[dum];
	    }
	    else {
		levels[nlev].pressure=0.;
	    }
	    if (!floatutils::myequalf(levels[nlev].pressure,0.)) {
		levels[nlev].type=1;
		++nlev;
	    }
	    off+=60;
	  }
// unpack the tropopause levels
	  for (n=0; n < ntrop; n++) {
	    bits::get(stream_buffer,dum,off,13);
	    if (dum == 0xfff) {
		levels[nlev].pressure=9999.9;
	    }
	    else {
		levels[nlev].pressure=dum;
	    }
	    levels[nlev].height=99999;
	    bits::get(stream_buffer,dum,off+13,11);
	    if (dum == 0x3ff) {
		levels[nlev].temperature=9999.9;
	    }
	    else {
		if (dum > 0x3ff) {
		  dum-=0x7ff;
		}
		levels[nlev].temperature=dum/10.;
	    }
	    bits::get(stream_buffer,dum,off+24,10);
	    if (dum == 0x1ff) {
		levels[nlev].moisture=9999.9;
	    }
	    else {
		if (dum > 0x1ff) {
		  dum-=0x3ff;
		}
		levels[nlev].moisture=dum/10.;
	    }
	    bits::get(stream_buffer,levels[nlev].wind_direction,off+34,10);
	    if (levels[nlev].wind_direction == 0x1ff || levels[nlev].wind_direction == 0x3ff) {
		levels[nlev].wind_direction=99999;
	    }
	    bits::get(stream_buffer,dum,off+44,10);
	    if (dum == 0x1ff || dum == 0x3ff) {
		levels[nlev].wind_speed=99999.;
	    }
	    else {
		levels[nlev].wind_speed=dum;
	    }
	    if (!floatutils::myequalf(levels[nlev].pressure,0.) && levels[nlev].pressure < 600.) {
		levels[nlev].type=4;
		++nlev;
	    }
	    off+=60;
	  }
// unpack the max wind levels
	  for (n=0; n < nwind; n++) {
	    bits::get(stream_buffer,dum,off,13);
	    if (dum == 0xfff)
		levels[nlev].pressure=99999;
	    else
		levels[nlev].pressure=dum;
	    levels[nlev].height=99999;
	    levels[nlev].temperature=9999.9;
	    levels[nlev].moisture=9999.9;
	    bits::get(stream_buffer,levels[nlev].wind_direction,off+13,7);
	    if (levels[nlev].wind_direction == 0x7f)
		levels[nlev].wind_direction=99999;
	    else
		levels[nlev].wind_direction*=5;
	    bits::get(stream_buffer,dum,off+20,10);
	    if (dum == 0x1ff || dum == 0x3ff)
		levels[nlev].wind_speed=99999.;
	    else
		levels[nlev].wind_speed=dum;
	    if (!floatutils::myequalf(levels[nlev].pressure,0.)) {
		levels[nlev].type=5;
		++nlev;
	    }
	    off+=30;
	  }
//	  binary_sort(levels,nlev,navy_compare);
binary_sort(levels,navy_compare,0,nlev);
	  nl=merge();
	  if (nl != nlev) {
//	    binary_sort(levels,nlev,navy_compare);
binary_sort(levels,navy_compare,0,nlev);
	    nlev=nl;
	  }
	  break;
	}
	case 6:
	{
// upper-air significant levels
	  bits::get(stream_buffer,sig_type,off,30);
	  if (sig_type == 0x3fffffff) {
	    n=1;
	    off+=30;
	  }
	  else
	    n=0;
	  for (; n < nmand*2; n++) {
	    switch (sig_type) {
		case 0x3fffffff:
		  bits::get(stream_buffer,dum,off,13);
		  if (dum == 0x1fff)
		    levels[nlev].pressure=9999.9;
		  else
		    levels[nlev].pressure=dum;
		  levels[nlev].height=99999;
		  levels[nlev].temperature=9999.9;
		  levels[nlev].moisture=9999.9;
		  bits::get(stream_buffer,levels[nlev].wind_direction,off+13,7);
		  if (levels[nlev].wind_direction == 0x7f)
		    levels[nlev].wind_direction=99999;
		  else
		    levels[nlev].wind_direction*=5;
		  bits::get(stream_buffer,dum,off+20,10);
		  if (dum == 0x1ff || dum == 0x3ff)
		    levels[nlev].wind_speed=99999.;
		  else
		    levels[nlev].wind_speed=dum;
		  break;
		default:
		  bits::get(stream_buffer,dum,off,11);
		  if (dum == 0x7ff)
		    levels[nlev].pressure=9999.9;
		  else
		    levels[nlev].pressure=dum;
		  levels[nlev].height=99999;
		  bits::get(stream_buffer,dum,off+11,11);
		  if (dum == 0x3ff)
		    levels[nlev].temperature=9999.9;
		  else {
		    if (dum > 0x3ff)
			dum-=0x7ff;
		    levels[nlev].temperature=dum/10.;
		  }
		  bits::get(stream_buffer,dum,off+22,8);
		  if (dum == 0x7f)
		    levels[nlev].moisture=9999.9;
		  else
		    levels[nlev].moisture=dum/10.;
		  levels[nlev].wind_direction=99999;
		  levels[nlev].wind_speed=99999.;
	    }
	    if (!floatutils::myequalf(levels[nlev].pressure,0.)) {
		levels[nlev].type=2;
		++nlev;
	    }
	    off+=30;
	  }
//	  binary_sort(levels,nlev,navy_compare);
binary_sort(levels,navy_compare,0,nlev);
	  nl=merge();
	  if (nl != nlev) {
//	    binary_sort(levels,nlev,navy_compare);
binary_sort(levels,navy_compare,0,nlev);
	    nlev=nl;
	  }
	  break;
	}
	case 5:
	{
// pibal mandatory levels
	  for (n=0; n < 3; n++) {
	    for (m=0; m < 4; m++) {
		missing=false;
		levels[nlev].pressure=pibal_mand_pres[n*4+m];
		levels[nlev].height=99999;
		bits::get(stream_buffer,levels[nlev].wind_direction,off+m*15,7);
		if (levels[nlev].wind_direction == 0x7f) {
		  missing=true;
		}
		else {
		  levels[nlev].wind_direction*=5;
		}
		bits::get(stream_buffer,dum,off+m*15+7,8);
		if (dum == 0xff) {
		  missing=true;
		}
		else {
		  levels[nlev].wind_speed=dum;
		}
		if (!missing) {
		  levels[nlev].type=1;
		  ++nlev;
		}
	    }
	    off+=60;
	  }
// sig and max-wind levels
	  off+=60;
	  for (short n=0; n < (nmax+nsig); ++n) {
	    missing=false;
	    bits::get(stream_buffer,levels[nlev].height,off+2,13);
	    bits::get(stream_buffer,levels[nlev].wind_direction,off+15,7);
	    if (levels[nlev].wind_direction == 0x7f) {
		missing=true;
	    }
	    else {
		levels[nlev].wind_direction*=5;
	    }
	    bits::get(stream_buffer,dum,off+22,8);
	    if (dum == 0xff) {
		missing=true;
	    }
	    else {
		levels[nlev].wind_speed=dum;
	    }
	    if (!missing) {
		if (n+1 <= nmax) {
		  bits::get(stream_buffer,x,off,1);
		  if (x == 1) {
		    levels[nlev].pressure=levels[nlev].height;
		    levels[nlev].height=99999;
		  }
		  else {
		    levels[nlev].pressure=99999;
		    levels[nlev].height*=10;
		  }
		  levels[nlev].type=5;
		}
		else {
		  levels[nlev].pressure=99999;
		  levels[nlev].height*=10;
		  levels[nlev].type=2;
		}
		++nlev;
	    }
	    off+=30;
	  }
	  if ( ((nmax+nsig) % 2) != 0) {
	    off+=30;
	  }
	  break;
	}
    }
  }
}

void NavySpotRaob::print(std::ostream& outs) const
{
  int n;

  print_header(outs,true);
  outs.setf(std::ios::fixed);
  outs.precision(1);
  if (nlev > 0) {
    switch (navyraob.type) {
	case 2:
	case 6:
	  outs << "\n   LEV    PRES   HEIGHT   TEMP   DEWDEP    DD     FF   TYPE"
          << std::endl;
	  for (n=0; n < nlev; ++n) {
	    outs << std::setw(6) << n+1 << std::setw(8) << levels[n].pressure << std::setw(9) << levels[n].height << std::setw(7) << levels[n].temperature << std::setw(9) << levels[n].moisture << std::setw(6) << levels[n].wind_direction << std::setw(7) << levels[n].wind_speed << std::setw(7) << levels[n].type << std::endl;
	  }
	  outs << std::endl;
	  break;
	case 5:
	  outs << "\n   LEV    PRES   HEIGHT    DD     FF   TYPE" << std::endl;
	  for (n=0; n < nlev; ++n) {
	    outs << std::setw(6) << n+1 << std::setw(8) << levels[n].pressure << std::setw(9) << levels[n].height << std::setw(6) << levels[n].wind_direction << std::setw(6) << std::setw(7) << levels[n].wind_speed << std::setw(7) << levels[n].type << std::endl;
	  }
	  break;
    }
  }
}

void NavySpotRaob::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (verbose) {
    outs << "Station: " << location_.ID << "  Type: " << navyraob.type << "  Date: " << date_time_.to_string() << "Z  Location: " << std::setw(6) << location_.latitude << std::setw(8) << location_.longitude << std::setw(6) << location_.elevation << "  NumLevels: " << nlev << "  Wind Units: " << navyraob.wind_ind << std::endl;
  }
  else {
  }
}
