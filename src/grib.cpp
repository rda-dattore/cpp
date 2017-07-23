#include <iomanip>
#include <string>
#include <list>
#include <grid.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>
#include <xml.hpp>
#ifdef __JASPER
#include <jasper/jasper.h>
#endif
#include <myerror.hpp>
#include <tempfile.hpp>

const char *GRIBGrid::levelTypeShortName[]={
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
const char *GRIBGrid::levelTypeUnits[]={"","","","","","","","","","","","","",
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
const char *GRIBGrid::timeUnits[]={"Minutes","Hours","Days","Months","Years",
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
const short GRIBGrid::octagonalGridParameterMap[]={ 0, 7,35, 0, 7,39, 1, 0, 0,
 0,11,12, 0, 0, 0, 0, 0, 0, 0,18,15,16, 0, 0, 0, 0, 0, 0, 2, 0,33,34, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0,52, 0, 0,80, 0, 0, 0, 0, 0, 0, 0, 0, 0,80, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0,66};
const short GRIBGrid::navyGridParameterMap[]={ 0, 7, 0, 0, 0, 0, 2, 0, 0, 0,11,
  0,18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,33,34, 0,31,32, 0, 0,
  0, 0, 0,40, 0, 0, 0, 0,55, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11,11,11};
const short GRIBGrid::on84GridParameterMap[]={  0,  7,  0,  0,  0,  0,  0,  0,
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

int InputGRIBStream::findGRIB(unsigned char *buffer)
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

int InputGRIBStream::peek()
{
  unsigned char buffer[16];
  int len=-9;
  while (len == -9) {
    len=findGRIB(buffer);
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
	    getBits(buffer,llen,0,24);
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
	  getBits(buffer,len,32,24);
	  if (len >= 0x800000) {
// check for ECMWF large-file
	    int computed_len=0,sec_len,flag;
// PDS length
	    getBits(buffer,sec_len,64,24);
	    getBits(buffer,flag,120,8);
	    computed_len=8+sec_len;
	    fs.seekg(curr_offset+computed_len,std::ios_base::beg);
	    if ( (flag & 0x80) == 0x80) {
// GDS included
		fs.read(reinterpret_cast<char *>(buffer),3);
		getBits(buffer,sec_len,0,24);
		fs.seekg(sec_len-3,std::ios_base::cur);
		computed_len+=sec_len;
	    }
	    if ( (flag & 0x40) == 0x40) {
// BMS included
		fs.read(reinterpret_cast<char *>(buffer),3);
		getBits(buffer,sec_len,0,24);
		fs.seekg(sec_len-3,std::ios_base::cur);
		computed_len+=sec_len;
	    }
	    fs.read(reinterpret_cast<char *>(buffer),3);
// BDS length
	    getBits(buffer,sec_len,0,24);
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
	  getBits(buffer,len,96,32);
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
  int len=-9,llen;
  int bytesRead=0;

  while (len == -9) {
    bytesRead=findGRIB(buffer);
    if (bytesRead < 0) {
	return bytesRead;
    }
    fs.read(reinterpret_cast<char *>(&buffer[4]),12);
    bytesRead+=fs.gcount();
    if (bytesRead != 16) {
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
	  bytesRead+=fs.gcount();
	  if (bytesRead != 31) {
	    return bfstream::eof;
	  }
	  len=28;
	  while (buffer[bytesRead-3] != '7' && buffer[bytesRead-2] != '7' && buffer[bytesRead-1] != '7') {
	    getBits(buffer,llen,len*8,24);
	    if (llen <= 0) {
		return bfstream::error;
	    }
	    len+=llen;
	    if (len > static_cast<int>(buffer_length)) {
		myerror="GRIB0 length "+strutils::itos(len)+" overflows buffer length "+strutils::itos(buffer_length)+" on record "+strutils::itos(num_read+1);
		exit(1);
	    }
	    fs.read(reinterpret_cast<char *>(&buffer[bytesRead]),llen);
	    bytesRead+=fs.gcount();
	    if (bytesRead != len+3) {
		if (fs.eof()) {
		  return bfstream::eof;
		}
		else {
		  return bfstream::error;
		}
	    }
	  }
	  fs.read(reinterpret_cast<char *>(&buffer[bytesRead]),1);
	  bytesRead+=fs.gcount();
	  break;
	}
	case 1:
	{
	  getBits(buffer,len,32,24);
	  if (len > static_cast<int>(buffer_length)) {
	    myerror="GRIB1 length "+strutils::itos(len)+" overflows buffer length "+strutils::itos(buffer_length)+" on record "+strutils::itos(num_read+1);
	    exit(1);
	  }
	  fs.read(reinterpret_cast<char *>(&buffer[16]),len-16);
	  bytesRead+=fs.gcount();
	  if (bytesRead != len) {
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
	    getBits(buffer,sec_len,64,24);
	    getBits(buffer,flag,120,8);
	    computed_len=8+sec_len;
	    if ( (flag & 0x80) == 0x80) {
// GDS included
		getBits(buffer,sec_len,computed_len*8,24);
		computed_len+=sec_len;
	    }
	    if ( (flag & 0x40) == 0x40) {
// BMS included
		getBits(buffer,sec_len,computed_len*8,24);
		computed_len+=sec_len;
	    }
// BDS length
	    getBits(buffer,sec_len,computed_len*8,24);
	    if (sec_len < 120) {
		len&=0x7fffff;
		len*=120;
		len-=sec_len;
		len+=4;
		if (len > static_cast<int>(buffer_length)) {
		  myerror="GRIB1 length "+strutils::itos(len)+" overflows buffer length "+strutils::itos(buffer_length)+" on record "+strutils::itos(num_read+1);
		  exit(1);
		}
		fs.read(reinterpret_cast<char *>(&buffer[bytesRead]),len-bytesRead);
		bytesRead+=fs.gcount();
	    }
	  }
	  if (std::string(reinterpret_cast<char *>(&buffer[len-4]),4) != "7777") {
/*
	    if (len > 0x800000) {
// check for ECMWF large-file
		int computed_len=0,sec_len,flag;
// PDS length
		getBits(buffer,sec_len,64,24);
		getBits(buffer,flag,120,8);
		computed_len=8+sec_len;
		if ( (flag & 0x80) == 0x80) {
// GDS included
		  getBits(buffer,sec_len,computed_len*8,24);
		  computed_len+=sec_len;
		}
		if ( (flag & 0x40) == 0x40) {
// BMS included
		  getBits(buffer,sec_len,computed_len*8,24);
		  computed_len+=sec_len;
		}
// BDS length
		getBits(buffer,sec_len,computed_len*8,24);
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
	  getBits(buffer,len,96,32);
	  if (len > static_cast<int>(buffer_length)) {
	    myerror="GRIB2 length "+strutils::itos(len)+" overflows buffer length "+strutils::itos(buffer_length)+" on record "+strutils::itos(num_read+1);
	    exit(1);
	  }
	  fs.read(reinterpret_cast<char *>(&buffer[16]),len-16);
	  bytesRead+=fs.gcount();
	  if (bytesRead != len) {
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
  return bytesRead;
}

GRIBMessage::~GRIBMessage()
{
  if (pds_supp != nullptr) {
    delete[] pds_supp;
  }
  clearGrids();
}

GRIBMessage& GRIBMessage::operator=(const GRIBMessage& source)
{
  edition=source.edition;
  discipline=source.discipline;
  mlength=source.mlength;
  curr_off=source.curr_off;
  lengths=source.lengths;
  if (pds_supp != nullptr) {
    delete[] pds_supp;
    pds_supp=nullptr;
  }
  if (source.pds_supp != nullptr) {
    pds_supp=new unsigned char[lengths.pds_supp];
    for (int n=0; n < lengths.pds_supp; ++n) {
	pds_supp[n]=source.pds_supp[n];
    }
  }
  gds_included=source.gds_included;
  bms_included=source.bms_included;
  grids=source.grids;
  return *this;
}

void GRIBMessage::addGrid(const Grid *grid)
{
  Grid *g=NULL;

  switch (edition) {
    case 0:
    case 1:
	g=new GRIBGrid;
	*(reinterpret_cast<GRIBGrid *>(g))=*(reinterpret_cast<GRIBGrid *>(const_cast<Grid *>(grid)));
	if (reinterpret_cast<GRIBGrid *>(g)->bitmap.applies) {
	  bms_included=true;
	}
	break;
    case 2:
	g=new GRIB2Grid;
	*(reinterpret_cast<GRIB2Grid *>(g))=*(reinterpret_cast<GRIB2Grid *>(const_cast<Grid *>(grid)));
	break;
  }
  grids.emplace_back(g);
}

void GRIBMessage::convertToGRIB1()
{
  GRIBGrid *g;

  if (grids.size() > 1) {
    myerror="unable to convert multiple grids to GRIB1";
    exit(1);
  }
  if (grids.size() == 0) {
    myerror="no grid found";
    exit(1);
  }
  g=reinterpret_cast<GRIBGrid *>(grids.front());
  switch (edition) {
    case 0:
	lengths.is=8;
	lengths.pds=28;
	edition=1;
	g->grib.table=3;
	if (g->referenceDateTime.getYear() == 0 || g->referenceDateTime.getYear() > 30) {
	  g->grib.century=20;
	  g->referenceDateTime.setYear(g->referenceDateTime.getYear()+1900);
	}
	else {
	  g->grib.century=21;
	  g->referenceDateTime.setYear(g->referenceDateTime.getYear()+2000);
	}
	g->grib.sub_center=0;
	g->grib.D=0;
	g->remapParameters();
	break;
  }
}

void GRIBMessage::packLength(unsigned char *output_buffer) const
{
  switch (edition) {
    case 1:
	setBits(output_buffer,mlength,32,24);
	break;
    case 2:
	setBits(output_buffer,mlength,64,64);
	break;
  }
}

void GRIBMessage::packIS(unsigned char *output_buffer,off_t& offset,Grid *g) const
{
  setBits(output_buffer,0x47524942,0,32);
  switch (edition) {
    case 1:
	setBits(output_buffer,edition,56,8);
	offset=8;
	break;
    case 2:
	setBits(output_buffer,0,32,16);
	setBits(output_buffer,(reinterpret_cast<GRIB2Grid *>(g))->getDiscipline(),48,8);
	setBits(output_buffer,edition,56,8);
	offset=16;
	break;
    default:
	myerror="unable to encode GRIB "+strutils::itos(edition);
	exit(1);
  }
}

void GRIBMessage::packPDS(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  unsigned char flag=0x0;
  short hr,min;
  size_t dum;
  GRIBGrid *g;

  g=reinterpret_cast<GRIBGrid *>(grid);
  if (lengths.pds_supp > 0) {
    setBits(output_buffer,41+lengths.pds_supp,off,24);
    offset+=(41+lengths.pds_supp);
  }
  else {
    setBits(output_buffer,28,off,24);
    if ((g->referenceDateTime.getYear() % 100) != 0)
	setBits(output_buffer,g->referenceDateTime.getYear() % 100,off+96,8);
    else
	setBits(output_buffer,100,off+96,8);
    offset+=28;
  }
  if (edition == 0) {
    setBits(output_buffer,edition,off+24,8);
    setBits(output_buffer,g->referenceDateTime.getYear(),off+96,8);
  }
  else
    setBits(output_buffer,g->grib.table,off+24,8);
  setBits(output_buffer,g->grid.src,off+32,8);
  setBits(output_buffer,g->grib.process,off+40,8);
  setBits(output_buffer,g->grib.grid_catalog_id,off+48,8);
  if (gds_included) {
    flag|=0x80;
  }
  if (bms_included) {
    flag|=0x40;
  }
  setBits(output_buffer,flag,off+56,8);
  setBits(output_buffer,g->grid.param,off+64,8);
  setBits(output_buffer,g->grid.level1_type,off+72,8);
  switch (g->grid.level1_type) {
    case 101:
	dum=g->grid.level1/10.;
	setBits(output_buffer,dum,off+80,8);
	dum=g->grid.level2/10.;
	setBits(output_buffer,dum,off+88,8);
	break;
    case 107:
    case 119:
	dum=g->grid.level1*10000.;
	setBits(output_buffer,dum,off+80,16);
	break;
    case 108:
	dum=g->grid.level1*100.;
	setBits(output_buffer,dum,off+80,8);
	dum=g->grid.level2*100.;
	setBits(output_buffer,dum,off+88,8);
	break;
    case 114:
	dum=475.-(g->grid.level1);
	setBits(output_buffer,dum,off+80,8);
	dum=475.-(g->grid.level2);
	setBits(output_buffer,dum,off+88,8);
	break;
    case 128:
	dum=(1.1-(g->grid.level1))*1000.;
	setBits(output_buffer,dum,off+80,8);
	dum=(1.1-(g->grid.level2))*1000.;
	setBits(output_buffer,dum,off+88,8);
	break;
    case 141:
	dum=g->grid.level1/10.;
	setBits(output_buffer,dum,off+80,8);
	dum=1100.-(g->grid.level2);
	setBits(output_buffer,dum,off+88,8);
	break;
    case 104:
    case 106:
    case 110:
    case 112:
    case 116:
    case 120:
    case 121:
	dum=g->grid.level1;
	setBits(output_buffer,dum,off+80,8);
	dum=g->grid.level2;
	setBits(output_buffer,dum,off+88,8);
	break;
    default:
	dum=g->grid.level1;
	setBits(output_buffer,dum,off+80,16);
  }
  setBits(output_buffer,g->referenceDateTime.getMonth(),off+104,8);
  setBits(output_buffer,g->referenceDateTime.getDay(),off+112,8);
  hr=g->referenceDateTime.getTime()/10000;
  min=(g->referenceDateTime.getTime() % 10000)/100;
  setBits(output_buffer,hr,off+120,8);
  setBits(output_buffer,min,off+128,8);
  setBits(output_buffer,g->grib.time_units,off+136,8);
  if (g->grib.t_range != 10) {
    setBits(output_buffer,g->grib.p1,off+144,8);
    setBits(output_buffer,g->grib.p2,off+152,8);
  }
  else
    setBits(output_buffer,g->grib.p1,off+144,16);
  setBits(output_buffer,g->grib.t_range,off+160,8);
  setBits(output_buffer,g->grid.nmean,off+168,16);
  setBits(output_buffer,g->grib.nmean_missing,off+184,8);
  if (edition == 1) {
    if ((g->referenceDateTime.getYear() % 100) != 0)
	setBits(output_buffer,g->referenceDateTime.getYear()/100+1,off+192,8);
    else
	setBits(output_buffer,g->referenceDateTime.getYear()/100,off+192,8);
    setBits(output_buffer,g->grib.sub_center,off+200,8);
    if (g->grib.D < 0)
	setBits(output_buffer,-(g->grib.D)+0x8000,off+208,16);
    else
	setBits(output_buffer,g->grib.D,off+208,16);
  }
// pack the PDS supplement, if it exists
  if (lengths.pds_supp > 0) {
    setBits(output_buffer,pds_supp,off+320,8,0,lengths.pds_supp);
  }
}

void GRIBMessage::packGDS(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  short sign;
  int dum;
  GRIBGrid *g;

  g=reinterpret_cast<GRIBGrid *>(grid);
setBits(output_buffer,255,off+24,8);
setBits(output_buffer,255,off+32,8);
  setBits(output_buffer,g->grid.grid_type,off+40,8);
  switch (g->grid.grid_type) {
// Latitude/Longitude
    case 0:
// Gaussian Lat/Lon
    case 4:
	setBits(output_buffer,g->dim.x,off+48,16);
	setBits(output_buffer,g->dim.y,off+64,16);
	dum=lround(g->def.slatitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	setBits(output_buffer,sign,off+80,1);
	setBits(output_buffer,dum,off+81,23);
	dum=lround(g->def.slongitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	setBits(output_buffer,sign,off+104,1);
	setBits(output_buffer,dum,off+105,23);
	setBits(output_buffer,g->grib.rescomp,off+128,8);
	dum=lround(g->def.elatitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	setBits(output_buffer,sign,off+136,1);
	setBits(output_buffer,dum,off+137,23);
	dum=lround(g->def.elongitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	setBits(output_buffer,sign,off+160,1);
	setBits(output_buffer,dum,off+161,23);
	if (myequalf(g->def.loincrement,-99.999))
	  dum=0xffff;
	else
	  dum=lround(g->def.loincrement*1000.);
	setBits(output_buffer,dum,off+184,16);
	if (g->grid.grid_type == 0) {
	  if (myequalf(g->def.laincrement,-99.999)) {
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
	setBits(output_buffer,dum,off+200,16);
	setBits(output_buffer,g->grib.scan_mode,off+216,8);
	setBits(output_buffer,0,off+224,32);
	setBits(output_buffer,32,off,24);
	offset+=32;
	break;
// Lambert Conformal
    case 3:
// Polar Stereographic
    case 5:
	setBits(output_buffer,g->dim.x,off+48,16);
	setBits(output_buffer,g->dim.y,off+64,16);
	dum=lround(g->def.slatitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	setBits(output_buffer,sign,off+80,1);
	setBits(output_buffer,dum,off+81,23);
	dum=lround(g->def.slongitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	setBits(output_buffer,sign,off+104,1);
	setBits(output_buffer,dum,off+105,23);
	setBits(output_buffer,g->grib.rescomp,off+128,8);
	dum=lround(g->def.olongitude*1000.);
	if (dum < 0) {
	  sign=1;
	  dum=-dum;
	}
	else
	  sign=0;
	setBits(output_buffer,sign,off+136,1);
	setBits(output_buffer,dum,off+137,23);
	dum=lround(g->def.dx*1000.);
	setBits(output_buffer,dum,off+160,24);
	dum=lround(g->def.dy*1000.);
	setBits(output_buffer,dum,off+184,24);
	setBits(output_buffer,g->def.projection_flag,off+208,8);
	setBits(output_buffer,g->grib.scan_mode,off+216,8);
	if (g->grid.grid_type == 3) {
	  dum=lround(g->def.stdparallel1*1000.);
	  if (dum < 0) {
	    sign=1;
	    dum=-dum;
	  }
	  else {
	    sign=0;
	  }
	  setBits(output_buffer,sign,off+224,1);
	  setBits(output_buffer,dum,off+225,23);
	  dum=lround(g->def.stdparallel2*1000.);
	  if (dum < 0) {
	    sign=1;
	    dum=-dum;
	  }
	  else {
	    sign=0;
	  }
	  setBits(output_buffer,sign,off+248,1);
	  setBits(output_buffer,dum,off+249,23);
	  dum=lround(g->grib.sp_lat*1000.);
	  if (dum < 0) {
		sign=1;
		dum=-dum;
	  }
	  else
		sign=0;
	  setBits(output_buffer,sign,off+272,1);
	  setBits(output_buffer,dum,off+273,23);
	  dum=lround(g->grib.sp_lon*1000.);
	  if (dum < 0) {
		sign=1;
		dum=-dum;
	  }
	  else
		sign=0;
	  setBits(output_buffer,sign,off+296,1);
	  setBits(output_buffer,dum,off+297,23);
	  setBits(output_buffer,40,off,24);
	  offset+=40;
	}
	else {
	  setBits(output_buffer,0,off+224,32);
	  setBits(output_buffer,32,off,24);
	  offset+=32;
	}
	break;
  }
}

void GRIBMessage::packBMS(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  int ub;
  GRIBGrid *g;
  GRIB2Grid *g2;

  switch (edition) {
    case 0:
    case 1:
	g=reinterpret_cast<GRIBGrid *>(grid);
	ub=8-(g->dim.size % 8);
	setBits(output_buffer,ub,off+24,8);
setBits(output_buffer,0,off+32,16);
	setBits(output_buffer,g->bitmap.map,off+48,1,0,g->dim.size);
	setBits(output_buffer,(g->dim.size+7)/8+6,off,24);
	offset+=(g->dim.size+7)/8+6;
	break;
    case 2:
	g2=reinterpret_cast<GRIB2Grid *>(grid);
	setBits(output_buffer,6,off+32,8);
	if (g2->bitmap.applies) {
setBits(output_buffer,0,off+40,8);
	  setBits(output_buffer,g2->bitmap.map,off+48,1,0,g2->dim.size);
	  setBits(output_buffer,(g2->dim.size+7)/8+6,off,32);
	  offset+=(g2->dim.size+7)/8;
	}
	else {
	  setBits(output_buffer,255,off+40,8);
	  setBits(output_buffer,6,off,32);
	}
	offset+=6;
	break;
  }
}

void GRIBMessage::packBDS(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  double d,e,range;
  int E=0,n,m,len;
  int cnt=0,num_packed=0,ibm_rep,*packed,max_pack;
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
    if (!myequalf(range,0.)) {
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
    setBits(output_buffer,(len+7)/8+11,off,24);
    if ( (ub=(len % 8)) > 0) {
	ub=8-ub;
    }
    setBits(output_buffer,ub,off+28,4);
    setBits(output_buffer,0,off+88+len,ub);
    offset+=(len+7)/8+11;
  }
  else {
    setBits(output_buffer,11,off,24);
    g->grib.pack_width=0;
    offset+=11;
  }
  setBits(output_buffer,g->grib.pack_width,off+80,8);
  flag=0x0;
  setBits(output_buffer,flag,off+24,4);
  ibm_rep=ibmconv(g->stats.min_val*d);
  setBits(output_buffer,abs(E),off+32,16);
  if (E < 0) {
    setBits(output_buffer,1,off+32,1);
  }
  setBits(output_buffer,ibm_rep,off+48,32);
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
//	    packed[cnt]=lroundf(((double)g->grid.pole-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->grid.pole*d)-lround(g->stats.min_val*d))/e);
	    cnt++;
	    break;
	}
    }
    switch (g->grid.grid_type) {
	case 0:
	case 3:
	case 4:
	  for (n=0; n < g->dim.y; n++) {
	    for (m=0; m < g->dim.x; m++) {
		if (!myequalf(g->gridpoints[n][m],Grid::missingValue)) {
//		  packed[cnt]=lroundf(((double)g->gridpoints[n][m]-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->gridpoints[n][m]*d)-lround(g->stats.min_val*d))/e);
		  cnt++;
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
//		  packed[cnt]=lroundf(((double)g->grid.pole-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->grid.pole*d)-lround(g->stats.min_val*d))/e);
		  break;
	    }
	  }
	  break;
	case 5:
	{
	  for (n=0; n < g->dim.y; ++n) {
	    for (m=0; m < g->dim.x; ++m) {
		if (!myequalf(g->gridpoints[n][m],Grid::missingValue)) {
//		  packed[cnt]=lroundf(((double)g->gridpoints[n][m]-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->gridpoints[n][m]*d)-lround(g->stats.min_val*d))/e);
		  ++cnt;
		}
	    }
	  }
	  break;
	}
	default:
	{
	  std::cerr << "Warning: packBDS does not recognize grid type " << g->grid.grid_type << std::endl;
	}
    }
    if (cnt != num_packed) {
	mywarning="packBDS: specified number of gridpoints "+strutils::itos(num_packed)+" does not agree with actual number packed "+strutils::itos(cnt)+"  date: "+g->referenceDateTime.toString()+" "+strutils::itos(g->grid.param);
    }
    setBits(output_buffer,packed,off+88,g->grib.pack_width,0,num_packed);
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

void GRIBMessage::packEND(unsigned char *output_buffer,off_t& offset) const
{
  setBits(output_buffer,0x37373737,offset*8,32);
  offset+=4;
}

off_t GRIBMessage::copyToBuffer(unsigned char *output_buffer,const size_t buffer_length)
{
  off_t offset;

  packIS(output_buffer,offset,grids.front());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copyToBuffer";
    exit(1);
  }
  packPDS(output_buffer,offset,grids.front());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copyToBuffer";
    exit(1);
  }
  if (gds_included) {
    packGDS(output_buffer,offset,grids.front());
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copyToBuffer";
	exit(1);
    }
  }
  if (bms_included) {
    packBMS(output_buffer,offset,grids.front());
  }
  packBDS(output_buffer,offset,grids.front());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copyToBuffer";
    exit(1);
  }
  packEND(output_buffer,offset);
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copyToBuffer";
    exit(1);
  }
  mlength=offset;
  packLength(output_buffer);
  return mlength;
}

void GRIBMessage::clearGrids()
{
  for (auto grid : grids)
    delete grid;
  grids.clear();
}

void GRIBMessage::unpackIS(const unsigned char *stream_buffer)
{
  int test;

  getBits(stream_buffer,test,0,32);
  if (test != 0x47524942) {
    myerror="not a GRIB message";
    exit(1);
  }
  getBits(stream_buffer,edition,56,8);
  switch (edition) {
    case 0:
	mlength=lengths.is=offsets.is=curr_off=4;
	break;
    case 1:
	getBits(stream_buffer,mlength,32,24);
	lengths.is=offsets.is=curr_off=8;
	break;
    case 2:
	getBits(stream_buffer,mlength,64,64);
	lengths.is=offsets.is=curr_off=16;
	break;
  }
}

void GRIBMessage::unpackPDS(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short flag,soff;
  short yr,mo,dy,hr,min;
  int dum;
  GRIBGrid *g;

  offsets.pds=curr_off;
  g=reinterpret_cast<GRIBGrid *>(grids.back());
  g->ensdata.fcst_type="";
  g->ensdata.ID="";
  g->ensdata.total_size=0;
  getBits(stream_buffer,lengths.pds,off,24);
  if (edition == 1) {
    getBits(stream_buffer,g->grib.table,off+24,8);
  }
  g->grib.last_src=g->grid.src;
  getBits(stream_buffer,g->grid.src,off+32,8);
  getBits(stream_buffer,g->grib.process,off+40,8);
  getBits(stream_buffer,g->grib.grid_catalog_id,off+48,8);
  g->grib.scan_mode=0x40;
  if (g->grid.src == 7) {
    switch (g->grib.grid_catalog_id) {
	case 21:
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
	case 22:
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
	case 23:
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
	case 24:
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
	case 25:
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
	case 26:
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
	case 61:
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
	case 62:
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
	case 63:
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
	case 64:
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
	default:
	  g->grib.scan_mode=0x0;
    }
  }
  getBits(stream_buffer,flag,off+56,8);
  if ( (flag & 0x80) == 0x80) {
    gds_included=true;
  }
  else {
    gds_included=false;
    lengths.gds=0;
  }
  if ( (flag & 0x40) == 0x40) {
    bms_included=true;
  }
  else {
    bms_included=false;
    lengths.bms=0;
  }
  getBits(stream_buffer,g->grid.param,off+64,8);
  getBits(stream_buffer,g->grid.level1_type,off+72,8);
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
	getBits(stream_buffer,dum,off+80,16);
	g->grid.level1=dum;
	g->grid.level2=0.;
	g->grid.level2_type=-1;
	break;
    case 101:
	getBits(stream_buffer,dum,off+80,8);
	g->grid.level1=dum*10.;
	getBits(stream_buffer,dum,off+88,8);
	g->grid.level2=dum*10.;
	g->grid.level2_type=g->grid.level1_type;
	break;
    case 107:
    case 119:
	getBits(stream_buffer,dum,off+80,16);
	g->grid.level1=dum/10000.;
	g->grid.level2=0.;
	g->grid.level2_type=-1;
	break;
    case 108:
    case 120:
	getBits(stream_buffer,dum,off+80,8);
	g->grid.level1=dum/100.;
	getBits(stream_buffer,dum,off+88,8);
	g->grid.level2=dum/100.;
	g->grid.level2_type=g->grid.level1_type;
	break;
    case 114:
	getBits(stream_buffer,dum,off+80,8);
	g->grid.level1=475.-dum;
	getBits(stream_buffer,dum,off+88,8);
	g->grid.level2=475.-dum;
	g->grid.level2_type=g->grid.level1_type;
	break;
    case 121:
	getBits(stream_buffer,dum,off+80,8);
	g->grid.level1=1100.-dum;
	getBits(stream_buffer,dum,off+88,8);
	g->grid.level2=1100.-dum;
	g->grid.level2_type=g->grid.level1_type;
	break;
    case 128:
	getBits(stream_buffer,dum,off+80,8);
	g->grid.level1=1.1-dum/1000.;
	getBits(stream_buffer,dum,off+88,8);
	g->grid.level2=1.1-dum/1000.;
	g->grid.level2_type=g->grid.level1_type;
	break;
    case 141:
	getBits(stream_buffer,dum,off+80,8);
	g->grid.level1=dum*10.;
	getBits(stream_buffer,dum,off+88,8);
	g->grid.level2=1100.-dum;
	g->grid.level2_type=g->grid.level1_type;
	break;
    default:
	getBits(stream_buffer,dum,off+80,8);
	g->grid.level1=dum;
	getBits(stream_buffer,dum,off+88,8);
	g->grid.level2=dum;
	g->grid.level2_type=g->grid.level1_type;
  }
  getBits(stream_buffer,yr,off+96,8);
  getBits(stream_buffer,mo,off+104,8);
  getBits(stream_buffer,dy,off+112,8);
  getBits(stream_buffer,hr,off+120,8);
  getBits(stream_buffer,min,off+128,8);
  hr=hr*100+min;
  getBits(stream_buffer,g->grib.time_units,off+136,8);
  switch (edition) {
    case 0:
	if (yr > 20) {
	  yr+=1900;
	}
	else {
	  yr+=2000;
	}
	g->grib.sub_center=0;
	g->grib.D=0;
	break;
    case 1:
	getBits(stream_buffer,g->grib.century,off+192,8);
	yr=yr+(g->grib.century-1)*100;
	getBits(stream_buffer,g->grib.sub_center,off+200,8);
	getBits(stream_buffer,g->grib.D,off+208,16);
	if (g->grib.D > 0x8000) {
	  g->grib.D=0x8000-g->grib.D;
	}
	break;
  }
  g->referenceDateTime.set(yr,mo,dy,hr*100);
  getBits(stream_buffer,g->grib.t_range,off+160,8);
  g->grid.nmean=g->grib.nmean_missing=0;
  g->grib.p2=0;
  switch (g->grib.t_range) {
    case 10:
	getBits(stream_buffer,g->grib.p1,off+144,16);
	break;
    default:
	getBits(stream_buffer,g->grib.p1,off+144,8);
	getBits(stream_buffer,g->grib.p2,off+152,8);
	getBits(stream_buffer,g->grid.nmean,off+168,16);
	getBits(stream_buffer,g->grib.nmean_missing,off+184,8);
	if (g->grib.t_range >= 113 && g->grib.time_units == 1 && g->grib.p2 > 24) {
	  std::cerr << "Warning: invalid P2 " << g->grib.p2;
	  getBits(stream_buffer,g->grib.p2,off+152,4);
	  std::cerr << " re-read as " << g->grib.p2 << " for time range " << g->grib.t_range << std::endl;
	}
  }
  g->forecastDateTime=g->validDateTime=g->referenceDateTime;
  g->grib.prod_descr=gributils::getGRIBProductDescription(g,g->forecastDateTime,g->validDateTime,g->grid.fcst_time);
// unpack PDS supplement, if it exists
  if (pds_supp != NULL) {
    delete[] pds_supp;
    pds_supp=NULL;
  }
  if (lengths.pds > 28) {
    if (lengths.pds < 41) {
	mywarning="supplement to PDS is in reserved position starting at octet 29";
	lengths.pds_supp=lengths.pds-28;
	soff=28;
    }
    else {
	lengths.pds_supp=lengths.pds-40;
	soff=40;
    }
    pds_supp=new unsigned char[lengths.pds_supp];
    memcpy(pds_supp,&stream_buffer[lengths.is+soff],lengths.pds_supp);
// NCEP ensemble grids are described in the PDS extension
    if (g->grid.src == 7 && g->grib.sub_center == 2 && pds_supp[0] == 1) {
	switch (pds_supp[1]) {
	  case 1:
	    switch (pds_supp[2]) {
		case 1:
		  g->ensdata.fcst_type="HRCTL";
		  break;
		case 2:
		  g->ensdata.fcst_type="LRCTL";
		  break;
	    }
	    break;
	  case 2:
	    g->ensdata.fcst_type="NEG";
	    g->ensdata.ID=strutils::itos(pds_supp[2]);
	    break;
	  case 3:
	    g->ensdata.fcst_type="POS";
	    g->ensdata.ID=strutils::itos(pds_supp[2]);
	    break;
	  case 5:
	    g->ensdata.ID="ALL";
	    switch (static_cast<int>(pds_supp[3])) {
		case 1:
		  g->ensdata.fcst_type="UNWTD_MEAN";
		  break;
		case 11:
		  g->ensdata.fcst_type="STDDEV";
		  break;
		default:
		  myerror="PDS byte 44 value "+std::string(&(reinterpret_cast<char *>(pds_supp))[3],1)+" not recognized";
		  exit(1);
	    }
	    if (lengths.pds_supp > 20) {
		g->ensdata.total_size=pds_supp[20];
	    }
	    break;
	  default:
	    myerror="PDS byte 42 value "+std::string(&(reinterpret_cast<char *>(pds_supp))[1],1)+" not recognized";
	    exit(1);
	}
    }
  }
  else {
    lengths.pds_supp=0;
  }
  g->grid.num_missing=0;
  curr_off+=lengths.pds;
}

void GRIBMessage::unpackGDS(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  GRIBGrid *g;
  int n,noff;
  int dum,sl_sign;
  short PV,NV,PL;

  offsets.gds=curr_off;
  g=reinterpret_cast<GRIBGrid *>(grids.back());
  getBits(stream_buffer,lengths.gds,off,24);
  PL=0xff;
  switch (edition) {
    case 1:
	getBits(stream_buffer,PV,off+32,8);
	if (PV != 0xff) {
	  getBits(stream_buffer,NV,off+24,8);
	  if (NV > 0)
	    PL=(4*NV)+PV;
	  else
	    PL=PV;
/*
    if (grib.PL >= grib.lengths.gds) {
	std::cerr << "Error in unpackGDS: grib.PL (" << grib.PL << ") exceeds grib.lengths.gds (" << grib.lengths.gds << "); NV=" << grib.NV << ", PV=" << grib.PV << std::endl;
	exit(1);
    }
*/
	  if (PL > lengths.gds) {
	    PL=0xff;
	  }
	  else {
	    if (g->plist != NULL) {
		delete[] g->plist;
	    }
	    getBits(stream_buffer,g->dim.y,off+64,16);
	    g->plist=new int[g->dim.y];
	    getBits(stream_buffer,g->plist,(PL+curr_off-1)*8,16,0,g->dim.y);
	  }
	}
	break;
  }
  getBits(stream_buffer,g->grid.grid_type,off+40,8);
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
// Latitude/Longitude
    case 0:
// Gaussian Lat/Lon
    case 4:
// Rotated Lat/Lon
    case 10:
    {
	getBits(stream_buffer,g->dim.x,off+48,16);
	getBits(stream_buffer,g->dim.y,off+64,16);
	if (g->dim.x == -1 && PL != 0xff) {
	  noff=off+(PL-1)*8;
	  g->dim.size=0;
	  for (n=PL; n < lengths.gds; n+=2) {
	    getBits(stream_buffer,dum,noff,16);
	    g->dim.size+=dum;
	    noff+=16;
	  }
	}
	else {
	  g->dim.size=g->dim.x*g->dim.y;
	}
	getBits(stream_buffer,dum,off+80,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.slatitude=dum/1000.;
	getBits(stream_buffer,dum,off+104,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.slongitude=dum/1000.;
/*
	dum=lround(64.*g->def.slongitude);
	g->def.slongitude=dum/64.;
*/
	getBits(stream_buffer,g->grib.rescomp,off+128,8);
	getBits(stream_buffer,dum,off+136,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.elatitude=dum/1000.;
	getBits(stream_buffer,sl_sign,off+160,1);
	sl_sign= (sl_sign == 0) ? 1 : -1;
	getBits(stream_buffer,dum,off+161,23);
	g->def.elongitude=(dum/1000.)*sl_sign;
/*
	dum=lround(64.*g->def.elongitude);
	g->def.elongitude=dum/64.;
*/
	getBits(stream_buffer,dum,off+184,16);
	if (dum == 0xffff) {
	  dum=-99000;
	}
	g->def.loincrement=dum/1000.;
/*
	dum=lround(64.*g->def.loincrement);
	g->def.loincrement=dum/64.;
*/
	getBits(stream_buffer,dum,off+200,16);
	if (dum == 0xffff) {
	  dum=-99999;
	}
	if (g->grid.grid_type == 0) {
	  g->def.laincrement=dum/1000.;
	}
	else {
	  g->def.num_circles=dum;
	}
	getBits(stream_buffer,g->grib.scan_mode,off+216,8);
/*
	if (g->def.elongitude < g->def.slongitude && g->grib.scan_mode < 0x80) {
	  g->def.elongitude+=360.;
	}
*/
	break;
    }
// Lambert Conformal
    case 3:
// Polar Stereographic
    case 5:
    {
	getBits(stream_buffer,g->dim.x,off+48,16);
	getBits(stream_buffer,g->dim.y,off+64,16);
	g->dim.size=g->dim.x*g->dim.y;
	getBits(stream_buffer,dum,off+80,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.slatitude=dum/1000.;
	getBits(stream_buffer,dum,off+104,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.slongitude=dum/1000.;
	getBits(stream_buffer,dum,off+104,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.slongitude=dum/1000.;
	getBits(stream_buffer,g->grib.rescomp,off+128,8);
	getBits(stream_buffer,dum,off+136,24);
	if (dum > 0x800000) {
	  dum=0x800000-dum;
	}
	g->def.olongitude=dum/1000.;
	getBits(stream_buffer,dum,off+160,24);
	g->def.dx=dum/1000.;
	getBits(stream_buffer,dum,off+184,24);
	g->def.dy=dum/1000.;
	getBits(stream_buffer,g->def.projection_flag,off+208,8);
	getBits(stream_buffer,g->grib.scan_mode,off+216,8);
	if (g->grid.grid_type == 3) {
	  getBits(stream_buffer,dum,off+224,24);
	  if (dum > 0x800000) {
	    dum=0x800000-dum;
	  }
	  g->def.stdparallel1=dum/1000.;
	  getBits(stream_buffer,dum,off+248,24);
	  if (dum > 0x800000) {
	    dum=0x800000-dum;
	  }
	  g->def.stdparallel2=dum/1000.;
	  getBits(stream_buffer,dum,off+272,24);
	  if (dum > 0x800000) {
	    dum=0x800000-dum;
	  }
	  g->grib.sp_lat=dum/1000.;
	  getBits(stream_buffer,dum,off+296,24);
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
	getBits(stream_buffer,g->def.trunc1,off+48,16);
	getBits(stream_buffer,g->def.trunc2,off+64,16);
	getBits(stream_buffer,g->def.trunc3,off+80,16);
	break;
    }
    default:
    {
	std::cerr << "Warning: unpackGDS does not recognize data representation " << g->grid.grid_type << std::endl;
    }
  }
  curr_off+=lengths.gds;
}

void GRIBMessage::unpackBMS(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  GRIBGrid *g;
  GRIB2Grid *g2;
  short sec_num,ind;
  int n,len;

  offsets.bms=curr_off;
  switch (edition) {
    case 1:
	g=reinterpret_cast<GRIBGrid *>(grids.back());
	getBits(stream_buffer,lengths.bms,off,24);
	getBits(stream_buffer,ind,off+32,16);
	if (ind == 0) {
	  if (g->dim.size > g->bitmap.capacity) {
	    if (g->bitmap.map != NULL) {
		delete[] g->bitmap.map;
	    }
	    g->bitmap.capacity=g->dim.size;
	    g->bitmap.map=new unsigned char[g->bitmap.capacity];
	  }
	  getBits(stream_buffer,g->bitmap.map,off+48,1,0,g->dim.size);
	  for (n=0; n < static_cast<int>(g->dim.size); n++) {
	    if (g->bitmap.map[n] == 0)
		g->grid.num_missing++;
	  }
	}
	break;
    case 2:
	g2=reinterpret_cast<GRIB2Grid *>(grids.back());
	getBits(stream_buffer,lengths.bms,off,32);
	getBits(stream_buffer,sec_num,off+32,8);
	if (sec_num != 6) {
	  mywarning="may not be unpacking the Bitmap Section";
	}
	getBits(stream_buffer,ind,off+40,8);
	switch (ind) {
	  case 0:
// bitmap applies and is specified in this section
	    if ( (len=(lengths.bms-6)*8) > 0) {
		if (len > static_cast<int>(g2->bitmap.capacity)) {
		  if (g2->bitmap.map != NULL) {
		    delete[] g2->bitmap.map;
		  }
		  g2->bitmap.capacity=len;
		  g2->bitmap.map=new unsigned char[g2->bitmap.capacity];
		}
/*
		for (n=0; n < len; n++) {
		  getBits(stream_buffer,bit,off+48+n,1);
		  g2->bitmap.map[n]=bit;
		}
*/
getBits(stream_buffer,g2->bitmap.map,off+48,1,0,len);
		g2->bitmap.applies=true;
	    }
	    else {
		g2->bitmap.applies=false;
	    }
	    break;
	  case 254:
// bitmap has already been defined for the current GRIB2 message
	    g2->bitmap.applies=true;
	    break;
	  case 255:
// bitmap does not apply to this message
	    g2->bitmap.applies=false;
	    break;
	  default:
	    myerror="not currently setup to deal with pre-defined bitmaps";
	    exit(1);
	}
	break;
  }
  curr_off+=lengths.bms;
}

void GRIBMessage::unpackBDS(const unsigned char *stream_buffer,bool fill_header_only)
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

  offsets.bds=curr_off;
  g=reinterpret_cast<GRIBGrid *>(grids.back());
  getBits(stream_buffer,lengths.bds,off,24);
  if (mlength >= 0x800000 && lengths.bds < 120) {
// ECMWF large-file
    mlength&=0x7fffff;
    mlength*=120;
    mlength-=lengths.bds;
    mlength+=4;
    lengths.bds=mlength-curr_off-4;
  }
  getBits(stream_buffer,bds_flag,off+24,4);
  if ((bds_flag & 0x4) == 0x4) {
    simple_packing=false;
  }
  getBits(stream_buffer,unused,off+28,4);
  d=pow(10.,g->grib.D);
  g->stats.min_val=ibmconv(stream_buffer,off+48)/d;
  if ((bds_flag & 0x2) == 0x2) {
    g->stats.min_val=lround(g->stats.min_val);
  }
  getBits(stream_buffer,g->grib.pack_width,off+80,8);
  getBits(stream_buffer,g->grib.E,off+32,16);
  if (g->grib.E > 0x8000) {
    g->grib.E=0x8000-g->grib.E;
  }
  if (simple_packing) {
    if (g->grib.pack_width > 0) {
	if (g->def.type != Grid::sphericalHarmonicsType) {
	  num_packed=g->dim.size-g->grid.num_missing;
	  npoints=((lengths.bds-11)*8-unused)/g->grib.pack_width;
	  if (npoints != num_packed && g->dim.size != 0 && npoints != g->dim.size)
	    std::cerr << "Warning: unpackBDS: specified # gridpoints " << num_packed << " vs. # packed " << npoints << "  date: " << g->referenceDateTime.toString() << "  param: " << g->grid.param << "  lengths.bds: " << lengths.bds << "  ubits: " << unused << "  pack_width: " << g->grib.pack_width << std::endl;
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
    g->grid.pole=Grid::missingValue;
    g->stats.max_val=-Grid::missingValue;
    g->stats.avg_val=0.;
    g->galloc();
    if (simple_packing) {
	pval=new int[num_packed];
	getBits(stream_buffer,pval,off+88,g->grib.pack_width,0,num_packed);
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
			    g->gridpoints[n][m]=g->stats.min_val+pval[cnt]*e/d;
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
			    g->gridpoints[n][m]=g->stats.min_val;
			    if (g->stats.min_i < 0) {
				g->stats.min_i=m+1;
				if (g->def.type == Grid::gaussianLatitudeLongitudeType && g->grib.scan_mode == 0x40)
				  g->stats.min_j=ydim-n;
				else
				  g->stats.min_j=n+1;
			    }
			  }
			  if (g->gridpoints[n][m] > g->stats.max_val) {
			    g->stats.max_val=g->gridpoints[n][m];
			    g->stats.max_i=m+1;
			    g->stats.max_j=n+1;
			  }
			  g->stats.avg_val+=g->gridpoints[n][m];
			  avg_cnt++;
			  cnt++;
			}
			else {
			  g->gridpoints[n][m]=Grid::missingValue;
			}
			++bcnt;
		    }
		    else {
			if (g->grib.pack_width > 0) {
			  g->gridpoints[n][m]=g->stats.min_val+pval[cnt]*e/d;
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
			  g->gridpoints[n][m]=g->stats.min_val;
			  if (g->stats.min_i < 0) {
			    g->stats.min_i=m+1;
			    if (g->def.type == Grid::gaussianLatitudeLongitudeType && g->grib.scan_mode == 0x40)
				g->stats.min_j=ydim-n;
			    else
				g->stats.min_j=n+1;
			  }
			}
			if (g->gridpoints[n][m] > g->stats.max_val) {
			  g->stats.max_val=g->gridpoints[n][m];
			  g->stats.max_i=m+1;
			  g->stats.max_j=n+1;
			}
			g->stats.avg_val+=g->gridpoints[n][m];
			avg_cnt++;
			cnt++;
		    }
		  }
		}
// reduced grids
		else {
		  for (m=1; m <= static_cast<int>(g->gridpoints[n][0]); ++m) {
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
			  g->gridpoints[n][m]=g->stats.min_val+pval[cnt]*e/d;
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
			  g->gridpoints[n][m]=g->stats.min_val;
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
			if (g->gridpoints[n][m] > g->stats.max_val) {
		 	  g->stats.max_val=g->gridpoints[n][m];
			  g->stats.max_i=m;
			  g->stats.max_j=n+1;
			}
			g->stats.avg_val+=g->gridpoints[n][m];
			++avg_cnt;
			++cnt;
		    }
		  }
		}
	    }
	    if (pole_at >= 0) {
// pole was first point packed
		if (ystart == 1) {
		  for (n=0; n < g->dim.x; g->gridpoints[0][n++]=g->grid.pole);
		}
// pole was last point packed
		else if (ydim != g->dim.y) {
		  for (n=0; n < g->dim.x; g->gridpoints[ydim][n++]=g->grid.pole);
		}
	    }
	    else {
		if (myequalf(g->def.slatitude,90.)) {
		  g->grid.pole=g->gridpoints[0][0];
		}
		else if (myequalf(g->def.elatitude,90.)) {
		  g->grid.pole=g->gridpoints[ydim-1][0];
		}
	    }
	    break;
	  default:
	    std::cerr << "Warning: unpackBDS does not recognize grid type " << g->grid.grid_type << std::endl;
	}
	delete[] pval;
    }
    else {
// second-order packing
 	if ((bds_flag & 0x4) == 0x4) {
	  if ((bds_flag & 0xc) == 0xc) {
/*
std::cerr << "here" << std::endl;
getBits(input_buffer,n,off+88,16);
std::cerr << "n=" << n << std::endl;
getBits(input_buffer,n,off+104,16);
std::cerr << "p=" << n << std::endl;
getBits(input_buffer,n,off+120,8);
getBits(input_buffer,m,off+128,8);
getBits(input_buffer,cnt,off+136,8);
std::cerr << "j,k,m=" << n << "," << m << "," << cnt << std::endl;
*/
	  }
	  else {
if (bms_included) {
myerror="unable to decode complex packing with bitmap defined";
exit(1);
}
	    short n1,ext_flg,n2,p1,p2;
	    getBits(stream_buffer,n1,off+88,16);
	    getBits(stream_buffer,ext_flg,off+104,8);
if ( (ext_flg & 0x20) == 0x20) {
myerror="unable to decode complex packing with secondary bitmap defined";
exit(1);
}
	    getBits(stream_buffer,n2,off+112,16);
	    getBits(stream_buffer,p1,off+128,16);
	    getBits(stream_buffer,p2,off+144,16);
	    if ( (ext_flg & 0x8) == 0x8) {
// ECMWF extension for complex packing with spatial differencing
		short width_pack_width,length_pack_width,nl,order_vals_width;
		getBits(stream_buffer,width_pack_width,off+168,8);
		getBits(stream_buffer,length_pack_width,off+176,8);
		getBits(stream_buffer,nl,off+184,16);
		getBits(stream_buffer,order_vals_width,off+200,8);
		auto norder=(ext_flg & 0x3);
		std::unique_ptr<int []> first_vals;
		first_vals.reset(new int[norder]);
		getBits(stream_buffer,first_vals.get(),off+208,order_vals_width,0,norder);
		short sign;
		int omin=0;
		auto noff=off+208+norder*order_vals_width;
		getBits(stream_buffer,sign,noff,1);
		getBits(stream_buffer,omin,noff+1,order_vals_width-1);
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
		getBits(stream_buffer,widths.get(),noff,width_pack_width,0,p1);
		getBits(stream_buffer,lengths.get(),off+(nl-1)*8,length_pack_width,0,p1);
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
		getBits(stream_buffer,group_ref_vals.get(),off+(n1-1)*8,8,0,p1);
		std::unique_ptr<int []> pvals(new int[max_length]);
		noff=off+(n2-1)*8;
		for (auto n=0; n < norder; ++n) {
		  auto j=n/g->dim.x;
		  auto i=(n % g->dim.x);
		  g->gridpoints[j][i]=g->stats.min_val+first_vals[n]*e/d;
		  if (g->gridpoints[j][i] > g->stats.max_val) {
		    g->stats.max_val=g->gridpoints[j][i];
		  }
		  g->stats.avg_val+=g->gridpoints[j][i];
		  ++avg_cnt;
		}
// unpack the field of differences
		for (int n=0,l=norder; n < p1; ++n) {
		  if (widths[n] > 0) {
		    getBits(stream_buffer,pvals.get(),noff,widths[n],0,lengths[n]);
		    noff+=widths[n]*lengths[n];
 		    for (auto m=0; m < lengths[n]; ++m) {
			auto j=l/g->dim.x;
			auto i=(l % g->dim.x);
			g->gridpoints[j][i]=pvals[m]+group_ref_vals[n]+omin;
			++l;
		    }
		  }
		  else {
// constant group
 		    for (auto m=0; m < lengths[n]; ++m) {
			auto j=l/g->dim.x;
			auto i=(l % g->dim.x);
			g->gridpoints[j][i]=group_ref_vals[n]+omin;
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
		    g->gridpoints[j][i]+=lastgp;
		    lastgp=g->gridpoints[j][i];
		  }
		}
		lastgp=g->stats.min_val*d/e+first_vals[norder-1];
		for (size_t l=norder; l < g->dim.size; ++l) {
		  auto j=l/g->dim.x;
		  auto i=(l % g->dim.x);
		  lastgp+=g->gridpoints[j][i];
		  g->gridpoints[j][i]=lastgp*e/d;
		  if (g->gridpoints[j][i] > g->stats.max_val) {
		    g->stats.max_val=g->gridpoints[j][i];
		  }
		  g->stats.avg_val+=g->gridpoints[j][i];
		  ++avg_cnt;
		}
		if ((ext_flg & 0x2) == 0x2) {
// gridpoints are boustrophedonic
 		  for (auto j=0; j < g->dim.y; ++j) {
		    if ( (j % 2) == 1) {
			for (auto i=0,l=g->dim.x-1; i < g->dim.x/2; ++i,--l) {
			  auto temp=g->gridpoints[j][i];
			  g->gridpoints[j][i]=g->gridpoints[j][l];
			  g->gridpoints[j][l]=temp;
			}
		    }
		  }
		}
	    }
	    else {
		std::unique_ptr<short []> p2_widths;
		if ((ext_flg & 0x10) == 0x10) {
		  p2_widths.reset(new short[p1]);
		  getBits(stream_buffer,p2_widths.get(),off+168,8,0,p1);
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
  curr_off+=lengths.bds;
}

void GRIBMessage::unpackEND(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  int test;

  getBits(stream_buffer,test,off,32);
  if (test != 0x37373737) {
    myerror="bad END Section - found *"+std::string(reinterpret_cast<char *>(const_cast<unsigned char *>(&stream_buffer[curr_off])),4)+"*";
    exit(1);
  }
}

void GRIBMessage::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  GRIBGrid *g;
  xmlutils::GridMapEntry gme;
  std::string sdum;

  if (stream_buffer == NULL) {
    myerror="empty file stream";
    exit(1);
  }
  clearGrids();
  unpackIS(stream_buffer);
  if (edition > 1) {
    myerror="can't decode edition "+strutils::itos(edition);
    exit(1);
  }
  g=new GRIBGrid;
  g->grid.filled=false;
  grids.emplace_back(g);
  unpackPDS(stream_buffer);
  if (edition == 0) {
    mlength+=lengths.pds;
  }
  if (gds_included) {
    unpackGDS(stream_buffer);
    if (edition == 0) {
	mlength+=lengths.gds;
    }
  }
  else {
    static my::map<xmlutils::GridMapEntry> gridMapTable;
    gme.key=strutils::itos(g->grid.src)+"-"+strutils::itos(g->grib.sub_center);
    if (!gridMapTable.found(gme.key,gme)) {
	static TempDir *temp_dir=nullptr;
	if (temp_dir == nullptr) {
	  temp_dir=new TempDir;
	  if (!temp_dir->create("/tmp")) {
	    myerror="unable to create temporary directory";
	    exit(1);
	  }
	}
	gme.g.fill(xmlutils::getMapFilename("GridTables",gme.key,temp_dir->name()));
	gridMapTable.insert(gme);
    }
    sdum=strutils::itos(g->grib.grid_catalog_id);
    g->def=gme.g.getGridDefinition(sdum);
    g->dim=gme.g.getGridDimensions(sdum);
    if (gme.g.isSinglePolePoint(sdum)) {
	g->grid.num_in_pole_sum=-std::stoi(gme.g.getPolePointLocation(sdum));
    }
    g->grib.scan_mode=0x40;
  }
  if (g->dim.x == 0 || g->dim.y == 0 || g->dim.size == 0) {
    mywarning="grid dimensions not defined";
  }
  if (bms_included) {
    unpackBMS(stream_buffer);
    if (edition == 0) {
	mlength+=lengths.bms;
    }
  }
  unpackBDS(stream_buffer,fill_header_only);
  if (edition == 0) {
    mlength+=(lengths.bds+4);
  }
  unpackEND(stream_buffer);
}

Grid *GRIBMessage::getGrid(size_t grid_number) const
{
  if (grids.size() == 0 || grid_number >= grids.size())
    return NULL;
  else
    return grids[grid_number];
}

void GRIBMessage::getPDSSupplement(unsigned char *pds_supplement,size_t& pds_supplement_length) const
{
  size_t n;

  pds_supplement_length=lengths.pds_supp;
  if (pds_supplement_length > 0) {
    for (n=0; n < pds_supplement_length; ++n) {
	pds_supplement[n]=pds_supp[n];
    }
  }
  else {
    pds_supplement=NULL;
  }
}

void GRIBMessage::initialize(short edition_number,unsigned char *pds_supplement,int pds_supplement_length,bool gds_is_included,bool bms_is_included)
{
  edition=edition_number;
  switch (edition) {
    case 0:
	myerror="can't create GRIB0";
	exit(1);
    case 1:
	lengths.is=8;
	break;
    case 2:
	lengths.is=16;
	break;
  }
  if (pds_supp != NULL) {
    delete[] pds_supp;
    pds_supp=NULL;
  }
  lengths.pds_supp=pds_supplement_length;
  if (lengths.pds_supp > 0) {
    pds_supp=new unsigned char[lengths.pds_supp];
  }
  else {
    pds_supp=NULL;
  }
  memcpy(pds_supp,pds_supplement,lengths.pds_supp);
  gds_included=gds_is_included;
  bms_included=bms_is_included;
  clearGrids();
}

void GRIBMessage::printHeader(std::ostream& outs,bool verbose) const
{
  int n;

  if (verbose) {
    outs << "  GRIB Ed: " << edition << "  Length: " << mlength << std::endl;
    if (lengths.pds_supp > 0) {
	outs << "\n  Supplement to the PDS:";
	for (n=0; n < lengths.pds_supp; ++n) {
	  if (pds_supp[n] < 32 || pds_supp[n] > 127) {
	    outs << " \\" << std::setw(3) << std::setfill('0') << std::oct << static_cast<int>(pds_supp[n]) << std::setfill(' ') << std::dec;
	  }
	  else {
	    outs << " " << pds_supp[n];
	  }
	}
    }
    (reinterpret_cast<GRIBGrid *>(grids.front()))->printHeader(outs,verbose);
  }
  else {
    outs << " Ed=" << edition;
    (reinterpret_cast<GRIBGrid *>(grids.front()))->printHeader(outs,verbose);
  }
}

void GRIB2Message::unpackIDS(const unsigned char *input_buffer)
{
  off_t off=curr_off*8;
  short sec_num;
  short yr,mo,dy,hr,min,sec;
  GRIB2Grid *g2=reinterpret_cast<GRIB2Grid *>(grids.back());

  offsets.ids=curr_off;
  getBits(input_buffer,lengths.ids,off,32);
  getBits(input_buffer,sec_num,off+32,8);
  if (sec_num != 1) {
    mywarning="may not be unpacking the Identification Section";
  }
  getBits(input_buffer,g2->grid.src,off+40,16);
  getBits(input_buffer,g2->grib.sub_center,off+56,16);
  getBits(input_buffer,g2->grib.table,off+72,8);
  getBits(input_buffer,g2->grib2.local_table,off+80,8);
  getBits(input_buffer,g2->grib2.time_sig,off+88,8);
  getBits(input_buffer,yr,off+96,16);
  getBits(input_buffer,mo,off+112,8);
  getBits(input_buffer,dy,off+120,8);
  getBits(input_buffer,hr,off+128,8);
  getBits(input_buffer,min,off+136,8);
  getBits(input_buffer,sec,off+144,8);
  g2->referenceDateTime.set(yr,mo,dy,hr*10000+min*100+sec);
  getBits(input_buffer,g2->grib2.prod_status,off+152,8);
  getBits(input_buffer,g2->grib2.data_type,off+160,8);
  curr_off+=lengths.ids;
}

void GRIB2Message::packIDS(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  short hr,min,sec;
  GRIB2Grid *g2;

  g2=reinterpret_cast<GRIB2Grid *>(grid);
setBits(output_buffer,21,off,32);
  setBits(output_buffer,1,off+32,8);
  setBits(output_buffer,g2->grid.src,off+40,16);
  setBits(output_buffer,g2->grib.sub_center,off+56,16);
  setBits(output_buffer,g2->grib.table,off+72,8);
  setBits(output_buffer,g2->grib2.local_table,off+80,8);
  setBits(output_buffer,g2->grib2.time_sig,off+88,8);
  setBits(output_buffer,g2->referenceDateTime.getYear(),off+96,16);
  setBits(output_buffer,g2->referenceDateTime.getMonth(),off+112,8);
  setBits(output_buffer,g2->referenceDateTime.getDay(),off+120,8);
  hr=g2->referenceDateTime.getTime()/10000;
  min=(g2->referenceDateTime.getTime()/100 % 100);
  sec=(g2->referenceDateTime.getTime() % 100);
  setBits(output_buffer,hr,off+128,8);
  setBits(output_buffer,min,off+136,8);
  setBits(output_buffer,sec,off+144,8);
  setBits(output_buffer,g2->grib2.prod_status,off+152,8);
  setBits(output_buffer,g2->grib2.data_type,off+160,8);
offset+=21;
}

void GRIB2Message::unpackLUS(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;

  getBits(stream_buffer,lus_len,off,32);
  curr_off+=lus_len;
}

void GRIB2Message::packLUS(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
}

void GRIB2Message::unpackPDS(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short sec_num;
  unsigned char sign;
  short scale;
  int dum,noff,n,num_ranges;
  short yr,mo,dy,hr,min,sec;
  DateTime ddum;
  GRIB2Grid::StatisticalProcessRange stat_process_range;
  DateTime lastValidDateTime;
  GRIB2Grid *g2;

  offsets.pds=curr_off;
  g2=reinterpret_cast<GRIB2Grid *>(grids.back());
  getBits(stream_buffer,lengths.pds,off,32);
  getBits(stream_buffer,sec_num,off+32,8);
  if (sec_num != 4) {
    mywarning="may not be unpacking the Product Description Section";
  }
  getBits(stream_buffer,g2->grib2.num_coord_vals,off+40,16);
  g2->grid.fcst_time=0;
  g2->ensdata.fcst_type="";
  g2->grib2.stat_process_ranges.clear();
// template number
  getBits(stream_buffer,g2->grib2.product_type,off+56,16);
  switch (g2->grib2.product_type) {
    case 0:
    case 1:
    case 2:
    case 8:
    case 11:
    case 12:
    case 15:
    case 61:
	getBits(stream_buffer,g2->grib2.param_cat,off+72,8);
	getBits(stream_buffer,g2->grid.param,off+80,8);
	getBits(stream_buffer,g2->grib.process,off+88,8);
	getBits(stream_buffer,g2->grib2.backgen_process,off+96,8);
	getBits(stream_buffer,g2->grib2.fcstgen_process,off+104,8);
	getBits(stream_buffer,g2->grib.time_units,off+136,8);
	getBits(stream_buffer,g2->grid.fcst_time,off+144,32);
	getBits(stream_buffer,g2->grid.level1_type,off+176,8);
	getBits(stream_buffer,sign,off+192,1);
	getBits(stream_buffer,dum,off+193,31);
	if (dum != 0x7fffffff || sign != 1) {
	  if (sign == 1) {
	    dum=-dum;
	  }
	  getBits(stream_buffer,sign,off+184,1);
	  getBits(stream_buffer,scale,off+185,7);
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
	  g2->grid.level1=Grid::missingValue;
	}
	getBits(stream_buffer,g2->grid.level2_type,off+224,8);
	if (g2->grid.level2_type != 255) {
	  getBits(stream_buffer,sign,off+240,1);
	  getBits(stream_buffer,dum,off+241,31);
	  if (dum != 0x7fffffff || sign != 1) {
	    if (sign == 1) {
		dum=-dum;
	    }
	    getBits(stream_buffer,sign,off+232,1);
	    getBits(stream_buffer,scale,off+233,7);
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
	    g2->grid.level2=Grid::missingValue;
	  }
	}
	else {
	  g2->grid.level2=Grid::missingValue;
	}
	switch (g2->grib2.product_type) {
	  case 1:
	  case 11:
	  case 61:
	    getBits(stream_buffer,dum,off+272,8);
	    switch (dum) {
		case 0:
		  g2->ensdata.fcst_type="HRCTL";
		  break;
		case 1:
		  g2->ensdata.fcst_type="LRCTL";
		  break;
		case 2:
		  g2->ensdata.fcst_type="NEG";
		  break;
		case 3:
		  g2->ensdata.fcst_type="POS";
		  break;
		case 255:
		  g2->ensdata.fcst_type="UNK";
		  break;
		default:
		  myerror="no ensemble type mapping for "+strutils::itos(dum);
		  exit(1);
	    }
	    getBits(stream_buffer,dum,off+280,8);
	    g2->ensdata.ID=strutils::itos(dum);
	    getBits(stream_buffer,g2->ensdata.total_size,off+288,8);
	    switch (g2->grib2.product_type) {
		case 61:
		  getBits(stream_buffer,yr,off+296,16);
		  getBits(stream_buffer,mo,off+312,8);
		  getBits(stream_buffer,dy,off+320,8);
		  getBits(stream_buffer,hr,off+328,8);
		  getBits(stream_buffer,min,off+336,8);
		  getBits(stream_buffer,sec,off+344,8);
		  g2->grib2.modelv_date.set(yr,mo,dy,hr*10000+min*100+sec);
		  off+=56;
		case 11:
		  getBits(stream_buffer,yr,off+296,16);
		  getBits(stream_buffer,mo,off+312,8);
		  getBits(stream_buffer,dy,off+320,8);
		  getBits(stream_buffer,hr,off+328,8);
		  getBits(stream_buffer,min,off+336,8);
		  getBits(stream_buffer,sec,off+344,8);
		  g2->grib2.stat_period_end.set(yr,mo,dy,hr*10000+min*100+sec);
getBits(stream_buffer,dum,off+352,8);
if (dum > 1) {
myerror="error processing multiple ("+strutils::itos(dum)+") statistical processes for product type "+strutils::itos(g2->grib2.product_type);
exit(1);
}
		  getBits(stream_buffer,g2->grib2.stat_process_nmissing,off+360,32);
		  getBits(stream_buffer,stat_process_range.type,off+392,8);
		  getBits(stream_buffer,stat_process_range.time_increment_type,off+400,8);
		  getBits(stream_buffer,stat_process_range.period_length.unit,off+408,8);
		  getBits(stream_buffer,stat_process_range.period_length.value,off+416,32);
		  getBits(stream_buffer,stat_process_range.period_time_increment.unit,off+448,8);
		  getBits(stream_buffer,stat_process_range.period_time_increment.value,off+456,32);
		  g2->grib2.stat_process_ranges.push_back(stat_process_range);
		  break;
	    }
	    break;
	  case 2:
	  case 12:
	    getBits(stream_buffer,dum,off+272,8);
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
	    g2->ensdata.ID="ALL";
	    getBits(stream_buffer,g2->ensdata.total_size,off+280,8);
	    switch (g2->grib2.product_type) {
		case 12:
		  getBits(stream_buffer,yr,off+288,16);
		  getBits(stream_buffer,mo,off+304,8);
		  getBits(stream_buffer,dy,off+312,8);
		  getBits(stream_buffer,hr,off+320,8);
		  getBits(stream_buffer,min,off+328,8);
		  getBits(stream_buffer,sec,off+336,8);
		  g2->grib2.stat_period_end.set(yr,mo,dy,hr*10000+min*100+sec);
getBits(stream_buffer,dum,off+344,8);
if (dum > 1) {
myerror="error processing multiple ("+strutils::itos(dum)+") statistical processes for product type "+strutils::itos(g2->grib2.product_type);
exit(1);
}
		  getBits(stream_buffer,g2->grib2.stat_process_nmissing,off+352,32);
		  getBits(stream_buffer,stat_process_range.type,off+384,8);
		  getBits(stream_buffer,stat_process_range.time_increment_type,off+392,8);
		  getBits(stream_buffer,stat_process_range.period_length.unit,off+400,8);
		  getBits(stream_buffer,stat_process_range.period_length.value,off+408,32);
		  getBits(stream_buffer,stat_process_range.period_time_increment.unit,off+440,8);
		  getBits(stream_buffer,stat_process_range.period_time_increment.value,off+448,32);
		  g2->grib2.stat_process_ranges.push_back(stat_process_range);
		  break;
	    }
	    break;
	  case 8:
	  {
	    getBits(stream_buffer,yr,off+272,16);
	    getBits(stream_buffer,mo,off+288,8);
	    getBits(stream_buffer,dy,off+296,8);
	    getBits(stream_buffer,hr,off+304,8);
	    getBits(stream_buffer,min,off+312,8);
	    getBits(stream_buffer,sec,off+320,8);
	    g2->grib2.stat_period_end.set(yr,mo,dy,hr*10000+min*100+sec);
	    getBits(stream_buffer,num_ranges,off+328,8);
// patch
	    if (num_ranges == 0) {
		num_ranges=(lengths.pds-46)/12;
	    }
	    getBits(stream_buffer,g2->grib2.stat_process_nmissing,off+336,32);
/*
if (num_ranges > 1) {
myerror="error processing multiple ("+strutils::itos(num_ranges)+") statistical processes for product type "+strutils::itos(g2->grib2.product_type);
exit(1);
}
*/
	    noff=368;
	    for (n=0; n < num_ranges; n++) {
		getBits(stream_buffer,stat_process_range.type,off+noff,8);
		getBits(stream_buffer,stat_process_range.time_increment_type,off+noff+8,8);
		getBits(stream_buffer,stat_process_range.period_length.unit,off+noff+16,8);
		getBits(stream_buffer,stat_process_range.period_length.value,off+noff+24,32);
		getBits(stream_buffer,stat_process_range.period_time_increment.unit,off+noff+56,8);
		getBits(stream_buffer,stat_process_range.period_time_increment.value,off+noff+64,32);
		g2->grib2.stat_process_ranges.push_back(stat_process_range);
		noff+=96;
	    }
	    break;
	  }
	  case 15:
	    getBits(stream_buffer,g2->grib2.spatial_process.stat_process,off+272,8);
	    getBits(stream_buffer,g2->grib2.spatial_process.type,off+280,8);
	    getBits(stream_buffer,g2->grib2.spatial_process.num_points,off+288,8);
	    break;
	}
	break;
    default:
	myerror="product template "+strutils::itos(g2->grib2.product_type)+" not recognized";
	exit(1);
  }
  switch (g2->grib.time_units) {
case 0:
break;
    case 1:
	break;
    case 3:
	ddum=g2->referenceDateTime.monthsAdded(g2->grid.fcst_time);
	g2->grid.fcst_time=ddum.getHoursSince(g2->referenceDateTime);
	break;
    case 11:
	g2->grid.fcst_time*=6;
	break;
    case 12:
	g2->grid.fcst_time*=12;
	break;
    default:
	myerror="unable to adjust valid time for time units "+strutils::itos(g2->grib.time_units);
	exit(1);
  }
/*
  if (g2->grid.src == 7 && g2->grib2.stat_process_ranges.size() > 1 && g2->grib2.stat_process_ranges[0].type == 255 && g2->grib2.stat_process_ranges[1].type != 255) {
    g2->grib2.stat_process_ranges[0].type=g2->grib2.stat_process_ranges[1].type;
  }
*/
/*
  if (g2->grib2.stat_process_ranges.size() > 0) {
    if (((g2->grib2.stat_process_ranges[0].type >= 0 && g2->grib2.stat_process_ranges[0].type <= 4) || g2->grib2.stat_process_ranges[0].type == 8 || (g2->grib2.stat_process_ranges[0].type == 255 && g2->grib2.stat_period_end.getYear() > 0))) {
//if (((g2->grib2.stat_process_ranges[0].type >= 0 && g2->grib2.stat_process_ranges[0].type <= 4) || g2->grib2.stat_process_ranges[0].type == 8)) {
	g2->validDateTime=g2->referenceDateTime.hoursAdded(g2->grid.fcst_time);
    }
    else if (g2->grib2.stat_process_ranges[0].type > 191) {
	g2->validDateTime=g2->referenceDateTime.hoursAdded(g2->grid.fcst_time);
	lastValidDateTime=g2->validDateTime;
	gributils::getGRIB2ProductDescription(g2,g2->validDateTime,lastValidDateTime);
	if (lastValidDateTime != g2->validDateTime) {
	  g2->grib2.stat_period_end=lastValidDateTime;
	}
    }
  }
  else {
    g2->validDateTime=g2->referenceDateTime.hoursAdded(g2->grid.fcst_time);
  }
*/
if (g2->grib.time_units == 0) {
g2->forecastDateTime=g2->validDateTime=g2->referenceDateTime.minutesAdded(g2->grid.fcst_time);
g2->grid.fcst_time*=100;
}
else {
  g2->forecastDateTime=g2->validDateTime=g2->referenceDateTime.hoursAdded(g2->grid.fcst_time);
  g2->grid.fcst_time*=10000;
}
  g2->grib.prod_descr=gributils::getGRIB2ProductDescription(g2,g2->forecastDateTime,g2->validDateTime);
  curr_off+=lengths.pds;
}

void GRIB2Message::packPDS(unsigned char *output_buffer,off_t& offset,Grid *grid) const
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
  setBits(output_buffer,4,off+32,8);
setBits(output_buffer,0,off+40,16);
  fcst_time=g2->grid.fcst_time/10000;
// template number
  setBits(output_buffer,g2->grib2.product_type,off+56,16);
  switch (g2->grib2.product_type) {
    case 0:
    case 1:
    case 2:
    case 8:
    case 11:
    case 12:
	setBits(output_buffer,g2->grib2.param_cat,off+72,8);
	setBits(output_buffer,g2->grid.param,off+80,8);
	setBits(output_buffer,g2->grib.process,off+88,8);
	setBits(output_buffer,g2->grib2.backgen_process,off+96,8);
	setBits(output_buffer,g2->grib2.fcstgen_process,off+104,8);
	setBits(output_buffer,g2->grib.time_units,off+136,8);
	setBits(output_buffer,fcst_time,off+144,32);
	setBits(output_buffer,g2->grid.level1_type,off+176,8);
	if (g2->grid.level1 == Grid::missingValue) {
	  setBits(output_buffer,0xff,off+184,8);
	  setBits(output_buffer,static_cast<size_t>(0xffffffff),off+192,32);
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
	  if (myequalf(lval,static_cast<long long>(lval),0.00000000001)) {
	    ldum=lval;
	    while (ldum > 0 && (ldum % 10) == 0) {
		scale++;
		ldum/=10;
	    }
	    setBits(output_buffer,ldum,off+193,31);
	  }
	  else {
	    lval*=10.;
	    scale--;
	    while (!myequalf(lval,llround(lval),0.00000000001)) {
		scale--;
		lval*=10.;
	    }
	    setBits(output_buffer,llround(lval),off+193,31);
	  }
	  if (scale > 0)
	    setBits(output_buffer,1,off+184,1);
	  else {
	    setBits(output_buffer,0,off+184,1);
	    scale=-scale;
	  }
	  setBits(output_buffer,scale,off+185,7);
	  setBits(output_buffer,sign,off+192,1);
	}
	setBits(output_buffer,g2->grid.level2_type,off+224,8);
	if (g2->grid.level2 == Grid::missingValue) {
	  setBits(output_buffer,0xff,off+232,8);
	  setBits(output_buffer,static_cast<size_t>(0xffffffff),off+240,32);
	}
	else {
	  lval=g2->grid.level2;
	  if (lval < 0.)
	    sign=1;
	  else
	    sign=0;
	  if (g2->grid.level2_type == 100 || g2->grid.level2_type == 108)
	    lval*=100.;
	  else if (g2->grid.level2_type == 109)
	    lval/=1000000000.;
	  scale=0;
	  if (myequalf(lval,static_cast<long long>(lval),0.00000000001)) {
	    ldum=lval;
	    while (ldum > 0 && (ldum % 10) == 0) {
		scale++;
		ldum/=10;
	    }
	    setBits(output_buffer,ldum,off+241,31);
	  }
	  else {
	    lval*=10.;
	    scale--;
	    while (!myequalf(lval,llround(lval),0.00000000001)) {
		scale--;
		lval*=10.;
	    }
	    setBits(output_buffer,llround(lval),off+241,31);
	  }
	  if (scale > 0)
	    setBits(output_buffer,1,off+232,1);
	  else {
	    setBits(output_buffer,0,off+232,1);
	    scale=-scale;
	  }
	  setBits(output_buffer,scale,off+233,7);
	  setBits(output_buffer,sign,off+240,1);
	}
	switch (g2->grib2.product_type) {
	  case 0:
	    setBits(output_buffer,34,off,32);
	    offset+=34;
	    break;
	  case 1:
	  case 11:
std::cerr << "unable to finish packing PDS for product type " << g2->grib2.product_type << std::endl;
	    break;
	  case 2:
	  case 12:
std::cerr << "unable to finish packing PDS for product type " << g2->grib2.product_type << std::endl;
	    break;
	  case 8:
	    setBits(output_buffer,g2->grib2.stat_period_end.getYear(),off+272,16);
	    setBits(output_buffer,g2->grib2.stat_period_end.getMonth(),off+288,8);
	    setBits(output_buffer,g2->grib2.stat_period_end.getDay(),off+296,8);
	    hr=g2->grib2.stat_period_end.getTime()/10000;
	    min=(g2->grib2.stat_period_end.getTime()/100 % 100);
	    sec=(g2->grib2.stat_period_end.getTime() % 100);
	    setBits(output_buffer,hr,off+304,8);
	    setBits(output_buffer,min,off+312,8);
	    setBits(output_buffer,sec,off+320,8);
	    setBits(output_buffer,g2->grib2.stat_process_ranges.size(),off+328,8);
	    setBits(output_buffer,g2->grib2.stat_process_nmissing,off+336,32);
	    noff=368;
	    for (n=0; n < static_cast<int>(g2->grib2.stat_process_ranges.size()); n++) {
		setBits(output_buffer,g2->grib2.stat_process_ranges[n].type,off+noff,8);
		setBits(output_buffer,g2->grib2.stat_process_ranges[n].time_increment_type,off+noff+8,8);
		setBits(output_buffer,g2->grib2.stat_process_ranges[n].period_length.unit,off+noff+16,8);
		setBits(output_buffer,g2->grib2.stat_process_ranges[n].period_length.value,off+noff+24,32);
		setBits(output_buffer,g2->grib2.stat_process_ranges[n].period_time_increment.unit,off+noff+56,8);
		setBits(output_buffer,g2->grib2.stat_process_ranges[n].period_time_increment.value,off+noff+64,32);
		noff+=96;
	    }
	    setBits(output_buffer,noff/8,off,32);
	    offset+=noff/8;
	    break;
	}
	break;
    }
}

void GRIB2Message::unpackGDS(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short sec_num;
  int dum;
  unsigned char sign;
  size_t sdum;
  GRIB2Grid *g2;

  offsets.gds=curr_off;
  g2=reinterpret_cast<GRIB2Grid *>(grids.back());
  getBits(stream_buffer,lengths.gds,off,32);
  getBits(stream_buffer,sec_num,off+32,8);
  if (sec_num != 3) {
    mywarning="may not be unpacking the Grid Description Section";
  }
  getBits(stream_buffer,dum,off+40,8);
if (dum != 0) {
myerror="grid has no template";
exit(1);
}
  getBits(stream_buffer,g2->dim.size,off+48,32);
  getBits(stream_buffer,g2->grib2.lp_width,off+80,8);
  if (g2->grib2.lp_width != 0)
    getBits(stream_buffer,g2->grib2.lpi,off+88,8);
  else
    g2->grib2.lpi=0;
// template number
  getBits(stream_buffer,g2->grid.grid_type,off+96,16);
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
	getBits(stream_buffer,g2->grib2.earth_shape,off+112,8);
	getBits(stream_buffer,g2->dim.x,off+256,16);
	getBits(stream_buffer,g2->dim.y,off+288,16);
	getBits(stream_buffer,sdum,off+369,31);
	g2->def.slatitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+368,1);
	if (sign == 1) {
	  g2->def.slatitude=-(g2->def.slatitude);
	}
	getBits(stream_buffer,sdum,off+401,31);
	g2->def.slongitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+400,1);
	if (sign == 1) {
	  g2->def.slongitude=-(g2->def.slongitude);
	}
	getBits(stream_buffer,g2->grib.rescomp,off+432,8);
	getBits(stream_buffer,sdum,off+441,31);
	g2->def.elatitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+440,1);
	if (sign == 1) {
	  g2->def.elatitude=-(g2->def.elatitude);
	}
	getBits(stream_buffer,sdum,off+473,31);
	g2->def.elongitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+472,1);
	if (sign == 1) {
	  g2->def.elongitude=-(g2->def.elongitude);
	}
	getBits(stream_buffer,sdum,off+504,32);
	g2->def.loincrement=sdum/1000000.;
	getBits(stream_buffer,g2->def.num_circles,off+536,32);
	getBits(stream_buffer,g2->grib.scan_mode,off+568,8);
	if (g2->grid.grid_type == 0) {
	  g2->def.laincrement=g2->def.num_circles/1000000.;
	}
	off=576;
	break;
    }
    case 10:
    {
	getBits(stream_buffer,g2->grib2.earth_shape,off+112,8);
	getBits(stream_buffer,g2->dim.x,off+256,16);
	getBits(stream_buffer,g2->dim.y,off+288,16);
	getBits(stream_buffer,sdum,off+305,31);
	g2->def.slatitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+304,1);
	if (sign == 1) {
	  g2->def.slatitude=-(g2->def.slatitude);
	}
	getBits(stream_buffer,sdum,off+337,31);
	g2->def.slongitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+336,1);
	if (sign == 1) {
	  g2->def.slongitude=-(g2->def.slongitude);
	}
	getBits(stream_buffer,g2->grib.rescomp,off+368,8);
	getBits(stream_buffer,sdum,off+377,31);
	g2->def.stdparallel1=sdum/1000000.;
	getBits(stream_buffer,sign,off+376,1);
	if (sign == 1) {
	  g2->def.stdparallel1=-(g2->def.stdparallel1);
	}
	getBits(stream_buffer,sdum,off+409,31);
	g2->def.elatitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+408,1);
	if (sign == 1) {
	  g2->def.elatitude=-(g2->def.elatitude);
	}
	getBits(stream_buffer,sdum,off+441,31);
	g2->def.elongitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+440,1);
	if (sign == 1) {
	  g2->def.elongitude=-(g2->def.elongitude);
	}
	getBits(stream_buffer,g2->grib.scan_mode,off+472,8);
	getBits(stream_buffer,sdum,off+512,32);
	g2->def.dx=sdum/1000000.;
	getBits(stream_buffer,sdum,off+544,32);
	g2->def.dy=sdum/1000000.;
	break;
    }
    case 30:
    {
	getBits(stream_buffer,g2->grib2.earth_shape,off+112,8);
	getBits(stream_buffer,g2->dim.x,off+256,16);
	getBits(stream_buffer,g2->dim.y,off+288,16);
	getBits(stream_buffer,sdum,off+305,31);
	g2->def.slatitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+304,1);
	if (sign == 1) {
	  g2->def.slatitude=-(g2->def.slatitude);
	}
	getBits(stream_buffer,sdum,off+337,31);
	g2->def.slongitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+336,1);
	if (sign == 1) {
	  g2->def.slongitude=-(g2->def.slongitude);
	}
	getBits(stream_buffer,g2->grib.rescomp,off+368,8);
	getBits(stream_buffer,sdum,off+377,31);
	g2->def.llatitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+376,1);
	if (sign == 1) {
	  g2->def.llatitude=-(g2->def.llatitude);
	}
	getBits(stream_buffer,sdum,off+409,31);
	g2->def.olongitude=sdum/1000000.;
	getBits(stream_buffer,sign,off+408,1);
	if (sign == 1) {
	  g2->def.olongitude=-(g2->def.olongitude);
	}
	getBits(stream_buffer,sdum,off+440,32);
	g2->def.dx=sdum/1000000.;
	getBits(stream_buffer,sdum,off+472,32);
	g2->def.dy=sdum/1000000.;
	getBits(stream_buffer,g2->def.projection_flag,off+504,8);
	getBits(stream_buffer,g2->grib.scan_mode,off+512,8);
	getBits(stream_buffer,sdum,off+521,31);
	g2->def.stdparallel1=sdum/1000000.;
	getBits(stream_buffer,sign,off+520,1);
	if (sign == 1) {
	  g2->def.stdparallel1=-(g2->def.stdparallel1);
	}
	getBits(stream_buffer,sdum,off+553,31);
	g2->def.stdparallel2=sdum/1000000.;
	getBits(stream_buffer,sign,off+552,1);
	if (sign == 1) {
	  g2->def.stdparallel2=-(g2->def.stdparallel2);
	}
	getBits(stream_buffer,sdum,off+585,31);
	g2->grib.sp_lat=sdum/1000000.;
	getBits(stream_buffer,sign,off+584,1);
	if (sign == 1) {
	  g2->grib.sp_lat=-(g2->grib.sp_lat);
	}
	getBits(stream_buffer,sdum,off+617,31);
	g2->grib.sp_lon=sdum/1000000.;
	getBits(stream_buffer,sign,off+616,1);
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
    if (g2->plist != NULL)
	delete[] g2->plist;
    g2->plist=new int[g2->dim.y];
    getBits(stream_buffer,g2->plist,off,g2->grib2.lp_width*8,0,g2->dim.y);
    g2->def.loincrement=(g2->def.elongitude-g2->def.slongitude)/(g2->plist[g2->dim.y/2]-1);
  }
  g2->def=fixGridDefinition(g2->def,g2->dim);
  curr_off+=lengths.gds;
}

void GRIB2Message::packGDS(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  long long sdum;
  GRIB2Grid *g2;

  g2=reinterpret_cast<GRIB2Grid *>(grid);
  setBits(output_buffer,3,off+32,8);
setBits(output_buffer,0,off+40,8);
  setBits(output_buffer,g2->dim.size,off+48,32);
  setBits(output_buffer,g2->grib2.lp_width,off+80,8);
  setBits(output_buffer,g2->grib2.lpi,off+88,8);
// template number
  setBits(output_buffer,g2->grid.grid_type,off+96,16);
  switch (g2->grid.grid_type) {
    case 0:
    case 40:
	setBits(output_buffer,g2->grib2.earth_shape,off+112,8);
	setBits(output_buffer,0,off+240,16);
	setBits(output_buffer,g2->dim.x,off+256,16);
	setBits(output_buffer,0,off+272,16);
	setBits(output_buffer,g2->dim.y,off+288,16);
setBits(output_buffer,0,off+304,32);
setBits(output_buffer,0xffffffff,off+336,32);
	sdum=g2->def.slatitude*1000000.;
	if (sdum < 0)
	  sdum=-sdum;
	setBits(output_buffer,sdum,off+369,31);
	if (g2->def.slatitude < 0.)
	  setBits(output_buffer,1,off+368,1);
	else
	  setBits(output_buffer,0,off+368,1);
	sdum=g2->def.slongitude*1000000.;
	if (sdum < 0)
	  sdum+=360000000;
	setBits(output_buffer,sdum,off+400,32);
	setBits(output_buffer,g2->grib.rescomp,off+432,8);
	sdum=g2->def.elatitude*1000000.;
	if (sdum < 0)
	  sdum=-sdum;
	setBits(output_buffer,sdum,off+441,31);
	if (g2->def.elatitude < 0.)
	  setBits(output_buffer,1,off+440,1);
	else
	  setBits(output_buffer,0,off+440,1);
	sdum=g2->def.elongitude*1000000.;
	if (sdum < 0)
	  sdum=-sdum;
	setBits(output_buffer,sdum,off+473,31);
	if (g2->def.elongitude < 0.)
	  setBits(output_buffer,1,off+472,1);
	else
	  setBits(output_buffer,0,off+472,1);
//	sdum=(long long)(g2->def.loincrement*1000000.);
sdum=llround(g2->def.loincrement*1000000.);
	setBits(output_buffer,sdum,off+504,32);
	if (g2->grid.grid_type == 0)
//	  sdum=(long long)(g2->def.laincrement*1000000.);
sdum=llround(g2->def.laincrement*1000000.);
	else
	  sdum=g2->def.num_circles;
	setBits(output_buffer,sdum,off+536,32);
	setBits(output_buffer,g2->grib.scan_mode,off+568,8);
	setBits(output_buffer,72,off,32);
	offset+=72;
	break;
    default:
	myerror="unable to pack GDS for template "+strutils::itos(g2->grid.grid_type);
	exit(1);
  }
}

void GRIB2Message::unpackDRS(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short sec_num;
  union {
    float dum;
    int idum;
  };
  GRIB2Grid *g2;
  int num_packed;

  offsets.drs=curr_off;
  g2=reinterpret_cast<GRIB2Grid *>(grids.back());
  getBits(stream_buffer,lengths.drs,off,32);
  getBits(stream_buffer,sec_num,off+32,8);
  if (sec_num != 5) {
    mywarning="may not be unpacking the Data Representation Section";
  }
  getBits(stream_buffer,num_packed,off+40,32);
  g2->grid.num_missing=g2->dim.size-num_packed;
// template number
  getBits(stream_buffer,g2->grib2.data_rep,off+72,16);
  switch (g2->grib2.data_rep) {
    case 0:
    case 2:
    case 3:
    case 40:
    case 40000:
	getBits(stream_buffer,idum,off+88,32);
	getBits(stream_buffer,g2->grib.E,off+120,16);
	if (g2->grib.E > 0x8000)
	  g2->grib.E=0x8000-g2->grib.E;
	getBits(stream_buffer,g2->grib.D,off+136,16);
	if (g2->grib.D > 0x8000)
	  g2->grib.D=0x8000-g2->grib.D;
	g2->stats.min_val=dum/pow(10.,g2->grib.D);
	getBits(stream_buffer,g2->grib.pack_width,off+152,8);
	getBits(stream_buffer,g2->grib2.orig_val_type,off+160,8);
	if (g2->grib2.data_rep == 2 || g2->grib2.data_rep == 3) {
	  getBits(stream_buffer,g2->grib2.complex_pack.grid_point.split_method,off+168,8);
	  getBits(stream_buffer,g2->grib2.complex_pack.grid_point.miss_val_mgmt,off+176,8);
	  if (g2->grib2.orig_val_type == 0) {
	    getBits(stream_buffer,idum,off+184,32);
	    g2->grib2.complex_pack.grid_point.primary_miss_sub=dum;
	    getBits(stream_buffer,idum,off+216,32);
	    g2->grib2.complex_pack.grid_point.secondary_miss_sub=dum;
	  }
	  else if (g2->grib2.orig_val_type == 1) {
	    getBits(stream_buffer,idum,off+184,32);
	    g2->grib2.complex_pack.grid_point.primary_miss_sub=idum;
	    getBits(stream_buffer,idum,off+216,32);
	    g2->grib2.complex_pack.grid_point.secondary_miss_sub=idum;
	  }
	  else {
	    myerror="unable to decode missing value substitutes for original value type of "+strutils::itos(g2->grib2.orig_val_type);
	    exit(1);
	  }
	  getBits(stream_buffer,g2->grib2.complex_pack.grid_point.num_groups,off+248,32);
	  getBits(stream_buffer,g2->grib2.complex_pack.grid_point.width_ref,off+280,8);
	  getBits(stream_buffer,g2->grib2.complex_pack.grid_point.width_pack_width,off+288,8);
	  getBits(stream_buffer,g2->grib2.complex_pack.grid_point.length_ref,off+296,32);
	  getBits(stream_buffer,g2->grib2.complex_pack.grid_point.length_incr,off+328,8);
	  getBits(stream_buffer,g2->grib2.complex_pack.grid_point.last_length,off+336,32);
	  getBits(stream_buffer,g2->grib2.complex_pack.grid_point.length_pack_width,off+368,8);
	  if (g2->grib2.data_rep == 3) {
	    getBits(stream_buffer,g2->grib2.complex_pack.grid_point.spatial_diff.order,off+376,8);
	    getBits(stream_buffer,g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width,off+384,8);
	  }
	}
	break;
    default:
	myerror="data template "+strutils::itos(g2->grib2.data_rep)+" is not understood";
	exit(1);
  }
  curr_off+=lengths.drs;
}

void GRIB2Message::packDRS(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  union {
    float dum;
    int idum;
  };
  GRIB2Grid *g2=reinterpret_cast<GRIB2Grid *>(grid);

  setBits(output_buffer,5,off+32,8);
  setBits(output_buffer,g2->dim.size-(g2->grid.num_missing),off+40,32);
// template number
  setBits(output_buffer,g2->grib2.data_rep,off+72,16);
  switch (g2->grib2.data_rep) {
    case 0:
    case 2:
    case 3:
    case 40:
    case 40000:
	dum=lroundf(g2->stats.min_val*pow(10.,g2->grib.D));
	setBits(output_buffer,idum,off+88,32);
	idum=g2->grib.E;
	if (idum < 0)
	  idum=0x8000-idum;
	setBits(output_buffer,idum,off+120,16);
	idum=g2->grib.D;
	if (idum < 0)
	  idum=0x8000-idum;
	setBits(output_buffer,idum,off+136,16);
	setBits(output_buffer,g2->grib.pack_width,off+152,8);
	setBits(output_buffer,g2->grib2.orig_val_type,off+160,8);
	switch (g2->grib2.data_rep) {
	  case 0:
	    setBits(output_buffer,21,off,32);
	    offset+=21;
	    break;
	  case 2:
	  case 3:
	    setBits(output_buffer,g2->grib2.complex_pack.grid_point.split_method,off+168,8);
	    setBits(output_buffer,g2->grib2.complex_pack.grid_point.miss_val_mgmt,off+176,8);
	    setBits(output_buffer,23,off,32);
	    offset+=23;
	    break;
	  case 40:
	    setBits(output_buffer,0,off+168,8);
	    setBits(output_buffer,0xff,off+176,8);
	    setBits(output_buffer,23,off,32);
	    offset+=23;
	    break;
	}
	break;
    default:
	myerror="unable to pack data template "+strutils::itos(g2->grib2.data_rep);
	exit(1);
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

void GRIB2Message::unpackDS(const unsigned char *stream_buffer,bool fill_header_only)
{
  off_t off=curr_off*8;
  short sec_num;
  size_t x,y,avg_cnt=0,pval=0;
  int n,m,l,len;
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
  double lastgp;
  double D=pow(10.,g2->grib.D);
  double E=pow(2.,g2->grib.E);

  offsets.ds=curr_off;
  getBits(stream_buffer,lengths.ds,off,32);
  getBits(stream_buffer,sec_num,off+32,8);
  if (sec_num != 7) {
    mywarning="may not be unpacking the Data Section";
  }
  if (!fill_header_only) {
    g2->stats.max_val=-Grid::missingValue;
    switch (g2->grib2.data_rep) {
	case 0:
	{
	  g2->galloc();
	  off+=40;
	  x=0;
	  for (n=0; n < g2->dim.y; n++) {
	    for (m=0; m < g2->dim.x; m++) {
		if (!g2->bitmap.applies || g2->bitmap.map[x] == 1) {
		  getBits(stream_buffer,pval,off,g2->grib.pack_width);
		  num_packed++;
		  g2->gridpoints[n][m]=g2->stats.min_val+pval*E/D;
		  if (g2->gridpoints[n][m] > g2->stats.max_val) {
		    g2->stats.max_val=g2->gridpoints[n][m];
		  }
		  g2->stats.avg_val+=g2->gridpoints[n][m];
		  avg_cnt++;
		  off+=g2->grib.pack_width;
		}
		else {
		  g2->gridpoints[n][m]=Grid::missingValue;
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
	    groups.miss_val=Grid::missingValue;
	  }
	  off+=40;
	  groups.ref_vals=new int[g2->grib2.complex_pack.grid_point.num_groups];
	  getBits(stream_buffer,groups.ref_vals,off,g2->grib.pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	  off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib.pack_width;
	  if ( (pad=(off % 8)) > 0) {
	    off+=8-pad;
	  }
	  groups.widths=new int[g2->grib2.complex_pack.grid_point.num_groups];
	  getBits(stream_buffer,groups.widths,off,g2->grib2.complex_pack.grid_point.width_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	  off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.width_pack_width;
	  if ( (pad=(off % 8)) > 0) {
	    off+=8-pad;
	  }
	  groups.lengths=new int[g2->grib2.complex_pack.grid_point.num_groups];
	  getBits(stream_buffer,groups.lengths,off,g2->grib2.complex_pack.grid_point.length_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	  off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.length_pack_width;
	  if ( (pad=(off % 8)) > 0) {
	    off+=8-pad;
	  }
	  groups.max_length=0;
	  for (n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
	    groups.lengths[n]=g2->grib2.complex_pack.grid_point.length_ref+groups.lengths[n]*g2->grib2.complex_pack.grid_point.length_incr;
	    if (groups.lengths[n] > groups.max_length) {
		groups.max_length=groups.lengths[n];
	    }
	  }
	  groups.pvals=new int[groups.max_length];
	  for (n=0,l=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
	    if (groups.widths[n] > 0) {
		if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
		  groups.group_miss_val=pow(2.,groups.widths[n])-1;
		}
		else {
		  groups.group_miss_val=Grid::missingValue;
		}
		getBits(stream_buffer,groups.pvals,off,groups.widths[n],0,groups.lengths[n]);
		off+=groups.widths[n]*groups.lengths[n];
		for (m=0; m < groups.lengths[n]; ++m) {
		  j=l/g2->dim.x;
		  i=(l % g2->dim.x);
		  if (groups.pvals[m] == groups.group_miss_val || (g2->bitmap.applies && g2->bitmap.map[l] == 0)) {
		    g2->gridpoints[j][i]=Grid::missingValue;
		  }
		  else {
		    g2->gridpoints[j][i]=g2->stats.min_val+(groups.pvals[m]+groups.ref_vals[n])*E/D;
		    if (g2->gridpoints[j][i] > g2->stats.max_val) {
			g2->stats.max_val=g2->gridpoints[j][i];
		    }
		    g2->stats.avg_val+=g2->gridpoints[j][i];
		    ++avg_cnt;
		  }
		  ++l;
		}
	    }
	    else {
// constant group
		for (m=0; m < groups.lengths[n]; ++m) {
		  j=l/g2->dim.x;
		  i=(l % g2->dim.x);
		  if (groups.ref_vals[n] == groups.miss_val || (g2->bitmap.applies && g2->bitmap.map[l] == 0)) {
		    g2->gridpoints[j][i]=Grid::missingValue;
		  }
		  else {
		    g2->gridpoints[j][i]=g2->stats.min_val+groups.ref_vals[n]*E/D;
		    if (g2->gridpoints[j][i] > g2->stats.max_val) {
			g2->stats.max_val=g2->gridpoints[j][i];
		    }
		    g2->stats.avg_val+=g2->gridpoints[j][i];
		    ++avg_cnt;
		  }
		  ++l;
		}
	    }
	  }
	  for (; l < static_cast<int>(g2->dim.size); ++l) {
	    j=l/g2->dim.x;
	    i=(l % g2->dim.x);
	    g2->gridpoints[j][i]=Grid::missingValue;
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
		groups.miss_val=Grid::missingValue;
	    }
	    off+=40;
	    groups.first_vals=new int[g2->grib2.complex_pack.grid_point.spatial_diff.order];
	    for (n=0; n < g2->grib2.complex_pack.grid_point.spatial_diff.order; ++n) {
		getBits(stream_buffer,groups.first_vals[n],off,g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8);
		off+=g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8;
	    }
	    getBits(stream_buffer,groups.sign,off,1);
	    getBits(stream_buffer,groups.omin,off+1,g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8-1);
	    if (groups.sign == 1) {
		groups.omin=-groups.omin;
	    }
	    off+=g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8;
	    groups.ref_vals=new int[g2->grib2.complex_pack.grid_point.num_groups];
	    getBits(stream_buffer,groups.ref_vals,off,g2->grib.pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	    off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib.pack_width;
	    if ( (pad=(off % 8)) > 0) {
		off+=8-pad;
	    }
	    groups.widths=new int[g2->grib2.complex_pack.grid_point.num_groups];
	    getBits(stream_buffer,groups.widths,off,g2->grib2.complex_pack.grid_point.width_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	    off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.width_pack_width;
	    if ( (pad=(off % 8)) > 0) {
		off+=8-pad;
	    }
	    groups.lengths=new int[g2->grib2.complex_pack.grid_point.num_groups];
	    getBits(stream_buffer,groups.lengths,off,g2->grib2.complex_pack.grid_point.length_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
	    off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.length_pack_width;
	    if ( (pad=(off % 8)) > 0) {
		off+=8-pad;
	    }
	    groups.max_length=0;
	    for (n=0,l=g2->grib2.complex_pack.grid_point.num_groups-1; n < l; ++n) {
		groups.lengths[n]=g2->grib2.complex_pack.grid_point.length_ref+groups.lengths[n]*g2->grib2.complex_pack.grid_point.length_incr;
		if (groups.lengths[n] > groups.max_length) {
		  groups.max_length=groups.lengths[n];
		}
	    }
	    groups.lengths[n]=g2->grib2.complex_pack.grid_point.last_length;
	    if (groups.lengths[n] > groups.max_length) {
		groups.max_length=groups.lengths[n];
	    }
	    groups.pvals=new int[groups.max_length];
// unpack the field of differences
	    for (n=0,l=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
		if (groups.widths[n] > 0) {
		  if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
		    groups.group_miss_val=pow(2.,groups.widths[n])-1;
		  }
		  else {
		    groups.group_miss_val=Grid::missingValue;
		  }
		  getBits(stream_buffer,groups.pvals,off,groups.widths[n],0,groups.lengths[n]);
		  off+=groups.widths[n]*groups.lengths[n];
		  for (m=0; m < groups.lengths[n]; ++m,++l) {
		    j=l/g2->dim.x;
		    i=(l % g2->dim.x);
		    if ((g2->bitmap.applies && g2->bitmap.map[l] == 0) || groups.pvals[m] == groups.group_miss_val) {
			g2->gridpoints[j][i]=Grid::missingValue;
		    }
		    else {
			g2->gridpoints[j][i]=groups.pvals[m]+groups.ref_vals[n]+groups.omin;
		    }
		  }
		}
		else {
// constant group
		  for (m=0; m < groups.lengths[n]; ++m,++l) {
		    j=l/g2->dim.x;
		    i=(l % g2->dim.x);
		    if ((g2->bitmap.applies && g2->bitmap.map[l] == 0) || groups.ref_vals[n] == groups.miss_val) {
			g2->gridpoints[j][i]=Grid::missingValue;
		    }
		    else {
			g2->gridpoints[j][i]=groups.ref_vals[n]+groups.omin;
		    }
		  }
		}
	    }
	    for (; l < static_cast<int>(g2->dim.size); ++l) {
		j=l/g2->dim.x;
		i=(l % g2->dim.x);
		g2->gridpoints[j][i]=Grid::missingValue;
	    }
	    for (n=g2->grib2.complex_pack.grid_point.spatial_diff.order-1; n > 0; --n) {
		lastgp=groups.first_vals[n]-groups.first_vals[n-1];
		for (l=0,m=0; l < static_cast<int>(g2->dim.size); ++l) {
		  j=l/g2->dim.x;
		  i=(l % g2->dim.x);
		  if (g2->gridpoints[j][i] != Grid::missingValue) {
		    if (m >= g2->grib2.complex_pack.grid_point.spatial_diff.order) {
			g2->gridpoints[j][i]+=lastgp;
			lastgp=g2->gridpoints[j][i];
		    }
		    ++m;
		  }
		}
	    }
	    for (l=0,m=0,lastgp=0; l < static_cast<int>(g2->dim.size); ++l) {
		j=l/g2->dim.x;
		i=(l % g2->dim.x);
		if (g2->gridpoints[j][i] != Grid::missingValue) {
		  if (m < g2->grib2.complex_pack.grid_point.spatial_diff.order) {
		    g2->gridpoints[j][i]=g2->stats.min_val+groups.first_vals[m]/D*E;
		    lastgp=g2->stats.min_val*D/E+groups.first_vals[m];
		  }
		  else {
		    lastgp+=g2->gridpoints[j][i];
		    g2->gridpoints[j][i]=lastgp*E/D;
		  }
		  if (g2->gridpoints[j][i] > g2->stats.max_val) {
		    g2->stats.max_val=g2->gridpoints[j][i];
		  }
		  g2->stats.avg_val+=g2->gridpoints[j][i];
		  ++avg_cnt;
		  ++m;
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
	    for (n=0; n < g2->dim.y; ++n) {
		for (m=0; m < g2->dim.x; ++m) {
		  g2->gridpoints[n][m]=Grid::missingValue;
		}
	    }
	    g2->stats.min_val=g2->stats.max_val=g2->stats.avg_val=Grid::missingValue;
	  }
	  g2->grid.filled=true;
	  break;
	}
#ifdef __JASPER
	case 40:
	case 40000:
	{
	  len=lengths.ds-5;
	  jvals=new int[g2->dim.size];
	  if (len > 0) {
	    if (dec_jpeg2000(reinterpret_cast<char *>(const_cast<unsigned char *>(&stream_buffer[curr_off+5])),len,jvals) < 0) {
		for (n=0; n < static_cast<int>(g2->dim.size); jvals[n++]=0);
	    }
	  }
	  else {
	    for (n=0; n < static_cast<int>(g2->dim.size); jvals[n++]=0);
	  }
	  g2->galloc();
	  x=y=0;
	  for (n=0; n < g2->dim.y; ++n) {
	    for (m=0; m < g2->dim.x; ++m) {
		if (!g2->bitmap.applies || g2->bitmap.map[x] == 1) {
		  g2->gridpoints[n][m]=g2->stats.min_val+jvals[y++]*E/D;
		  if (g2->gridpoints[n][m] > g2->stats.max_val) {
		    g2->stats.max_val=g2->gridpoints[n][m];
		  }
		  g2->stats.avg_val+=g2->gridpoints[n][m];
		  ++avg_cnt;
		}
		else {
		  g2->gridpoints[n][m]=Grid::missingValue;
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
  curr_off+=lengths.ds;
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

void GRIB2Message::packDS(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  int noff,jval,len;
  int n,m,x=0,y=0,cps,bps;
  unsigned char *cin;
  int pwidth,pheight,pnbits,ltype=0,ratio=0,retry=1,jpclen;
  GRIB2Grid *g2=reinterpret_cast<GRIB2Grid *>(grid);
  double d=pow(10.,g2->grib.D),e=pow(2.,g2->grib.E);
  int *pval,cnt=0;

  setBits(output_buffer,7,off+32,8);
  switch (g2->grib2.data_rep) {
    case 0:
    {
	pval=new int[g2->dim.size];
	for (n=0; n < g2->dim.y; n++) {
	  for (m=0; m < g2->dim.x; m++) {
	    if (!g2->bitmap.applies || g2->bitmap.map[x] == 1)
		pval[cnt++]=static_cast<int>(lround((g2->gridpoints[n][m]-g2->stats.min_val)*pow(10.,g2->grib.D))/pow(2.,g2->grib.E));
	  }
	}
	setBits(&output_buffer[off/8+5],pval,0,g2->grib.pack_width,0,cnt);
	len=(cnt*g2->grib.pack_width+7)/8;
	setBits(output_buffer,len+5,off,32);
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
		if (g2->gridpoints[n][m] == Grid::missingValue)
		  jval=0;
		else
		  jval=lround((lround(g2->gridpoints[n][m]*d)-lround(g2->stats.min_val*d))/e);
		setBits(cin,jval,y*bps,bps);
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
	setBits(output_buffer,len+5,off,32);
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

off_t GRIB2Message::copyToBuffer(unsigned char *output_buffer,const size_t buffer_length)
{
  off_t offset;

  packIS(output_buffer,offset,grids.front());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copyToBuffer";
    exit(1);
  }
  packIDS(output_buffer,offset,grids.front());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copyToBuffer";
    exit(1);
  }
  for (auto grid : grids) {
    packLUS(output_buffer,offset,grid);
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copyToBuffer";
	exit(1);
    }
    if (gds_included) {
	packGDS(output_buffer,offset,grid);
	if (offset > static_cast<off_t>(buffer_length)) {
	  myerror="buffer overflow in copyToBuffer";
	  exit(1);
	}
    }
    packPDS(output_buffer,offset,grid);
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copyToBuffer";
	exit(1);
    }
    packDRS(output_buffer,offset,grid);
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copyToBuffer";
	exit(1);
    }
    packBMS(output_buffer,offset,grid);
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copyToBuffer";
	exit(1);
    }
    packDS(output_buffer,offset,grid);
    if (offset > static_cast<off_t>(buffer_length)) {
	myerror="buffer overflow in copyToBuffer";
	exit(1);
    }
  }
  packEND(output_buffer,offset);
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copyToBuffer";
    exit(1);
  }
  mlength=offset;
  packLength(output_buffer);
  return mlength;
}

void GRIB2Message::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int test,len;
  short sec_num,last_sec_num=99;
  GRIB2Grid *g2;

  if (stream_buffer == NULL) {
    myerror="empty file stream";
    exit(1);
  }
  clearGrids();
  unpackIS(stream_buffer);
  if (edition != 2) {
    myerror="can't decode edition "+strutils::itos(edition);
    exit(1);
  }
  getBits(stream_buffer,test,curr_off*8,32);
  while (test != 0x37373737) {
// patch for bad END section
    if ( (curr_off+4) == mlength) {
	mywarning="bad END section repaired";
	test=0x37373737;
	break;
    }
    getBits(stream_buffer,len,curr_off*8,32);
    getBits(stream_buffer,sec_num,curr_off*8+32,8);
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
	    g2->quickCopy(*(reinterpret_cast<GRIB2Grid *>(grids.back())));
	  }
	  getBits(stream_buffer,discipline,48,8);
	  g2->grib2.discipline=discipline;
	  g2->grid.filled=false;
	  grids.push_back(g2);
	}
	last_sec_num=sec_num;
	switch (sec_num) {
	  case 1:
	  {
	    unpackIDS(stream_buffer);
	    break;
	  }
	  case 2:
	  {
	    unpackLUS(stream_buffer);
	    break;
	  }
	  case 3:
	  {
	    unpackGDS(stream_buffer);
	    break;
	  }
	  case 4:
	  {
	    unpackPDS(stream_buffer);
	    break;
	  }
	  case 5:
	  {
	    unpackDRS(stream_buffer);
	    break;
	  }
	  case 6:
	  {
	    unpackBMS(stream_buffer);
	    break;
	  }
	  case 7:
	  {
	    unpackDS(stream_buffer,fill_header_only);
	    break;
	  }
	}
    }
    getBits(stream_buffer,test,curr_off*8,32);
  }
}

void GRIB2Message::printHeader(std::ostream& outs,bool verbose) const
{
  int n=0;

  if (verbose) {
    outs << "  GRIB Ed: " << edition << "  Length: " << mlength << "  Number of Grids: " << grids.size() << std::endl;
    for (auto grid : grids) {
	outs << "  Grid #" << ++n << ":" << std::endl;
	(reinterpret_cast<GRIB2Grid *>(grid))->printHeader(outs,verbose);
    }
  }
  else {
    outs << " Ed=" << edition << " NGrds=" << grids.size();
    for (auto grid : grids) {
	outs << " Grd=" << ++n;
	(reinterpret_cast<GRIB2Grid *>(grid))->printHeader(outs,verbose);
    }
  }
}

void GRIB2Message::quickFill(const unsigned char *stream_buffer)
{
  int test,off;
  short sec_num,last_sec_num=99;
  GRIB2Grid *g2=NULL;
  union {
    float dum;
    int idum;
  };

  if (stream_buffer == NULL) {
    myerror="empty file stream";
    exit(1);
  }
  clearGrids();
  edition=2;
  curr_off=16;
  getBits(stream_buffer,test,curr_off*8,32);
  while (test != 0x37373737) {
    getBits(stream_buffer,sec_num,curr_off*8+32,8);
    if (sec_num < last_sec_num) {
	g2=new GRIB2Grid;
	if (grids.size() > 0) {
	  g2->quickCopy(*(reinterpret_cast<GRIB2Grid *>(grids.back())));
	}
	g2->grid.filled=false;
	grids.push_back(g2);
    }
    switch (sec_num) {
	case 1:
	  curr_off+=test;
	  break;
	case 2:
	  curr_off+=test;
	  break;
	case 3:
	  off=curr_off*8;
	  getBits(stream_buffer,g2->dim.size,off+48,32);
	  getBits(stream_buffer,g2->dim.x,off+256,16);
	  getBits(stream_buffer,g2->dim.y,off+288,16);
	  switch (g2->grid.grid_type) {
	    case 0:
	    case 1:
	    case 2:
	    case 3:
	    case 40:
	    case 41:
	    case 42:
	    case 43:
		getBits(stream_buffer,g2->grib.scan_mode,off+568,8);
		break;
	    case 10:
		getBits(stream_buffer,g2->grib.scan_mode,off+472,8);
		break;
	    case 20:
	    case 30:
	    case 31:
		getBits(stream_buffer,g2->grib.scan_mode,off+512,8);
		break;
	    default:
		std::cerr << "Error: quickFill not implemented for grid type " << g2->grid.grid_type << std::endl;
		exit(1);
	  }
	  curr_off+=test;
	  break;
	case 4:
	  off=curr_off*8;
	  getBits(stream_buffer,g2->grib2.param_cat,off+72,8);
	  getBits(stream_buffer,g2->grid.param,off+80,8);
	  curr_off+=test;
	  break;
	case 5:
	  unpackDRS(stream_buffer);
	  break;
	case 6:
unpackBMS(stream_buffer);
/*
	  off=curr_off*8;
	  int ind;
	  getBits(stream_buffer,ind,off+40,8);
	  if (ind == 0) {
	    getBits(stream_buffer,lengths.bms,off,32);
	    if ( (len=(lengths.bms-6)*8) > 0) {
		if (len > g2->bitmap.capacity) {
		  if (g2->bitmap.map != NULL) {
		    delete[] g2->bitmap.map;
		  }
		  g2->bitmap.capacity=len;
		  g2->bitmap.map=new unsigned char[g2->bitmap.capacity];
		}
		getBits(stream_buffer,g2->bitmap.map,off+48,1,0,len);
	    }
	    g2->bitmap.applies=true;
	  }
	  else if (ind != 255) {
	    std::cerr << "Error: bitmap not implemented for quickFill" << std::endl;
	    exit(1);
	  }
	  else {
	    g2->bitmap.applies=false;
	  }
	  curr_off+=test;
*/
	  break;
	case 7:
	  unpackDS(stream_buffer,false);
	  break;
    }
    last_sec_num=sec_num;
    getBits(stream_buffer,test,curr_off*8,32);
  }
}

GRIBGrid::~GRIBGrid()
{
  if (plist != NULL) {
    delete[] plist;
    plist=NULL;
  }
  if (bitmap.map != NULL) {
    delete[] bitmap.map;
    bitmap.map=NULL;
  }
  if (gridpoints != NULL) {
    for (size_t n=0; n < grib.capacity.y; ++n) {
	delete[] gridpoints[n];
    }
    delete[] gridpoints;
    gridpoints=NULL;
  }
  if (map.param != NULL) {
    delete[] map.param;
    map.param=NULL;
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
  referenceDateTime=source.referenceDateTime;
  validDateTime=source.validDateTime;
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
  if (gridpoints != nullptr && grib.capacity.points < source.grib.capacity.points) {
    for (n=0; n < grib.capacity.y; ++n) {
	delete[] gridpoints[n];
    }
    delete[] gridpoints;
    gridpoints=nullptr;
  }
  grib=source.grib;
  if (source.grid.filled) {
    galloc();
    for (n=0; n < static_cast<size_t>(source.dim.y); ++n) {
	for (m=0; m < static_cast<size_t>(source.dim.x); ++m) {
	  gridpoints[n][m]=source.gridpoints[n][m];
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

  if (gridpoints != nullptr && dim.size > grib.capacity.points) {
    for (n=0; n < grib.capacity.y; ++n) {
	delete[] gridpoints[n];
    }
    delete[] gridpoints;
    gridpoints=nullptr;
  }
  if (gridpoints == nullptr) {
    grib.capacity.y=dim.y;
    gridpoints=new double *[grib.capacity.y];
    if (dim.x == -1) {
	for (n=0; n < grib.capacity.y; ++n) {
	  gridpoints[n]=new double[plist[n]+1];
	  gridpoints[n][0]=plist[n];
	}
    }
    else {
	for (n=0; n < grib.capacity.y; ++n) {
	  gridpoints[n]=new double[dim.x];
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

void GRIBGrid::fillFromGRIBData(const GRIBData& source)
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
    if (!fillGaussianLatitudes(gaus_lats,def.num_circles,(grib.scan_mode&0x40) != 0x40)) {
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
	grid.level1=source.levels[0];
      grid.level2=0.;
      break;
    default:
	grid.level1=source.levels[0];
	grid.level2=source.levels[1];
  }
  referenceDateTime=source.referenceDateTime;
  validDateTime=source.validDateTime;
  grib.time_units=source.time_units;
  grib.t_range=source.time_range;
  grid.nmean=0;
  grib.nmean_missing=0;
  switch (grib.t_range) {
    case 4:
    case 10:
	grib.p1=source.p1;
	grib.p2=0;
      break;
    default:
	grib.p1=source.p1;
	grib.p2=source.p2;
  }
  grid.nmean=source.num_averaged;
  grib.nmean_missing=source.num_averaged_missing;
  grib.sub_center=source.sub_center;
  grib.D=source.D;
  grid.num_missing=0;
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != NULL) {
	delete[] bitmap.map;
	bitmap.map=NULL;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  bitmap.applies=false;
  stats.min_val=Grid::missingValue;
  stats.max_val=-Grid::missingValue;
  stats.avg_val=0.;
  grid.num_missing=0;
  galloc();
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	gridpoints[n][m]=source.gridpoints[n][m];
	if (myequalf(gridpoints[n][m],Grid::missingValue)) {
	  bitmap.map[cnt++]=0;
	  ++grid.num_missing;
	  bitmap.applies=true;
	}
	else {
	  bitmap.map[cnt++]=1;
	  if (gridpoints[n][m] > stats.max_val) {
	    stats.max_val=gridpoints[n][m];
	  }
	  if (gridpoints[n][m] < stats.min_val) {
	    stats.min_val=gridpoints[n][m];
	  }
	  stats.avg_val+=gridpoints[n][m];
	  ++avg_cnt;
	}
    }
  }
  stats.avg_val/=static_cast<double>(avg_cnt);
  grib.pack_width=source.pack_width;
  grid.filled=true;
}

float GRIBGrid::getLatitudeAt(int index,my::map<GLatEntry> *gaus_lats) const
{
  GLatEntry glat_entry;
  float findex=index;
  int n;

  if (def.type == Grid::gaussianLatitudeLongitudeType) {
    glat_entry.key=def.num_circles;
    if (gaus_lats == NULL || !gaus_lats->found(glat_entry.key,glat_entry)) {
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

int GRIBGrid::getLatitudeIndexOf(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  GLatEntry glat_entry;
  size_t n=0;

  if (def.type == Grid::gaussianLatitudeLongitudeType) {
    glat_entry.key=def.num_circles;
    if (gaus_lats == NULL || !gaus_lats->found(glat_entry.key,glat_entry)) {
	return -1;
    }
    glat_entry.key*=2;
    while (n < glat_entry.key) {
	if (myequalf(latitude,glat_entry.lats[n],0.01)) {
	  return n;
	}
	++n;
    }
  }
  else {
    return Grid::getLatitudeIndexOf(latitude,gaus_lats);
  }
  return -1;
}

int GRIBGrid::getLatitudeIndexNorthOf(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  GLatEntry glat_entry;
  int n;

  if (def.type == Grid::gaussianLatitudeLongitudeType) {
    glat_entry.key=def.num_circles;
    if (gaus_lats == NULL || !gaus_lats->found(glat_entry.key,glat_entry)) {
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
    return Grid::getLatitudeIndexNorthOf(latitude,gaus_lats);
  }
  return -1;
}

int GRIBGrid::getLatitudeIndexSouthOf(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  GLatEntry glat_entry;
  int n;

  if (def.type == Grid::gaussianLatitudeLongitudeType) {
    glat_entry.key=def.num_circles;
    if (gaus_lats == NULL || !gaus_lats->found(glat_entry.key,glat_entry)) {
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
    return Grid::getLatitudeIndexSouthOf(latitude,gaus_lats);
  }
  return -1;
}

void GRIBGrid::remapParameters()
{
  std::ifstream ifs;
  char line[80];
  std::string file_name;
  size_t n;

  if (!grib.map_available) {
    return;
  }
  if (!grib.map_filled || grib.last_src != grid.src) {
    if (map.param != NULL) {
	delete[] map.param;
	delete[] map.level_type;
	delete[] map.lvl;
	delete[] map.lvl2;
	delete[] map.mult;
    }
    ifs.open(("/local/dss/share/GRIB/parameter_map."+strutils::itos(grid.src)).c_str());
    if (!ifs.is_open()) {
	ifs.open(("/glade/home/dss/share/GRIB/parameter_map."+strutils::itos(grid.src)).c_str());
	if (!ifs.is_open()) {
	  ifs.open(("/glade/u/home/rdadata/share/GRIB/parameter_map."+strutils::itos(grid.src)).c_str());
	}
    }
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
  if (!myequalf(map.mult[grid.param],1.)) {
    this->multiplyBy(map.mult[grid.param]);
  }
  if (map.param[grid.param] != -1) {
    grid.param=map.param[grid.param];
  }
}

void GRIBGrid::setScaleAndPackingWidth(short decimal_scale_factor,short maximum_pack_width)
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
	this->multiplyBy(grid.nmean);
    }
    else
	grid.nmean=1;
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
	stats.max_val=-Grid::missingValue;
	stats.min_val=Grid::missingValue;
	stats.avg_val=0.;
	if (source.grib.t_range == 51)
	  mult=source.grid.nmean;
	else
	  mult=1.;
	for (n=0; n < dim.y; n++) {
	  for (m=0; m < dim.x; m++) {
	    l= (source.grib.scan_mode == grib.scan_mode) ? n : dim.y-n-1;
	    if (!myequalf(gridpoints[n][m],Grid::missingValue) && !myequalf(source.gridpoints[l][m],Grid::missingValue)) {
		gridpoints[n][m]+=(source.gridpoints[l][m]*mult);
		bitmap.map[cnt++]=1;
		if (gridpoints[n][m] > stats.max_val)
		  stats.max_val=gridpoints[n][m];
		if (gridpoints[n][m] < stats.min_val)
		  stats.min_val=gridpoints[n][m];
		stats.avg_val+=gridpoints[n][m];
		avg_cnt++;
	    }
	    else {
		gridpoints[n][m]=(Grid::missingValue*mult);
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
	if (source.referenceDateTime.getMonth() == referenceDateTime.getMonth()) {
	  if (source.referenceDateTime.getDay() == referenceDateTime.getDay() && source.referenceDateTime.getYear() > referenceDateTime.getYear()) {
	    grib.t_range=151;
	    grib.p1=0;
	    grib.p2=1;
	    grib.time_units=3;
	  }
	  else if (source.referenceDateTime.getYear() == referenceDateTime.getYear() && source.referenceDateTime.getDay() > referenceDateTime.getDay()){
	    grib.t_range=114;
	    grib.p2=1;
	    grib.time_units=2;
	  }
	  else if (source.referenceDateTime.getYear() == referenceDateTime.getYear() && source.referenceDateTime.getDay() == referenceDateTime.getDay() && source.referenceDateTime.getTime() > referenceDateTime.getTime()) {
	    grib.t_range=114;
	    grib.p2=source.referenceDateTime.getTime()-referenceDateTime.getTime()/100.;
	    grib.time_units=1;
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
	stats.max_val=-Grid::missingValue;
	stats.min_val=Grid::missingValue;
	stats.avg_val=0;
	for (n=0; n < dim.y; n++) {
	  for (m=0; m < dim.x; m++) {
	    l= (source.grib.scan_mode == grib.scan_mode) ? n : dim.y-n-1;
	    if (!myequalf(gridpoints[n][m],Grid::missingValue) && !myequalf(source.gridpoints[l][m],Grid::missingValue)) {
		gridpoints[n][m]-=source.gridpoints[l][m];
		bitmap.map[cnt++]=1;
		if (gridpoints[n][m] > stats.max_val)
		  stats.max_val=gridpoints[n][m];
		if (gridpoints[n][m] < stats.min_val)
		  stats.min_val=gridpoints[n][m];
		stats.avg_val+=gridpoints[n][m];
		avg_cnt++;
	    }
	    else {
		gridpoints[n][m]=Grid::missingValue;
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
	if (avg_cnt > 0)
	  stats.avg_val/=static_cast<double>(avg_cnt);
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
    stats.max_val=-Grid::missingValue;
    stats.min_val=Grid::missingValue;
    stats.avg_val=0.;
    for (n=0; n < dim.y; n++) {
	for (m=0; m < dim.x; m++) {
	  if (!myequalf(source.gridpoints[n][m],Grid::missingValue)) {
	    gridpoints[n][m]*=source.gridpoints[n][m];
	    if (gridpoints[n][m] > stats.max_val)
		stats.max_val=gridpoints[n][m];
	    if (gridpoints[n][m] < stats.min_val)
		stats.min_val=gridpoints[n][m];
	    stats.avg_val+=gridpoints[n][m];
	    avg_cnt++;
	  }
	  else
	    gridpoints[n][m]=Grid::missingValue;
	}
    }
  }
  if (avg_cnt > 0)
    stats.avg_val/=static_cast<double>(avg_cnt);
}

void GRIBGrid::operator/=(const GRIBGrid& source)
{
  int n,m;
  size_t avg_cnt=0;

  if (grib.capacity.points == 0)
    return;

  if (dim.size != source.dim.size || dim.x != source.dim.x || dim.y !=
      source.dim.y) {
    std::cerr << "Warning: unable to perform grid division" << std::endl;
    return;
  }
  else {
    stats.max_val=-Grid::missingValue;
    stats.min_val=Grid::missingValue;
    stats.avg_val=0.;
    for (n=0; n < dim.y; n++) {
	for (m=0; m < dim.x; m++) {
	  if (!myequalf(source.gridpoints[n][m],Grid::missingValue)) {
	    gridpoints[n][m]/=source.gridpoints[n][m];
	    if (gridpoints[n][m] > stats.max_val) stats.max_val=gridpoints[n][m];
	    if (gridpoints[n][m] < stats.min_val) stats.min_val=gridpoints[n][m];
	    stats.avg_val+=gridpoints[n][m];
	    avg_cnt++;
	  }
	  else
	    gridpoints[n][m]=Grid::missingValue;
	}
    }
  }

  if (avg_cnt > 0)
    stats.avg_val/=static_cast<double>(avg_cnt);
}

void GRIBGrid::divideBy(double div)
{
  int n,m;
  size_t avg_cnt=0;

  if (grib.capacity.points == 0)
    return;
  switch (grib.t_range) {
    case 114:
	grib.t_range=113;
	break;
    case 151:
	grib.t_range=51;
	break;
  }
  stats.min_val/=div;
  stats.max_val/=div;
  stats.avg_val=0.;
  for (n=0; n < dim.y; n++) {
    for (m=0; m < dim.x; m++) {
	if (!myequalf(gridpoints[n][m],Grid::missingValue)) {
	  gridpoints[n][m]/=div;
	  stats.avg_val+=gridpoints[n][m];
	  avg_cnt++;
	}
    }
  }
  if (avg_cnt > 0)
    stats.avg_val/=static_cast<double>(avg_cnt);
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
  switch (source.getModel()) {
    case 6:
	switch (source.getParameter()) {
// Height
	  case 1000:
	    grid.param=7;
	    grid.level1_type=100;
	    break;
// MSLP
	  case 1201:
	    grid.param=2;
	    grid.level1_type=102;
	    mult=100.;
	    grib.D-=2;
	    break;
// temperature
	  case 2000:
	    grid.param=11;
	    grid.level1_type=100;
	    break;
// RH
	  case 3000:
	    grid.param=52;
	    grid.level1_type=100;
	    break;
// mean RH through a sigma layer
	  case 3001:
	  case 3002:
	    grid.param=52;
	    grid.level1_type=108;
	    grid.level1=47.;
	    grid.level2=100.;
	    break;
// 6-hr total precip
	  case 3211:
	    grid.param=61;
	    grid.level1_type=1;
	    mult=1000.;
	    grib.D-=3;
	    if ( (source.grid.fcst_time % 10000) > 0) {
		grib.time_units=0;
		grib.p2=source.grid.fcst_time/10000*60+(source.grid.fcst_time % 10000)/100;
		grib.p1=grib.p2-360;
	    }
	    else {
		grib.time_units=1;
		grib.p2=source.grid.fcst_time/10000;
		grib.p1=grib.p2-6;
	    }
	    grib.t_range=4;
	    break;
// 12-hr total precip
	  case 3221:
	    grid.param=61;
	    grid.level1_type=1;
	    mult=1000.;
	    grib.D-=3;
	    if ( (source.grid.fcst_time % 10000) > 0) {
		grib.time_units=0;
		grib.p2=source.grid.fcst_time/10000*60+(source.grid.fcst_time % 10000)/100;
		grib.p1=grib.p2-720;
	    }
	    else {
		grib.time_units=1;
		grib.p2=source.grid.fcst_time/10000;
		grib.p1=grib.p2-12;
	    }
	    grib.t_range=4;
	    break;
// total precipitable water
	  case 3350:
	    grid.param=54;
	    grid.level1_type=200;
	    break;
// u-component of the wind
	  case 4000:
	    grid.param=33;
	    grid.level1_type=100;
	    grib.rescomp|=0x8;
	    break;
// 10m u-component
	  case 4020:
	    grid.param=33;
	    grid.level1_type=105;
	    grib.rescomp|=0x8;
	    break;
// v-component of the wind
	  case 4100:
	    grid.param=34;
	    grid.level1_type=100;
	    grib.rescomp|=0x8;
	    break;
// 10m v-component
	  case 4120:
	    grid.param=34;
	    grid.level1_type=105;
	    grib.rescomp|=0x8;
	    break;
// vertical velocity
	  case 5000:
	  case 5003:
	    grid.param=39;
	    grid.level1_type=100;
	    mult=100.;
	    grib.D-=2;
	    break;
	  default:
	    myerror="unable to convert parameter "+strutils::itos(source.getParameter());
	    exit(1);
	}
	grib.process=39;
	break;
    default:
	myerror="unrecognized model "+strutils::itos(source.getModel());
	exit(1);
  }
  grid.level2_type=-1;
  grib.sub_center=11;
  grib.scan_mode=0x40;
  if (grid.grid_type == 5 && !myequalf(def.elatitude,60.)) {
    myerror="unable to handle a grid length not at 60 degrees latitude";
    exit(1);
  }
  galloc();
  stats.avg_val=0.;
  stats.min_val=Grid::missingValue;
  stats.max_val=-Grid::missingValue;
  for (n=0; n < dim.y; n++) {
    for (m=0; m < dim.x; m++) {
	gridpoints[n][m]=source.getGridpoint(m,n)*mult;
	if (gridpoints[n][m] > stats.max_val) stats.max_val=gridpoints[n][m];
	if (gridpoints[n][m] < stats.min_val) stats.min_val=gridpoints[n][m];
	stats.avg_val+=gridpoints[n][m];
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
  switch (source.getSource()) {
    case 1:
	grid.src=7;
	break;
    case 3:
	grid.src=58;
	break;
    default:
	myerror="source code "+strutils::itos(source.getSource())+" not recognized";
	exit(1);
  }
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.param=octagonalGridParameterMap[source.getParameter()];
  grid.level2_type=-1;
  switch (grid.param) {
    case 1:
	if (myequalf(source.getFirstLevelValue(),1013.)) {
	  grid.level1_type=2;
	  grid.param=2;
	}
	else
	  grid.level1_type=1;
	factor=100.;
	break;
    case 2:
	grid.level1_type=2;
	break;
    case 7:
	grid.level1_type=100;
	break;
    case 11:
    case 12:
	if (myequalf(source.getFirstLevelValue(),1001.))
	  grid.level1_type=1;
	else
	  grid.level1_type=100;
	offset=273.15;
	break;
    case 33:
    case 34:
    case 39:
	grid.level1_type=100;
	break;
    case 52:
	grid.level1_type=101;
	break;
    case 80:
	grid.level1_type=1;
	offset=273.15;
	break;
    default:
	myerror="unable to convert parameter "+strutils::itos(source.getParameter());
	exit(1);
  }
  if (!myequalf(source.getSecondLevelValue(),0.)) {
    grid.level1=source.getSecondLevelValue();
    grid.level2=source.getFirstLevelValue();
  }
  else {
    grid.level1=source.getFirstLevelValue();
    grid.level2=0.;
  }
  referenceDateTime=source.getReferenceDateTime();
  if (!source.isAveragedGrid()) {
    grib.p1=source.getForecastTime()/10000;
    grib.time_units=1;
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
  dim=source.getDimensions();
  def=source.getDefinition();
  def.projection_flag=0x80;
  grib.rescomp=0x80;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != NULL) {
	delete[] bitmap.map;
	bitmap.map=NULL;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::missingValue;
  stats.max_val=-Grid::missingValue;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	gridpoints[n][m]=source.getGridpoint(m,n);
	if (myequalf(gridpoints[n][m],Grid::missingValue)) {
	  bitmap.map[cnt]=0;
	  grid.num_missing++;
	}
	else {
	  gridpoints[n][m]*=factor;
	  gridpoints[n][m]+=offset;
	  bitmap.map[cnt]=1;
	  if (gridpoints[n][m] > stats.max_val) stats.max_val=gridpoints[n][m];
	  if (gridpoints[n][m] < stats.min_val) stats.min_val=gridpoints[n][m];
	  stats.avg_val+=gridpoints[n][m];
	  avg_cnt++;
	}
	++cnt;
    }
  }
  if (avg_cnt > 0)
    stats.avg_val/=static_cast<double>(avg_cnt);
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


GRIBGrid& GRIBGrid::operator=(const LatLonGrid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double range;
  float factor=1.,offset=0.;

  grib.table=3;
  switch (source.getSource()) {
    case 1:
    case 5:
	grid.src=7;
	break;
    case 2:
	grid.src=57;
	break;
    case 3:
	grid.src=58;
	break;
    case 4:
	grid.src=255;
/*
	grib.lengths.pds_supp=10;
	grib.lengths.pds+=(12+grib.lengths.pds_supp);
	if (pds_supp != NULL)
	  delete[] pds_supp;
	pds_supp=new unsigned char[11];
	strcpy((char *)pds_supp,"ESSPO GRID");
*/
	break;
    case 9:
	if (myequalf(source.getFirstLevelValue(),700.))
	  grid.src=7;
	else {
	  myerror="source code "+strutils::itos(source.getSource())+" not recognized";
	  exit(1);
	}
	break;
    default:
	myerror="source code "+strutils::itos(source.getSource())+" not recognized";
	exit(1);
  }
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.param=octagonalGridParameterMap[source.getParameter()];
  grid.level2_type=-1;
  switch (grid.param) {
    case 1:
	if (myequalf(source.getFirstLevelValue(),1013.)) {
	  grid.level1_type=2;
	  grid.param=2;
	}
	else
	  grid.level1_type=1;
	factor=100.;
	break;
    case 2:
	grid.level1_type=2;
	break;
    case 7:
	grid.level1_type=100;
	if (myequalf(source.getFirstLevelValue(),700.) && source.getSource() == 9)
	  factor=0.01;
	break;
    case 11:
    case 12:
	if (myequalf(source.getFirstLevelValue(),1001.))
	  grid.level1_type=1;
	else
	  grid.level1_type=100;
	offset=273.15;
	break;
    case 33:
    case 34:
    case 39:
	grid.level1_type=100;
	break;
    case 52:
	grid.level1_type=101;
	break;
    case 80:
	grid.level1_type=1;
	offset=273.15;
	break;
    default:
	myerror="unable to convert parameter "+strutils::itos(source.getParameter());
	exit(1);
  }
  if (grid.level1_type > 10 && grid.level1_type != 102 && grid.level1_type != 200 && grid.level1_type != 201) {
    if (!myequalf(source.getSecondLevelValue(),0.)) {
	grid.level1=source.getFirstLevelValue();
	grid.level2=source.getSecondLevelValue();
    }
    else {
	grid.level1=source.getFirstLevelValue();
	grid.level2=0.;
    }
  }
  else
    grid.level1=grid.level2=0.;
  referenceDateTime=source.getReferenceDateTime();
  if (!source.isAveragedGrid()) {
    grib.p1=source.getForecastTime()/10000;
    grib.time_units=1;
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
  dim=source.getDimensions();
  def=source.getDefinition();
  grib.rescomp=0x80;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != NULL) {
	delete[] bitmap.map;
	bitmap.map=NULL;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::missingValue;
  stats.max_val=-Grid::missingValue;
  for (n=0; n < dim.y; n++) {
    for (m=0; m < dim.x; m++) {
	gridpoints[n][m]=source.getGridpoint(m,n);
	if (myequalf(gridpoints[n][m],Grid::missingValue)) {
	  bitmap.map[cnt]=0;
	  grid.num_missing++;
	}
	else {
	  gridpoints[n][m]*=factor;
	  gridpoints[n][m]+=offset;
	  bitmap.map[cnt]=1;
	  if (gridpoints[n][m] > stats.max_val) stats.max_val=gridpoints[n][m];
	  if (gridpoints[n][m] < stats.min_val) stats.min_val=gridpoints[n][m];
	  stats.avg_val+=gridpoints[n][m];
	  avg_cnt++;
	}
	cnt++;
    }
  }
  if (avg_cnt > 0)
    stats.avg_val/=static_cast<double>(avg_cnt);
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

GRIBGrid& GRIBGrid::operator=(const SLPGrid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double range;
  short months[]={0,31,28,31,30,31,30,31,31,30,31,30,31};

  grib.table=3;
  switch (source.getSource()) {
    case 3:
	grid.src=58;
	break;
    case 4:
	grid.src=57;
	break;
    case 11:
	grid.src=255;
	break;
    case 10:
	grid.src=59;
	break;
    case 12:
	grid.src=74;
	break;
    case 28:
	grid.src=7;
	break;
    case 29:
	grid.src=255;
	break;
    default:
	myerror="source code "+strutils::itos(source.getSource())+" not recognized";
	exit(1);
  }
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.param=2;
  grid.level1_type=102;
  grid.level1=0.;
  grid.level2_type=-1;
  grid.level2=0.;
  grib.p1=0;
  if (!source.isAveragedGrid()) {
    grib.p2=0;
    referenceDateTime=source.getReferenceDateTime();
    grib.time_units=1;
    grib.t_range=10;
    grid.nmean=0;
    grib.nmean_missing=0;
  }
  else {
    if (source.getReferenceDateTime().getTime() == 310000) {
	referenceDateTime.set(source.getReferenceDateTime().getYear(),source.getReferenceDateTime().getMonth(),1,0);
    }
    else {
	referenceDateTime.set(source.getReferenceDateTime().getYear(),source.getReferenceDateTime().getMonth(),1,source.getReferenceDateTime().getTime());
    }
    grib.time_units=3;
    grib.t_range=113;
    if ( (referenceDateTime.getYear() % 4) == 0) {
	months[2]=29;
    }
    grid.nmean=months[referenceDateTime.getMonth()];
    if (source.getNumberAveraged() > grid.nmean) {
	grid.nmean*=2;
	grib.time_units=1;
	grib.p2=12;
    }
    else {
	grib.time_units=2;
	grib.p2=1;
    }
    grib.nmean_missing=grid.nmean-source.getNumberAveraged();
  }
  grib.sub_center=0;
  grib.D=2;
  grid.grid_type=0;
  grib.scan_mode=0x40;
  dim=source.getDimensions();
  dim.size=dim.x*dim.y;
  def=source.getDefinition();
  def.elatitude=90.;
  grib.rescomp=0x80;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != NULL) {
	delete[] bitmap.map;
	bitmap.map=NULL;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::missingValue;
  stats.max_val=-Grid::missingValue;
  for (n=0; n < dim.y-1; n++) {
    for (m=0; m < dim.x; m++) {
	gridpoints[n][m]=source.getGridpoint(m,n);
	if (myequalf(gridpoints[n][m],Grid::missingValue)) {
	  bitmap.map[cnt]=0;
	  grid.num_missing++;
	}
	else {
	  gridpoints[n][m]*=100.;
	  bitmap.map[cnt]=1;
	  if (gridpoints[n][m] > stats.max_val) stats.max_val=gridpoints[n][m];
	  if (gridpoints[n][m] < stats.min_val) stats.min_val=gridpoints[n][m];
	  stats.avg_val+=gridpoints[n][m];
	  avg_cnt++;
	}
	cnt++;
    }
  }
  for (m=0; m < dim.x; m++) {
    gridpoints[n][m]=source.getPoleValue();
    if (myequalf(gridpoints[n][m],Grid::missingValue)) {
	bitmap.map[cnt]=0;
	grid.num_missing++;
    }
    else {
	gridpoints[n][m]*=100.;
	bitmap.map[cnt]=1;
	if (gridpoints[n][m] > stats.max_val) stats.max_val=gridpoints[n][m];
	if (gridpoints[n][m] < stats.min_val) stats.min_val=gridpoints[n][m];
	stats.avg_val+=gridpoints[n][m];
	avg_cnt++;
    }
    cnt++;
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
  setScaleAndPackingWidth(grib.D,grib.pack_width);
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
  grib.process=source.getGeneratingProgram();
  grib.grid_catalog_id=255;
  grid.param=on84GridParameterMap[source.getParameter()];
  grib.D=2;
  switch (grid.param) {
    case 1:
	mult=100.;
	grib.D=0;
	break;
    case 39:
	mult=100.;
	grib.D=3;
	break;
    case 61:
	mult=1000.;
	grib.D=1;
	break;
    case 210:
	precision=5;
	grib.D=0;
	grib.table=129;
	break;
  }
  if (grid.param == 0) {
    myerror="unable to convert parameter "+strutils::itos(source.getParameter());
    exit(1);
  }
  grid.level2_type=-1;
  switch (source.getFirstLevelType()) {
// height above ground
    case 6:
	grid.level1_type=105;
	grid.level1=source.getFirstLevelValue();
	grid.level2=source.getSecondLevelValue();
	if (!myequalf(grid.level1,source.getFirstLevelValue()) || !myequalf(grid.level2,source.getSecondLevelValue())) {
	  myerror="error converting fractional level(s) "+strutils::ftos(source.getFirstLevelValue())+" "+strutils::ftos(source.getSecondLevelValue());
	  exit(1);
	}
	break;
// pressure level
    case 8:
	grid.level1_type=100;
	grid.level1=source.getFirstLevelValue();
	grid.level2=source.getSecondLevelValue();
	break;
// mean sea-level
    case 128:
	grid.level1_type=102;
	grid.level1=grid.level2=0.;
	if (grid.param == 1) grid.param=2;
	break;
// surface
    case 129:
	if (source.getParameter() == 16) {
	  grid.level1_type=105;
	  grid.level1=2.;
	  grid.level2=0.;
	}
	else {
	  grid.level1_type=1;
	  grid.level1=grid.level2=0.;
	}
	break;
// tropopause level
    case 130:
	grid.level1_type=7;
	grid.level1=grid.level2=0.;
	break;
// maximum wind speed level
    case 131:
	grid.level1_type=6;
	grid.level1=grid.level2=0.;
	break;
// boundary
    case 144:
	if (myequalf(source.getFirstLevelValue(),0.) && myequalf(source.getSecondLevelValue(),1.)) {
	  grid.level1_type=107;
	  grid.level1=0.9950;
	  grid.level2=0.;
	}
	else {
	  myerror="error converting boundary layer "+strutils::ftos(source.getFirstLevelValue())+" "+strutils::ftos(source.getSecondLevelValue());
	  exit(1);
	}
	break;
// troposphere
    case 145:
// sigma level
	if (myequalf(source.getSecondLevelValue(),0.)) {
	  grid.level1_type=107;
	  grid.level1=source.getFirstLevelValue();
	  grid.level2=0.;
	}
// sigma layer
	else {
	  if (myequalf(source.getFirstLevelValue(),0.) && myequalf(source.getSecondLevelValue(),1.)) {
	    grid.level1_type=200;
	    grid.level1=0.;
	  }
	  else {
	    grid.level1_type=108;
	    grid.level1=source.getFirstLevelValue();
	    grid.level2=source.getSecondLevelValue();
	  }
	}
	break;
// entire atmosphere
    case 148:
	grid.level1_type=200;
	grid.level1=grid.level2=0.;
	break;
    default:
	myerror="unable to convert level type "+strutils::itos(source.getFirstLevelType());
	exit(1);
  }
  referenceDateTime=source.getReferenceDateTime();
  if (!source.isAveragedGrid()) {
    switch (source.getTimeMarker()) {
	case 0:
	  grib.p1=source.getForecastTime()/10000;
	  grib.p2=0;
	  grib.t_range=10;
	  grib.time_units=1;
	  break;
	case 3:
	  if (grid.param == 61) {
	    grib.p1=source.getF1()-source.getF2();
	    grib.p2=source.getF1();
	    grib.t_range=5;
	    grib.time_units=1;
	  }
	  else {
	    myerror="unable to convert time marker "+strutils::itos(source.getTimeMarker());
	    exit(1);
	  }
	  break;
	default:
	  myerror="unable to convert time marker "+strutils::itos(source.getTimeMarker());
	  exit(1);
    }
    grid.nmean=grib.nmean_missing=0;
  }
  else {
    myerror="unable to convert a mean grid";
    exit(1);
  }
  grib.sub_center=0;
  switch (source.getType()) {
    case 29:
    case 30:
	grid.grid_type=0;
	grib.scan_mode=0x40;
	break;
    case 27:
    case 28:
    case 36:
    case 47:
	grid.grid_type=5;
	def.projection_flag=0;
	grib.scan_mode=0x40;
	grib.sub_center=11;
	break;
    default:
	myerror="unable to convert grid type "+strutils::itos(source.getType());
	exit(1);
  }
  dim=source.getDimensions();
  def=source.getDefinition();
  grib.rescomp=0x80;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != NULL) {
	delete[] bitmap.map;
	bitmap.map=NULL;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.max_val=-Grid::missingValue;
  stats.min_val=Grid::missingValue;
  for (n=0; n < dim.y; n++) {
    for (m=0; m < dim.x; m++) {
	gridpoints[n][m]=source.getGridpoint(m,n);
	if (myequalf(gridpoints[n][m],Grid::missingValue)) {
	  bitmap.map[cnt]=0;
	  grid.num_missing++;
	}
	else {
	  gridpoints[n][m]*=mult;
	  bitmap.map[cnt]=1;
	  if (gridpoints[n][m] > stats.max_val)
	    stats.max_val=gridpoints[n][m];
	  if (gridpoints[n][m] < stats.min_val)
	    stats.min_val=gridpoints[n][m];
	  stats.avg_val+=gridpoints[n][m];
	  avg_cnt++;
	}
	cnt++;
    }
  }
  if (avg_cnt > 0)
    stats.avg_val/=static_cast<double>(avg_cnt);
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
  if (strcmp(source.getParameterName()," ALB") == 0) {
    grid.param=84;
    grid.level1_type=1;
    grid.level1=0;
  }
  else if (strcmp(source.getParameterName(),"  AU") == 0) {
    grid.param=33;
    grid.level1_type=105;
    grid.level1=10.;
  }
  else if (strcmp(source.getParameterName(),"  AV") == 0) {
    grid.param=34;
    grid.level1_type=105;
    grid.level1=10.;
  }
  else if (strcmp(source.getParameterName(),"CLDT") == 0) {
    grid.param=71;
    grid.level1_type=200;
    grid.level1=0.;
  }
  else if (strcmp(source.getParameterName()," FSR") == 0) {
    grid.param=116;
    grid.level1_type=8;
    grid.level1=0.;
  }
  else if (strcmp(source.getParameterName()," FSS") == 0) {
    grid.param=116;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (strcmp(source.getParameterName(),"FLAG") == 0) {
    grid.param=115;
    grid.level1_type=8;
    grid.level1=0.;
  }
  else if (strcmp(source.getParameterName(),"FSRG") == 0) {
    grid.param=116;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (strcmp(source.getParameterName(),"  GT") == 0) {
    grid.param=11;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (strcmp(source.getParameterName(),"  PS") == 0) {
    grid.param=2;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (strcmp(source.getParameterName()," PHI") == 0) {
    grid.param=7;
    grid.level1_type=100;
    grid.level1=source.getFirstLevelValue();
  }
  else if (strcmp(source.getParameterName()," PCP") == 0) {
    grid.param=61;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (strcmp(source.getParameterName(),"PMSL") == 0) {
    grid.param=2;
    grid.level1_type=102;
    grid.level1=0.;
  }
  else if (strcmp(source.getParameterName(),"  SQ") == 0) {
    grid.param=51;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (strcmp(source.getParameterName(),"  ST") == 0) {
    grid.param=11;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (strcmp(source.getParameterName()," SNO") == 0) {
    grid.param=79;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (strcmp(source.getParameterName(),"SHUM") == 0) {
    grid.param=51;
    grid.level1_type=100;
    grid.level1=source.getFirstLevelValue();
  }
  else if (strcmp(source.getParameterName(),"STMX") == 0) {
    grid.param=15;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (strcmp(source.getParameterName(),"STMN") == 0) {
    grid.param=16;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (strcmp(source.getParameterName(),"SWMX") == 0) {
    grid.param=32;
    grid.level1_type=105;
    grid.level1=10.;
  }
  else if (strcmp(source.getParameterName(),"TEMP") == 0) {
    grid.param=11;
    grid.level1_type=100;
    grid.level1=source.getFirstLevelValue();
  }
  else if (strcmp(source.getParameterName(),"   U") == 0) {
    grid.param=33;
    grid.level1_type=100;
    grid.level1=source.getFirstLevelValue();
  }
  else if (strcmp(source.getParameterName(),"   V") == 0) {
    grid.param=34;
    grid.level1_type=100;
    grid.level1=source.getFirstLevelValue();
  }
  else {
    myerror="unable to convert parameter "+std::string(source.getParameterName());
    exit(1);
  }
  grid.level2=0.;
  referenceDateTime=source.getReferenceDateTime();
  if (referenceDateTime.getDay() == 0) {
    referenceDateTime.setDay(1);
    grib.t_range=113;
    grib.time_units=3;
  }
  else {
    grib.t_range=1;
    grib.time_units=1;
  }
  grib.p1=0;
  grib.p2=0;
  grid.nmean=0;
  grib.nmean_missing=0;
  grib.sub_center=0;
  grid.grid_type=4;
  dim=source.getDimensions();
  dim.x--;
  def=source.getDefinition();
  def.type=gaussianLatitudeLongitudeType;
  grib.rescomp=0x80;
  grib.scan_mode=0x0;
  dim.size=dim.x*dim.y;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != NULL) {
	delete[] bitmap.map;
	bitmap.map=NULL;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.max_val=-Grid::missingValue;
  stats.min_val=Grid::missingValue;
  stats.avg_val=0.;
  l=dim.y-1;
  for (n=0; n < dim.y; n++,l--) {
    for (m=0; m < dim.x; m++) {
	gridpoints[n][m]=source.getGridpoint(m,l);
	if (myequalf(gridpoints[n][m],Grid::missingValue)) {
	  bitmap.map[cnt]=0;
	  grid.num_missing++;
	}
	else {
	  bitmap.map[cnt]=1;
	  switch (grid.param) {
	    case 2:
		gridpoints[n][m]*=100.;
		break;
	    case 11:
	    case 15:
	    case 16:
		gridpoints[n][m]+=273.15;
		break;
	    case 61:
		gridpoints[n][m]*=86400.;
		break;
	  }
	  if (gridpoints[n][m] > stats.max_val) stats.max_val=gridpoints[n][m];
	  if (gridpoints[n][m] < stats.min_val) stats.min_val=gridpoints[n][m];
	  stats.avg_val+=gridpoints[n][m];
	  avg_cnt++;
	}
	cnt++;
    }
  }
  if (avg_cnt > 0)
    stats.avg_val/=static_cast<double>(avg_cnt);
  if (grid.param == 2)
    grib.D=0;
  else
    grib.D=2;
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

bool GRIBGrid::isAveragedGrid() const
{
  switch (grib.t_range) {
    case 51:
    case 113:
    case 115:
    case 117:
    case 123:
	return true;
    default:
	return false;
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
  referenceDateTime=source.getReferenceDateTime();
  grib.time_units=1;
  grib.t_range=10;
  grid.nmean=0;
  grib.nmean_missing=0;
  grib.sub_center=0;
  grib.D=2;
  grid.grid_type=0;
  grib.scan_mode=0x0;
  dim=source.getDimensions();
  dim.size=dim.x*dim.y;
  def=source.getDefinition();
  grib.rescomp=0x80;
  galloc();
  if (bitmap.capacity < dim.size) {
    if (bitmap.map != NULL) {
	delete[] bitmap.map;
	bitmap.map=NULL;
    }
    bitmap.capacity=dim.size;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::missingValue;
  stats.max_val=-Grid::missingValue;
  for (n=0; n < dim.y; n++) {
    for (m=0; m < dim.x; m++) {
	gridpoints[n][m]=source.getGridpoint(m,n);
	if (myequalf(gridpoints[n][m],Grid::missingValue)) {
	  bitmap.map[cnt]=0;
	  grid.num_missing++;
	}
	else {
	  bitmap.map[cnt]=1;
	  if (gridpoints[n][m] > stats.max_val) stats.max_val=gridpoints[n][m];
	  if (gridpoints[n][m] < stats.min_val) stats.min_val=gridpoints[n][m];
	  stats.avg_val+=gridpoints[n][m];
	  avg_cnt++;
	}
	cnt++;
    }
  }
  if (avg_cnt > 0)
    stats.avg_val/=static_cast<double>(avg_cnt);
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

short mapParameterData(short center,size_t disc,short param_cat,short param_num,short& table)
{
  switch (disc) {
// meteorological products
    case 0:
	switch (param_cat) {
// temperature parameters
	  case 0:
	    switch (param_num) {
		case 0:
		  return 11;
		case 1:
		  return 12;
		case 2:
		  return 13;
		case 3:
		  return 14;
		case 4:
		  return 15;
		case 5:
		  return 16;
		case 6:
		  return 17;
		case 7:
		  return 18;
		case 8:
		  return 19;
		case 9:
		  return 25;
		case 10:
		  return 121;
		case 11:
		  return 122;
		case 12:
		  myerror="there is no GRIB1 parameter code for 'Heat index'";
		  exit(1);
		case 13:
		  myerror="there is no GRIB1 parameter code for 'Wind chill factor'";
		  exit(1);
		case 14:
		  myerror="there is no GRIB1 parameter code for 'Minimum dew point depression'";
		  exit(1);
		case 15:
		  myerror="there is no GRIB1 parameter code for 'Virtual potential temperature'";
		  exit(1);
		case 16:
		  myerror="there is no GRIB1 parameter code for 'Snow phase change heat flux'";
		  exit(1);
		case 21:
		  switch (center) {
		    case 7:
			table=131;
			return 193;
		  }
		  myerror="there is no GRIB1 parameter code for 'Apparent temperature'";
		  exit(1);
		case 192:
		  switch (center) {
		    case 7:
			return 229;
		  }
	    }
	    break;
// moisture parameters
	  case 1:
	    switch (param_num) {
		case 0:
		  return 51;
		case 1:
		  return 52;
		case 2:
		  return 53;
		case 3:
		  return 54;
		case 4:
		  return 55;
		case 5:
		  return 56;
		case 6:
		  return 57;
		case 7:
		  return 59;
		case 8:
		  return 61;
		case 9:
		  return 62;
		case 10:
		  return 63;
		case 11:
		  return 66;
		case 12:
		  return 64;
		case 13:
		  return 65;
		case 14:
		  return 78;
		case 15:
		  return 79;
		case 16:
		  return 99;
		case 17:
		  myerror="there is no GRIB1 parameter code for 'Snow age'";
		  exit(1);
		case 18:
		  myerror="there is no GRIB1 parameter code for 'Absolute humidity'";
		  exit(1);
		case 19:
		  myerror="there is no GRIB1 parameter code for 'Precipitation type'";
		  exit(1);
		case 20:
		  myerror="there is no GRIB1 parameter code for 'Integrated liquid water'";
		  exit(1);
		case 21:
		  myerror="there is no GRIB1 parameter code for 'Condensate water'";
		  exit(1);
		case 22:
		  switch (center) {
		    case 7:
			return 153;
		  }
		  myerror="there is no GRIB1 parameter code for 'Cloud mixing ratio'";
		  exit(1);
		case 23:
		  myerror="there is no GRIB1 parameter code for 'Ice water mixing ratio'";
		  exit(1);
		case 24:
		  myerror="there is no GRIB1 parameter code for 'Rain mixing ratio'";
		  exit(1);
		case 25:
		  myerror="there is no GRIB1 parameter code for 'Snow mixing ratio'";
		  exit(1);
		case 26:
		  myerror="there is no GRIB1 parameter code for 'Horizontal moisture convergence'";
		  exit(1);
		case 27:
		  myerror="there is no GRIB1 parameter code for 'Maximum relative humidity'";
		  exit(1);
		case 28:
		  myerror="there is no GRIB1 parameter code for 'Maximum absolute humidity'";
		  exit(1);
		case 29:
		  myerror="there is no GRIB1 parameter code for 'Total snowfall'";
		  exit(1);
		case 30:
		  myerror="there is no GRIB1 parameter code for 'Precipitable water category'";
		  exit(1);
		case 31:
		  myerror="there is no GRIB1 parameter code for 'Hail'";
		  exit(1);
		case 32:
		  myerror="there is no GRIB1 parameter code for 'Graupel (snow pellets)'";
		  exit(1);
		case 33:
		  myerror="there is no GRIB1 parameter code for 'Categorical rain'";
		  exit(1);
		case 34:
		  myerror="there is no GRIB1 parameter code for 'Categorical freezing rain'";
		  exit(1);
		case 35:
		  myerror="there is no GRIB1 parameter code for 'Categorical ice pellets'";
		  exit(1);
		case 36:
		  myerror="there is no GRIB1 parameter code for 'Categorical snow'";
		  exit(1);
		case 37:
		  myerror="there is no GRIB1 parameter code for 'Convective precipitation rate'";
		  exit(1);
		case 38:
		  myerror="there is no GRIB1 parameter code for 'Horizontal moisture divergence'";
		  exit(1);
		case 39:
		  switch (center) {
		    case 7:
			return 194;
		  }
		  myerror="there is no GRIB1 parameter code for 'Percent frozen precipitation'";
		  exit(1);
		case 40:
		  myerror="there is no GRIB1 parameter code for 'Potential evaporation'";
		  exit(1);
		case 41:
		  myerror="there is no GRIB1 parameter code for 'Potential evaporation rate'";
		  exit(1);
		case 42:
		  myerror="there is no GRIB1 parameter code for 'Snow cover'";
		  exit(1);
		case 43:
		  myerror="there is no GRIB1 parameter code for 'Rain fraction of total water'";
		  exit(1);
		case 44:
		  myerror="there is no GRIB1 parameter code for 'Rime factor'";
		  exit(1);
		case 45:
		  myerror="there is no GRIB1 parameter code for 'Total column integrated rain'";
		  exit(1);
		case 46:
		  myerror="there is no GRIB1 parameter code for 'Total column integrated snow'";
		  exit(1);
		case 192:
		  switch (center) {
		    case 7:
			return 140;
		  }
		case 193:
		  switch (center) {
		    case 7:
			return 141;
		  }
		case 194:
		  switch (center) {
		    case 7:
			return 142;
		  }
		case 195:
		  switch (center) {
		    case 7:
			return 143;
		  }
		case 196:
		  switch (center) {
		    case 7:
			return 214;
		  }
		case 197:
		  switch (center) {
		    case 7:
			return 135;
		  }
		case 199:
		  switch (center) {
		    case 7:
			return 228;
		  }
		case 200:
		  switch (center) {
		    case 7:
			return 145;
		  }
		case 201:
		  switch (center) {
		    case 7:
			return 238;
		  }
		case 206:
		  switch (center) {
		    case 7:
			return 186;
		  }
		case 207:
		  switch (center) {
		    case 7:
			return 198;
		  }
		case 208:
		  switch (center) {
		    case 7:
			return 239;
		  }
		case 213:
		  switch (center) {
		    case 7:
			return 243;
		  }
		case 214:
		  switch (center) {
		    case 7:
			return 245;
		  }
		case 215:
		  switch (center) {
		    case 7:
			return 249;
		  }
		case 216:
		  switch (center) {
		    case 7:
			return 159;
		  }
	    }
	    break;
// momentum parameters
	  case 2:
	    switch(param_num) {
		case 0:
		  return 31;
		case 1:
		  return 32;
		case 2:
		  return 33;
		case 3:
		  return 34;
		case 4:
		  return 35;
		case 5:
		  return 36;
		case 6:
		  return 37;
		case 7:
		  return 38;
		case 8:
		  return 39;
		case 9:
		  return 40;
		case 10:
		  return 41;
		case 11:
		  return 42;
		case 12:
		  return 43;
		case 13:
		  return 44;
		case 14:
		  return 4;
		case 15:
		  return 45;
		case 16:
		  return 46;
		case 17:
		  return 124;
		case 18:
		  return 125;
		case 19:
		  return 126;
		case 20:
		  return 123;
		case 21:
		  myerror="there is no GRIB1 parameter code for 'Maximum wind speed'";
		  exit(1);
		case 22:
		  switch (center) {
		    case 7:
			return 180;
		    default:
			myerror="there is no GRIB1 parameter code for 'Wind speed (gust)'";
			exit(1);
		  }
		case 23:
		  myerror="there is no GRIB1 parameter code for 'u-component of wind (gust)'";
		  exit(1);
		case 24:
		  myerror="there is no GRIB1 parameter code for 'v-component of wind (gust)'";
		  exit(1);
		case 25:
		  myerror="there is no GRIB1 parameter code for 'Vertical speed shear'";
		  exit(1);
		case 26:
		  myerror="there is no GRIB1 parameter code for 'Horizontal momentum flux'";
		  exit(1);
		case 27:
		  myerror="there is no GRIB1 parameter code for 'u-component storm motion'";
		  exit(1);
		case 28:
		  myerror="there is no GRIB1 parameter code for 'v-component storm motion'";
		  exit(1);
		case 29:
		  myerror="there is no GRIB1 parameter code for 'Drag coefficient'";
		  exit(1);
		case 30:
		  myerror="there is no GRIB1 parameter code for 'Frictional velocity'";
		  exit(1);
		case 192:
		  switch (center) {
		    case 7:
			return 136;
		  }
		case 193:
		  switch (center) {
		    case 7:
			return 172;
		  }
		case 194:
		  switch (center) {
		    case 7:
			return 196;
		  }
		case 195:
		  switch (center) {
		    case 7:
			return 197;
		  }
		case 196:
		  switch (center) {
		    case 7:
			return 252;
		  }
		case 197:
		  switch (center) {
		    case 7:
			return 253;
		  }
		case 224:
		  switch (center) {
		    case 7:
			table=129;
			return 241;
		  }
	    }
	    break;
// mass parameters
	  case 3:
	    switch (param_num) {
		case 0:
		  return 1;
		case 1:
		  return 2;
		case 2:
		  return 3;
		case 3:
		  return 5;
		case 4:
		  return 6;
		case 5:
		  return 7;
		case 6:
		  return 8;
		case 7:
		  return 9;
		case 8:
		  return 26;
		case 9:
		  return 27;
		case 10:
		  return 89;
		case 11:
		  myerror="there is no GRIB1 parameter code for 'Altimeter setting'";
		  exit(1);
		case 12:
		  myerror="there is no GRIB1 parameter code for 'Thickness'";
		  exit(1);
		case 13:
		  myerror="there is no GRIB1 parameter code for 'Pressure altitude'";
		  exit(1);
		case 14:
		  myerror="there is no GRIB1 parameter code for 'Density altitude'";
		  exit(1);
		case 15:
		  myerror="there is no GRIB1 parameter code for '5-wave geopotential height'";
		  exit(1);
		case 16:
		  myerror="there is no GRIB1 parameter code for 'Zonal flux of gravity wave stress'";
		  exit(1);
		case 17:
		  myerror="there is no GRIB1 parameter code for 'Meridional flux of gravity wave stress'";
		  exit(1);
		case 18:
		  myerror="there is no GRIB1 parameter code for 'Planetary boundary layer height'";
		  exit(1);
		case 19:
		  myerror="there is no GRIB1 parameter code for '5-wave geopotential height anomaly'";
		  exit(1);
		case 192:
		  switch (center) {
		    case 7:
			return 130;
		  }
		case 193:
		  switch (center) {
		    case 7:
			return 222;
		  }
		case 194:
		  switch (center) {
		    case 7:
			return 147;
		  }
		case 195:
		  switch (center) {
		    case 7:
			return 148;
		  }
		case 196:
		  switch (center) {
		    case 7:
			return 221;
		  }
		case 197:
		  switch (center) {
		    case 7:
			return 230;
		  }
		case 198:
		  switch (center) {
		    case 7:
			return 129;
		  }
		case 199:
		  switch (center) {
		    case 7:
			return 137;
		  }
		case 200:
		  switch (center) {
		    case 7:
			table=129;
			return 141;
		  }
	    }
	    break;
// short-wave radiation parameters
	  case 4:
	    switch (param_num) {
		case 0:
		  return 111;
		case 1:
		  return 113;
		case 2:
		  return 116;
		case 3:
		  return 117;
		case 4:
		  return 118;
		case 5:
		  return 119;
		case 6:
		  return 120;
		case 7:
		  myerror="there is no GRIB1 parameter code for 'Downward short-wave radiation flux'";
		  exit(1);
		case 8:
		  myerror="there is no GRIB1 parameter code for 'Upward short-wave radiation flux'";
		  exit(1);
		case 192:
		  switch (center) {
		    case 7:
			return 204;
		  }
		case 193:
		  switch (center) {
		    case 7:
			return 211;
		  }
		case 196:
		  switch (center) {
		    case 7:
			return 161;
		  }
	    }
	    break;
// long-wave radiation parameters
	  case 5:
	    switch (param_num) {
		case 0:
		  return 112;
		case 1:
		  return 114;
		case 2:
		  return 115;
		case 3:
		  myerror="there is no GRIB1 parameter code for 'Downward long-wave radiation flux'";
		  exit(1);
		case 4:
		  myerror="there is no GRIB1 parameter code for 'Upward long-wave radiation flux'";
		  exit(1);
		case 192:
		  switch (center) {
		    case 7:
			return 205;
		  }
		case 193:
		  switch (center) {
		    case 7:
			return 212;
		  }
	    }
	    break;
// cloud parameters
	  case 6:
	    switch (param_num) {
		case 0:
		  return 58;
		case 1:
		  return 71;
		case 2:
		  return 72;
		case 3:
		  return 73;
		case 4:
		  return 74;
		case 5:
		  return 75;
		case 6:
		  return 76;
		case 7:
		  myerror="there is no GRIB1 parameter code for 'Cloud amount'";
		  exit(1);
		case 8:
		  myerror="there is no GRIB1 parameter code for 'Cloud type'";
		  exit(1);
		case 9:
		  myerror="there is no GRIB1 parameter code for 'Thunderstorm maximum tops'";
		  exit(1);
		case 10:
		  myerror="there is no GRIB1 parameter code for 'Thunderstorm coverage'";
		  exit(1);
		case 11:
		  myerror="there is no GRIB1 parameter code for 'Cloud base'";
		  exit(1);
		case 12:
		  myerror="there is no GRIB1 parameter code for 'Cloud top'";
		  exit(1);
		case 13:
		  myerror="there is no GRIB1 parameter code for 'Ceiling'";
		  exit(1);
		case 14:
		  myerror="there is no GRIB1 parameter code for 'Non-convective cloud cover'";
		  exit(1);
		case 15:
		  myerror="there is no GRIB1 parameter code for 'Cloud work function'";
		  exit(1);
		case 16:
		  myerror="there is no GRIB1 parameter code for 'Convective cloud efficiency'";
		  exit(1);
		case 17:
		  myerror="there is no GRIB1 parameter code for 'Total condensate'";
		  exit(1);
		case 18:
		  myerror="there is no GRIB1 parameter code for 'Total column-integrated cloud water'";
		  exit(1);
		case 19:
		  myerror="there is no GRIB1 parameter code for 'Total column-integrated cloud ice'";
		  exit(1);
		case 20:
		  myerror="there is no GRIB1 parameter code for 'Total column-integrated cloud condensate'";
		  exit(1);
		case 21:
		  myerror="there is no GRIB1 parameter code for 'Ice fraction of total condensate'";
		  exit(1);
		case 192:
		  switch (center) {
		    case 7:
			return 213;
		  }
		case 193:
		  switch (center) {
		    case 7:
			return 146;
		  }
		case 201:
		  switch (center) {
		    case 7:
			table=133;
			return 191;
		  }
	    }
	    break;
// thermodynamic stability index parameters
	  case 7:
	    switch (param_num) {
		case 0:
		  return 24;
		case 1:
		  return 77;
		case 2:
		  myerror="there is no GRIB1 parameter code for 'K index'";
		  exit(1);
		case 3:
		  myerror="there is no GRIB1 parameter code for 'KO index'";
		  exit(1);
		case 4:
		  myerror="there is no GRIB1 parameter code for 'Total totals index'";
		  exit(1);
		case 5:
		  myerror="there is no GRIB1 parameter code for 'Sweat index'";
		  exit(1);
		case 6:
		  switch (center) {
		    case 7:
			return 157;
		    default:
			myerror="there is no GRIB1 parameter code for 'Convective available potential energy'";
			exit(1);
		  }
		case 7:
		  switch (center) {
		    case 7:
			return 156;
		    default:
			myerror="there is no GRIB1 parameter code for 'Convective inhibition'";
			exit(1);
		  }
		case 8:
		  switch (center) {
		    case 7:
			return 190;
		    default:
			myerror="there is no GRIB1 parameter code for 'Storm-relative helicity'";
			exit(1);
		  }
		case 9:
		  myerror="there is no GRIB1 parameter code for 'Energy helicity index'";
		  exit(1);
		case 10:
		  myerror="there is no GRIB1 parameter code for 'Surface lifted index'";
		  exit(1);
		case 11:
		  myerror="there is no GRIB1 parameter code for 'Best (4-layer) lifted index'";
		  exit(1);
		case 12:
		  myerror="there is no GRIB1 parameter code for 'Richardson number'";
		  exit(1);
		case 192:
		  switch (center) {
		    case 7:
			return 131;
		  }
		case 193:
		  switch (center) {
		    case 7:
			return 132;
		  }
		case 194:
		  switch (center) {
		    case 7:
			return 254;
		  }
	    }
	    break;
// aerosol parameters
	  case 13:
	    switch (param_num) {
		case 0:
		  myerror="there is no GRIB1 parameter code for 'Aerosol type'";
		  exit(1);
	    }
	    break;
// trace gas parameters
	  case 14:
	    switch (param_num) {
		case 0:
		  return 10;
		case 1:
		  myerror="there is no GRIB1 parameter code for 'Ozone mixing ratio'";
		  exit(1);
		case 192:
		  switch (center) {
		    case 7:
			return 154;
		  }
	    }
	    break;
// radar parameters
	  case 15:
	    switch (param_num) {
		case 0:
		  myerror="there is no GRIB1 parameter code for 'Base spectrum width'";
		  exit(1);
		case 1:
		  myerror="there is no GRIB1 parameter code for 'Base reflectivity'";
		  exit(1);
		case 2:
		  myerror="there is no GRIB1 parameter code for 'Base radial velocity'";
		  exit(1);
		case 3:
		  myerror="there is no GRIB1 parameter code for 'Vertically-integrated liquid'";
		  exit(1);
		case 4:
		  myerror="there is no GRIB1 parameter code for 'Layer-maximum base reflectivity'";
		  exit(1);
		case 5:
		  myerror="there is no GRIB1 parameter code for 'Radar precipitation'";
		  exit(1);
		case 6:
		  return 21;
		case 7:
		  return 22;
		case 8:
		  return 23;
	    }
	    break;
// nuclear/radiology parameters
	  case 18:
	    break;
// physical atmospheric property parameters
	  case 19:
	    switch (param_num) {
		case 0:
		  return 20;
		case 1:
		  return 84;
		case 2:
		  return 60;
		case 3:
		  return 67;
		case 4:
		  myerror="there is no GRIB1 parameter code for 'Volcanic ash'";
		  exit(1);
		case 5:
		  myerror="there is no GRIB1 parameter code for 'Icing top'";
		  exit(1);
		case 6:
		  myerror="there is no GRIB1 parameter code for 'Icing base'";
		  exit(1);
		case 7:
		  myerror="there is no GRIB1 parameter code for 'Icing'";
		  exit(1);
		case 8:
		  myerror="there is no GRIB1 parameter code for 'Turbulence top'";
		  exit(1);
		case 9:
		  myerror="there is no GRIB1 parameter code for 'Turbulence base'";
		  exit(1);
		case 10:
		  myerror="there is no GRIB1 parameter code for 'Turbulence'";
		  exit(1);
		case 11:
		  myerror="there is no GRIB1 parameter code for 'Turbulent kinetic energy'";
		  exit(1);
		case 12:
		  myerror="there is no GRIB1 parameter code for 'Planetary boundary layer regime'";
		  exit(1);
		case 13:
		  myerror="there is no GRIB1 parameter code for 'Contrail intensity'";
		  exit(1);
		case 14:
		  myerror="there is no GRIB1 parameter code for 'Contrail engine type'";
		  exit(1);
		case 15:
		  myerror="there is no GRIB1 parameter code for 'Contrail top'";
		  exit(1);
		case 16:
		  myerror="there is no GRIB1 parameter code for 'Contrail base'";
		  exit(1);
		case 17:
		  myerror="there is no GRIB1 parameter code for 'Maximum snow albedo'";
		  exit(1);
		case 18:
		  myerror="there is no GRIB1 parameter code for 'Snow-free albedo'";
		  exit(1);
		case 204:
		  switch (center) {
		    case 7:
			return 209;
		  }
	    }
	    break;
	}
	break;
// hydrologic products
    case 1:
	switch (param_cat) {
// hydrology basic products
	  case 0:
	    switch (param_num) {
		case 192:
		  switch (center) {
		    case 7:
			return 234;
		  }
		case 193:
		  switch (center) {
		    case 7:
			return 235;
		  }
	    }
	  case 1:
	    switch (param_num) {
		case 192:
		  switch (center) {
		    case 7:
			return 194;
		  }
		case 193:
		  switch (center) {
		    case 7:
			return 195;
		  }
	    }
	    break;
	}
// land surface products
    case 2:
	switch (param_cat) {
// vegetation/biomass
	  case 0:
	    switch (param_num) {
		case 0:
		  return 81;
		case 1:
		  return 83;
		case 2:
		  return 85;
		case 3:
		  return 86;
		case 4:
		  return 87;
		case 5:
		  return 90;
		case 192:
		  switch (center) {
		    case 7:
			return 144;
		  }
		case 193:
		  switch (center) {
		    case 7:
			return 155;
		  }
		case 194:
		  switch (center) {
		    case 7:
			return 207;
		  }
		case 195:
		  switch (center) {
		    case 7:
			return 208;
		  }
		case 196:
		  switch (center) {
		    case 7:
			return 223;
		  }
		case 197:
		  switch (center) {
		    case 7:
			return 226;
		  }
		case 198:
		  switch (center) {
		    case 7:
			return 225;
		  }
		case 201:
		  switch (center) {
		    case 7:
			table=130;
			return 219;
		  }
		case 207:
		  switch (center) {
		    case 7:
			return 201;
		  }
	    }
	    break;
	  case 3:
// soil products
 	    switch (param_num) {
		case 203:
		  switch (center) {
		    case 7:
			table=130;
			return 220;
		  }
 	    }
	    break;
// fire weather
	  case 4:
	    switch (param_num) {
		case 2:
		  switch (center) {
		    case 7:
			table=129;
			return 250;
		  }
	    }
	    break;
	}
	break;
// oceanographic products
    case 10:
	switch (param_cat) {
// waves parameters
	  case 0:
	    switch (param_num) {
		case 0:
		  return 28;
		case 1:
		  return 29;
		case 2:
		  return 30;
		case 3:
		  return 100;
		case 4:
		  return 101;
		case 5:
		  return 102;
		case 6:
		  return 103;
		case 7:
		  return 104;
		case 8:
		  return 105;
		case 9:
		  return 106;
		case 10:
		  return 107;
		case 11:
		  return 108;
		case 12:
		  return 109;
		case 13:
		  return 110;
	    }
	    break;
// currents parameters
	  case 1:
	    switch (param_num) {
		case 0:
		  return 47;
		case 1:
		  return 48;
		case 2:
		  return 49;
		case 3:
		  return 50;
	    }
	    break;
// ice parameters
	  case 2:
	    switch (param_num) {
		case 0:
		  return 91;
		case 1:
		  return 92;
		case 2:
		  return 93;
		case 3:
		  return 94;
		case 4:
		  return 95;
		case 5:
		  return 96;
		case 6:
		  return 97;
		case 7:
		  return 98;
	    }
	    break;
// surface properties parameters
	  case 3:
	    switch (param_num) {
		case 0:
		  return 80;
		case 1:
		  return 82;
	    }
	    break;
// sub-surface properties parameters
	  case 4:
	    switch (param_num) {
		case 0:
		  return 69;
		case 1:
		  return 70;
		case 2:
		  return 68;
		case 3:
		  return 88;
	    }
	    break;
	}
	break;
  }
  myerror="there is no GRIB1 parameter code for discipline "+strutils::itos(disc)+", parameter category "+strutils::itos(param_cat)+", parameter number "+strutils::itos(param_num);
  exit(1);
}

void mapLevelData(const GRIB2Grid& grid,short& level_type,double& level1,double& level2,short center)
{
  if (grid.getSecondLevelType() != 255 && grid.getFirstLevelType() != grid.getSecondLevelType()) {
    myerror="unable to indicate a layer bounded by different level types "+strutils::itos(grid.getFirstLevelType())+" and "+strutils::itos(grid.getSecondLevelType())+" in GRIB1";
    exit(1);
  }
  level1=level2=0.;
  switch (grid.getFirstLevelType()) {
    case 1:
	level_type=1;
	break;
    case 2:
	level_type=2;
	break;
    case 3:
	level_type=3;
	break;
    case 4:
	level_type=4;
	break;
    case 5:
	level_type=5;
	break;
    case 6:
	level_type=6;
	break;
    case 7:
	level_type=7;
	break;
    case 8:
	level_type=8;
	break;
    case 9:
	level_type=9;
	break;
    case 20:
	level_type=20;
	break;
    case 100:
	if (grid.getSecondLevelType() == 255) {
	  level_type=100;
	  level1=grid.getFirstLevelValue();
	}
	else {
	  level_type=101;
	  level1=grid.getFirstLevelValue()/10.;
	  level2=grid.getSecondLevelValue()/10.;
	}
	break;
    case 101:
	level_type=102;
	break;
    case 102:
	if (grid.getSecondLevelType() == 255) {
	  level_type=103;
	  level1=grid.getFirstLevelValue();
	}
	else {
	  level_type=104;
	  level1=grid.getFirstLevelValue()/100.;
	  level2=grid.getSecondLevelValue()/100.;
	}
	break;
    case 103:
	if (grid.getSecondLevelType() == 255) {
	  level_type=105;
	  level1=grid.getFirstLevelValue();
	}
	else {
	  level_type=106;
	  level1=grid.getFirstLevelValue()/100.;
	  level2=grid.getSecondLevelValue()/100.;
	}
	break;
    case 104:
	if (grid.getSecondLevelType() == 255) {
	  level_type=107;
	  level1=grid.getFirstLevelValue()*10000.;
	}
	else {
	  level_type=108;
	  level1=grid.getFirstLevelValue()*100.;
	  level2=grid.getSecondLevelValue()*100.;
	}
	break;
    case 105:
	level1=grid.getFirstLevelValue();
	if (myequalf(grid.getSecondLevelValue(),255.))
	  level_type=109;
	else {
	  level_type=110;
	  level2=grid.getSecondLevelValue();
	}
	break;
    case 106:
	level1=grid.getFirstLevelValue()*100.;
	if (grid.getSecondLevelType() == 255)
	  level_type=111;
	else {
	  level_type=112;
	  level2=grid.getSecondLevelValue()*100.;
	}
	break;
    case 107:
	if (grid.getSecondLevelType() == 255) {
	  level_type=113;
	  level1=grid.getFirstLevelValue();
	}
	else {
	  level_type=114;
	  level1=475.-grid.getFirstLevelValue();
	  level2=475.-grid.getSecondLevelValue();
	}
	break;
    case 108:
	level1=grid.getFirstLevelValue();
	if (grid.getSecondLevelType() == 255)
	  level_type=115;
	else {
	  level_type=116;
	  level2=grid.getSecondLevelValue();
	}
	break;
    case 109:
	level_type=117;
	level1=grid.getFirstLevelValue();
	break;
    case 111:
	if (grid.getSecondLevelType() == 255) {
	  level_type=119;
	  level1=grid.getFirstLevelValue()*10000.;
	}
	else {
	  level_type=120;
	  level1=grid.getFirstLevelValue()*100.;
	  level2=grid.getSecondLevelValue()*100.;
	}
	break;
    case 117:
	myerror="there is no GRIB1 level code for 'Mixed layer depth'";
	exit(1);
    case 160:
	level_type=160;
	level1=grid.getFirstLevelValue();
	break;
    case 200:
	switch (center) {
	  case 7:
	    level_type=200;
	    break;
	}
	break;
  }
}

short mapStatisticalEndTime(const GRIB2Grid& grid)
{
  DateTime dt1=grid.getReferenceDateTime(),dt2=grid.getStatisticalProcessEndDateTime();

  switch (grid.getTimeUnits()) {
    case 0:
	return (dt2.getTime()/100 % 100)-(dt1.getTime()/100 % 100);
    case 1:
	 return (dt2.getTime()/10000-dt1.getTime()/10000);
    case 2:
	return (dt2.getDay()-dt1.getDay());
    case 3:
	return (dt2.getMonth()-dt1.getMonth());
    case 4:
	return (dt2.getYear()-dt1.getYear());
    default:
	myerror="unable to map end time with units "+strutils::itos(grid.getTimeUnits())+" to GRIB1";
	exit(1);
  }
}

void mapTimeRange(const GRIB2Grid& grid,short& p1,short& p2,short& t_range,short& nmean,short& nmean_missing,short center)
{
  std::vector<GRIB2Grid::StatisticalProcessRange> stat_process_ranges;
  size_t n;

  switch (grid.getProductType()) {
    case 0:
    case 1:
    case 2:
	t_range=0;
	p1=grid.getForecastTime();
	p2=0;
	nmean=nmean_missing=0;
	break;
    case 8:
    case 11:
    case 12:
	if (grid.getNumberOfStatisticalProcessRanges() > 1) {
	  myerror="unable to map multiple ("+strutils::itos(grid.getNumberOfStatisticalProcessRanges())+") statistical processes to GRIB1";
	  exit(1);
	}
	stat_process_ranges=grid.getStatisticalProcessRanges();
	for (n=0; n < stat_process_ranges.size(); n++) {
	  switch(stat_process_ranges[n].type) {
	    case 0:
	    case 1:
	    case 4:
		switch(stat_process_ranges[n].type) {
// average
		  case 0:
		    t_range=3;
		    break;
// accumulation
		  case 1:
		    t_range=4;
		    break;
// difference
		  case 4:
		    t_range=5;
		    break;
		}
		p1=grid.getForecastTime();
		p2=mapStatisticalEndTime(grid);
		if (stat_process_ranges[n].period_time_increment.value == 0)
		  nmean=0;
		else {
		  myerror="unable to map discrete processing to GRIB1";
		  exit(1);
		}
		break;
// maximum
	    case 2:
// minimum
	    case 3:
		t_range=2;
		p1=grid.getForecastTime();
		p2=mapStatisticalEndTime(grid);
		if (stat_process_ranges[n].period_time_increment.value == 0)
		  nmean=0;
		else {
		  myerror="unable to map discrete processing to GRIB1";
		  exit(1);
		}
		break;
	    default:
// patch for NCEP grids
		if (stat_process_ranges[n].type == 255 && center == 7) {
 		  if (grid.getDiscipline() == 0) {
		    if (grid.getParameterCategory() == 0) {
			switch (grid.getParameter()) {
			  case 4:
			  case 5:
			    t_range=2;
			    p1=grid.getForecastTime();
			    p2=mapStatisticalEndTime(grid);
			    if (stat_process_ranges[n].period_time_increment.value == 0)
				nmean=0;
			    else {
				myerror="unable to map discrete processing to GRIB1";
				exit(1);
			    }
			    break;
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
	nmean_missing=grid.getNumberMissingFromStatisticalProcess();
	break;
    default:
	myerror="unable to map time range for Product Definition Template "+strutils::itos(grid.getProductType())+" into GRIB1";
	exit(1);
  }
}

GRIBGrid& GRIBGrid::operator=(const GRIB2Grid& source)
{
  int n,m;

  grib=source.grib;
  grid=source.grid;
  if (source.ensdata.fcst_type.length() > 0) {
/*
    if (pds_supp != NULL)
	delete[] pds_supp;
    if (source.ensdata.ID == "ALL") {
	grib.lengths.pds_supp=ensdata.fcst_type.getLength()+1;
	pds_supp=new unsigned char[grib.lengths.pds_supp];
	memcpy(pds_supp,ensdata.fcst_type.toChar(),ensdata.fcst_type.getLength());
    }
    else {
	grib.lengths.pds_supp=ensdata.fcst_type.getLength()+2;
	pds_supp=new unsigned char[grib.lengths.pds_supp];
	memcpy(pds_supp,(ensdata.fcst_type+ensdata.ID).toChar(),ensdata.fcst_type.getLength()+1);
    }
    pds_supp[grib.lengths.pds_supp-1]=(unsigned char)ensdata.total_size;
    grib.lengths.pds=40+grib.lengths.pds_supp;
*/
  }
  grib.table=3;
  grib.grid_catalog_id=255;
  grid.param=mapParameterData(source.grid.src,source.getDiscipline(),source.getParameterCategory(),source.grid.param,grib.table);
  mapLevelData(source,grid.level1_type,grid.level1,grid.level2,source.grid.src);
  referenceDateTime=source.getReferenceDateTime();
  mapTimeRange(source,grib.p1,grib.p2,grib.t_range,grid.nmean,grib.nmean_missing,source.grid.src);
  referenceDateTime=source.referenceDateTime;
  switch (source.grid.grid_type) {
    case 0:
// lat-lon
	grid.grid_type=0;
	dim=source.dim;
	def=source.def;
	grib.rescomp=0;
	if (source.grib.rescomp == 0x20)
	  grib.rescomp|=0x80;
	if (source.getEarthShape() == 2)
	  grib.rescomp|=0x40;
	if ( (source.grib.rescomp&0x8) == 0x8)
	  grib.rescomp|=0x8;
	break;
    case 30:
// lambert-conformal
	grid.grid_type=3;
	dim=source.dim;
	def=source.def;
	grib.rescomp=0;
	if (source.grib.rescomp == 0x20)
	  grib.rescomp|=0x80;
	if (source.getEarthShape() == 2)
	  grib.rescomp|=0x40;
	if ( (source.grib.rescomp&0x8) == 0x8)
	  grib.rescomp|=0x8;
	break;
    default:
	myerror="unable to map Grid Definition Template "+strutils::itos(source.grid.grid_type)+" into GRIB1";
	exit(1);
  }
  if (source.bitmap.applies) {
    if (bitmap.capacity < dim.size) {
	if (bitmap.map != NULL) {
	  delete[] bitmap.map;
	  bitmap.map=NULL;
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
  for (n=0; n < dim.y; n++) {
    for (m=0; m < dim.x; m++)
	gridpoints[n][m]=source.gridpoints[n][m];
  }
  return *this;
}

bool GRIBGrid::isAccumulatedGrid() const
{
  switch (grib.t_range) {
    case 114:
    case 116:
    case 124:
	return true;
    default:
	return false;
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
    if (!myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01)
	scientific=true;
    if (def.type == Grid::gaussianLatitudeLongitudeType) {
	if (!fillGaussianLatitudes(gaus_lats,def.num_circles,(grib.scan_mode&0x40) != 0x40)) {
	  myerror="unable to get gaussian latitudes for "+strutils::itos(def.num_circles)+" circles";
	  exit(0);
	}
    }
    switch (def.type) {
	case Grid::latitudeLongitudeType:
	case Grid::gaussianLatitudeLongitudeType:
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
		    for (j=0; j < dim.y; j++) {
			if (def.type == Grid::latitudeLongitudeType) {
			  outs.precision(3);
			  outs << std::setw(7) << (def.slatitude-j*def.laincrement) << " | ";
			  outs.precision(2);
			}
			else if (def.type == Grid::gaussianLatitudeLongitudeType) {
			  outs.precision(3);
			  outs << std::setw(7) << getLatitudeAt(j,&gaus_lats) << " | ";
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
			  if (gridpoints[j][i] >= Grid::missingValue)
			    outs << "   MISSING";
			  else
			    outs << std::setw(10) << gridpoints[j][i];
			}
			if (scientific) {
			  outs.unsetf(std::ios::scientific);
			  outs.setf(std::ios::fixed);
			}
			outs.precision(2);
			outs << std::endl;
		    }
		    break;
		  case 0x40:
		    for (j=dim.y-1; j >= 0; j--) {
			if (def.type == Grid::latitudeLongitudeType) {
			  outs.precision(3);
			  outs << std::setw(7) << (def.elatitude-(dim.y-j-1)*def.laincrement) << " | ";
			  outs.precision(2);
			}
			else if (def.type == Grid::gaussianLatitudeLongitudeType) {
			  outs.precision(3);
			  outs << std::setw(7) << getLatitudeAt(j,&gaus_lats) << " | ";
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
			  if (gridpoints[j][i] >= Grid::missingValue)
			    outs << "   MISSING";
			  else
			    outs << std::setw(10) << gridpoints[j][i];
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
// reduced grid
	  else {
	    glat_entry.key=def.num_circles;
	    gaus_lats.found(glat_entry.key,glat_entry);
	    for (j=0; j < dim.y; j++) {
		std::cout << "\nLAT\\LON";
		for (i=1; i <= static_cast<int>(gridpoints[j][0]); ++i) {
		  std::cout << std::setw(10) << (def.elongitude-def.slongitude)/(gridpoints[j][0]-1.)*static_cast<float>(i-1);
		}
		std::cout << std::endl;
		std::cout << std::setw(7) << glat_entry.lats[j];
		for (i=1; i <= static_cast<int>(gridpoints[j][0]); ++i) {
		  std::cout << std::setw(10) << gridpoints[j][i];
		}
		std::cout << std::endl;
	    }
	  }
	  break;
	case Grid::polarStereographicType:
	case Grid::lambertConformalType:
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
		  for (j=0; j < dim.y; ++j) {
		    outs << std::setw(3) << dim.y-j << " | ";
		    for (k=i; k < max_i; ++k) {
			if (myequalf(gridpoints[j][k],Grid::missingValue)) {
			  outs << "   MISSING";
			}
			else {
			  if (scientific) {
			    outs.unsetf(std::ios::fixed);
			    outs.setf(std::ios::scientific);
			    outs.precision(3);
			    outs << std::setw(10) << gridpoints[j][k];
			    outs.unsetf(std::ios::scientific);
			    outs.setf(std::ios::fixed);
			    outs.precision(2);
			  }
			  else {
			    outs << std::setw(10) << gridpoints[j][k];
			  }
			}
		    }
		    outs << std::endl;
		  }
		  break;
		case 0x40:
		  for (j=dim.y-1; j >= 0; --j) {
		    outs << std::setw(3) << j+1 << " | ";
		    for (k=i; k < max_i; k++) {
			if (myequalf(gridpoints[j][k],Grid::missingValue)) {
			  outs << "   MISSING";
			}
			else {
			  if (scientific) {
			    outs.unsetf(std::ios::fixed);
			    outs.setf(std::ios::scientific);
			    outs.precision(3);
			    outs << std::setw(10) << gridpoints[j][k];
			    outs.unsetf(std::ios::scientific);
			    outs.setf(std::ios::fixed);
			    outs.precision(2);
			  }
			  else {
			    outs << std::setw(10) << gridpoints[j][k];
			  }
			}
		    }
		    outs << std::endl;
		  }
		  break;
	    }
	  }
	  break;
    }
    outs << std::endl;
  }
}

void GRIBGrid::print(std::ostream& outs,float bottom_latitude,float top_latitude,float west_longitude,float east_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void GRIBGrid::printASCII(std::ostream& outs) const
{
}

std::string GRIBGrid::buildLevelSearchKey() const
{
  return strutils::itos(grid.src)+"-"+strutils::itos(grib.sub_center);
}

std::string GRIBGrid::getFirstLevelDescription(xmlutils::LevelMapper& level_mapper) const
{
  short edition;

  if (grib.table < 0)
    edition=0;
  else
    edition=1;
  return level_mapper.getDescription("WMO_GRIB"+strutils::itos(edition),strutils::itos(grid.level1_type),buildLevelSearchKey());
}

std::string GRIBGrid::getFirstLevelUnits(xmlutils::LevelMapper& level_mapper) const
{
  short edition;

  if (grib.table < 0)
    edition=0;
  else
    edition=1;
  return level_mapper.getUnits("WMO_GRIB"+strutils::itos(edition),strutils::itos(grid.level1_type),buildLevelSearchKey());
}

std::string GRIBGrid::buildParameterSearchKey() const
{
  return strutils::itos(grid.src)+"-"+strutils::itos(grib.sub_center)+"."+strutils::itos(grib.table);
}

std::string GRIBGrid::getParameterCFKeyword(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;

  if (grib.table < 0)
    edition=0;
  else
    edition=1;
  return parameter_mapper.getCFKeyword("WMO_GRIB"+strutils::itos(edition),buildParameterSearchKey()+":"+strutils::itos(grid.param));
}

std::string GRIBGrid::getParameterDescription(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;

  if (grib.table < 0)
    edition=0;
  else
    edition=1;
  return parameter_mapper.getDescription("WMO_GRIB"+strutils::itos(edition),buildParameterSearchKey()+":"+strutils::itos(grid.param));
}

std::string GRIBGrid::getParameterShortName(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;

  if (grib.table < 0)
    edition=0;
  else
    edition=1;
  return parameter_mapper.getShortName("WMO_GRIB"+strutils::itos(edition),buildParameterSearchKey()+":"+strutils::itos(grid.param));
}

std::string GRIBGrid::getParameterUnits(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;

  if (grib.table < 0)
    edition=0;
  else
    edition=1;
  return parameter_mapper.getUnits("WMO_GRIB"+strutils::itos(edition),buildParameterSearchKey()+":"+strutils::itos(grid.param));
}

void GRIBGrid::printHeader(std::ostream& outs,bool verbose) const
{
  static xmlutils::ParameterMapper parameter_mapper;
  bool scientific=false;

  outs.setf(std::ios::fixed);
  outs.precision(2);
  if (!myequalf(stats.min_val,0.) && fabs(stats.min_val) < 0.01) {
    scientific=true;
  }
  if (verbose) {
    outs << "  Center: " << std::setw(3) << grid.src;
    if (grib.table >= 0) {
	outs << "-" << std::setw(2) << grib.sub_center << "  Table: " << std::setw(3) << grib.table;
    }
    outs << "  Process: " << std::setw(3) << grib.process << "  GridID: " << std::setw(3) << grib.grid_catalog_id << "  DataBits: " << std::setw(3) << grib.pack_width << std::endl;
    outs << "  RefTime: " << referenceDateTime.toString() << "  Fcst: " << std::setw(4) << std::setfill('0') << grid.fcst_time/100 << std::setfill(' ') << "  NumAvg: " << std::setw(3) << grid.nmean-grib.nmean_missing << "  TimeRng: " << std::setw(3) << grib.t_range << "  P1: " << std::setw(3) << grib.p1;
    if (grib.t_range != 10) {
	outs << " P2: " << std::setw(3) << grib.p2;
    }
    outs << " Units: ";
    if (grib.time_units < 13) {
	outs << timeUnits[grib.time_units];
    }
    else {
	outs << grib.time_units;
    }
    if (ensdata.ID.length() > 0) {
	outs << " Ensemble: " << ensdata.ID;
	if (ensdata.ID == "ALL" && ensdata.total_size > 0) {
	  outs << "/" << ensdata.total_size;
	}
	outs << "/" << ensdata.fcst_type;
    }
    outs << "  Param: " << std::setw(3) << grid.param << "(" << getParameterShortName(parameter_mapper) << ")";
    if (grid.level1_type < 100 || grid.level1_type == 102 || grid.level1_type == 200 || grid.level1_type == 201) {
	outs << "  Level: " << levelTypeShortName[grid.level1_type];
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
	    outs << "  Level: " << std::setw(5) << grid.level1 << levelTypeUnits[grid.level1_type] << " " << levelTypeShortName[grid.level1_type];
	    break;
	  case 107:
	    outs.precision(4);
	    outs << "  Level: " << std::setw(6) << grid.level1 << " " << levelTypeShortName[grid.level1_type];
	    outs.precision(2);
	    break;
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
	    outs << "  Layer- Top: " << std::setw(5) << grid.level1 << levelTypeUnits[grid.level1_type] << " Bottom: " << std::setw(5) << grid.level2 << levelTypeUnits[grid.level1_type] << " " << levelTypeShortName[grid.level1_type];
	    break;
	  case 108:
	    outs << "  Layer- Top: " << std::setw(5) << grid.level1 << " Bottom: " << std::setw(5) << grid.level2 << " " << levelTypeShortName[grid.level1_type];
	    break;
	  case 128:
	    outs.precision(3);
	    outs << "  Layer- Top: " << std::setw(5) << 1.1-grid.level1 << " Bottom: " << std::setw(5) << 1.1-grid.level2 << " " << levelTypeShortName[grid.level1_type];
	    outs.precision(2);
	    break;
	  default:
	    outs << "  Level/Layer: " << std::setw(3) << grid.level1_type << " " << std::setw(5) << grid.level1 << " " << std::setw(5) << grid.level2;
	}
    }
    switch (grid.grid_type) {
// Latitude/Longitude
	case 0:
	  outs.precision(3);
	  outs << "\n  Grid: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type << "  LonRange: " << std::setw(7) << def.slongitude << " to " << std::setw(3) << def.elongitude << " by " << std::setw(5) << def.loincrement << "  LatRange: " << std::setw(3) << def.slatitude << " to " << std::setw(7) << def.elatitude << " by " << std::setw(5) << def.laincrement << "  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
	  outs.precision(2);
	  break;
// Gaussian Lat/Lon
// Rotated Lat/Lon
	case 4:
	case 10:
	  outs.precision(3);
	  outs << "\n  Grid: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type << "  LonRange: " << std::setw(3) << def.slongitude << " to " << std::setw(7) << def.elongitude << " by " << std::setw(5) << def.loincrement << "  LatRange: " << std::setw(7) << def.slatitude << " to " << std::setw(7) << def.elatitude << "  Circles: " << std::setw(3) << def.num_circles << "  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
	  outs.precision(2);
	  break;
// Lambert Conformal
// Polar Stereographic
	case 3:
	case 5:
	  outs.precision(3);
	  outs << "\n  Grid: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type << "  Lat: " << std::setw(7) << def.slatitude << " Lon: " << std::setw(7) << def.slongitude << " OrientLon: " << std::setw(7) << def.olongitude << " Proj: 0x" << std::hex << def.projection_flag << std::dec << "  Dx: " << std::setw(7) << def.dx << "km Dy: " << std::setw(7) << def.dy << "km  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
	  if (grid.grid_type == 3) {
	    outs << std::endl;
	    outs << "    Standard Parallels: " << std::setw(7) << def.stdparallel1 << " " << std::setw(7) << def.stdparallel2 << "  (Lat,Lon) of Southern Pole: (" << std::setw(7) << grib.sp_lat << "," << std::setw(7) << grib.sp_lon << ")";
	  }
	  outs.precision(2);
	  break;
	case 50:
	  outs << "\n  Type: " << std::setw(3) << grid.grid_type << "  Truncation Parameters: " << std::setw(3) << def.trunc1 << std::setw(5) << def.trunc2 << std::setw(5) << def.trunc3;
	  break;
    }
    outs << std::endl;
    outs << "  Decimal Scale (D): " << std::setw(3) << grib.D << "  Scale (E): " << std::setw(3) << grib.E << "  RefVal: ";
    if (scientific) {
	outs.unsetf(std::ios::fixed);
	outs.setf(std::ios::scientific);
	outs.precision(3);
	outs << stats.min_val;
	if (grid.filled)
	  outs << " (" << stats.min_i << "," << stats.min_j << ")  MaxVal: " << stats.max_val << " (" << stats.max_i << "," << stats.max_j << ")  AvgVal: " << stats.avg_val;
	outs.unsetf(std::ios::scientific);
	outs.setf(std::ios::fixed);
	outs.precision(2);
    }
    else {
	outs << std::setw(9) << stats.min_val;
	if (grid.filled)
	  outs << " (" << stats.min_i << "," << stats.min_j << ")  MaxVal: " << std::setw(9) << stats.max_val << " (" << stats.max_i << "," << stats.max_j << ")  AvgVal: " << std::setw(9) << stats.avg_val;
    }
    outs << std::endl;
  }
  else {
    if (grib.table < 0)
	outs << " Ctr=" << grid.src;
    else
	outs << " Tbl=" << grib.table << " Ctr=" << grid.src << "-" << grib.sub_center;
    outs << " ID=" << grib.grid_catalog_id << " RTime=" << referenceDateTime.toString("%Y%m%d%H%MM") << " Fcst=" << std::setw(4) << std::setfill('0') << grid.fcst_time/100 << std::setfill(' ') << " NAvg=" << grid.nmean-grib.nmean_missing << " TRng=" << grib.t_range << " P1=" << grib.p1;
    if (grib.t_range != 10)
	outs << " P2=" << grib.p2;
    outs << " Units=";
    if (grib.time_units < 13)
	outs << timeUnits[grib.time_units];
    else
	outs << grib.time_units;
    if (ensdata.ID.length() > 0) {
	outs << " Ens=" << ensdata.ID;
	if (ensdata.ID == "ALL" && ensdata.total_size > 0)
	  outs << "/" << ensdata.total_size;
	outs << "/" << ensdata.fcst_type;
    }
    outs << " Param=" << grid.param;
    if (grid.level1_type < 100 || grid.level1_type == 102 || grid.level1_type == 200 || grid.level1_type == 201) {
	outs << " Level=" << levelTypeShortName[grid.level1_type];
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
	    outs << " Level=" << grid.level1 << levelTypeUnits[grid.level1_type];
	    break;
	  case 107:
	    outs.precision(4);
	    outs << " Level=" << grid.level1;
	    outs.precision(2);
	    break;
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
	    outs << " Layer=" << grid.level1 << levelTypeUnits[grid.level1_type] << "," << grid.level2 << levelTypeUnits[grid.level1_type];
	    break;
	  case 108:
	    outs << " Layer=" << grid.level1 << "," << grid.level2;
	    break;
	  case 128:
	    outs.precision(3);
	    outs << " Layer=" << 1.1-grid.level1 << "," << 1.1-grid.level2;
	    outs.precision(2);
	    break;
	  default:
	    outs << " Levels=" << grid.level1_type << "," << grid.level1 << "," << grid.level2;
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

void GRIBGrid::reverseScan()
{
  float temp;
  int n,m;
  size_t l,bcnt;

  switch (grib.scan_mode) {
    case 0x0:
    case 0x40:
	temp=def.slatitude;
	def.slatitude=def.elatitude;
	def.elatitude=temp;
	grib.scan_mode=(0x40-grib.scan_mode);
	for (n=0; n < dim.y/2; ++n) {
	  for (m=0; m < dim.x; ++m) {
	    temp=gridpoints[n][m];
	    l=dim.y-n-1;
	    gridpoints[n][m]=gridpoints[l][m];
	    gridpoints[l][m]=temp;
	  }
	}
	if (bitmap.map != NULL) {
	  bcnt=0;
	  for (n=0; n < dim.y; ++n) {
	    for (m=0; m < dim.x; ++m) {
		if (myequalf(gridpoints[n][m],Grid::missingValue)) {
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

GRIBGrid fabs(const GRIBGrid& source)
{
  GRIBGrid fabs_grid;
  int n,m;
  size_t avg_cnt=0;

  fabs_grid=source;
  if (fabs_grid.grib.capacity.points == 0)
    return fabs_grid;

  fabs_grid.grid.pole=::fabs(fabs_grid.grid.pole);

  fabs_grid.stats.max_val=-Grid::missingValue;
  fabs_grid.stats.min_val=Grid::missingValue;
  fabs_grid.stats.avg_val=0.;
  for (n=0; n < fabs_grid.dim.y; n++) {
    for (m=0; m < fabs_grid.dim.x; m++) {
	if (!myequalf(fabs_grid.gridpoints[n][m],Grid::missingValue)) {
	  fabs_grid.gridpoints[n][m]=::fabs(fabs_grid.gridpoints[n][m]);
	  if (fabs_grid.gridpoints[n][m] > fabs_grid.stats.max_val)
	    fabs_grid.stats.max_val=fabs_grid.gridpoints[n][m];
	  if (fabs_grid.gridpoints[n][m] < fabs_grid.stats.min_val)
	    fabs_grid.stats.min_val=fabs_grid.gridpoints[n][m];
	  fabs_grid.stats.avg_val+=fabs_grid.gridpoints[n][m];
	  avg_cnt++;
	}
    }
  }

  if (avg_cnt > 0)
    fabs_grid.stats.avg_val/=static_cast<double>(avg_cnt);

  return fabs_grid;
}

GRIBGrid interpGaussToLatLon(const GRIBGrid& source)
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
  if (!fillGaussianLatitudes(gaus_lats,source.def.num_circles,(source.grib.scan_mode&0x40) != 0x40)) {
    myerror="unable to get gaussian latitudes for "+strutils::itos(source.def.num_circles)+" circles";
    exit(0);
  }
  grib_data.referenceDateTime=source.referenceDateTime;
  grib_data.validDateTime=source.validDateTime;
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
  grib_data.time_units=source.grib.time_units;
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
  for (n=0; n < grib_data.dim.y; n++)
    grib_data.gridpoints[n]=new double[grib_data.dim.x];
  for (n=0; n < grib_data.dim.y; n++) {
    lat= (grib_data.scan_mode == 0x0) ? grib_data.def.slatitude-n*grib_data.def.laincrement : grib_data.def.slatitude+n*grib_data.def.laincrement;
    ni=source.getLatitudeIndexNorthOf(lat,&gaus_lats);
    si=source.getLatitudeIndexSouthOf(lat,&gaus_lats);
    for (m=0; m < grib_data.dim.x; m++) {
	if (ni >= 0 && si >= 0) {
	  dyt=source.getLatitudeAt(ni,&gaus_lats)-source.getLatitudeAt(si,&gaus_lats);
	  dya=dyt-(source.getLatitudeAt(ni,&gaus_lats)-lat);
	  dyb=dyt-(lat-source.getLatitudeAt(si,&gaus_lats));
	  grib_data.gridpoints[n][m]=(source.getGridpoint(m,ni)*dya+source.getGridpoint(m,si)*dyb)/dyt;
	}
	else {
grib_data.gridpoints[n][m]=Grid::missingValue;
	}
    }
  }
  latlon_grid.fillFromGRIBData(grib_data);
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
	if (!myequalf(pow_grid.gridpoints[n][m],Grid::missingValue)) {
	  pow_grid.gridpoints[n][m]=::pow(pow_grid.gridpoints[n][m],exponent);
	  pow_grid.stats.avg_val+=pow_grid.gridpoints[n][m];
	  avg_cnt++;
	}
    }
  }

  if (avg_cnt > 0)
    pow_grid.stats.avg_val/=static_cast<double>(avg_cnt);

  return pow_grid;
}

GRIBGrid sqrt(const GRIBGrid& source)
{
  GRIBGrid sqrt_grid;
  int n,m;
  size_t avg_cnt=0;

  sqrt_grid=source;
  if (sqrt_grid.grib.capacity.points == 0)
    return sqrt_grid;

  sqrt_grid.grid.pole=::sqrt(sqrt_grid.grid.pole);

  sqrt_grid.stats.max_val=::sqrt(sqrt_grid.stats.max_val);
  sqrt_grid.stats.min_val=::sqrt(sqrt_grid.stats.min_val);
  sqrt_grid.stats.avg_val=0.;
  for (n=0; n < sqrt_grid.dim.y; n++) {
    for (m=0; m < sqrt_grid.dim.x; m++) {
	if (!myequalf(sqrt_grid.gridpoints[n][m],Grid::missingValue)) {
	  sqrt_grid.gridpoints[n][m]=::sqrt(sqrt_grid.gridpoints[n][m]);
	  sqrt_grid.stats.avg_val+=sqrt_grid.gridpoints[n][m];
	  avg_cnt++;
	}
    }
  }

  if (avg_cnt > 0)
    sqrt_grid.stats.avg_val/=static_cast<double>(avg_cnt);

  return sqrt_grid;
}

GRIBGrid combineHemispheres(const GRIBGrid& nhgrid,const GRIBGrid& shgrid)
{
  GRIBGrid combined;
  const GRIBGrid *addon;
  int n,m,dim_y;
  size_t l;

  if (nhgrid.getReferenceDateTime() != shgrid.getReferenceDateTime())
    return combined;
  if (nhgrid.dim.x != shgrid.dim.x || nhgrid.dim.y != shgrid.dim.y)
    return combined;
  if (nhgrid.bitmap.map != NULL || shgrid.bitmap.map != NULL) {
myerror="no handling of missing data yet";
exit(1);
  }

  combined.grib.capacity.points=nhgrid.dim.size*2-nhgrid.dim.x;
  dim_y=nhgrid.dim.y*2-1;
  combined.dim.x=nhgrid.dim.x;
  combined.gridpoints=new double *[dim_y];
  for (n=0; n < dim_y; n++)
    combined.gridpoints[n]=new double[combined.dim.x];
  switch (nhgrid.grib.scan_mode) {
    case 0x0:
	combined=nhgrid;
	combined.def.elatitude=shgrid.def.elatitude;
	addon=&shgrid;
	if (shgrid.stats.max_val > combined.stats.max_val)
	  combined.stats.max_val=shgrid.stats.max_val;
	if (shgrid.stats.min_val < combined.stats.min_val)
	  combined.stats.min_val=shgrid.stats.min_val;
	break;
    case 0x40:
	combined=shgrid;
	combined.def.elatitude=nhgrid.def.elatitude;
	addon=&nhgrid;
	if (nhgrid.stats.max_val > combined.stats.max_val)
	  combined.stats.max_val=nhgrid.stats.max_val;
	if (nhgrid.stats.min_val < combined.stats.min_val)
	  combined.stats.min_val=nhgrid.stats.min_val;
	break;
    default:
	return combined;
  }
  combined.grib.capacity.y=combined.dim.y=dim_y;
  l=0;
  for (n=combined.dim.y/2; n < combined.dim.y; n++) {
    for (m=0; m < combined.dim.x; m++)
	combined.gridpoints[n][m]=addon->gridpoints[l][m];
    l++;
  }
combined.grid.num_missing=0;
  combined.grib.D= (nhgrid.grib.D > shgrid.grib.D) ? nhgrid.grib.D : shgrid.grib.D;
  combined.setScaleAndPackingWidth(combined.grib.D,combined.grib.pack_width);
  return combined;
}

GRIBGrid createSubsetGrid(const GRIBGrid& source,float bottom_latitude,float top_latitude,float left_longitude,float right_longitude)
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
  subset_grid.referenceDateTime=source.referenceDateTime;
  subset_grid.validDateTime=source.validDateTime;
  subset_grid.grib.scan_mode=source.grib.scan_mode;
  subset_grid.def.laincrement=source.def.laincrement;
  subset_grid.def.type=source.def.type;
  if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
    if (!fillGaussianLatitudes(gaus_lats,subset_grid.def.num_circles,(subset_grid.grib.scan_mode&0x40) != 0x40)) {
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
// top down
    case 0x0:
	if (!myequalf(source.getGridpointAt(bottom_latitude,source.def.slongitude),Grid::badValue)) {
	  subset_grid.def.elatitude=bottom_latitude;
	}
	else {
	  if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
	    subset_grid.def.elatitude=glat_entry.lats[source.getLatitudeIndexSouthOf(bottom_latitude,&gaus_lats)];
	  }
	  else {
	    subset_grid.def.elatitude=source.def.slatitude-source.getLatitudeIndexSouthOf(bottom_latitude,&gaus_lats)*subset_grid.def.laincrement;
	  }
	}
	if (!myequalf(source.getGridpointAt(top_latitude,source.def.slongitude),Grid::badValue)) {
	  subset_grid.def.slatitude=top_latitude;
	}
	else {
	  if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
	    subset_grid.def.slatitude=glat_entry.lats[source.getLatitudeIndexNorthOf(top_latitude,&gaus_lats)];
	  }
	  else {
	    subset_grid.def.slatitude=source.def.slatitude-source.getLatitudeIndexNorthOf(top_latitude,&gaus_lats)*subset_grid.def.laincrement;
	  }
	}
	if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
	  subset_grid.dim.y=0;
	  for (n=0; n < static_cast<int>(subset_grid.def.num_circles*2); ++n) {
	    if ((glat_entry.lats[n] > subset_grid.def.elatitude && glat_entry.lats[n] < subset_grid.def.slatitude) || myequalf(glat_entry.lats[n],subset_grid.def.elatitude) || myequalf(glat_entry.lats[n],subset_grid.def.slatitude)) {
		subset_grid.dim.y++;
	    }
	  }
	}
	else {
	  subset_grid.dim.y=lroundf((subset_grid.def.slatitude-subset_grid.def.elatitude)/subset_grid.def.laincrement)+1;
	}
	break;
// bottom up
    case 0x40:
	if (!myequalf(source.getGridpointAt(bottom_latitude,source.def.slongitude),Grid::badValue)) {
	  subset_grid.def.slatitude=bottom_latitude;
	}
	else {
	  if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
	    subset_grid.def.slatitude=glat_entry.lats[source.getLatitudeIndexSouthOf(bottom_latitude,&gaus_lats)];
	  }
	  else {
	    if (bottom_latitude < source.def.slatitude) {
		subset_grid.def.slatitude=source.def.slatitude;
	    }
	    else {
		subset_grid.def.slatitude=source.def.slatitude+source.getLatitudeIndexSouthOf(bottom_latitude,&gaus_lats)*subset_grid.def.laincrement;
	    }
	  }
	}
	if (!myequalf(source.getGridpointAt(top_latitude,source.def.slongitude),Grid::badValue)) {
	  subset_grid.def.elatitude=top_latitude;
	}
	else {
	  if (subset_grid.def.type == Grid::gaussianLatitudeLongitudeType) {
	    subset_grid.def.elatitude=glat_entry.lats[source.getLatitudeIndexNorthOf(top_latitude,&gaus_lats)];
	  }
	  else {
	    if (top_latitude > source.def.elatitude) {
		subset_grid.def.elatitude=source.def.elatitude;
	    }
	    else {
		subset_grid.def.elatitude=source.def.slatitude+source.getLatitudeIndexNorthOf(top_latitude,&gaus_lats)*subset_grid.def.laincrement;
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
  if ( (lat_index=source.getLatitudeIndexOf(subset_grid.def.slatitude,&gaus_lats)) >= static_cast<size_t>(source.dim.y)) {
    subset_grid.grid.filled=false;
    return subset_grid;
  }
  subset_grid.def.loincrement=source.def.loincrement;
  if (!myequalf(source.getGridpointAt(source.def.slatitude,left_longitude),Grid::badValue)) {
    subset_grid.def.slongitude=left_longitude;
  }
  else {
    subset_grid.def.slongitude=subset_grid.def.loincrement*source.getLongitudeIndexWestOf(left_longitude);
  }
  if (!myequalf(source.getGridpointAt(source.def.slatitude,right_longitude),Grid::badValue)) {
    subset_grid.def.elongitude=right_longitude;
  }
  else {
    subset_grid.def.elongitude=subset_grid.def.loincrement*source.getLongitudeIndexEastOf(right_longitude);
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
    if (subset_grid.bitmap.map != NULL) {
	delete[] subset_grid.bitmap.map;
    }
    subset_grid.bitmap.capacity=subset_grid.dim.size;
    subset_grid.bitmap.map=new unsigned char[subset_grid.bitmap.capacity];
  }
  subset_grid.grid.num_missing=0;
  subset_grid.galloc();
  subset_grid.stats.max_val=-Grid::missingValue;
  subset_grid.stats.min_val=Grid::missingValue;
  subset_grid.stats.avg_val=0.;
  for (n=0; n < subset_grid.dim.y; ++n) {
    for (m=0; m < subset_grid.dim.x; ++m) {
	lon_index=source.getLongitudeIndexOf(subset_grid.def.slongitude+m*subset_grid.def.loincrement);
	subset_grid.gridpoints[n][m]=source.getGridpoint(lon_index,lat_index);
	if (myequalf(subset_grid.gridpoints[n][m],Grid::missingValue)) {
	  subset_grid.bitmap.map[cnt]=0;
	  ++subset_grid.grid.num_missing;
	}
	else {
	  subset_grid.bitmap.map[cnt]=1;
	  if (subset_grid.gridpoints[n][m] > subset_grid.stats.max_val) {
	    subset_grid.stats.max_val=subset_grid.gridpoints[n][m];
	    subset_grid.stats.max_i=m;
	    subset_grid.stats.max_j=n;
	  }
	  if (subset_grid.gridpoints[n][m] < subset_grid.stats.min_val) {
	    subset_grid.stats.min_val=subset_grid.gridpoints[n][m];
	    subset_grid.stats.min_i=m;
	    subset_grid.stats.min_j=n;
	  }
	  subset_grid.stats.avg_val+=subset_grid.gridpoints[n][m];
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
  subset_grid.setScaleAndPackingWidth(source.grib.D,subset_grid.grib.pack_width);
  subset_grid.grid.filled=true;
  return subset_grid;
}

GRIB2Grid& GRIB2Grid::operator=(const GRIB2Grid& source)
{
  GRIBGrid::operator=(static_cast<GRIBGrid>(source));
  grib2=source.grib2;
  return *this;
}

void GRIB2Grid::quickCopy(const GRIB2Grid& source)
{
  size_t n;

  if (this == &source)
    return;
  referenceDateTime=source.referenceDateTime;
  validDateTime=source.validDateTime;
  dim=source.dim;
  def=source.def;
  stats=source.stats;
  ensdata=source.ensdata;
  grid=source.grid;
  if (source.plist != NULL) {
    if (plist != NULL)
	delete[] plist;
    plist=new int[dim.y];
    for (n=0; n < static_cast<size_t>(dim.y); n++)
	plist[n]=source.plist[n];
  }
  else {
    if (plist != NULL)
	delete[] plist;
    plist=NULL;
  }
  if (source.bitmap.applies) {
    if (bitmap.capacity < source.bitmap.capacity) {
	if (bitmap.map != NULL) {
	  delete[] bitmap.map;
	  bitmap.map=NULL;
	}
	bitmap.capacity=source.bitmap.capacity;
	bitmap.map=new unsigned char[bitmap.capacity];
	for (n=0; n < dim.size; ++n) {
	  bitmap.map[n]=source.bitmap.map[n];
	}
    }
  }
  bitmap.applies=source.bitmap.applies;
  grib=source.grib;
  grib2=source.grib2;
}

std::string GRIB2Grid::buildLevelSearchKey() const
{
  return strutils::itos(grid.src)+"-"+strutils::itos(grib.sub_center);
}

std::string GRIB2Grid::getFirstLevelDescription(xmlutils::LevelMapper& level_mapper) const
{
  return level_mapper.getDescription("WMO_GRIB2",strutils::itos(grid.level1_type),buildLevelSearchKey());
}

std::string GRIB2Grid::getFirstLevelUnits(xmlutils::LevelMapper& level_mapper) const
{
  return level_mapper.getUnits("WMO_GRIB2",strutils::itos(grid.level1_type),buildLevelSearchKey());
}

std::string GRIB2Grid::buildParameterSearchKey() const
{
  return strutils::itos(grid.src)+"-"+strutils::itos(grib.sub_center)+"."+strutils::itos(grib.table)+"-"+strutils::itos(grib2.local_table);
}

std::string GRIB2Grid::getParameterCFKeyword(xmlutils::ParameterMapper& parameter_mapper) const
{
  std::string level_type;

  level_type=strutils::itos(grid.level1_type);
  if (grid.level2_type > 0 && grid.level2_type < 255)
    level_type+="-"+strutils::itos(grid.level2_type);
  return parameter_mapper.getCFKeyword("WMO_GRIB2",buildParameterSearchKey()+":"+strutils::itos(grib2.discipline)+"."+strutils::itos(grib2.param_cat)+"."+strutils::itos(grid.param),level_type);
}

std::string GRIB2Grid::getParameterDescription(xmlutils::ParameterMapper& parameter_mapper) const
{
  return parameter_mapper.getDescription("WMO_GRIB2",buildParameterSearchKey()+":"+strutils::itos(grib2.discipline)+"."+strutils::itos(grib2.param_cat)+"."+strutils::itos(grid.param));
}

std::string GRIB2Grid::getParameterShortName(xmlutils::ParameterMapper& parameter_mapper) const
{
  return parameter_mapper.getShortName("WMO_GRIB2",buildParameterSearchKey()+":"+strutils::itos(grib2.discipline)+"."+strutils::itos(grib2.param_cat)+"."+strutils::itos(grid.param));
}

std::string GRIB2Grid::getParameterUnits(xmlutils::ParameterMapper& parameter_mapper) const
{
  return parameter_mapper.getUnits("WMO_GRIB2",buildParameterSearchKey()+":"+strutils::itos(grib2.discipline)+"."+strutils::itos(grib2.param_cat)+"."+strutils::itos(grid.param));
}

void GRIB2Grid::computeMean()
{
//  double cnt;

  if (grib2.stat_process_ranges.size() > 0 && grib2.stat_process_ranges[0].type == 1) {
/*
    cnt=(grib2.stat_process_ranges[0].period_length.value+grib2.stat_process_ranges[0].period_time_increment.value)/grib2.stat_process_ranges[0].period_time_increment.value;
    this->divideBy(cnt);
*/
this->divideBy(grid.nmean);
    grib2.stat_process_ranges[0].type=0;
  }
  else {
    myerror="unable to compute the mean";
    exit(1);
  }
}

GRIB2Grid GRIB2Grid::createSubset(double south_latitude,double north_latitude,int latitude_stride,double west_longitude,double east_longitude,int longitude_stride) const
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
  if (latitude_stride < 1)
    latitude_stride=1;
  if (longitude_stride < 1)
    longitude_stride=1;
  switch (def.type) {
    case Grid::latitudeLongitudeType:
    case Grid::gaussianLatitudeLongitudeType:
	dval=(def.elongitude-def.slongitude)/(dim.x-1);
// special case of -180 to +180
	if (static_cast<int>(west_longitude) == -180 && static_cast<int>(east_longitude) == 180)
	  east_longitude-=dval;
	if (def.slongitude >= 0. && def.elongitude >= 0.) {
	  if (west_longitude < 0.)
	    west_longitude+=360.;
	  if (east_longitude < 0.)
	    east_longitude+=360.;
	}
	for (m=0; m < dim.x; m++) {
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
	if (end_x < 0)
	  end_x=dim.x-1;
	switch (def.type) {
	  case Grid::latitudeLongitudeType:
	    dval=(def.slatitude-def.elatitude)/(dim.y-1);
	    for (n=0; n < dim.y; n++) {
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
	    if (end_y < 0)
		end_y=dim.y-1;
	    break;
	  case Grid::gaussianLatitudeLongitudeType:
	    if (!fillGaussianLatitudes(gaus_lats,def.num_circles,(grib.scan_mode&0x40) != 0x40)) {
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
	    if (end_y < 0)
		end_y=n-1;
	    break;
	}
	if (end_x < start_x || (end_x == start_x && wlon == -(east_longitude))) {
	  crosses_greenwich=true;
	  new_grid.dim.x=end_x+(dim.x-start_x)+1;
	  new_grid.def.slongitude-=360.;
	}
	else
	  new_grid.dim.x=end_x-start_x+1;
	new_grid.dim.y=end_y-start_y+1;
	new_grid.dim.size=new_grid.dim.x*new_grid.dim.y;
	new_grid.stats.min_val=Grid::missingValue;
	new_grid.stats.max_val=-Grid::missingValue;
	x=0;
	new_grid.grid.num_missing=0;
	for (n=start_y; n <= end_y; ++n) {
	  if (crosses_greenwich)
	    ex=dim.x-1;
	  else
	    ex=end_x;
	  for (m=start_x; m <= ex; ++m) {
	    if (new_grid.bitmap.capacity > 0) {
		new_grid.bitmap.map[x]=bitmap.map[n*dim.x+m];
		if (new_grid.bitmap.map[x] == 0) {
		  ++new_grid.grid.num_missing;
		}
		++x;
	    }
	    new_grid.gridpoints[n-start_y][m-start_x]=gridpoints[n][m];
	    if (gridpoints[n][m] < new_grid.stats.min_val)
		new_grid.stats.min_val=gridpoints[n][m];
	    if (gridpoints[n][m] > new_grid.stats.max_val)
		new_grid.stats.max_val=gridpoints[n][m];
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
		new_grid.gridpoints[n-start_y][m+dim.x-start_x]=gridpoints[n][m];
		if (gridpoints[n][m] < new_grid.stats.min_val)
		  new_grid.stats.min_val=gridpoints[n][m];
		if (gridpoints[n][m] > new_grid.stats.max_val)
		  new_grid.stats.max_val=gridpoints[n][m];
	    }
	  }
	}
	break;
    default:
	myerror="unable to create a subset for grid type "+strutils::itos(def.type);
	exit(1);
  }
  return new_grid;
}

void GRIB2Grid::printHeader(std::ostream& outs,bool verbose) const
{
  size_t m;

  outs.setf(std::ios::fixed);
  outs.precision(2);
  if (verbose) {
    outs << "  Center: " << std::setw(3) << grid.src << "-" << std::setw(2) << grib.sub_center << "  Tables: " << std::setw(3) << grib.table << "." << std::setw(2) << grib2.local_table << "  RefTime: " << referenceDateTime.toString() << "  TimeSig: " << grib2.time_sig << "  DRS Template: " << grib2.data_rep << "    Valid Time: ";
    if (grib2.stat_period_end.getYear() != 0)
	outs << referenceDateTime.toString() << " to " << grib2.stat_period_end.toString();
    else
	outs << validDateTime.toString();
    outs << "  Dimensions: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type;
    switch (grid.grid_type) {
	case 0:
	case 40:
	  outs.precision(6);
	  outs << "  LonRange: " << std::setw(11) << def.slongitude << " to " << std::setw(11) << def.elongitude << " by " << std::setw(8) << def.loincrement << "  LatRange: " << std::setw(10) << def.slatitude << " to " << std::setw(10) << def.elatitude;
	  switch (grid.grid_type) {
	    case 0:
		outs << " by " << std::setw(8) << def.laincrement;
		break;
	    case 40:
		outs << " Circles: " << std::setw(4) << def.num_circles;
		break;
	  }
	  outs << "  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
	  outs.precision(2);
	  break;
    }
    outs << "  Param: " << std::setw(3) << grib2.discipline << "/" << std::setw(3) << grib2.param_cat << "/" << std::setw(3) << grid.param;
    switch (grib2.product_type) {
	case 0:
	case 1:
	case 8:
	  outs << "  Generating Process: " << grib.process << "/" << grib2.backgen_process << "/" << grib2.fcstgen_process;
//	  if (!myequalf(grid.level2,Grid::missingValue))
	  if (grid.level2_type != 255) {
	    outs << "  Levels: " << grid.level1_type;
	    if (!myequalf(grid.level1,Grid::missingValue))
		outs << "/" << grid.level1;
	    outs << "," << grid.level2_type;
	    if (!myequalf(grid.level2,Grid::missingValue))
		outs << "/" << grid.level2;
	  }
	  else {
	    outs << "  Level: " << grid.level1_type;
	    if (!myequalf(grid.level1,Grid::missingValue))
		outs << "/" << grid.level1;
	  }
	  switch (grib2.product_type) {
	    case 1:
		outs << "  Ensemble Type/ID/Size: " << ensdata.fcst_type << "/" << ensdata.ID << "/" << ensdata.total_size;
		break;
	    case 8:
		outs << "  Outermost Statistical Process: " << grib2.stat_process_ranges[0].type;
		break;
	  }
	  break;
	default:
	  outs << "  Product: " << grib2.product_type << "?";
    }
    outs << "  Decimal Scale (D): " << std::setw(3) << grib.D << "  Binary Scale (E): " << std::setw(3) << grib.E << "  Pack Width: " << std::setw(2) << grib.pack_width;
    outs << std::endl;
  }
  else {
    outs << "|PDef=" << grib2.product_type << "|Ctr=" << grid.src << "-" << grib.sub_center << "|Tbls=" << grib.table << "." << grib2.local_table << "|RTime=" << grib2.time_sig << ":" << referenceDateTime.toString("%Y%m%d%H%MM%SS") << "|Fcst=" << grid.fcst_time << "|GDef=" << grid.grid_type << "," << dim.x << "," << dim.y << ",";
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
    outs << "|VTime=" << forecastDateTime.toString("%Y%m%d%H%MM%SS");
    if (validDateTime != forecastDateTime) {
	outs << ":" << validDateTime.toString("%Y%m%d%H%MM%SS");
    }
    for (m=0; m < grib2.stat_process_ranges.size(); m++) {
	if (grib2.stat_process_ranges[m].type >= 0) {
	  outs << "|SP" << m << "=" << grib2.stat_process_ranges[m].type << "." << grib2.stat_process_ranges[m].time_increment_type << "." << grib2.stat_process_ranges[m].period_length.unit << "." << grib2.stat_process_ranges[m].period_length.value << "." << grib2.stat_process_ranges[m].period_time_increment.unit << "." << grib2.stat_process_ranges[m].period_time_increment.value;
	}
    }
    if (ensdata.fcst_type.length() > 0) {
	outs << "|E=" << ensdata.fcst_type << "." << ensdata.ID << "." << ensdata.total_size;
	if (grib2.modelv_date.getYear() != 0) {
	  outs << "|Mver=" << grib2.modelv_date.toString("%Y%m%d%H%MM%SS");
	}
    }
    if (grib2.spatial_process.type >= 0) {
	outs << "|Spatial=" << grib2.spatial_process.stat_process << "," << grib2.spatial_process.type << "," << grib2.spatial_process.num_points;
    }
    outs << "|P=" << grib2.discipline << "." << grib2.param_cat << "." << grid.param << "|L=" << grid.level1_type;
    if (!myequalf(grid.level1,Grid::missingValue))
	outs << ":" << grid.level1;
    if (grid.level2_type < 255) {
	outs << "," << grid.level2_type;
	if (!myequalf(grid.level2,Grid::missingValue))
	  outs << ":" << grid.level2;
    }
    outs << "|DDef=" << grib2.data_rep;
    if (!myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
	outs.unsetf(std::ios::fixed);
	outs.setf(std::ios::scientific);
	outs.precision(3);
	outs << "|R=" << stats.min_val;
	if (grid.filled)
	  outs << "|Max=" << stats.max_val << "|Avg=" << stats.avg_val;
	outs.unsetf(std::ios::scientific);
	outs.setf(std::ios::fixed);
	outs.precision(2);
    }
    else {
	outs << "|R=" << stats.min_val;
	if (grid.filled)
	  outs << "|Max=" << stats.max_val << "|Avg=" << stats.avg_val;
    }
    outs << std::endl;
  }
}

void GRIB2Grid::setStatisticalProcessRanges(const std::vector<StatisticalProcessRange>& source)
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
	stats.max_val=-Grid::missingValue;
	stats.min_val=Grid::missingValue;
	stats.avg_val=0.;
	for (n=0; n < dim.y; n++) {
	  for (m=0; m < dim.x; m++) {
	    l= (source.grib.scan_mode == grib.scan_mode) ? n : dim.y-n-1;
	    if (!myequalf(gridpoints[n][m],Grid::missingValue) && !myequalf(source.gridpoints[l][m],Grid::missingValue)) {
		gridpoints[n][m]+=(source.gridpoints[l][m]);
		bitmap.map[cnt++]=1;
		if (gridpoints[n][m] > stats.max_val)
		  stats.max_val=gridpoints[n][m];
		if (gridpoints[n][m] < stats.min_val)
		  stats.min_val=gridpoints[n][m];
		stats.avg_val+=gridpoints[n][m];
		avg_cnt++;
	    }
	    else {
		gridpoints[n][m]=(Grid::missingValue);
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
	if (avg_cnt > 0)
	  stats.avg_val/=static_cast<double>(avg_cnt);
	grid.nmean++;
	if (grid.nmean == 2) {
	  bool can_add=false;
	  if (grib2.stat_process_ranges.size() > 0) {
	    if (grib2.stat_process_ranges.size() == 1) {
		srange=grib2.stat_process_ranges[0];
		grib2.stat_process_ranges.push_back(srange);
		can_add=true;
	    }
	    else {
std::cerr << "A " << source.grid.src << " " << grib2.stat_process_ranges.size() << " " << grib2.stat_process_ranges[0].type << std::endl;
		if (source.grid.src == 7) {
		  switch (source.grib2.stat_process_ranges[0].type) {
		    case 193:
		    case 194:
			if (source.grib2.stat_process_ranges.size() == 2) {
			  float x=source.grib2.stat_process_ranges[0].period_length.value/static_cast<float>(getDaysInMonth(source.referenceDateTime.getYear(),source.referenceDateTime.getMonth()));
std::cerr << "B194 " << x << " " << source.referenceDateTime.getYear() << " " << source.referenceDateTime.getMonth() << std::endl;
			  if (myequalf(x,static_cast<int>(x),0.001)) {
			    srange.type=0;
			    srange.time_increment_type=1;
			    srange.period_length.unit=2;
			    srange.period_length.value=getDaysInMonth(source.referenceDateTime.getYear(),source.referenceDateTime.getMonth());
			    srange.period_time_increment.unit=1;
			    srange.period_time_increment.value=source.grib2.stat_process_ranges[0].period_time_increment.value;
			    grib2.stat_process_ranges[1]=srange;
			    can_add=true;
			  }
			}
			break;
		    case 204:
			if (source.grib2.stat_process_ranges.size() == 2) {
			  float x=source.grib2.stat_process_ranges[0].period_length.value/static_cast<float>(getDaysInMonth(source.referenceDateTime.getYear(),source.referenceDateTime.getMonth()));
std::cerr << "B204 " << x << " " << source.referenceDateTime.getYear() << " " << source.referenceDateTime.getMonth() << std::endl;
			  if (myequalf(x,static_cast<int>(x),0.001)) {
			    srange.type=0;
			    srange.time_increment_type=1;
			    srange.period_length.unit=2;
			    srange.period_length.value=getDaysInMonth(source.referenceDateTime.getYear(),source.referenceDateTime.getMonth());
			    srange.period_time_increment.unit=1;
			    srange.period_time_increment.value=6;
			    grib2.stat_process_ranges[1]=srange;
			    srange.type=1;
			    srange.time_increment_type=2;
			    srange.period_length.unit=1;
			    srange.period_length.value=source.grib2.stat_process_ranges[0].period_time_increment.value;
			    srange.period_time_increment.unit=255;
			    srange.period_time_increment.value=0;
			    grib2.stat_process_ranges.push_back(srange);
			    can_add=true;
			  }
			}
			break;
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
	  if (referenceDateTime.getYear() != source.referenceDateTime.getYear() && referenceDateTime.getMonth() == source.referenceDateTime.getMonth() && referenceDateTime.getDay() == source.referenceDateTime.getDay() && referenceDateTime.getTime() == source.referenceDateTime.getTime()) {
	    srange.period_length.unit=4;
	    srange.period_time_increment.unit=4;
	    srange.period_time_increment.value=source.referenceDateTime.getYearsSince(referenceDateTime);
	  }
	  else {
	    srange.period_length.unit=1;
	    srange.period_time_increment.unit=1;
	    srange.period_time_increment.value=source.referenceDateTime.getHoursSince(referenceDateTime);
	  }
	  grib2.stat_process_ranges[0]=srange;
	}
	if (referenceDateTime.getYear() != source.referenceDateTime.getYear() && referenceDateTime.getMonth() == source.referenceDateTime.getMonth() && referenceDateTime.getDay() == source.referenceDateTime.getDay() && referenceDateTime.getTime() == source.referenceDateTime.getTime()) {
	  grib2.stat_process_ranges[0].period_length.value=source.referenceDateTime.getYearsSince(referenceDateTime)+1;
	  grib2.stat_period_end=source.grib2.stat_period_end;
	}
	else {
	  grib2.stat_process_ranges[0].period_length.value=source.referenceDateTime.getHoursSince(referenceDateTime);
	  grib2.stat_period_end=source.validDateTime;
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

std::string Interval24HourForecastAveragesToString(std::string format,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=NULL;

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
    if (navg == static_cast<int>(getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth()))) {
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
	lastValidDateTime=firstValidDateTime.hoursAdded((navg-1)*24+(p2-p1));
	break;
    }
    case 2:
    {
	lastValidDateTime=firstValidDateTime.daysAdded((navg-1)+(p2-p1));
	break;
    }
    default:
    {
	myerror="unable to compute lastValidDateTime from time units "+strutils::itos(tunit);
	exit(1);
    }
  }
  return product;
}

std::string successiveForecastAveragesToString(std::string format,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=NULL;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  if (tunit >= 1 && tunit <= 2) {
    int n= (tunit == 2) ? 1 : 24/p2;
    if ( (getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth())-navg/n) < 3) {
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
	lastValidDateTime=firstValidDateTime.hoursAdded(navg*(p2-p1));
	break;
    case 2:
	lastValidDateTime=firstValidDateTime.daysAdded(navg*(p2-p1));
	break;
    case 11:
	lastValidDateTime=firstValidDateTime.hoursAdded(navg*(p2-p1)*6);
	break;
    default:
	myerror="unable to compute lastValidDateTime from time units "+strutils::itos(tunit)+" (successiveForecastAveragesToString)";
	exit(1);
  }
  return product;
}

std::string timeRange2ToString(std::string format,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=NULL;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  lastValidDateTime=firstValidDateTime;
  if (navg > 0) {
    int n=getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth());
    if ( (navg % n) == 0) {
	product="Monthly Mean ("+strutils::itos(navg/n)+" per day) of ";
	lastValidDateTime.addHours((navg-1)*24*n/navg);
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
	lastValidDateTime.addHours(p2-p1);
	break;
    }
    case 2:
    {
	lastValidDateTime.addHours((p2-p1)*24);
	break;
    }
    default:
    {
	myerror="unable to compute lastValidDateTime from time units "+strutils::itos(tunit);
	exit(1);
    }
  }
  return product;
}

std::string timeRange113ToString(std::string format,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=NULL;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  else {
    myerror="timeRange113ToString returned error: no time units defined for GRIB format "+format;
    exit(1);
  }
  if (tunit >= 1 && tunit <= 2) {
    int n= (tunit == 2) ? 1 : 24/p2;
    if ( (getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth())-navg/n) < 3) {
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
	lastValidDateTime=firstValidDateTime.hoursAdded((navg-1)*p2);
	break;
    }
    case 2:
    {
	lastValidDateTime=firstValidDateTime.hoursAdded((navg-1)*p2*24);
	break;
    }
    default:
    {
	myerror="unable to compute lastValidDateTime from time units "+strutils::itos(tunit);
	exit(1);
    }
  }
  return product;
}

std::string timeRange118ToString(std::string format,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
{
  std::string product;

  if (tunit >= 1 && tunit <= 2) {
    int n= (tunit == 2) ? 1 : 24/p2;
    if ( (getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth())-navg/n) < 3) {
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
	lastValidDateTime=firstValidDateTime.hoursAdded((navg-1)*p2);
	break;
    }
    case 2:
    {
	lastValidDateTime=firstValidDateTime.hoursAdded((navg-1)*p2*24);
	break;
    }
    default:
    {
	myerror="unable to compute lastValidDateTime from time units "+strutils::itos(tunit)+" (scanFile case 118)";
	exit(1);
    }
  }
  return product;
}

std::string timeRange120ToString(std::string format,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
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
    if ( (getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth())-navg/n) < 3) {
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
	lastValidDateTime=firstValidDateTime.hoursAdded(navg*(p2-p1));
	break;
    }
    default:
    {
	myerror="unable to compute lastValidDateTime from time units "+strutils::itos(tunit)+" (timeRange120ToString)";
	exit(1);
    }
  }
  return product;
}

std::string timeRange123ToString(std::string format,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p2,int navg)
{
  std::string product;
  char **tu=NULL;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  if (tunit >= 1 && tunit <= 2) {
    int n= (tunit == 2) ? 1 : 24/p2;
    if ( (getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth())-navg/n) < 3) {
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
	lastValidDateTime=firstValidDateTime.hoursAdded((navg-1)*p2);
	break;
    }
    case 2:
    {
	lastValidDateTime=firstValidDateTime.hoursAdded((navg-1)*p2*24);
	break;
    }
    default:
    {
	myerror="unable to compute lastValidDateTime from time units "+strutils::itos(tunit);
	exit(1);
    }
  }
  return product;
}

std::string timeRange128ToString(std::string format,short center,short subcenter,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=NULL;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  switch (center) {
    case 7:
    case 60:
	if (tunit >= 1 && tunit <= 2) {
	  if (navg == static_cast<int>(getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth()))) {
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
	    lastValidDateTime=firstValidDateTime.hoursAdded((navg-1)*24+(p2-p1));
	    break;
	  }
	  case 2:
	  {
	    lastValidDateTime=firstValidDateTime.daysAdded((navg-1)+(p2-p1));
	    break;
	  }
	  default:
	  {
	    myerror="unable to compute lastValidDateTime from time units "+strutils::itos(tunit);
	    exit(1);
	  }
	}
	break;
    case 34:
	switch (subcenter) {
	  case 241:
	    product=Interval24HourForecastAveragesToString(format,firstValidDateTime,lastValidDateTime,tunit,p1,p2,navg);
	    break;
	  default:
	    myerror="time range code 128 is not defined for center 34, subcenter "+strutils::itos(subcenter);
	    exit(1);
	}
	break;
    default:
	myerror="time range code 128 is not defined for center "+strutils::itos(center);
	exit(1);
  }
  return product;
}

std::string timeRange129ToString(std::string format,short center,short subcenter,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
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
	if (tunit >= 1 && tunit <= 2) {
	  int n= (tunit == 2) ? 1 : 24/p2;
	  if ( (getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth())-navg/n) < 3) {
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
	    lastValidDateTime=firstValidDateTime.hoursAdded(navg*(p2-p1));
	    break;
	  case 2:
	    lastValidDateTime=firstValidDateTime.daysAdded(navg*(p2-p1));
	    break;
	  default:
	    myerror="unable to compute lastValidDateTime for center 7 from time units "+strutils::itos(tunit)+" (timeRange129ToString)";
	    exit(1);
	}
	break;
    case 34:
	switch (subcenter) {
	  case 241:
	    if (tunit == 1) {
		if ( (getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth())-navg) < 3) {
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
		  lastValidDateTime=firstValidDateTime.hoursAdded((navg-1)*24+p2);
		  break;
		default:
		  myerror="unable to compute lastValidDateTime for center 34, subcenter 241 from time units "+strutils::itos(tunit)+" (timeRange129ToString)";
		  exit(1);
            }
	    break;
	  default:
	    myerror="time range code 129 is not defined for center  34, subcenter "+strutils::itos(center);
	    exit(1);
	}
	break;
    default:
	myerror="time range code 129 is not defined for center "+strutils::itos(center);
	exit(1);
  }
  return product;
}

std::string timeRange130ToString(std::string format,short center,short subcenter,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
{
  std::string product;

  switch (center) {
    case 7:
	product=Interval24HourForecastAveragesToString(format,firstValidDateTime,lastValidDateTime,tunit,p1,p2,navg);
	break;
    case 34:
	switch (subcenter) {
	  case 241:
	    product=successiveForecastAveragesToString(format,firstValidDateTime,lastValidDateTime,tunit,p1,p2,navg);
	    break;
	  default:
	    myerror="time range code 130 is not defined for center 34, subcenter "+strutils::itos(subcenter);
	    exit(1);
	}
	break;
    default:
	myerror="time range code 130 is not defined for center "+strutils::itos(center);
	exit(1);
  }
  return product;
}

std::string timeRange131ToString(std::string format,short center,short subcenter,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
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
	product=successiveForecastAveragesToString(format,firstValidDateTime,lastValidDateTime,tunit,p1,p2,navg);
	break;
    case 34:
	switch (subcenter) {
	  case 241:
	    if (tunit == 1) {
		int n=24/p2;
		if ( (getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth())-navg/n) < 3) {
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
		  lastValidDateTime=firstValidDateTime.hoursAdded(navg*(p2-p1));
		  break;
		default:
		  myerror="unable to compute lastValidDateTime for center 34, subcenter 241 from time units "+strutils::itos(tunit)+" (timeRange131ToString)";
		  exit(1);
	    }
	    break;
	  default:
	    myerror="time range code 131 is not defined for center 34, subcenter "+strutils::itos(subcenter);
	    exit(1);
	}
	break;
    default:
	myerror="time range code 131 is not defined for center "+strutils::itos(center);
	exit(1);
  }
  return product;
}

std::string timeRange132ToString(std::string format,short center,short subcenter,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
{
  std::string product;

  switch (center) {
    case 34:
	switch (subcenter) {
	  case 241:
	    product=timeRange118ToString(format,firstValidDateTime,lastValidDateTime,tunit,p1,p2,navg);
	    break;
	  default:
	    myerror="time range code 132 is not defined for center 34, subcenter "+strutils::itos(subcenter);
	    exit(1);
	}
	break;
    default:
        myerror="time range code 132 is not defined for center "+strutils::itos(center);
	exit(1);
  }
  return product;
}

std::string timeRange137ToString(std::string format,short center,short subcenter,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=NULL;

  if (format == "grib") {
    tu=const_cast<char **>(grib1_time_unit);
  }
  else if (format == "grib2") {
    tu=const_cast<char **>(grib2_time_unit);
  }
  switch (center) {
    case 7:
    case 60:
	if (tunit >= 1 && tunit <= 2) {
	  if (navg/4 == static_cast<int>(getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth()))) {
	    product="Monthly Mean (4 per day) of ";
	  }
	  else if (navg == static_cast<int>(getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth()))) {
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
	    lastValidDateTime=firstValidDateTime.hoursAdded((navg-1)*6+(p2-p1));
	    break;
	  }
	  default:
	  {
	    myerror="unable to compute lastValidDateTime from time units "+strutils::itos(tunit);
	    exit(1);
	  }
	}
	break;
    default:
	myerror="time range code 137 is not defined for center "+strutils::itos(center);
	exit(1);
  }
  return product;
}

std::string timeRange138ToString(std::string format,short center,short subcenter,DateTime& firstValidDateTime,DateTime& lastValidDateTime,short tunit,short p1,short p2,int navg)
{
  std::string product;
  char **tu=NULL;
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
	if (tunit == 11) {
	  p1*=6;
	  p2*=6;
	  tunit=1;
	}
	if (tunit >= 1 && tunit <= 2) {
	  if (navg/4 == static_cast<int>(getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth()))) {
	    product="Monthly Mean (4 per day) of ";
	    hours_to_last=(navg-1)*6;
	  }
	  else if (navg == static_cast<int>(getDaysInMonth(firstValidDateTime.getYear(),firstValidDateTime.getMonth()))) {
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
	lastValidDateTime=firstValidDateTime;
	switch (tunit) {
	  case 1:
	  {
	    lastValidDateTime.addHours(hours_to_last+(p2-p1));
	    break;
	  }
	  default:
	  {
	    myerror="unable to compute lastValidDateTime from time units "+strutils::itos(tunit);
	    exit(1);
	  }
	}
	break;
    default:
        myerror="time range code 138 is not defined for center "+strutils::itos(center);
	exit(1);
  }
  return product;
}

void setForecastDateTime(size_t p1,short tunits,const DateTime& referenceDateTime,DateTime& forecastDateTime,size_t& fcst_time)
{
  switch (tunits) {
    case 0:
	forecastDateTime=referenceDateTime.minutesAdded(p1);
	fcst_time=p1*100;
	break;
    case 1:
	forecastDateTime=referenceDateTime.hoursAdded(p1);
	fcst_time=p1*10000;
	break;
    case 2:
	forecastDateTime=referenceDateTime.daysAdded(p1);
	fcst_time=p1*240000;
	break;
    case 3:
	forecastDateTime=referenceDateTime.monthsAdded(p1);
	fcst_time=forecastDateTime.getTimeSince(referenceDateTime);
	break;
    case 4:
	forecastDateTime=referenceDateTime.yearsAdded(p1);
	fcst_time=forecastDateTime.getTimeSince(referenceDateTime);
	break;
    case 12:
	forecastDateTime=referenceDateTime.hoursAdded(p1*12);
	fcst_time=p1*12*10000;
	break;
    default:
	myerror="unable to convert forecast hour for time units "+strutils::itos(tunits);
	exit(1);
  }
}

std::string getGRIBProductDescription(GRIBGrid *grid,DateTime& forecastDateTime,DateTime& validDateTime,size_t& fcst_time)
{
  std::string product_description;

  short tunits=grid->getTimeUnits();
  short p1=grid->getP1()*grib1_unit_mult[tunits];
  short p2=grid->getP2()*grib1_unit_mult[tunits];
  switch (grid->getTimeRange()) {
    case 0:
    case 10:
	if (p1 == 0) {
	  product_description="Analysis";
	  fcst_time=0;
	  forecastDateTime=grid->getReferenceDateTime();
	}
	else {
	  product_description=strutils::itos(p1)+"-"+grib1_time_unit[tunits]+" Forecast";
	  setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	}
	validDateTime=forecastDateTime;
	break;
    case 1:
	product_description=strutils::itos(p1)+"-"+grib1_time_unit[tunits]+" Forecast";
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	validDateTime=forecastDateTime;
	break;
    case 2:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange2ToString("grib",forecastDateTime,validDateTime,tunits,p1,p2,grid->getNumberAveraged());
	break;
    case 3:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	if (p1 == 0 && tunits == 2 && p2 == static_cast<int>(getDaysInMonth(forecastDateTime.getYear(),forecastDateTime.getMonth()))) {
	  product_description="Monthly Average";
	}
	else {
	  product_description=strutils::itos(p2-p1)+"-"+grib1_time_unit[tunits]+" Average (initial+"+strutils::itos(p1)+" to initial+"+strutils::itos(p2)+")";
	}
	switch (tunits) {
	  case 1:
	  {
	    validDateTime=forecastDateTime.hoursAdded(p2-p1);
	    break;
	  }
	  case 2:
	  {
	    validDateTime=forecastDateTime.daysAdded(p2-p1);
	    break;
	  }
	  case 3:
	  {
	    validDateTime=forecastDateTime.monthsAdded(p2-p1);
	    break;
	  }
	  default:
	  {
	    myerror="unable to compute validDateTime from time units "+strutils::itos(tunits);
	    exit(1);
	  }
	}
	break;
    case 4:
    case 5:
	fcst_time=0;
	product_description=strutils::itos(std::abs(p2-p1))+"-"+grib1_time_unit[tunits]+" ";
	if (grid->getTimeRange() == 4) {
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
	forecastDateTime=validDateTime=grid->getReferenceDateTime();
	switch (tunits) {
	  case 1:
	    if (p2 < 0) {
		validDateTime.subtractHours(-p2);
	    }
	    else {
		validDateTime.addHours(p2);
	    }
	    if (p1 < 0) {
		forecastDateTime.subtractHours(-p1);
	    }
	    break;
	  case 2:
	    if (p2 < 0) {
		validDateTime.subtractDays(-p2);
	    }
	    else {
		validDateTime.addDays(p2);
	    }
	    if (p1 < 0) {
		forecastDateTime.subtractDays(-p1);
	    }
	    break;
	  case 3:
	    if (p2 < 0) {
		validDateTime.subtractMonths(-p2);
	    }
	    else {
		validDateTime.addMonths(p2);
	    }
	    if (p1 < 0) {
		forecastDateTime.subtractMonths(-p1);
	    }
	    break;
	  default:
	    myerror="unable to compute validDateTime from time units "+strutils::itos(tunits);
	    exit(1);
	}
	break;
    case 7:
    {
	if (grid->getSource() == 7) {
	  if (p2 == 0) {
	    forecastDateTime=validDateTime=grid->getReferenceDateTime();
	    switch (tunits) {
		case 2:
		{
		  forecastDateTime.subtractDays(p1);
		  size_t n=p1+1;
		  if (getDaysInMonth(forecastDateTime.getYear(),forecastDateTime.getMonth()) == n) {
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
	  myerror="time range 7 not defined for center "+strutils::itos(grid->getSource());
	  exit(1);
	}
	break;
    }
    case 51:
	product_description=strutils::itos(grid->getNumberAveraged())+"-year Climatology of ";
	switch (tunits) {
	  case 3:
	    product_description+="Monthly Means";
	    forecastDateTime=validDateTime=grid->getReferenceDateTime().yearsAdded(grid->getNumberAveraged()-1).monthsAdded(1);
	    break;
	  default:
	    myerror="bad P2 "+strutils::itos(p2)+" for climatological mean";
	    exit(1);
	}
	break;
    case 113:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange113ToString("grib",forecastDateTime,validDateTime,tunits,p1,p2,grid->getNumberAveraged());
	break;
    case 118:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange118ToString("grib",forecastDateTime,validDateTime,tunits,p1,p2,grid->getNumberAveraged());
	break;
    case 120:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange120ToString("grib",forecastDateTime,validDateTime,tunits,p1,p2,grid->getNumberAveraged());
	break;
    case 123:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange123ToString("grib",forecastDateTime,validDateTime,tunits,p2,grid->getNumberAveraged());
	break;
    case 128:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange128ToString("grib",grid->getSource(),grid->getSubCenterID(),forecastDateTime,validDateTime,tunits,p1,p2,grid->getNumberAveraged());
	break;
    case 129:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange129ToString("grib",grid->getSource(),grid->getSubCenterID(),forecastDateTime,validDateTime,tunits,p1,p2,grid->getNumberAveraged());
	break;
    case 130:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange130ToString("grib",grid->getSource(),grid->getSubCenterID(),forecastDateTime,validDateTime,tunits,p1,p2,grid->getNumberAveraged());
	break;
    case 131:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange131ToString("grib",grid->getSource(),grid->getSubCenterID(),forecastDateTime,validDateTime,tunits,p1,p2,grid->getNumberAveraged());
	break;
    case 132:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange132ToString("grib",grid->getSource(),grid->getSubCenterID(),forecastDateTime,validDateTime,tunits,p1,p2,grid->getNumberAveraged());
	break;
    case 137:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange137ToString("grib",grid->getSource(),grid->getSubCenterID(),forecastDateTime,validDateTime,tunits,p1,p2,grid->getNumberAveraged());
	break;
    case 138:
	setForecastDateTime(p1,tunits,grid->getReferenceDateTime(),forecastDateTime,fcst_time);
	product_description=timeRange138ToString("grib",grid->getSource(),grid->getSubCenterID(),forecastDateTime,validDateTime,tunits,p1,p2,grid->getNumberAveraged());
	break;
    default:
	myerror="GRIB time range indicator "+strutils::itos((reinterpret_cast<GRIBGrid *>(grid))->getTimeRange())+" not recognized";
	exit(1);
  }
  return product_description;
}

std::string getGRIB2ProductDescription(GRIB2Grid *grid,DateTime& forecastDateTime,DateTime& validDateTime)
{
  std::string product_description;

  short ptype=grid->getProductType();
  short tunits=grid->getTimeUnits();
  int fcst_hr=grid->getForecastTime()/10000;
  switch (ptype) {
    case 0:
    case 1:
    case 2:
	if (fcst_hr == 0) {
	  product_description="Analysis";
	}
	else {
	  product_description=strutils::itos(fcst_hr)+"-"+grib2_time_unit[tunits]+" Forecast";
	}
	break;
    case 8:
    case 11:
    case 12:
    case 61:
    {
	std::vector<GRIB2Grid::StatisticalProcessRange> spranges=grid->getStatisticalProcessRanges();
	product_description="";
	for (size_t n=0; n < spranges.size(); n++) {
	  if (product_description.length() > 0) {
	    product_description+=" of ";
	  }
	  if (spranges.size() == 2 && spranges[n].type >= 192) {
	    switch (grid->getSource()) {
		case 7:
		case 60:
		  switch (spranges[0].type) {
		    case 193:
// average of analyses
			product_description+=timeRange113ToString("grib2",forecastDateTime,validDateTime,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    case 194:
			product_description+=timeRange123ToString("grib2",forecastDateTime,validDateTime,tunits,spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    case 195:
// average of foreast accumulations at 24-hour intervals
			product_description+=timeRange128ToString("grib2",grid->getSource(),(reinterpret_cast<GRIB2Grid *>(grid))->getSubCenterID(),forecastDateTime,validDateTime,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    case 197:
// average of foreast averages at 24-hour intervals
			product_description+=timeRange130ToString("grib2",grid->getSource(),(reinterpret_cast<GRIB2Grid *>(grid))->getSubCenterID(),forecastDateTime,validDateTime,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    case 204:
// average of foreast accumulations at 6-hour intervals
			product_description+=timeRange137ToString("grib2",grid->getSource(),(reinterpret_cast<GRIB2Grid *>(grid))->getSubCenterID(),forecastDateTime,validDateTime,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    case 205:
// average of foreast averages at 6-hour intervals
			product_description+=timeRange138ToString("grib2",grid->getSource(),(reinterpret_cast<GRIB2Grid *>(grid))->getSubCenterID(),forecastDateTime,validDateTime,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    case 255:
			product_description+=timeRange2ToString("grib2",forecastDateTime,validDateTime,tunits,(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
			break;
		    default:
			myerror="unable to decode NCEP local-use statistical process type "+strutils::itos(spranges[0].type);
			exit(1);
		  }
		  n=spranges.size();
		  break;
		default:
		  myerror="unable to decode local-use statistical processing for center "+strutils::itos(grid->getSource());
		  exit(1);
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
		    product_description+=strutils::itos(-spranges[n].period_length.value*grib2_unit_mult[spranges[n].period_length.unit])+"-"+grib2_time_unit[spranges[n].period_length.unit]+" Accumulation (initial";
//		    forecastDateTime.subtract(grib2_time_unit[spranges[n].period_length.unit]+std::string("s"),-spranges[n].period_length.value*grib2_unit_mult[spranges[n].period_length.unit]);
forecastDateTime=validDateTime.subtracted(grib2_time_unit[spranges[n].period_length.unit]+std::string("s"),-spranges[n].period_length.value*grib2_unit_mult[spranges[n].period_length.unit]);
		    if (forecastDateTime < grid->getReferenceDateTime()) {
			product_description+="-"+strutils::itos(grid->getReferenceDateTime().getHoursSince(forecastDateTime));
		    }
		    else {
			product_description+="+"+strutils::itos(forecastDateTime.getHoursSince(grid->getReferenceDateTime()));
		    }
		    product_description+=" to initial";
		    if (validDateTime < grid->getReferenceDateTime()) {
			product_description+="-"+strutils::itos(grid->getReferenceDateTime().getHoursSince(validDateTime));
		    }
		    else {
			product_description+="+"+strutils::itos(validDateTime.getHoursSince(grid->getReferenceDateTime()));
		    }
		    product_description+=")";
		    break;
		  default:
		    myerror="no decoding for negative period and statistical process type "+strutils::itos(spranges[n].type)+", length: "+strutils::itos(spranges[n].period_length.value)+"; date - "+forecastDateTime.toString();
		    exit(1);
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
		    switch (spranges[n].type) {
			case 0:
			  product_description="Average";
			  break;
			case 1:
			  product_description="Accumulation";
			  break;
			case 2:
			  product_description="Maximum";
			  break;
			case 3:
			  product_description="Minimum";
			  break;
			case 255:
			  product_description="Period";
			  break;
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
		    validDateTime=forecastDateTime.added(grib2_time_unit[spranges[n].period_length.unit]+std::string("s"),spranges[n].period_length.value*grib2_unit_mult[spranges[n].period_length.unit]);
		    break;
		  default:
		    myerror="no decoding for positive period and statistical process type "+strutils::itos(spranges[n].type);
		    exit(1);
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

} // end namespace gributils
