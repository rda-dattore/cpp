#include <iomanip>
#include <string>
#include <list>
#include <grid.hpp>
#include <gridutils.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>
#include <xml.hpp>
#ifdef __JASPER
#include <jasper/jasper.h>
#endif
#include <myerror.hpp>
#include <tempfile.hpp>

int InputGRIBStream::peek()
{
  unsigned char buffer[16];
  int len=-9;
  while (len == -9) {
    len=find_grib(buffer);
    if (len < 0) {
	return len;
    }
    fs.read(reinterpret_cast<char *>(&buffer[4]),12);
    len+=fs.gcount();
    if (len != 16) {
	return bfstream::error;
    }
    switch (static_cast<int>(buffer[7])) {
	case 0:
	{
	  len=4;
	  fs.seekg(curr_offset+4,std::ios_base::beg);
	  fs.read(reinterpret_cast<char *>(buffer),3);
	  if (fs.eof()) {
	    return bfstream::eof;
	  }
	  else if (!fs.good()) {
	    return bfstream::error;
	  }
	  while (buffer[0] != '7' && buffer[1] != '7' && buffer[2] != '7') {
	    int llen;
	    bits::get(buffer,llen,0,24);
	    if (llen <= 0) {
		return bfstream::error;
	    }
	    len+=llen;
	    fs.seekg(llen-3,std::ios_base::cur);
	    fs.read(reinterpret_cast<char *>(buffer),3);
	    if (fs.eof()) {
		return bfstream::eof;
	    }
	    else if (!fs.good()) {
		return bfstream::error;
	    }
	  }
	  len+=4;
	  break;
	}
	case 1:
	{
	  bits::get(buffer,len,32,24);
	  if (len >= 0x800000) {
// check for ECMWF large-file
	    int computed_len=0,sec_len,flag;
// PDS length
	    bits::get(buffer,sec_len,64,24);
	    bits::get(buffer,flag,120,8);
	    computed_len=8+sec_len;
	    fs.seekg(curr_offset+computed_len,std::ios_base::beg);
	    if ( (flag & 0x80) == 0x80) {
// GDS included
		fs.read(reinterpret_cast<char *>(buffer),3);
		bits::get(buffer,sec_len,0,24);
		fs.seekg(sec_len-3,std::ios_base::cur);
		computed_len+=sec_len;
	    }
	    if ( (flag & 0x40) == 0x40) {
// BMS included
		fs.read(reinterpret_cast<char *>(buffer),3);
		bits::get(buffer,sec_len,0,24);
		fs.seekg(sec_len-3,std::ios_base::cur);
		computed_len+=sec_len;
	    }
	    fs.read(reinterpret_cast<char *>(buffer),3);
// BDS length
	    bits::get(buffer,sec_len,0,24);
	    if (sec_len < 120) {
		len&=0x7fffff;
		len*=120;
		len-=sec_len;
		fs.seekg(curr_offset+len,std::ios_base::beg);
		len+=4;
	    }
	    else {
		fs.seekg(sec_len-3,std::ios_base::cur);
	    }
	  }
	  else {
	    fs.seekg(len-20,std::ios_base::cur);
	  }
	  fs.read(reinterpret_cast<char *>(buffer),4);
	  if (std::string(reinterpret_cast<char *>(buffer),4) != "7777") {
	    len=-9;
	    fs.seekg(curr_offset+4,std::ios_base::beg);
	  }
	  break;
	}
	case 2:
	{
	  bits::get(buffer,len,96,32);
	  break;
	}
	default:
	{
	  return peek();
	}
    }
  }
  fs.seekg(curr_offset,std::ios_base::beg);
  return len;
}

int InputGRIBStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read=0;
  int len=-9;
  while (len == -9) {
    bytes_read=find_grib(buffer);
    if (bytes_read < 0) {
	return bytes_read;
    }
    fs.read(reinterpret_cast<char *>(&buffer[4]),12);
    bytes_read+=fs.gcount();
    if (bytes_read != 16) {
	if (fs.eof()) {
	  return bfstream::eof;
	}
	else {
	  return bfstream::error;
	}
    }
    switch (static_cast<int>(buffer[7])) {
	case 0:
	{
	  fs.read(reinterpret_cast<char *>(&buffer[16]),15);
	  bytes_read+=fs.gcount();
	  if (bytes_read != 31) {
	    return bfstream::eof;
	  }
	  len=28;
	  while (buffer[bytes_read-3] != '7' && buffer[bytes_read-2] != '7' && buffer[bytes_read-1] != '7') {
	    int llen;
	    bits::get(buffer,llen,len*8,24);
	    if (llen <= 0) {
		return bfstream::error;
	    }
	    len+=llen;
	    if (len > static_cast<int>(buffer_length)) {
		myerror="GRIB0 length "+strutils::itos(len)+" overflows buffer length "+strutils::itos(buffer_length)+" on record "+strutils::itos(num_read+1);
		exit(1);
	    }
	    fs.read(reinterpret_cast<char *>(&buffer[bytes_read]),llen);
	    bytes_read+=fs.gcount();
	    if (bytes_read != len+3) {
		if (fs.eof()) {
		  return bfstream::eof;
		}
		else {
		  return bfstream::error;
		}
	    }
	  }
	  fs.read(reinterpret_cast<char *>(&buffer[bytes_read]),1);
	  bytes_read+=fs.gcount();
	  break;
	}
	case 1:
	{
	  bits::get(buffer,len,32,24);
	  if (len > static_cast<int>(buffer_length)) {
	    myerror="GRIB1 length "+strutils::itos(len)+" overflows buffer length "+strutils::itos(buffer_length)+" on record "+strutils::itos(num_read+1);
	    exit(1);
	  }
	  fs.read(reinterpret_cast<char *>(&buffer[16]),len-16);
	  bytes_read+=fs.gcount();
	  if (bytes_read != len) {
	    if (fs.eof()) {
		return bfstream::eof;
	    }
	    else {
		return bfstream::error;
	    }
	  }
	  if (len >= 0x800000) {
// check for ECMWF large-file
	    int computed_len=0,sec_len,flag;
// PDS length
	    bits::get(buffer,sec_len,64,24);
	    bits::get(buffer,flag,120,8);
	    computed_len=8+sec_len;
	    if ( (flag & 0x80) == 0x80) {
// GDS included
		bits::get(buffer,sec_len,computed_len*8,24);
		computed_len+=sec_len;
	    }
	    if ( (flag & 0x40) == 0x40) {
// BMS included
		bits::get(buffer,sec_len,computed_len*8,24);
		computed_len+=sec_len;
	    }
// BDS length
	    bits::get(buffer,sec_len,computed_len*8,24);
	    if (sec_len < 120) {
		len&=0x7fffff;
		len*=120;
		len-=sec_len;
		len+=4;
		if (len > static_cast<int>(buffer_length)) {
		  myerror="GRIB1 length "+strutils::itos(len)+" overflows buffer length "+strutils::itos(buffer_length)+" on record "+strutils::itos(num_read+1);
		  exit(1);
		}
		fs.read(reinterpret_cast<char *>(&buffer[bytes_read]),len-bytes_read);
		bytes_read+=fs.gcount();
	    }
	  }
	  if (std::string(reinterpret_cast<char *>(&buffer[len-4]),4) != "7777") {
/*
	    if (len > 0x800000) {
// check for ECMWF large-file
		int computed_len=0,sec_len,flag;
// PDS length
		bits::get(buffer,sec_len,64,24);
		bits::get(buffer,flag,120,8);
		computed_len=8+sec_len;
		if ( (flag & 0x80) == 0x80) {
// GDS included
		  bits::get(buffer,sec_len,computed_len*8,24);
		  computed_len+=sec_len;
		}
		if ( (flag & 0x40) == 0x40) {
// BMS included
		  bits::get(buffer,sec_len,computed_len*8,24);
		  computed_len+=sec_len;
		}
// BDS length
		bits::get(buffer,sec_len,computed_len*8,24);
		len&=0x7fffff;
		len*=120;
		len-=sec_len;
		len+=4;
		if (len > static_cast<int>(buffer_length)) {
		  myerror="GRIB1 length "+strutils::itos(len)+" overflows buffer length "+strutils::itos(buffer_length)+" on record "+strutils::itos(num_read+1);
		  exit(1);
		}
		bytesRead+=fread(&buffer[bytesRead],1,len-bytesRead,fp);
		if (std::string(reinterpret_cast<char *>(&buffer[len-4]),4) != "7777") {
		  len=-9;
		  fseeko(fp,curr_offset+4,SEEK_SET);
		}
	    }
	    else {
*/
		len=-9;
		fs.seekg(curr_offset+4,std::ios_base::beg);
//	    }
	  }
	  break;
	}
	case 2:
	{
	  bits::get(buffer,len,96,32);
	  if (len > static_cast<int>(buffer_length)) {
	    myerror="GRIB2 length "+strutils::itos(len)+" overflows buffer length "+strutils::itos(buffer_length)+" on record "+strutils::itos(num_read+1);
	    exit(1);
	  }
	  fs.read(reinterpret_cast<char *>(&buffer[16]),len-16);
	  bytes_read+=fs.gcount();
	  if (bytes_read != len) {
	    if (fs.eof()) {
		return bfstream::eof;
	    }
	    else {
		return bfstream::error;
	    }
	  }
	  if (std::string(reinterpret_cast<char *>(buffer)+len-4,4) != "7777") {
/*
	    len=-9;
	    fs.seekg(curr_offset+4,std::ios_base::beg);
*/
myerror="missing END section";
exit(1);
	  }
	  break;
	}
	default:
	{
	  return read(buffer,buffer_length);
	}
    }
  }
  ++num_read;
  return bytes_read;
}

bool InputGRIBStream::open(std::string filename)
{
  icstream chkstream;

  if (is_open()) {
    myerror="currently connected to another file stream";
    exit(1);
  } 
  num_read=0;
// test for COS-blocking
  if (!chkstream.open(filename))
    return false;
  if (chkstream.ignore() > 0) {
    myerror="COS-blocking must be removed from GRIB files before reading";
    exit(1);
  }
  fs.open(filename.c_str(),std::ios::in);
  if (!fs.is_open()) {
    return false;
  }
  file_name=filename;
  return true;
}

int InputGRIBStream::find_grib(unsigned char *buffer)
{
  buffer[0]='X';
  fs.read(reinterpret_cast<char *>(buffer),4);
  while (buffer[0] != 'G' || buffer[1] != 'R' || buffer[2] != 'I' || buffer[3] != 'B') {
    if (fs.eof() || !fs.good()) {
	if (num_read == 0) {
	  myerror="not a GRIB file";
	  exit(1);
	}
	else {
	  return (fs.eof()) ? bfstream::eof : bfstream::error;
	}
    }
    buffer[0]=buffer[1];
    buffer[1]=buffer[2];
    buffer[2]=buffer[3];
    fs.read(reinterpret_cast<char *>(&buffer[3]),1);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (!fs.good()) {
	return bfstream::error;
    }
  }
  curr_offset=static_cast<off_t>(fs.tellg())-4;
  return 4;
}

#ifndef __cosutils
const char *GRIBGrid::level_type_short_name[]={
"0Reserved","SFC" ,"CBL" ,"CTL" ,"0DEG","ADCL","MWSL","TRO" ,"NTAT","SEAB",""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,"TMPL",""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,
""    ,"ISBL","ISBY","MSL" ,"GPML","GPMY","HTGL","HTGY","SIGL","SIGY","HYBL",
"HYBY","DBLL","DBLY","THEL","THEY","SPDL","SPDY","PVL" ,""    ,"ETAL","ETAY",
"IBYH",""    ,""    ,""    ,"HGLH",""    ,""    ,"SGYH",""    ,""    ,""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,"IBYM",""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,
""    ,""    ,""    ,""    ,""    ,""    ,"DBSL",""    ,""    ,""    ,""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,
""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,""    ,
""    ,""    ,"EATM","EOCN"};
const char *GRIBGrid::level_type_units[]={"","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","mbars","mbars","","m above MSL","hm above MSL",
"m AGL","hm AGL","sigma","sigma","","","cm below surface","cm below surface",
"degK","degK","mbars","mbars","10^-6Km^2/kgs","","","","mbars","","","",
"cm AGL","","","sigma","","","","","","","","","","","","","mbars","","","","",
"","","","","","","","","","","","","","","m below MSL","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","sigma",""};
const char *GRIBGrid::time_units[]={"Minutes","Hours","Days","Months","Years",
"Decades","Normals","Centuries","","","3Hours","6Hours","12Hours",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","","","","","","","","","","","",
"","Seconds","Missing"};
const short GRIBGrid::octagonal_grid_parameter_map[]={ 0, 7,35, 0, 7,39, 1, 0, 0,
 0,11,12, 0, 0, 0, 0, 0, 0, 0,18,15,16, 0, 0, 0, 0, 0, 0, 2, 0,33,34, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0,52, 0, 0,80, 0, 0, 0, 0, 0, 0, 0, 0, 0,80, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0,66};
const short GRIBGrid::navy_grid_parameter_map[]={ 0, 7, 0, 0, 0, 0, 2, 0, 0, 0,11,
  0,18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,33,34, 0,31,32, 0, 0,
  0, 0, 0,40, 0, 0, 0, 0,55, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11,11,11};
const short GRIBGrid::on84_grid_parameter_map[]={  0,  7,  0,  0,  0,  0,  0,  0,
    1,  0,  0,  0,  0,  0,  0,  0, 11, 17, 18, 13, 15, 16,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 39,  0,  0,  0,  0,  0,
    0,  0, 33, 34,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0, 41,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0, 52, 54, 61,  0,  0, 66,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,210,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

GRIBMessage::~GRIBMessage()
{
  clear_grids();
}

GRIBMessage& GRIBMessage::operator=(const GRIBMessage& source)
{
  edition_=source.edition_;
  discipline=source.discipline;
  mlength=source.mlength;
  curr_off=source.curr_off;
  lengths_=source.lengths_;
  pds_supp.reset(new unsigned char[lengths_.pds_supp]);
    for (int n=0; n < lengths_.pds_supp; ++n) {
	pds_supp[n]=source.pds_supp[n];
    }
  gds_included=source.gds_included;
  bms_included=source.bms_included;
  grids=source.grids;
  return *this;
}

void GRIBMessage::append_grid(const Grid *grid)
{
  Grid *g=nullptr;
  switch (edition_) {
    case 0:
    case 1:
    {
	g=new GRIBGrid;
	*(reinterpret_cast<GRIBGrid *>(g))=*(reinterpret_cast<GRIBGrid *>(const_cast<Grid *>(grid)));
	if (reinterpret_cast<GRIBGrid *>(g)->bitmap.applies) {
	  bms_included=true;
	}
	break;
    }
    case 2:
    {
	g=new GRIB2Grid;
	*(reinterpret_cast<GRIB2Grid *>(g))=*(reinterpret_cast<GRIB2Grid *>(const_cast<Grid *>(grid)));
	break;
     }
  }
  grids.emplace_back(g);
}

void GRIBMessage::convert_to_grib1()
{
  if (grids.size() > 1) {
    myerror="unable to convert multiple grids to GRIB1";
    exit(1);
  }
  if (grids.size() == 0) {
    myerror="no grid found";
    exit(1);
  }
  auto g=reinterpret_cast<GRIBGrid *>(grids.front());
  switch (edition_) {
    case 0:
    {
	lengths_.is=8;
	lengths_.pds=28;
	edition_=1;
	g->grib.table=3;
	if (g->reference_date_time_.year() == 0 || g->reference_date_time_.year() > 30) {
	  g->grib.century=20;
	  g->reference_date_time_.set_year(g->reference_date_time_.year()+1900);
	}
	else {
	  g->grib.century=21;
	  g->reference_date_time_.set_year(g->reference_date_time_.year()+2000);
	}
	g->grib.sub_center=0;
	g->grib.D=0;
	g->remap_parameters();
	break;
    }
  }
}

void GRIBMessage::pack_length(unsigned char *output_buffer) const
{
  switch (edition_) {
    case 1:
    {
	bits::set(output_buffer,mlength,32,24);
	break;
    }
    case 2:
    {
	bits::set(output_buffer,mlength,64,64);
	break;
    }
  }
}

void GRIBMessage::pack_is(unsigned char *output_buffer,off_t& offset,Grid *g) const
{
  bits::set(output_buffer,0x47524942,0,32);
  switch (edition_) {
    case 1:
    {
	bits::set(output_buffer,edition_,56,8);
	offset=8;
	break;
    }
    case 2:
    {
	bits::set(output_buffer,0,32,16);
	bits::set(output_buffer,(reinterpret_cast<GRIB2Grid *>(g))->discipline(),48,8);
	bits::set(output_buffer,edition_,56,8);
	offset=16;
	break;
    }
    default:
    {
	myerror="unable to encode GRIB "+strutils::itos(edition_);
	exit(1);
    }
  }
}

void GRIBMessage::pack_pds(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  unsigned char flag=0x0;
  short hr,min;
  size_t dum;
  GRIBGrid *g;

  g=reinterpret_cast<GRIBGrid *>(grid);
  if (lengths_.pds_supp > 0) {
    bits::set(output_buffer,41+lengths_.pds_supp,off,24);
    offset+=(41+lengths_.pds_supp);
  }
  else {
    bits::set(output_buffer,28,off,24);
    if ((g->reference_date_time_.year() % 100) != 0) {
	bits::set(output_buffer,g->reference_date_time_.year() % 100,off+96,8);
    }
    else {
	bits::set(output_buffer,100,off+96,8);
    }
    offset+=28;
  }
  if (edition_ == 0) {
    bits::set(output_buffer,edition_,off+24,8);
    bits::set(output_buffer,g->reference_date_time_.year(),off+96,8);
  }
  else {
    bits::set(output_buffer,g->grib.table,off+24,8);
  }
  bits::set(output_buffer,g->grid.src,off+32,8);
  bits::set(output_buffer,g->grib.process,off+40,8);
  bits::set(output_buffer,g->grib.grid_catalog_id,off+48,8);
  if (gds_included) {
    flag|=0x80;
  }
  if (bms_included) {
    flag|=0x40;
  }
  bits::set(output_buffer,flag,off+56,8);
  bits::set(output_buffer,g->grid.param,off+64,8);
  bits::set(output_buffer,g->grid.level1_type,off+72,8);
  switch (g->grid.level1_type) {
    case 101:
    {
	dum=g->grid.level1/10.;
	bits::set(output_buffer,dum,off+80,8);
	dum=g->grid.level2/10.;
	bits::set(output_buffer,dum,off+88,8);
	break;
    }
    case 107:
    case 119:
    {
	dum=g->grid.level1*10000.;
	bits::set(output_buffer,dum,off+80,16);
	break;
    }
    case 108:
    {
	dum=g->grid.level1*100.;
	bits::set(output_buffer,dum,off+80,8);
	dum=g->grid.level2*100.;
	bits::set(output_buffer,dum,off+88,8);
	break;
    }
    case 114:
    {
	dum=475.-(g->grid.level1);
	bits::set(output_buffer,dum,off+80,8);
	dum=475.-(g->grid.level2);
	bits::set(output_buffer,dum,off+88,8);
	break;
    }
    case 128:
    {
	dum=(1.1-(g->grid.level1))*1000.;
	bits::set(output_buffer,dum,off+80,8);
	dum=(1.1-(g->grid.level2))*1000.;
	bits::set(output_buffer,dum,off+88,8);
	break;
    }
    case 141:
    {
	dum=g->grid.level1/10.;
	bits::set(output_buffer,dum,off+80,8);
	dum=1100.-(g->grid.level2);
	bits::set(output_buffer,dum,off+88,8);
	break;
    }
    case 104:
    case 106:
    case 110:
    case 112:
    case 116:
    case 120:
    case 121:
    {
	dum=g->grid.level1;
	bits::set(output_buffer,dum,off+80,8);
	dum=g->grid.level2;
	bits::set(output_buffer,dum,off+88,8);
	break;
    }
    default:
    {
	dum=g->grid.level1;
	bits::set(output_buffer,dum,off+80,16);
    }
  }
  bits::set(output_buffer,g->reference_date_time_.month(),off+104,8);
  bits::set(output_buffer,g->reference_date_time_.day(),off+112,8);
  hr=g->reference_date_time_.time()/10000;
  min=(g->reference_date_time_.time() % 10000)/100;
  bits::set(output_buffer,hr,off+120,8);
  bits::set(output_buffer,min,off+128,8);
  bits::set(output_buffer,g->grib.time_unit,off+136,8);
  if (g->grib.t_range != 10) {
    bits::set(output_buffer,g->grib.p1,off+144,8);
    bits::set(output_buffer,g->grib.p2,off+152,8);
  }
  else {
    bits::set(output_buffer,g->grib.p1,off+144,16);
  }
  bits::set(output_buffer,g->grib.t_range,off+160,8);
  bits::set(output_buffer,g->grid.nmean,off+168,16);
  bits::set(output_buffer,g->grib.nmean_missing,off+184,8);
  if (edition_ == 1) {
    if ((g->reference_date_time_.year() % 100) != 0) {
	bits::set(output_buffer,g->reference_date_time_.year()/100+1,off+192,8);
    }
    else {
	bits::set(output_buffer,g->reference_date_time_.year()/100,off+192,8);
    }
    bits::set(output_buffer,g->grib.sub_center,off+200,8);
    if (g->grib.D < 0) {
	bits::set(output_buffer,-(g->grib.D)+0x8000,off+208,16);
    }
    else {
	bits::set(output_buffer,g->grib.D,off+208,16);
    }
  }
// pack the PDS supplement, if it exists
  if (lengths_.pds_supp > 0) {
    bits::set(output_buffer,pds_supp.get(),off+320,8,0,lengths_.pds_supp);
  }
}

void GRIBMessage::pack_gds(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  short sign;
  int dum;
  GRIBGrid *g;

  g=reinterpret_cast<GRIBGrid *>(grid);
bits::set(output_buffer,255,off+24,8);
bits::set(output_buffer,255,off+32,8);
  bits::set(output_buffer,g->grid.grid_type,off+40,8);
  switch (g->grid.grid_type) {
    case 0:
    case 4:
    {
// Latitude/Longitude
// Gaussian Lat/Lon
	bits::set(output_buffer,g->dim.x,off+48,16);
	bits::set(output_buffer,g->dim.y,off+64,16);
	dum=lround(g->def.slatitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	bits::set(output_buffer,sign,off+80,1);
	bits::set(output_buffer,dum,off+81,23);
	dum=lround(g->def.slongitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	bits::set(output_buffer,sign,off+104,1);
	bits::set(output_buffer,dum,off+105,23);
	bits::set(output_buffer,g->grib.rescomp,off+128,8);
	dum=lround(g->def.elatitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else {
	  sign=0;
	}
	bits::set(output_buffer,sign,off+136,1);
	bits::set(output_buffer,dum,off+137,23);
	dum=lround(g->def.elongitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	bits::set(output_buffer,sign,off+160,1);
	bits::set(output_buffer,dum,off+161,23);
	if (floatutils::myequalf(g->def.loincrement,-99.999)) {
	  dum=0xffff;
	}
	else {
	  dum=lround(g->def.loincrement*1000.);
	}
	bits::set(output_buffer,dum,off+184,16);
	if (g->grid.grid_type == 0) {
	  if (floatutils::myequalf(g->def.laincrement,-99.999)) {
	    dum=0xffff;
	  }
	  else {
	    dum=lround(g->def.laincrement*1000.);
	  }
	}
	else {
	  if (g->def.num_circles == 0) {
	    dum=0xffff;
	  }
	  else {
	    dum=g->def.num_circles;
	  }
	}
	bits::set(output_buffer,dum,off+200,16);
	bits::set(output_buffer,g->grib.scan_mode,off+216,8);
	bits::set(output_buffer,0,off+224,32);
	bits::set(output_buffer,32,off,24);
	offset+=32;
	break;
    }
    case 3:
    case 5:
    {
// Lambert Conformal
// Polar Stereographic
	bits::set(output_buffer,g->dim.x,off+48,16);
	bits::set(output_buffer,g->dim.y,off+64,16);
	dum=lround(g->def.slatitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	bits::set(output_buffer,sign,off+80,1);
	bits::set(output_buffer,dum,off+81,23);
	dum=lround(g->def.slongitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	bits::set(output_buffer,sign,off+104,1);
	bits::set(output_buffer,dum,off+105,23);
	bits::set(output_buffer,g->grib.rescomp,off+128,8);
	dum=lround(g->def.olongitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	bits::set(output_buffer,sign,off+136,1);
	bits::set(output_buffer,dum,off+137,23);
	dum=lround(g->def.dx*1000.);
	bits::set(output_buffer,dum,off+160,24);
	dum=lround(g->def.dy*1000.);
	bits::set(output_buffer,dum,off+184,24);
	bits::set(output_buffer,g->def.projection_flag,off+208,8);
	bits::set(output_buffer,g->grib.scan_mode,off+216,8);
	if (g->grid.grid_type == 3) {
	  dum=lround(g->def.stdparallel1*1000.);
	  if (dum < 0) {
	    sign=1;
	    dum=-dum;
	  }
	  else {
	    sign=0;
	  }
	  bits::set(output_buffer,sign,off+224,1);
	  bits::set(output_buffer,dum,off+225,23);
	  dum=lround(g->def.stdparallel2*1000.);
	  if (dum < 0) {
	    sign=1;
	    dum=-dum;
	  }
	  else {
	    sign=0;
	  }
	  bits::set(output_buffer,sign,off+248,1);
	  bits::set(output_buffer,dum,off+249,23);
	  dum=lround(g->grib.sp_lat*1000.);
	  if (dum < 0) {
		sign=1;
		dum=-dum;
	  }
	  else
		sign=0;
	  bits::set(output_buffer,sign,off+272,1);
	  bits::set(output_buffer,dum,off+273,23);
	  dum=lround(g->grib.sp_lon*1000.);
	  if (dum < 0) {
		sign=1;
		dum=-dum;
	  }
	  else
		sign=0;
	  bits::set(output_buffer,sign,off+296,1);
	  bits::set(output_buffer,dum,off+297,23);
	  bits::set(output_buffer,40,off,24);
	  offset+=40;
	}
	else {
	  bits::set(output_buffer,0,off+224,32);
	  bits::set(output_buffer,32,off,24);
	  offset+=32;
	}
	break;
    }
  }
}

void GRIBMessage::pack_bms(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  int ub;
  GRIBGrid *g;
  GRIB2Grid *g2;

  switch (edition_) {
    case 0:
    case 1:
    {
	g=reinterpret_cast<GRIBGrid *>(grid);
	ub=8-(g->dim.size % 8);
	bits::set(output_buffer,ub,off+24,8);
bits::set(output_buffer,0,off+32,16);
	bits::set(output_buffer,g->bitmap.map,off+48,1,0,g->dim.size);
	bits::set(output_buffer,(g->dim.size+7)/8+6,off,24);
	offset+=(g->dim.size+7)/8+6;
	break;
    }
    case 2:
    {
	g2=reinterpret_cast<GRIB2Grid *>(grid);
	bits::set(output_buffer,6,off+32,8);
	if (g2->bitmap.applies) {
bits::set(output_buffer,0,off+40,8);
	  bits::set(output_buffer,g2->bitmap.map,off+48,1,0,g2->dim.size);
	  bits::set(output_buffer,(g2->dim.size+7)/8+6,off,32);
	  offset+=(g2->dim.size+7)/8;
	}
	else {
	  bits::set(output_buffer,255,off+40,8);
	  bits::set(output_buffer,6,off,32);
	}
	offset+=6;
	break;
    }
  }
}

void GRIBMessage::pack_bds(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  double d,e,range;
  int E=0,n,m,len;
  int cnt=0,num_packed=0,*packed,max_pack;
  short ub,flag;
  GRIBGrid *g=reinterpret_cast<GRIBGrid *>(grid);

  if (!g->grid.filled) {
    myerror="empty grid";
    exit(1);
  }
  d=pow(10.,g->grib.D);
  range=(g->stats.max_val-g->stats.min_val)*d;
  if (range > 0.) {
    max_pack=pow(2.,g->grib.pack_width)-1;
    if (!floatutils::myequalf(range,0.)) {
	while (lround(range) <= max_pack) {
	  range*=2.;
	  E--;
	}
	while (lround(range) > max_pack) {
	  range*=0.5;
	  E++;
	}
    }
    num_packed=g->dim.size-(g->grid.num_missing);
    len=num_packed*g->grib.pack_width;
    bits::set(output_buffer,(len+7)/8+11,off,24);
    if ( (ub=(len % 8)) > 0) {
	ub=8-ub;
    }
    bits::set(output_buffer,ub,off+28,4);
    bits::set(output_buffer,0,off+88+len,ub);
    offset+=(len+7)/8+11;
  }
  else {
    bits::set(output_buffer,11,off,24);
    g->grib.pack_width=0;
    offset+=11;
  }
  bits::set(output_buffer,g->grib.pack_width,off+80,8);
  flag=0x0;
  bits::set(output_buffer,flag,off+24,4);
  auto ibm_rep=floatutils::ibmconv(g->stats.min_val*d);
  bits::set(output_buffer,abs(E),off+32,16);
  if (E < 0) {
    bits::set(output_buffer,1,off+32,1);
  }
  bits::set(output_buffer,ibm_rep,off+48,32);
  if (g->grib.pack_width == 0) {
    return;
  }
  e=pow(2.,E);
  if ((flag & 0x4) == 0) {
// simple packing
    packed=new int[num_packed];
    cnt=0;
    if (g->grid.src == 7 && g->grib.grid_catalog_id != 255) {
	switch (g->grib.grid_catalog_id) {
	  case 23:
	  case 24:
	  case 26:
	  case 63:
	  case 64:
	  {
//	    packed[cnt]=lroundf(((double)g->grid.pole-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->grid.pole*d)-lround(g->stats.min_val*d))/e);
	    cnt++;
	    break;
	  }
	}
    }
    switch (g->grid.grid_type) {
	case 0:
	case 3:
	case 4:
	{
	  for (n=0; n < g->dim.y; n++) {
	    for (m=0; m < g->dim.x; m++) {
		if (!floatutils::myequalf(g->gridpoints_[n][m],Grid::missing_value)) {
//		  packed[cnt]=lroundf(((double)g->gridpoints_[n][m]-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->gridpoints_[n][m]*d)-lround(g->stats.min_val*d))/e);
		  ++cnt;
		}
	    }
	  }
	  if (g->grid.src == 7 && g->grib.grid_catalog_id != 255) {
	    switch (g->grib.grid_catalog_id) {
		case 21:
		case 22:
		case 25:
		case 61:
		case 62:
		{
//		  packed[cnt]=lroundf(((double)g->grid.pole-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->grid.pole*d)-lround(g->stats.min_val*d))/e);
		  break;
		}
	    }
	  }
	  break;
	}
	case 5:
	{
	  for (n=0; n < g->dim.y; ++n) {
	    for (m=0; m < g->dim.x; ++m) {
		if (!floatutils::myequalf(g->gridpoints_[n][m],Grid::missing_value)) {
//		  packed[cnt]=lroundf(((double)g->gridpoints_[n][m]-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->gridpoints_[n][m]*d)-lround(g->stats.min_val*d))/e);
		  ++cnt;
		}
	    }
	  }
	  break;
	}
	default:
	{
	  std::cerr << "Warning: pack_bds does not recognize grid type " << g->grid.grid_type << std::endl;
	}
    }
    if (cnt != num_packed) {
	mywarning="pack_bds: specified number of gridpoints "+strutils::itos(num_packed)+" does not agree with actual number packed "+strutils::itos(cnt)+"  date: "+g->reference_date_time_.to_string()+" "+strutils::itos(g->grid.param);
    }
    bits::set(output_buffer,packed,off+88,g->grib.pack_width,0,num_packed);
  }
  else {
// second-order packing
    myerror="complex packing not defined";
    exit(1);
  }
  if (num_packed > 0) {
    delete[] packed;
  }
}

void GRIBMessage::pack_end(unsigned char *output_buffer,off_t& offset) const
{
  bits::set(output_buffer,0x37373737,offset*8,32);
  offset+=4;
}

off_t GRIBMessage::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length)
{
  off_t offset;

  pack_is(output_buffer,offset,grids.front());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  pack_pds(output_buffer,offset,grids.front());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  if (gds_included) {
    pack_gds(output_buffer,offset,grids.front());
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copy_to_buffer()";
	exit(1);
    }
  }
  if (bms_included) {
    pack_bms(output_buffer,offset,grids.front());
  }
  pack_bds(output_buffer,offset,grids.front());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  pack_end(output_buffer,offset);
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  mlength=offset;
  pack_length(output_buffer);
  return mlength;
}

void GRIBMessage::clear_grids()
{
  for (auto grid : grids)
    delete grid;
  grids.clear();
}

void GRIBMessage::unpack_is(const unsigned char *stream_buffer)
{
  int test;

  bits::get(stream_buffer,test,0,32);
  if (test != 0x47524942) {
    myerror="not a GRIB message";
    exit(1);
  }
  bits::get(stream_buffer,edition_,56,8);
  switch (edition_) {
    case 0:
    {
	mlength=lengths_.is=offsets_.is=curr_off=4;
	break;
    }
    case 1:
    {
	bits::get(stream_buffer,mlength,32,24);
	lengths_.is=offsets_.is=curr_off=8;
	break;
    }
    case 2:
    {
	bits::get(stream_buffer,mlength,64,64);
	lengths_.is=offsets_.is=curr_off=16;
	break;
    }
  }
}

void GRIBMessage::unpack_pds(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short flag,soff;
  short yr,mo,dy,hr,min;
  int dum;
  GRIBGrid *g;

  offsets_.pds=curr_off;
  g=reinterpret_cast<GRIBGrid *>(grids.back());
  g->ensdata.fcst_type="";
  g->ensdata.id="";
  g->ensdata.total_size=0;
  bits::get(stream_buffer,lengths_.pds,off,24);
  if (edition_ == 1) {
    bits::get(stream_buffer,g->grib.table,off+24,8);
  }
  g->grib.last_src=g->grid.src;
  bits::get(stream_buffer,g->grid.src,off+32,8);
  bits::get(stream_buffer,g->grib.process,off+40,8);
  bits::get(stream_buffer,g->grib.grid_catalog_id,off+48,8);
  g->grib.scan_mode=0x40;
  if (g->grid.src == 7) {
    switch (g->grib.grid_catalog_id) {
	case 21:
	{
	  g->dim.y=36;
	  g->dim.x=37;
	  g->dim.size=1333;
	  g->def.slatitude=0.;
	  g->def.elatitude=90.;
	  g->def.laincrement=2.5;
	  g->def.slongitude=0.;
	  g->def.elongitude=180.;
	  g->def.loincrement=5.;
	  break;
	}
	case 22:
	{
	  g->dim.y=36;
	  g->dim.x=37;
	  g->dim.size=1333;
	  g->def.slatitude=0.;
	  g->def.elatitude=90.;
	  g->def.laincrement=2.5;
	  g->def.slongitude=-180.;
	  g->def.elongitude=0.;
	  g->def.loincrement=5.;
	  break;
	}
	case 23:
	{
	  g->dim.y=36;
	  g->dim.x=37;
	  g->dim.size=1333;
	  g->def.slatitude=-90.;
	  g->def.elatitude=0.;
	  g->def.laincrement=2.5;
	  g->def.slongitude=0.;
	  g->def.elongitude=180.;
	  g->def.loincrement=5.;
	  break;
	}
	case 24:
	{
	  g->dim.y=36;
	  g->dim.x=37;
	  g->dim.size=1333;
	  g->def.slatitude=-90.;
	  g->def.elatitude=0.;
	  g->def.laincrement=2.5;
	  g->def.slongitude=-180.;
	  g->def.elongitude=0.;
	  g->def.loincrement=5.;
	  break;
	}
	case 25:
	{
	  g->dim.y=18;
	  g->dim.x=72;
	  g->dim.size=1297;
	  g->def.slatitude=0.;
	  g->def.elatitude=90.;
	  g->def.laincrement=5.;
	  g->def.slongitude=0.;
	  g->def.elongitude=355.;
	  g->def.loincrement=5.;
	  break;
	}
	case 26:
	{
	  g->dim.y=18;
	  g->dim.x=72;
	  g->dim.size=1297;
	  g->def.slatitude=-90.;
	  g->def.elatitude=0.;
	  g->def.laincrement=5.;
	  g->def.slongitude=0.;
	  g->def.elongitude=355.;
	  g->def.loincrement=5.;
	  break;
	}
	case 61:
	{
	  g->dim.y=45;
	  g->dim.x=91;
	  g->dim.size=4096;
	  g->def.slatitude=0.;
	  g->def.elatitude=90.;
	  g->def.laincrement=2.;
	  g->def.slongitude=0.;
	  g->def.elongitude=180.;
	  g->def.loincrement=2.;
	  break;
	}
	case 62:
	{
	  g->dim.y=45;
	  g->dim.x=91;
	  g->dim.size=4096;
	  g->def.slatitude=0.;
	  g->def.elatitude=90.;
	  g->def.laincrement=2.;
	  g->def.slongitude=-180.;
	  g->def.elongitude=0.;
	  g->def.loincrement=2.;
	  break;
	}
	case 63:
	{
	  g->dim.y=45;
	  g->dim.x=91;
	  g->dim.size=4096;
	  g->def.slatitude=-90.;
	  g->def.elatitude=0.;
	  g->def.laincrement=2.;
	  g->def.slongitude=0.;
	  g->def.elongitude=180.;
	  g->def.loincrement=2.;
	  break;
	}
	case 64:
	{
	  g->dim.y=45;
	  g->dim.x=91;
	  g->dim.size=4096;
	  g->def.slatitude=-90.;
	  g->def.elatitude=0.;
	  g->def.laincrement=2.;
	  g->def.slongitude=-180.;
	  g->def.elongitude=0.;
	  g->def.loincrement=2.;
	  break;
	}
	default:
	{
	  g->grib.scan_mode=0x0;
	}
    }
  }
  bits::get(stream_buffer,flag,off+56,8);
  if ( (flag & 0x80) == 0x80) {
    gds_included=true;
  }
  else {
    gds_included=false;
    lengths_.gds=0;
  }
  if ( (flag & 0x40) == 0x40) {
    bms_included=true;
  }
  else {
    bms_included=false;
    lengths_.bms=0;
  }
  bits::get(stream_buffer,g->grid.param,off+64,8);
  bits::get(stream_buffer,g->grid.level1_type,off+72,8);
  switch (g->grid.level1_type) {
    case 100:
    case 103:
    case 105:
    case 109:
    case 111:
    case 113:
    case 115:
    case 125:
    case 160:
    case 200:
    case 201:
    {
	bits::get(stream_buffer,dum,off+80,16);
	g->grid.level1=dum;
	g->grid.level2=0.;
	g->grid.level2_type=-1;
	break;
    }
    case 101:
    {
	bits::get(stream_buffer,dum,off+80,8);
	g->grid.level1=dum*10.;
	bits::get(stream_buffer,dum,off+88,8);
	g->grid.level2=dum*10.;
	g->grid.level2_type=g->grid.level1_type;
	break;
    }
    case 107:
    case 119:
    {
	bits::get(stream_buffer,dum,off+80,16);
	g->grid.level1=dum/10000.;
	g->grid.level2=0.;
	g->grid.level2_type=-1;
	break;
    }
    case 108:
    case 120:
    {
	bits::get(stream_buffer,dum,off+80,8);
	g->grid.level1=dum/100.;
	bits::get(stream_buffer,dum,off+88,8);
	g->grid.level2=dum/100.;
	g->grid.level2_type=g->grid.level1_type;
	break;
    }
    case 114:
    {
	bits::get(stream_buffer,dum,off+80,8);
	g->grid.level1=475.-dum;
	bits::get(stream_buffer,dum,off+88,8);
	g->grid.level2=475.-dum;
	g->grid.level2_type=g->grid.level1_type;
	break;
    }
    case 121:
    {
	bits::get(stream_buffer,dum,off+80,8);
	g->grid.level1=1100.-dum;
	bits::get(stream_buffer,dum,off+88,8);
	g->grid.level2=1100.-dum;
	g->grid.level2_type=g->grid.level1_type;
	break;
    }
    case 128:
    {
	bits::get(stream_buffer,dum,off+80,8);
	g->grid.level1=1.1-dum/1000.;
	bits::get(stream_buffer,dum,off+88,8);
	g->grid.level2=1.1-dum/1000.;
	g->grid.level2_type=g->grid.level1_type;
	break;
    }
    case 141:
    {
	bits::get(stream_buffer,dum,off+80,8);
	g->grid.level1=dum*10.;
	bits::get(stream_buffer,dum,off+88,8);
	g->grid.level2=1100.-dum;
	g->grid.level2_type=g->grid.level1_type;
	break;
    }
    default:
    {
	bits::get(stream_buffer,dum,off+80,8);
	g->grid.level1=dum;
	bits::get(stream_buffer,dum,off+88,8);
	g->grid.level2=dum;
	g->grid.level2_type=g->grid.level1_type;
     }
  }
  bits::get(stream_buffer,yr,off+96,8);
  bits::get(stream_buffer,mo,off+104,8);
  bits::get(stream_buffer,dy,off+112,8);
  bits::get(stream_buffer,hr,off+120,8);
  bits::get(stream_buffer,min,off+128,8);
  hr=hr*100+min;
  bits::get(stream_buffer,g->grib.time_unit,off+136,8);
  switch (edition_) {
    case 0:
    {
	if (yr > 20) {
	  yr+=1900;
	}
	else {
	  yr+=2000;
	}
	g->grib.sub_center=0;
	g->grib.D=0;
	break;
    }
    case 1:
    {
	bits::get(stream_buffer,g->grib.century,off+192,8);
	yr=yr+(g->grib.century-1)*100;
	bits::get(stream_buffer,g->grib.sub_center,off+200,8);
	bits::get(stream_buffer,g->grib.D,off+208,16);
	if (g->grib.D > 0x8000) {
	  g->grib.D=0x8000-g->grib.D;
	}
	break;
    }
  }
  g->reference_date_time_.set(yr,mo,dy,hr*100);
  bits::get(stream_buffer,g->grib.t_range,off+160,8);
  g->grid.nmean=g->grib.nmean_missing=0;
  g->grib.p2=0;
  switch (g->grib.t_range) {
    case 10:
    {
	bits::get(stream_buffer,g->grib.p1,off+144,16);
	break;
    }
    default:
    {
	bits::get(stream_buffer,g->grib.p1,off+144,8);
	bits::get(stream_buffer,g->grib.p2,off+152,8);
	bits::get(stream_buffer,g->grid.nmean,off+168,16);
	bits::get(stream_buffer,g->grib.nmean_missing,off+184,8);
	if (g->grib.t_range >= 113 && g->grib.time_unit == 1 && g->grib.p2 > 24) {
	  std::cerr << "Warning: invalid P2 " << g->grib.p2;
	  bits::get(stream_buffer,g->grib.p2,off+152,4);
	  std::cerr << " re-read as " << g->grib.p2 << " for time range " << g->grib.t_range << std::endl;
	}
    }
  }
  g->forecast_date_time_=g->valid_date_time_=g->reference_date_time_;
  g->grib.prod_descr=gributils::grib_product_description(g,g->forecast_date_time_,g->valid_date_time_,g->grid.fcst_time);
// unpack PDS supplement, if it exists
  if (lengths_.pds > 28) {
    if (lengths_.pds < 41) {
	mywarning="supplement to PDS is in reserved position starting at octet 29";
	lengths_.pds_supp=lengths_.pds-28;
	soff=28;
    }
    else {
	lengths_.pds_supp=lengths_.pds-40;
	soff=40;
    }
    pds_supp.reset(new unsigned char[lengths_.pds_supp]);
    std::copy(&stream_buffer[lengths_.is+soff],&stream_buffer[lengths_.is+soff+lengths_.pds_supp],pds_supp.get());
// NCEP ensemble grids are described in the PDS extension
    if (g->grid.src == 7 && g->grib.sub_center == 2 && pds_supp[0] == 1) {
	switch (pds_supp[1]) {
	  case 1:
	  {
	    switch (pds_supp[2]) {
		case 1:
		{
		  g->ensdata.fcst_type="HRCTL";
		  break;
		}
		case 2:
		{
		  g->ensdata.fcst_type="LRCTL";
		  break;
		}
	    }
	    break;
	  }
	  case 2:
	  {
	    g->ensdata.fcst_type="NEG";
	    g->ensdata.id=strutils::itos(pds_supp[2]);
	    break;
	  }
	  case 3:
	  {
	    g->ensdata.fcst_type="POS";
	    g->ensdata.id=strutils::itos(pds_supp[2]);
	    break;
	  }
	  case 5:
	  {
	    g->ensdata.id="ALL";
	    switch (static_cast<int>(pds_supp[3])) {
		case 1:
		{
		  g->ensdata.fcst_type="UNWTD_MEAN";
		  break;
		}
		case 11:
		{
		  g->ensdata.fcst_type="STDDEV";
		  break;
		}
		default:
		{
		  myerror="PDS byte 44 value "+std::string(&(reinterpret_cast<char *>(pds_supp.get()))[3],1)+" not recognized";
		  exit(1);
		}
	    }
	    if (lengths_.pds_supp > 20) {
		g->ensdata.total_size=pds_supp[20];
	    }
	    break;
	  }
	  default:
	  {
	    myerror="PDS byte 42 value "+std::string(&(reinterpret_cast<char *>(pds_supp.get()))[1],1)+" not recognized";
	    exit(1);
	  }
	}
    }
  }
  else {
    lengths_.pds_supp=0;
    pds_supp.reset(nullptr);
  }
  g->grid.num_missing=0;
  curr_off+=lengths_.pds;
}

void GRIBMessage::unpack_gds(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  GRIBGrid *g;
  int n,noff;
  int dum,sl_sign;
  short PV,NV,PL;

  offsets_.gds=curr_off;
  g=reinterpret_cast<GRIBGrid *>(grids.back());
  bits::get(stream_buffer,lengths_.gds,off,24);
  PL=0xff;
  switch (edition_) {
    case 1:
    {
	bits::get(stream_buffer,PV,off+32,8);
	if (PV != 0xff) {
	  bits::get(stream_buffer,NV,off+24,8);
	  if (NV > 0)
	    PL=(4*NV)+PV;
	  else
	    PL=PV;
/*
    if (grib.PL >= grib.lengths_.gds) {
	std::cerr << "Error in unpack_gds: grib.PL (" << grib.PL << ") exceeds grib.lengths_.gds (" << grib.lengths_.gds << "); NV=" << grib.NV << ", PV=" << grib.PV << std::endl;
	exit(1);
    }
*/
	  if (PL > lengths_.gds) {
	    PL=0xff;
	  }
	  else {
	    if (g->plist != nullptr) {
		delete[] g->plist;
	    }
	    bits::get(stream_buffer,g->dim.y,off+64,16);
	    g->plist=new int[g->dim.y];
	    bits::get(stream_buffer,g->plist,(PL+curr_off-1)*8,16,0,g->dim.y);
	  }
	}
	break;
    }
  }
  bits::get(stream_buffer,g->grid.grid_type,off+40,8);
  switch (g->grid.grid_type) {
    case 0:
    {
	g->def.type=Grid::latitudeLongitudeType;
	break;
    }
    case 1:
    {
	g->def.type=Grid::mercatorType;
	break;
    }
    case 3:
    {
	g->def.type=Grid::lambertConformalType;
	break;
    }
    case 5:
    {
	g->def.type=Grid::polarStereographicType;
	break;
    }
    case 4:
    case 10:
    {
	g->def.type=Grid::gaussianLatitudeLongitudeType;
	break;
    }
    case 50:
    {
	g->def.type=Grid::sphericalHarmonicsType;
	break;
    }
  }
  switch (g->grid.grid_type) {
    case 0:
    case 4:
    case 10:
    {
// Latitude/Longitude
// Gaussian Lat/Lon
// Rotated Lat/Lon
	bits::get(stream_buffer,g->dim.x,off+48,16);
	bits::get(stream_buffer,g->dim.y,off+64,16);
	if (g->dim.x == -1 && PL != 0xff) {
	  noff=off+(PL-1)*8;
	  g->dim.size=0;
	  for (n=PL; n < lengths_.gds; n+=2) {
	    bits::get(stream_buffer,dum,noff,16);
	    g->dim.size+=dum;
	    noff+=16;
	  }
	}
	else {
	  g->dim.size=g->dim.x*g->dim.y;
	}
	bits::get(stream_buffer,dum,off+80,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.slatitude=dum/1000.;
	bits::get(stream_buffer,dum,off+104,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.slongitude=dum/1000.;
/*
	dum=lround(64.*g->def.slongitude);
	g->def.slongitude=dum/64.;
*/
	bits::get(stream_buffer,g->grib.rescomp,off+128,8);
	bits::get(stream_buffer,dum,off+136,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.elatitude=dum/1000.;
	bits::get(stream_buffer,sl_sign,off+160,1);
	sl_sign= (sl_sign == 0) ? 1 : -1;
	bits::get(stream_buffer,dum,off+161,23);
	g->def.elongitude=(dum/1000.)*sl_sign;
/*
	dum=lround(64.*g->def.elongitude);
	g->def.elongitude=dum/64.;
*/
	bits::get(stream_buffer,dum,off+184,16);
	if (dum == 0xffff) {
	  dum=-99000;
	}
	g->def.loincrement=dum/1000.;
/*
	dum=lround(64.*g->def.loincrement);
	g->def.loincrement=dum/64.;
*/
	bits::get(stream_buffer,dum,off+200,16);
	if (dum == 0xffff) {
	  dum=-99999;
	}
	if (g->grid.grid_type == 0) {
	  g->def.laincrement=dum/1000.;
	}
	else {
	  g->def.num_circles=dum;
	}
	bits::get(stream_buffer,g->grib.scan_mode,off+216,8);
/*
	if (g->def.elongitude < g->def.slongitude && g->grib.scan_mode < 0x80) {
	  g->def.elongitude+=360.;
	}
*/
	break;
    }
    case 3:
    case 5:
    {
// Lambert Conformal
// Polar Stereographic
	bits::get(stream_buffer,g->dim.x,off+48,16);
	bits::get(stream_buffer,g->dim.y,off+64,16);
	g->dim.size=g->dim.x*g->dim.y;
	bits::get(stream_buffer,dum,off+80,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.slatitude=dum/1000.;
	bits::get(stream_buffer,dum,off+104,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.slongitude=dum/1000.;
	bits::get(stream_buffer,dum,off+104,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.slongitude=dum/1000.;
	bits::get(stream_buffer,g->grib.rescomp,off+128,8);
	bits::get(stream_buffer,dum,off+136,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.olongitude=dum/1000.;
	bits::get(stream_buffer,dum,off+160,24);
	g->def.dx=dum/1000.;
	bits::get(stream_buffer,dum,off+184,24);
	g->def.dy=dum/1000.;
	bits::get(stream_buffer,g->def.projection_flag,off+208,8);
	bits::get(stream_buffer,g->grib.scan_mode,off+216,8);
	if (g->grid.grid_type == 3) {
	  bits::get(stream_buffer,dum,off+224,24);
	  if (dum > 0x800000) {
	    dum=0x800000-dum;
	  }
	  g->def.stdparallel1=dum/1000.;
	  bits::get(stream_buffer,dum,off+248,24);
	  if (dum > 0x800000) {
	    dum=0x800000-dum;
	  }
	  g->def.stdparallel2=dum/1000.;
	  bits::get(stream_buffer,dum,off+272,24);
	  if (dum > 0x800000) {
	    dum=0x800000-dum;
	  }
	  g->grib.sp_lat=dum/1000.;
	  bits::get(stream_buffer,dum,off+296,24);
	  if (dum > 0x800000) {
	    dum=0x800000-dum;
	  }
	  g->grib.sp_lon=dum/1000.;
	  g->def.llatitude= (fabs(g->def.stdparallel1) > fabs(g->def.stdparallel2)) ? g->def.stdparallel1 : g->def.stdparallel2;
	  if ( (g->def.projection_flag & 0x40) == 0x40) {
	    g->def.num_centers=2;
	  }
	  else {
	    g->def.num_centers=1;
	  }
	}
	else {
	  if (g->def.projection_flag < 0x80) {
	    g->def.llatitude=60.;
	  }
	  else {
	    g->def.llatitude=-60.;
	  }
	}
	break;
    }
    case 50:
    {
	g->dim.x=g->dim.y=g->dim.size=1;
	bits::get(stream_buffer,g->def.trunc1,off+48,16);
	bits::get(stream_buffer,g->def.trunc2,off+64,16);
	bits::get(stream_buffer,g->def.trunc3,off+80,16);
	break;
    }
    default:
    {
	std::cerr << "Warning: unpack_gds does not recognize data representation " << g->grid.grid_type << std::endl;
    }
  }
  curr_off+=lengths_.gds;
}

void GRIBMessage::unpack_bms(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  GRIBGrid *g;
  GRIB2Grid *g2;
  short sec_num,ind;
  int n,len;

  offsets_.bms=curr_off;
  switch (edition_) {
    case 1:
    {
	g=reinterpret_cast<GRIBGrid *>(grids.back());
	bits::get(stream_buffer,lengths_.bms,off,24);
	bits::get(stream_buffer,ind,off+32,16);
	if (ind == 0) {
	  if (g->dim.size > g->bitmap.capacity) {
	    if (g->bitmap.map != nullptr) {
		delete[] g->bitmap.map;
	    }
	    g->bitmap.capacity=g->dim.size;
	    g->bitmap.map=new unsigned char[g->bitmap.capacity];
	  }
	  bits::get(stream_buffer,g->bitmap.map,off+48,1,0,g->dim.size);
	  for (n=0; n < static_cast<int>(g->dim.size); n++) {
	    if (g->bitmap.map[n] == 0)
		g->grid.num_missing++;
	  }
	}
	break;
    }
    case 2:
    {
	g2=reinterpret_cast<GRIB2Grid *>(grids.back());
	bits::get(stream_buffer,lengths_.bms,off,32);
	bits::get(stream_buffer,sec_num,off+32,8);
	if (sec_num != 6) {
	  mywarning="may not be unpacking the Bitmap Section";
	}
	bits::get(stream_buffer,ind,off+40,8);
	switch (ind) {
	  case 0:
	  {
// bitmap applies and is specified in this section
	    if ( (len=(lengths_.bms-6)*8) > 0) {
		if (len > static_cast<int>(g2->bitmap.capacity)) {
		  if (g2->bitmap.map != nullptr) {
		    delete[] g2->bitmap.map;
		  }
		  g2->bitmap.capacity=len;
		  g2->bitmap.map=new unsigned char[g2->bitmap.capacity];
		}
/*
		for (n=0; n < len; n++) {
		  bits::get(stream_buffer,bit,off+48+n,1);
		  g2->bitmap.map[n]=bit;
		}
*/
bits::get(stream_buffer,g2->bitmap.map,off+48,1,0,len);
		g2->bitmap.applies=true;
	    }
	    else {
		g2->bitmap.applies=false;
	    }
	    break;
	  }
	  case 254:
	  {
// bitmap has already been defined for the current GRIB2 message
	    g2->bitmap.applies=true;
	    break;
	  }
	  case 255:
	  {
// bitmap does not apply to this message
	    g2->bitmap.applies=false;
	    break;
	  }
	  default:
	  {
	    myerror="not currently setup to deal with pre-defined bitmaps";
	    exit(1);
	  }
	}
	break;
    }
  }
  curr_off+=lengths_.bms;
}

void GRIBMessage::unpack_bds(const unsigned char *stream_buffer,bool fill_header_only)
{
  off_t off=curr_off*8;
  GRIBGrid *g;
  short bds_flag,unused;
  bool simple_packing=true;
  double d,e;
  size_t num_packed=0,npoints;
  short ydim;
  int *pval,pole_at=-1,ystart=0;
  int cnt=0,bcnt=0,avg_cnt=0;
  int n,m;

  offsets_.bds=curr_off;
  g=reinterpret_cast<GRIBGrid *>(grids.back());
  bits::get(stream_buffer,lengths_.bds,off,24);
  if (mlength >= 0x800000 && lengths_.bds < 120) {
// ECMWF large-file
    mlength&=0x7fffff;
    mlength*=120;
    mlength-=lengths_.bds;
    mlength+=4;
    lengths_.bds=mlength-curr_off-4;
  }
  bits::get(stream_buffer,bds_flag,off+24,4);
  if ((bds_flag & 0x4) == 0x4) {
    simple_packing=false;
  }
  bits::get(stream_buffer,unused,off+28,4);
  d=pow(10.,g->grib.D);
  g->stats.min_val=floatutils::ibmconv(stream_buffer,off+48)/d;
  if ((bds_flag & 0x2) == 0x2) {
    g->stats.min_val=lround(g->stats.min_val);
  }
  bits::get(stream_buffer,g->grib.pack_width,off+80,8);
  bits::get(stream_buffer,g->grib.E,off+32,16);
  if (g->grib.E > 0x8000) {
    g->grib.E=0x8000-g->grib.E;
  }
  if (simple_packing) {
    if (g->grib.pack_width > 0) {
	if (g->def.type != Grid::sphericalHarmonicsType) {
	  num_packed=g->dim.size-g->grid.num_missing;
	  npoints=((lengths_.bds-11)*8-unused)/g->grib.pack_width;
	  if (npoints != num_packed && g->dim.size != 0 && npoints != g->dim.size)
	    std::cerr << "Warning: unpack_bds: specified # gridpoints " << num_packed << " vs. # packed " << npoints << "  date: " << g->reference_date_time_.to_string() << "  param: " << g->grid.param << "  lengths.bds: " << lengths_.bds << "  ubits: " << unused << "  pack_width: " << g->grib.pack_width << std::endl;
	}
	else {
	}
    }
    else
	num_packed=0;
  }
  ydim=g->dim.y;
  if (fill_header_only) {
    if (g->dim.x == -1) {
	n=ydim/2;
	if (g->def.elongitude >= g->def.slongitude)
	  g->def.loincrement=(g->def.elongitude-g->def.slongitude)/(g->plist[n]-1);
	else
	  g->def.loincrement=(g->def.elongitude+360.-(g->def.slongitude))/(g->plist[n]-1);
    }
  }
  else {
    e=pow(2.,g->grib.E);
    g->grid.filled=true;
    g->grid.pole=Grid::missing_value;
    g->stats.max_val=-Grid::missing_value;
    g->stats.avg_val=0.;
    g->galloc();
    if (simple_packing) {
	pval=new int[num_packed];
	bits::get(stream_buffer,pval,off+88,g->grib.pack_width,0,num_packed);
	g->stats.min_i=-1;
	if (g->grid.num_in_pole_sum < 0) {
	  pole_at=-(g->grid.num_in_pole_sum+1);
	  if (pole_at == 0)
	    ystart=1;
	  else if (pole_at == static_cast<int>(g->dim.size-1))
	    ydim--;
	  else {
	    myerror="don't know how to unpack grid";
	    exit(1);
	  }
	}
	switch (g->grid.grid_type) {
	  case 0:
	  case 3:
	  case 4:
	  case 5:
	  {
	    if (pole_at >= 0) {
		g->grid.pole=g->stats.min_val+pval[pole_at]*e/d;
	    }
	    if (pole_at == 0) {
		++cnt;
	    }
	    for (n=ystart; n < ydim; ++n) {
// non-reduced grids
		if (g->dim.x != -1) {
		  for (m=0; m < g->dim.x; ++m) {
		    if (bms_included) {
			if (g->bitmap.map[bcnt] == 1) {
			  if (g->grib.pack_width > 0) {
			    g->gridpoints_[n][m]=g->stats.min_val+pval[cnt]*e/d;
			    if (pval[cnt] == 0) {
				if (g->stats.min_i < 0) {
				  g->stats.min_i=m+1;
				  if (g->def.type == Grid::gaussianLatitudeLongitudeType && g->grib.scan_mode == 0x40) {
				    g->stats.min_j=ydim-n;
				  }
				  else {
				    g->stats.min_j=n+1;
				  }
				}
			    }
			  }
			  else {
			    g->gridpoints_[n][m]=g->stats.min_val;
			    if (g->stats.min_i < 0) {
				g->stats.min_i=m+1;
				if (g->def.type == Grid::gaussianLatitudeLongitudeType && g->grib.scan_mode == 0x40)
				  g->stats.min_j=ydim-n;
				else
				  g->stats.min_j=n+1;
			    }
			  }
			  if (g->gridpoints_[n][m] > g->stats.max_val) {
			    g->stats.max_val=g->gridpoints_[n][m];
			    g->stats.max_i=m+1;
			    g->stats.max_j=n+1;
			  }
			  g->stats.avg_val+=g->gridpoints_[n][m];
			  avg_cnt++;
			  cnt++;
			}
			else {
			  g->gridpoints_[n][m]=Grid::missing_value;
			}
			++bcnt;
		    }
		    else {
			if (g->grib.pack_width > 0) {
			  g->gridpoints_[n][m]=g->stats.min_val+pval[cnt]*e/d;
			  if (pval[cnt] == 0) {
			    if (g->stats.min_i < 0) {
				g->stats.min_i=m+1;
				if (g->def.type == Grid::gaussianLatitudeLongitudeType && g->grib.scan_mode == 0x40)
			 	  g->stats.min_j=ydim-n;
				else
				  g->stats.min_j=n+1;
			    }
			  }
			}
			else {
			  g->gridpoints_[n][m]=g->stats.min_val;
			  if (g->stats.min_i < 0) {
			    g->stats.min_i=m+1;
			    if (g->def.type == Grid::gaussianLatitudeLongitudeType && g->grib.scan_mode == 0x40)
				g->stats.min_j=ydim-n;
			    else
				g->stats.min_j=n+1;
			  }
			}
			if (g->gridpoints_[n][m] > g->stats.max_val) {
			  g->stats.max_val=g->gridpoints_[n][m];
			  g->stats.max_i=m+1;
			  g->stats.max_j=n+1;
			}
			g->stats.avg_val+=g->gridpoints_[n][m];
			avg_cnt++;
			cnt++;
		    }
		  }
		}
// reduced grids
		else {
		  for (m=1; m <= static_cast<int>(g->gridpoints_[n][0]); ++m) {
		    if (bms_included) {
myerror="unable to unpack reduced grids with bitmap";
exit(1);
			if (g->grib.pack_width > 0) {
			}
			else {
			}
		    }
		    else {
			if (g->grib.pack_width > 0) {
			  g->gridpoints_[n][m]=g->stats.min_val+pval[cnt]*e/d;
			  if (pval[cnt] == 0) {
			    if (g->stats.min_i < 0) {
				g->stats.min_i=m;
				if (g->def.type == Grid::gaussianLatitudeLongitudeType && g->grib.scan_mode == 0x40) {
			 	  g->stats.min_j=ydim-n;
				}
				else {
				  g->stats.min_j=n+1;
				}
			    }
			  }
			}
			else {
			  g->gridpoints_[n][m]=g->stats.min_val;
			  if (g->stats.min_i < 0) {
			    g->stats.min_i=m;
			    if (g->def.type == Grid::gaussianLatitudeLongitudeType && g->grib.scan_mode == 0x40) {
				g->stats.min_j=ydim-n;
			    }
			    else {
				g->stats.min_j=n+1;
			    }
			  }
			}
			if (g->gridpoints_[n][m] > g->stats.max_val) {
		 	  g->stats.max_val=g->gridpoints_[n][m];
			  g->stats.max_i=m;
			  g->stats.max_j=n+1;
			}
			g->stats.avg_val+=g->gridpoints_[n][m];
			++avg_cnt;
			++cnt;
		    }
		  }
		}
	    }
	    if (pole_at >= 0) {
// pole was first point packed
		if (ystart == 1) {
		  for (n=0; n < g->dim.x; g->gridpoints_[0][n++]=g->grid.pole);
		}
// pole was last point packed
		else if (ydim != g->dim.y) {
		  for (n=0; n < g->dim.x; g->gridpoints_[ydim][n++]=g->grid.pole);
		}
	    }
	    else {
		if (floatutils::myequalf(g->def.slatitude,90.)) {
		  g->grid.pole=g->gridpoints_[0][0];
		}
		else if (floatutils::myequalf(g->def.elatitude,90.)) {
		  g->grid.pole=g->gridpoints_[ydim-1][0];
		}
	    }
	    break;
	  }
	  default:
	  {
	    std::cerr << "Warning: unpack_bds does not recognize grid type " << g->grid.grid_type << std::endl;
	  }
	}
	delete[] pval;
    }
    else {
// second-order packing
 	if ((bds_flag & 0x4) == 0x4) {
	  if ((bds_flag & 0xc) == 0xc) {
/*
std::cerr << "here" << std::endl;
bits::get(input_buffer,n,off+88,16);
std::cerr << "n=" << n << std::endl;
bits::get(input_buffer,n,off+104,16);
std::cerr << "p=" << n << std::endl;
bits::get(input_buffer,n,off+120,8);
bits::get(input_buffer,m,off+128,8);
bits::get(input_buffer,cnt,off+136,8);
std::cerr << "j,k,m=" << n << "," << m << "," << cnt << std::endl;
*/
	  }
	  else {
if (bms_included) {
myerror="unable to decode complex packing with bitmap defined";
exit(1);
}
	    short n1,ext_flg,n2,p1,p2;
	    bits::get(stream_buffer,n1,off+88,16);
	    bits::get(stream_buffer,ext_flg,off+104,8);
if ( (ext_flg & 0x20) == 0x20) {
myerror="unable to decode complex packing with secondary bitmap defined";
exit(1);
}
	    bits::get(stream_buffer,n2,off+112,16);
	    bits::get(stream_buffer,p1,off+128,16);
	    bits::get(stream_buffer,p2,off+144,16);
	    if ( (ext_flg & 0x8) == 0x8) {
// ECMWF extension for complex packing with spatial differencing
		short width_pack_width,length_pack_width,nl,order_vals_width;
		bits::get(stream_buffer,width_pack_width,off+168,8);
		bits::get(stream_buffer,length_pack_width,off+176,8);
		bits::get(stream_buffer,nl,off+184,16);
		bits::get(stream_buffer,order_vals_width,off+200,8);
		auto norder=(ext_flg & 0x3);
		std::unique_ptr<int []> first_vals;
		first_vals.reset(new int[norder]);
		bits::get(stream_buffer,first_vals.get(),off+208,order_vals_width,0,norder);
		short sign;
		int omin=0;
		auto noff=off+208+norder*order_vals_width;
		bits::get(stream_buffer,sign,noff,1);
		bits::get(stream_buffer,omin,noff+1,order_vals_width-1);
		if (sign == 1) {
		  omin=-omin;
		}
		noff+=order_vals_width;
		auto pad=(noff % 8);
		if (pad > 0) {
		  pad=8-pad;
		}
		noff+=pad;
		std::unique_ptr<int []> widths(new int[p1]),lengths(new int[p1]);
		bits::get(stream_buffer,widths.get(),noff,width_pack_width,0,p1);
		bits::get(stream_buffer,lengths.get(),off+(nl-1)*8,length_pack_width,0,p1);
		int max_length=0,lsum=0;
		for (auto n=0; n < p1; ++n) {
		  if (lengths[n] > max_length) {
		    max_length=lengths[n];
		  }
		  lsum+=lengths[n];
		}
		if (lsum != p2) {
		  myerror="inconsistent number of 2nd-order values - computed: "+strutils::itos(lsum)+" vs. encoded: "+strutils::itos(p2);
		  exit(1);
		}
		std::unique_ptr<int []> group_ref_vals(new int[p1]);
		bits::get(stream_buffer,group_ref_vals.get(),off+(n1-1)*8,8,0,p1);
		std::unique_ptr<int []> pvals(new int[max_length]);
		noff=off+(n2-1)*8;
		for (auto n=0; n < norder; ++n) {
		  auto j=n/g->dim.x;
		  auto i=(n % g->dim.x);
		  g->gridpoints_[j][i]=g->stats.min_val+first_vals[n]*e/d;
		  if (g->gridpoints_[j][i] > g->stats.max_val) {
		    g->stats.max_val=g->gridpoints_[j][i];
		  }
		  g->stats.avg_val+=g->gridpoints_[j][i];
		  ++avg_cnt;
		}
// unpack the field of differences
		for (int n=0,l=norder; n < p1; ++n) {
		  if (widths[n] > 0) {
		    bits::get(stream_buffer,pvals.get(),noff,widths[n],0,lengths[n]);
		    noff+=widths[n]*lengths[n];
 		    for (auto m=0; m < lengths[n]; ++m) {
			auto j=l/g->dim.x;
			auto i=(l % g->dim.x);
			g->gridpoints_[j][i]=pvals[m]+group_ref_vals[n]+omin;
			++l;
		    }
		  }
		  else {
// constant group
 		    for (auto m=0; m < lengths[n]; ++m) {
			auto j=l/g->dim.x;
			auto i=(l % g->dim.x);
			g->gridpoints_[j][i]=group_ref_vals[n]+omin;
			++l;
		    }
		  }
		}
		double lastgp;
		for (int n=norder-1; n > 0; --n) {
		  lastgp=first_vals[n]-first_vals[n-1];
		  for (size_t l=n+1; l < g->dim.size; ++l) {
		    auto j=l/g->dim.x;
		    auto i=(l % g->dim.x);
		    g->gridpoints_[j][i]+=lastgp;
		    lastgp=g->gridpoints_[j][i];
		  }
		}
		lastgp=g->stats.min_val*d/e+first_vals[norder-1];
		for (size_t l=norder; l < g->dim.size; ++l) {
		  auto j=l/g->dim.x;
		  auto i=(l % g->dim.x);
		  lastgp+=g->gridpoints_[j][i];
		  g->gridpoints_[j][i]=lastgp*e/d;
		  if (g->gridpoints_[j][i] > g->stats.max_val) {
		    g->stats.max_val=g->gridpoints_[j][i];
		  }
		  g->stats.avg_val+=g->gridpoints_[j][i];
		  ++avg_cnt;
		}
		if ((ext_flg & 0x2) == 0x2) {
// gridpoints_ are boustrophedonic
 		  for (auto j=0; j < g->dim.y; ++j) {
		    if ( (j % 2) == 1) {
			for (auto i=0,l=g->dim.x-1; i < g->dim.x/2; ++i,--l) {
			  auto temp=g->gridpoints_[j][i];
			  g->gridpoints_[j][i]=g->gridpoints_[j][l];
			  g->gridpoints_[j][l]=temp;
			}
		    }
		  }
		}
	    }
	    else {
		std::unique_ptr<short []> p2_widths;
		if ((ext_flg & 0x10) == 0x10) {
		  p2_widths.reset(new short[p1]);
		  bits::get(stream_buffer,p2_widths.get(),off+168,8,0,p1);
		}
		else {
		  p2_widths.reset(new short[1]);
		}
	    }
	  }
	}
	else {
	  myerror="complex unpacking not defined for bds_flag = "+strutils::itos(bds_flag);
	  exit(1);
	}
    }
    if (avg_cnt > 0) {
	g->stats.avg_val/=static_cast<double>(avg_cnt);
    }
  }
  curr_off+=lengths_.bds;
}

void GRIBMessage::unpack_end(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  int test;
  bits::get(stream_buffer,test,off,32);
  if (test != 0x37373737) {
    myerror="bad END Section - found *"+std::string(reinterpret_cast<char *>(const_cast<unsigned char *>(&stream_buffer[curr_off])),4)+"*";
    exit(1);
  }
}

void GRIBMessage::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  if (stream_buffer == nullptr) {
    myerror="empty file stream";
    exit(1);
  }
  clear_grids();
  unpack_is(stream_buffer);
  if (edition_ > 1) {
    myerror="can't decode edition "+strutils::itos(edition_);
    exit(1);
  }
  auto g=new GRIBGrid;
  g->grid.filled=false;
  grids.emplace_back(g);
  unpack_pds(stream_buffer);
  if (edition_ == 0) {
    mlength+=lengths_.pds;
  }
  if (gds_included) {
    unpack_gds(stream_buffer);
    if (edition_ == 0) {
	mlength+=lengths_.gds;
    }
  }
  else {
    static my::map<xmlutils::GridMapEntry> grid_map_table;
    xmlutils::GridMapEntry gme;
    gme.key=strutils::itos(g->grid.src)+"-"+strutils::itos(g->grib.sub_center);
    if (!grid_map_table.found(gme.key,gme)) {
	static TempDir *temp_dir=nullptr;
	if (temp_dir == nullptr) {
	  temp_dir=new TempDir;
	  if (!temp_dir->create("/tmp")) {
	    myerror="GRIBMessage::fill(): unable to create temporary directory";
	    exit(1);
	  }
	}
	gme.g.fill(xmlutils::map_filename("GridTables",gme.key,temp_dir->name()));
	grid_map_table.insert(gme);
    }
    auto catalog_id=strutils::itos(g->grib.grid_catalog_id);
    g->def=gme.g.grid_definition(catalog_id);
    g->dim=gme.g.grid_dimensions(catalog_id);
    if (gme.g.is_single_pole_point(catalog_id)) {
	g->grid.num_in_pole_sum=-std::stoi(gme.g.pole_point_location(catalog_id));
    }
    g->grib.scan_mode=0x40;
  }
  if (g->dim.x == 0 || g->dim.y == 0 || g->dim.size == 0) {
    mywarning="grid dimensions not defined";
  }
  if (bms_included) {
    unpack_bms(stream_buffer);
    if (edition_ == 0) {
	mlength+=lengths_.bms;
    }
  }
  unpack_bds(stream_buffer,fill_header_only);
  if (edition_ == 0) {
    mlength+=(lengths_.bds+4);
  }
  unpack_end(stream_buffer);
}

Grid *GRIBMessage::grid(size_t grid_number) const
{
  if (grids.size() == 0 || grid_number >= grids.size()) {
    return nullptr;
  }
  else {
    return grids[grid_number];
  }
}

void GRIBMessage::pds_supplement(unsigned char *pds_supplement,size_t& pds_supplement_length) const
{
  pds_supplement_length=lengths_.pds_supp;
  if (pds_supplement_length > 0) {
    for (size_t n=0; n < pds_supplement_length; ++n) {
	pds_supplement[n]=pds_supp[n];
    }
  }
  else {
    pds_supplement=nullptr;
  }
}

void GRIBMessage::initialize(short edition_number,unsigned char *pds_supplement,int pds_supplement_length,bool gds_is_included,bool bms_is_included)
{
  edition_=edition_number;
  switch (edition_) {
    case 0:
    {
	myerror="can't create GRIB0";
	exit(1);
    }
    case 1:
    {
	lengths_.is=8;
	break;
    }
    case 2:
    {
	lengths_.is=16;
	break;
    }
  }
  lengths_.pds_supp=pds_supplement_length;
  if (lengths_.pds_supp > 0) {
    pds_supp.reset(new unsigned char[lengths_.pds_supp]);
    std::copy(pds_supplement,pds_supplement+lengths_.pds_supp,pds_supp.get());
  }
  else {
    pds_supp.reset(nullptr);
  }
  gds_included=gds_is_included;
  bms_included=bms_is_included;
  clear_grids();
}

void GRIBMessage::print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  if (verbose) {
    outs << "  GRIB Ed: " << edition_ << "  Length: " << mlength << std::endl;
    if (lengths_.pds_supp > 0) {
	outs << "\n  Supplement to the PDS:";
	for (int n=0; n < lengths_.pds_supp; ++n) {
	  if (pds_supp[n] < 32 || pds_supp[n] > 127) {
	    outs << " \\" << std::setw(3) << std::setfill('0') << std::oct << static_cast<int>(pds_supp[n]) << std::setfill(' ') << std::dec;
	  }
	  else {
	    outs << " " << pds_supp[n];
	  }
	}
    }
    (reinterpret_cast<GRIBGrid *>(grids.front()))->print_header(outs,verbose,path_to_parameter_map);
  }
  else {
    outs << " Ed=" << edition_;
    (reinterpret_cast<GRIBGrid *>(grids.front()))->print_header(outs,verbose,path_to_parameter_map);
  }
}

void GRIB2Message::unpack_ids(const unsigned char *input_buffer)
{
  off_t off=curr_off*8;
  GRIB2Grid *g2=reinterpret_cast<GRIB2Grid *>(grids.back());
  offsets_.ids=curr_off;
  bits::get(input_buffer,lengths_.ids,off,32);
  short sec_num;
  bits::get(input_buffer,sec_num,off+32,8);
  if (sec_num != 1) {
    mywarning="may not be unpacking the Identification Section";
  }
  bits::get(input_buffer,g2->grid.src,off+40,16);
  bits::get(input_buffer,g2->grib.sub_center,off+56,16);
  bits::get(input_buffer,g2->grib.table,off+72,8);
  bits::get(input_buffer,g2->grib2.local_table,off+80,8);
  bits::get(input_buffer,g2->grib2.time_sig,off+88,8);
  short yr,mo,dy,hr,min,sec;
  bits::get(input_buffer,yr,off+96,16);
  bits::get(input_buffer,mo,off+112,8);
  bits::get(input_buffer,dy,off+120,8);
  bits::get(input_buffer,hr,off+128,8);
  bits::get(input_buffer,min,off+136,8);
  bits::get(input_buffer,sec,off+144,8);
  g2->reference_date_time_.set(yr,mo,dy,hr*10000+min*100+sec);
  bits::get(input_buffer,g2->grib2.prod_status,off+152,8);
  bits::get(input_buffer,g2->grib2.data_type,off+160,8);
  curr_off+=lengths_.ids;
}

void GRIB2Message::pack_ids(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  short hr,min,sec;
  GRIB2Grid *g2;

  g2=reinterpret_cast<GRIB2Grid *>(grid);
bits::set(output_buffer,21,off,32);
  bits::set(output_buffer,1,off+32,8);
  bits::set(output_buffer,g2->grid.src,off+40,16);
  bits::set(output_buffer,g2->grib.sub_center,off+56,16);
  bits::set(output_buffer,g2->grib.table,off+72,8);
  bits::set(output_buffer,g2->grib2.local_table,off+80,8);
  bits::set(output_buffer,g2->grib2.time_sig,off+88,8);
  bits::set(output_buffer,g2->reference_date_time_.year(),off+96,16);
  bits::set(output_buffer,g2->reference_date_time_.month(),off+112,8);
  bits::set(output_buffer,g2->reference_date_time_.day(),off+120,8);
  hr=g2->reference_date_time_.time()/10000;
  min=(g2->reference_date_time_.time()/100 % 100);
  sec=(g2->reference_date_time_.time() % 100);
  bits::set(output_buffer,hr,off+128,8);
  bits::set(output_buffer,min,off+136,8);
  bits::set(output_buffer,sec,off+144,8);
  bits::set(output_buffer,g2->grib2.prod_status,off+152,8);
  bits::set(output_buffer,g2->grib2.data_type,off+160,8);
offset+=21;
}

void GRIB2Message::unpack_lus(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;

  bits::get(stream_buffer,lus_len,off,32);
  curr_off+=lus_len;
}

void GRIB2Message::pack_lus(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
}

void GRIB2Message::unpack_pds(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short sec_num;
  unsigned char sign;
  short scale;
  int dum,noff,n,num_ranges;
  short yr,mo,dy,hr,min,sec;
  DateTime ddum;
  GRIB2Grid::StatisticalProcessRange stat_process_range;
  GRIB2Grid *g2;

  offsets_.pds=curr_off;
  g2=reinterpret_cast<GRIB2Grid *>(grids.back());
  bits::get(stream_buffer,lengths_.pds,off,32);
  bits::get(stream_buffer,sec_num,off+32,8);
  if (sec_num != 4) {
    mywarning="may not be unpacking the Product Description Section";
  }
  bits::get(stream_buffer,g2->grib2.num_coord_vals,off+40,16);
  g2->grid.fcst_time=0;
  g2->ensdata.fcst_type="";
  g2->grib2.stat_process_ranges.clear();
// template number
  bits::get(stream_buffer,g2->grib2.product_type,off+56,16);
  switch (g2->grib2.product_type) {
    case 0:
    case 1:
    case 2:
    case 8:
    case 11:
    case 12:
    case 15:
    case 61:
    {
	bits::get(stream_buffer,g2->grib2.param_cat,off+72,8);
	bits::get(stream_buffer,g2->grid.param,off+80,8);
	bits::get(stream_buffer,g2->grib.process,off+88,8);
	bits::get(stream_buffer,g2->grib2.backgen_process,off+96,8);
	bits::get(stream_buffer,g2->grib2.fcstgen_process,off+104,8);
	bits::get(stream_buffer,g2->grib.time_unit,off+136,8);
	bits::get(stream_buffer,g2->grid.fcst_time,off+144,32);
	bits::get(stream_buffer,g2->grid.level1_type,off+176,8);
	bits::get(stream_buffer,sign,off+192,1);
	bits::get(stream_buffer,dum,off+193,31);
	if (dum != 0x7fffffff || sign != 1) {
	  if (sign == 1) {
	    dum=-dum;
	  }
	  bits::get(stream_buffer,sign,off+184,1);
	  bits::get(stream_buffer,scale,off+185,7);
	  if (sign == 1) {
	    scale=-scale;
	  }
	  g2->grid.level1=dum/pow(10.,scale);
	  if (g2->grid.level1_type == 100 || g2->grid.level1_type == 108) {
	    g2->grid.level1/=100.;
	  }
	  else if (g2->grid.level1_type == 109) {
	    g2->grid.level1*=1000000000.;
	  }
	}
	else {
	  g2->grid.level1=Grid::missing_value;
	}
	bits::get(stream_buffer,g2->grid.level2_type,off+224,8);
	if (g2->grid.level2_type != 255) {
	  bits::get(stream_buffer,sign,off+240,1);
	  bits::get(stream_buffer,dum,off+241,31);
	  if (dum != 0x7fffffff || sign != 1) {
	    if (sign == 1) {
		dum=-dum;
	    }
	    bits::get(stream_buffer,sign,off+232,1);
	    bits::get(stream_buffer,scale,off+233,7);
	    if (sign == 1) {
		scale=-scale;
	    }
	    g2->grid.level2=dum/pow(10.,scale);
	    if (g2->grid.level2_type == 100 || g2->grid.level2_type == 108) {
		g2->grid.level2/=100.;
	    }
	    else if (g2->grid.level2_type == 109) {
		g2->grid.level2*=1000000000.;
	    }
	  }
	  else {
	    g2->grid.level2=Grid::missing_value;
	  }
	}
	else {
	  g2->grid.level2=Grid::missing_value;
	}
	switch (g2->grib2.product_type) {
	  case 1:
	  case 11:
	  case 61:
	  {
	    bits::get(stream_buffer,dum,off+272,8);
	    switch (dum) {
		case 0:
		{
		  g2->ensdata.fcst_type="HRCTL";
		  break;
		}
		case 1:
		{
		  g2->ensdata.fcst_type="LRCTL";
		  break;
		}
		case 2:
		{
		  g2->ensdata.fcst_type="NEG";
		  break;
		}
		case 3:
		{
		  g2->ensdata.fcst_type="POS";
		  break;
		}
		case 255:
		{
		  g2->ensdata.fcst_type="UNK";
		  break;
		}
		default:
		{
		  myerror="no ensemble type mapping for "+strutils::itos(dum);
		  exit(1);
		}
	    }
	    bits::get(stream_buffer,dum,off+280,8);
	    g2->ensdata.id=strutils::itos(dum);
	    bits::get(stream_buffer,g2->ensdata.total_size,off+288,8);
	    switch (g2->grib2.product_type) {
		case 61:
		{
		  bits::get(stream_buffer,yr,off+296,16);
		  bits::get(stream_buffer,mo,off+312,8);
		  bits::get(stream_buffer,dy,off+320,8);
		  bits::get(stream_buffer,hr,off+328,8);
		  bits::get(stream_buffer,min,off+336,8);
		  bits::get(stream_buffer,sec,off+344,8);
		  g2->grib2.modelv_date.set(yr,mo,dy,hr*10000+min*100+sec);
		  off+=56;
		}
		case 11:
		{
		  bits::get(stream_buffer,yr,off+296,16);
		  bits::get(stream_buffer,mo,off+312,8);
		  bits::get(stream_buffer,dy,off+320,8);
		  bits::get(stream_buffer,hr,off+328,8);
		  bits::get(stream_buffer,min,off+336,8);
		  bits::get(stream_buffer,sec,off+344,8);
		  g2->grib2.stat_period_end.set(yr,mo,dy,hr*10000+min*100+sec);
bits::get(stream_buffer,dum,off+352,8);
if (dum > 1) {
myerror="error processing multiple ("+strutils::itos(dum)+") statistical processes for product type "+strutils::itos(g2->grib2.product_type);
exit(1);
}
		  bits::get(stream_buffer,g2->grib2.stat_process_nmissing,off+360,32);
		  bits::get(stream_buffer,stat_process_range.type,off+392,8);
		  bits::get(stream_buffer,stat_process_range.time_increment_type,off+400,8);
		  bits::get(stream_buffer,stat_process_range.period_length.unit,off+408,8);
		  bits::get(stream_buffer,stat_process_range.period_length.value,off+416,32);
		  bits::get(stream_buffer,stat_process_range.period_time_increment.unit,off+448,8);
		  bits::get(stream_buffer,stat_process_range.period_time_increment.value,off+456,32);
		  g2->grib2.stat_process_ranges.emplace_back(stat_process_range);
		  break;
		}
	    }
	    break;
	  }
	  case 2:
	  case 12:
	  {
	    bits::get(stream_buffer,dum,off+272,8);
	    switch (dum) {
		case 0:
		{
		  g2->ensdata.fcst_type="UNWTD_MEAN";
		  break;
		}
		case 1:
		{
		  g2->ensdata.fcst_type="WTD_MEAN";
		  break;
		}
		case 2:
		{
		  g2->ensdata.fcst_type="STDDEV";
		  break;
		}
		case 3:
		{
		  g2->ensdata.fcst_type="STDDEV_NORMED";
		  break;
		}
		case 4:
		{
		  g2->ensdata.fcst_type="SPREAD";
		  break;
		}
		case 5:
		{
		  g2->ensdata.fcst_type="LRG_ANOM_INDEX";
		  break;
		}
		case 6:
		{
		  g2->ensdata.fcst_type="UNWTD_MEAN";
		  break;
		}
		default:
		{
		  myerror="no ensemble type mapping for "+strutils::itos(dum);
		  exit(1);
		}
	    }
	    g2->ensdata.id="ALL";
	    bits::get(stream_buffer,g2->ensdata.total_size,off+280,8);
	    switch (g2->grib2.product_type) {
		case 12:
		{
		  bits::get(stream_buffer,yr,off+288,16);
		  bits::get(stream_buffer,mo,off+304,8);
		  bits::get(stream_buffer,dy,off+312,8);
		  bits::get(stream_buffer,hr,off+320,8);
		  bits::get(stream_buffer,min,off+328,8);
		  bits::get(stream_buffer,sec,off+336,8);
		  g2->grib2.stat_period_end.set(yr,mo,dy,hr*10000+min*100+sec);
bits::get(stream_buffer,dum,off+344,8);
if (dum > 1) {
myerror="error processing multiple ("+strutils::itos(dum)+") statistical processes for product type "+strutils::itos(g2->grib2.product_type);
exit(1);
}
		  bits::get(stream_buffer,g2->grib2.stat_process_nmissing,off+352,32);
		  bits::get(stream_buffer,stat_process_range.type,off+384,8);
		  bits::get(stream_buffer,stat_process_range.time_increment_type,off+392,8);
		  bits::get(stream_buffer,stat_process_range.period_length.unit,off+400,8);
		  bits::get(stream_buffer,stat_process_range.period_length.value,off+408,32);
		  bits::get(stream_buffer,stat_process_range.period_time_increment.unit,off+440,8);
		  bits::get(stream_buffer,stat_process_range.period_time_increment.value,off+448,32);
		  g2->grib2.stat_process_ranges.emplace_back(stat_process_range);
		  break;
		}
	    }
	    break;
	  }
	  case 8:
	  {
	    bits::get(stream_buffer,yr,off+272,16);
	    bits::get(stream_buffer,mo,off+288,8);
	    bits::get(stream_buffer,dy,off+296,8);
	    bits::get(stream_buffer,hr,off+304,8);
	    bits::get(stream_buffer,min,off+312,8);
	    bits::get(stream_buffer,sec,off+320,8);
	    g2->grib2.stat_period_end.set(yr,mo,dy,hr*10000+min*100+sec);
	    bits::get(stream_buffer,num_ranges,off+328,8);
// patch
	    if (num_ranges == 0) {
		num_ranges=(lengths_.pds-46)/12;
	    }
	    bits::get(stream_buffer,g2->grib2.stat_process_nmissing,off+336,32);
/*
if (num_ranges > 1) {
myerror="error processing multiple ("+strutils::itos(num_ranges)+") statistical processes for product type "+strutils::itos(g2->grib2.product_type);
exit(1);
}
*/
	    noff=368;
	    for (n=0; n < num_ranges; n++) {
		bits::get(stream_buffer,stat_process_range.type,off+noff,8);
		bits::get(stream_buffer,stat_process_range.time_increment_type,off+noff+8,8);
		bits::get(stream_buffer,stat_process_range.period_length.unit,off+noff+16,8);
		bits::get(stream_buffer,stat_process_range.period_length.value,off+noff+24,32);
		bits::get(stream_buffer,stat_process_range.period_time_increment.unit,off+noff+56,8);
		bits::get(stream_buffer,stat_process_range.period_time_increment.value,off+noff+64,32);
		g2->grib2.stat_process_ranges.emplace_back(stat_process_range);
		noff+=96;
	    }
	    break;
	  }
	  case 15:
	  {
	    bits::get(stream_buffer,g2->grib2.spatial_process.stat_process,off+272,8);
	    bits::get(stream_buffer,g2->grib2.spatial_process.type,off+280,8);
	    bits::get(stream_buffer,g2->grib2.spatial_process.num_points,off+288,8);
	    break;
	  }
	}
	break;
    }
    default:
    {
	myerror="product template "+strutils::itos(g2->grib2.product_type)+" not recognized";
	exit(1);
    }
  }
  switch (g2->grib.time_unit) {
case 0:
{
break;
}
    case 1:
    {
	break;
    }
    case 3:
    {
	ddum=g2->reference_date_time_.months_added(g2->grid.fcst_time);
	g2->grid.fcst_time=ddum.hours_since(g2->reference_date_time_);
	break;
    }
    case 11:
    {
	g2->grid.fcst_time*=6;
	break;
    }
    case 12:
    {
	g2->grid.fcst_time*=12;
	break;
    }
    default:
    {
	myerror="unable to adjust valid time for time units "+strutils::itos(g2->grib.time_unit);
	exit(1);
    }
  }
/*
  if (g2->grid.src == 7 && g2->grib2.stat_process_ranges.size() > 1 && g2->grib2.stat_process_ranges[0].type == 255 && g2->grib2.stat_process_ranges[1].type != 255) {
    g2->grib2.stat_process_ranges[0].type=g2->grib2.stat_process_ranges[1].type;
  }
*/
/*
  if (g2->grib2.stat_process_ranges.size() > 0) {
    if (((g2->grib2.stat_process_ranges[0].type >= 0 && g2->grib2.stat_process_ranges[0].type <= 4) || g2->grib2.stat_process_ranges[0].type == 8 || (g2->grib2.stat_process_ranges[0].type == 255 && g2->grib2.stat_period_end.year() > 0))) {
//if (((g2->grib2.stat_process_ranges[0].type >= 0 && g2->grib2.stat_process_ranges[0].type <= 4) || g2->grib2.stat_process_ranges[0].type == 8)) {
	g2->valid_date_time_=g2->reference_date_time_.hours_added(g2->grid.fcst_time);
    }
    else if (g2->grib2.stat_process_ranges[0].type > 191) {
	g2->valid_date_time_=g2->reference_date_time_.hours_added(g2->grid.fcst_time);
	lastValidDateTime=g2->valid_date_time_;
	gributils::GRIB2_product_description(g2,g2->valid_date_time_,lastValidDateTime);
	if (lastValidDateTime != g2->valid_date_time_) {
	  g2->grib2.stat_period_end=lastValidDateTime;
	}
    }
  }
  else {
    g2->valid_date_time_=g2->reference_date_time_.hours_added(g2->grid.fcst_time);
  }
*/
if (g2->grib.time_unit == 0) {
g2->forecast_date_time_=g2->valid_date_time_=g2->reference_date_time_.minutes_added(g2->grid.fcst_time);
g2->grid.fcst_time*=100;
}
else {
  g2->forecast_date_time_=g2->valid_date_time_=g2->reference_date_time_.hours_added(g2->grid.fcst_time);
  g2->grid.fcst_time*=10000;
}
  g2->grib.prod_descr=gributils::grib2_product_description(g2,g2->forecast_date_time_,g2->valid_date_time_);
  curr_off+=lengths_.pds;
}

void GRIB2Message::pack_pds(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  size_t fcst_time;
  short hr,min,sec,scale;
  long long ldum;
  double lval;
  unsigned char sign;
  int noff,n;
  GRIB2Grid *g2;

  g2=reinterpret_cast<GRIB2Grid *>(grid);
  bits::set(output_buffer,4,off+32,8);
bits::set(output_buffer,0,off+40,16);
  fcst_time=g2->grid.fcst_time/10000;
// template number
  bits::set(output_buffer,g2->grib2.product_type,off+56,16);
  switch (g2->grib2.product_type) {
    case 0:
    case 1:
    case 2:
    case 8:
    case 11:
    case 12:
    {
	bits::set(output_buffer,g2->grib2.param_cat,off+72,8);
	bits::set(output_buffer,g2->grid.param,off+80,8);
	bits::set(output_buffer,g2->grib.process,off+88,8);
	bits::set(output_buffer,g2->grib2.backgen_process,off+96,8);
	bits::set(output_buffer,g2->grib2.fcstgen_process,off+104,8);
	bits::set(output_buffer,g2->grib.time_unit,off+136,8);
	bits::set(output_buffer,fcst_time,off+144,32);
	bits::set(output_buffer,g2->grid.level1_type,off+176,8);
	if (g2->grid.level1 == Grid::missing_value) {
	  bits::set(output_buffer,0xff,off+184,8);
	  bits::set(output_buffer,static_cast<size_t>(0xffffffff),off+192,32);
	}
	else {
	  lval=g2->grid.level1;
	  if (lval < 0.)
	    sign=1;
	  else
	    sign=0;
	  if (g2->grid.level1_type == 100 || g2->grid.level1_type == 108)
	    lval*=100.;
	  else if (g2->grid.level1_type == 109)
	    lval/=1000000000.;
	  scale=0;
	  if (floatutils::myequalf(lval,static_cast<long long>(lval),0.00000000001)) {
	    ldum=lval;
	    while (ldum > 0 && (ldum % 10) == 0) {
		++scale;
		ldum/=10;
	    }
	    bits::set(output_buffer,ldum,off+193,31);
	  }
	  else {
	    lval*=10.;
	    --scale;
	    while (!floatutils::myequalf(lval,llround(lval),0.00000000001)) {
		--scale;
		lval*=10.;
	    }
	    bits::set(output_buffer,llround(lval),off+193,31);
	  }
	  if (scale > 0) {
	    bits::set(output_buffer,1,off+184,1);
	  }
	  else {
	    bits::set(output_buffer,0,off+184,1);
	    scale=-scale;
	  }
	  bits::set(output_buffer,scale,off+185,7);
	  bits::set(output_buffer,sign,off+192,1);
	}
	bits::set(output_buffer,g2->grid.level2_type,off+224,8);
	if (g2->grid.level2 == Grid::missing_value) {
	  bits::set(output_buffer,0xff,off+232,8);
	  bits::set(output_buffer,static_cast<size_t>(0xffffffff),off+240,32);
	}
	else {
	  lval=g2->grid.level2;
	  if (lval < 0.) {
	    sign=1;
	  }
	  else {
	    sign=0;
	  }
	  if (g2->grid.level2_type == 100 || g2->grid.level2_type == 108) {
	    lval*=100.;
	  }
	  else if (g2->grid.level2_type == 109) {
	    lval/=1000000000.;
	  }
	  scale=0;
	  if (floatutils::myequalf(lval,static_cast<long long>(lval),0.00000000001)) {
	    ldum=lval;
	    while (ldum > 0 && (ldum % 10) == 0) {
		++scale;
		ldum/=10;
	    }
	    bits::set(output_buffer,ldum,off+241,31);
	  }
	  else {
	    lval*=10.;
	    --scale;
	    while (!floatutils::myequalf(lval,llround(lval),0.00000000001)) {
		--scale;
		lval*=10.;
	    }
	    bits::set(output_buffer,llround(lval),off+241,31);
	  }
	  if (scale > 0)
	    bits::set(output_buffer,1,off+232,1);
	  else {
	    bits::set(output_buffer,0,off+232,1);
	    scale=-scale;
	  }
	  bits::set(output_buffer,scale,off+233,7);
	  bits::set(output_buffer,sign,off+240,1);
	}
	switch (g2->grib2.product_type) {
	  case 0:
	  {
	    bits::set(output_buffer,34,off,32);
	    offset+=34;
	    break;
	  }
	  case 1:
	  case 11:
	  {
std::cerr << "unable to finish packing PDS for product type " << g2->grib2.product_type << std::endl;
	    break;
	  }
	  case 2:
	  case 12:
	  {
std::cerr << "unable to finish packing PDS for product type " << g2->grib2.product_type << std::endl;
	    break;
	  }
	  case 8:
	  {
	    bits::set(output_buffer,g2->grib2.stat_period_end.year(),off+272,16);
	    bits::set(output_buffer,g2->grib2.stat_period_end.month(),off+288,8);
	    bits::set(output_buffer,g2->grib2.stat_period_end.day(),off+296,8);
	    hr=g2->grib2.stat_period_end.time()/10000;
	    min=(g2->grib2.stat_period_end.time()/100 % 100);
	    sec=(g2->grib2.stat_period_end.time() % 100);
	    bits::set(output_buffer,hr,off+304,8);
	    bits::set(output_buffer,min,off+312,8);
	    bits::set(output_buffer,sec,off+320,8);
	    bits::set(output_buffer,g2->grib2.stat_process_ranges.size(),off+328,8);
	    bits::set(output_buffer,g2->grib2.stat_process_nmissing,off+336,32);
	    noff=368;
	    for (n=0; n < static_cast<int>(g2->grib2.stat_process_ranges.size()); n++) {
		bits::set(output_buffer,g2->grib2.stat_process_ranges[n].type,off+noff,8);
		bits::set(output_buffer,g2->grib2.stat_process_ranges[n].time_increment_type,off+noff+8,8);
		bits::set(output_buffer,g2->grib2.stat_process_ranges[n].period_length.unit,off+noff+16,8);
		bits::set(output_buffer,g2->grib2.stat_process_ranges[n].period_length.value,off+noff+24,32);
		bits::set(output_buffer,g2->grib2.stat_process_ranges[n].period_time_increment.unit,off+noff+56,8);
		bits::set(output_buffer,g2->grib2.stat_process_ranges[n].period_time_increment.value,off+noff+64,32);
		noff+=96;
	    }
	    bits::set(output_buffer,noff/8,off,32);
	    offset+=noff/8;
	    break;
	  }
	}
	break;
    }
  }
}

void GRIB2Message::unpack_gds(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short sec_num;
  int dum;
  unsigned char sign;
  size_t sdum;
  GRIB2Grid *g2;

  offsets_.gds=curr_off;
  g2=reinterpret_cast<GRIB2Grid *>(grids.back());
  bits::get(stream_buffer,lengths_.gds,off,32);
  bits::get(stream_buffer,sec_num,off+32,8);
  if (sec_num != 3) {
    mywarning="may not be unpacking the Grid Description Section";
  }
  bits::get(stream_buffer,dum,off+40,8);
if (dum != 0) {
myerror="grid has no template";
exit(1);
}
  bits::get(stream_buffer,g2->dim.size,off+48,32);
  bits::get(stream_buffer,g2->grib2.lp_width,off+80,8);
  if (g2->grib2.lp_width != 0)
    bits::get(stream_buffer,g2->grib2.lpi,off+88,8);
  else
    g2->grib2.lpi=0;
// template number
  bits::get(stream_buffer,g2->grid.grid_type,off+96,16);
  switch (g2->grid.grid_type) {
    case 0:
    {
	g2->def.type=Grid::latitudeLongitudeType;
	break;
    }
    case 10:
    {
	g2->def.type=Grid::mercatorType;
	break;
    }
    case 30:
    {
	g2->def.type=Grid::lambertConformalType;
	break;
    }
    case 40:
    {
	g2->def.type=Grid::gaussianLatitudeLongitudeType;
	break;
    }
    default:
    {
	myerror="no unpacking for grid type "+strutils::itos(g2->grid.grid_type);
	exit(1);
    }
  }
  switch (g2->grid.grid_type) {
    case 0:
    case 40:
    {
	bits::get(stream_buffer,g2->grib2.earth_shape,off+112,8);
	bits::get(stream_buffer,g2->dim.x,off+256,16);
	bits::get(stream_buffer,g2->dim.y,off+288,16);
	bits::get(stream_buffer,sdum,off+369,31);
	g2->def.slatitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+368,1);
	if (sign == 1) {
	  g2->def.slatitude=-(g2->def.slatitude);
	}
	bits::get(stream_buffer,sdum,off+401,31);
	g2->def.slongitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+400,1);
	if (sign == 1) {
	  g2->def.slongitude=-(g2->def.slongitude);
	}
	bits::get(stream_buffer,g2->grib.rescomp,off+432,8);
	bits::get(stream_buffer,sdum,off+441,31);
	g2->def.elatitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+440,1);
	if (sign == 1) {
	  g2->def.elatitude=-(g2->def.elatitude);
	}
	bits::get(stream_buffer,sdum,off+473,31);
	g2->def.elongitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+472,1);
	if (sign == 1) {
	  g2->def.elongitude=-(g2->def.elongitude);
	}
	bits::get(stream_buffer,sdum,off+504,32);
	g2->def.loincrement=sdum/1000000.;
	bits::get(stream_buffer,g2->def.num_circles,off+536,32);
	bits::get(stream_buffer,g2->grib.scan_mode,off+568,8);
	if (g2->grid.grid_type == 0) {
	  g2->def.laincrement=g2->def.num_circles/1000000.;
	}
	off=576;
	break;
    }
    case 10:
    {
	bits::get(stream_buffer,g2->grib2.earth_shape,off+112,8);
	bits::get(stream_buffer,g2->dim.x,off+256,16);
	bits::get(stream_buffer,g2->dim.y,off+288,16);
	bits::get(stream_buffer,sdum,off+305,31);
	g2->def.slatitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+304,1);
	if (sign == 1) {
	  g2->def.slatitude=-(g2->def.slatitude);
	}
	bits::get(stream_buffer,sdum,off+337,31);
	g2->def.slongitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+336,1);
	if (sign == 1) {
	  g2->def.slongitude=-(g2->def.slongitude);
	}
	bits::get(stream_buffer,g2->grib.rescomp,off+368,8);
	bits::get(stream_buffer,sdum,off+377,31);
	g2->def.stdparallel1=sdum/1000000.;
	bits::get(stream_buffer,sign,off+376,1);
	if (sign == 1) {
	  g2->def.stdparallel1=-(g2->def.stdparallel1);
	}
	bits::get(stream_buffer,sdum,off+409,31);
	g2->def.elatitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+408,1);
	if (sign == 1) {
	  g2->def.elatitude=-(g2->def.elatitude);
	}
	bits::get(stream_buffer,sdum,off+441,31);
	g2->def.elongitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+440,1);
	if (sign == 1) {
	  g2->def.elongitude=-(g2->def.elongitude);
	}
	bits::get(stream_buffer,g2->grib.scan_mode,off+472,8);
	bits::get(stream_buffer,sdum,off+512,32);
	g2->def.dx=sdum/1000000.;
	bits::get(stream_buffer,sdum,off+544,32);
	g2->def.dy=sdum/1000000.;
	break;
    }
    case 30:
    {
	bits::get(stream_buffer,g2->grib2.earth_shape,off+112,8);
	bits::get(stream_buffer,g2->dim.x,off+256,16);
	bits::get(stream_buffer,g2->dim.y,off+288,16);
	bits::get(stream_buffer,sdum,off+305,31);
	g2->def.slatitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+304,1);
	if (sign == 1) {
	  g2->def.slatitude=-(g2->def.slatitude);
	}
	bits::get(stream_buffer,sdum,off+337,31);
	g2->def.slongitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+336,1);
	if (sign == 1) {
	  g2->def.slongitude=-(g2->def.slongitude);
	}
	bits::get(stream_buffer,g2->grib.rescomp,off+368,8);
	bits::get(stream_buffer,sdum,off+377,31);
	g2->def.llatitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+376,1);
	if (sign == 1) {
	  g2->def.llatitude=-(g2->def.llatitude);
	}
	bits::get(stream_buffer,sdum,off+409,31);
	g2->def.olongitude=sdum/1000000.;
	bits::get(stream_buffer,sign,off+408,1);
	if (sign == 1) {
	  g2->def.olongitude=-(g2->def.olongitude);
	}
	bits::get(stream_buffer,sdum,off+440,32);
	g2->def.dx=sdum/1000000.;
	bits::get(stream_buffer,sdum,off+472,32);
	g2->def.dy=sdum/1000000.;
	bits::get(stream_buffer,g2->def.projection_flag,off+504,8);
	bits::get(stream_buffer,g2->grib.scan_mode,off+512,8);
	bits::get(stream_buffer,sdum,off+521,31);
	g2->def.stdparallel1=sdum/1000000.;
	bits::get(stream_buffer,sign,off+520,1);
	if (sign == 1) {
	  g2->def.stdparallel1=-(g2->def.stdparallel1);
	}
	bits::get(stream_buffer,sdum,off+553,31);
	g2->def.stdparallel2=sdum/1000000.;
	bits::get(stream_buffer,sign,off+552,1);
	if (sign == 1) {
	  g2->def.stdparallel2=-(g2->def.stdparallel2);
	}
	bits::get(stream_buffer,sdum,off+585,31);
	g2->grib.sp_lat=sdum/1000000.;
	bits::get(stream_buffer,sign,off+584,1);
	if (sign == 1) {
	  g2->grib.sp_lat=-(g2->grib.sp_lat);
	}
	bits::get(stream_buffer,sdum,off+617,31);
	g2->grib.sp_lon=sdum/1000000.;
	bits::get(stream_buffer,sign,off+616,1);
	if (sign == 1) {
	  g2->grib.sp_lon=-(g2->grib.sp_lon);
	}
	break;
    }
    default:
    {
	myerror="grid template "+strutils::itos(dum)+" not recognized";
	exit(1);
    }
  }
// reduced grids
  if (g2->dim.x < 0) {
    if (g2->plist != nullptr)
	delete[] g2->plist;
    g2->plist=new int[g2->dim.y];
    bits::get(stream_buffer,g2->plist,off,g2->grib2.lp_width*8,0,g2->dim.y);
    g2->def.loincrement=(g2->def.elongitude-g2->def.slongitude)/(g2->plist[g2->dim.y/2]-1);
  }
  g2->def=gridutils::fix_grid_definition(g2->def,g2->dim);
  curr_off+=lengths_.gds;
}

void GRIB2Message::pack_gds(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  long long sdum;
  GRIB2Grid *g2;

  g2=reinterpret_cast<GRIB2Grid *>(grid);
  bits::set(output_buffer,3,off+32,8);
bits::set(output_buffer,0,off+40,8);
  bits::set(output_buffer,g2->dim.size,off+48,32);
  bits::set(output_buffer,g2->grib2.lp_width,off+80,8);
  bits::set(output_buffer,g2->grib2.lpi,off+88,8);
// template number
  bits::set(output_buffer,g2->grid.grid_type,off+96,16);
  switch (g2->grid.grid_type) {
    case 0:
    case 40:
    {
	bits::set(output_buffer,g2->grib2.earth_shape,off+112,8);
	bits::set(output_buffer,0,off+240,16);
	bits::set(output_buffer,g2->dim.x,off+256,16);
	bits::set(output_buffer,0,off+272,16);
	bits::set(output_buffer,g2->dim.y,off+288,16);
bits::set(output_buffer,0,off+304,32);
bits::set(output_buffer,0xffffffff,off+336,32);
	sdum=g2->def.slatitude*1000000.;
	if (sdum < 0)
	  sdum=-sdum;
	bits::set(output_buffer,sdum,off+369,31);
	if (g2->def.slatitude < 0.)
	  bits::set(output_buffer,1,off+368,1);
	else
	  bits::set(output_buffer,0,off+368,1);
	sdum=g2->def.slongitude*1000000.;
	if (sdum < 0)
	  sdum+=360000000;
	bits::set(output_buffer,sdum,off+400,32);
	bits::set(output_buffer,g2->grib.rescomp,off+432,8);
	sdum=g2->def.elatitude*1000000.;
	if (sdum < 0)
	  sdum=-sdum;
	bits::set(output_buffer,sdum,off+441,31);
	if (g2->def.elatitude < 0.)
	  bits::set(output_buffer,1,off+440,1);
	else
	  bits::set(output_buffer,0,off+440,1);
	sdum=g2->def.elongitude*1000000.;
	if (sdum < 0)
	  sdum=-sdum;
	bits::set(output_buffer,sdum,off+473,31);
	if (g2->def.elongitude < 0.)
	  bits::set(output_buffer,1,off+472,1);
	else
	  bits::set(output_buffer,0,off+472,1);
//	sdum=(long long)(g2->def.loincrement*1000000.);
sdum=llround(g2->def.loincrement*1000000.);
	bits::set(output_buffer,sdum,off+504,32);
	if (g2->grid.grid_type == 0)
//	  sdum=(long long)(g2->def.laincrement*1000000.);
sdum=llround(g2->def.laincrement*1000000.);
	else
	  sdum=g2->def.num_circles;
	bits::set(output_buffer,sdum,off+536,32);
	bits::set(output_buffer,g2->grib.scan_mode,off+568,8);
	bits::set(output_buffer,72,off,32);
	offset+=72;
	break;
    }
    default:
    {
	myerror="unable to pack GDS for template "+strutils::itos(g2->grid.grid_type);
	exit(1);
    }
  }
}

void GRIB2Message::unpack_drs(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short sec_num;
  union {
    float dum;
    int idum;
  };
  GRIB2Grid *g2;
  int num_packed;

  offsets_.drs=curr_off;
  g2=reinterpret_cast<GRIB2Grid *>(grids.back());
  bits::get(stream_buffer,lengths_.drs,off,32);
  bits::get(stream_buffer,sec_num,off+32,8);
  if (sec_num != 5) {
    mywarning="may not be unpacking the Data Representation Section";
  }
  bits::get(stream_buffer,num_packed,off+40,32);
  g2->grid.num_missing=g2->dim.size-num_packed;
// template number
  bits::get(stream_buffer,g2->grib2.data_rep,off+72,16);
  switch (g2->grib2.data_rep) {
    case 0:
    case 2:
    case 3:
    case 40:
    case 40000:
    {
	bits::get(stream_buffer,idum,off+88,32);
	bits::get(stream_buffer,g2->grib.E,off+120,16);
	if (g2->grib.E > 0x8000)
	  g2->grib.E=0x8000-g2->grib.E;
	bits::get(stream_buffer,g2->grib.D,off+136,16);
	if (g2->grib.D > 0x8000)
	  g2->grib.D=0x8000-g2->grib.D;
	g2->stats.min_val=dum/pow(10.,g2->grib.D);
	bits::get(stream_buffer,g2->grib.pack_width,off+152,8);
	bits::get(stream_buffer,g2->grib2.orig_val_type,off+160,8);
	if (g2->grib2.data_rep == 2 || g2->grib2.data_rep == 3) {
	  bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.split_method,off+168,8);
	  bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.miss_val_mgmt,off+176,8);
	  if (g2->grib2.orig_val_type == 0) {
	    bits::get(stream_buffer,idum,off+184,32);
	    g2->grib2.complex_pack.grid_point.primary_miss_sub=dum;
	    bits::get(stream_buffer,idum,off+216,32);
	    g2->grib2.complex_pack.grid_point.secondary_miss_sub=dum;
	  }
	  else if (g2->grib2.orig_val_type == 1) {
	    bits::get(stream_buffer,idum,off+184,32);
	    g2->grib2.complex_pack.grid_point.primary_miss_sub=idum;
	    bits::get(stream_buffer,idum,off+216,32);
	    g2->grib2.complex_pack.grid_point.secondary_miss_sub=idum;
	  }
	  else {
	    myerror="unable to decode missing value substitutes for original value type of "+strutils::itos(g2->grib2.orig_val_type);
	    exit(1);
	  }
	  bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.num_groups,off+248,32);
	  bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.width_ref,off+280,8);
	  bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.width_pack_width,off+288,8);
	  bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.length_ref,off+296,32);
	  bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.length_incr,off+328,8);
	  bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.last_length,off+336,32);
	  bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.length_pack_width,off+368,8);
	  if (g2->grib2.data_rep == 3) {
	    bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.spatial_diff.order,off+376,8);
	    bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width,off+384,8);
	  }
	}
	break;
    }
    default:
    {
	myerror="data template "+strutils::itos(g2->grib2.data_rep)+" is not understood";
	exit(1);
    }
  }
  curr_off+=lengths_.drs;
}

void GRIB2Message::pack_drs(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  union {
    float dum;
    int idum;
  };
  GRIB2Grid *g2=reinterpret_cast<GRIB2Grid *>(grid);

  bits::set(output_buffer,5,off+32,8);
  bits::set(output_buffer,g2->dim.size-(g2->grid.num_missing),off+40,32);
// template number
  bits::set(output_buffer,g2->grib2.data_rep,off+72,16);
  switch (g2->grib2.data_rep) {
    case 0:
    case 2:
    case 3:
    case 40:
    case 40000:
    {
	dum=lroundf(g2->stats.min_val*pow(10.,g2->grib.D));
	bits::set(output_buffer,idum,off+88,32);
	idum=g2->grib.E;
	if (idum < 0)
	  idum=0x8000-idum;
	bits::set(output_buffer,idum,off+120,16);
	idum=g2->grib.D;
	if (idum < 0)
	  idum=0x8000-idum;
	bits::set(output_buffer,idum,off+136,16);
	bits::set(output_buffer,g2->grib.pack_width,off+152,8);
	bits::set(output_buffer,g2->grib2.orig_val_type,off+160,8);
	switch (g2->grib2.data_rep) {
	  case 0:
	  {
	    bits::set(output_buffer,21,off,32);
	    offset+=21;
	    break;
	  }
	  case 2:
	  case 3:
	  {
	    bits::set(output_buffer,g2->grib2.complex_pack.grid_point.split_method,off+168,8);
	    bits::set(output_buffer,g2->grib2.complex_pack.grid_point.miss_val_mgmt,off+176,8);
	    bits::set(output_buffer,23,off,32);
	    offset+=23;
	    break;
	  }
	  case 40:
	  {
	    bits::set(output_buffer,0,off+168,8);
	    bits::set(output_buffer,0xff,off+176,8);
	    bits::set(output_buffer,23,off,32);
	    offset+=23;
	    break;
	  }
	}
	break;
    }
    default:
    {
	myerror="unable to pack data template "+strutils::itos(g2->grib2.data_rep);
	exit(1);
    }
  }
}

void GRIB2Message::unpack_ds(const unsigned char *stream_buffer,bool fill_header_only)
{
  off_t off=curr_off*8;
  short sec_num;
  size_t avg_cnt=0,pval=0;
  int len;
  int *jvals,num_packed=0;
  GRIB2Grid *g2=reinterpret_cast<GRIB2Grid *>(grids.back());
  struct {
    int *ref_vals,*widths,*lengths,*pvals;
    short sign;
    int *first_vals,omin;
    long long miss_val,group_miss_val;
    int max_length;
  } groups;
  int pad,i,j;
  double D=pow(10.,g2->grib.D);
  double E=pow(2.,g2->grib.E);

  offsets_.ds=curr_off;
  bits::get(stream_buffer,lengths_.ds,off,32);
  bits::get(stream_buffer,sec_num,off+32,8);
  if (sec_num != 7) {
    mywarning="may not be unpacking the Data Section";
  }
  if (!fill_header_only) {
    g2->stats.max_val=-Grid::missing_value;
    switch (g2->grib2.data_rep) {
	case 0:
	{
	  g2->galloc();
	  off+=40;
	  size_t x=0;
	  for (int n=0; n < g2->dim.y; ++n) {
	    for (int m=0; m < g2->dim.x; ++m) {
		if (!g2->bitmap.applies || g2->bitmap.map[x] == 1) {
		  bits::get(stream_buffer,pval,off,g2->grib.pack_width);
		  num_packed++;
		  g2->gridpoints_[n][m]=g2->stats.min_val+pval*E/D;
		  if (g2->gridpoints_[n][m] > g2->stats.max_val) {
		    g2->stats.max_val=g2->gridpoints_[n][m];
		  }
		  g2->stats.avg_val+=g2->gridpoints_[n][m];
		  avg_cnt++;
		  off+=g2->grib.pack_width;
		}
		else {
		  g2->gridpoints_[n][m]=Grid::missing_value;
		}
		x++;
	    }
	  }
	  g2->stats.avg_val/=static_cast<double>(avg_cnt);
	  g2->grid.filled=true;
	  break;
	}
	case 2:
	{
	  if (g2->grib.scan_mode != 0) {
	    myerror="unable to decode ddef2 for scan mode "+strutils::itos(g2->grib.scan_mode);
	    exit(1);
	  }
	  g2->galloc();
	  if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
	    groups.miss_val=pow(2.,g2->grib.pack_width)-1;
	  }
	  else {
	    groups.miss_val=Grid::missing_value;
	  }
	  off+=40;
	  groups.ref_vals=new int[g2->grib2.complex_pack.grid_point.num_groups];
	  bits::get(stream_buffer,groups.ref_vals,off,g2->grib.pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	  off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib.pack_width;
	  if ( (pad=(off % 8)) > 0) {
	    off+=8-pad;
	  }
	  groups.widths=new int[g2->grib2.complex_pack.grid_point.num_groups];
	  bits::get(stream_buffer,groups.widths,off,g2->grib2.complex_pack.grid_point.width_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	  off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.width_pack_width;
	  if ( (pad=(off % 8)) > 0) {
	    off+=8-pad;
	  }
	  groups.lengths=new int[g2->grib2.complex_pack.grid_point.num_groups];
	  bits::get(stream_buffer,groups.lengths,off,g2->grib2.complex_pack.grid_point.length_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	  off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.length_pack_width;
	  if ( (pad=(off % 8)) > 0) {
	    off+=8-pad;
	  }
	  groups.max_length=0;
	  for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
	    groups.lengths[n]=g2->grib2.complex_pack.grid_point.length_ref+groups.lengths[n]*g2->grib2.complex_pack.grid_point.length_incr;
	    if (groups.lengths[n] > groups.max_length) {
		groups.max_length=groups.lengths[n];
	    }
	  }
	  groups.pvals=new int[groups.max_length];
	  size_t gridpoint_index=0;
	  for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
	    if (groups.widths[n] > 0) {
		if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
		  groups.group_miss_val=pow(2.,groups.widths[n])-1;
		}
		else {
		  groups.group_miss_val=Grid::missing_value;
		}
		bits::get(stream_buffer,groups.pvals,off,groups.widths[n],0,groups.lengths[n]);
		off+=groups.widths[n]*groups.lengths[n];
		for (int m=0; m < groups.lengths[n]; ++m) {
		  j=gridpoint_index/g2->dim.x;
		  i=(gridpoint_index % g2->dim.x);
		  if (groups.pvals[m] == groups.group_miss_val || (g2->bitmap.applies && g2->bitmap.map[gridpoint_index] == 0)) {
		    g2->gridpoints_[j][i]=Grid::missing_value;
		  }
		  else {
		    g2->gridpoints_[j][i]=g2->stats.min_val+(groups.pvals[m]+groups.ref_vals[n])*E/D;
		    if (g2->gridpoints_[j][i] > g2->stats.max_val) {
			g2->stats.max_val=g2->gridpoints_[j][i];
		    }
		    g2->stats.avg_val+=g2->gridpoints_[j][i];
		    ++avg_cnt;
		  }
		  ++gridpoint_index;
		}
	    }
	    else {
// constant group
		for (int m=0; m < groups.lengths[n]; ++m) {
		  j=gridpoint_index/g2->dim.x;
		  i=(gridpoint_index % g2->dim.x);
		  if (groups.ref_vals[n] == groups.miss_val || (g2->bitmap.applies && g2->bitmap.map[gridpoint_index] == 0)) {
		    g2->gridpoints_[j][i]=Grid::missing_value;
		  }
		  else {
		    g2->gridpoints_[j][i]=g2->stats.min_val+groups.ref_vals[n]*E/D;
		    if (g2->gridpoints_[j][i] > g2->stats.max_val) {
			g2->stats.max_val=g2->gridpoints_[j][i];
		    }
		    g2->stats.avg_val+=g2->gridpoints_[j][i];
		    ++avg_cnt;
		  }
		  ++gridpoint_index;
		}
	    }
	  }
	  for (; gridpoint_index < g2->dim.size; ++gridpoint_index) {
	    j=gridpoint_index/g2->dim.x;
	    i=(gridpoint_index % g2->dim.x);
	    g2->gridpoints_[j][i]=Grid::missing_value;
	  }
	  g2->stats.avg_val/=static_cast<double>(avg_cnt);
	  g2->grid.filled=true;
	  break;
	}
	case 3:
	{
	  if (g2->grib.scan_mode != 0) {
	    myerror="unable to decode ddef3 for scan mode "+strutils::itos(g2->grib.scan_mode);
	    exit(1);
	  }
	  g2->galloc();
	  if (g2->grib2.complex_pack.grid_point.num_groups > 0) {
	    if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
		groups.miss_val=pow(2.,g2->grib.pack_width)-1;
	    }
	    else {
		groups.miss_val=Grid::missing_value;
	    }
	    off+=40;
	    groups.first_vals=new int[g2->grib2.complex_pack.grid_point.spatial_diff.order];
	    for (int n=0; n < g2->grib2.complex_pack.grid_point.spatial_diff.order; ++n) {
		bits::get(stream_buffer,groups.first_vals[n],off,g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8);
		off+=g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8;
	    }
	    bits::get(stream_buffer,groups.sign,off,1);
	    bits::get(stream_buffer,groups.omin,off+1,g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8-1);
	    if (groups.sign == 1) {
		groups.omin=-groups.omin;
	    }
	    off+=g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8;
	    groups.ref_vals=new int[g2->grib2.complex_pack.grid_point.num_groups];
	    bits::get(stream_buffer,groups.ref_vals,off,g2->grib.pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	    off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib.pack_width;
	    if ( (pad=(off % 8)) > 0) {
		off+=8-pad;
	    }
	    groups.widths=new int[g2->grib2.complex_pack.grid_point.num_groups];
	    bits::get(stream_buffer,groups.widths,off,g2->grib2.complex_pack.grid_point.width_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	    off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.width_pack_width;
	    if ( (pad=(off % 8)) > 0) {
		off+=8-pad;
	    }
	    groups.lengths=new int[g2->grib2.complex_pack.grid_point.num_groups];
	    bits::get(stream_buffer,groups.lengths,off,g2->grib2.complex_pack.grid_point.length_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	    off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.length_pack_width;
	    if ( (pad=(off % 8)) > 0) {
		off+=8-pad;
	    }
	    groups.max_length=0;
	    int end=g2->grib2.complex_pack.grid_point.num_groups-1;
	    for (int n=0; n < end; ++n) {
		groups.lengths[n]=g2->grib2.complex_pack.grid_point.length_ref+groups.lengths[n]*g2->grib2.complex_pack.grid_point.length_incr;
		if (groups.lengths[n] > groups.max_length) {
		  groups.max_length=groups.lengths[n];
		}
	    }
	    groups.lengths[end]=g2->grib2.complex_pack.grid_point.last_length;
	    if (groups.lengths[end] > groups.max_length) {
		groups.max_length=groups.lengths[end];
	    }
	    groups.pvals=new int[groups.max_length];
// unpack the field of differences
	    size_t gridpoint_index=0;
	    for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
		if (groups.widths[n] > 0) {
		  if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
		    groups.group_miss_val=pow(2.,groups.widths[n])-1;
		  }
		  else {
		    groups.group_miss_val=Grid::missing_value;
		  }
		  bits::get(stream_buffer,groups.pvals,off,groups.widths[n],0,groups.lengths[n]);
		  off+=groups.widths[n]*groups.lengths[n];
		  for (int m=0; m < groups.lengths[n]; ++m) {
		    j=gridpoint_index/g2->dim.x;
		    i=(gridpoint_index % g2->dim.x);
		    if ((g2->bitmap.applies && g2->bitmap.map[gridpoint_index] == 0) || groups.pvals[m] == groups.group_miss_val) {
			g2->gridpoints_[j][i]=Grid::missing_value;
		    }
		    else {
			g2->gridpoints_[j][i]=groups.pvals[m]+groups.ref_vals[n]+groups.omin;
		    }
		    ++gridpoint_index;
		  }
		}
		else {
// constant group
		  for (int m=0; m < groups.lengths[n]; ++m) {
		    j=gridpoint_index/g2->dim.x;
		    i=(gridpoint_index % g2->dim.x);
		    if ((g2->bitmap.applies && g2->bitmap.map[gridpoint_index] == 0) || groups.ref_vals[n] == groups.miss_val) {
			g2->gridpoints_[j][i]=Grid::missing_value;
		    }
		    else {
			g2->gridpoints_[j][i]=groups.ref_vals[n]+groups.omin;
		    }
		    ++gridpoint_index;
		  }
		}
	    }
	    for (; gridpoint_index < g2->dim.size; ++gridpoint_index) {
		j=gridpoint_index/g2->dim.x;
		i=(gridpoint_index % g2->dim.x);
		g2->gridpoints_[j][i]=Grid::missing_value;
	    }
	    for (int n=g2->grib2.complex_pack.grid_point.spatial_diff.order-1; n > 0; --n) {
		auto lastgp=groups.first_vals[n]-groups.first_vals[n-1];
		int num_not_missing=0;
		for (size_t l=0; l < g2->dim.size; ++l) {
		  j=l/g2->dim.x;
		  i=(l % g2->dim.x);
		  if (g2->gridpoints_[j][i] != Grid::missing_value) {
		    if (num_not_missing >= g2->grib2.complex_pack.grid_point.spatial_diff.order) {
			g2->gridpoints_[j][i]+=lastgp;
			lastgp=g2->gridpoints_[j][i];
		    }
		    ++num_not_missing;
		  }
		}
	    }
	    double lastgp=0.;
	    int num_not_missing=0;
	    for (size_t l=0; l < g2->dim.size; ++l) {
		j=l/g2->dim.x;
		i=(l % g2->dim.x);
		if (g2->gridpoints_[j][i] != Grid::missing_value) {
		  if (num_not_missing < g2->grib2.complex_pack.grid_point.spatial_diff.order) {
		    g2->gridpoints_[j][i]=g2->stats.min_val+groups.first_vals[num_not_missing]/D*E;
		    lastgp=g2->stats.min_val*D/E+groups.first_vals[num_not_missing];
		  }
		  else {
		    lastgp+=g2->gridpoints_[j][i];
		    g2->gridpoints_[j][i]=lastgp*E/D;
		  }
		  if (g2->gridpoints_[j][i] > g2->stats.max_val) {
		    g2->stats.max_val=g2->gridpoints_[j][i];
		  }
		  g2->stats.avg_val+=g2->gridpoints_[j][i];
		  ++avg_cnt;
		  ++num_not_missing;
		}
	    }
	    delete[] groups.pvals;
	    delete[] groups.ref_vals;
	    delete[] groups.widths;
	    delete[] groups.lengths;
	    delete[] groups.first_vals;
	    g2->stats.avg_val/=static_cast<double>(avg_cnt);
	  }
	  else {
	    for (int n=0; n < g2->dim.y; ++n) {
		for (int m=0; m < g2->dim.x; ++m) {
		  g2->gridpoints_[n][m]=Grid::missing_value;
		}
	    }
	    g2->stats.min_val=g2->stats.max_val=g2->stats.avg_val=Grid::missing_value;
	  }
	  g2->grid.filled=true;
	  break;
	}
#ifdef __JASPER
	case 40:
	case 40000:
	{
	  len=lengths_.ds-5;
	  jvals=new int[g2->dim.size];
	  if (len > 0) {
	    if (gributils::dec_jpeg2000(reinterpret_cast<char *>(const_cast<unsigned char *>(&stream_buffer[curr_off+5])),len,jvals) < 0) {
		for (size_t n=0; n < g2->dim.size; ++n) {
		  jvals[n]=0;
		}
	    }
	  }
	  else {
	    for (size_t n=0; n < g2->dim.size; ++n) {
		jvals[n]=0;
	    }
	  }
	  g2->galloc();
	  size_t x=0,y=0;
	  for (int n=0; n < g2->dim.y; ++n) {
	    for (int m=0; m < g2->dim.x; ++m) {
		if (!g2->bitmap.applies || g2->bitmap.map[x] == 1) {
		  g2->gridpoints_[n][m]=g2->stats.min_val+jvals[y++]*E/D;
		  if (g2->gridpoints_[n][m] > g2->stats.max_val) {
		    g2->stats.max_val=g2->gridpoints_[n][m];
		  }
		  g2->stats.avg_val+=g2->gridpoints_[n][m];
		  ++avg_cnt;
		}
		else {
		  g2->gridpoints_[n][m]=Grid::missing_value;
		}
		++x;
	    }
	  }
	  delete[] jvals;
	  g2->stats.avg_val/=static_cast<double>(avg_cnt);
	  g2->grid.filled=true;
	  break;
	}
#endif
	default:
	{
	  myerror="unable to decode data section for data representation "+strutils::itos(g2->grib2.data_rep);
	  exit(1);
	}
    }
  }
  curr_off+=lengths_.ds;
}

#ifdef __JASPER
int enc_jpeg2000(unsigned char *cin,int *pwidth,int *pheight,int *pnbits,
                 int *ltype, int *ratio, int *retry, char *outjpc, 
                 int *jpclen)
/*$$$  SUBPROGRAM DOCUMENTATION BLOCK
*                .      .    .                                       .
* SUBPROGRAM:    enc_jpeg2000      Encodes JPEG2000 code stream
*   PRGMMR: Gilbert          ORG: W/NP11     DATE: 2002-12-02
*
* ABSTRACT: This Function encodes a grayscale image into a JPEG2000 code stream
*   specified in the JPEG2000 Part-1 standard (i.e., ISO/IEC 15444-1) 
*   using JasPer Software version 1.500.4 (or 1.700.2 ) written by the 
*   University of British Columbia, Image Power Inc, and others.
*   JasPer is available at http://www.ece.uvic.ca/~mdadams/jasper/.
*
* PROGRAM HISTORY LOG:
* 2002-12-02  Gilbert
* 2004-07-20  GIlbert - Added retry argument/option to allow option of
*                       increasing the maximum number of guard bits to the
*                       JPEG2000 algorithm.
*
* USAGE:    int enc_jpeg2000(unsigned char *cin,g2int *pwidth,g2int *pheight,
*                            g2int *pnbits, g2int *ltype, g2int *ratio, 
*                            g2int *retry, char *outjpc, g2int *jpclen)
*
*   INPUT ARGUMENTS:
*      cin   - Packed matrix of Grayscale image values to encode.
*    pwidth  - Pointer to width of image
*    pheight - Pointer to height of image
*    pnbits  - Pointer to depth (in bits) of image.  i.e number of bits
*              used to hold each data value
*    ltype   - Pointer to indicator of lossless or lossy compression
*              = 1, for lossy compression
*              != 1, for lossless compression
*    ratio   - Pointer to target compression ratio.  (ratio:1)
*              Used only when *ltype == 1.
*    retry   - Pointer to option type.
*              1 = try increasing number of guard bits
*              otherwise, no additional options
*    jpclen  - Number of bytes allocated for new JPEG2000 code stream in
*              outjpc.
*
*   INPUT ARGUMENTS:
*     outjpc - Output encoded JPEG2000 code stream
*
*   RETURN VALUES :
*        > 0 = Length in bytes of encoded JPEG2000 code stream
*         -3 = Error decode jpeg2000 code stream.
*         -5 = decoded image had multiple color components.
*              Only grayscale is expected.
*
* REMARKS:
*
*      Requires JasPer Software version 1.500.4 or 1.700.2
*
* ATTRIBUTES:
*   LANGUAGE: C
*   MACHINE:  IBM SP
*
*$$$*/
{
    int ier,rwcnt;
    jas_image_t image;
    jas_stream_t *jpcstream,*istream;
    jas_image_cmpt_t cmpt,*pcmpt;
    const int MAXOPTSSIZE=1024;
    char opts[MAXOPTSSIZE];
    int width,height,nbits;

    width=*pwidth;
    height=*pheight;
    nbits=*pnbits;
// Set lossy compression options, if requested.
    if ( *ltype != 1 ) {
       opts[0]='\0';
    }
    else {
       snprintf(opts,MAXOPTSSIZE,"mode=real\nrate=%f",1.0/(*ratio));
    }
    if ( *retry == 1 ) {             // option to increase number of guard bits
       strcat(opts,"\nnumgbits=4");
    }
    //printf("SAGopts: %s\n",opts);
// Initialize the JasPer image structure describing the grayscale image to
// encode into the JPEG2000 code stream.
    image.tlx_=0;
    image.tly_=0;
    image.brx_=static_cast<jas_image_coord_t>(width);
    image.bry_=static_cast<jas_image_coord_t>(height);
    image.numcmpts_=1;
    image.maxcmpts_=1;
    image.clrspc_=JAS_CLRSPC_SGRAY;
    image.cmprof_=0; 
    image.inmem_=1;
    cmpt.tlx_=0;
    cmpt.tly_=0;
    cmpt.hstep_=1;
    cmpt.vstep_=1;
    cmpt.width_=static_cast<jas_image_coord_t>(width);
    cmpt.height_=static_cast<jas_image_coord_t>(height);
    cmpt.type_=JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y);
    cmpt.prec_=nbits;
    cmpt.sgnd_=0;
    cmpt.cps_=(nbits+7)/8;
    pcmpt=&cmpt;
    image.cmpts_=&pcmpt;
// Open a JasPer stream containing the input grayscale values
    istream=jas_stream_memopen(reinterpret_cast<char *>(cin),height*width*cmpt.cps_);
    cmpt.stream_=istream;
// Open an output stream that will contain the encoded jpeg2000 code stream.
    jpcstream=jas_stream_memopen(outjpc,static_cast<int>(*jpclen));
// Encode image.
    ier=jpc_encode(&image,jpcstream,opts);
    if ( ier != 0 ) {
       fprintf(stderr," jpc_encode return = %d \n",ier);
       return -3;
    }
// Clean up JasPer work structures.
    rwcnt=jpcstream->rwcnt_;
    ier=jas_stream_close(istream);
    ier=jas_stream_close(jpcstream);
// Return size of jpeg2000 code stream
    return (rwcnt);
}
#endif

void GRIB2Message::pack_ds(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  int noff,jval,len;
  int n,m,x=0,y=0,cps,bps;
  unsigned char *cin;
  int pwidth,pheight,pnbits,ltype=0,ratio=0,retry=1,jpclen;
  GRIB2Grid *g2=reinterpret_cast<GRIB2Grid *>(grid);
  double d=pow(10.,g2->grib.D),e=pow(2.,g2->grib.E);
  int *pval,cnt=0;

  bits::set(output_buffer,7,off+32,8);
  switch (g2->grib2.data_rep) {
    case 0:
    {
	pval=new int[g2->dim.size];
	for (n=0; n < g2->dim.y; n++) {
	  for (m=0; m < g2->dim.x; m++) {
	    if (!g2->bitmap.applies || g2->bitmap.map[x] == 1)
		pval[cnt++]=static_cast<int>(lround((g2->gridpoints_[n][m]-g2->stats.min_val)*pow(10.,g2->grib.D))/pow(2.,g2->grib.E));
	  }
	}
	bits::set(&output_buffer[off/8+5],pval,0,g2->grib.pack_width,0,cnt);
	len=(cnt*g2->grib.pack_width+7)/8;
	bits::set(output_buffer,len+5,off,32);
	offset+=(len+5);
	delete[] pval;
	break;
    }
#ifdef __JASPER
    case 40:
    case 40000:
    {
	cps=(g2->grib.pack_width+7)/8;
	jpclen=g2->dim.size*cps;
// for some unknown reason, enc_jpeg2000 fails if jpclen is too small
if (jpclen < 100000)
jpclen=100000;
	bps=cps*8;
	cin=new unsigned char[jpclen];
	for (n=0; n < g2->dim.y; n++) {
	  for (m=0; m < g2->dim.x; m++) {
	    if (!g2->bitmap.applies || g2->bitmap.map[x] == 1) {
		if (g2->gridpoints_[n][m] == Grid::missing_value)
		  jval=0;
		else
		  jval=lround((lround(g2->gridpoints_[n][m]*d)-lround(g2->stats.min_val*d))/e);
		bits::set(cin,jval,y*bps,bps);
		y++;
	    }
	    x++;
	  }
	}
	if (g2->bitmap.applies) {
	  pwidth=y;
	  pheight=1;
	}
	else {
	  pwidth=g2->dim.x;
	  pheight=g2->dim.y;
	}
	if (pwidth > 0) {
	  pnbits=g2->grib.pack_width;
	  noff=off/8+5;
	  len=enc_jpeg2000(cin,&pwidth,&pheight,&pnbits,&ltype,&ratio,&retry,reinterpret_cast<char *>(&output_buffer[noff]),&jpclen);
	}
	else
	  len=0;
	delete[] cin;
	bits::set(output_buffer,len+5,off,32);
	offset+=(len+5);
	break;
    }
#endif
    default:
    {
	myerror="unable to encode data section for data representation "+strutils::itos(g2->grib2.data_rep);
	exit(1);
    }
  }
}

off_t GRIB2Message::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length)
{
  off_t offset;

  pack_is(output_buffer,offset,grids.front());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  pack_ids(output_buffer,offset,grids.front());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  for (auto grid : grids) {
    pack_lus(output_buffer,offset,grid);
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copy_to_buffer()";
	exit(1);
    }
    if (gds_included) {
	pack_gds(output_buffer,offset,grid);
	if (offset > static_cast<off_t>(buffer_length)) {
	  myerror="buffer overflow in copy_to_buffer()";
	  exit(1);
	}
    }
    pack_pds(output_buffer,offset,grid);
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copy_to_buffer()";
	exit(1);
    }
    pack_drs(output_buffer,offset,grid);
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copy_to_buffer()";
	exit(1);
    }
    pack_bms(output_buffer,offset,grid);
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copy_to_buffer()";
	exit(1);
    }
    pack_ds(output_buffer,offset,grid);
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copy_to_buffer()";
	exit(1);
    }
  }
  pack_end(output_buffer,offset);
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  mlength=offset;
  pack_length(output_buffer);
  return mlength;
}

void GRIB2Message::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int test,len;
  short sec_num,last_sec_num=99;
  GRIB2Grid *g2;

  if (stream_buffer == nullptr) {
    myerror="empty file stream";
    exit(1);
  }
  clear_grids();
  unpack_is(stream_buffer);
  if (edition_ != 2) {
    myerror="can't decode edition "+strutils::itos(edition_);
    exit(1);
  }
  bits::get(stream_buffer,test,curr_off*8,32);
  while (test != 0x37373737) {
// patch for bad END section
    if ( (curr_off+4) == mlength) {
	mywarning="bad END section repaired";
	test=0x37373737;
	break;
    }
    bits::get(stream_buffer,len,curr_off*8,32);
    bits::get(stream_buffer,sec_num,curr_off*8+32,8);
// patch for bad grid
    if (sec_num < 1 || sec_num > 7 || len > mlength) {
	mywarning="bad grid ignored - offset: "+strutils::itos(curr_off)+", sec_num: "+strutils::itos(sec_num)+", len: "+strutils::itos(len)+", length: "+strutils::itos(mlength);
	test=0x37373737;
	break;
    }
    else {
	if (sec_num < last_sec_num) {
	  g2=new GRIB2Grid;
	  if (grids.size() > 0) {
	    g2->quick_copy(*(reinterpret_cast<GRIB2Grid *>(grids.back())));
	  }
	  bits::get(stream_buffer,discipline,48,8);
	  g2->grib2.discipline=discipline;
	  g2->grid.filled=false;
	  grids.emplace_back(g2);
	}
	last_sec_num=sec_num;
	switch (sec_num) {
	  case 1:
	  {
	    unpack_ids(stream_buffer);
	    break;
	  }
	  case 2:
	  {
	    unpack_lus(stream_buffer);
	    break;
	  }
	  case 3:
	  {
	    unpack_gds(stream_buffer);
	    break;
	  }
	  case 4:
	  {
	    unpack_pds(stream_buffer);
	    break;
	  }
	  case 5:
	  {
	    unpack_drs(stream_buffer);
	    break;
	  }
	  case 6:
	  {
	    unpack_bms(stream_buffer);
	    break;
	  }
	  case 7:
	  {
	    unpack_ds(stream_buffer,fill_header_only);
	    break;
	  }
	}
    }
    bits::get(stream_buffer,test,curr_off*8,32);
  }
}

void GRIB2Message::print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  int n=0;

  if (verbose) {
    outs << "  GRIB Ed: " << edition_ << "  Length: " << mlength << "  Number of Grids: " << grids.size() << std::endl;
    for (auto grid : grids) {
	outs << "  Grid #" << ++n << ":" << std::endl;
	(reinterpret_cast<GRIB2Grid *>(grid))->print_header(outs,verbose,path_to_parameter_map);
    }
  }
  else {
    outs << " Ed=" << edition_ << " NGrds=" << grids.size();
    for (auto grid : grids) {
	outs << " Grd=" << ++n;
	(reinterpret_cast<GRIB2Grid *>(grid))->print_header(outs,verbose,path_to_parameter_map);
    }
  }
}

void GRIB2Message::quick_fill(const unsigned char *stream_buffer)
{
  int test,off;
  short sec_num,last_sec_num=99;
  GRIB2Grid *g2=nullptr;
  union {
    float dum;
    int idum;
  };

  if (stream_buffer == nullptr) {
    myerror="empty file stream";
    exit(1);
  }
  clear_grids();
  edition_=2;
  curr_off=16;
  bits::get(stream_buffer,test,curr_off*8,32);
  while (test != 0x37373737) {
    bits::get(stream_buffer,sec_num,curr_off*8+32,8);
    if (sec_num < last_sec_num) {
	g2=new GRIB2Grid;
	if (grids.size() > 0) {
	  g2->quick_copy(*(reinterpret_cast<GRIB2Grid *>(grids.back())));
	}
	g2->grid.filled=false;
	grids.emplace_back(g2);
    }
    switch (sec_num) {
	case 1:
	{
	  curr_off+=test;
	  break;
	}
	case 2:
	{
	  curr_off+=test;
	  break;
	}
	case 3:
	{
	  off=curr_off*8;
	  bits::get(stream_buffer,g2->dim.size,off+48,32);
	  bits::get(stream_buffer,g2->dim.x,off+256,16);
	  bits::get(stream_buffer,g2->dim.y,off+288,16);
	  switch (g2->grid.grid_type) {
	    case 0:
	    case 1:
	    case 2:
	    case 3:
	    case 40:
	    case 41:
	    case 42:
	    case 43:
	    {
		bits::get(stream_buffer,g2->grib.scan_mode,off+568,8);
		break;
	    }
	    case 10:
	    {
		bits::get(stream_buffer,g2->grib.scan_mode,off+472,8);
		break;
	    }
	    case 20:
	    case 30:
	    case 31:
	    {
		bits::get(stream_buffer,g2->grib.scan_mode,off+512,8);
		break;
	    }
	    default:
	    {
		std::cerr << "Error: quick_fill not implemented for grid type " << g2->grid.grid_type << std::endl;
		exit(1);
	    }
	  }
	  curr_off+=test;
	  break;
	}
	case 4:
	{
	  off=curr_off*8;
	  bits::get(stream_buffer,g2->grib2.param_cat,off+72,8);
	  bits::get(stream_buffer,g2->grid.param,off+80,8);
	  curr_off+=test;
	  break;
	}
	case 5:
	{
	  unpack_drs(stream_buffer);
	  break;
	}
	case 6:
	{
unpack_bms(stream_buffer);
/*
	  off=curr_off*8;
	  int ind;
	  bits::get(stream_buffer,ind,off+40,8);
	  if (ind == 0) {
	    bits::get(stream_buffer,lengths_.bms,off,32);
	    if ( (len=(lengths_.bms-6)*8) > 0) {
		if (len > g2->bitmap.capacity) {
		  if (g2->bitmap.map != nullptr) {
		    delete[] g2->bitmap.map;
		  }
		  g2->bitmap.capacity=len;
		  g2->bitmap.map=new unsigned char[g2->bitmap.capacity];
		}
		bits::get(stream_buffer,g2->bitmap.map,off+48,1,0,len);
	    }
	    g2->bitmap.applies=true;
	  }
	  else if (ind != 255) {
	    std::cerr << "Error: bitmap not implemented for quick_fill()" << std::endl;
	    exit(1);
	  }
	  else {
	    g2->bitmap.applies=false;
	  }
	  curr_off+=test;
*/
	  break;
	}
	case 7:
	{
	  unpack_ds(stream_buffer,false);
	  break;
	}
    }
    last_sec_num=sec_num;
    bits::get(stream_buffer,test,curr_off*8,32);
  }
}

GRIBGrid::~GRIBGrid()
{
  if (plist != nullptr) {
    delete[] plist;
    plist=nullptr;
  }
  if (bitmap.map != nullptr) {
    delete[] bitmap.map;
    bitmap.map=nullptr;
  }
  if (gridpoints_ != nullptr) {
    for (size_t n=0; n < grib.capacity.y; ++n) {
	delete[] gridpoints_[n];
    }
    delete[] gridpoints_;
    gridpoints_=nullptr;
  }
  if (map.param != nullptr) {
    delete[] map.param;
    map.param=nullptr;
    delete[] map.level_type;
    delete[] map.lvl;
    delete[] map.lvl2;
    delete[] map.mult;
  }
}

GRIBGrid& GRIBGrid::operator=(const GRIBGrid& source)
{
  size_t n,m;

  if (this == &source) {
    return *this;
  }
  reference_date_time_=source.reference_date_time_;
  valid_date_time_=source.valid_date_time_;
  dim=source.dim;
  def=source.def;
  stats=source.stats;
  ensdata=source.ensdata;
  grid=source.grid;
  if (source.plist != nullptr) {
    if (plist != nullptr) {
	delete[] plist;
    }
    plist=new int[dim.y];
    for (n=0; n < static_cast<size_t>(dim.y); ++n) {
	plist[n]=source.plist[n];
    }
  }
  else {
    if (plist != nullptr) {
	delete[] plist;
    }
    plist=nullptr;
  }
  bitmap.applies=source.bitmap.applies;
  if (bitmap.capacity != source.bitmap.capacity) {
    if (bitmap.map != nullptr) {
	delete[] bitmap.map;
	bitmap.map=nullptr;
    }
    bitmap.capacity=source.bitmap.capacity;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  for (n=0; n < bitmap.capacity; ++n) {
    bitmap.map[n]=source.bitmap.map[n];
  }
  if (gridpoints_ != nullptr && grib.capacity.points < source.grib.capacity.points) {
    for (n=0; n < grib.capacity.y; ++n) {
	delete[] gridpoints_[n];
    }
    delete[] gridpoints_;
    gridpoints_=nullptr;
  }
  grib=source.grib;
  if (source.grid.filled) {
    galloc();
    for (n=0; n < static_cast<size_t>(source.dim.y); ++n) {
	for (m=0; m < static_cast<size_t>(source.dim.x); ++m) {
	  gridpoints_[n][m]=source.gridpoints_[n][m];
	}
    }
  }
  if (source.map.param != nullptr) {
    if (map.param != nullptr) {
	delete[] map.param;
	delete[] map.level_type;
	delete[] map.lvl;
	delete[] map.lvl2;
	delete[] map.mult;
    }
    map.param=new short[255];
    map.level_type=new short[255];
    map.lvl=new short[255];
    map.lvl2=new short[255];
    map.mult=new float[255];
    for (n=0; n < 255; n++) {
	map.param[n]=source.map.param[n];
	map.level_type[n]=source.map.level_type[n];
	map.lvl[n]=source.map.lvl[n];
	map.lvl2[n]=source.map.lvl2[n];
	map.mult[n]=source.map.mult[n];
    }
  }
  else {
    if (map.param != nullptr) {
	delete[] map.param;
	delete[] map.level_type;
	delete[] map.lvl;
	delete[] map.lvl2;
	delete[] map.mult;
    }
    map.param=map.level_type=map.lvl=map.lvl2=nullptr;
    map.mult=nullptr;
  }
  return *this;
}

void GRIBGrid::galloc()
{
  size_t n;

  if (gridpoints_ != nullptr && dim.size > grib.capacity.points) {
    for (n=0; n < grib.capacity.y; ++n) {
	delete[] gridpoints_[n];
    }
    delete[] gridpoints_;
    gridpoints_=nullptr;
  }
  if (gridpoints_ == nullptr) {
    grib.capacity.y=dim.y;
    gridpoints_=new double *[grib.capacity.y];
    if (dim.x == -1) {
	for (n=0; n < grib.capacity.y; ++n) {
	  gridpoints_[n]=new double[plist[n]+1];
	  gridpoints_[n][0]=plist[n];
	}
    }
    else {
	for (n=0; n < grib.capacity.y; ++n) {
	  gridpoints_[n]=new double[dim.x];
	}
    }
    grib.capacity.points=dim.size;
  }
}

void GRIBGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  myerror="a GRIB grid must be filled from a GRIB message";
  exit(1);
}

void GRIBGrid::fill_from_grib_data(const GRIBData& source)
{
  int n,m,cnt=0,avg_cnt=0;
  my::map<Grid::GLatEntry> gaus_lats;

  grib.table=source.param_version;
  grid.src=source.source;
  grib.process=source.process;
  grib.grid_catalog_id=source.grid_catalog_id;
  grib.scan_mode=source.scan_mode;
  grid.grid_type=source.grid_type;
  dim=source.dim;
  def=source.def;
  if (def.type == Grid::gaussianLatitudeLongitudeType) {
    if (_path_to_gauslat_lists.empty()) {
	myerror="path to gaussian latitude data was not specified";
	exit(0);
    }
    else if (!gridutils::fill_gaussian_latitudes(_path_to_gauslat_lists,gaus_lats,def.num_circles,(grib.scan_mode&0x40) != 0x40)) {
	myerror="unable to get gaussian latitudes for "+strutils::itos(def.num_circles)+" circles";
	exit(0);
    }
  }
  grid.param=source.param_code;
  grid.level1_type=source.level_types[0];
  grid.level2_type=source.level_types[1];
  switch (grid.level1_type) {
    case 1:
    case 100:
    case 103:
    case 105:
    case 107:
    case 109:
    case 111:
    case 113:
    case 115:
    case 117:
    case 125:
    case 160:
    case 200:
    case 201:
    {
	grid.level1=source.levels[0];
	grid.level2=0.;
	break;
    }
    default:
    {
	grid.level1=source.levels[0];
	grid.level2=source.levels[1];
    }
  }
  reference_date_time_=source.reference_date_time;
  valid_date_time_=source.valid_date_time;
  grib.time_unit=source.time_unit;
  grib.t_range=source.time_range;
  grid.nmean=0;
  grib.nmean_missing=0;
  switch (grib.t_range) {
    case 4:
    case 10:
    {
	grib.p1=source.p1;
	grib.p2=0;
	break;
    }
    default:
    {
	grib.p1=source.p1;
	grib.p2=source.p2;
    }
  }
  grid.nmean=source.num_averaged;
  grib.nmean_missing=source.num_averaged_missing;
  grib.sub_center=source.sub_center;
  grib.D=source.D;
  grid.num_missing=0;
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != nullptr) {
	delete[] bitmap.map;
	bitmap.map=nullptr;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  bitmap.applies=false;
  stats.min_val=Grid::missing_value;
  stats.max_val=-Grid::missing_value;
  stats.avg_val=0.;
  grid.num_missing=0;
  galloc();
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	gridpoints_[n][m]=source.gridpoints[n][m];
	if (floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  bitmap.map[cnt++]=0;
	  ++grid.num_missing;
	  bitmap.applies=true;
	}
	else {
	  bitmap.map[cnt++]=1;
	  if (gridpoints_[n][m] > stats.max_val) {
	    stats.max_val=gridpoints_[n][m];
	  }
	  if (gridpoints_[n][m] < stats.min_val) {
	    stats.min_val=gridpoints_[n][m];
	  }
	  stats.avg_val+=gridpoints_[n][m];
	  ++avg_cnt;
	}
    }
  }
  stats.avg_val/=static_cast<double>(avg_cnt);
  grib.pack_width=source.pack_width;
  grid.filled=true;
}

float GRIBGrid::latitude_at(int index,my::map<GLatEntry> *gaus_lats) const
{
  GLatEntry glat_entry;
  float findex=index;
  int n;

  if (def.type == Grid::gaussianLatitudeLongitudeType) {
    glat_entry.key=def.num_circles;
    if (gaus_lats == nullptr || !gaus_lats->found(glat_entry.key,glat_entry)) {
	return -99.9;
    }
    if (static_cast<size_t>(dim.y) == def.num_circles*2) {
	return glat_entry.lats[index];
    }
    else {
	n=0;
	while (static_cast<size_t>(n) < def.num_circles*2) {
	  if (fabs(def.slatitude-glat_entry.lats[n]) < 0.001) {
	    break;
	  }
	  ++n;
	}
	if (grib.scan_mode == 0x0) {
	  if (static_cast<size_t>(n+index) < def.num_circles*2) {
	    return glat_entry.lats[n+index];
	  }
	  else {
	    myerror="error finding first latitude for gaussian circles "+strutils::itos(def.num_circles);
	    exit(1);
	  }
	}
	else {
	  if ( (dim.y-(n+index)) > 0) {
	    return glat_entry.lats[dim.y-(n+index)-1];
	  }
	  else {
	    myerror="error finding first latitude for gaussian circles "+strutils::itos(def.num_circles);
	    exit(1);
	  }
	}
    }
  }
  else {
    findex*=def.laincrement;
    if (grib.scan_mode == 0x0) {
	findex*=-1.;
    }
    return def.slatitude+findex;
  }
}

int GRIBGrid::latitude_index_of(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  GLatEntry glat_entry;
  size_t n=0;

  if (def.type == Grid::gaussianLatitudeLongitudeType) {
    glat_entry.key=def.num_circles;
    if (gaus_lats == nullptr || !gaus_lats->found(glat_entry.key,glat_entry)) {
	return -1;
    }
    glat_entry.key*=2;
    while (n < glat_entry.key) {
	if (floatutils::myequalf(latitude,glat_entry.lats[n],0.01)) {
	  return n;
	}
	++n;
    }
  }
  else {
    return Grid::latitude_index_of(latitude,gaus_lats);
  }
  return -1;
}

int GRIBGrid::latitude_index_north_of(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  GLatEntry glat_entry;
  int n;

  if (def.type == Grid::gaussianLatitudeLongitudeType) {
    glat_entry.key=def.num_circles;
    if (gaus_lats == nullptr || !gaus_lats->found(glat_entry.key,glat_entry)) {
	return -1;
    }
    glat_entry.key*=2;
    if (grib.scan_mode == 0x0) {
	n=0;
	while (n < static_cast<int>(glat_entry.key)) {
	  if (latitude > glat_entry.lats[n]) {
	    return (n-1);
	  }
	  ++n;
	}
    }
    else {
	n=glat_entry.key-1;
	while (n >= 0) {
	  if (latitude > glat_entry.lats[n]) {
	    return (n < static_cast<int>(glat_entry.key-1)) ? (n+1) : -1;
	  }
	  --n;
	}
    }
  }
  else {
    return Grid::latitude_index_north_of(latitude,gaus_lats);
  }
  return -1;
}

int GRIBGrid::latitude_index_south_of(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  GLatEntry glat_entry;
  int n;

  if (def.type == Grid::gaussianLatitudeLongitudeType) {
    glat_entry.key=def.num_circles;
    if (gaus_lats == nullptr || !gaus_lats->found(glat_entry.key,glat_entry)) {
	return -1;
    }
    glat_entry.key*=2;
    if (grib.scan_mode == 0x0) {
	n=0;
	while (n < static_cast<int>(glat_entry.key)) {
	  if (latitude > glat_entry.lats[n]) {
	    return n;
	  }
	  ++n;
	}
    }
    else {
	n=glat_entry.key-1;
	while (n >= 0) {
	  if (latitude > glat_entry.lats[n]) {
	    return n;
	  }
	  --n;
	}
    }
  }
  else {
    return Grid::latitude_index_south_of(latitude,gaus_lats);
  }
  return -1;
}

void GRIBGrid::remap_parameters()
{
  std::ifstream ifs;
  char line[80];
  std::string file_name;
  size_t n;

  if (!grib.map_available) {
    return;
  }
  if (!grib.map_filled || grib.last_src != grid.src) {
    if (map.param != nullptr) {
	delete[] map.param;
	delete[] map.level_type;
	delete[] map.lvl;
	delete[] map.lvl2;
	delete[] map.mult;
    }
    ifs.open(("/glade/u/home/rdadata/share/GRIB/parameter_map."+strutils::itos(grid.src)).c_str());
    if (ifs.is_open()) {
	map.param=new short[255];
	map.level_type=new short[255];
	map.lvl=new short[255];
	map.lvl2=new short[255];
	map.mult=new float[255];
	for (n=0; n < 255; n++) {
	  map.param[n]=map.level_type[n]=-1;
	}
	ifs.getline(line,80);
	while (!ifs.eof()) {
	  strutils::strget(line,n,3);
	  strutils::strget(&line[4],map.param[n],3);
	  strutils::strget(&line[8],map.level_type[n],3);
	  if (map.level_type[n] != -1) {
	    strutils::strget(&line[12],map.lvl[n],5);
	    strutils::strget(&line[18],map.lvl2[n],5);
	  }
	  strutils::strget(&line[24],map.mult[n],14);
	  ifs.getline(line,80);
	}
	grib.map_filled=true;
	ifs.close();
    }
    else {
	grib.map_available=false;
    }
  }
  if (!grib.map_available) {
    return;
  }
  if (map.level_type[grid.param] != -1) {
    grid.level1_type=map.level_type[grid.param];
    grid.level1=map.lvl[grid.param];
    grid.level2=map.lvl2[grid.param];
  }
  if (!floatutils::myequalf(map.mult[grid.param],1.)) {
    this->multiply_by(map.mult[grid.param]);
  }
  if (map.param[grid.param] != -1) {
    grid.param=map.param[grid.param];
  }
}

void GRIBGrid::set_scale_and_packing_width(short decimal_scale_factor,short maximum_pack_width)
{
  double range,d;
  long long lrange;

  grib.D=decimal_scale_factor;
  d=pow(10.,grib.D);
  range=lroundf(stats.max_val*d)-lroundf(stats.min_val*d);
  lrange=llround(range);
  grib.pack_width=1;
  while (grib.pack_width < maximum_pack_width && lrange >= pow(2.,grib.pack_width))
    grib.pack_width++;
  grib.E=0;
  while (grib.E < 20 && lrange >= pow(2.,grib.pack_width)) {
    grib.E++;
    lrange/=2;
  }
  if (lrange >= pow(2.,grib.pack_width)) {
myerror="unable to scale - range: "+strutils::lltos(lrange)+" pack width: "+strutils::itos(grib.pack_width);
exit(1);
  }
}

void GRIBGrid::operator+=(const GRIBGrid& source)
{
  int n,m;
  size_t l,avg_cnt=0,cnt=0;
  double mult;

  grid.src=60;
  grib.sub_center=1;
  if (grib.capacity.points == 0) {
    *this=source;
    if (source.grib.t_range == 51) {
	grid.nmean=source.grid.nmean;
	this->multiply_by(grid.nmean);
    }
    else {
	grid.nmean=1;
    }
    grib.t_range=0;
  }
  else {
    if (dim.size != source.dim.size || dim.x != source.dim.x || dim.y != source.dim.y) {
	std::cerr << "Warning: unable to perform grid addition" << std::endl;
	return;
    }
    else {
	if (bitmap.capacity == 0) {
	  bitmap.capacity=dim.size;
	  bitmap.map=new unsigned char[bitmap.capacity];
	  grid.num_missing=0;
	}
	stats.max_val=-Grid::missing_value;
	stats.min_val=Grid::missing_value;
	stats.avg_val=0.;
	if (source.grib.t_range == 51)
	  mult=source.grid.nmean;
	else
	  mult=1.;
	for (n=0; n < dim.y; n++) {
	  for (m=0; m < dim.x; m++) {
	    l= (source.grib.scan_mode == grib.scan_mode) ? n : dim.y-n-1;
	    if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value) && !floatutils::myequalf(source.gridpoints_[l][m],Grid::missing_value)) {
		gridpoints_[n][m]+=(source.gridpoints_[l][m]*mult);
		bitmap.map[cnt++]=1;
		if (gridpoints_[n][m] > stats.max_val) {
		  stats.max_val=gridpoints_[n][m];
		}
		if (gridpoints_[n][m] < stats.min_val) {
		  stats.min_val=gridpoints_[n][m];
		}
		stats.avg_val+=gridpoints_[n][m];
		++avg_cnt;
	    }
	    else {
		gridpoints_[n][m]=(Grid::missing_value*mult);
		bitmap.map[cnt++]=0;
		grid.num_missing++;
	    }
	  }
	}
	if (grid.num_missing == 0) {
	  bitmap.applies=false;
	}
	else {
	  bitmap.applies=true;
	}
	if (source.grib.t_range == 51)
	  grid.nmean=grid.nmean+source.grid.nmean;
	else
	  grid.nmean++;
	if (avg_cnt > 0)
	  stats.avg_val/=static_cast<double>(avg_cnt);
    }
    if (grib.t_range == 0) {
	if (source.reference_date_time_.month() == reference_date_time_.month()) {
	  if (source.reference_date_time_.day() == reference_date_time_.day() && source.reference_date_time_.year() > reference_date_time_.year()) {
	    grib.t_range=151;
	    grib.p1=0;
	    grib.p2=1;
	    grib.time_unit=3;
	  }
	  else if (source.reference_date_time_.year() == reference_date_time_.year() && source.reference_date_time_.day() > reference_date_time_.day()){
	    grib.t_range=114;
	    grib.p2=1;
	    grib.time_unit=2;
	  }
	  else if (source.reference_date_time_.year() == reference_date_time_.year() && source.reference_date_time_.day() == reference_date_time_.day() && source.reference_date_time_.time() > reference_date_time_.time()) {
	    grib.t_range=114;
	    grib.p2=source.reference_date_time_.time()-reference_date_time_.time()/100.;
	    grib.time_unit=1;
	  }
	}
    }
// adding a mean and an accumulation indicates a variance grid
    else if ((grib.t_range == 113 && source.grib.t_range == 114) ||
             (grib.t_range == 114 && source.grib.t_range == 113)) {
	grib.t_range=118;
	grid.nmean--;
    }
  }
}

void GRIBGrid::operator-=(const GRIBGrid& source)
{
  int n,m;
  size_t l,avg_cnt=0,cnt=0;

  if (grib.capacity.points == 0) {
    *this=source;
    grib.t_range=5;
    grib.p2=source.grib.p2;
  }
  else {
    if (dim.size != source.dim.size || dim.x != source.dim.x || dim.y != source.dim.y) {
	std::cerr << "Warning: unable to perform grid subtraction" << std::endl;
	return;
    }
    else {
	if (bitmap.capacity == 0) {
	  bitmap.capacity=dim.size;
	  bitmap.map=new unsigned char[bitmap.capacity];
	  grid.num_missing=0;
	}
	stats.max_val=-Grid::missing_value;
	stats.min_val=Grid::missing_value;
	stats.avg_val=0;
	for (n=0; n < dim.y; n++) {
	  for (m=0; m < dim.x; m++) {
	    l= (source.grib.scan_mode == grib.scan_mode) ? n : dim.y-n-1;
	    if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value) && !floatutils::myequalf(source.gridpoints_[l][m],Grid::missing_value)) {
		gridpoints_[n][m]-=source.gridpoints_[l][m];
		bitmap.map[cnt++]=1;
		if (gridpoints_[n][m] > stats.max_val) {
		  stats.max_val=gridpoints_[n][m];
		}
		if (gridpoints_[n][m] < stats.min_val) {
		  stats.min_val=gridpoints_[n][m];
		}
		stats.avg_val+=gridpoints_[n][m];
		++avg_cnt;
	    }
	    else {
		gridpoints_[n][m]=Grid::missing_value;
		bitmap.map[cnt++]=0;
		++grid.num_missing;
	    }
	  }
	}
	if (grid.num_missing == 0) {
	  bitmap.applies=false;
	}
	else {
	  bitmap.applies=true;
	}
	if (avg_cnt > 0) {
	  stats.avg_val/=static_cast<double>(avg_cnt);
	}
    }
    grib.p1=source.grib.p1;
  }
}

void GRIBGrid::operator*=(const GRIBGrid& source)
{
  int n,m;
  size_t avg_cnt=0;

  if (grib.capacity.points == 0)
    return;

  if (dim.size != source.dim.size || dim.x != source.dim.x || dim.y !=
      source.dim.y) {
    std::cerr << "Warning: unable to perform grid multiplication" << std::endl;
    return;
  }
  else {
    stats.max_val=-Grid::missing_value;
    stats.min_val=Grid::missing_value;
    stats.avg_val=0.;
    for (n=0; n < dim.y; ++n) {
	for (m=0; m < dim.x; ++m) {
	  if (!floatutils::myequalf(source.gridpoints_[n][m],Grid::missing_value)) {
	    gridpoints_[n][m]*=source.gridpoints_[n][m];
	    if (gridpoints_[n][m] > stats.max_val) {
		stats.max_val=gridpoints_[n][m];
	    }
	    if (gridpoints_[n][m] < stats.min_val) {
		stats.min_val=gridpoints_[n][m];
	    }
	    stats.avg_val+=gridpoints_[n][m];
	    ++avg_cnt;
	  }
	  else {
	    gridpoints_[n][m]=Grid::missing_value;
	  }
	}
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
}

void GRIBGrid::operator/=(const GRIBGrid& source)
{
  int n,m;
  size_t avg_cnt=0;

  if (grib.capacity.points == 0) {
    return;
  }
  if (dim.size != source.dim.size || dim.x != source.dim.x || dim.y != source.dim.y) {
    std::cerr << "Warning: unable to perform grid division" << std::endl;
    return;
  }
  else {
    stats.max_val=-Grid::missing_value;
    stats.min_val=Grid::missing_value;
    stats.avg_val=0.;
    for (n=0; n < dim.y; ++n) {
	for (m=0; m < dim.x; ++m) {
	  if (!floatutils::myequalf(source.gridpoints_[n][m],Grid::missing_value)) {
	    gridpoints_[n][m]/=source.gridpoints_[n][m];
	    if (gridpoints_[n][m] > stats.max_val) {
		stats.max_val=gridpoints_[n][m];
	    }
	    if (gridpoints_[n][m] < stats.min_val) {
		stats.min_val=gridpoints_[n][m];
	    }
	    stats.avg_val+=gridpoints_[n][m];
	    ++avg_cnt;
	  }
	  else {
	    gridpoints_[n][m]=Grid::missing_value;
	  }
	}
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
}

void GRIBGrid::divide_by(double div)
{
  int n,m;
  size_t avg_cnt=0;

  if (grib.capacity.points == 0) {
    return;
  }
  switch (grib.t_range) {
    case 114:
    {
	grib.t_range=113;
	break;
    }
    case 151:
    {
	grib.t_range=51;
	break;
    }
  }
  stats.min_val/=div;
  stats.max_val/=div;
  stats.avg_val=0.;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  gridpoints_[n][m]/=div;
	  stats.avg_val+=gridpoints_[n][m];
	  ++avg_cnt;
	}
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
}

GRIBGrid& GRIBGrid::operator=(const TDLGRIBGrid& source)
{
  int n,m;
  size_t avg_cnt=0,max_pack;
  double mult=1.,range;

  *this=*(reinterpret_cast<GRIBGrid *>(const_cast<TDLGRIBGrid *>(&source)));
  grid.num_missing=0;
  grib.table=3;
  grid.src=7;
  grib.grid_catalog_id=255;
  grib.rescomp=0;
  grib.p2=0;
  grib.t_range=10;
  switch (source.model()) {
    case 6:
    {
	switch (source.parameter()) {
	  case 1000:
	  {
// Height
	    grid.param=7;
	    grid.level1_type=100;
	    break;
	  }
	  case 1201:
	  {
// MSLP
	    grid.param=2;
	    grid.level1_type=102;
	    mult=100.;
	    grib.D-=2;
	    break;
	  }
	  case 2000:
	  {
// temperature
	    grid.param=11;
	    grid.level1_type=100;
	    break;
	  }
	  case 3000:
	  {
// RH
	    grid.param=52;
	    grid.level1_type=100;
	    break;
	  }
	  case 3001:
	  case 3002:
	  {
// mean RH through a sigma layer
	    grid.param=52;
	    grid.level1_type=108;
	    grid.level1=47.;
	    grid.level2=100.;
	    break;
	  }
	  case 3211:
	  {
// 6-hr total precip
	    grid.param=61;
	    grid.level1_type=1;
	    mult=1000.;
	    grib.D-=3;
	    if ( (source.grid.fcst_time % 10000) > 0) {
		grib.time_unit=0;
		grib.p2=source.grid.fcst_time/10000*60+(source.grid.fcst_time % 10000)/100;
		grib.p1=grib.p2-360;
	    }
	    else {
		grib.time_unit=1;
		grib.p2=source.grid.fcst_time/10000;
		grib.p1=grib.p2-6;
	    }
	    grib.t_range=4;
	    break;
	  }
	  case 3221:
	  {
// 12-hr total precip
	    grid.param=61;
	    grid.level1_type=1;
	    mult=1000.;
	    grib.D-=3;
	    if ( (source.grid.fcst_time % 10000) > 0) {
		grib.time_unit=0;
		grib.p2=source.grid.fcst_time/10000*60+(source.grid.fcst_time % 10000)/100;
		grib.p1=grib.p2-720;
	    }
	    else {
		grib.time_unit=1;
		grib.p2=source.grid.fcst_time/10000;
		grib.p1=grib.p2-12;
	    }
	    grib.t_range=4;
	    break;
	  }
	  case 3350:
	  {
// total precipitable water
	    grid.param=54;
	    grid.level1_type=200;
	    break;
	  }
	  case 4000:
	  {
// u-component of the wind
	    grid.param=33;
	    grid.level1_type=100;
	    grib.rescomp|=0x8;
	    break;
	  }
	  case 4020:
	  {
// 10m u-component
	    grid.param=33;
	    grid.level1_type=105;
	    grib.rescomp|=0x8;
	    break;
	  }
	  case 4100:
	  {
// v-component of the wind
	    grid.param=34;
	    grid.level1_type=100;
	    grib.rescomp|=0x8;
	    break;
	  }
	  case 4120:
	  {
// 10m v-component
	    grid.param=34;
	    grid.level1_type=105;
	    grib.rescomp|=0x8;
	    break;
	  }
	  case 5000:
	  case 5003:
	  {
// vertical velocity
	    grid.param=39;
	    grid.level1_type=100;
	    mult=100.;
	    grib.D-=2;
	    break;
	  }
	  default:
	  {
	    myerror="unable to convert parameter "+strutils::itos(source.parameter());
	    exit(1);
	  }
	}
	grib.process=39;
	break;
    }
    default:
    {
	myerror="unrecognized model "+strutils::itos(source.model());
	exit(1);
    }
  }
  grid.level2_type=-1;
  grib.sub_center=11;
  grib.scan_mode=0x40;
  if (grid.grid_type == 5 && !floatutils::myequalf(def.elatitude,60.)) {
    myerror="unable to handle a grid length not at 60 degrees latitude";
    exit(1);
  }
  galloc();
  stats.avg_val=0.;
  stats.min_val=Grid::missing_value;
  stats.max_val=-Grid::missing_value;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	gridpoints_[n][m]=source.gridpoint(m,n)*mult;
	if (gridpoints_[n][m] > stats.max_val) {
	  stats.max_val=gridpoints_[n][m];
	}
	if (gridpoints_[n][m] < stats.min_val) {
	  stats.min_val=gridpoints_[n][m];
	}
	stats.avg_val+=gridpoints_[n][m];
	++avg_cnt;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round(stats.max_val-stats.min_val);
  while (max_pack > 0 && max_pack > range) {
    grib.pack_width--;
    max_pack/=2;
  }
  grid.filled=true;
  return *this;
}

GRIBGrid& GRIBGrid::operator=(const OctagonalGrid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double range;
  float factor=1.,offset=0.;

  grib.table=3;
  switch (source.source()) {
    case 1:
    {
	grid.src=7;
	break;
    }
    case 3:
    {
	grid.src=58;
	break;
    }
    default:
    {
	myerror="source code "+strutils::itos(source.source())+" not recognized";
	exit(1);
    }
  }
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.param=octagonal_grid_parameter_map[source.parameter()];
  grid.level2_type=-1;
  switch (grid.param) {
    case 1:
    {
	if (floatutils::myequalf(source.first_level_value(),1013.)) {
	  grid.level1_type=2;
	  grid.param=2;
	}
	else
	  grid.level1_type=1;
	factor=100.;
	break;
    }
    case 2:
    {
	grid.level1_type=2;
	break;
    }
    case 7:
    {
	grid.level1_type=100;
	break;
    }
    case 11:
    case 12:
    {
	if (floatutils::myequalf(source.first_level_value(),1001.)) {
	  grid.level1_type=1;
	}
	else {
	  grid.level1_type=100;
	}
	offset=273.15;
	break;
    }
    case 33:
    case 34:
    case 39:
    {
	grid.level1_type=100;
	break;
    }
    case 52:
    {
	grid.level1_type=101;
	break;
    }
    case 80:
    {
	grid.level1_type=1;
	offset=273.15;
	break;
    }
    default:
    {
	myerror="unable to convert parameter "+strutils::itos(source.parameter());
	exit(1);
    }
  }
  if (!floatutils::myequalf(source.second_level_value(),0.)) {
    grid.level1=source.second_level_value();
    grid.level2=source.first_level_value();
  }
  else {
    grid.level1=source.first_level_value();
    grid.level2=0.;
  }
  reference_date_time_=source.reference_date_time();
  if (!source.is_averaged_grid()) {
    grib.p1=source.forecast_time()/10000;
    grib.time_unit=1;
    grib.p2=0;
    grib.t_range=10;
  }
  else {
    myerror="unable to convert a mean grid";
    exit(1);
  }
  grib.sub_center=0;
  grib.D=2;
  grid.grid_type=5;
  grib.scan_mode=0x40;
  dim=source.dimensions();
  def=source.definition();
  def.projection_flag=0x80;
  grib.rescomp=0x80;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != nullptr) {
	delete[] bitmap.map;
	bitmap.map=nullptr;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::missing_value;
  stats.max_val=-Grid::missing_value;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	gridpoints_[n][m]=source.gridpoint(m,n);
	if (floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  bitmap.map[cnt]=0;
	  ++grid.num_missing;
	}
	else {
	  gridpoints_[n][m]*=factor;
	  gridpoints_[n][m]+=offset;
	  bitmap.map[cnt]=1;
	  if (gridpoints_[n][m] > stats.max_val) {
	    stats.max_val=gridpoints_[n][m];
	  }
	  if (gridpoints_[n][m] < stats.min_val) {
	    stats.min_val=gridpoints_[n][m];
	  }
	  stats.avg_val+=gridpoints_[n][m];
	  ++avg_cnt;
	}
	++cnt;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round(stats.max_val-stats.min_val);
  while (max_pack > 0 && max_pack > range) {
    --grib.pack_width;
    max_pack/=2;
  }
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  grid.filled=true;
  return *this;
}


GRIBGrid& GRIBGrid::operator=(const LatLonGrid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double range;
  float factor=1.,offset=0.;

  grib.table=3;
  switch (source.source()) {
    case 1:
    case 5:
    {
	grid.src=7;
	break;
    }
    case 2:
    {
	grid.src=57;
	break;
    }
    case 3:
    {
	grid.src=58;
	break;
    }
    case 4:
    {
	grid.src=255;
/*
	grib.lengths_.pds_supp=10;
	grib.lengths_.pds+=(12+grib.lengths_.pds_supp);
	if (pds_supp != nullptr)
	  delete[] pds_supp;
	pds_supp=new unsigned char[11];
	strcpy((char *)pds_supp,"ESSPO GRID");
*/
	break;
    }
    case 9:
    {
	if (floatutils::myequalf(source.first_level_value(),700.)) {
	  grid.src=7;
	}
	else {
	  myerror="source code "+strutils::itos(source.source())+" not recognized";
	  exit(1);
	}
	break;
    }
    default:
    {
	myerror="source code "+strutils::itos(source.source())+" not recognized";
	exit(1);
    }
  }
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.param=octagonal_grid_parameter_map[source.parameter()];
  grid.level2_type=-1;
  switch (grid.param) {
    case 1:
    {
	if (floatutils::myequalf(source.first_level_value(),1013.)) {
	  grid.level1_type=2;
	  grid.param=2;
	}
	else {
	  grid.level1_type=1;
	}
	factor=100.;
	break;
    }
    case 2:
    {
	grid.level1_type=2;
	break;
    }
    case 7:
    {
	grid.level1_type=100;
	if (floatutils::myequalf(source.first_level_value(),700.) && source.source() == 9) {
	  factor=0.01;
	}
	break;
    }
    case 11:
    case 12:
    {
	if (floatutils::myequalf(source.first_level_value(),1001.)) {
	  grid.level1_type=1;
	}
	else {
	  grid.level1_type=100;
	}
	offset=273.15;
	break;
    }
    case 33:
    case 34:
    case 39:
    {
	grid.level1_type=100;
	break;
    }
    case 52:
    {
	grid.level1_type=101;
	break;
    }
    case 80:
    {
	grid.level1_type=1;
	offset=273.15;
	break;
    }
    default:
    {
	myerror="unable to convert parameter "+strutils::itos(source.parameter());
	exit(1);
    }
  }
  if (grid.level1_type > 10 && grid.level1_type != 102 && grid.level1_type != 200 && grid.level1_type != 201) {
    if (!floatutils::myequalf(source.second_level_value(),0.)) {
	grid.level1=source.first_level_value();
	grid.level2=source.second_level_value();
    }
    else {
	grid.level1=source.first_level_value();
	grid.level2=0.;
    }
  }
  else {
    grid.level1=grid.level2=0.;
  }
  reference_date_time_=source.reference_date_time();
  if (!source.is_averaged_grid()) {
    grib.p1=source.forecast_time()/10000;
    grib.time_unit=1;
    grib.p2=0;
    grib.t_range=10;
    grid.nmean=0;
  }
  else {
    myerror="unable to convert a mean grid";
    exit(1);
  }
  grib.sub_center=0;
  grib.D=2;
  grid.grid_type=0;
  grib.scan_mode=0x40;
  dim=source.dimensions();
  def=source.definition();
  grib.rescomp=0x80;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != nullptr) {
	delete[] bitmap.map;
	bitmap.map=nullptr;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::missing_value;
  stats.max_val=-Grid::missing_value;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	gridpoints_[n][m]=source.gridpoint(m,n);
	if (floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  bitmap.map[cnt]=0;
	  ++grid.num_missing;
	}
	else {
	  gridpoints_[n][m]*=factor;
	  gridpoints_[n][m]+=offset;
	  bitmap.map[cnt]=1;
	  if (gridpoints_[n][m] > stats.max_val) {
	    stats.max_val=gridpoints_[n][m];
	  }
	  if (gridpoints_[n][m] < stats.min_val) {
	    stats.min_val=gridpoints_[n][m];
	  }
	  stats.avg_val+=gridpoints_[n][m];
	  ++avg_cnt;
	}
	cnt++;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round(stats.max_val-stats.min_val);
  while (max_pack > 0 && max_pack > range) {
    --grib.pack_width;
    max_pack/=2;
  }
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  grid.filled=true;
  return *this;
}

GRIBGrid& GRIBGrid::operator=(const SLPGrid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double range;
  short months[]={0,31,28,31,30,31,30,31,31,30,31,30,31};

  grib.table=3;
  switch (source.source()) {
    case 3:
    {
	grid.src=58;
	break;
    }
    case 4:
    {
	grid.src=57;
	break;
    }
    case 11:
    {
	grid.src=255;
	break;
    }
    case 10:
    {
	grid.src=59;
	break;
    }
    case 12:
    {
	grid.src=74;
	break;
    }
    case 28:
    {
	grid.src=7;
	break;
    }
    case 29:
    {
	grid.src=255;
	break;
    }
    default:
    {
	myerror="source code "+strutils::itos(source.source())+" not recognized";
	exit(1);
    }
  }
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.param=2;
  grid.level1_type=102;
  grid.level1=0.;
  grid.level2_type=-1;
  grid.level2=0.;
  grib.p1=0;
  if (!source.is_averaged_grid()) {
    grib.p2=0;
    reference_date_time_=source.reference_date_time();
    grib.time_unit=1;
    grib.t_range=10;
    grid.nmean=0;
    grib.nmean_missing=0;
  }
  else {
    if (source.reference_date_time().time() == 310000) {
	reference_date_time_.set(source.reference_date_time().year(),source.reference_date_time().month(),1,0);
    }
    else {
	reference_date_time_.set(source.reference_date_time().year(),source.reference_date_time().month(),1,source.reference_date_time().time());
    }
    grib.time_unit=3;
    grib.t_range=113;
    if ( (reference_date_time_.year() % 4) == 0) {
	months[2]=29;
    }
    grid.nmean=months[reference_date_time_.month()];
    if (source.number_averaged() > grid.nmean) {
	grid.nmean*=2;
	grib.time_unit=1;
	grib.p2=12;
    }
    else {
	grib.time_unit=2;
	grib.p2=1;
    }
    grib.nmean_missing=grid.nmean-source.number_averaged();
  }
  grib.sub_center=0;
  grib.D=2;
  grid.grid_type=0;
  grib.scan_mode=0x40;
  dim=source.dimensions();
  dim.size=dim.x*dim.y;
  def=source.definition();
  def.elatitude=90.;
  grib.rescomp=0x80;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != nullptr) {
	delete[] bitmap.map;
	bitmap.map=nullptr;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::missing_value;
  stats.max_val=-Grid::missing_value;
  for (n=0; n < dim.y-1; ++n) {
    for (m=0; m < dim.x; ++m) {
	gridpoints_[n][m]=source.gridpoint(m,n);
	if (floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  bitmap.map[cnt]=0;
	  ++grid.num_missing;
	}
	else {
	  gridpoints_[n][m]*=100.;
	  bitmap.map[cnt]=1;
	  if (gridpoints_[n][m] > stats.max_val) {
	    stats.max_val=gridpoints_[n][m];
	  }
	  if (gridpoints_[n][m] < stats.min_val) {
	    stats.min_val=gridpoints_[n][m];
	  }
	  stats.avg_val+=gridpoints_[n][m];
	  ++avg_cnt;
	}
	++cnt;
    }
  }
  for (m=0; m < dim.x; ++m) {
    gridpoints_[n][m]=source.pole_value();
    if (floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	bitmap.map[cnt]=0;
	grid.num_missing++;
    }
    else {
	gridpoints_[n][m]*=100.;
	bitmap.map[cnt]=1;
	if (gridpoints_[n][m] > stats.max_val) {
	  stats.max_val=gridpoints_[n][m];
	}
	if (gridpoints_[n][m] < stats.min_val) {
	  stats.min_val=gridpoints_[n][m];
	}
	stats.avg_val+=gridpoints_[n][m];
	++avg_cnt;
    }
    ++cnt;
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round(stats.max_val-stats.min_val);
  while (max_pack > 0 && max_pack > range) {
    --grib.pack_width;
    max_pack/=2;
  }
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  set_scale_and_packing_width(grib.D,grib.pack_width);
  grid.filled=true;
  return *this;
}

GRIBGrid& GRIBGrid::operator=(const ON84Grid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double mult=1.,range,precision;

  grib.table=3;
  grid.src=7;
  grib.process=source.generating_program();
  grib.grid_catalog_id=255;
  grid.param=on84_grid_parameter_map[source.parameter()];
  grib.D=2;
  switch (grid.param) {
    case 1:
    {
	mult=100.;
	grib.D=0;
	break;
    }
    case 39:
    {
	mult=100.;
	grib.D=3;
	break;
    }
    case 61:
    {
	mult=1000.;
	grib.D=1;
	break;
    }
    case 210:
    {
	precision=5;
	grib.D=0;
	grib.table=129;
	break;
    }
  }
  if (grid.param == 0) {
    myerror="unable to convert parameter "+strutils::itos(source.parameter());
    exit(1);
  }
  grid.level2_type=-1;
  switch (source.first_level_type()) {
    case 6:
    {
// height above ground
	grid.level1_type=105;
	grid.level1=source.first_level_value();
	grid.level2=source.second_level_value();
	if (!floatutils::myequalf(grid.level1,source.first_level_value()) || !floatutils::myequalf(grid.level2,source.second_level_value())) {
	  myerror="error converting fractional level(s) "+strutils::ftos(source.first_level_value())+" "+strutils::ftos(source.second_level_value());
	  exit(1);
	}
	break;
    }
    case 8:
    {
// pressure level
	grid.level1_type=100;
	grid.level1=source.first_level_value();
	grid.level2=source.second_level_value();
	break;
    }
    case 128:
    {
// mean sea-level
	grid.level1_type=102;
	grid.level1=grid.level2=0.;
	if (grid.param == 1) grid.param=2;
	break;
    }
    case 129:
    {
// surface
	if (source.parameter() == 16) {
	  grid.level1_type=105;
	  grid.level1=2.;
	  grid.level2=0.;
	}
	else {
	  grid.level1_type=1;
	  grid.level1=grid.level2=0.;
	}
	break;
    }
    case 130:
    {
// tropopause level
	grid.level1_type=7;
	grid.level1=grid.level2=0.;
	break;
    }
    case 131:
    {
// maximum wind speed level
	grid.level1_type=6;
	grid.level1=grid.level2=0.;
	break;
    }
    case 144:
    {
// boundary
	if (floatutils::myequalf(source.first_level_value(),0.) && floatutils::myequalf(source.second_level_value(),1.)) {
	  grid.level1_type=107;
	  grid.level1=0.9950;
	  grid.level2=0.;
	}
	else {
	  myerror="error converting boundary layer "+strutils::ftos(source.first_level_value())+" "+strutils::ftos(source.second_level_value());
	  exit(1);
	}
	break;
    }
    case 145:
    {
// troposphere
// sigma level
	if (floatutils::myequalf(source.second_level_value(),0.)) {
	  grid.level1_type=107;
	  grid.level1=source.first_level_value();
	  grid.level2=0.;
	}
// sigma layer
	else {
	  if (floatutils::myequalf(source.first_level_value(),0.) && floatutils::myequalf(source.second_level_value(),1.)) {
	    grid.level1_type=200;
	    grid.level1=0.;
	  }
	  else {
	    grid.level1_type=108;
	    grid.level1=source.first_level_value();
	    grid.level2=source.second_level_value();
	  }
	}
	break;
    }
    case 148:
    {
// entire atmosphere
	grid.level1_type=200;
	grid.level1=grid.level2=0.;
	break;
    }
    default:
    {
	myerror="unable to convert level type "+strutils::itos(source.first_level_type());
	exit(1);
    }
  }
  reference_date_time_=source.reference_date_time();
  if (!source.is_averaged_grid()) {
    switch (source.time_marker()) {
	case 0:
	{
	  grib.p1=source.forecast_time()/10000;
	  grib.p2=0;
	  grib.t_range=10;
	  grib.time_unit=1;
	  break;
	}
	case 3:
	{
	  if (grid.param == 61) {
	    grib.p1=source.F1()-source.F2();
	    grib.p2=source.F1();
	    grib.t_range=5;
	    grib.time_unit=1;
	  }
	  else {
	    myerror="unable to convert time marker "+strutils::itos(source.time_marker());
	    exit(1);
	  }
	  break;
	}
	default:
	{
	  myerror="unable to convert time marker "+strutils::itos(source.time_marker());
	  exit(1);
	}
    }
    grid.nmean=grib.nmean_missing=0;
  }
  else {
    myerror="unable to convert a mean grid";
    exit(1);
  }
  grib.sub_center=0;
  switch (source.type()) {
    case 29:
    case 30:
    {
	grid.grid_type=0;
	grib.scan_mode=0x40;
	break;
    }
    case 27:
    case 28:
    case 36:
    case 47:
    {
	grid.grid_type=5;
	def.projection_flag=0;
	grib.scan_mode=0x40;
	grib.sub_center=11;
	break;
    }
    default:
    {
	myerror="unable to convert grid type "+strutils::itos(source.type());
	exit(1);
    }
  }
  dim=source.dimensions();
  def=source.definition();
  grib.rescomp=0x80;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != nullptr) {
	delete[] bitmap.map;
	bitmap.map=nullptr;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.max_val=-Grid::missing_value;
  stats.min_val=Grid::missing_value;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	gridpoints_[n][m]=source.gridpoint(m,n);
	if (floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  bitmap.map[cnt]=0;
	  grid.num_missing++;
	}
	else {
	  gridpoints_[n][m]*=mult;
	  bitmap.map[cnt]=1;
	  if (gridpoints_[n][m] > stats.max_val) {
	    stats.max_val=gridpoints_[n][m];
	  }
	  if (gridpoints_[n][m] < stats.min_val) {
	    stats.min_val=gridpoints_[n][m];
	  }
	  stats.avg_val+=gridpoints_[n][m];
	  ++avg_cnt;
	}
	++cnt;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round((stats.max_val-stats.min_val)*pow(10.,precision));
  while (max_pack > 0 && max_pack > range) {
    grib.pack_width--;
    max_pack/=2;
  }
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  grid.filled=true;
  return *this;
}

GRIBGrid& GRIBGrid::operator=(const CGCM1Grid& source)
{
  int n,m;
  size_t l,cnt=0,avg_cnt=0;

  grib.table=3;
  grid.src=54;
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.level2_type=-1;
  auto pname=std::string(source.parameter_name());
  if (pname == " ALB") {
    grid.param=84;
    grid.level1_type=1;
    grid.level1=0;
  }
  else if (pname == "  AU") {
    grid.param=33;
    grid.level1_type=105;
    grid.level1=10.;
  }
  else if (pname == "  AV") {
    grid.param=34;
    grid.level1_type=105;
    grid.level1=10.;
  }
  else if (pname == "CLDT") {
    grid.param=71;
    grid.level1_type=200;
    grid.level1=0.;
  }
  else if (pname == " FSR") {
    grid.param=116;
    grid.level1_type=8;
    grid.level1=0.;
  }
  else if (pname == " FSS") {
    grid.param=116;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == "FLAG") {
    grid.param=115;
    grid.level1_type=8;
    grid.level1=0.;
  }
  else if (pname == "FSRG") {
    grid.param=116;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == "  GT") {
    grid.param=11;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == "  PS") {
    grid.param=2;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == " PHI") {
    grid.param=7;
    grid.level1_type=100;
    grid.level1=source.first_level_value();
  }
  else if (pname == " PCP") {
    grid.param=61;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == "PMSL") {
    grid.param=2;
    grid.level1_type=102;
    grid.level1=0.;
  }
  else if (pname == "  SQ") {
    grid.param=51;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (pname == "  ST") {
    grid.param=11;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (pname == " SNO") {
    grid.param=79;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == "SHUM") {
    grid.param=51;
    grid.level1_type=100;
    grid.level1=source.first_level_value();
  }
  else if (pname == "STMX") {
    grid.param=15;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (pname == "STMN") {
    grid.param=16;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (pname == "SWMX") {
    grid.param=32;
    grid.level1_type=105;
    grid.level1=10.;
  }
  else if (pname == "TEMP") {
    grid.param=11;
    grid.level1_type=100;
    grid.level1=source.first_level_value();
  }
  else if (pname == "   U") {
    grid.param=33;
    grid.level1_type=100;
    grid.level1=source.first_level_value();
  }
  else if (pname == "   V") {
    grid.param=34;
    grid.level1_type=100;
    grid.level1=source.first_level_value();
  }
  else {
    myerror="unable to convert parameter "+pname;
    exit(1);
  }
  grid.level2=0.;
  reference_date_time_=source.reference_date_time();
  if (reference_date_time_.day() == 0) {
    reference_date_time_.set_day(1);
    grib.t_range=113;
    grib.time_unit=3;
  }
  else {
    grib.t_range=1;
    grib.time_unit=1;
  }
  grib.p1=0;
  grib.p2=0;
  grid.nmean=0;
  grib.nmean_missing=0;
  grib.sub_center=0;
  grid.grid_type=4;
  dim=source.dimensions();
  --dim.x;
  def=source.definition();
  def.type=gaussianLatitudeLongitudeType;
  grib.rescomp=0x80;
  grib.scan_mode=0x0;
  dim.size=dim.x*dim.y;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != nullptr) {
	delete[] bitmap.map;
	bitmap.map=nullptr;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.max_val=-Grid::missing_value;
  stats.min_val=Grid::missing_value;
  stats.avg_val=0.;
  l=dim.y-1;
  for (n=0; n < dim.y; n++,l--) {
    for (m=0; m < dim.x; m++) {
	gridpoints_[n][m]=source.gridpoint(m,l);
	if (floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  bitmap.map[cnt]=0;
	  grid.num_missing++;
	}
	else {
	  bitmap.map[cnt]=1;
	  switch (grid.param) {
	    case 2:
	    {
		gridpoints_[n][m]*=100.;
		break;
	    }
	    case 11:
	    case 15:
	    case 16:
	    {
		gridpoints_[n][m]+=273.15;
		break;
	    }
	    case 61:
	    {
		gridpoints_[n][m]*=86400.;
		break;
	    }
	  }
	  if (gridpoints_[n][m] > stats.max_val) {
	    stats.max_val=gridpoints_[n][m];
	  }
	  if (gridpoints_[n][m] < stats.min_val) {
	    stats.min_val=gridpoints_[n][m];
	  }
	  stats.avg_val+=gridpoints_[n][m];
	  ++avg_cnt;
	}
	++cnt;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  if (grid.param == 2) {
    grib.D=0;
  }
  else {
    grib.D=2;
  }
  grib.pack_width=16;
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  grid.filled=true;
  return *this;
}

bool GRIBGrid::is_averaged_grid() const
{
  switch (grib.t_range) {
    case 51:
    case 113:
    case 115:
    case 117:
    case 123:
    {
	return true;
    }
    default:
    {
	return false;
    }
  }
}

GRIBGrid& GRIBGrid::operator=(const USSRSLPGrid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double range;

  grib.table=3;
  grid.src=4;
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.param=2;
  grid.level1_type=102;
  grid.level1=0.;
  grid.level2_type=-1;
  grid.level2=0.;
  grib.p1=0;
  grib.p2=0;
  reference_date_time_=source.reference_date_time();
  grib.time_unit=1;
  grib.t_range=10;
  grid.nmean=0;
  grib.nmean_missing=0;
  grib.sub_center=0;
  grib.D=2;
  grid.grid_type=0;
  grib.scan_mode=0x0;
  dim=source.dimensions();
  dim.size=dim.x*dim.y;
  def=source.definition();
  grib.rescomp=0x80;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != nullptr) {
	delete[] bitmap.map;
	bitmap.map=nullptr;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::missing_value;
  stats.max_val=-Grid::missing_value;
  for (n=0; n < dim.y; n++) {
    for (m=0; m < dim.x; m++) {
	gridpoints_[n][m]=source.gridpoint(m,n);
	if (floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  bitmap.map[cnt]=0;
	  grid.num_missing++;
	}
	else {
	  bitmap.map[cnt]=1;
	  if (gridpoints_[n][m] > stats.max_val) {
	    stats.max_val=gridpoints_[n][m];
	  }
	  if (gridpoints_[n][m] < stats.min_val) {
	    stats.min_val=gridpoints_[n][m];
	  }
	  stats.avg_val+=gridpoints_[n][m];
	  ++avg_cnt;
	}
	++cnt;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round(stats.max_val-stats.min_val);
  while (max_pack > 0 && max_pack > range) {
    grib.pack_width--;
    max_pack/=2;
  }
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  grid.filled=true;
  return *this;
}

short mapped_parameter_data(short center,size_t disc,short param_cat,short param_num,short& table)
{
  switch (disc) {
    case 0:
    {
// meteorological products
	switch (param_cat) {
	  case 0:
	  {
// temperature parameters
	    switch (param_num) {
		case 0:
		{
		  return 11;
		}
		case 1:
		{
		  return 12;
		}
		case 2:
		{
		  return 13;
		}
		case 3:
		{
		  return 14;
		}
		case 4:
		{
		  return 15;
		}
		case 5:
		{
		  return 16;
		}
		case 6:
		{
		  return 17;
		}
		case 7:
		{
		  return 18;
		}
		case 8:
		{
		  return 19;
		}
		case 9:
		{
		  return 25;
		}
		case 10:
		{
		  return 121;
		}
		case 11:
		{
		  return 122;
		}
		case 12:
		{
		  myerror="there is no GRIB1 parameter code for 'Heat index'";
		  exit(1);
		}
		case 13:
		{
		  myerror="there is no GRIB1 parameter code for 'Wind chill factor'";
		  exit(1);
		}
		case 14:
		{
		  myerror="there is no GRIB1 parameter code for 'Minimum dew point depression'";
		  exit(1);
		}
		case 15:
		{
		  myerror="there is no GRIB1 parameter code for 'Virtual potential temperature'";
		  exit(1);
		}
		case 16:
		{
		  myerror="there is no GRIB1 parameter code for 'Snow phase change heat flux'";
		  exit(1);
		}
		case 21:
		{
		  switch (center) {
		    case 7:
		    {
			table=131;
			return 193;
		    }
		  }
		  myerror="there is no GRIB1 parameter code for 'Apparent temperature'";
		  exit(1);
		}
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 229;
		    }
		  }
		}
	    }
	    break;
	  }
	  case 1:
	  {
// moisture parameters
	    switch (param_num) {
		case 0:
		{
		  return 51;
		}
		case 1:
		{
		  return 52;
		}
		case 2:
		{
		  return 53;
		}
		case 3:
		{
		  return 54;
		}
		case 4:
		{
		  return 55;
		}
		case 5:
		{
		  return 56;
		}
		case 6:
		{
		  return 57;
		}
		case 7:
		{
		  return 59;
		}
		case 8:
		{
		  return 61;
		}
		case 9:
		{
		  return 62;
		}
		case 10:
		{
		  return 63;
		}
		case 11:
		{
		  return 66;
		}
		case 12:
		{
		  return 64;
		}
		case 13:
		{
		  return 65;
		}
		case 14:
		{
		  return 78;
		}
		case 15:
		{
		  return 79;
		}
		case 16:
		{
		  return 99;
		}
		case 17:
		{
		  myerror="there is no GRIB1 parameter code for 'Snow age'";
		  exit(1);
		}
		case 18:
		{
		  myerror="there is no GRIB1 parameter code for 'Absolute humidity'";
		  exit(1);
		}
		case 19:
		{
		  myerror="there is no GRIB1 parameter code for 'Precipitation type'";
		  exit(1);
		}
		case 20:
		{
		  myerror="there is no GRIB1 parameter code for 'Integrated liquid water'";
		  exit(1);
		}
		case 21:
		{
		  myerror="there is no GRIB1 parameter code for 'Condensate water'";
		  exit(1);
		}
		case 22:
		{
		  switch (center) {
		    case 7:
		    {
			return 153;
		    }
		  }
		  myerror="there is no GRIB1 parameter code for 'Cloud mixing ratio'";
		  exit(1);
		}
		case 23:
		{
		  myerror="there is no GRIB1 parameter code for 'Ice water mixing ratio'";
		  exit(1);
		}
		case 24:
		{
		  myerror="there is no GRIB1 parameter code for 'Rain mixing ratio'";
		  exit(1);
		}
		case 25:
		{
		  myerror="there is no GRIB1 parameter code for 'Snow mixing ratio'";
		  exit(1);
		}
		case 26:
		{
		  myerror="there is no GRIB1 parameter code for 'Horizontal moisture convergence'";
		  exit(1);
		}
		case 27:
		{
		  myerror="there is no GRIB1 parameter code for 'Maximum relative humidity'";
		  exit(1);
		}
		case 28:
		{
		  myerror="there is no GRIB1 parameter code for 'Maximum absolute humidity'";
		  exit(1);
		}
		case 29:
		{
		  myerror="there is no GRIB1 parameter code for 'Total snowfall'";
		  exit(1);
		}
		case 30:
		{
		  myerror="there is no GRIB1 parameter code for 'Precipitable water category'";
		  exit(1);
		}
		case 31:
		{
		  myerror="there is no GRIB1 parameter code for 'Hail'";
		  exit(1);
		}
		case 32:
		{
		  myerror="there is no GRIB1 parameter code for 'Graupel (snow pellets)'";
		  exit(1);
		}
		case 33:
		{
		  myerror="there is no GRIB1 parameter code for 'Categorical rain'";
		  exit(1);
		}
		case 34:
		{
		  myerror="there is no GRIB1 parameter code for 'Categorical freezing rain'";
		  exit(1);
		}
		case 35:
		{
		  myerror="there is no GRIB1 parameter code for 'Categorical ice pellets'";
		  exit(1);
		}
		case 36:
		{
		  myerror="there is no GRIB1 parameter code for 'Categorical snow'";
		  exit(1);
		}
		case 37:
		{
		  myerror="there is no GRIB1 parameter code for 'Convective precipitation rate'";
		  exit(1);
		}
		case 38:
		{
		  myerror="there is no GRIB1 parameter code for 'Horizontal moisture divergence'";
		  exit(1);
		}
		case 39:
		{
		  switch (center) {
		    case 7:
		    {
			return 194;
		    }
		  }
		  myerror="there is no GRIB1 parameter code for 'Percent frozen precipitation'";
		  exit(1);
		}
		case 40:
		{
		  myerror="there is no GRIB1 parameter code for 'Potential evaporation'";
		  exit(1);
		}
		case 41:
		{
		  myerror="there is no GRIB1 parameter code for 'Potential evaporation rate'";
		  exit(1);
		}
		case 42:
		{
		  myerror="there is no GRIB1 parameter code for 'Snow cover'";
		  exit(1);
		}
		case 43:
		{
		  myerror="there is no GRIB1 parameter code for 'Rain fraction of total water'";
		  exit(1);
		}
		case 44:
		{
		  myerror="there is no GRIB1 parameter code for 'Rime factor'";
		  exit(1);
		}
		case 45:
		{
		  myerror="there is no GRIB1 parameter code for 'Total column integrated rain'";
		  exit(1);
		}
		case 46:
		{
		  myerror="there is no GRIB1 parameter code for 'Total column integrated snow'";
		  exit(1);
		}
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 140;
		    }
		  }
		}
		case 193:
		{
		  switch (center) {
		    case 7:
		    {
			return 141;
		    }
		  }
		}
		case 194:
		{
		  switch (center) {
		    case 7:
		    {
			return 142;
		    }
		  }
		}
		case 195:
		{
		  switch (center) {
		    case 7:
		    {
			return 143;
		    }
		  }
		}
		case 196:
		{
		  switch (center) {
		    case 7:
		    {
			return 214;
		    }
		  }
		}
		case 197:
		{
		  switch (center) {
		    case 7:
		    {
			return 135;
		    }
		  }
		}
		case 199:
		{
		  switch (center) {
		    case 7:
		    {
			return 228;
		    }
		  }
		}
		case 200:
		{
		  switch (center) {
		    case 7:
		    {
			return 145;
		    }
		  }
		}
		case 201:
		{
		  switch (center) {
		    case 7:
		    {
			return 238;
		    }
		  }
		}
		case 206:
		{
		  switch (center) {
		    case 7:
		    {
			return 186;
		    }
		  }
		}
		case 207:
		{
		  switch (center) {
		    case 7:
		    {
			return 198;
		    }
		  }
		}
		case 208:
		{
		  switch (center) {
		    case 7:
		    {
			return 239;
		    }
		  }
		}
		case 213:
		{
		  switch (center) {
		    case 7:
		    {
			return 243;
		    }
		  }
		}
		case 214:
		{
		  switch (center) {
		    case 7:
		    {
			return 245;
		    }
		  }
		}
		case 215:
		{
		  switch (center) {
		    case 7:
		    {
			return 249;
		    }
		  }
		}
		case 216:
		{
		  switch (center) {
		    case 7:
		    {
			return 159;
		    }
		  }
		}
	    }
	    break;
	  }
	  case 2:
	  {
// momentum parameters
	    switch(param_num) {
		case 0:
		{
		  return 31;
		}
		case 1:
		{
		  return 32;
		}
		case 2:
		{
		  return 33;
		}
		case 3:
		{
		  return 34;
		}
		case 4:
		{
		  return 35;
		}
		case 5:
		{
		  return 36;
		}
		case 6:
		{
		  return 37;
		}
		case 7:
		{
		  return 38;
		}
		case 8:
		{
		  return 39;
		}
		case 9:
		{
		  return 40;
		}
		case 10:
		{
		  return 41;
		}
		case 11:
		{
		  return 42;
		}
		case 12:
		{
		  return 43;
		}
		case 13:
		{
		  return 44;
		}
		case 14:
		{
		  return 4;
		}
		case 15:
		{
		  return 45;
		}
		case 16:
		{
		  return 46;
		}
		case 17:
		{
		  return 124;
		}
		case 18:
		{
		  return 125;
		}
		case 19:
		{
		  return 126;
		}
		case 20:
		{
		  return 123;
		}
		case 21:
		{
		  myerror="there is no GRIB1 parameter code for 'Maximum wind speed'";
		  exit(1);
		}
		case 22:
		{
		  switch (center) {
		    case 7:
			return 180;
		    default:
			myerror="there is no GRIB1 parameter code for 'Wind speed (gust)'";
			exit(1);
		  }
		}
		case 23:
		{
		  myerror="there is no GRIB1 parameter code for 'u-component of wind (gust)'";
		  exit(1);
		}
		case 24:
		{
		  myerror="there is no GRIB1 parameter code for 'v-component of wind (gust)'";
		  exit(1);
		}
		case 25:
		{
		  myerror="there is no GRIB1 parameter code for 'Vertical speed shear'";
		  exit(1);
		}
		case 26:
		{
		  myerror="there is no GRIB1 parameter code for 'Horizontal momentum flux'";
		  exit(1);
		}
		case 27:
		{
		  myerror="there is no GRIB1 parameter code for 'u-component storm motion'";
		  exit(1);
		}
		case 28:
		{
		  myerror="there is no GRIB1 parameter code for 'v-component storm motion'";
		  exit(1);
		}
		case 29:
		{
		  myerror="there is no GRIB1 parameter code for 'Drag coefficient'";
		  exit(1);
		}
		case 30:
		{
		  myerror="there is no GRIB1 parameter code for 'Frictional velocity'";
		  exit(1);
		}
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 136;
		    }
		  }
		}
		case 193:
		{
		  switch (center) {
		    case 7:
		    {
			return 172;
		    }
		  }
		}
		case 194:
		{
		  switch (center) {
		    case 7:
		    {
			return 196;
		    }
		  }
		}
		case 195:
		{
		  switch (center) {
		    case 7:
		    {
			return 197;
		    }
		  }
		}
		case 196:
		{
		  switch (center) {
		    case 7:
		    {
			return 252;
		    }
		  }
		}
		case 197:
		{
		  switch (center) {
		    case 7:
		    {
			return 253;
		    }
		  }
		}
		case 224:
		{
		  switch (center) {
		    case 7:
		    {
			table=129;
			return 241;
		    }
		  }
		}
	    }
	    break;
	  }
	  case 3:
	  {
// mass parameters
	    switch (param_num) {
		case 0:
		{
		  return 1;
		}
		case 1:
		{
		  return 2;
		}
		case 2:
		{
		  return 3;
		}
		case 3:
		{
		  return 5;
		}
		case 4:
		{
		  return 6;
		}
		case 5:
		{
		  return 7;
		}
		case 6:
		{
		  return 8;
		}
		case 7:
		{
		  return 9;
		}
		case 8:
		{
		  return 26;
		}
		case 9:
		{
		  return 27;
		}
		case 10:
		{
		  return 89;
		}
		case 11:
		{
		  myerror="there is no GRIB1 parameter code for 'Altimeter setting'";
		  exit(1);
		}
		case 12:
		{
		  myerror="there is no GRIB1 parameter code for 'Thickness'";
		  exit(1);
		}
		case 13:
		{
		  myerror="there is no GRIB1 parameter code for 'Pressure altitude'";
		  exit(1);
		}
		case 14:
		{
		  myerror="there is no GRIB1 parameter code for 'Density altitude'";
		  exit(1);
		}
		case 15:
		{
		  myerror="there is no GRIB1 parameter code for '5-wave geopotential height'";
		  exit(1);
		}
		case 16:
		{
		  myerror="there is no GRIB1 parameter code for 'Zonal flux of gravity wave stress'";
		  exit(1);
		}
		case 17:
		{
		  myerror="there is no GRIB1 parameter code for 'Meridional flux of gravity wave stress'";
		  exit(1);
		}
		case 18:
		{
		  myerror="there is no GRIB1 parameter code for 'Planetary boundary layer height'";
		  exit(1);
		}
		case 19:
		{
		  myerror="there is no GRIB1 parameter code for '5-wave geopotential height anomaly'";
		  exit(1);
		}
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 130;
		    }
		  }
		}
		case 193:
		{
		  switch (center) {
		    case 7:
		    {
			return 222;
		    }
		  }
		}
		case 194:
		{
		  switch (center) {
		    case 7:
		    {
			return 147;
		    }
		  }
		}
		case 195:
		{
		  switch (center) {
		    case 7:
		    {
			return 148;
		    }
		  }
		}
		case 196:
		{
		  switch (center) {
		    case 7:
		    {
			return 221;
		    }
		  }
		}
		case 197:
		{
		  switch (center) {
		    case 7:
		    {
			return 230;
		    }
		  }
		}
		case 198:
		{
		  switch (center) {
		    case 7:
		    {
			return 129;
		    }
		  }
		}
		case 199:
		{
		  switch (center) {
		    case 7:
		    {
			return 137;
		    }
		  }
		}
		case 200:
		{
		  switch (center) {
		    case 7:
		    {
			table=129;
			return 141;
		    }
		  }
		}
	    }
	    break;
	  }
	  case 4:
	  {
// short-wave radiation parameters
	    switch (param_num) {
		case 0:
		{
		  return 111;
		}
		case 1:
		{
		  return 113;
		}
		case 2:
		{
		  return 116;
		}
		case 3:
		{
		  return 117;
		}
		case 4:
		{
		  return 118;
		}
		case 5:
		{
		  return 119;
		}
		case 6:
		{
		  return 120;
		}
		case 7:
		{
		  myerror="there is no GRIB1 parameter code for 'Downward short-wave radiation flux'";
		  exit(1);
		}
		case 8:
		{
		  myerror="there is no GRIB1 parameter code for 'Upward short-wave radiation flux'";
		  exit(1);
		}
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 204;
		    }
		  }
		}
		case 193:
		{
		  switch (center) {
		    case 7:
		    {
			return 211;
		    }
		  }
		}
		case 196:
		{
		  switch (center) {
		    case 7:
		    {
			return 161;
		    }
		  }
		}
	    }
	    break;
	  }
	  case 5:
	  {
// long-wave radiation parameters
	    switch (param_num) {
		case 0:
		{
		  return 112;
		}
		case 1:
		{
		  return 114;
		}
		case 2:
		{
		  return 115;
		}
		case 3:
		{
		  myerror="there is no GRIB1 parameter code for 'Downward long-wave radiation flux'";
		  exit(1);
		}
		case 4:
		{
		  myerror="there is no GRIB1 parameter code for 'Upward long-wave radiation flux'";
		  exit(1);
		}
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 205;
		    }
		  }
		}
		case 193:
		{
		  switch (center) {
		    case 7:
		    {
			return 212;
		    }
		  }
		}
	    }
	    break;
	  }
	  case 6:
	  {
// cloud parameters
	    switch (param_num) {
		case 0:
		{
		  return 58;
		}
		case 1:
		{
		  return 71;
		}
		case 2:
		{
		  return 72;
		}
		case 3:
		{
		  return 73;
		}
		case 4:
		{
		  return 74;
		}
		case 5:
		{
		  return 75;
		}
		case 6:
		{
		  return 76;
		}
		case 7:
		{
		  myerror="there is no GRIB1 parameter code for 'Cloud amount'";
		  exit(1);
		}
		case 8:
		{
		  myerror="there is no GRIB1 parameter code for 'Cloud type'";
		  exit(1);
		}
		case 9:
		{
		  myerror="there is no GRIB1 parameter code for 'Thunderstorm maximum tops'";
		  exit(1);
		}
		case 10:
		{
		  myerror="there is no GRIB1 parameter code for 'Thunderstorm coverage'";
		  exit(1);
		}
		case 11:
		{
		  myerror="there is no GRIB1 parameter code for 'Cloud base'";
		  exit(1);
		}
		case 12:
		{
		  myerror="there is no GRIB1 parameter code for 'Cloud top'";
		  exit(1);
		}
		case 13:
		{
		  myerror="there is no GRIB1 parameter code for 'Ceiling'";
		  exit(1);
		}
		case 14:
		{
		  myerror="there is no GRIB1 parameter code for 'Non-convective cloud cover'";
		  exit(1);
		}
		case 15:
		{
		  myerror="there is no GRIB1 parameter code for 'Cloud work function'";
		  exit(1);
		}
		case 16:
		{
		  myerror="there is no GRIB1 parameter code for 'Convective cloud efficiency'";
		  exit(1);
		}
		case 17:
		{
		  myerror="there is no GRIB1 parameter code for 'Total condensate'";
		  exit(1);
		}
		case 18:
		{
		  myerror="there is no GRIB1 parameter code for 'Total column-integrated cloud water'";
		  exit(1);
		}
		case 19:
		{
		  myerror="there is no GRIB1 parameter code for 'Total column-integrated cloud ice'";
		  exit(1);
		}
		case 20:
		{
		  myerror="there is no GRIB1 parameter code for 'Total column-integrated cloud condensate'";
		  exit(1);
		}
		case 21:
		{
		  myerror="there is no GRIB1 parameter code for 'Ice fraction of total condensate'";
		  exit(1);
		}
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 213;
		    }
		  }
		}
		case 193:
		{
		  switch (center) {
		    case 7:
		    {
			return 146;
		    }
		  }
		}
		case 201:
		{
		  switch (center) {
		    case 7:
		    {
			table=133;
			return 191;
		    }
		  }
		}
	    }
	    break;
	  }
	  case 7:
	  {
// thermodynamic stability index parameters
	    switch (param_num) {
		case 0:
		{
		  return 24;
		}
		case 1:
		{
		  return 77;
		}
		case 2:
		{
		  myerror="there is no GRIB1 parameter code for 'K index'";
		  exit(1);
		}
		case 3:
		{
		  myerror="there is no GRIB1 parameter code for 'KO index'";
		  exit(1);
		}
		case 4:
		{
		  myerror="there is no GRIB1 parameter code for 'Total totals index'";
		  exit(1);
		}
		case 5:
		{
		  myerror="there is no GRIB1 parameter code for 'Sweat index'";
		  exit(1);
		}
		case 6:
		{
		  switch (center) {
		    case 7:
		    {
			return 157;
		    }
		    default:
		    {
			myerror="there is no GRIB1 parameter code for 'Convective available potential energy'";
			exit(1);
		    }
		  }
		}
		case 7:
		{
		  switch (center) {
		    case 7:
		    {
			return 156;
		    }
		    default:
		    {
			myerror="there is no GRIB1 parameter code for 'Convective inhibition'";
			exit(1);
		    }
		  }
		}
		case 8:
		{
		  switch (center) {
		    case 7:
		    {
			return 190;
		    }
		    default:
		    {
			myerror="there is no GRIB1 parameter code for 'Storm-relative helicity'";
			exit(1);
		    }
		  }
		}
		case 9:
		{
		  myerror="there is no GRIB1 parameter code for 'Energy helicity index'";
		  exit(1);
		}
		case 10:
		{
		  myerror="there is no GRIB1 parameter code for 'Surface lifted index'";
		  exit(1);
		}
		case 11:
		{
		  myerror="there is no GRIB1 parameter code for 'Best (4-layer) lifted index'";
		  exit(1);
		}
		case 12:
		{
		  myerror="there is no GRIB1 parameter code for 'Richardson number'";
		  exit(1);
		}
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 131;
		    }
		  }
		}
		case 193:
		{
		  switch (center) {
		    case 7:
		    {
			return 132;
		    }
		  }
		}
		case 194:
		{
		  switch (center) {
		    case 7:
		    {
			return 254;
		    }
		  }
		}
	    }
	    break;
	  }
	  case 13:
	  {
// aerosol parameters
	    switch (param_num) {
		case 0:
		{
		  myerror="there is no GRIB1 parameter code for 'Aerosol type'";
		  exit(1);
		}
	    }
	    break;
	  }
	  case 14:
	  {
// trace gas parameters
	    switch (param_num) {
		case 0:
		{
		  return 10;
		}
		case 1:
		{
		  myerror="there is no GRIB1 parameter code for 'Ozone mixing ratio'";
		  exit(1);
		}
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 154;
		    }
		  }
		}
	    }
	    break;
	  }
	  case 15:
	  {
// radar parameters
	    switch (param_num) {
		case 0:
		{
		  myerror="there is no GRIB1 parameter code for 'Base spectrum width'";
		  exit(1);
		}
		case 1:
		{
		  myerror="there is no GRIB1 parameter code for 'Base reflectivity'";
		  exit(1);
		}
		case 2:
		{
		  myerror="there is no GRIB1 parameter code for 'Base radial velocity'";
		  exit(1);
		}
		case 3:
		{
		  myerror="there is no GRIB1 parameter code for 'Vertically-integrated liquid'";
		  exit(1);
		}
		case 4:
		{
		  myerror="there is no GRIB1 parameter code for 'Layer-maximum base reflectivity'";
		  exit(1);
		}
		case 5:
		{
		  myerror="there is no GRIB1 parameter code for 'Radar precipitation'";
		  exit(1);
		}
		case 6:
		{
		  return 21;
		}
		case 7:
		{
		  return 22;
		}
		case 8:
		{
		  return 23;
		}
	    }
	    break;
	  }
	  case 18:
	  {
// nuclear/radiology parameters
	    break;
	  }
	  case 19:
	  {
// physical atmospheric property parameters
	    switch (param_num) {
		case 0:
		{
		  return 20;
		}
		case 1:
		{
		  return 84;
		}
		case 2:
		{
		  return 60;
		}
		case 3:
		{
		  return 67;
		}
		case 4:
		{
		  myerror="there is no GRIB1 parameter code for 'Volcanic ash'";
		  exit(1);
		}
		case 5:
		{
		  myerror="there is no GRIB1 parameter code for 'Icing top'";
		  exit(1);
		}
		case 6:
		{
		  myerror="there is no GRIB1 parameter code for 'Icing base'";
		  exit(1);
		}
		case 7:
		{
		  myerror="there is no GRIB1 parameter code for 'Icing'";
		  exit(1);
		}
		case 8:
		{
		  myerror="there is no GRIB1 parameter code for 'Turbulence top'";
		  exit(1);
		}
		case 9:
		{
		  myerror="there is no GRIB1 parameter code for 'Turbulence base'";
		  exit(1);
		}
		case 10:
		{
		  myerror="there is no GRIB1 parameter code for 'Turbulence'";
		  exit(1);
		}
		case 11:
		{
		  myerror="there is no GRIB1 parameter code for 'Turbulent kinetic energy'";
		  exit(1);
		}
		case 12:
		{
		  myerror="there is no GRIB1 parameter code for 'Planetary boundary layer regime'";
		  exit(1);
		}
		case 13:
		{
		  myerror="there is no GRIB1 parameter code for 'Contrail intensity'";
		  exit(1);
		}
		case 14:
		{
		  myerror="there is no GRIB1 parameter code for 'Contrail engine type'";
		  exit(1);
		}
		case 15:
		{
		  myerror="there is no GRIB1 parameter code for 'Contrail top'";
		  exit(1);
		}
		case 16:
		{
		  myerror="there is no GRIB1 parameter code for 'Contrail base'";
		  exit(1);
		}
		case 17:
		{
		  myerror="there is no GRIB1 parameter code for 'Maximum snow albedo'";
		  exit(1);
		}
		case 18:
		{
		  myerror="there is no GRIB1 parameter code for 'Snow-free albedo'";
		  exit(1);
		}
		case 204:
		{
		  switch (center) {
		    case 7:
		    {
			return 209;
		    }
		  }
		}
	    }
	    break;
	  }
	}
	break;
    }
    case 1:
    {
// hydrologic products
	switch (param_cat) {
	  case 0:
	  {
// hydrology basic products
	    switch (param_num) {
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 234;
		    }
		  }
		}
		case 193:
		{
		  switch (center) {
		    case 7:
		    {
			return 235;
		    }
		  }
		}
	    }
	  }
	  case 1:
	  {
	    switch (param_num) {
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 194;
		    }
		  }
		}
		case 193:
		{
		  switch (center) {
		    case 7:
		    {
			return 195;
		    }
		  }
		}
	    }
	    break;
	  }
	}
    }
    case 2:
    {
// land surface products
	switch (param_cat) {
	  case 0:
	  {
// vegetation/biomass
	    switch (param_num) {
		case 0:
		{
		  return 81;
		}
		case 1:
		{
		  return 83;
		}
		case 2:
		{
		  return 85;
		}
		case 3:
		{
		  return 86;
		}
		case 4:
		{
		  return 87;
		}
		case 5:
		{
		  return 90;
		}
		case 192:
		{
		  switch (center) {
		    case 7:
		    {
			return 144;
		    }
		  }
		}
		case 193:
		{
		  switch (center) {
		    case 7:
		    {
			return 155;
		    }
		  }
		}
		case 194:
		{
		  switch (center) {
		    case 7:
		    {
			return 207;
		    }
		  }
		}
		case 195:
		{
		  switch (center) {
		    case 7:
		    {
			return 208;
		    }
		  }
		}
		case 196:
		{
		  switch (center) {
		    case 7:
		    {
			return 223;
		    }
		  }
		}
		case 197:
		{
		  switch (center) {
		    case 7:
		    {
			return 226;
		    }
		  }
		}
		case 198:
		{
		  switch (center) {
		    case 7:
		    {
			return 225;
		    }
		  }
		}
		case 201:
		{
		  switch (center) {
		    case 7:
		    {
			table=130;
			return 219;
		    }
		  }
		}
		case 207:
		{
		  switch (center) {
		    case 7:
		    {
			return 201;
		    }
		  }
		}
	    }
	    break;
	  }
	  case 3:
	  {
// soil products
 	    switch (param_num) {
		case 203:
		{
		  switch (center) {
		    case 7:
		    {
			table=130;
			return 220;
		    }
		  }
		}
 	    }
	    break;
	  }
	  case 4:
	  {
// fire weather
	    switch (param_num) {
		case 2:
		{
		  switch (center) {
		    case 7:
		    {
			table=129;
			return 250;
		    }
		  }
		}
	    }
	    break;
	  }
	}
	break;
    }
    case 10:
    {
// oceanographic products
	switch (param_cat) {
	  case 0:
	  {
// waves parameters
	    switch (param_num) {
		case 0:
		{
		  return 28;
		}
		case 1:
		{
		  return 29;
		}
		case 2:
		{
		  return 30;
		}
		case 3:
		{
		  return 100;
		}
		case 4:
		{
		  return 101;
		}
		case 5:
		{
		  return 102;
		}
		case 6:
		{
		  return 103;
		}
		case 7:
		{
		  return 104;
		}
		case 8:
		{
		  return 105;
		}
		case 9:
		{
		  return 106;
		}
		case 10:
		{
		  return 107;
		}
		case 11:
		{
		  return 108;
		}
		case 12:
		{
		  return 109;
		}
		case 13:
		{
		  return 110;
		}
	    }
	    break;
	  }
	  case 1:
	  {
// currents parameters
	    switch (param_num) {
		case 0:
		{
		  return 47;
		}
		case 1:
		{
		  return 48;
		}
		case 2:
		{
		  return 49;
		}
		case 3:
		{
		  return 50;
		}
	    }
	    break;
	  }
	  case 2:
	  {
// ice parameters
	    switch (param_num) {
		case 0:
		{
		  return 91;
		}
		case 1:
		{
		  return 92;
		}
		case 2:
		{
		  return 93;
		}
		case 3:
		{
		  return 94;
		}
		case 4:
		{
		  return 95;
		}
		case 5:
		{
		  return 96;
		}
		case 6:
		{
		  return 97;
		}
		case 7:
		{
		  return 98;
		}
	    }
	    break;
	  }
	  case 3:
	  {
// surface properties parameters
	    switch (param_num) {
		case 0:
		{
		  return 80;
		}
		case 1:
		{
		  return 82;
		}
	    }
	    break;
	  }
	  case 4:
	  {
// sub-surface properties parameters
	    switch (param_num) {
		case 0:
		{
		  return 69;
		}
		case 1:
		{
		  return 70;
		}
		case 2:
		{
		  return 68;
		}
		case 3:
		{
		  return 88;
		}
	    }
	    break;
	  }
	}
	break;
    }
  }
  myerror="there is no GRIB1 parameter code for discipline "+strutils::itos(disc)+", parameter category "+strutils::itos(param_cat)+", parameter number "+strutils::itos(param_num);
  exit(1);
}

void fill_level_data(const GRIB2Grid& grid,short& level_type,double& level1,double& level2)
{
  if (grid.second_level_type() != 255 && grid.first_level_type() != grid.second_level_type()) {
    myerror="unable to indicate a layer bounded by different level types "+strutils::itos(grid.first_level_type())+" and "+strutils::itos(grid.second_level_type())+" in GRIB1";
    exit(1);
  }
  level1=level2=0.;
  switch (grid.first_level_type()) {
    case 1:
    {
	level_type=1;
	break;
    }
    case 2:
    {
	level_type=2;
	break;
    }
    case 3:
    {
	level_type=3;
	break;
    }
    case 4:
    {
	level_type=4;
	break;
    }
    case 5:
    {
	level_type=5;
	break;
    }
    case 6:
    {
	level_type=6;
	break;
    }
    case 7:
    {
	level_type=7;
	break;
    }
    case 8:
    {
	level_type=8;
	break;
    }
    case 9:
    {
	level_type=9;
	break;
    }
    case 20:
    {
	level_type=20;
	break;
    }
    case 100:
    {
	if (grid.second_level_type() == 255) {
	  level_type=100;
	  level1=grid.first_level_value();
	}
	else {
	  level_type=101;
	  level1=grid.first_level_value()/10.;
	  level2=grid.second_level_value()/10.;
	}
	break;
    }
    case 101:
    {
	level_type=102;
	break;
    }
    case 102:
    {
	if (grid.second_level_type() == 255) {
	  level_type=103;
	  level1=grid.first_level_value();
	}
	else {
	  level_type=104;
	  level1=grid.first_level_value()/100.;
	  level2=grid.second_level_value()/100.;
	}
	break;
    }
    case 103:
    {
	if (grid.second_level_type() == 255) {
	  level_type=105;
	  level1=grid.first_level_value();
	}
	else {
	  level_type=106;
	  level1=grid.first_level_value()/100.;
	  level2=grid.second_level_value()/100.;
	}
	break;
    }
    case 104:
    {
	if (grid.second_level_type() == 255) {
	  level_type=107;
	  level1=grid.first_level_value()*10000.;
	}
	else {
	  level_type=108;
	  level1=grid.first_level_value()*100.;
	  level2=grid.second_level_value()*100.;
	}
	break;
    }
    case 105:
    {
	level1=grid.first_level_value();
	if (floatutils::myequalf(grid.second_level_value(),255.)) {
	  level_type=109;
	}
	else {
	  level_type=110;
	  level2=grid.second_level_value();
	}
	break;
    }
    case 106:
    {
	level1=grid.first_level_value()*100.;
	if (grid.second_level_type() == 255) {
	  level_type=111;
	}
	else {
	  level_type=112;
	  level2=grid.second_level_value()*100.;
	}
	break;
    }
    case 107:
    {
	if (grid.second_level_type() == 255) {
	  level_type=113;
	  level1=grid.first_level_value();
	}
	else {
	  level_type=114;
	  level1=475.-grid.first_level_value();
	  level2=475.-grid.second_level_value();
	}
	break;
    }
    case 108:
    {
	level1=grid.first_level_value();
	if (grid.second_level_type() == 255) {
	  level_type=115;
	}
	else {
	  level_type=116;
	  level2=grid.second_level_value();
	}
	break;
    }
    case 109:
    {
	level_type=117;
	level1=grid.first_level_value();
	break;
    }
    case 111:
    {
	if (grid.second_level_type() == 255) {
	  level_type=119;
	  level1=grid.first_level_value()*10000.;
	}
	else {
	  level_type=120;
	  level1=grid.first_level_value()*100.;
	  level2=grid.second_level_value()*100.;
	}
	break;
    }
    case 117:
    {
	myerror="there is no GRIB1 level code for 'Mixed layer depth'";
	exit(1);
    }
    case 160:
    {
	level_type=160;
	level1=grid.first_level_value();
	break;
    }
    case 200:
    {
	switch (grid.source()) {
	  case 7:
	  {
	    level_type=200;
	    break;
	  }
	}
	break;
    }
  }
}

void fill_time_range_data(const GRIB2Grid& grid,short& p1,short& p2,short& t_range,short& nmean,short& nmean_missing)
{
  std::vector<GRIB2Grid::StatisticalProcessRange> stat_process_ranges;
  size_t n;

  switch (grid.product_type()) {
    case 0:
    case 1:
    case 2:
    {
	t_range=0;
	p1=grid.forecast_time();
	p2=0;
	nmean=nmean_missing=0;
	break;
    }
    case 8:
    case 11:
    case 12:
    {
	if (grid.number_of_statistical_process_ranges() > 1) {
	  myerror="unable to map multiple ("+strutils::itos(grid.number_of_statistical_process_ranges())+") statistical processes to GRIB1";
	  exit(1);
	}
	stat_process_ranges=grid.statistical_process_ranges();
	for (n=0; n < stat_process_ranges.size(); n++) {
	  switch(stat_process_ranges[n].type) {
	    case 0:
	    case 1:
	    case 4:
	    {
		switch(stat_process_ranges[n].type) {
		  case 0:
		  {
// average
		    t_range=3;
		    break;
		  }
		  case 1:
		  {
// accumulation
		    t_range=4;
		    break;
		  }
		  case 4:
		  {
// difference
		    t_range=5;
		    break;
		  }
		}
		p1=grid.forecast_time();
		p2=gributils::p2_from_statistical_end_time(grid);
		if (stat_process_ranges[n].period_time_increment.value == 0) {
		  nmean=0;
		}
		else {
		  myerror="unable to map discrete processing to GRIB1";
		  exit(1);
		}
		break;
	    }
	    case 2:
	    case 3:
	    {
// maximum
// minimum
		t_range=2;
		p1=grid.forecast_time();
		p2=gributils::p2_from_statistical_end_time(grid);
		if (stat_process_ranges[n].period_time_increment.value == 0) {
		  nmean=0;
		}
		else {
		  myerror="unable to map discrete processing to GRIB1";
		  exit(1);
		}
		break;
	    }
	    default:
	    {
// patch for NCEP grids
		if (stat_process_ranges[n].type == 255 && grid.source() == 7) {
 		  if (grid.discipline() == 0) {
		    if (grid.parameter_category() == 0) {
			switch (grid.parameter()) {
			  case 4:
			  case 5:
			  {
			    t_range=2;
			    p1=grid.forecast_time();
			    p2=gributils::p2_from_statistical_end_time(grid);
			    if (stat_process_ranges[n].period_time_increment.value == 0) {
				nmean=0;
			    }
			    else {
				myerror="unable to map discrete processing to GRIB1";
				exit(1);
			    }
			    break;
			  }
			}
		    }
		  }
		}
		else {
		  myerror="unable to map statistical process "+strutils::itos(stat_process_ranges[n].type)+" to GRIB1";
		  exit(1);
		}
	    }
	  }
	}
	nmean_missing=grid.number_missing_from_statistical_process();
	break;
    }
    default:
    {
	myerror="unable to map time range for Product Definition Template "+strutils::itos(grid.product_type())+" into GRIB1";
	exit(1);
    }
  }
}

GRIBGrid& GRIBGrid::operator=(const GRIB2Grid& source)
{
  int n,m;

  grib=source.grib;
  grid=source.grid;
  if (!source.ensdata.fcst_type.empty()) {
/*
    if (pds_supp != nullptr)
	delete[] pds_supp;
    if (source.ensdata.id == "ALL") {
	grib.lengths_.pds_supp=ensdata.fcst_type.getLength()+1;
	pds_supp=new unsigned char[grib.lengths_.pds_supp];
	memcpy(pds_supp,ensdata.fcst_type.toChar(),ensdata.fcst_type.getLength());
    }
    else {
	grib.lengths_.pds_supp=ensdata.fcst_type.getLength()+2;
	pds_supp=new unsigned char[grib.lengths_.pds_supp];
	memcpy(pds_supp,(ensdata.fcst_type+ensdata.id).toChar(),ensdata.fcst_type.getLength()+1);
    }
    pds_supp[grib.lengths_.pds_supp-1]=(unsigned char)ensdata.total_size;
    grib.lengths_.pds=40+grib.lengths_.pds_supp;
*/
  }
  grib.table=3;
  grib.grid_catalog_id=255;
  grid.param=mapped_parameter_data(source.grid.src,source.discipline(),source.parameter_category(),source.grid.param,grib.table);
  fill_level_data(source,grid.level1_type,grid.level1,grid.level2);
  reference_date_time_=source.reference_date_time();
  fill_time_range_data(source,grib.p1,grib.p2,grib.t_range,grid.nmean,grib.nmean_missing);
  reference_date_time_=source.reference_date_time_;
  switch (source.grid.grid_type) {
    case 0:
    {
// lat-lon
	grid.grid_type=0;
	dim=source.dim;
	def=source.def;
	grib.rescomp=0;
	if (source.grib.rescomp == 0x20) {
	  grib.rescomp|=0x80;
	}
	if (source.earth_shape() == 2) {
	  grib.rescomp|=0x40;
	}
	if ( (source.grib.rescomp&0x8) == 0x8) {
	  grib.rescomp|=0x8;
	}
	break;
    }
    case 30:
    {
// lambert-conformal
	grid.grid_type=3;
	dim=source.dim;
	def=source.def;
	grib.rescomp=0;
	if (source.grib.rescomp == 0x20) {
	  grib.rescomp|=0x80;
	}
	if (source.earth_shape() == 2) {
	  grib.rescomp|=0x40;
	}
	if ( (source.grib.rescomp&0x8) == 0x8) {
	  grib.rescomp|=0x8;
	}
	break;
    }
    default:
    {
	myerror="unable to map Grid Definition Template "+strutils::itos(source.grid.grid_type)+" into GRIB1";
	exit(1);
    }
  }
  if (source.bitmap.applies) {
    if (bitmap.capacity < dim.size) {
	if (bitmap.map != nullptr) {
	  delete[] bitmap.map;
	  bitmap.map=nullptr;
	}
	bitmap.capacity=dim.size;
	bitmap.map=new unsigned char[bitmap.capacity];
    }
    for (n=0; n < static_cast<int>(dim.size); ++n) {
	bitmap.map[n]=source.bitmap.map[n];
    }
    bitmap.applies=true;
  }
  else {
    bitmap.applies=false;
  }
  stats=source.stats;
  galloc();
  if (dim.x < 0) {
    myerror="unable to convert reduced grids";
    exit(1);
  }
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	gridpoints_[n][m]=source.gridpoints_[n][m];
    }
  }
  return *this;
}

bool GRIBGrid::is_accumulated_grid() const
{
  switch (grib.t_range) {
    case 114:
    case 116:
    case 124:
    {
	return true;
    }
    default:
    {
	return false;
    }
  }
}

void GRIBGrid::print(std::ostream& outs) const
{
  int n;
  float lon_print;
  int i,j,k,cnt,max_i;
  bool scientific=false;
  GLatEntry glat_entry;
  my::map<Grid::GLatEntry> gaus_lats;

  outs.setf(std::ios::fixed);
  outs.precision(2);
  if (grid.filled) {
    if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
	scientific=true;
    }
    if (def.type == Grid::gaussianLatitudeLongitudeType) {
	if (!gridutils::fill_gaussian_latitudes(_path_to_gauslat_lists,gaus_lats,def.num_circles,(grib.scan_mode&0x40) != 0x40)) {
	  myerror="unable to get gaussian latitudes for "+strutils::itos(def.num_circles)+" circles";
	  exit(0);
	}
    }
    switch (def.type) {
	case Grid::latitudeLongitudeType:
	case Grid::gaussianLatitudeLongitudeType:
	{
// non-reduced grid
	  if (dim.x != -1) {
	    for (n=0; n < dim.x; n+=14) {
		outs << "\n    \\ LON ";
		lon_print=def.slongitude+n*def.loincrement;
		if (def.slongitude > def.elongitude) {
		  lon_print-=360.;
		}
		outs.precision(4);
		cnt=0;
		while (cnt < 14 && lon_print <= def.elongitude) {
		  outs << std::setw(10) << lon_print;
		  lon_print+=def.loincrement;
		  ++cnt;
		}
		outs.precision(2);
		outs << "\n LAT \\  +-";
		for (i=0; i < cnt; i++)
		  outs << "----------";
		outs << std::endl;
		switch (grib.scan_mode) {
		  case 0x0:
		  {
		    for (j=0; j < dim.y; j++) {
			if (def.type == Grid::latitudeLongitudeType) {
			  outs.precision(3);
			  outs << std::setw(7) << (def.slatitude-j*def.laincrement) << " | ";
			  outs.precision(2);
			}
			else if (def.type == Grid::gaussianLatitudeLongitudeType) {
			  outs.precision(3);
			  outs << std::setw(7) << latitude_at(j,&gaus_lats) << " | ";
			  outs.precision(2);
			}
			else
			  outs << std::setw(7) << j+1 << " | ";
			if (scientific) {
			  outs.unsetf(std::ios::fixed);
			  outs.setf(std::ios::scientific);
			  outs.precision(3);
			}
			else {
			  if (grib.D > 0)
			    outs.precision(grib.D);
			  else
			    outs.precision(0);
			}
			for (i=n; i < n+cnt; i++) {
			  if (gridpoints_[j][i] >= Grid::missing_value)
			    outs << "   MISSING";
			  else
			    outs << std::setw(10) << gridpoints_[j][i];
			}
			if (scientific) {
			  outs.unsetf(std::ios::scientific);
			  outs.setf(std::ios::fixed);
			}
			outs.precision(2);
			outs << std::endl;
		    }
		    break;
		  }
		  case 0x40:
		  {
		    for (j=dim.y-1; j >= 0; j--) {
			if (def.type == Grid::latitudeLongitudeType) {
			  outs.precision(3);
			  outs << std::setw(7) << (def.elatitude-(dim.y-j-1)*def.laincrement) << " | ";
			  outs.precision(2);
			}
			else if (def.type == Grid::gaussianLatitudeLongitudeType) {
			  outs.precision(3);
			  outs << std::setw(7) << latitude_at(j,&gaus_lats) << " | ";
			  outs.precision(2);
			}
			else
			  outs << std::setw(7) << dim.y-j << " | ";
			if (scientific) {
			  outs.unsetf(std::ios::fixed);
			  outs.setf(std::ios::scientific);
			  outs.precision(3);
			}
			else
			  outs.precision(grib.D);
			for (i=n; i < n+cnt; i++) {
			  if (gridpoints_[j][i] >= Grid::missing_value)
			    outs << "   MISSING";
			  else
			    outs << std::setw(10) << gridpoints_[j][i];
			}
			if (scientific) {
			  outs.unsetf(std::ios::scientific);
			  outs.setf(std::ios::fixed);
			}
			outs.precision(2);
			outs << std::endl;
		    }
		    break;
		  }
		}
	    }
	  }
// reduced grid
	  else {
	    glat_entry.key=def.num_circles;
	    gaus_lats.found(glat_entry.key,glat_entry);
	    for (j=0; j < dim.y; j++) {
		std::cout << "\nLAT\\LON";
		for (i=1; i <= static_cast<int>(gridpoints_[j][0]); ++i) {
		  std::cout << std::setw(10) << (def.elongitude-def.slongitude)/(gridpoints_[j][0]-1.)*static_cast<float>(i-1);
		}
		std::cout << std::endl;
		std::cout << std::setw(7) << glat_entry.lats[j];
		for (i=1; i <= static_cast<int>(gridpoints_[j][0]); ++i) {
		  std::cout << std::setw(10) << gridpoints_[j][i];
		}
		std::cout << std::endl;
	    }
	  }
	  break;
	}
	case Grid::polarStereographicType:
	case Grid::lambertConformalType:
	{
	  for (i=0; i < dim.x; i+=14) {
	    max_i=i+14;
	    if (max_i > dim.x) max_i=dim.x;
	    outs << "\n   \\ X";
	    for (k=i; k < max_i; ++k) {
		outs << std::setw(10) << k+1;
	    }
	    outs << "\n Y  +-";
	    for (k=i; k < max_i; ++k) {
		outs << "----------";
	    }
	    outs << std::endl;
	    switch (grib.scan_mode) {
		case 0x0:
		{
		  for (j=0; j < dim.y; ++j) {
		    outs << std::setw(3) << dim.y-j << " | ";
		    for (k=i; k < max_i; ++k) {
			if (floatutils::myequalf(gridpoints_[j][k],Grid::missing_value)) {
			  outs << "   MISSING";
			}
			else {
			  if (scientific) {
			    outs.unsetf(std::ios::fixed);
			    outs.setf(std::ios::scientific);
			    outs.precision(3);
			    outs << std::setw(10) << gridpoints_[j][k];
			    outs.unsetf(std::ios::scientific);
			    outs.setf(std::ios::fixed);
			    outs.precision(2);
			  }
			  else {
			    outs << std::setw(10) << gridpoints_[j][k];
			  }
			}
		    }
		    outs << std::endl;
		  }
		  break;
		}
		case 0x40:
		{
		  for (j=dim.y-1; j >= 0; --j) {
		    outs << std::setw(3) << j+1 << " | ";
		    for (k=i; k < max_i; k++) {
			if (floatutils::myequalf(gridpoints_[j][k],Grid::missing_value)) {
			  outs << "   MISSING";
			}
			else {
			  if (scientific) {
			    outs.unsetf(std::ios::fixed);
			    outs.setf(std::ios::scientific);
			    outs.precision(3);
			    outs << std::setw(10) << gridpoints_[j][k];
			    outs.unsetf(std::ios::scientific);
			    outs.setf(std::ios::fixed);
			    outs.precision(2);
			  }
			  else {
			    outs << std::setw(10) << gridpoints_[j][k];
			  }
			}
		    }
		    outs << std::endl;
		  }
		  break;
		}
	    }
	  }
	  break;
	}
    }
    outs << std::endl;
  }
}

void GRIBGrid::print(std::ostream& outs,float bottom_latitude,float top_latitude,float west_longitude,float east_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void GRIBGrid::print_ascii(std::ostream& outs) const
{
}

std::string GRIBGrid::build_level_search_key() const
{
  return strutils::itos(grid.src)+"-"+strutils::itos(grib.sub_center);
}

std::string GRIBGrid::first_level_description(xmlutils::LevelMapper& level_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return level_mapper.description("WMO_GRIB"+strutils::itos(edition),strutils::itos(grid.level1_type),build_level_search_key());
}

std::string GRIBGrid::first_level_units(xmlutils::LevelMapper& level_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return level_mapper.units("WMO_GRIB"+strutils::itos(edition),strutils::itos(grid.level1_type),build_level_search_key());
}

std::string GRIBGrid::build_parameter_search_key() const
{
  return strutils::itos(grid.src)+"-"+strutils::itos(grib.sub_center)+"."+strutils::itos(grib.table);
}

std::string GRIBGrid::parameter_cf_keyword(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return parameter_mapper.cf_keyword("WMO_GRIB"+strutils::itos(edition),build_parameter_search_key()+":"+strutils::itos(grid.param));
}

std::string GRIBGrid::parameter_description(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return parameter_mapper.description("WMO_GRIB"+strutils::itos(edition),build_parameter_search_key()+":"+strutils::itos(grid.param));
}

std::string GRIBGrid::parameter_short_name(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return parameter_mapper.short_name("WMO_GRIB"+strutils::itos(edition),build_parameter_search_key()+":"+strutils::itos(grid.param));
}

std::string GRIBGrid::parameter_units(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return parameter_mapper.units("WMO_GRIB"+strutils::itos(edition),build_parameter_search_key()+":"+strutils::itos(grid.param));
}

void GRIBGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  static xmlutils::ParameterMapper parameter_mapper(path_to_parameter_map);
  bool scientific=false;

  outs.setf(std::ios::fixed);
  outs.precision(2);
  if (!floatutils::myequalf(stats.min_val,0.) && fabs(stats.min_val) < 0.01) {
    scientific=true;
  }
  if (verbose) {
    outs << "  Center: " << std::setw(3) << grid.src;
    if (grib.table >= 0) {
	outs << "-" << std::setw(2) << grib.sub_center << "  Table: " << std::setw(3) << grib.table;
    }
    outs << "  Process: " << std::setw(3) << grib.process << "  GridID: " << std::setw(3) << grib.grid_catalog_id << "  DataBits: " << std::setw(3) << grib.pack_width << std::endl;
    outs << "  RefTime: " << reference_date_time_.to_string() << "  Fcst: " << std::setw(4) << std::setfill('0') << grid.fcst_time/100 << std::setfill(' ') << "  NumAvg: " << std::setw(3) << grid.nmean-grib.nmean_missing << "  TimeRng: " << std::setw(3) << grib.t_range << "  P1: " << std::setw(3) << grib.p1;
    if (grib.t_range != 10) {
	outs << " P2: " << std::setw(3) << grib.p2;
    }
    outs << " Units: ";
    if (grib.time_unit < 13) {
	outs << time_units[grib.time_unit];
    }
    else {
	outs << grib.time_unit;
    }
    if (!ensdata.id.empty()) {
	outs << " Ensemble: " << ensdata.id;
	if (ensdata.id == "ALL" && ensdata.total_size > 0) {
	  outs << "/" << ensdata.total_size;
	}
	outs << "/" << ensdata.fcst_type;
    }
    outs << "  Param: " << std::setw(3) << grid.param << "(" << parameter_short_name(parameter_mapper) << ")";
    if (grid.level1_type < 100 || grid.level1_type == 102 || grid.level1_type == 200 || grid.level1_type == 201) {
	outs << "  Level: " << level_type_short_name[grid.level1_type];
    }
    else {
	switch (grid.level1_type) {
	  case 100:
	  case 103:
	  case 105:
	  case 109:
	  case 111:
	  case 113:
	  case 115:
	  case 117:
	  case 119:
	  case 125:
	  case 160:
	  {
	    outs << "  Level: " << std::setw(5) << grid.level1 << level_type_units[grid.level1_type] << " " << level_type_short_name[grid.level1_type];
	    break;
	  }
	  case 107:
	  {
	    outs.precision(4);
	    outs << "  Level: " << std::setw(6) << grid.level1 << " " << level_type_short_name[grid.level1_type];
	    outs.precision(2);
	    break;
	  }
	  case 101:
	  case 104:
	  case 106:
	  case 110:
	  case 112:
	  case 114:
	  case 116:
	  case 120:
	  case 121:
	  case 141:
	  {
	    outs << "  Layer- Top: " << std::setw(5) << grid.level1 << level_type_units[grid.level1_type] << " Bottom: " << std::setw(5) << grid.level2 << level_type_units[grid.level1_type] << " " << level_type_short_name[grid.level1_type];
	    break;
	  }
	  case 108:
	  {
	    outs << "  Layer- Top: " << std::setw(5) << grid.level1 << " Bottom: " << std::setw(5) << grid.level2 << " " << level_type_short_name[grid.level1_type];
	    break;
	  }
	  case 128:
	  {
	    outs.precision(3);
	    outs << "  Layer- Top: " << std::setw(5) << 1.1-grid.level1 << " Bottom: " << std::setw(5) << 1.1-grid.level2 << " " << level_type_short_name[grid.level1_type];
	    outs.precision(2);
	    break;
	  }
	  default:
	  {
	    outs << "  Level/Layer: " << std::setw(3) << grid.level1_type << " " << std::setw(5) << grid.level1 << " " << std::setw(5) << grid.level2;
	  }
	}
    }
    switch (grid.grid_type) {
	case 0:
	{
// Latitude/Longitude
	  outs.precision(3);
	  outs << "\n  Grid: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type << "  LonRange: " << std::setw(7) << def.slongitude << " to " << std::setw(3) << def.elongitude << " by " << std::setw(5) << def.loincrement << "  LatRange: " << std::setw(3) << def.slatitude << " to " << std::setw(7) << def.elatitude << " by " << std::setw(5) << def.laincrement << "  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
	  outs.precision(2);
	  break;
	}
	case 4:
	case 10:
	{
// Gaussian Lat/Lon
// Rotated Lat/Lon
	  outs.precision(3);
	  outs << "\n  Grid: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type << "  LonRange: " << std::setw(3) << def.slongitude << " to " << std::setw(7) << def.elongitude << " by " << std::setw(5) << def.loincrement << "  LatRange: " << std::setw(7) << def.slatitude << " to " << std::setw(7) << def.elatitude << "  Circles: " << std::setw(3) << def.num_circles << "  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
	  outs.precision(2);
	  break;
	}
	case 3:
	case 5:
	{
// Lambert Conformal
// Polar Stereographic
	  outs.precision(3);
	  outs << "\n  Grid: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type << "  Lat: " << std::setw(7) << def.slatitude << " Lon: " << std::setw(7) << def.slongitude << " OrientLon: " << std::setw(7) << def.olongitude << " Proj: 0x" << std::hex << def.projection_flag << std::dec << "  Dx: " << std::setw(7) << def.dx << "km Dy: " << std::setw(7) << def.dy << "km  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
	  if (grid.grid_type == 3) {
	    outs << std::endl;
	    outs << "    Standard Parallels: " << std::setw(7) << def.stdparallel1 << " " << std::setw(7) << def.stdparallel2 << "  (Lat,Lon) of Southern Pole: (" << std::setw(7) << grib.sp_lat << "," << std::setw(7) << grib.sp_lon << ")";
	  }
	  outs.precision(2);
	  break;
	}
	case 50:
	{
	  outs << "\n  Type: " << std::setw(3) << grid.grid_type << "  Truncation Parameters: " << std::setw(3) << def.trunc1 << std::setw(5) << def.trunc2 << std::setw(5) << def.trunc3;
	  break;
	}
    }
    outs << std::endl;
    outs << "  Decimal Scale (D): " << std::setw(3) << grib.D << "  Scale (E): " << std::setw(3) << grib.E << "  RefVal: ";
    if (scientific) {
	outs.unsetf(std::ios::fixed);
	outs.setf(std::ios::scientific);
	outs.precision(3);
	outs << stats.min_val;
	if (grid.filled) {
	  outs << " (" << stats.min_i << "," << stats.min_j << ")  MaxVal: " << stats.max_val << " (" << stats.max_i << "," << stats.max_j << ")  AvgVal: " << stats.avg_val;
	}
	outs.unsetf(std::ios::scientific);
	outs.setf(std::ios::fixed);
	outs.precision(2);
    }
    else {
	outs << std::setw(9) << stats.min_val;
	if (grid.filled) {
	  outs << " (" << stats.min_i << "," << stats.min_j << ")  MaxVal: " << std::setw(9) << stats.max_val << " (" << stats.max_i << "," << stats.max_j << ")  AvgVal: " << std::setw(9) << stats.avg_val;
	}
    }
    outs << std::endl;
  }
  else {
    if (grib.table < 0) {
	outs << " Ctr=" << grid.src;
    }
    else {
	outs << " Tbl=" << grib.table << " Ctr=" << grid.src << "-" << grib.sub_center;
    }
    outs << " ID=" << grib.grid_catalog_id << " RTime=" << reference_date_time_.to_string("%Y%m%d%H%MM") << " Fcst=" << std::setw(4) << std::setfill('0') << grid.fcst_time/100 << std::setfill(' ') << " NAvg=" << grid.nmean-grib.nmean_missing << " TRng=" << grib.t_range << " P1=" << grib.p1;
    if (grib.t_range != 10) {
	outs << " P2=" << grib.p2;
    }
    outs << " Units=";
    if (grib.time_unit < 13) {
	outs << time_units[grib.time_unit];
    }
    else {
	outs << grib.time_unit;
    }
    if (!ensdata.id.empty()) {
	outs << " Ens=" << ensdata.id;
	if (ensdata.id == "ALL" && ensdata.total_size > 0) {
	  outs << "/" << ensdata.total_size;
	}
	outs << "/" << ensdata.fcst_type;
    }
    outs << " Param=" << grid.param;
    if (grid.level1_type < 100 || grid.level1_type == 102 || grid.level1_type == 200 || grid.level1_type == 201) {
	outs << " Level=" << level_type_short_name[grid.level1_type];
    }
    else {
	switch (grid.level1_type) {
	  case 100:
	  case 103:
	  case 105:
	  case 109:
	  case 111:
	  case 113:
	  case 115:
	  case 117:
	  case 119:
	  case 125:
	  case 160:
	  {
	    outs << " Level=" << grid.level1 << level_type_units[grid.level1_type];
	    break;
	  }
	  case 107:
	  {
	    outs.precision(4);
	    outs << " Level=" << grid.level1;
	    outs.precision(2);
	    break;
	  }
	  case 101:
	  case 104:
	  case 106:
	  case 110:
	  case 112:
	  case 114:
	  case 116:
	  case 120:
	  case 121:
	  case 141:
	  {
	    outs << " Layer=" << grid.level1 << level_type_units[grid.level1_type] << "," << grid.level2 << level_type_units[grid.level1_type];
	    break;
	  }
	  case 108:
	  {
	    outs << " Layer=" << grid.level1 << "," << grid.level2;
	    break;
	  }
	  case 128:
	  {
	    outs.precision(3);
	    outs << " Layer=" << 1.1-grid.level1 << "," << 1.1-grid.level2;
	    outs.precision(2);
	    break;
	  }
	  default:
	  {
	    outs << " Levels=" << grid.level1_type << "," << grid.level1 << "," << grid.level2;
	  }
	}
    }
    if (scientific) {
	outs.unsetf(std::ios::fixed);
	outs.setf(std::ios::scientific);
	outs.precision(3);
	outs << " R=" << stats.min_val;
	if (grid.filled)
	  outs << " Max=" << stats.max_val << " Avg=" << stats.avg_val;
	outs << std::endl;
	outs.unsetf(std::ios::scientific);
	outs.setf(std::ios::fixed);
	outs.precision(2);
    }
    else {
	outs << " R=" << stats.min_val;
	if (grid.filled)
	  outs << " Max=" << stats.max_val << " Avg=" << stats.avg_val;
	outs << std::endl;
    }
  }
}

void GRIBGrid::reverse_scan()
{
  float temp;
  int n,m;
  size_t l,bcnt;

  switch (grib.scan_mode) {
    case 0x0:
    case 0x40:
    {
	temp=def.slatitude;
	def.slatitude=def.elatitude;
	def.elatitude=temp;
	grib.scan_mode=(0x40-grib.scan_mode);
	for (n=0; n < dim.y/2; ++n) {
	  for (m=0; m < dim.x; ++m) {
	    temp=gridpoints_[n][m];
	    l=dim.y-n-1;
	    gridpoints_[n][m]=gridpoints_[l][m];
	    gridpoints_[l][m]=temp;
	  }
	}
	if (bitmap.map != nullptr) {
	  bcnt=0;
	  for (n=0; n < dim.y; ++n) {
	    for (m=0; m < dim.x; ++m) {
		if (floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
		  bitmap.map[bcnt]=0;
		}
		else {
		  bitmap.map[bcnt]=1;
		}
		++bcnt;
	    }
	  }
	}
	break;
    }
  }
}

GRIBGrid fabs(const GRIBGrid& source)
{
  GRIBGrid fabs_grid;
  fabs_grid=source;
  if (fabs_grid.grib.capacity.points == 0) {
    return fabs_grid;
  }
  fabs_grid.grid.pole=::fabs(fabs_grid.grid.pole);
  fabs_grid.stats.max_val=-Grid::missing_value;
  fabs_grid.stats.min_val=Grid::missing_value;
  size_t avg_cnt=0;
  fabs_grid.stats.avg_val=0.;
  for (int n=0; n < fabs_grid.dim.y; ++n) {
    for (int m=0; m < fabs_grid.dim.x; ++m) {
	if (!floatutils::myequalf(fabs_grid.gridpoints_[n][m],Grid::missing_value)) {
	  fabs_grid.gridpoints_[n][m]=::fabs(fabs_grid.gridpoints_[n][m]);
	  if (fabs_grid.gridpoints_[n][m] > fabs_grid.stats.max_val) {
	    fabs_grid.stats.max_val=fabs_grid.gridpoints_[n][m];
	  }
	  if (fabs_grid.gridpoints_[n][m] < fabs_grid.stats.min_val) {
	    fabs_grid.stats.min_val=fabs_grid.gridpoints_[n][m];
	  }
	  fabs_grid.stats.avg_val+=fabs_grid.gridpoints_[n][m];
	  ++avg_cnt;
	}
    }
  }
  if (avg_cnt > 0) {
    fabs_grid.stats.avg_val/=static_cast<double>(avg_cnt);
  }
  return fabs_grid;
}

GRIBGrid interpolate_gaussian_to_lat_lon(const GRIBGrid& source,std::string path_to_gauslat_lists)
{
  GRIBGrid latlon_grid;
  GRIBGrid::GRIBData grib_data;
  int n,m;
  int ni,si;
  float lat,dyt,dya,dyb;
  my::map<Grid::GLatEntry> gaus_lats;
  Grid::GLatEntry glat_entry;

  if (source.def.type != Grid::gaussianLatitudeLongitudeType)
    return source;
  if (!gridutils::fill_gaussian_latitudes(path_to_gauslat_lists,gaus_lats,source.def.num_circles,(source.grib.scan_mode&0x40) != 0x40)) {
    myerror="unable to get gaussian latitudes for "+strutils::itos(source.def.num_circles)+" circles";
    exit(0);
  }
  grib_data.reference_date_time=source.reference_date_time_;
  grib_data.valid_date_time=source.valid_date_time_;
  grib_data.dim.x=source.dim.x;
  grib_data.dim.y=73;
  grib_data.def.slongitude=source.def.slongitude;
  grib_data.def.elongitude=source.def.elongitude;
  grib_data.def.loincrement=source.def.loincrement;
  if (!gaus_lats.found(source.def.num_circles,glat_entry)) {
    myerror="error getting gaussian latitudes for "+strutils::itos(source.def.num_circles)+" circles";
    exit(1);
  }
  if (source.grib.scan_mode == 0x0) {
    grib_data.def.slatitude=90.;
    while (grib_data.def.slatitude > glat_entry.lats[0]) {
	grib_data.def.slatitude-=2.5;
	grib_data.dim.y-=2;
    }
  }
  else {
    grib_data.def.slatitude=-90.;
    while (grib_data.def.slatitude < glat_entry.lats[0]) {
	grib_data.def.slatitude+=2.5;
	grib_data.dim.y-=2;
    }
  }
  grib_data.def.elatitude=-grib_data.def.slatitude;
  grib_data.def.laincrement=2.5;
  grib_data.dim.size=grib_data.dim.x*grib_data.dim.y;
  grib_data.def.projection_flag=source.def.projection_flag;
  grib_data.def.type=Grid::latitudeLongitudeType;
  grib_data.param_version=source.grib.table;
  grib_data.source=source.grid.src;
  grib_data.process=source.grib.process;
  grib_data.grid_catalog_id=source.grib.grid_catalog_id;
  grib_data.time_unit=source.grib.time_unit;
  grib_data.time_range=source.grib.t_range;
  grib_data.sub_center=source.grib.sub_center;
  grib_data.D=source.grib.D;
  grib_data.grid_type=0;
  grib_data.scan_mode=source.grib.scan_mode;
  grib_data.param_code=source.grid.param;
  grib_data.level_types[0]=source.grid.level1_type;
  grib_data.level_types[1]=-1;
  grib_data.levels[0]=source.grid.level1;
  grib_data.levels[1]=source.grid.level2;
  grib_data.p1=source.grib.p1;
  grib_data.p2=source.grib.p2;
  grib_data.num_averaged=source.grid.nmean;
  grib_data.num_averaged_missing=source.grib.nmean_missing;
  grib_data.pack_width=source.grib.pack_width;
  grib_data.gds_included=true;
  grib_data.gridpoints=new double *[grib_data.dim.y];
  for (n=0; n < grib_data.dim.y; ++n) {
    grib_data.gridpoints[n]=new double[grib_data.dim.x];
  }
  for (n=0; n < grib_data.dim.y; ++n) {
    lat= (grib_data.scan_mode == 0x0) ? grib_data.def.slatitude-n*grib_data.def.laincrement : grib_data.def.slatitude+n*grib_data.def.laincrement;
    ni=source.latitude_index_north_of(lat,&gaus_lats);
    si=source.latitude_index_south_of(lat,&gaus_lats);
    for (m=0; m < grib_data.dim.x; ++m) {
	if (ni >= 0 && si >= 0) {
	  dyt=source.latitude_at(ni,&gaus_lats)-source.latitude_at(si,&gaus_lats);
	  dya=dyt-(source.latitude_at(ni,&gaus_lats)-lat);
	  dyb=dyt-(lat-source.latitude_at(si,&gaus_lats));
	  grib_data.gridpoints[n][m]=(source.gridpoint(m,ni)*dya+source.gridpoint(m,si)*dyb)/dyt;
	}
	else {
grib_data.gridpoints[n][m]=Grid::missing_value;
	}
    }
  }
  latlon_grid.fill_from_grib_data(grib_data);
  return latlon_grid;
}


GRIBGrid pow(const GRIBGrid& source,double exponent)
{
  GRIBGrid pow_grid;
  int n,m;
  size_t avg_cnt=0;

  pow_grid=source;
  if (pow_grid.grib.capacity.points == 0)
    return pow_grid;

  pow_grid.grid.pole=::pow(pow_grid.grid.pole,exponent);

  pow_grid.stats.max_val=::pow(pow_grid.stats.max_val,exponent);
  pow_grid.stats.min_val=::pow(pow_grid.stats.min_val,exponent);
  pow_grid.stats.avg_val=0.;
  for (n=0; n < pow_grid.dim.y; n++) {
    for (m=0; m < pow_grid.dim.x; m++) {
	if (!floatutils::myequalf(pow_grid.gridpoints_[n][m],Grid::missing_value)) {
	  pow_grid.gridpoints_[n][m]=::pow(pow_grid.gridpoints_[n][m],exponent);
	  pow_grid.stats.avg_val+=pow_grid.gridpoints_[n][m];
	  avg_cnt++;
	}
    }
  }
  if (avg_cnt > 0) {
    pow_grid.stats.avg_val/=static_cast<double>(avg_cnt);
  }
  return pow_grid;
}

GRIBGrid sqrt(const GRIBGrid& source)
{
  GRIBGrid sqrt_grid;
  int n,m;
  size_t avg_cnt=0;

  sqrt_grid=source;
  if (sqrt_grid.grib.capacity.points == 0) {
    return sqrt_grid;
  }
  sqrt_grid.grid.pole=::sqrt(sqrt_grid.grid.pole);
  sqrt_grid.stats.max_val=::sqrt(sqrt_grid.stats.max_val);
  sqrt_grid.stats.min_val=::sqrt(sqrt_grid.stats.min_val);
  sqrt_grid.stats.avg_val=0.;
  for (n=0; n < sqrt_grid.dim.y; ++n) {
    for (m=0; m < sqrt_grid.dim.x; ++m) {
	if (!floatutils::myequalf(sqrt_grid.gridpoints_[n][m],Grid::missing_value)) {
	  sqrt_grid.gridpoints_[n][m]=::sqrt(sqrt_grid.gridpoints_[n][m]);
	  sqrt_grid.stats.avg_val+=sqrt_grid.gridpoints_[n][m];
	  ++avg_cnt;
	}
    }
  }
  if (avg_cnt > 0) {
    sqrt_grid.stats.avg_val/=static_cast<double>(avg_cnt);
  }
  return sqrt_grid;
}

GRIBGrid combine_hemispheres(const GRIBGrid& nhgrid,const GRIBGrid& shgrid)
{
  GRIBGrid combined;
  const GRIBGrid *addon;
  int n,m,dim_y;
  size_t l;

  if (nhgrid.reference_date_time() != shgrid.reference_date_time()) {
    return combined;
  }
  if (nhgrid.dim.x != shgrid.dim.x || nhgrid.dim.y != shgrid.dim.y) {
    return combined;
  }
  if (nhgrid.bitmap.map != nullptr || shgrid.bitmap.map != nullptr) {
myerror="no handling of missing data yet";
exit(1);
  }
  combined.grib.capacity.points=nhgrid.dim.size*2-nhgrid.dim.x;
  dim_y=nhgrid.dim.y*2-1;
  combined.dim.x=nhgrid.dim.x;
  combined.gridpoints_=new double *[dim_y];
  for (n=0; n < dim_y; ++n) {
    combined.gridpoints_[n]=new double[combined.dim.x];
  }
  switch (nhgrid.grib.scan_mode) {
    case 0x0:
    {
	combined=nhgrid;
	combined.def.elatitude=shgrid.def.elatitude;
	addon=&shgrid;
	if (shgrid.stats.max_val > combined.stats.max_val) {
	  combined.stats.max_val=shgrid.stats.max_val;
	}
	if (shgrid.stats.min_val < combined.stats.min_val) {
	  combined.stats.min_val=shgrid.stats.min_val;
	}
	break;
    }
    case 0x40:
    {
	combined=shgrid;
	combined.def.elatitude=nhgrid.def.elatitude;
	addon=&nhgrid;
	if (nhgrid.stats.max_val > combined.stats.max_val) {
	  combined.stats.max_val=nhgrid.stats.max_val;
	}
	if (nhgrid.stats.min_val < combined.stats.min_val) {
	  combined.stats.min_val=nhgrid.stats.min_val;
	}
	break;
    }
    default:
    {
	return combined;
    }
  }
  combined.grib.capacity.y=combined.dim.y=dim_y;
  l=0;
  for (n=combined.dim.y/2; n < combined.dim.y; ++n) {
    for (m=0; m < combined.dim.x; ++m) {
	combined.gridpoints_[n][m]=addon->gridpoints_[l][m];
    }
    ++l;
  }
combined.grid.num_missing=0;
  combined.grib.D= (nhgrid.grib.D > shgrid.grib.D) ? nhgrid.grib.D : shgrid.grib.D;
  combined.set_scale_and_packing_width(combined.grib.D,combined.grib.pack_width);
  return combined;
}

GRIBGrid create_subset_grid(const GRIBGrid& source,float bottom_latitude,float top_latitude,float left_longitude,float right_longitude)
{
  GRIBGrid subset_grid;
  int n,m;
  size_t cnt=0,avg_cnt=0;
  my::map<Grid::GLatEntry> gaus_lats;
  Grid::GLatEntry glat_entry;
  size_t lat_index,lon_index;

  if (right_longitude < left_longitude) {
    myerror="bad subset specified";
    exit(1);
  }
  subset_grid.reference_date_time_=source.reference_date_time_;
  subset_grid.valid_date_time_=source.valid_date_time_;
  subset_grid.grib.scan_mode=source.grib.scan_mode;
  subset_grid.def.laincrement=source.def.laincrement;
  subset_grid.def.type=source.def.type;
  if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
    if (source.path_to_gaussian_latitude_data().empty()) {
	myerror="path to gaussian latitude data was not specified";
	exit(0);
    }
    else if (!gridutils::fill_gaussian_latitudes(source.path_to_gaussian_latitude_data(),gaus_lats,subset_grid.def.num_circles,(subset_grid.grib.scan_mode&0x40) != 0x40)) {
	myerror="unable to get gaussian latitudes for "+strutils::itos(subset_grid.def.num_circles)+" circles";
	exit(0);
    }
    glat_entry.key=subset_grid.def.num_circles;
    if (!gaus_lats.found(glat_entry.key,glat_entry)) {
	myerror="unable to subset gaussian grid with "+strutils::itos(subset_grid.def.num_circles)+" latitude circles";
	exit(1);
    }
  }
  switch (subset_grid.grib.scan_mode) {
    case 0x0:
    {
// top down
	if (!floatutils::myequalf(source.gridpoint_at(bottom_latitude,source.def.slongitude),Grid::bad_value)) {
	  subset_grid.def.elatitude=bottom_latitude;
	}
	else {
	  if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
	    subset_grid.def.elatitude=glat_entry.lats[source.latitude_index_south_of(bottom_latitude,&gaus_lats)];
	  }
	  else {
	    subset_grid.def.elatitude=source.def.slatitude-source.latitude_index_south_of(bottom_latitude,&gaus_lats)*subset_grid.def.laincrement;
	  }
	}
	if (!floatutils::myequalf(source.gridpoint_at(top_latitude,source.def.slongitude),Grid::bad_value)) {
	  subset_grid.def.slatitude=top_latitude;
	}
	else {
	  if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
	    subset_grid.def.slatitude=glat_entry.lats[source.latitude_index_north_of(top_latitude,&gaus_lats)];
	  }
	  else {
	    subset_grid.def.slatitude=source.def.slatitude-source.latitude_index_north_of(top_latitude,&gaus_lats)*subset_grid.def.laincrement;
	  }
	}
	if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
	  subset_grid.dim.y=0;
	  for (n=0; n < static_cast<int>(subset_grid.def.num_circles*2); ++n) {
	    if ((glat_entry.lats[n] > subset_grid.def.elatitude && glat_entry.lats[n] < subset_grid.def.slatitude) || floatutils::myequalf(glat_entry.lats[n],subset_grid.def.elatitude) || floatutils::myequalf(glat_entry.lats[n],subset_grid.def.slatitude)) {
		subset_grid.dim.y++;
	    }
	  }
	}
	else {
	  subset_grid.dim.y=lroundf((subset_grid.def.slatitude-subset_grid.def.elatitude)/subset_grid.def.laincrement)+1;
	}
	break;
    }
    case 0x40:
    {
// bottom up
	if (!floatutils::myequalf(source.gridpoint_at(bottom_latitude,source.def.slongitude),Grid::bad_value)) {
	  subset_grid.def.slatitude=bottom_latitude;
	}
	else {
	  if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
	    subset_grid.def.slatitude=glat_entry.lats[source.latitude_index_south_of(bottom_latitude,&gaus_lats)];
	  }
	  else {
	    if (bottom_latitude < source.def.slatitude) {
		subset_grid.def.slatitude=source.def.slatitude;
	    }
	    else {
		subset_grid.def.slatitude=source.def.slatitude+source.latitude_index_south_of(bottom_latitude,&gaus_lats)*subset_grid.def.laincrement;
	    }
	  }
	}
	if (!floatutils::myequalf(source.gridpoint_at(top_latitude,source.def.slongitude),Grid::bad_value)) {
	  subset_grid.def.elatitude=top_latitude;
	}
	else {
	  if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
	    subset_grid.def.elatitude=glat_entry.lats[source.latitude_index_north_of(top_latitude,&gaus_lats)];
	  }
	  else {
	    if (top_latitude > source.def.elatitude) {
		subset_grid.def.elatitude=source.def.elatitude;
	    }
	    else {
		subset_grid.def.elatitude=source.def.slatitude+source.latitude_index_north_of(top_latitude,&gaus_lats)*subset_grid.def.laincrement;
	    }
	  }
	}
	if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
	  subset_grid.dim.y=0;
	  for (n=0; n < static_cast<int>(subset_grid.def.num_circles*2); ++n) {
	    if (glat_entry.lats[n] >= subset_grid.def.slatitude && glat_entry.lats[n] <= subset_grid.def.elatitude) {
		subset_grid.dim.y++;
	    }
	  }
	}
	else {
	  subset_grid.dim.y=lroundf((subset_grid.def.elatitude-subset_grid.def.slatitude)/subset_grid.def.laincrement)+1;
	}
	break;
    }
  }
  if ( (lat_index=source.latitude_index_of(subset_grid.def.slatitude,&gaus_lats)) >= static_cast<size_t>(source.dim.y)) {
    subset_grid.grid.filled=false;
    return subset_grid;
  }
  subset_grid.def.loincrement=source.def.loincrement;
  if (!floatutils::myequalf(source.gridpoint_at(source.def.slatitude,left_longitude),Grid::bad_value)) {
    subset_grid.def.slongitude=left_longitude;
  }
  else {
    subset_grid.def.slongitude=subset_grid.def.loincrement*source.longitude_index_west_of(left_longitude);
  }
  if (!floatutils::myequalf(source.gridpoint_at(source.def.slatitude,right_longitude),Grid::bad_value)) {
    subset_grid.def.elongitude=right_longitude;
  }
  else {
    subset_grid.def.elongitude=subset_grid.def.loincrement*source.longitude_index_east_of(right_longitude);
  }
  if (subset_grid.def.elongitude < subset_grid.def.slongitude) {
    subset_grid.dim.x=lroundf((subset_grid.def.elongitude+360.-subset_grid.def.slongitude)/subset_grid.def.loincrement)+1;
  }
  else {
    subset_grid.dim.x=lroundf((subset_grid.def.elongitude-subset_grid.def.slongitude)/subset_grid.def.loincrement)+1;
  }
  subset_grid.dim.size=subset_grid.dim.x*subset_grid.dim.y;
  subset_grid.grid=source.grid;
  subset_grid.grib=source.grib;
  if (subset_grid.bitmap.capacity < subset_grid.dim.size) {
    if (subset_grid.bitmap.map != nullptr) {
	delete[] subset_grid.bitmap.map;
    }
    subset_grid.bitmap.capacity=subset_grid.dim.size;
    subset_grid.bitmap.map=new unsigned char[subset_grid.bitmap.capacity];
  }
  subset_grid.grid.num_missing=0;
  subset_grid.galloc();
  subset_grid.stats.max_val=-Grid::missing_value;
  subset_grid.stats.min_val=Grid::missing_value;
  subset_grid.stats.avg_val=0.;
  for (n=0; n < subset_grid.dim.y; ++n) {
    for (m=0; m < subset_grid.dim.x; ++m) {
	lon_index=source.longitude_index_of(subset_grid.def.slongitude+m*subset_grid.def.loincrement);
	subset_grid.gridpoints_[n][m]=source.gridpoint(lon_index,lat_index);
	if (floatutils::myequalf(subset_grid.gridpoints_[n][m],Grid::missing_value)) {
	  subset_grid.bitmap.map[cnt]=0;
	  ++subset_grid.grid.num_missing;
	}
	else {
	  subset_grid.bitmap.map[cnt]=1;
	  if (subset_grid.gridpoints_[n][m] > subset_grid.stats.max_val) {
	    subset_grid.stats.max_val=subset_grid.gridpoints_[n][m];
	    subset_grid.stats.max_i=m;
	    subset_grid.stats.max_j=n;
	  }
	  if (subset_grid.gridpoints_[n][m] < subset_grid.stats.min_val) {
	    subset_grid.stats.min_val=subset_grid.gridpoints_[n][m];
	    subset_grid.stats.min_i=m;
	    subset_grid.stats.min_j=n;
	  }
	  subset_grid.stats.avg_val+=subset_grid.gridpoints_[n][m];
	  ++avg_cnt;
	}
	++cnt;
    }
    ++lat_index;
  }
  if (avg_cnt > 0) {
    subset_grid.stats.avg_val/=static_cast<double>(avg_cnt);
  }
  if (subset_grid.grid.num_missing == 0) {
    subset_grid.bitmap.applies=false;
  }
  else {
    subset_grid.bitmap.applies=true;
  }
  subset_grid.set_scale_and_packing_width(source.grib.D,subset_grid.grib.pack_width);
  subset_grid.grid.filled=true;
  return subset_grid;
}

GRIB2Grid& GRIB2Grid::operator=(const GRIB2Grid& source)
{
  GRIBGrid::operator=(static_cast<GRIBGrid>(source));
  grib2=source.grib2;
  return *this;
}

void GRIB2Grid::quick_copy(const GRIB2Grid& source)
{
  if (this == &source) {
    return;
  }
  reference_date_time_=source.reference_date_time_;
  valid_date_time_=source.valid_date_time_;
  dim=source.dim;
  def=source.def;
  stats=source.stats;
  ensdata=source.ensdata;
  grid=source.grid;
  if (source.plist != nullptr) {
    if (plist != nullptr) {
	delete[] plist;
    }
    plist=new int[dim.y];
    for (size_t n=0; n < static_cast<size_t>(dim.y); ++n) {
	plist[n]=source.plist[n];
    }
  }
  else {
    if (plist != nullptr)
	delete[] plist;
    plist=nullptr;
  }
  if (source.bitmap.applies) {
    if (bitmap.capacity < source.bitmap.capacity) {
	if (bitmap.map != nullptr) {
	  delete[] bitmap.map;
	  bitmap.map=nullptr;
	}
	bitmap.capacity=source.bitmap.capacity;
	bitmap.map=new unsigned char[bitmap.capacity];
	for (size_t n=0; n < dim.size; ++n) {
	  bitmap.map[n]=source.bitmap.map[n];
	}
    }
  }
  bitmap.applies=source.bitmap.applies;
  grib=source.grib;
  grib2=source.grib2;
}

std::string GRIB2Grid::build_level_search_key() const
{
  return strutils::itos(grid.src)+"-"+strutils::itos(grib.sub_center);
}

std::string GRIB2Grid::first_level_description(xmlutils::LevelMapper& level_mapper) const
{
  return level_mapper.description("WMO_GRIB2",strutils::itos(grid.level1_type),build_level_search_key());
}

std::string GRIB2Grid::first_level_units(xmlutils::LevelMapper& level_mapper) const
{
  return level_mapper.units("WMO_GRIB2",strutils::itos(grid.level1_type),build_level_search_key());
}

std::string GRIB2Grid::build_parameter_search_key() const
{
  return strutils::itos(grid.src)+"-"+strutils::itos(grib.sub_center)+"."+strutils::itos(grib.table)+"-"+strutils::itos(grib2.local_table);
}

std::string GRIB2Grid::parameter_cf_keyword(xmlutils::ParameterMapper& parameter_mapper) const
{
  auto level_type=strutils::itos(grid.level1_type);
  if (grid.level2_type > 0 && grid.level2_type < 255) {
    level_type+="-"+strutils::itos(grid.level2_type);
  }
  return parameter_mapper.cf_keyword("WMO_GRIB2",build_parameter_search_key()+":"+strutils::itos(grib2.discipline)+"."+strutils::itos(grib2.param_cat)+"."+strutils::itos(grid.param),level_type);
}

std::string GRIB2Grid::parameter_description(xmlutils::ParameterMapper& parameter_mapper) const
{
  return parameter_mapper.description("WMO_GRIB2",build_parameter_search_key()+":"+strutils::itos(grib2.discipline)+"."+strutils::itos(grib2.param_cat)+"."+strutils::itos(grid.param));
}

std::string GRIB2Grid::parameter_short_name(xmlutils::ParameterMapper& parameter_mapper) const
{
  return parameter_mapper.short_name("WMO_GRIB2",build_parameter_search_key()+":"+strutils::itos(grib2.discipline)+"."+strutils::itos(grib2.param_cat)+"."+strutils::itos(grid.param));
}

std::string GRIB2Grid::parameter_units(xmlutils::ParameterMapper& parameter_mapper) const
{
  return parameter_mapper.units("WMO_GRIB2",build_parameter_search_key()+":"+strutils::itos(grib2.discipline)+"."+strutils::itos(grib2.param_cat)+"."+strutils::itos(grid.param));
}

void GRIB2Grid::compute_mean()
{
//  double cnt;

  if (grib2.stat_process_ranges.size() > 0 && grib2.stat_process_ranges[0].type == 1) {
/*
    cnt=(grib2.stat_process_ranges[0].period_length.value+grib2.stat_process_ranges[0].period_time_increment.value)/grib2.stat_process_ranges[0].period_time_increment.value;
    this->divide_by(cnt);
*/
this->divide_by(grid.nmean);
    grib2.stat_process_ranges[0].type=0;
  }
  else {
    myerror="unable to compute the mean";
    exit(1);
  }
}

GRIB2Grid GRIB2Grid::create_subset(double south_latitude,double north_latitude,int latitude_stride,double west_longitude,double east_longitude,int longitude_stride) const
{
  GRIB2Grid new_grid;
  float dval,fdum;
  int n,m,x;
  int start_x=-1,end_x=-1,ex;
  int start_y=-1,end_y=-1;
  my::map<Grid::GLatEntry> gaus_lats;
  Grid::GLatEntry glat_entry;
  double wlon=west_longitude;
  bool crosses_greenwich=false;

// check for filled grid
  if (!grid.filled) {
    myerror="can't create subset from unfilled grid";
    exit(1);
  }
  if (grib.scan_mode != 0x0) {
    myerror="can't create subset for scanning mode "+strutils::itos(grib.scan_mode);
    exit(1);
  }
  new_grid=*this;
  new_grid.grid.src=60;
  new_grid.grib.sub_center=1;
  if (latitude_stride < 1) {
    latitude_stride=1;
  }
  if (longitude_stride < 1) {
    longitude_stride=1;
  }
  switch (def.type) {
    case Grid::latitudeLongitudeType:
    case Grid::gaussianLatitudeLongitudeType:
    {
	dval=(def.elongitude-def.slongitude)/(dim.x-1);
// special case of -180 to +180
	if (static_cast<int>(west_longitude) == -180 && static_cast<int>(east_longitude) == 180) {
	  east_longitude-=dval;
	}
	if (def.slongitude >= 0. && def.elongitude >= 0.) {
	  if (west_longitude < 0.) {
	    west_longitude+=360.;
	  }
	  if (east_longitude < 0.) {
	    east_longitude+=360.;
	  }
	}
	for (m=0; m < dim.x; ++m) {
	  fdum=def.slongitude+m*dval;
	  if (start_x < 0 && fdum >= west_longitude) {
	    start_x=m;
	    new_grid.def.slongitude=fdum;
	  }
	  if (end_x < 0 && fdum > east_longitude) {
	    end_x=m-1;
	    new_grid.def.elongitude=fdum-dval;
	  }
	}
	if (end_x < 0) {
	  end_x=dim.x-1;
	}
	switch (def.type) {
	  case Grid::latitudeLongitudeType:
	  {
	    dval=(def.slatitude-def.elatitude)/(dim.y-1);
	    for (n=0; n < dim.y; ++n) {
		fdum=def.slatitude-n*dval;
		if (start_y < 0 && fdum <= north_latitude) {
		  start_y=n;
		  new_grid.def.slatitude=fdum;
		}
		if (end_y < 0 && fdum < south_latitude) {
		  end_y=n-1;
		  new_grid.def.elatitude=fdum+dval;
		}
	    }
	    if (end_y < 0) {
		end_y=dim.y-1;
	    }
	    break;
	  }
	  case Grid::gaussianLatitudeLongitudeType:
	  {
	    if (!gridutils::fill_gaussian_latitudes(_path_to_gauslat_lists,gaus_lats,def.num_circles,(grib.scan_mode&0x40) != 0x40)) {
		myerror="unable to get gaussian latitudes for "+strutils::itos(def.num_circles)+" circles";
		exit(0);
	    }
	    gaus_lats.found(def.num_circles,glat_entry);
	    for (n=0; n < static_cast<int>(def.num_circles*2); n++) {
		if (start_y < 0 && glat_entry.lats[n] <= north_latitude) {
		  start_y=n;
		  new_grid.def.slatitude=glat_entry.lats[n];
		}
		if (end_y < 0 && glat_entry.lats[n] < south_latitude) {
		  end_y=n-1;
		  new_grid.def.elatitude=glat_entry.lats[n-1];
		}
	    }
	    if (end_y < 0) {
		end_y=n-1;
	    }
	    break;
	  }
	}
	if (end_x < start_x || (end_x == start_x && wlon == -(east_longitude))) {
	  crosses_greenwich=true;
	  new_grid.dim.x=end_x+(dim.x-start_x)+1;
	  new_grid.def.slongitude-=360.;
	}
	else {
	  new_grid.dim.x=end_x-start_x+1;
	}
	new_grid.dim.y=end_y-start_y+1;
	new_grid.dim.size=new_grid.dim.x*new_grid.dim.y;
	new_grid.stats.min_val=Grid::missing_value;
	new_grid.stats.max_val=-Grid::missing_value;
	x=0;
	new_grid.grid.num_missing=0;
	for (n=start_y; n <= end_y; ++n) {
	  if (crosses_greenwich) {
	    ex=dim.x-1;
	  }
	  else {
	    ex=end_x;
	  }
	  for (m=start_x; m <= ex; ++m) {
	    if (new_grid.bitmap.capacity > 0) {
		new_grid.bitmap.map[x]=bitmap.map[n*dim.x+m];
		if (new_grid.bitmap.map[x] == 0) {
		  ++new_grid.grid.num_missing;
		}
		++x;
	    }
	    new_grid.gridpoints_[n-start_y][m-start_x]=gridpoints_[n][m];
	    if (gridpoints_[n][m] < new_grid.stats.min_val) {
		new_grid.stats.min_val=gridpoints_[n][m];
	    }
	    if (gridpoints_[n][m] > new_grid.stats.max_val) {
		new_grid.stats.max_val=gridpoints_[n][m];
	    }
	  }
	  if (crosses_greenwich) {
	    for (m=0; m <= end_x; ++m) {
		if (new_grid.bitmap.capacity > 0) {
		  new_grid.bitmap.map[x]=bitmap.map[n*dim.x+m];
		  if (new_grid.bitmap.map[x] == 0) {
		    ++new_grid.grid.num_missing;
		  }
		  ++x;
		}
		new_grid.gridpoints_[n-start_y][m+dim.x-start_x]=gridpoints_[n][m];
		if (gridpoints_[n][m] < new_grid.stats.min_val) {
		  new_grid.stats.min_val=gridpoints_[n][m];
		}
		if (gridpoints_[n][m] > new_grid.stats.max_val) {
		  new_grid.stats.max_val=gridpoints_[n][m];
		}
	    }
	  }
	}
	break;
    }
    default:
    {
	myerror="GRIB2Grid::create_subset(): unable to create a subset for grid type "+strutils::itos(def.type);
	exit(1);
    }
  }
  return new_grid;
}

void GRIB2Grid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  size_t m;

  outs.setf(std::ios::fixed);
  outs.precision(2);
  if (verbose) {
    outs << "  Center: " << std::setw(3) << grid.src << "-" << std::setw(2) << grib.sub_center << "  Tables: " << std::setw(3) << grib.table << "." << std::setw(2) << grib2.local_table << "  RefTime: " << reference_date_time_.to_string() << "  TimeSig: " << grib2.time_sig << "  DRS Template: " << grib2.data_rep << "    Valid Time: ";
    if (grib2.stat_period_end.year() != 0)
	outs << reference_date_time_.to_string() << " to " << grib2.stat_period_end.to_string();
    else
	outs << valid_date_time_.to_string();
    outs << "  Dimensions: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type;
    switch (grid.grid_type) {
	case 0:
	case 40:
	{
	  outs.precision(6);
	  outs << "  LonRange: " << std::setw(11) << def.slongitude << " to " << std::setw(11) << def.elongitude << " by " << std::setw(8) << def.loincrement << "  LatRange: " << std::setw(10) << def.slatitude << " to " << std::setw(10) << def.elatitude;
	  switch (grid.grid_type) {
	    case 0:
	    {
		outs << " by " << std::setw(8) << def.laincrement;
		break;
	    }
	    case 40:
	    {
		outs << " Circles: " << std::setw(4) << def.num_circles;
		break;
	    }
	  }
	  outs << "  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
	  outs.precision(2);
	  break;
	}
    }
    outs << "  Param: " << std::setw(3) << grib2.discipline << "/" << std::setw(3) << grib2.param_cat << "/" << std::setw(3) << grid.param;
    switch (grib2.product_type) {
	case 0:
	case 1:
	case 8:
	{
	  outs << "  Generating Process: " << grib.process << "/" << grib2.backgen_process << "/" << grib2.fcstgen_process;
//	  if (!floatutils::myequalf(grid.level2,Grid::missing_value))
	  if (grid.level2_type != 255) {
	    outs << "  Levels: " << grid.level1_type;
	    if (!floatutils::myequalf(grid.level1,Grid::missing_value)) {
		outs << "/" << grid.level1;
	    }
	    outs << "," << grid.level2_type;
	    if (!floatutils::myequalf(grid.level2,Grid::missing_value)) {
		outs << "/" << grid.level2;
	    }
	  }
	  else {
	    outs << "  Level: " << grid.level1_type;
	    if (!floatutils::myequalf(grid.level1,Grid::missing_value))
		outs << "/" << grid.level1;
	  }
	  switch (grib2.product_type) {
	    case 1:
	    {
		outs << "  Ensemble Type/ID/Size: " << ensdata.fcst_type << "/" << ensdata.id << "/" << ensdata.total_size;
		break;
	    }
	    case 8:
	    {
		outs << "  Outermost Statistical Process: " << grib2.stat_process_ranges[0].type;
		break;
	    }
	  }
	  break;
	}
	default:
	{
	  outs << "  Product: " << grib2.product_type << "?";
	}
    }
    outs << "  Decimal Scale (D): " << std::setw(3) << grib.D << "  Binary Scale (E): " << std::setw(3) << grib.E << "  Pack Width: " << std::setw(2) << grib.pack_width;
    outs << std::endl;
  }
  else {
    outs << "|PDef=" << grib2.product_type << "|Ctr=" << grid.src << "-" << grib.sub_center << "|Tbls=" << grib.table << "." << grib2.local_table << "|RTime=" << grib2.time_sig << ":" << reference_date_time_.to_string("%Y%m%d%H%MM%SS") << "|Fcst=" << grid.fcst_time << "|GDef=" << grid.grid_type << "," << dim.x << "," << dim.y << ",";
    switch (grid.grid_type) {
	case 0:
	case 40:
	{
	  outs << def.slatitude << "," << def.slongitude << "," << def.elatitude << "," << def.elongitude << "," << def.loincrement << ",";
	  if (grid.grid_type == 0) {
	    outs << def.laincrement;
	  }
	  else {
	    outs << def.num_circles;
	  }
	  outs << "," << grib2.earth_shape;
	  break;
	}
	case 10:
	{
	  outs << def.slatitude << "," << def.slongitude << "," << def.llatitude << "," << def.olongitude << "," << def.stdparallel1 << "," << def.dx << "," << def.dy << "," << grib2.earth_shape;
	  break;
	}
	case 30:
	{
	  outs << def.slatitude << "," << def.slongitude << "," << def.llatitude << "," << def.olongitude << ",";
	  if ( (def.projection_flag&0x40) == 0) {
	    if ( (def.projection_flag&0x80) == 0x80) {
		outs << "S,";
	    }
	    else {
		outs << "N,";
	    }
	  }
	  outs << def.stdparallel1 << "," << def.stdparallel2 << "," << grib2.earth_shape;
	  break;
	}
    }
    outs << "|VTime=" << forecast_date_time_.to_string("%Y%m%d%H%MM%SS");
    if (valid_date_time_ != forecast_date_time_) {
	outs << ":" << valid_date_time_.to_string("%Y%m%d%H%MM%SS");
    }
    for (m=0; m < grib2.stat_process_ranges.size(); ++m) {
	if (grib2.stat_process_ranges[m].type >= 0) {
	  outs << "|SP" << m << "=" << grib2.stat_process_ranges[m].type << "." << grib2.stat_process_ranges[m].time_increment_type << "." << grib2.stat_process_ranges[m].period_length.unit << "." << grib2.stat_process_ranges[m].period_length.value << "." << grib2.stat_process_ranges[m].period_time_increment.unit << "." << grib2.stat_process_ranges[m].period_time_increment.value;
	}
    }
    if (!ensdata.fcst_type.empty()) {
	outs << "|E=" << ensdata.fcst_type << "." << ensdata.id << "." << ensdata.total_size;
	if (grib2.modelv_date.year() != 0) {
	  outs << "|Mver=" << grib2.modelv_date.to_string("%Y%m%d%H%MM%SS");
	}
    }
    if (grib2.spatial_process.type >= 0) {
	outs << "|Spatial=" << grib2.spatial_process.stat_process << "," << grib2.spatial_process.type << "," << grib2.spatial_process.num_points;
    }
    outs << "|P=" << grib2.discipline << "." << grib2.param_cat << "." << grid.param << "|L=" << grid.level1_type;
    if (!floatutils::myequalf(grid.level1,Grid::missing_value)) {
	outs << ":" << grid.level1;
    }
    if (grid.level2_type < 255) {
	outs << "," << grid.level2_type;
	if (!floatutils::myequalf(grid.level2,Grid::missing_value)) {
	  outs << ":" << grid.level2;
	}
    }
    outs << "|DDef=" << grib2.data_rep;
    if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
	outs.unsetf(std::ios::fixed);
	outs.setf(std::ios::scientific);
	outs.precision(3);
	outs << "|R=" << stats.min_val;
	if (grid.filled) {
	  outs << "|Max=" << stats.max_val << "|Avg=" << stats.avg_val;
	}
	outs.unsetf(std::ios::scientific);
	outs.setf(std::ios::fixed);
	outs.precision(2);
    }
    else {
	outs << "|R=" << stats.min_val;
	if (grid.filled) {
	  outs << "|Max=" << stats.max_val << "|Avg=" << stats.avg_val;
	}
    }
    outs << std::endl;
  }
}

void GRIB2Grid::set_statistical_process_ranges(const std::vector<StatisticalProcessRange>& source)
{
  grib2.stat_process_ranges.clear();
  grib2.stat_process_ranges=source;
}

void GRIB2Grid::operator+=(const GRIB2Grid& source)
{
  int n,m;
  size_t l,avg_cnt=0,cnt=0;
  StatisticalProcessRange srange;

  grid.src=60;
  grib.sub_center=1;
  if (grib.capacity.points == 0) {
// using a new grid
    *this=source;
    grid.nmean=1;
  }
  else {
    if (dim.size != source.dim.size || dim.x != source.dim.x || dim.y != source.dim.y) {
	std::cerr << "Warning: unable to perform grid addition" << std::endl;
	return;
    }
    else {
// using an existing grid
	if (bitmap.capacity == 0) {
	  bitmap.capacity=dim.size;
	  bitmap.map=new unsigned char[bitmap.capacity];
	  grid.num_missing=0;
	}
	stats.max_val=-Grid::missing_value;
	stats.min_val=Grid::missing_value;
	stats.avg_val=0.;
	for (n=0; n < dim.y; ++n) {
	  for (m=0; m < dim.x; ++m) {
	    l= (source.grib.scan_mode == grib.scan_mode) ? n : dim.y-n-1;
	    if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value) && !floatutils::myequalf(source.gridpoints_[l][m],Grid::missing_value)) {
		gridpoints_[n][m]+=(source.gridpoints_[l][m]);
		bitmap.map[cnt++]=1;
		if (gridpoints_[n][m] > stats.max_val) {
		  stats.max_val=gridpoints_[n][m];
		}
		if (gridpoints_[n][m] < stats.min_val) {
		  stats.min_val=gridpoints_[n][m];
		}
		stats.avg_val+=gridpoints_[n][m];
		++avg_cnt;
	    }
	    else {
		gridpoints_[n][m]=(Grid::missing_value);
		bitmap.map[cnt++]=0;
		++grid.num_missing;
	    }
	  }
	}
	if (grid.num_missing == 0) {
	  bitmap.applies=false;
	}
	else {
	  bitmap.applies=true;
	}
	if (avg_cnt > 0) {
	  stats.avg_val/=static_cast<double>(avg_cnt);
	}
	++grid.nmean;
	if (grid.nmean == 2) {
	  bool can_add=false;
	  if (grib2.stat_process_ranges.size() > 0) {
	    if (grib2.stat_process_ranges.size() == 1) {
		srange=grib2.stat_process_ranges[0];
		grib2.stat_process_ranges.emplace_back(srange);
		can_add=true;
	    }
	    else {
std::cerr << "A " << source.grid.src << " " << grib2.stat_process_ranges.size() << " " << grib2.stat_process_ranges[0].type << std::endl;
		if (source.grid.src == 7) {
		  switch (source.grib2.stat_process_ranges[0].type) {
		    case 193:
		    case 194:
		    {
			if (source.grib2.stat_process_ranges.size() == 2) {
			  float x=source.grib2.stat_process_ranges[0].period_length.value/static_cast<float>(dateutils::days_in_month(source.reference_date_time_.year(),source.reference_date_time_.month()));
std::cerr << "B194 " << x << " " << source.reference_date_time_.year() << " " << source.reference_date_time_.month() << std::endl;
			  if (floatutils::myequalf(x,static_cast<int>(x),0.001)) {
			    srange.type=0;
			    srange.time_increment_type=1;
			    srange.period_length.unit=2;
			    srange.period_length.value=dateutils::days_in_month(source.reference_date_time_.year(),source.reference_date_time_.month());
			    srange.period_time_increment.unit=1;
			    srange.period_time_increment.value=source.grib2.stat_process_ranges[0].period_time_increment.value;
			    grib2.stat_process_ranges[1]=srange;
			    can_add=true;
			  }
			}
			break;
		    }
		    case 204:
		    {
			if (source.grib2.stat_process_ranges.size() == 2) {
			  float x=source.grib2.stat_process_ranges[0].period_length.value/static_cast<float>(dateutils::days_in_month(source.reference_date_time_.year(),source.reference_date_time_.month()));
std::cerr << "B204 " << x << " " << source.reference_date_time_.year() << " " << source.reference_date_time_.month() << std::endl;
			  if (floatutils::myequalf(x,static_cast<int>(x),0.001)) {
			    srange.type=0;
			    srange.time_increment_type=1;
			    srange.period_length.unit=2;
			    srange.period_length.value=dateutils::days_in_month(source.reference_date_time_.year(),source.reference_date_time_.month());
			    srange.period_time_increment.unit=1;
			    srange.period_time_increment.value=6;
			    grib2.stat_process_ranges[1]=srange;
			    srange.type=1;
			    srange.time_increment_type=2;
			    srange.period_length.unit=1;
			    srange.period_length.value=source.grib2.stat_process_ranges[0].period_time_increment.value;
			    srange.period_time_increment.unit=255;
			    srange.period_time_increment.value=0;
			    grib2.stat_process_ranges.emplace_back(srange);
			    can_add=true;
			  }
			}
			break;
		    }
		  }
		}
	    }
	  }
	  else {
	    grib2.stat_process_ranges.resize(1);
	    can_add=true;
std::cerr << can_add << std::endl;
	  }
	  if (!can_add) {
	    myerror="unable to combine grids with more than one statistical process";
	    exit(1);
	  }
	  srange.type=1;
	  srange.time_increment_type=1;
	  srange.period_length.value=0;
	  if (reference_date_time_.year() != source.reference_date_time_.year() && reference_date_time_.month() == source.reference_date_time_.month() && reference_date_time_.day() == source.reference_date_time_.day() && reference_date_time_.time() == source.reference_date_time_.time()) {
	    srange.period_length.unit=4;
	    srange.period_time_increment.unit=4;
	    srange.period_time_increment.value=source.reference_date_time_.years_since(reference_date_time_);
	  }
	  else {
	    srange.period_length.unit=1;
	    srange.period_time_increment.unit=1;
	    srange.period_time_increment.value=source.reference_date_time_.hours_since(reference_date_time_);
	  }
	  grib2.stat_process_ranges[0]=srange;
	}
	if (reference_date_time_.year() != source.reference_date_time_.year() && reference_date_time_.month() == source.reference_date_time_.month() && reference_date_time_.day() == source.reference_date_time_.day() && reference_date_time_.time() == source.reference_date_time_.time()) {
	  grib2.stat_process_ranges[0].period_length.value=source.reference_date_time_.years_since(reference_date_time_)+1;
	  grib2.stat_period_end=source.grib2.stat_period_end;
	}
	else {
	  grib2.stat_process_ranges[0].period_length.value=source.reference_date_time_.hours_since(reference_date_time_);
	  grib2.stat_period_end=source.valid_date_time_;
	}
	grib2.product_type=8;
    }
  }
}

namespace gributils {

const char *grib1_time_unit[]={"minute","hour","day","month","year","year","year","year","","","hour","hour","hour"};
const int grib1_per_day[]={1440,24,1,0,0,0,0,0,0,0,24,24,24};
const int grib1_unit_mult[]={1,1,1,1,1,10,30,100,1,1,3,6,12};
const char *grib2_time_unit[]={"minute","hour","day","month","year","year","year","year","","","hour","hour","hour","second"};
const int grib2_per_day[]={1440,24,1,0,0,0,0,0,0,0,24,24,24,86400};
const int grib2_unit_mult[]={1,1,1,1,1,10,30,100,1,1,3,6,12,1};

std::string interval_24_hour_forecast_averages_to_string(std::string format,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=nullptr;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  if (tunit == 11) {
    p1*=6;
    p2*=6;
    tunit=1;
  }
  if (tunit >= 1 && tunit <= 2) {
    if (navg == static_cast<int>(dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month()))) {
	product="Monthly Mean (1 per day) of ";
    }
    else {
	product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
    }
  }
  else {
    product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
  }
  product+=strutils::itos(p2-p1)+"-"+tu[tunit]+" Average (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
  switch (tunit) {
    case 1:
    {
	last_valid_date_time=first_valid_date_time.hours_added((navg-1)*24+(p2-p1));
	break;
    }
    case 2:
    {
	last_valid_date_time=first_valid_date_time.days_added((navg-1)+(p2-p1));
	break;
    }
    default:
    {
	myerror="unable to compute last_valid_date_time from time units "+strutils::itos(tunit);
	exit(1);
    }
  }
  return product;
}

std::string successive_forecast_averages_to_string(std::string format,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=nullptr;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  if (tunit >= 1 && tunit <= 2) {
    int n= (tunit == 2) ? 1 : 24/p2;
    if ( (dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month())-navg/n) < 3) {
	product="Monthly Mean ("+strutils::itos(n)+" per day) of Forecasts of ";
    }
    else {
	product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
    }
  }
  else {
    product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
  }
  product+=strutils::itos(p2-p1)+"-"+tu[tunit]+" Average (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
  switch (tunit) {
    case 1:
    {
	last_valid_date_time=first_valid_date_time.hours_added(navg*(p2-p1));
	break;
    }
    case 2:
    {
	last_valid_date_time=first_valid_date_time.days_added(navg*(p2-p1));
	break;
    }
    case 11:
    {
	last_valid_date_time=first_valid_date_time.hours_added(navg*(p2-p1)*6);
	break;
    }
    default:
    {
	myerror="unable to compute last_valid_date_time from time units "+strutils::itos(tunit)+" (successive_forecast_averages_to_string)";
	exit(1);
    }
  }
  return product;
}

std::string time_range_2_to_string(std::string format,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=nullptr;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  last_valid_date_time=first_valid_date_time;
  if (navg > 0) {
    int n=dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month());
    if ( (navg % n) == 0) {
	product="Monthly Mean ("+strutils::itos(navg/n)+" per day) of ";
	last_valid_date_time.add_hours((navg-1)*24*n/navg);
    }
    else {
	myerror="mean with time range indicator 2 does not make sense";
	exit(1);
    }
  }
  if (p1 == 0) {
    if (p2 == 0) {
	product+="Analyses";
    }
    else {
	product+=strutils::itos(p2)+"-"+tu[tunit]+" Product";
	if (p2 > 0) {
	  product+=" (initial+0 to initial+"+strutils::itos(p2)+")";
	}
    }
  }
  else {
    product+=strutils::itos(p2-p1)+"-"+tu[tunit]+" Product";
    if (p1 > 0 || p2 > 0) {
	product+=" (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
    }
  }
  switch (tunit) {
    case 1:
    {
	last_valid_date_time.add_hours(p2-p1);
	break;
    }
    case 2:
    {
	last_valid_date_time.add_hours((p2-p1)*24);
	break;
    }
    default:
    {
	myerror="unable to compute last_valid_date_time from time units "+strutils::itos(tunit);
	exit(1);
    }
  }
  return product;
}

std::string time_range_113_to_string(std::string format,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=nullptr;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  else {
    myerror="time_range_113_to_string(): no time units defined for GRIB format "+format;
    exit(1);
  }
  if (tunit >= 1 && tunit <= 2) {
    int n= (tunit == 2) ? 1 : 24/p2;
    if ( (dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month())-navg/n) < 3) {
	if (p1 == 0) {
	  product="Monthly Mean ("+strutils::itos(n)+" per day) of Analyses";
	}
	else {
	  product="Monthly Mean ("+strutils::itos(n)+" per day) of "+strutils::itos(p1)+"-"+tu[tunit]+" Forecasts";
	}
    }
    else {
	product="Mean of "+strutils::itos(navg)+" "+strutils::itos(p1)+"-"+tu[tunit]+" Forecasts at "+strutils::itos(p2)+"-"+tu[tunit]+" intervals";
    }
  }
  else {
    product="Mean of "+strutils::itos(navg)+" "+strutils::itos(p1)+"-"+tu[tunit]+" Forecasts at "+strutils::itos(p2)+"-"+tu[tunit]+" intervals";
  }
  switch (tunit) {
    case 1:
    {
	last_valid_date_time=first_valid_date_time.hours_added((navg-1)*p2);
	break;
    }
    case 2:
    {
	last_valid_date_time=first_valid_date_time.hours_added((navg-1)*p2*24);
	break;
    }
    default:
    {
	myerror="unable to compute last_valid_date_time from time units "+strutils::itos(tunit);
	exit(1);
    }
  }
  return product;
}

std::string time_range_118_to_string(std::string format,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;

  if (tunit >= 1 && tunit <= 2) {
    int n= (tunit == 2) ? 1 : 24/p2;
    if ( (dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month())-navg/n) < 3) {
	if (p1 == 0) {
	  product="Monthly Variance/Covariance ("+strutils::itos(n)+" per day) of Analyses";
	}
	else {
	  product="Monthly Variance/Covariance ("+strutils::itos(n)+" per day) of "+strutils::itos(p1)+"-"+grib1_time_unit[tunit]+" Forecasts";
	}
    }
    else {
	product="Variance/Covariance of "+strutils::itos(navg)+" "+strutils::itos(p1)+"-"+grib1_time_unit[tunit]+" Forecasts at "+strutils::itos(p2)+"-"+grib1_time_unit[tunit]+" intervals";
    }
  }
  else {
    product="Variance/Covariance of "+strutils::itos(navg)+" "+strutils::itos(p1)+"-"+grib1_time_unit[tunit]+" Forecasts at "+strutils::itos(p2)+"-"+grib1_time_unit[tunit]+" intervals";
  }
  switch (tunit) {
    case 1:
    {
	last_valid_date_time=first_valid_date_time.hours_added((navg-1)*p2);
	break;
    }
    case 2:
    {
	last_valid_date_time=first_valid_date_time.hours_added((navg-1)*p2*24);
	break;
    }
    default:
    {
	myerror="unable to compute last_valid_date_time from time units "+strutils::itos(tunit)+" (time_range_118_to_string)";
	exit(1);
    }
  }
  return product;
}

std::string time_range_120_to_string(std::string format,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=nullptr;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  if (tunit == 1) {
    int n=24/(p2-p1);
    if ( (dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month())-navg/n) < 3) {
        product="Monthly Mean ("+strutils::itos(n)+" per day) of Forecasts of ";
    }
    else {
        product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
    }
  }
  else {
    product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
  }
  product+=strutils::itos(p2-p1)+"-"+tu[tunit]+" Accumulation (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
  switch (tunit) {
    case 1:
    {
	last_valid_date_time=first_valid_date_time.hours_added(navg*(p2-p1));
	break;
    }
    default:
    {
	myerror="unable to compute last_valid_date_time from time units "+strutils::itos(tunit)+" (time_range_120_to_string)";
	exit(1);
    }
  }
  return product;
}

std::string time_range_123_to_string(std::string format,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p2,int navg)
{
  std::string product;
  char **tu=nullptr;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  if (tunit >= 1 && tunit <= 2) {
    int n= (tunit == 2) ? 1 : 24/p2;
    if ( (dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month())-navg/n) < 3) {
	product="Monthly Mean ("+strutils::itos(n)+" per day) of Analyses";
    }
    else {
	product="Mean of "+strutils::itos(navg)+" Analyses at "+strutils::itos(p2)+"-"+tu[tunit]+" intervals";
    }
  }
  else {
    product="Mean of "+strutils::itos(navg)+" Analyses at "+strutils::itos(p2)+"-"+tu[tunit]+" intervals";
  }
  switch (tunit) {
    case 1:
    {
	last_valid_date_time=first_valid_date_time.hours_added((navg-1)*p2);
	break;
    }
    case 2:
    {
	last_valid_date_time=first_valid_date_time.hours_added((navg-1)*p2*24);
	break;
    }
    default:
    {
	myerror="unable to compute last_valid_date_time from time units "+strutils::itos(tunit);
	exit(1);
    }
  }
  return product;
}

std::string time_range_128_to_string(std::string format,short center,short subcenter,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=nullptr;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  switch (center) {
    case 7:
    case 60:
    {
	if (tunit >= 1 && tunit <= 2) {
	  if (navg == static_cast<int>(dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month()))) {
	    product="Monthly Mean (1 per day) of ";
	  }
	  else {
	    product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2)+"-"+tu[tunit]+" intervals of ";
	  }
	}
	else {
	  product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2)+"-"+tu[tunit]+" intervals of ";
	}
	product+=strutils::itos(p2-p1)+"-"+tu[tunit]+" Accumulation (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
	switch (tunit) {
	  case 1:
	  {
	    last_valid_date_time=first_valid_date_time.hours_added((navg-1)*24+(p2-p1));
	    break;
	  }
	  case 2:
	  {
	    last_valid_date_time=first_valid_date_time.days_added((navg-1)+(p2-p1));
	    break;
	  }
	  default:
	  {
	    myerror="unable to compute last_valid_date_time from time units "+strutils::itos(tunit);
	    exit(1);
	  }
	}
	break;
    }
    case 34:
    {
	switch (subcenter) {
	  case 241:
	  {
	    product=interval_24_hour_forecast_averages_to_string(format,first_valid_date_time,last_valid_date_time,tunit,p1,p2,navg);
	    break;
	  }
	  default:
	  {
	    myerror="time range code 128 is not defined for center 34, subcenter "+strutils::itos(subcenter);
	    exit(1);
	  }
	}
	break;
    }
    default:
    {
	myerror="time range code 128 is not defined for center "+strutils::itos(center);
	exit(1);
    }
  }
  return product;
}

std::string time_range_129_to_string(std::string format,short center,short subcenter,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=nullptr;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  switch (center) {
    case 7:
    {
	if (tunit >= 1 && tunit <= 2) {
	  int n= (tunit == 2) ? 1 : 24/p2;
	  if ( (dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month())-navg/n) < 3) {
	    product="Monthly Mean ("+strutils::itos(n)+" per day) of Forecasts of ";
	  }
	  else {
	    product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
	  }
	}
	else {
	  product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
	}
	product+=strutils::itos(p2-p1)+"-"+tu[tunit]+" Accumulation (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
	switch (tunit) {
	  case 1:
	    last_valid_date_time=first_valid_date_time.hours_added(navg*(p2-p1));
	    break;
	  case 2:
	    last_valid_date_time=first_valid_date_time.days_added(navg*(p2-p1));
	    break;
	  default:
	    myerror="unable to compute last_valid_date_time for center 7 from time units "+strutils::itos(tunit)+" (time_range_129_to_string)";
	    exit(1);
	}
	break;
    }
    case 34:
    {
	switch (subcenter) {
	  case 241:
	  {
	    if (tunit == 1) {
		if ( (dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month())-navg) < 3) {
		  product="Monthly Variance/Covariance (1 per day) of Forecasts of ";
		}
		else {
		  product="Variance/Covariance of "+strutils::itos(navg)+" Forecasts at 24-hour intervals of ";
		}
	    }
	    else {
		product="Variance/Covariance of "+strutils::itos(navg)+" Forecasts at 24-hour intervals of ";
	    }
	    product+=strutils::itos(p2-p1)+"-"+tu[tunit]+" Average (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
	    switch (tunit) {
		case 1:
		{
		  last_valid_date_time=first_valid_date_time.hours_added((navg-1)*24+p2);
		  break;
		}
		default:
		{
		  myerror="unable to compute last_valid_date_time for center 34, subcenter 241 from time units "+strutils::itos(tunit)+" (time_range_129_to_string)";
		  exit(1);
		}
            }
	    break;
	  }
	  default:
	  {
	    myerror="time range code 129 is not defined for center  34, subcenter "+strutils::itos(center);
	    exit(1);
	  }
	}
	break;
    }
    default:
    {
	myerror="time range code 129 is not defined for center "+strutils::itos(center);
	exit(1);
    }
  }
  return product;
}

std::string time_range_130_to_string(std::string format,short center,short subcenter,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;

  switch (center) {
    case 7:
    {
	product=interval_24_hour_forecast_averages_to_string(format,first_valid_date_time,last_valid_date_time,tunit,p1,p2,navg);
	break;
    }
    case 34:
    {
	switch (subcenter) {
	  case 241:
	  {
	    product=successive_forecast_averages_to_string(format,first_valid_date_time,last_valid_date_time,tunit,p1,p2,navg);
	    break;
	  }
	  default:
	  {
	    myerror="time range code 130 is not defined for center 34, subcenter "+strutils::itos(subcenter);
	    exit(1);
	  }
	}
	break;
    }
    default:
    {
	myerror="time range code 130 is not defined for center "+strutils::itos(center);
	exit(1);
    }
  }
  return product;
}

std::string time_range_131_to_string(std::string format,short center,short subcenter,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=nullptr;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  switch (center) {
    case 7:
    {
	product=successive_forecast_averages_to_string(format,first_valid_date_time,last_valid_date_time,tunit,p1,p2,navg);
	break;
    }
    case 34:
    {
	switch (subcenter) {
	  case 241:
	  {
	    if (tunit == 1) {
		int n=24/p2;
		if ( (dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month())-navg/n) < 3) {
		  product="Monthly Variance/Covariance ("+strutils::itos(n)+" per day) of Forecasts of ";
		}
		else {
		  product="Variance/Covariance of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
		}
	    }
	    else {
		product="Variance/Covariance of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
	    }
	    product+=strutils::itos(p2-p1)+"-"+tu[tunit]+" Average (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
	    switch (tunit) {
		case 1:
		{
		  last_valid_date_time=first_valid_date_time.hours_added(navg*(p2-p1));
		  break;
		}
		default:
		{
		  myerror="unable to compute last_valid_date_time for center 34, subcenter 241 from time units "+strutils::itos(tunit)+" (time_range_131_to_string)";
		  exit(1);
		}
	    }
	    break;
	  }
	  default:
	  {
	    myerror="time range code 131 is not defined for center 34, subcenter "+strutils::itos(subcenter);
	    exit(1);
	  }
	}
	break;
    }
    default:
    {
	myerror="time range code 131 is not defined for center "+strutils::itos(center);
	exit(1);
    }
  }
  return product;
}

std::string time_range_132_to_string(std::string format,short center,short subcenter,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;

  switch (center) {
    case 34:
    {
	switch (subcenter) {
	  case 241:
	  {
	    product=time_range_118_to_string(format,first_valid_date_time,last_valid_date_time,tunit,p1,p2,navg);
	    break;
	  }
	  default:
	  {
	    myerror="time range code 132 is not defined for center 34, subcenter "+strutils::itos(subcenter);
	    exit(1);
	  }
	}
	break;
    }
    default:
    {
        myerror="time range code 132 is not defined for center "+strutils::itos(center);
	exit(1);
    }
  }
  return product;
}

std::string time_range_137_to_string(std::string format,short center,short subcenter,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=nullptr;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  switch (center) {
    case 7:
    case 60:
    {
	if (tunit >= 1 && tunit <= 2) {
	  if (navg/4 == static_cast<int>(dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month()))) {
	    product="Monthly Mean (4 per day) of ";
	  }
	  else if (navg == static_cast<int>(dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month()))) {
	    product="Monthly Mean (1 per day) of ";
	  }
	  else {
	    product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2)+"-"+tu[tunit]+" intervals of ";
	  }
	}
	else {
	  product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2)+"-"+tu[tunit]+" intervals of ";
	}
	product+=strutils::itos(p2-p1)+"-"+tu[tunit]+" Accumulation (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
	switch (tunit) {
	  case 1:
	  {
	    last_valid_date_time=first_valid_date_time.hours_added((navg-1)*6+(p2-p1));
	    break;
	  }
	  default:
	  {
	    myerror="unable to compute last_valid_date_time from time units "+strutils::itos(tunit);
	    exit(1);
	  }
	}
	break;
    }
    default:
    {
	myerror="time range code 137 is not defined for center "+strutils::itos(center);
	exit(1);
    }
  }
  return product;
}

std::string time_range_138_to_string(std::string format,short center,short subcenter,DateTime& first_valid_date_time,DateTime& last_valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=nullptr;
  int hours_to_last=0;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  switch (center) {
    case 7:
    case 60:
    {
	if (tunit == 11) {
	  p1*=6;
	  p2*=6;
	  tunit=1;
	}
	if (tunit >= 1 && tunit <= 2) {
	  if (navg/4 == static_cast<int>(dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month()))) {
	    product="Monthly Mean (4 per day) of ";
	    hours_to_last=(navg-1)*6;
	  }
	  else if (navg == static_cast<int>(dateutils::days_in_month(first_valid_date_time.year(),first_valid_date_time.month()))) {
	    product="Monthly Mean (1 per day) of ";
	    hours_to_last=(navg-1)*24;
	  }
	  else {
	    product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
	  }
	}
	else {
	  product="Mean of "+strutils::itos(navg)+" Forecasts at "+strutils::itos(p2-p1)+"-"+tu[tunit]+" intervals of ";
	}
	product+=strutils::itos(p2-p1)+"-"+tu[tunit]+" Average (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
	last_valid_date_time=first_valid_date_time;
	switch (tunit) {
	  case 1:
	  {
	    last_valid_date_time.add_hours(hours_to_last+(p2-p1));
	    break;
	  }
	  default:
	  {
	    myerror="unable to compute last_valid_date_time from time units "+strutils::itos(tunit);
	    exit(1);
	  }
	}
	break;
    }
    default:
    {
        myerror="time range code 138 is not defined for center "+strutils::itos(center);
	exit(1);
    }
  }
  return product;
}

void set_forecast_date_time(size_t p1,short tunits,const DateTime& reference_date_time,DateTime& forecast_date_time,size_t& fcst_time)
{
  switch (tunits) {
    case 0:
    {
	forecast_date_time=reference_date_time.minutes_added(p1);
	fcst_time=p1*100;
	break;
    }
    case 1:
    {
	forecast_date_time=reference_date_time.hours_added(p1);
	fcst_time=p1*10000;
	break;
    }
    case 2:
    {
	forecast_date_time=reference_date_time.days_added(p1);
	fcst_time=p1*240000;
	break;
    }
    case 3:
    {
	forecast_date_time=reference_date_time.months_added(p1);
	fcst_time=forecast_date_time.time_since(reference_date_time);
	break;
    }
    case 4:
    {
	forecast_date_time=reference_date_time.years_added(p1);
	fcst_time=forecast_date_time.time_since(reference_date_time);
	break;
    }
    case 12:
    {
	forecast_date_time=reference_date_time.hours_added(p1*12);
	fcst_time=p1*12*10000;
	break;
    }
    default:
    {
	myerror="unable to convert forecast hour for time units "+strutils::itos(tunits);
	exit(1);
    }
  }
}

std::string grib_product_description(GRIBGrid *grid,DateTime& forecast_date_time,DateTime& valid_date_time,size_t& fcst_time)
{
  std::string product_description;

  short tunits=grid->time_unit();
  short p1=grid->P1()*grib1_unit_mult[tunits];
  short p2=grid->P2()*grib1_unit_mult[tunits];
  switch (grid->time_range()) {
    case 0:
    case 10:
    {
	if (p1 == 0) {
	  product_description="Analysis";
	  fcst_time=0;
	  forecast_date_time=grid->reference_date_time();
	}
	else {
	  product_description=strutils::itos(p1)+"-"+grib1_time_unit[tunits]+" Forecast";
	  set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	}
	valid_date_time=forecast_date_time;
	break;
    }
    case 1:
    {
	product_description=strutils::itos(p1)+"-"+grib1_time_unit[tunits]+" Forecast";
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	valid_date_time=forecast_date_time;
	break;
    }
    case 2:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_2_to_string("grib",forecast_date_time,valid_date_time,tunits,p1,p2,grid->number_averaged());
	break;
    }
    case 3:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	if (p1 == 0 && tunits == 2 && p2 == static_cast<int>(dateutils::days_in_month(forecast_date_time.year(),forecast_date_time.month()))) {
	  product_description="Monthly Average";
	}
	else {
	  product_description=strutils::itos(p2-p1)+"-"+grib1_time_unit[tunits]+" Average (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
	}
	switch (tunits) {
	  case 1:
	  {
	    valid_date_time=forecast_date_time.hours_added(p2-p1);
	    break;
	  }
	  case 2:
	  {
	    valid_date_time=forecast_date_time.days_added(p2-p1);
	    break;
	  }
	  case 3:
	  {
	    valid_date_time=forecast_date_time.months_added(p2-p1);
	    break;
	  }
	  default:
	  {
	    myerror="unable to compute valid_date_time from time units "+strutils::itos(tunits);
	    exit(1);
	  }
	}
	break;
    }
    case 4:
    case 5:
    {
	fcst_time=0;
	product_description=strutils::itos(std::abs(p2-p1))+"-"+grib1_time_unit[tunits]+" ";
	if (grid->time_range() == 4) {
	  product_description+="Accumulation";
	}
	else {
	  product_description+="Difference";
	}
	product_description+=" (initial";
	if (p1 < 0) {
	  product_description+="-";
	}
	else {
	  product_description+="+";
	}
	product_description+=strutils::itos(p1)+" to initial";
	if (p2 < 0) {
	  product_description+="-";
	}
	else {
	  product_description+="+";
	}
	product_description+=strutils::itos(p2)+")";
	forecast_date_time=valid_date_time=grid->reference_date_time();
	switch (tunits) {
	  case 1:
	    if (p2 < 0) {
		valid_date_time.subtract_hours(-p2);
	    }
	    else {
		valid_date_time.add_hours(p2);
	    }
	    if (p1 < 0) {
		forecast_date_time.subtract_hours(-p1);
	    }
	    break;
	  case 2:
	    if (p2 < 0) {
		valid_date_time.subtract_days(-p2);
	    }
	    else {
		valid_date_time.add_days(p2);
	    }
	    if (p1 < 0) {
		forecast_date_time.subtract_days(-p1);
	    }
	    break;
	  case 3:
	    if (p2 < 0) {
		valid_date_time.subtract_months(-p2);
	    }
	    else {
		valid_date_time.add_months(p2);
	    }
	    if (p1 < 0) {
		forecast_date_time.subtract_months(-p1);
	    }
	    break;
	  default:
	    myerror="unable to compute valid_date_time from time units "+strutils::itos(tunits);
	    exit(1);
	}
	break;
    }
    case 7:
    {
	if (grid->source() == 7) {
	  if (p2 == 0) {
	    forecast_date_time=valid_date_time=grid->reference_date_time();
	    switch (tunits) {
		case 2:
		{
		  forecast_date_time.subtract_days(p1);
		  size_t n=p1+1;
		  if (dateutils::days_in_month(forecast_date_time.year(),forecast_date_time.month()) == n) {
		    product_description="Monthly Mean";
		  }
		  else if (n == 5) {
		    product_description="Pentad Mean";
		  }
		  else if (n == 7) {
		    product_description="Weekly Mean";
		  }
		  else {
		    product_description=strutils::itos(n)+"-"+grib1_time_unit[tunits]+" Average";
		  }
		  break;
		}
		default:
		  myerror="time unit "+strutils::itos(tunits)+" not recognized for time range indicator 7 and P2 == 0";
		  exit(1);
	    }
	  }
	  else {
	    myerror="time unit "+strutils::itos(tunits)+" not recognized for time range indicator 7 and P2 != 0";
	    exit(1);
	  }
	}
	else {
	  myerror="time range 7 not defined for center "+strutils::itos(grid->source());
	  exit(1);
	}
	break;
    }
    case 51:
    {
	product_description=strutils::itos(grid->number_averaged())+"-year Climatology of ";
	switch (tunits) {
	  case 3:
	  {
	    product_description+="Monthly Means";
	    forecast_date_time=valid_date_time=grid->reference_date_time().years_added(grid->number_averaged()-1).months_added(1);
	    break;
	  }
	  default:
	  {
	    myerror="bad P2 "+strutils::itos(p2)+" for climatological mean";
	    exit(1);
	  }
	}
	break;
    }
    case 113:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_113_to_string("grib",forecast_date_time,valid_date_time,tunits,p1,p2,grid->number_averaged());
	break;
    }
    case 118:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_118_to_string("grib",forecast_date_time,valid_date_time,tunits,p1,p2,grid->number_averaged());
	break;
    }
    case 120:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_120_to_string("grib",forecast_date_time,valid_date_time,tunits,p1,p2,grid->number_averaged());
	break;
    }
    case 123:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_123_to_string("grib",forecast_date_time,valid_date_time,tunits,p2,grid->number_averaged());
	break;
    }
    case 128:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_128_to_string("grib",grid->source(),grid->sub_center_id(),forecast_date_time,valid_date_time,tunits,p1,p2,grid->number_averaged());
	break;
    }
    case 129:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_129_to_string("grib",grid->source(),grid->sub_center_id(),forecast_date_time,valid_date_time,tunits,p1,p2,grid->number_averaged());
	break;
    }
    case 130:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_130_to_string("grib",grid->source(),grid->sub_center_id(),forecast_date_time,valid_date_time,tunits,p1,p2,grid->number_averaged());
	break;
    }
    case 131:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_131_to_string("grib",grid->source(),grid->sub_center_id(),forecast_date_time,valid_date_time,tunits,p1,p2,grid->number_averaged());
	break;
    }
    case 132:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_132_to_string("grib",grid->source(),grid->sub_center_id(),forecast_date_time,valid_date_time,tunits,p1,p2,grid->number_averaged());
	break;
    }
    case 137:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_137_to_string("grib",grid->source(),grid->sub_center_id(),forecast_date_time,valid_date_time,tunits,p1,p2,grid->number_averaged());
	break;
    }
    case 138:
    {
	set_forecast_date_time(p1,tunits,grid->reference_date_time(),forecast_date_time,fcst_time);
	product_description=time_range_138_to_string("grib",grid->source(),grid->sub_center_id(),forecast_date_time,valid_date_time,tunits,p1,p2,grid->number_averaged());
	break;
    }
    default:
    {
	myerror="GRIB time range indicator "+strutils::itos((reinterpret_cast<GRIBGrid *>(grid))->time_range())+" not recognized";
	exit(1);
    }
  }
  return product_description;
}

std::string grib2_product_description(GRIB2Grid *grid,DateTime& forecast_date_time,DateTime& valid_date_time)
{
  std::string product_description;

  short ptype=grid->product_type();
  short tunits=grid->time_unit();
  int fcst_hr=grid->forecast_time()/10000;
  switch (ptype) {
    case 0:
    case 1:
    case 2:
    {
	if (fcst_hr == 0) {
	  product_description="Analysis";
	}
	else {
	  product_description=strutils::itos(fcst_hr)+"-"+grib2_time_unit[tunits]+" Forecast";
	}
	break;
    }
    case 8:
    case 11:
    case 12:
    case 61:
    {
	std::vector<GRIB2Grid::StatisticalProcessRange> spranges=grid->statistical_process_ranges();
	product_description="";
	for (size_t n=0; n < spranges.size(); n++) {
	  if (!product_description.empty()) {
	    product_description+=" of ";
	  }
	  if (spranges.size() == 2 && spranges[n].type >= 192) {
	    switch (grid->source()) {
		case 7:
		case 60:
		{
		  switch (spranges[0].type) {
		    case 193:
		    {
// average of analyses
			product_description+=time_range_113_to_string("grib2",forecast_date_time,valid_date_time,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    }
		    case 194:
		    {
			product_description+=time_range_123_to_string("grib2",forecast_date_time,valid_date_time,tunits,spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    }
		    case 195:
		    {
// average of foreast accumulations at 24-hour intervals
			product_description+=time_range_128_to_string("grib2",grid->source(),(reinterpret_cast<GRIB2Grid *>(grid))->sub_center_id(),forecast_date_time,valid_date_time,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    }
		    case 197:
		    {
// average of foreast averages at 24-hour intervals
			product_description+=time_range_130_to_string("grib2",grid->source(),(reinterpret_cast<GRIB2Grid *>(grid))->sub_center_id(),forecast_date_time,valid_date_time,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    }
		    case 204:
		    {
// average of foreast accumulations at 6-hour intervals
			product_description+=time_range_137_to_string("grib2",grid->source(),(reinterpret_cast<GRIB2Grid *>(grid))->sub_center_id(),forecast_date_time,valid_date_time,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    }
		    case 205:
		    {
// average of foreast averages at 6-hour intervals
			product_description+=time_range_138_to_string("grib2",grid->source(),(reinterpret_cast<GRIB2Grid *>(grid))->sub_center_id(),forecast_date_time,valid_date_time,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    }
		    case 255:
		    {
			product_description+=time_range_2_to_string("grib2",forecast_date_time,valid_date_time,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    }
		    default:
		    {
			myerror="unable to decode NCEP local-use statistical process type "+strutils::itos(spranges[0].type);
			exit(1);
		    }
		  }
		  n=spranges.size();
		  break;
		}
		default:
		{
		  myerror="unable to decode local-use statistical processing for center "+strutils::itos(grid->source());
		  exit(1);
		}
	    }
	  }
/*
	  else if (spranges[n].period_length.value == 0) {
	    if (strutils::has_ending(product_description,"Forecasts of")) {
		product_description=product_description.substr(0,product_description.length()-12);
	    }
	    if (fcst_hr == 0) {
		if (n == 0) {
		  product_description+="Analysis";
		}
		else {
		  product_description+="Analyses";
		}
	    }
	    else {
		product_description+=strutils::itos(fcst_hr)+"-"+grib2_time_unit[tunits]+" Forecast";
		if (n > 0) {
		  product_description+="s";
		}
	    }
	  }
*/
	  else if (spranges[n].period_time_increment.value == 0) {
// continuous processes
	    if (spranges[n].period_length.value < 0) {
		switch (spranges[n].type) {
		  case 1:
		  {
		    product_description+=strutils::itos(-spranges[n].period_length.value*grib2_unit_mult[spranges[n].period_length.unit])+"-"+grib2_time_unit[spranges[n].period_length.unit]+" Accumulation (initial";
//		    forecast_date_time.subtract(grib2_time_unit[spranges[n].period_length.unit]+std::string("s"),-spranges[n].period_length.value*grib2_unit_mult[spranges[n].period_length.unit]);
forecast_date_time=valid_date_time.subtracted(grib2_time_unit[spranges[n].period_length.unit]+std::string("s"),-spranges[n].period_length.value*grib2_unit_mult[spranges[n].period_length.unit]);
		    if (forecast_date_time < grid->reference_date_time()) {
			product_description+="-"+strutils::itos(grid->reference_date_time().hours_since(forecast_date_time));
		    }
		    else {
			product_description+="+"+strutils::itos(forecast_date_time.hours_since(grid->reference_date_time()));
		    }
		    product_description+=" to initial";
		    if (valid_date_time < grid->reference_date_time()) {
			product_description+="-"+strutils::itos(grid->reference_date_time().hours_since(valid_date_time));
		    }
		    else {
			product_description+="+"+strutils::itos(valid_date_time.hours_since(grid->reference_date_time()));
		    }
		    product_description+=")";
		    break;
		  }
		  default:
		  {
		    myerror="no decoding for negative period and statistical process type "+strutils::itos(spranges[n].type)+", length: "+strutils::itos(spranges[n].period_length.value)+"; date - "+forecast_date_time.to_string();
		    exit(1);
		  }
		}
	    }
	    else {
		switch (spranges[n].type) {
// average (0)
// accumulation (1)
// maximum (2)
// minimum (3)
// unspecified (255) - call it a period
		  case 0:
		  case 1:
		  case 2:
		  case 3:
		  case 255:
		  {
		    switch (spranges[n].type) {
			case 0:
			{
			  product_description="Average";
			  break;
			}
			case 1:
			{
			  product_description="Accumulation";
			  break;
			}
			case 2:
			{
			  product_description="Maximum";
			  break;
			}
			case 3:
			{
			  product_description="Minimum";
			  break;
			}
			case 255:
			{
			  product_description="Period";
			  break;
			}
		    }
/*
		    if (fcst_hr == 0)
			product_description+=" (reference date/time to valid date/time)";
		    else {
*/
			product_description=strutils::itos(spranges[n].period_length.value*grib2_unit_mult[spranges[n].period_length.unit])+"-"+grib2_time_unit[spranges[n].period_length.unit]+" "+product_description;
			if (spranges[n].period_length.value > 0) {
			  product_description+=" (initial+"+strutils::itos(fcst_hr)+" to initial+"+strutils::itos(fcst_hr+spranges[n].period_length.value*grib2_unit_mult[spranges[n].period_length.unit])+")";
			}
//		    }
		    valid_date_time=forecast_date_time.added(grib2_time_unit[spranges[n].period_length.unit]+std::string("s"),spranges[n].period_length.value*grib2_unit_mult[spranges[n].period_length.unit]);
		    break;
		  }
		  default:
		  {
		    myerror="no decoding for positive period and statistical process type "+strutils::itos(spranges[n].type);
		    exit(1);
		  }
		}
	    }
	  }
	  else {
	    myerror="unable to decode statistical process data for period length "+strutils::itos(spranges[0].period_length.value)+"/"+strutils::itos(spranges[n].period_length.unit);
	    exit(1);
	  }
	}
	break;
    }
    default:
	myerror="product type "+strutils::itos(ptype)+" not recognized";
	exit(1);
  }
  return product_description;
}

short p2_from_statistical_end_time(const GRIB2Grid& grid)
{
  DateTime dt1=grid.reference_date_time(),dt2=grid.statistical_process_end_date_time();

  switch (grid.time_unit()) {
    case 0:
    {
	return (dt2.time()/100 % 100)-(dt1.time()/100 % 100);
    }
    case 1:
    {
	 return (dt2.time()/10000-dt1.time()/10000);
    }
    case 2:
    {
	return (dt2.day()-dt1.day());
    }
    case 3:
    {
	return (dt2.month()-dt1.month());
    }
    case 4:
    {
	return (dt2.year()-dt1.year());
    }
    default:
    {
	myerror="unable to map end time with units "+strutils::itos(grid.time_unit())+" to GRIB1";
	exit(1);
    }
  }
}

#ifdef __JASPER
extern "C" int dec_jpeg2000(char *injpc,int bufsize,int *outfld)
/*$$$  SUBPROGRAM DOCUMENTATION BLOCK
*                .      .    .                                       .
* SUBPROGRAM:    dec_jpeg2000      Decodes JPEG2000 code stream
*   PRGMMR: Gilbert          ORG: W/NP11     DATE: 2002-12-02
*
* ABSTRACT: This Function decodes a JPEG2000 code stream specified in the
*   JPEG2000 Part-1 standard (i.e., ISO/IEC 15444-1) using JasPer 
*   Software version 1.500.4 (or 1.700.2) written by the University of British
*   Columbia and Image Power Inc, and others.
*   JasPer is available at http://www.ece.uvic.ca/~mdadams/jasper/.
*
* PROGRAM HISTORY LOG:
* 2002-12-02  Gilbert
*
* USAGE:     int dec_jpeg2000(char *injpc,int bufsize,int *outfld)
*
*   INPUT ARGUMENTS:
*      injpc - Input JPEG2000 code stream.
*    bufsize - Length (in bytes) of the input JPEG2000 code stream.
*
*   OUTPUT ARGUMENTS:
*     outfld - Output matrix of grayscale image values.
*
*   RETURN VALUES :
*          0 = Successful decode
*         -3 = Error decode jpeg2000 code stream.
*         -5 = decoded image had multiple color components.
*              Only grayscale is expected.
*
* REMARKS:
*
*      Requires JasPer Software version 1.500.4 or 1.700.2
*
* ATTRIBUTES:
*   LANGUAGE: C
*   MACHINE:  IBM SP
*
*$$$*/
{
    int ier;
    int i,j,k;
    jas_image_t *image=0;
    jas_stream_t *jpcstream;
    jas_image_cmpt_t *pcmpt;
    char *opts=0;
    jas_matrix_t *data;

//    jas_init();
    ier=0;
// Create jas_stream_t containing input JPEG200 codestream in memory.
    jpcstream=jas_stream_memopen(injpc,bufsize);
// Decode JPEG200 codestream into jas_image_t structure.
    image=jpc_decode(jpcstream,opts);
    if ( image == 0 ) {
       jas_eprintf(" jpc_decode return = %d \n",ier);
       return -3;
    }
    pcmpt=image->cmpts_[0];
// Expecting jpeg2000 image to be grayscale only.
// No color components.
    if (image->numcmpts_ != 1 ) {
       jas_eprintf("dec_jpeg2000: Found color image.  Grayscale expected.\n");
       return (-5);
    }
// Create a data matrix of grayscale image values decoded from the jpeg2000 code
// stream.
    data=jas_matrix_create(jas_image_height(image), jas_image_width(image));
    jas_image_readcmpt(image,0,0,0,jas_image_width(image),
                       jas_image_height(image),data);
// Copy data matrix to output integer array.
    k=0;
    for (i=0;i<pcmpt->height_;i++) 
      for (j=0;j<pcmpt->width_;j++) 
        outfld[k++]=data->rows_[i][j];
// Clean up JasPer work structures.
    jas_matrix_destroy(data);
    ier=jas_stream_close(jpcstream);
    jas_image_destroy(image);
    return 0;
}
#endif

} // end namespace gributils
#endif
