#include <iomanip>
#include <raob.hpp>
#include <bits.hpp>

const size_t KuniaRaob::mand_pres[7]={1000,850,700,500,400,300,200};
const size_t KuniaRaob::std_z[7]={111,1457,3012,5574,7185,9164,11784};

int InputKuniaRaobStream::ignore()
{
return 0;
}

bool InputKuniaRaobStream::open(const char *filename)
{
  if (is_open()) {
    std::cerr << "Error: currently connected to another file stream" << std::endl;
    exit(1);
  }
  num_read=0;
  ics.reset(new icstream);
  if (!ics->open(filename)) {
    ics.reset(nullptr);
    return false;
  }
  offset=0;
  return true;
}

int InputKuniaRaobStream::peek()
{
  if (!is_open()) {
    std::cerr << "Error: no InputKuniaRaobStream has been opened" << std::endl;
    exit(1);
  }
return bfstream::error;
}

int InputKuniaRaobStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;
  int block_len,report_type=0;

  if (!is_open()) {
    std::cerr << "Error: no InputKuniaRaobStream has been opened" << std::endl;
    exit(1);
  }
  if (ics != NULL) {
    if (offset == 0) {
	while (report_type != 2) {
	  block_len=ics->read(buf,3000);
	  if (block_len == craystream::eod)
	    return bfstream::eof;
	  else if (block_len == bfstream::eof || block_len == 200)
	    block_len=ics->read(buf,3000);
	  bits::get(buf,report_type,21,5);
	}
	offset=6;
    }
    if (buffer_length < 6) {
	std::copy(buf,buf+buffer_length,buffer);
	bytes_read=buffer_length;
    }
    else {
	std::copy(buf,buf+6,buffer);
	if (buffer_length < 54) {
	  std::copy(&buf[offset],&buf[offset-buffer_length+6],&buffer[6]);
	  bytes_read=buffer_length;
	}
	else {
	  std::copy(&buf[offset],&buf[offset+48],&buffer[6]);
	  bytes_read=48;
	}
    }
    offset+=48;
    offset%=2982;
  }
  else
    bytes_read=bfstream::error;
  num_read++;
  return bytes_read;
}

size_t KuniaRaob::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void KuniaRaob::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  short yr,mo,dy,hr;
  int dum;
  size_t n,off,num_missing;

  bits::get(stream_buffer,yr,0,7);
  bits::get(stream_buffer,mo,7,4);
  bits::get(stream_buffer,dy,11,5);
  bits::get(stream_buffer,hr,16,5);
  date_time_.set(1900+yr,mo,dy,hr*10000);
  bits::get(stream_buffer,dum,49,11);
  if (dum >= 0x400)
    dum-=0x7ff;
  location_.latitude=dum/10.;
  bits::get(stream_buffer,dum,60,12);
  if (dum >= 0x800)
    dum-=0xfff;
  location_.longitude=dum/10.;
  bits::get(stream_buffer,location_.elevation,74,8);
  nlev=0;
  if (!fill_header_only) {
    off=96;
// unpack the 7 mandatory levels
    for (n=0; n < 7; n++) {
	num_missing=0;
	levels[nlev].pressure=mand_pres[n];
	bits::get(stream_buffer,levels[nlev].height,off,13);
	if (levels[nlev].height == 0xfff || levels[nlev].height == 0x1fff) {
	  levels[nlev].height=99999;
	  num_missing++;
	}
	else {
	  if (levels[nlev].height >= 0x1000)
	    levels[nlev].height=levels[nlev].height-0x1fff;
	  levels[nlev].height+=std_z[n];
	}
	bits::get(stream_buffer,dum,off+13,9);
	if (dum == 0xff || dum == 0x1ff) {
	  levels[nlev].temperature=-99.9;
	  num_missing++;
	}
	else {
	  if (dum >= 0x100)
	    dum-=0x1ff;
	  levels[nlev].temperature=dum/2.;
	}
	bits::get(stream_buffer,dum,off+22,6);
	kunia.mois_flag[nlev]=0;
	if (dum == 0x1f || dum == 0x3f) {
	  levels[nlev].moisture=-99.9;
	  kunia.mois_flag[nlev]=9;
	  num_missing++;
	}
	else {
	  if (dum >= 0x20) {
	    dum=0x3f-dum;
	    kunia.mois_flag[nlev]=1;
	  }
	  levels[nlev].moisture=levels[nlev].temperature-dum;
	}
	bits::get(stream_buffer,dum,off+28,10);
	if (dum == 0x1ff || dum == 0x3ff) {
	  levels[nlev].ucomp=-999.9;
	  ++num_missing;
	}
	else {
	  if (dum >= 0x200) {
	    dum-=0x3ff;
	  }
	  levels[nlev].ucomp=dum;
	}
	bits::get(stream_buffer,dum,off+38,10);
	if (dum == 0x1ff || dum == 0x3ff) {
	  levels[nlev].vcomp=-999.9;
	  ++num_missing;
	}
	else {
	  if (dum >= 0x200) {
	    dum-=0x3ff;
	  }
	  levels[nlev].vcomp=dum;
	}
	if (num_missing < 5) {
	  ++nlev;
	}
	off+=48;
    }
  }
}

void KuniaRaob::print(std::ostream& outs) const
{
  print_header(outs,true);
  outs << "\n   LEV    PRES   HEIGHT   TEMP   DEWPT F   UCOMP   VCOMP" << std::endl;
  for (int n=0; n < nlev; ++n) {
    outs << std::setw(6) << n+1 << std::setw(8) << levels[n].pressure << std::setw(9) << levels[n].height << std::setw(7) << levels[n].temperature << std::setw(8) << levels[n].moisture << std::setw(2) << kunia.mois_flag[n] << std::setw(8) << levels[n].ucomp << std::setw(8) << levels[n].vcomp << std::endl;
  }
}

void KuniaRaob::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);
  if (verbose) {
    outs << "Date: " << date_time_.to_string() << "Z  Location: " << std::setw(6) << location_.latitude << std::setw(8) << location_.longitude << std::setw(6) << location_.elevation << "  Number of Levels: " << std::setw(3) << nlev << std::endl;
  }
  else {
  }
}
