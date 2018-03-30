// FILE: argentinaraob.cpp

#include <iomanip>
#include <raob.hpp>
#include <bsort.hpp>
#include <strutils.hpp>

int InputArgentinaRaobStream::ignore()
{
return -1;
}

int InputArgentinaRaobStream::peek()
{
return -1;
}

int InputArgentinaRaobStream::read(unsigned char *buffer,size_t buffer_length)
{
  const size_t BUFFER_LENGTH=256;
  char buf[BUFFER_LENGTH];
  int bytes_read=0,b;

  if (icosstream != NULL) {
    if (last_ID.empty()) {
	b=icosstream->read(reinterpret_cast<unsigned char *>(buf),buffer_length);
	if (b < 0)
	  return bfstream::eof;
	last_record=std::string(buf,b);
	last_ID=last_record.substr(2,11);
    }
    while (last_record.substr(2,11) == last_ID) {
	std::copy(last_record.c_str(),last_record.c_str()+last_record.length(),&buffer[bytes_read]);
	bytes_read+=last_record.length();
	buffer[bytes_read++]=0xa;
	if ( (buffer_length-bytes_read) < 81) {
	  std::cerr << "Error: buffer overflow" << std::endl;
	  exit(1);
	}
	b=icosstream->read(reinterpret_cast<unsigned char *>(buf),buffer_length);
	if (b < 0) {
	  last_record="";
	  break;
	}
	last_record=std::string(buf,b);
    }
  }
  else {
    if (last_ID.empty()) {
	fs.getline(buf,BUFFER_LENGTH);
	if (fs.eof()) {
	  return bfstream::eof;
	}
	last_record=buf;
	last_ID=last_record.substr(2,11);
    }
    while (last_record.substr(2,11) == last_ID) {
	std::copy(last_record.c_str(),last_record.c_str()+last_record.length(),&buffer[bytes_read]);
	bytes_read+=last_record.length();
	if ( (buffer_length-bytes_read) < 81) {
	  std::cerr << "Error: buffer overflow" << std::endl;
	  exit(1);
	}
	fs.getline(buf,BUFFER_LENGTH);
	if (fs.eof()) {
	  last_record="";
	  break;
	}
	last_record=buf;
    }
  }
  buffer[--bytes_read]=0;
  if (!last_record.empty()) {
    last_ID=last_record.substr(2,11);
  }
  else {
    last_ID="";
  }
  ++num_read;
  return bytes_read;
}

size_t ArgentinaRaob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

int compareArgentinaPressures(ArgentinaRaob::SortEntry& left,ArgentinaRaob::SortEntry& right)
{
  if (left.level.pressure >= right.level.pressure)
    return -1;
  else
    return 1;
}
void ArgentinaRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *buffer=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  std::string check;
  std::deque<std::string> sp;
  size_t n;
  int m,cnt=0;
  short yr,mo,dy,hr;
  size_t off,ind,lastInd=0;
  bool unsorted=false;
//  ArgentinaRaob::SortEntry *array;
std::vector<ArgentinaRaob::SortEntry> array;

  sp=strutils::split(buffer,"\n");
/*
  if (raob.capacity < (sp.size()*3) ) {
    if (levels != NULL)
	delete[] levels;
    if (rh != NULL)
	delete[] rh;
    raob.capacity=sp.size()*3;
    levels=new RaobLevel[raob.capacity];
    rh=new int[raob.capacity];
  }
*/
if (levels.capacity() < (sp.size()*3)) {
levels.resize(sp.size()*3);
rh.resize(sp.size()*3);
}
  location_.latitude=-99.9;
  location_.longitude=-199.9;
  location_.elevation=-999;
  nlev=sp.size()*3;
  for (n=0; n < sp.size(); ++n) {
    if (n == 0) {
	location_.ID=sp[n].substr(2,3);
	yr=std::stoi(sp[n].substr(5,2));
	mo=std::stoi(sp[n].substr(7,2));
	dy=std::stoi(sp[n].substr(9,2));
	hr=std::stoi(sp[n].substr(11,2));
	date_time_.set(1900+yr,mo,dy,hr*10000);
    }
    if (!fill_header_only) {
	off=13;
	ind=std::stoi(sp[n].substr(1,1));
	if (ind < lastInd) {
	  std::cerr << "Error: records out of order" << std::endl;
	  exit(1);
	}
	for (m=0; m < 3; ++m) {
	  check=sp[n].substr(off,20);
	  if (check != "-                   " && check != "                    ") {
	    switch (ind) {
		case 1:
		{
		  if (m == 0) {
		    levels[cnt].pressure=std::stof(sp[n].substr(74,4))/10.;
		    if (sp[n][73] == '1' || sp[n][73] == 'X') {
			levels[cnt].pressure+=1000.;
		    }
		    levels[cnt].type=0;
		  }
		  else {
		    levels[cnt].pressure= (m == 1) ? 1000. : 900.;
		    levels[cnt].type=1;
		  }
		  break;
		}
		case 2:
		{
		  if (m == 0) {
		    levels[cnt].pressure=850.;
		  }
		  else if (m == 1) {
		    levels[cnt].pressure=800.;
		  }
		  else {
		    levels[cnt].pressure=700.;
		  }
		  levels[cnt].type=1;
		  break;
		}
		case 3:
		{
		  levels[cnt].pressure=600.-(m*100.);
		  levels[cnt].type=1;
		  break;
		}
		case 4:
		{
		  levels[cnt].pressure=300.-(m*50.);
		  levels[cnt].type=1;
		  break;
		}
		case 5:
		{
		  if (m == 0) {
		    levels[cnt].pressure=150.;
		  }
		  else if (m == 1) {
		    levels[cnt].pressure=100.;
		  }
		  else {
		    levels[cnt].pressure=70.;
		  }
		  levels[cnt].type=1;
		  break;
		}
		case 6:
		{
		  levels[cnt].pressure=50.-(m*10.);
		  levels[cnt].type=1;
		  break;
		}
		case 7:
		{
		  levels[cnt].pressure=20.-(m*5.);
		  levels[cnt].type=1;
		  break;
		}
		case 8:
		{
		  levels[cnt].pressure= (m == 0) ? 5. : 2.;
		  levels[cnt].type=1;
		  break;
		}
		default:
		{
		  std::cerr << "Error: unrecognized indicator " << ind << std::endl;
		  exit(1);
		}
	    }
	    if (sp[n].substr(off,5) == "0-   ") {
		levels[cnt].height=-99999;
	    }
	    else {
		levels[cnt].height=std::stoi(sp[n].substr(off,4))*10;
		if (sp[n][off+4] >= '0' && sp[n][off+4] <= '9')
		  levels[cnt].height+=std::stoi(sp[n].substr(off+4,1));
		else {
		  if (sp[n][off+4] >= 'J' && sp[n][off+4] <= 'R')
		    levels[cnt].height+=static_cast<int>(sp[n][off+4])-73;
		  levels[cnt].height=-levels[cnt].height;
		}
	    }
	    if (sp[n].substr(off+5,4) == "0-  ")
		levels[cnt].temperature=-99.9;
	    else
		levels[cnt].temperature=std::stof(sp[n].substr(off+5,4))/10.;
	    if (sp[n].substr(off+9,2) == "0-")
		rh[cnt]=-99;
	    else
		rh[cnt]=std::stoi(sp[n].substr(off+9,2));
	    if (sp[n].substr(off+11,4) == "0-  ")
		levels[cnt].moisture=-99.9;
	    else
		levels[cnt].moisture=std::stof(sp[n].substr(off+11,4))/10.;
	    if (sp[n].substr(off+15,3) == "0- ")
		levels[cnt].wind_direction=500;
	    else
		levels[cnt].wind_direction=std::stoi(sp[n].substr(off+15,3));
	    if (sp[n].substr(off+18,2) == "0-")
		levels[cnt].wind_speed=250.;
	    else {
		if (sp[n][off+18] >= '0' && sp[n][off+18] <= '9')
		  levels[cnt].wind_speed=std::stof(sp[n].substr(off+18,2));
		else {
		  if (sp[n][off+18] >= 'A' && sp[n][off+18] <= 'H')
		    levels[cnt].wind_speed=(static_cast<int>(sp[n][off+18])-55)*10.;
		  else if (sp[n][off+18] >= 'J' && sp[n][off+18] <= 'N')
		    levels[cnt].wind_speed=(static_cast<int>(sp[n][off+18])-56)*10.;
		  else if (sp[n][off+18] >= 'P' && sp[n][off+18] <= 'Z')
		    levels[cnt].wind_speed=(static_cast<int>(sp[n][off+18])-57)*10.;
		  levels[cnt].wind_speed+=std::stoi(sp[n].substr(off+19,1));
		}
	    }
	    if (cnt > 0 && levels[cnt].pressure > levels[cnt-1].pressure)
		unsorted=true;
	    cnt++;
	  }
	  off+=20;
	}
	nlev=cnt;
	lastInd=ind;
    }
  }

  if (unsorted) {
//    array=new SortEntry[nlev];
array.resize(nlev);
    for (n=0; n < static_cast<size_t>(nlev); n++) {
	array[n].level=levels[n];
	array[n].rh=rh[n];
    }
//    binary_sort(array,nlev,
binary_sort(array,
    [](SortEntry& left,SortEntry& right) -> bool
    {
	if (left.level.pressure >= right.level.pressure) {
	  return true;
	}
	else {
	  return false;
	}
//    });
},0,nlev);
    for (n=0; n < static_cast<size_t>(nlev); ++n) {
	levels[n]=array[n].level;
	rh[n]=array[n].rh;
    }
//    delete[] array;
  }
}

void ArgentinaRaob::print(std::ostream& outs) const
{
  int n;

  print_header(outs,true);
  outs.setf(std::ios::fixed);
  outs.precision(1);
  outs << "  LEVEL   PRES    HGT  TEMP  RH  DEWPT  DD    FF" << std::endl;
  for (n=0; n < nlev; n++)
    outs << std::setw(7) << n+1 << std::setw(7) << levels[n].pressure << std::setw(7) << levels[n].height << std::setw(6) << levels[n].temperature << std::setw(4) << rh[n] << std::setw(7) << levels[n].moisture << std::setw(4) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << std::endl;
}

void ArgentinaRaob::print_header(std::ostream& outs,bool verbose) const
{
  if (verbose)
    outs << "Station: " << location_.ID << "  Date: " << date_time_.to_string() << "Z  Number of Levels: " << std::setw(2) << nlev << std::endl;
  else
    outs << "Stn=" << location_.ID << " Date=" << date_time_.to_string("%Y%m%d%HH") << " NLev=" << nlev << std::endl;
}
