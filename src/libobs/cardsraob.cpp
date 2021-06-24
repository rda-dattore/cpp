#include <iomanip>
#include <bfstream.hpp>
#include <raob.hpp>
#include <strutils.hpp>

void InputCardsRaobStream::close()
{
  idstream::close();
  at_eof=false;
}

int InputCardsRaobStream::ignore()
{
return bfstream::error;
}

bool InputCardsRaobStream::open(const char *filename)
{
  if (!idstream::open(filename))
    return false;

  if (ics != NULL) {
    if (ics->read(first_card,80) == bfstream::eof)
	return false;
  }
  else {
  }

  at_eof=false;
  return true;
}

int InputCardsRaobStream::peek()
{
return bfstream::error;
}

int InputCardsRaobStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (!is_open()) {
    std::cerr << "Error: no InputCardsRaobStream has been opened" << std::endl;
    exit(1);
  }
  if (!at_eof) {
    std::copy(first_card,first_card+80,buffer);
  }
  else {
    return bfstream::eof;
  }
  if (ics != NULL) {
    size_t card_num=0,off;
    do {
	++card_num;
	off=card_num*80;
	if (off+80 > buffer_length) {
	  std::cerr << "Error: buffer overflow" << std::endl;
	  exit(1);
	}
	if (ics->read(&buffer[off],80) == bfstream::eof)
	  at_eof=true;
    } while (!at_eof && std::string(reinterpret_cast<char *>(first_card),13) == std::string(reinterpret_cast<char *>(&buffer[off]),13));
    off=card_num*80;
    if (!at_eof) {
	std::copy(&buffer[off],&buffer[off+80],reinterpret_cast<char *>(first_card));
    }
    buffer[off]='X';
    ++num_read;
    return off;
  }
  else {
return bfstream::error;
  }
}

size_t CardsRaob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void CardsRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *buffer=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  int dum;
  short yr,mo,dy,hr;
  size_t n,m,l,off,card_num;
  short mpres[8][4]={{1013,1000,950,900},
                     { 850, 800,750,700},
                     { 650, 600,550,500},
                     { 450, 400,350,300},
                     { 250, 200,175,150},
                     { 125, 100, 80, 60},
                     {  50,  40, 30, 20},
                     {  10,   0,  0,  0}};
  char header[13];
  bool bad_field;

  if (levels.size() == 0) {
    levels.resize(29);
  }
  location_.ID.assign(&buffer[2],3);
  location_.ID="72"+location_.ID;
// change Merida, Mexico to a number in our libraries
  if (location_.ID == "72106") {
    location_.ID="76644";
  }
  location_.latitude=-99.9;
  location_.longitude=-199.9;
  location_.elevation=-999;

/* ship location coding (ignore for now)
  strutils::strget(buffer+1,lat,2);
  strutils::strget(buffer+3,lon,2);
  switch (buffer[1]) {
    case 0:
	loc.latitude=lat;
	loc.longitude=-lon;
	break;
    case 1:
	loc.latitude=lat;
	if (lon <= 99)
	  loc.longitude=-lon;
	else
	  loc.longitude=-(100+lon);
	break;
    case 2:
	loc.latitude=lat;
	if (lon <= 99)
	  loc.longitude=lon;
	else
	  loc.longitude=100+lon;
	break;
    case 3:
	loc.latitude=lat;
	loc.longitude=lon;
	break;
    case 5:
	loc.latitude=-lat;
	loc.longitude=-lon;
	break;
    case 6:
	loc.latitude=-lat;
	if (lon <= 99)
	  loc.longitude=-lon;
	else
	  loc.longitude=-(100+lon);
	break;
    case 7:
	loc.latitude=-lat;
	if (lon <= 99)
	  loc.longitude=lon;
	else
	  loc.longitude=100+lon;
	break;
    case 8:
	loc.latitude=-lat;
	loc.longitude=lon;
	break;
  }
*/

  strutils::strget(buffer+5,yr,2);
  strutils::strget(buffer+7,mo,2);
  strutils::strget(buffer+9,dy,2);
  strutils::strget(buffer+11,hr,2);
  date_time_.set(1900+yr,mo,dy,hr*10000);
  nlev=0;
  if (!fill_header_only) {
    std::copy(buffer,buffer+13,header);
    n=0;
    do {
	off=n*80+14;
	if (buffer[off-1] >= '1' && buffer[off-1] <= '8') {
	  card_num=buffer[off-1]-49;
	  for (m=0; m < 4; ++m) {
	    levels[nlev].pressure=mpres[card_num][m];
	    if (std::string(buffer+off,4) == "    ") {
		if (card_num == 0 && m == 0) {
		  levels[nlev].pressure=-9999.;
		}
		levels[nlev].height=-9999;
	    }
	    else {
		bad_field=false;
		for (l=off; l < off+4; l++) {
		  if (buffer[l] < '0' || buffer[l] > '9') {
		    bad_field=true;
		    l=off+4;
		  }
		}
		if (bad_field) {
		  levels[nlev].height=-9999;
		  std::cerr << "Warning: bad height field on level " << nlev+1 << " - header and field follow:" << std::endl;
		  std::cerr.write(header,13);
		  std::cerr << " ";
		  std::cerr.write(buffer+off,4);
		  std::cerr << std::endl;
		}
		else {
		  strutils::strget(buffer+off,levels[nlev].height,4);
		  if (card_num == 0 && m == 0) {
		    levels[nlev].pressure=levels[nlev].height;
		    levels[nlev].height=-9999;
		  }
		  else {
		    if (levels[nlev].pressure < 300. && levels[nlev].pressure > 50.)
			levels[nlev].height+=10000;
		    else if (levels[nlev].pressure < 51. && levels[nlev].pressure > 49.) {
			if (levels[nlev].height < 8000)
			  levels[nlev].height+=20000;
			else
			  levels[nlev].height+=10000;
		    }
		    else if (levels[nlev].pressure < 50.)
			levels[nlev].height+=20000;
		  }
		}
	    }
	    if (std::string(buffer+off+4,4) == "    ") {
		levels[nlev].temperature=-99.9;
	    }
	    else {
		strutils::strget(buffer+off+5,dum,3);
		if (buffer[off+4] == '0')
		  levels[nlev].temperature=dum/10.;
		else if (buffer[off+4] == 'X' || buffer[off+4] == '-')
		  levels[nlev].temperature=-dum/10.;
		else {
		  levels[nlev].temperature=-99.9;
		  std::cerr << "Warning: bad temperature field on level " << nlev+1 << " - header and field follow:" << std::endl;
		  std::cerr.write(header,13);
		  std::cerr << " ";
		  std::cerr.write(buffer+off+4,4);
		  std::cerr << std::endl;
		}
	    }
	    if (std::string(buffer+off+8,2) == "  ") {
		levels[nlev].moisture=-99.0;
	    }
	    else {
		levels[nlev].moisture=(buffer[off+8]-48)*10;
		if ((buffer[off+9] >= 'J' && buffer[off+9] <= 'R') || buffer[off+9] == '}')
		  levels[nlev].moisture+=buffer[off+9]-'I';
		else
		  levels[nlev].moisture+=buffer[off+9]-48;
	    }
	    if (std::string(buffer+off+10,2) == "  ") {
		levels[nlev].wind_direction=999;
	    }
	    else {
		strutils::strget(buffer+off+10,levels[nlev].wind_direction,2);
		levels[nlev].wind_direction*=10;
	    }
	    if (std::string(buffer+off+12,3) == "   ") {
		levels[nlev].wind_speed=999.;
	    }
	    else {
		strutils::strget(buffer+off+12,levels[nlev].wind_speed,3);
	    }
	    if (card_num == 0 && m == 0) {
		levels[nlev].type=0;
	    }
	    else {
		levels[nlev].type=1;
	    }
	    off+=15;

// eliminate missing levels
	    if ((levels[nlev].type == 0 && levels[nlev].pressure > -9999.) || (levels[nlev].type == 1 && (levels[nlev].height != -9999 || levels[nlev].temperature > -99. || levels[nlev].moisture > -99. || levels[nlev].wind_direction != 999 || levels[nlev].wind_speed < 999.)))
		++nlev;
	  }
	}
	++n;
    } while (std::string(header,13) == std::string(&buffer[n*80],13));
  }
}

void CardsRaob::print(std::ostream& outs) const
{
  int n;

  print_header(outs,true);
  if (nlev > 0) {
    outs << "\n  LEV   PRES   HEIGHT   TEMP    RH   DD    FF  TYPE" << std::endl;
    for (n=0; n < nlev; ++n) {
	outs << std::setw(5) << n+1 << std::setw(7) << levels[n].pressure << std::setw(9) << levels[n].height << std::setw(7) << levels[n].temperature << std::setw(6) << levels[n].moisture << std::setw(5) << levels[n].wind_direction << std::setw(6) << levels[n].wind_speed << std::setw(6) << levels[n].type << std::endl;
    }
  }
}

void CardsRaob::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (verbose) {
    outs << "Station: " << location_.ID << "  Date: " << date_time_.to_string() << "  Location: " << std::setw(5) << location_.latitude << std::setw(7) << location_.longitude << std::setw(5) << location_.elevation << std::endl;
  }
  else {
  }
}
