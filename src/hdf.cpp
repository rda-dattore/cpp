#include <zlib.h>
#include <hdf.hpp>
#include <myerror.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

HDF4Stream::HDF4Stream() : data_types(),tag_table()
{
  TagEntry te;

  data_types.emplace_back("DFNT_NONE");
  data_types.emplace_back("DFNT_VERSION");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("DFNT_UCHAR8");
  data_types.emplace_back("DFNT_CHAR8");
  data_types.emplace_back("DFNT_FLOAT32");
  data_types.emplace_back("DFNT_FLOAT64");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("Undefined");
  data_types.emplace_back("DFNT_INT8");
  data_types.emplace_back("DFNT_UINT8");
  data_types.emplace_back("DFNT_INT16");
  data_types.emplace_back("DFNT_UINT16");
  data_types.emplace_back("DFNT_INT32");
  data_types.emplace_back("DFNT_UINT32");
  data_types.emplace_back("DFNT_INT64");
  data_types.emplace_back("DFNT_UINT64");
  te.key=1;
  te.ID="DFTAG_NULL";
  te.description="No data";
  tag_table.insert(te);
  te.key=30;
  te.ID="DFTAG_VERSION";
  te.description="Library version number";
  tag_table.insert(te);
  te.key=106;
  te.ID="DFTAG_NT";
  te.description="Number type";
  tag_table.insert(te);
  te.key=701;
  te.ID="DFTAG_SDD";
  te.description="Scientific data dimension";
  tag_table.insert(te);
  te.key=702;
  te.ID="DFTAG_SD";
  te.description="Scientific data";
  tag_table.insert(te);
  te.key=720;
  te.ID="DFTAG_NDG";
  te.description="Numeric data group";
  tag_table.insert(te);
  te.key=721;
  te.ID="**DUMMY**";
  te.description="Dummy for scientific data - nothing actually written to file";
  tag_table.insert(te);
  te.key=1962;
  te.ID="DFTAG_VH";
  te.description="Vdata description";
  tag_table.insert(te);
  te.key=1963;
  te.ID="DFTAG_VS";
  te.description="Vdata";
  tag_table.insert(te);
  te.key=1965;
  te.ID="DFTAG_VG";
  te.description="Vgroup";
  tag_table.insert(te);
}

bool InputHDF4Stream::open(const char *filename)
{
  if (is_open()) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="currently connected to another file stream";
    return false;
  }
  data_descriptors.clear();
  reference_table.clear();
  num_read=0;
  fs.open(filename,std::ios::in);
  if (!fs.is_open()) {
    return false;
  }
  unsigned char buf[12];
  char *cbuf=reinterpret_cast<char *>(buf);
// check for 0x0e031301 in first four bytes (^N^C^S^A)
  fs.read(cbuf,4);
  if (fs.gcount() != 4) {
    return false;
  }
  if (buf[0] != 0x0e || buf[1] != 0x03 || buf[2] != 0x13 || buf[3] != 0x01) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="not an HDF4 file";
    return false;
  }
  file_name=filename;
  auto next=-1;
  while (next != 0) {
    fs.read(cbuf,6);
    if (fs.gcount() != 6) {
	return bfstream::error;
    }
    size_t num_dds;
    getBits(buf,num_dds,0,16);
    getBits(buf,next,16,32);
    for (size_t n=0; n < num_dds; ++n) {
	fs.read(cbuf,12);
	if (fs.gcount() != 12) {
	  return bfstream::error;
	}
	DataDescriptor dd;
	getBits(buf,dd.key,0,16);
	if (dd.key != 1) {
	  getBits(buf,dd.reference_number,16,16);
	  getBits(buf,dd.offset,32,32);
	  getBits(buf,dd.length,64,32);
	  data_descriptors.emplace_back(dd);
	  ReferenceEntry re;
	  if (!reference_table.found(dd.reference_number,re)) {
	    re.key=dd.reference_number;
	    re.data_descriptor_table.reset(new my::map<DataDescriptor>);
	    reference_table.insert(re);
	  }
	  re.data_descriptor_table->insert(dd);
	}
    }
    fs.seekg(next,std::ios_base::beg);
  }
  return true;
}

int InputHDF4Stream::peek()
{
return bfstream::error;
}

void InputHDF4Stream::printDataDescriptor(const DataDescriptor& data_descriptor,std::string indent)
{
  TagEntry te;
  ReferenceEntry re;
  DataDescriptor dd;
  int n,off;
  short shdum;
  int idum;
  union {
    long long ldum;
    double ddum;
  };

  re.data_descriptor_table=NULL;
  if (tag_table.found(data_descriptor.key,te))
    std::cout << te.ID << "(" << data_descriptor.key << ")/" << data_descriptor.reference_number << " - " << te.description << "  Offset: " << data_descriptor.offset << "  Length: " << data_descriptor.length << std::endl;
  else
    std::cout << "**" << data_descriptor.key << "/" << data_descriptor.reference_number << "  Offset: " << data_descriptor.offset << "  Length: " << data_descriptor.length << std::endl;
  switch (data_descriptor.key) {
    case 30:
    {
	fs.seekg(data_descriptor.offset,std::ios_base::beg);
	auto buffer=new unsigned char[data_descriptor.length];
	char *cbuf=reinterpret_cast<char *>(buffer);
	fs.read(cbuf,data_descriptor.length);
	if (fs.gcount() != data_descriptor.length) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	std::cout << indent;
	getBits(buffer,idum,0,32);
	std::cout << "  Version: " << idum;
	getBits(buffer,idum,32,32);
	std::cout << "." << idum;
	getBits(buffer,idum,64,32);
	std::cout << "." << idum << "  Description: ";
	std::cout.write(&cbuf[12],data_descriptor.length-12);
	std::cout << std::endl;
	delete[] buffer;
	break;
    }
    case 106:
    {
	fs.seekg(data_descriptor.offset,std::ios_base::beg);
	auto buffer=new unsigned char[data_descriptor.length];
	fs.read(reinterpret_cast<char *>(buffer),data_descriptor.length);
	if (fs.gcount() != data_descriptor.length) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	std::cout << indent;
	getBits(buffer,idum,0,8);
	std::cout << "  version: " << idum;
	getBits(buffer,idum,8,8);
	std::cout << "  type: " << data_types[idum] << "(" << idum << ")";
	getBits(buffer,idum,16,8);
	std::cout << "  width: " << idum;
	getBits(buffer,idum,24,8);
	std::cout << "  class: " << idum;
	std::cout << std::endl;
	delete[] buffer;
	break;
    }
    case 701:
    {
	int ndims;
	int *nvals;
	short *scale_nt_refs;

	fs.seekg(data_descriptor.offset,std::ios_base::beg);
	auto buffer=new unsigned char[data_descriptor.length];
	fs.read(reinterpret_cast<char *>(buffer),data_descriptor.length);
	if (fs.gcount() != data_descriptor.length) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	std::cout << indent;
	getBits(buffer,ndims,0,16);
	std::cout << "  #dims: " << ndims;
	nvals=new int[ndims];
	off=16;
	getBits(buffer,nvals,off,32,0,ndims);
	off+=32*ndims;
	getBits(buffer,idum,off,16);
	std::cout << "  data_nt_ref: " << idum << std::endl;
	off+=16;
	scale_nt_refs=new short[ndims];
	getBits(buffer,scale_nt_refs,off,16,0,ndims);
	for (n=0; n < ndims; ++n) {
	  std::cout << indent << "  Dimension: " << n << "  #vals: " << nvals[n] << "  scale_nt_ref: " << scale_nt_refs[n] << std::endl;
	}
	delete[] nvals;
	delete[] scale_nt_refs;
	delete[] buffer;
	break;
    }
    case 720:
    {
	short *vals;

	fs.seekg(data_descriptor.offset,std::ios_base::beg);
	auto buffer=new unsigned char[data_descriptor.length];
	fs.read(reinterpret_cast<char *>(buffer),data_descriptor.length);
	if (fs.gcount() != data_descriptor.length) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	vals=new short[data_descriptor.length/2];
	getBits(buffer,vals,0,16,0,data_descriptor.length/2);
	for (n=0; n < data_descriptor.length/2; n+=2) {
	  std::cout << indent << "  Member: " << n/2 << "  tag/ref#: ";
	  if (tag_table.found(vals[n],te)) {
	    std::cout << te.ID << "(" << vals[n] << ")";
	  }
	  else
	    std::cout << vals[n];
	  std::cout << "/" << vals[n+1] << std::endl;
	}
	delete[] vals;
	delete[] buffer;
	break;
    }
    case 1962:
    {
	int nentries,nfields;
	short *types,*sizes,*offsets,*orders,*fldnmlens;
	char **fldnms,*cdum;

	fs.seekg(data_descriptor.offset,std::ios_base::beg);
	auto buffer=new unsigned char[data_descriptor.length];
	fs.read(reinterpret_cast<char *>(buffer),data_descriptor.length);
	if (fs.gcount() != data_descriptor.length) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	std::cout << indent;
	getBits(buffer,idum,0,16);
	std::cout << "  Interlace: " << idum;
	getBits(buffer,nentries,16,32);
	std::cout << "  #entries: " << nentries;
	getBits(buffer,idum,48,16);
	std::cout << "  entry len: " << idum;
	getBits(buffer,nfields,64,16);
	std::cout << "  #fields/entry: " << nfields << std::endl;
	off=80;
	types=new short[nfields];
	sizes=new short[nfields];
	offsets=new short[nfields];
	orders=new short[nfields];
	fldnmlens=new short[nfields];
	fldnms=new char *[nfields];
	getBits(buffer,types,off,16,0,nfields);
	off+=16*nfields;
	getBits(buffer,sizes,off,16,0,nfields);
	off+=16*nfields;
	getBits(buffer,offsets,off,16,0,nfields);
	off+=16*nfields;
	getBits(buffer,orders,off,16,0,nfields);
	off+=16*nfields;
	for (n=0; n < nfields; ++n) {
	  getBits(buffer,fldnmlens[n],off,16);
	  off+=16;
	  fldnms[n]=new char[fldnmlens[n]];
	  getBits(buffer,fldnms[n],off,8,0,fldnmlens[n]);
	  off+=8*fldnmlens[n];
	}
	for (n=0; n < nfields; ++n) {
	  std::cout << indent << "  Field: " << n << "  type: " << data_types[types[n]] << "  size: " << sizes[n] << "  offset: " << offsets[n] << "  order: " << orders[n] << "  fieldname: ";
	  std::cout.write(fldnms[n],fldnmlens[n]);
	  std::cout << std::endl;
	}
	delete[] sizes;
	delete[] offsets;
	delete[] orders;
	delete[] fldnmlens;
	for (n=0; n < nfields; ++n) {
	  delete[] fldnms[n];
	}
	delete[] fldnms;
	getBits(buffer,idum,off,16);
	off+=16;
	std::cout << indent;
	cdum=new char[idum];
	getBits(buffer,cdum,off,8,0,idum);
	off+=8*idum;
	std::cout << "  Name: '";
	std::cout.write(cdum,idum);
	delete[] cdum;
	getBits(buffer,idum,off,16);
	off+=16;
	cdum=new char[idum];
	getBits(buffer,cdum,off,8,0,idum);
	std::cout << "'  Class: '";
	std::cout.write(cdum,idum);
	std::cout << "'" << std::endl;
	if (nfields == 1) {
	  std::cout << indent << "    DATA: ";
	  reference_table.found(data_descriptor.reference_number,re);
	  if (!re.data_descriptor_table->found(1963,dd)) {
	    std::cout << " **ERROR: no Vdata found";
	  }
	  else {
	    fs.seekg(dd.offset,std::ios_base::beg);
	    delete[] buffer;
	    buffer=new unsigned char[dd.length];
	    fs.read(reinterpret_cast<char *>(*buffer),dd.length);
	    if (fs.gcount() != dd.length) {
		if (myerror.length() > 0) {
		  myerror+=", ";
		}
		myerror+="Bad data descriptor length";
		exit(1);
	    }
	    dd.length/=nentries;
	    off=0;
	    for (n=0; n < nentries; ++n) {
		if ( (n % 8) != 0) {
		  std::cout << ", ";
		}
		if (data_types[types[0]] == "DFNT_CHAR8" || data_types[types[0]] == "DFNT_UINT8") {
		  std::cout << "'";
		  std::cout.write(reinterpret_cast<char *>(&buffer[off]),dd.length);
		  std::cout << "'";
		  off+=dd.length;
		}
		else if (data_types[types[0]] == "DFNT_INT8") {
		  std::cout << static_cast<int>(buffer[off]);
		  off+=8;
		}
		else if (data_types[types[0]] == "DFNT_INT16") {
		  getBits(buffer,shdum,off,16);
		  std::cout << shdum;
		  off+=16;
		}
		else if (data_types[types[0]] == "DFNT_UINT16") {
		  getBits(buffer,idum,off,16);
		  std::cout << idum;
		  off+=16;
		}
		else if (data_types[types[0]] == "DFNT_INT32") {
		  getBits(buffer,idum,off,32);
		  std::cout << idum;
		  off+=32;
		}
		else if (data_types[types[0]] == "DFNT_FLOAT64") {
		  getBits(buffer,ldum,off,64);
		  std::cout << ddum;
		  off+=64;
		}
		else {
std::cout << "BLAH!!";
		}
		if ( (n+1) < nentries && ((n+1) % 8) == 0) {
		  std::cout << "," << std::endl;
		  std::cout << indent << "          ";
		}
	    }
	  }
	  std::cout << std::endl;
	}
	delete[] types;
	delete[] cdum;
	delete[] buffer;
	break;
    }
    case 1965:
    {
	int nelements;
	short *tags,*ref_nos;

	fs.seekg(data_descriptor.offset,std::ios_base::beg);
	auto buffer=new unsigned char[data_descriptor.length];
	fs.read(reinterpret_cast<char *>(buffer),data_descriptor.length);
	if (fs.gcount() != data_descriptor.length) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	getBits(buffer,nelements,0,16);
	std::cout << indent << "  #elements: " << nelements << std::endl;
	off=16;
	tags=new short[nelements];
	getBits(buffer,tags,off,16,0,nelements);
	off+=16*nelements;
	ref_nos=new short[nelements];
	getBits(buffer,ref_nos,off,16,0,nelements);
	off+=16*nelements;
	for (n=0; n < nelements; ++n) {
	  std::cout << indent << "  Element: " << n << "  ";
	  reference_table.found(ref_nos[n],re);
	  re.data_descriptor_table->found(tags[n],dd);
	  printDataDescriptor(dd,indent+"  ");
	}
	delete[] tags;
	delete[] ref_nos;
	delete[] buffer;
	break;
    }
  }
}

void InputHDF4Stream::printDataDescriptors(size_t tag_number)
{
  for (size_t n=0; n < data_descriptors.size(); ++n) {
    if (data_descriptors[n].key == tag_number) {
	printDataDescriptor(data_descriptors[n]);
    }
  }
}

int InputHDF4Stream::read(unsigned char *buffer,size_t buffer_length)
{
return bfstream::error;
}

InputHDF5Stream::~InputHDF5Stream()
{
  close();
}

void InputHDF5Stream::close()
{
  if (fs.is_open()) {
    fs.close();
  }
  clearGroups(root_group);
  if (ref_table != nullptr) {
    ref_table=nullptr;
  }
  show_debug=false;
}

InputHDF5Stream::Chunk::Chunk(long long addr,size_t sz,size_t len,const std::vector<size_t>& offs) : address(addr),buffer(nullptr),size(sz),length(len),offsets()
{
  for (const auto& off : offs) {
    offsets.emplace_back(off);
  }
}

InputHDF5Stream::Chunk::~Chunk()
{
  if (buffer != nullptr) {
    buffer.reset();
  }
}

InputHDF5Stream::Chunk& InputHDF5Stream::Chunk::operator=(const Chunk& source)
{
  if (this == &source) {
    return *this;
  }
  address=source.address;
  size=source.size;
  length=source.length;
  if (source.buffer != nullptr) {
    allocate();
    std::copy(source.buffer.get(),source.buffer.get()+length,buffer.get());
  }
  for (const auto& off : source.offsets) {
    offsets.emplace_back(off);
  }
  return *this;
}

void InputHDF5Stream::Chunk::allocate()
{
  if (length > 0) {
    buffer.reset(new unsigned char[length]);
  }
}

bool InputHDF5Stream::Chunk::fill(std::fstream& fs,const Dataset& dataset,bool show_debug)
{
  auto curr_off=fs.tellg();
  fs.seekg(address,std::ios_base::beg);
  if (dataset.filters.size() == 0) {
    allocate();
    fs.read(reinterpret_cast<char *>(buffer.get()),length);
    if (fs.gcount() != static_cast<int>(length)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="error reading data chunk";
	return false;
    }
  }
  else {
    if (show_debug) {
	std::cerr << "applying filters..." << std::endl;
    }
    auto buf=new unsigned char[size];
    fs.read(reinterpret_cast<char *>(buf),size);
    if (fs.gcount() != static_cast<int>(size)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="error reading data chunk";
	return false;
    }
    for (size_t n=0; n < dataset.filters.size(); ++n) {
	switch (dataset.filters[n]) {
	  case 1:
// gzip
	    allocate();
	    uncompress(buffer.get(),&length,buf,size);
	    if (show_debug) {
		std::cerr << "--GUNZIPPED-------" << std::endl;
		dump(buffer.get(),length);
		std::cerr << "------------------" << std::endl;
	    }
	    break;
// shuffle
	  case 2:
	  {
	    if ( (length % dataset.data.size_of_element) != 0) {
		if (myerror.length() > 0) {
		  myerror+=", ";
		}
		myerror+="can't unshuffle buffer size of "+strutils::itos(length)+" and data element size of "+strutils::itos(dataset.data.size_of_element);
		return false;
	    }
	    unsigned char *cbuf=new unsigned char[length];
	    std::copy(buffer.get(),buffer.get()+length,cbuf);
	    auto sl=length/dataset.data.size_of_element;
	    for (size_t x=1,y=0; x < length-1; ++x) {
		auto z=(x % sl);
		if (z == 0) {
		  ++y;
		}
		buffer[z*dataset.data.size_of_element+y]=cbuf[x];
	    }
	    delete[] cbuf;
	    if (show_debug) {
		std::cerr << "--UNSHUFFLED------" << std::endl;
		dump(buffer.get(),length);
		std::cerr << "------------------" << std::endl;
	    }
	    break;
	  }
	  case 3:
	  {
	    break;
	  }
	  default:
	  {
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to handle filter ID '"+strutils::itos(dataset.filters[n])+"'";
	    return false;
	  }
	}
    }
    delete[] buf;
  }
  fs.seekg(curr_off,std::ios_base::beg);
  return true;
}

InputHDF5Stream::Datatype::~Datatype()
{
  if (properties != nullptr) {
    properties=nullptr;
  }
}

InputHDF5Stream::Datatype& InputHDF5Stream::Datatype::operator=(const Datatype& source)
{
  if (this == &source) {
    return *this;
  }
  class_=source.class_;
  version=source.version;
  bit_fields[0]=source.bit_fields[0];
  bit_fields[1]=source.bit_fields[1];
  bit_fields[2]=source.bit_fields[2];
  size=source.size;
  prop_len=source.prop_len;
  properties.reset(new unsigned char[prop_len]);
  std::copy(source.properties.get(),source.properties.get()+prop_len,properties.get());
  return *this;
}

InputHDF5Stream::DataValue& InputHDF5Stream::DataValue::operator=(const DataValue& source)
{
  if (this == &source) {
    return *this;
  }
  class_=source.class_;
  precision=source.precision;
  size=source.size;
  dim_sizes=source.dim_sizes;
  switch (class_) {
    case 0:
    case 4:
    {
	size_t max_cnt=1;
	if (dim_sizes.size() == 1) {
	  max_cnt*=dim_sizes[0];
	}
	switch (precision) {
	  case 8:
	    value=new unsigned char[max_cnt];
	    for (size_t cnt=0; cnt < max_cnt; ++cnt) {
		(reinterpret_cast<unsigned char *>(value))[cnt]=(reinterpret_cast<unsigned char *>(source.value))[cnt];
	    }
	    break;
	  case 16:
	    value=new float[max_cnt];
	    for (size_t cnt=0; cnt < max_cnt; ++cnt) {
		(reinterpret_cast<short *>(value))[cnt]=(reinterpret_cast<short *>(source.value))[cnt];
	    }
	    break;
	  case 32:
	    value=new int[max_cnt];
	    for (size_t cnt=0; cnt < max_cnt; ++cnt) {
		(reinterpret_cast<int *>(value))[cnt]=(reinterpret_cast<int *>(source.value))[cnt];
	    }
	    break;
	  case 64:
	    value=new long long[max_cnt];
	    for (size_t cnt=0; cnt < max_cnt; ++cnt) {
		(reinterpret_cast<long long *>(value))[cnt]=(reinterpret_cast<long long *>(source.value))[cnt];
	    }
	    break;
	}
	break;
    }
    case 1:
    {
	size_t max_cnt=1;
	if (dim_sizes.size() == 1) {
	  max_cnt*=dim_sizes[0];
	}
	switch (precision) {
	  case 32:
	  {
	    value=new float[max_cnt];
	    for (size_t cnt=0; cnt < max_cnt; ++cnt) {
		(reinterpret_cast<float *>(value))[cnt]=(reinterpret_cast<float *>(source.value))[cnt];
	    }
	    break;
	  }
	  case 64:
	  {
	    value=new double[max_cnt];
	    for (size_t cnt=0; cnt < max_cnt; ++cnt) {
		(reinterpret_cast<double *>(value))[cnt]=(reinterpret_cast<double *>(source.value))[cnt];
	    }
	    break;
	  }
	}
	break;
    }
    case 3:
    {
	value=new char[precision+1];
	std::copy(reinterpret_cast<char *>(source.value),reinterpret_cast<char *>(source.value)+precision,reinterpret_cast<char *>(value));
	(reinterpret_cast<char *>(value))[precision]='\0';
	break;
    }
    case 6:
    {
	compound.members.resize(source.compound.members.size());
	auto m=0;
	for (size_t n=0; n < compound.members.size(); ++n) {
	  compound.members[n].name=source.compound.members[n].name;
	  compound.members[n].class_=source.compound.members[n].class_;
	  compound.members[n].precision=source.compound.members[n].precision;
	  compound.members[n].size=source.compound.members[n].size;
	  m+=compound.members[n].size;
	}
	if (dim_sizes.size() > 0) {
	  m*=dim_sizes[0];
	}
	value=new unsigned char[m];
	std::copy(reinterpret_cast<unsigned char *>(source.value),reinterpret_cast<unsigned char *>(source.value)+m,reinterpret_cast<unsigned char *>(value));
	break;
    }
    case 9:
    {
	value=new unsigned char[size+1];
	std::copy(reinterpret_cast<unsigned char *>(source.value),reinterpret_cast<unsigned char *>(source.value)+size+1,reinterpret_cast<unsigned char *>(value));
	vlen.class_=source.vlen.class_;
	if (source.vlen.size > 0) {
	  vlen.size=source.vlen.size;
	  vlen.buffer.reset(new unsigned char[vlen.size]);
	  std::copy(&source.vlen.buffer[0],&source.vlen.buffer[vlen.size],vlen.buffer.get());
	}
	else {
	  vlen.size=0;
	  vlen.buffer.reset(nullptr);
	}
	break;
    }
  }
  return *this;
}

void InputHDF5Stream::DataValue::clear()
{
  if (value != nullptr) {
    switch (class_) {
	case 0:
	{
	  switch (precision) {
	    case 8:
	    {
		delete[] reinterpret_cast<unsigned char *>(value);
		break;
	    }
	    case 16: 
	    {
		delete[] reinterpret_cast<short *>(value);
		break;
	    }
	    case 32: 
	    {
		delete[] reinterpret_cast<int *>(value);
		break;
	    }
	    case 64: 
	    {
		delete[] reinterpret_cast<long long *>(value);
		break;
	    }
	  }
	  break;
	}
	case 1:
	{
	  switch (precision) {
	    case 32: 
	    {
		delete[] reinterpret_cast<float *>(value);
		break;
	    }
	    case 64: 
	    {
		delete[] reinterpret_cast<double *>(value);
		break;
	    }
	  }
	  break;
	}
	case 3:
	{
	  delete[] reinterpret_cast<char *>(value);
	  break;
	}
	case 6:
	{
	  if (compound.members.size() > 0) {
	    compound.members.clear();
	    delete[] reinterpret_cast<unsigned char *>(value);
	  }
	  break;
	}
	case 9:
	{
	  delete[] reinterpret_cast<unsigned char *>(value);
	  vlen.buffer.reset(nullptr);
	  break;
	}
    }
    value=nullptr;
  }
}

void InputHDF5Stream::DataValue::print(std::ostream& ofs,std::shared_ptr<my::map<ReferenceEntry>> ref_table) const
{
  if (value == nullptr) {
    ofs << "?????0 (" << class_ << ")";
    return;
  }
  switch (class_) {
    case 0:
// fixed point numbers (integers)
    case 4:
// bitfield
    {
	size_t max_cnt=1;
	if (dim_sizes.size() == 1) {
	  max_cnt*=dim_sizes[0];
	}
	for (size_t n=0; n < max_cnt; ++n) {
	  if (n > 0) {
	    ofs << ", ";
	  }
	  switch (precision) {
	    case 8:
	    {
		ofs << static_cast<int>((reinterpret_cast<unsigned char *>(value))[n]) << "d";
		break;
	    }
	    case 16:
	    {
		ofs << (reinterpret_cast<short *>(value))[n] << "d";
		break;
	    }
	    case 32:
	    {
		ofs << (reinterpret_cast<int *>(value))[n] << "d";
		break;
	    }
	    case 64:
	    {
		ofs << (reinterpret_cast<long long *>(value))[n] << "d";
		break;
	    }
	    default:
	    {
		ofs << "?????d";
	    }
	  }
	}
	break;
    }
    case 1:
// floating point numbers
    {
	size_t max_cnt=1;
	if (dim_sizes.size() == 1) {
	  max_cnt*=dim_sizes[0];
	}
 	ofs.setf(std::ios::fixed);
	for (size_t n=0; n < max_cnt; ++n) {
	  if (n > 0) {
	    ofs << ", ";
	  }
	  switch (precision) {
	    case 32:
	    {
		ofs << (reinterpret_cast<float *>(value))[n] << "f";
		break;
	    }
	    case 64:
	    {
		ofs << (reinterpret_cast<double *>(value))[n] << "f";
		break;
	    }
	    default:
	    {
		ofs << "????.?f";
	    }
	  }
	}
	break;
    }
    case 3:
// strings
	ofs << "\"" << reinterpret_cast<char *>(value) << "\"";
	break;
    case 6:
    {
	DataValue *cval=new DataValue;
	ofs << "(";
	for (size_t n=0; n < compound.members.size(); ++n) {
	  if (n > 0) {
	    ofs << ", ";
	  }
	  ofs << compound.members[n].name;
	}
	ofs << "): ";
	size_t end=0;
	if (dim_sizes.size() > 0) {
	  ofs << "[ ";
	  end=dim_sizes[0];
	}
	else {
	  end=1;
	}
	auto off=0;
	for (size_t n=0; n < end; ++n) {
	  if (n > 0) {
	    ofs << ", ";
	  }
	  ofs << "(";
	  for (size_t m=0; m < compound.members.size(); ++m) {
	    if (m > 0) {
		ofs << ", ";
	    }
	    cval->class_=compound.members[m].class_;
	    cval->precision=compound.members[m].precision;
	    cval->size=compound.members[m].size;
	    cval->value=&(reinterpret_cast<unsigned char *>(value))[off];
	    cval->print(ofs,ref_table);
	    off+=compound.members[m].size;
	  }
	  ofs << ")";
	}
	if (dim_sizes.size() > 0) {
	  ofs << " ]";
	}
	cval->value=nullptr;
	delete cval;
	break;
    }
    case 7:
    {
// reference
	if (ref_table != nullptr) {
	  ReferenceEntry re;
	  re.key=HDF5::getValue(reinterpret_cast<unsigned char *>(value),size);
	  ref_table->found(re.key,re);
	  ofs << re.name;
	}
	else {
	  ofs << HDF5::getValue(reinterpret_cast<unsigned char *>(value),size) << "R";
	}
	break;
    }
    case 9:
    {
// variable-length
	size_t end=1;
	if (dim_sizes.size() > 0) {
	  end=dim_sizes[0];
	}
	auto off=0;
	switch (vlen.class_) {
	  case 3:
	  {
	    for (size_t n=0; n < end; ++n) {
		if (n > 0) {
		  ofs << ", ";
		}
		int len;
		getBits(&vlen.buffer[off],len,0,32);
		ofs << "\"" << std::string(reinterpret_cast<char *>(&vlen.buffer[off+4]),len) << "\"" << std::endl;
		off+=(4+len);
	    }
	    break;
	  }
	  case 7:
	  {
	    if (dim_sizes.size() > 0) {
		ofs << "[";
	    }
	    for (size_t n=0; n < end; ++n) {
		if (n > 0) {
		  ofs << ", ";
		}
		if (ref_table != nullptr) {
		  ReferenceEntry re;
		  re.key=HDF5::getValue(&vlen.buffer[off+4],precision);
		  ref_table->found(re.key,re);
		  ofs << re.name;
		}
		else {
		  ofs << "(" << HDF5::getValue(&vlen.buffer[off+4],precision) << "R";
		}
                off+=(4+precision);
	    }
	    if (dim_sizes.size() > 0) {
		ofs << "]";
	    }
	    break;
	  }
	}
	break;
    }
    default:
    {
	ofs << "?????u (" << class_ << ")";
    }
  }
}

bool InputHDF5Stream::DataValue::set(std::fstream& fs,unsigned char *buffer,short size_of_offsets,short size_of_lengths,const InputHDF5Stream::Datatype& datatype,const InputHDF5Stream::Dataspace& dataspace,bool show_debug)
{
  clear();
  class_=datatype.class_;
  dim_sizes=dataspace.sizes;
  if (show_debug) {
    std::cerr << "class: " << class_ << " dim_sizes size: " << dim_sizes.size() << std::endl;
    for (auto& dim_size : dim_sizes) {
	std::cerr << "  dim_size: " << dim_size << std::endl;
    }
  }
  switch (class_) {
    case 0:
// fixed point numbers (integers)
    case 4:
// bitfield
    {
	short byte_order[2];
	getBits(datatype.bit_fields,byte_order[0],7,1);
	short lo_pad;
	getBits(datatype.bit_fields,lo_pad,6,1);
	short hi_pad;
	getBits(datatype.bit_fields,hi_pad,5,1);
	short sign;
	if (class_ == 0) {
	  getBits(datatype.bit_fields,sign,4,1);
	}
	auto off=HDF5::getValue(&datatype.properties[0],2);
	precision=HDF5::getValue(&datatype.properties[2],2);
	size_t max_cnt=1;
	if (dim_sizes.size() == 1) {
	  max_cnt*=dim_sizes[0];
	}
	switch (byte_order[0]) {
	  case 0:
	    if (off == 0) {
		if ( (precision % 8) == 0) {
		  auto byte_len=precision/8;
		  for (size_t n=0; n < max_cnt; ++n) {
		    switch (byte_len) {
			case 1:
			{
			  if (n == 0) {
			    value=new unsigned char[max_cnt];
			  }
			  (reinterpret_cast<unsigned char *>(value))[n]=HDF5::getValue(&buffer[byte_len*n],1);
			  break;
			}
			case 2:
			{
			  if (n == 0) {
			    value=new short[max_cnt];
			  }
			  (reinterpret_cast<short *>(value))[n]=HDF5::getValue(&buffer[byte_len*n],2);
			  break;
			}
			case 4:
			{
			  if (n == 0) {
			    value=new int[max_cnt];
			  }
			  (reinterpret_cast<int *>(value))[n]=HDF5::getValue(&buffer[byte_len*n],4);
			  break;
			}
			case 8:
			{
			  if (n == 0) {
			    value=new long long[max_cnt];
			  }
			  (reinterpret_cast<long long *>(value))[n]=HDF5::getValue(&buffer[byte_len*n],8);
			  break;
			}
		    }
		  }
		  size=byte_len*max_cnt;
		}
		else {
		  if (!myerror.empty()) {
		    myerror+=", ";
		  }
		  myerror+="unable to decode little-endian integer with precision "+strutils::itos(precision);
		  return false;
		}
	    }
	    else {
		if (!myerror.empty()) {
		  myerror+=", ";
		}
		myerror+="unable to decode little-endian integer with bit offset "+strutils::itos(off);
		return false;
	    }
	    break;
	  case 1:
	    if (off == 0) {
		if ( (precision % 8) == 0) {
		  auto byte_len=precision/8;
		  for (size_t n=0; n < max_cnt; ++n) {
		    switch (byte_len) {
			case 1:
			{
			  if (n == 0) {
			  value=new unsigned char[max_cnt];
			  }
			  getBits(&buffer[byte_len*n],(reinterpret_cast<unsigned char *>(value))[n],0,precision);
			  break;
			}
			case 2:
			{
			  if (n == 0) {
			    value=new short[max_cnt];
			  }
			  getBits(&buffer[byte_len*n],(reinterpret_cast<short *>(value))[n],0,precision);
			  break;
			}
			case 4:
			{
			  if (n == 0) {
			    value=new int[max_cnt];
			  }
			  getBits(&buffer[byte_len*n],(reinterpret_cast<int *>(value))[n],0,precision);
			  break;
			}
			case 8:
			{
			  if (n == 0) {
			    value=new long long[max_cnt];
			  }
			  getBits(&buffer[byte_len*n],(reinterpret_cast<long long *>(value))[n],0,precision);
			  break;
			}
		    }
		  }
		  size=byte_len*max_cnt;
		}
		else {
		  if (!myerror.empty()) {
		    myerror+=", ";
		  }
		  myerror+="unable to decode big-endian integer with precision "+strutils::itos(precision);
		  return false;
		}
	    }
	    else {
		if (!myerror.empty()) {
		  myerror+=", ";
		}
		myerror+="unable to decode big-endian integer with bit offset "+strutils::itos(off);
		return false;
	    }
	    break;
	}
	break;
    }
    case 1:
// floating-point numbers
    {
	short byte_order[2];
	getBits(datatype.bit_fields,byte_order[0],1,1);
	getBits(datatype.bit_fields,byte_order[1],7,1);
	short lo_pad;
	getBits(datatype.bit_fields,lo_pad,6,1);
	short hi_pad;
	getBits(datatype.bit_fields,hi_pad,5,1);
	short int_pad;
	getBits(datatype.bit_fields,int_pad,4,1);
	short mant_norm;
	getBits(datatype.bit_fields,mant_norm,2,2);
	precision=HDF5::getValue(&datatype.properties[2],2);
	size_t max_cnt=1;
	if (dim_sizes.size() == 1) {
	  max_cnt*=dim_sizes[0];
	}
	for (size_t n=0; n < max_cnt; ++n) {
	  unsigned char v[8];
	  if (byte_order[0] == 0 && byte_order[1] == 0) {
// little-endian
	    size_t byte_len=precision/8;
	    int idx=byte_len-1;
	    for (size_t m=0; m < byte_len; ++m) {
		v[m]=(reinterpret_cast<unsigned char *>(buffer))[byte_len*n+idx];
		--idx;
	    }
	  }
	  else if (byte_order[0] == 0 && byte_order[1] == 1) {
// big-endian
	    auto byte_len=precision/8;
	    auto start=byte_len*n;
	    std::copy(&buffer[start],&buffer[start+byte_len],v);
	  }
	  else {
	    if (!myerror.empty()) {
		myerror+=", ";
	    }
	    myerror+="unknown byte order for data value";
	    return false;
	  }
	  if (HDF5::getValue(&datatype.properties[0],2) == 0) {
	    long long exp,mant;
	    getBits(reinterpret_cast<unsigned char *>(v),exp,precision-static_cast<int>(datatype.properties[4])-static_cast<int>(datatype.properties[5]),static_cast<int>(datatype.properties[5]));
	    getBits(reinterpret_cast<unsigned char *>(v),mant,precision-static_cast<int>(datatype.properties[6])-static_cast<int>(datatype.properties[7]),static_cast<int>(datatype.properties[7]));
	    short sign;
	    getBits(reinterpret_cast<unsigned char *>(v),sign,precision-static_cast<int>(datatype.bit_fields[1])-1,1);
	    if (n == 0) {
		switch (precision) {
		  case 32:
		  {
		    value=new float[max_cnt];
		    size=sizeof(float)*max_cnt;
		    break;
		  }
		  case 64:
		  {
		    value=new double[max_cnt];
		    size=sizeof(double)*max_cnt;
		    break;
		  }
		}
	    }
	    if (mant == 0 && (mant_norm != 2 || exp == 0)) {
		switch (precision) {
		  case 32:
		  {
		    (reinterpret_cast<float *>(value))[n]=0.;
		    break;
		  }
		  case 64:
		  {
		    (reinterpret_cast<double *>(value))[n]=0.;
		    break;
		  }
		}
	    }
	    else {
		exp-=HDF5::getValue(&datatype.properties[8],4);
		if (mant_norm == 2) {
		  switch (precision) {
		    case 32:
		    {
		      (reinterpret_cast<float *>(value))[n]=pow(-1.,sign)*(1+pow(2.,-static_cast<double>(datatype.properties[7]))*mant)*pow(2.,exp);
		      break;
		    }
		    case 64:
		    {
		      (reinterpret_cast<double *>(value))[n]=pow(-1.,sign)*(1+pow(2.,-static_cast<double>(datatype.properties[7]))*mant)*pow(2.,exp);
		      break;
		    }
		  }
		}
	    }
	  }
	}
	break;
    }
    case 3:
    {
// strings
	if (dataspace.dimensionality < 0) {
	  precision=0;
	}
	else {
	  precision=datatype.size;
	}
	size=precision;
	value=new char[precision+1];
	getBits(buffer,(reinterpret_cast<char *>(value)),0,8,0,precision);
	(reinterpret_cast<char *>(value))[precision]='\0';
	break;
    }
    case 6:
    {
// compound
	if (show_debug) {
	  std::cerr << "Setting value for compound type - version: " << datatype.version << std::endl;
	}
	compound.members.resize(HDF5::getValue(datatype.bit_fields,2));
	if (show_debug) {
	  std::cerr << "  number of members: " << compound.members.size() << " " << datatype.prop_len << std::endl;
	}
	std::unique_ptr<Datatype[]> l_datatypes;
	l_datatypes.reset(new Datatype[compound.members.size()]);
	auto total_size=0;
	switch (datatype.version) {
	  case 1:
	  {
	    auto off=0;
	    for (size_t n=0; n < compound.members.size(); ++n) {
		compound.members[n].name=&(reinterpret_cast<char *>(datatype.properties.get()))[off];
		if (show_debug) {
		  std::cerr << "  name: " << compound.members[n].name << " at offset " << off << std::endl;
		}
		auto len=compound.members[n].name.length();
		off+=len;
		while ( (len++ % 8) > 0) {
		  ++off;
		}
		off+=4;
		if (static_cast<int>(datatype.properties[off]) != 0) {
		  if (myerror.length() > 0) {
		    myerror+=", ";
		  }
		  myerror+="unable to decode compound type version "+strutils::itos(datatype.version)+" with dimensionality "+strutils::itos(static_cast<int>(datatype.properties[off]));
		  exit(1);
		}
		off+=28;
		HDF5::decodeDatatype(&datatype.properties[off],l_datatypes[n],show_debug);
		compound.members[n].class_=l_datatypes[n].class_;
		compound.members[n].size=l_datatypes[n].size;
		total_size+=l_datatypes[n].size;
		off+=8+l_datatypes[n].prop_len;
	    }
	    break;
	  }
	  case 3:
	  {
	    if (dim_sizes.size() > 1) {
		std::cerr << "unable to decode compound type with dimensionality "+strutils::itos(dim_sizes.size()) << std::endl;
		exit(1);
	    }
	    auto off=0;
	    for (size_t n=0; n < compound.members.size(); ++n) {
		compound.members[n].name=&(reinterpret_cast<char *>(datatype.properties.get()))[off];
		if (show_debug) {
		  std::cerr << "  name: " << compound.members[n].name << " at offset " << off << std::endl;
		}
		while (datatype.properties[off++] != 0);
		auto len=1;
		auto s=datatype.size;
		while (s/256 > 0) {
		  ++len;
		  s/=256;
		}
		off+=len;
		HDF5::decodeDatatype(&datatype.properties[off],l_datatypes[n],show_debug);
		compound.members[n].class_=l_datatypes[n].class_;
		compound.members[n].size=l_datatypes[n].size;
		total_size+=l_datatypes[n].size;
		off+=8+l_datatypes[n].prop_len;
	    }
	    break;
	  }
	  default:
	  {
	    if (!myerror.empty()) {
		myerror+=", ";
	    }
	    myerror+="unable to decode compound type version "+strutils::itos(datatype.version);
	    return false;
	  }
	}
	value= (dim_sizes.size() > 0) ? new unsigned char[total_size*dim_sizes[0]] : new unsigned char[total_size];
	if (show_debug) {
	  std::cerr << "num values: " << compound.members.size() << std::endl;
	}
	auto off=0;
	auto voff=off;
	auto end= (dim_sizes.size() > 0) ? dim_sizes[0] : 1;
	for (size_t n=0; n < end; ++n) {
	  auto roff=off;
	  for (size_t m=0; m < compound.members.size(); ++m) {
	    DataValue l_value;
	    l_value.set(fs,&buffer[roff],size_of_offsets,size_of_lengths,l_datatypes[m],dataspace,show_debug);
	    if (n == 0) {
		compound.members[m].precision=l_value.precision;
	    }
	    std::copy(reinterpret_cast<unsigned char *>(l_value.value),reinterpret_cast<unsigned char *>(l_value.value)+l_datatypes[m].size,&(reinterpret_cast<unsigned char *>(value))[voff]);
	    roff+=l_datatypes[m].size;
	    voff+=l_datatypes[m].size;
	  }
	  off+=datatype.size;
	}
	l_datatypes=nullptr;
	size=off;
	break;
    }
    case 7:
    {
// reference
	auto type=((datatype.bit_fields[0] & 0xf0) >> 4);
	if (type == 0 || type == 1) {
	  size=size_of_offsets;
	  value=new unsigned char[size];
	  std::copy(buffer,buffer+size,reinterpret_cast<unsigned char *>(value));
	}
	else {
	  if (!myerror.empty()) {
	    myerror+=", ";
	  }
	  myerror+="unable to set class 7 value for bits 0-3="+strutils::itos(type);
	  return false;
	}
	break;
    }
    case 9:
    {
// variable-length
	short type,pad,charset;
	getBits(datatype.bit_fields,type,0,4);
	getBits(datatype.bit_fields,pad,4,4);
	getBits(datatype.bit_fields,charset,8,4);
	if (show_debug) {
	  std::cerr << "decoding data class 9 -  type: " << type << " pad: " << pad << " charset: " << charset << " size: " << datatype.size << " base type class and version: " << static_cast<int>(datatype.properties[0] & 0xf) << "/" << static_cast<int>((datatype.properties[0] & 0xf0) >> 4) << std::endl;
	}
	if (type == 0) {
	  size=datatype.size;
	  precision=size_of_offsets;
	  size_t num_pointers=1;
	  if (dim_sizes.size() > 0) {	
	    num_pointers=dim_sizes.front();
	    size*=dim_sizes.front();
	  }
	  value=new unsigned char[size];
	  vlen.class_=(datatype.properties[0] & 0xf);
	  auto element_size=HDF5::getValue(&datatype.properties[4],4);
// patch for strings stored as sequence of fixed-point numbers
	  if (vlen.class_ == 0 && datatype.size > 1 && element_size == 1) {
	    vlen.class_=3;
	    if (show_debug) {
		std::cerr << "***BASE CLASS changed from 'fixed-point number' to 'string'" << std::endl;
	    }
	  }
	  vlen.size=0;
	  auto off=0;
	  std::vector<int> lengths;
	  for (size_t n=0; n < num_pointers; ++n) {
	    lengths.emplace_back(HDF5::getValue(&buffer[off],4)*element_size);
	    vlen.size+=lengths.back();
	    off+=(8+precision);
	  }
	  vlen.size+=4*num_pointers;
	  vlen.buffer.reset(new unsigned char[vlen.size]);
	  if (show_debug) {
	    std::cerr << "Variable length (class " << vlen.class_ << ") buffer set to " << vlen.size << " bytes" << std::endl;
	  }
	  off=0;
	  auto voff=0;
	  for (size_t n=0; n < num_pointers; ++n) {
	    setBits(&vlen.buffer[voff],lengths[n],0,32);
	    unsigned char *buf2=nullptr;
	    auto len=HDF5::getGlobalHeapObject(fs,size_of_lengths,HDF5::getValue(&buffer[off+4],precision),HDF5::getValue(&buffer[off+4+precision],4),&buf2);
	    std::copy(&buf2[0],&buf2[lengths[n]],&vlen.buffer[voff+4]);
	    if (show_debug) {
		std::cerr << "***LEN=" << len;
		if (vlen.class_ == 7) {
		  std::cerr << " " << HDF5::getValue(buf2,size_of_offsets);
		}
		std::cerr << std::endl;
	    }
	    if (len > 0) {
		delete[] buf2;
	    }
	    off+=(8+precision);
	    voff+=(4+lengths[n]);
	  }
	  std::copy(&buffer[0],&buffer[off],reinterpret_cast<unsigned char *>(value));
	}
	else {
	  if (!myerror.empty()) {
	    myerror+=", ";
	  }
	  myerror+="unable to set class 9 value for type "+strutils::itos(type);
	  return false;
	}
	break;
    }
    default:
    {
	if (!myerror.empty()) {
	  myerror+=", ";
	}
	myerror+="unable to decode data of class "+strutils::itos(datatype.class_);
	return false;
    }
  }
  return true;
}

InputHDF5Stream::Attribute InputHDF5Stream::getAttribute(std::string xpath)
{
  Attribute attr;

  auto xparts=strutils::split(xpath,"@");
  if (xparts.size() != 2) {
    return attr;
  }
  auto attr_name=xparts[1];
  xparts=strutils::split(xpath,"/");
  Group *g=&root_group;
  for (size_t n=1; n < xparts.size()-1; ++n) {
    GroupEntry ge;
    if (!g->groups.found(xparts[n],ge)) {
	return attr;
    }
    g=ge.group.get();
  }
  DatasetEntry dse;
  dse.key=xparts.back();
  strutils::replace_all(dse.key,"@"+attr_name,"");
  if (!g->datasets.found(dse.key,dse)) {
    return attr;
  }
  dse.dataset->attributes.found(attr_name,attr);
  return attr;
}

void InputHDF5Stream::Dataset::free()
{
  for (auto& chunk : data.chunks) {
    if (chunk.buffer != nullptr) {
	chunk.buffer.reset(nullptr);
    }
  }
}

InputHDF5Stream::Dataset *InputHDF5Stream::getDataset(std::string xpath)
{
  Group *g=&root_group;
  auto xparts=strutils::split(xpath,"/");
  for (size_t n=1; n < xparts.size()-1; ++n) {
    GroupEntry ge;
    if (!g->groups.found(xparts[n],ge)) {
	return nullptr;
    }
    g=ge.group.get();
  }
  DatasetEntry dse;
  dse.key=xparts.back();
  if (!g->datasets.found(dse.key,dse)) {
    return nullptr;
  }
  for (size_t n=0; n < dse.dataset->data.chunks.size(); ++n) {
    if (dse.dataset->data.chunks[n].buffer == nullptr) {
	if (!dse.dataset->data.chunks[n].fill(fs,*dse.dataset,show_debug)) {
	  exit(1);
	}
    }
  }
  return dse.dataset.get();
}

std::list<InputHDF5Stream::DatasetEntry> InputHDF5Stream::getDatasetsWithAttribute(std::string attribute_path,Group *g)
{
  std::list<DatasetEntry> return_list,dse_list;
  std::string attribute,value;
  GroupEntry ge;
  DatasetEntry dse;
  Attribute attr;

  auto aparts=strutils::split(attribute_path,"=");
  attribute=aparts[0];
  if (aparts.size() > 1) {
    value=aparts[1];
  }
  if (g == nullptr) {
    g=&root_group;
  }
  for (auto& key : g->groups.keys()) {
    g->groups.found(key,ge);
    dse_list=getDatasetsWithAttribute(attribute_path,ge.group.get());
    return_list.insert(return_list.end(),dse_list.begin(),dse_list.end());
  }
  for (auto& key : g->datasets.keys()) {
    g->datasets.found(key,dse);
    for (auto key2 : dse.dataset->attributes.keys()) {
	if (key2 == attribute) {
	  if (value.length() == 0) {
	    return_list.emplace_back(dse);
	  }
	  else if  (dse.dataset->attributes.found(key2,attr)) {
	    if (attr.value.class_ == 3) {
		if (std::string(reinterpret_cast<char *>(attr.value.value)) == value) {
		  return_list.emplace_back(dse);
		}
	    }
	    else {
		if (myerror.length() > 0) {
		  myerror+=", ";
		}
		myerror+="can't match an attribute value of class "+strutils::itos(attr.value.class_);
	    }
	  }
	  break;
	}
    }
  }
  return return_list;
}

bool InputHDF5Stream::open(const char *filename)
{
  if (is_open()) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="currently connected to another file stream";
    return false;
  }
  fs.open(filename,std::ios::in);
  if (!fs.is_open()) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to open "+std::string(filename);
    return false;
  }
  SymbolTableEntry ste;
  if (!decodeSuperblock(ste.objhdr_addr)) {
    return false;
  }
  if (sb_version == 0 || sb_version == 1) {
    if (!decodeSymbolTableEntry(ste,NULL))
	return false;
  }
  if (!decodeObjectHeader(ste,NULL)) {
    return false;
  }
  if (show_debug) {
    std::cerr << "ROOT GROUP heap address: " << root_group.local_heap.addr << std::endl;
    std::cerr << "ROOT GROUP heap data start address: " << root_group.local_heap.data_start << std::endl;
    std::cerr << "ROOT GROUP b-tree address: " << root_group.btree_addr << std::endl;
  }
  return true;
}

int InputHDF5Stream::peek()
{
return bfstream::error;
}

void InputHDF5Stream::printFillValue(std::string xpath)
{
  auto xparts=strutils::split(xpath,"/");
  Group *g=&root_group;
  for (size_t n=1; n < xparts.size()-1; ++n) {
    GroupEntry ge;
    if (!g->groups.found(xparts[n],ge)) {
	std::cout << "?????";
	return;
    }
    g=ge.group.get();
  }
  DatasetEntry dse;
  dse.key=xparts.back();
  if (!g->datasets.found(dse.key,dse)) {
    std::cout << "?????";
    return;
  }
  if (dse.dataset->fillvalue.bytes != nullptr) {
    HDF5::printDataValue(dse.dataset->datatype,dse.dataset->fillvalue.bytes);
  }
  else {
    std::cout << "Fillvalue(s) not defined";
  }
}

int InputHDF5Stream::read(unsigned char *buffer,size_t buffer_length)
{
return bfstream::error;
}

void InputHDF5Stream::showFileStructure()
{
  printAGroupTree(root_group);
}

void InputHDF5Stream::clearGroups(Group& group)
{
  for (auto& key : group.groups.keys()) {
    GroupEntry ge;
    group.groups.found(key,ge);
    clearGroups(*ge.group);
    ge.group=nullptr;
  }
  group.groups.clear();
  for (auto& key : group.datasets.keys()) {
    DatasetEntry dse;
    group.datasets.found(key,dse);
    if (dse.dataset != nullptr) {
	if (dse.dataset->fillvalue.bytes != nullptr) {
	  delete[] dse.dataset->fillvalue.bytes;
	}
	dse.dataset=nullptr;
    }
  }
  group.datasets.clear();
}

bool InputHDF5Stream::decodeAttribute(unsigned char *buffer,Attribute& attribute,int& length,int ohdr_version)
{
  struct {
    int name,datatype,dataspace;
  } l_sizes;
  int name_off;
  Datatype datatype;
  Dataspace dataspace;

  if ( (l_sizes.name=HDF5::getValue(&buffer[2],2)) == 0) {
    return false;
  }
  if ( (l_sizes.datatype=HDF5::getValue(&buffer[4],2)) == 0) {
    return false;
  }
  if ( (l_sizes.dataspace=HDF5::getValue(&buffer[6],2)) == 0) {
    return false;
  }
  length=8;
  if (show_debug) {
    std::cerr << "decode attribute version " << static_cast<int>(buffer[0]) << " name size: " << l_sizes.name << " datatype size: " << l_sizes.datatype << " dataspace size: " << l_sizes.dataspace << " ohdr_version: " << ohdr_version << std::endl;
  }
  switch (static_cast<int>(buffer[0])) {
    case 1:
    {
	name_off=length;
	length+=l_sizes.name;
	int mod;
	if ( (mod=(l_sizes.name % 8)) > 0) {
	  length+=8-mod;
	}
	if (!HDF5::decodeDatatype(&buffer[length],datatype,show_debug)) {
	  return false;
	}
	length+=l_sizes.datatype;
	if ( (mod=(l_sizes.datatype % 8)) > 0) {
	  length+=8-mod;
	}
	if (!HDF5::decodeDataspace(&buffer[length],sizes.lengths,dataspace,show_debug)) {
	  return false;
	}
	length+=l_sizes.dataspace;
	if ( (mod=(l_sizes.dataspace % 8)) > 0) {
	  length+=8-mod;
	}
	if (!attribute.value.set(fs,&buffer[length],sizes.offsets,sizes.lengths,datatype,dataspace,show_debug)) {
	  return false;
	}
	length+=attribute.value.size;
	if (ohdr_version == 1 && datatype.class_ == 3) {
	  mod=(length % 8);
	  if (mod != 0) {
	    length+=8-mod;
	    if (show_debug) {
		std::cerr << "string length adjusted from " << attribute.value.size << " to " << (attribute.value.size+8-mod) << std::endl;
	    }
	  }
	}
	break;
    }
    case 2:
    {
	name_off=length;
	length+=l_sizes.name;
	if (!HDF5::decodeDatatype(&buffer[length],datatype,show_debug)) {
	  return false;
	}
	length+=l_sizes.datatype;
	if (!HDF5::decodeDataspace(&buffer[length],sizes.lengths,dataspace,show_debug)) {
	  return false;
	}
	length+=l_sizes.dataspace;
	if (!attribute.value.set(fs,&buffer[length],sizes.offsets,sizes.lengths,datatype,dataspace,show_debug)) {
	  return false;
	}
	length+=attribute.value.size;
	break;
    }
    case 3:
    {
	length++;
	name_off=length;
	length+=l_sizes.name;
	if (!HDF5::decodeDatatype(&buffer[length],datatype,show_debug)) {
	  return false;
	}
	length+=l_sizes.datatype;
	if (show_debug) {
	  std::cerr << "C offset: " << length << std::endl;
	}
	if (!HDF5::decodeDataspace(&buffer[length],sizes.lengths,dataspace,show_debug)) {
	  return false;
	}
	length+=l_sizes.dataspace;
	if (dataspace.dimensionality > 1) {
	  if (!myerror.empty()) {
	    myerror+=", ";
	  }
	  myerror+="unable to decode attribute with dimensionality "+strutils::itos(dataspace.dimensionality);
	  return false;
	}
	if (show_debug) {
	  std::cerr << "setting value at offset: " << length << std::endl;
dump(buffer,length+64);
	}
	if (!attribute.value.set(fs,&buffer[length],sizes.offsets,sizes.lengths,datatype,dataspace,show_debug)) {
	  return false;
	}
	length+=attribute.value.size;
	break;
    }
    default:
    {
	return false;
    }
  }
  if (show_debug) {
    std::cerr << "value set: ";
    attribute.value.print(std::cerr,ref_table);
    std::cerr << std::endl;
  }
  attribute.key.assign(reinterpret_cast<char *>(&buffer[name_off]),l_sizes.name);
  if (attribute.key.back() == '\0') {
    attribute.key.pop_back();
  }
  return true;
}

bool InputHDF5Stream::decodeBTree(Group& group,FractalHeapData *frhp_data)
{
  if (show_debug) {
    std::cerr << "seeking " << group.btree_addr << std::endl;
  }
  fs.seekg(group.btree_addr,std::ios_base::beg);
  char buf[4];
  fs.read(buf,4);
  if (fs.gcount() != 4) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error tree signature";
    return false;
  }
  auto signature=std::string(buf,4);
  if (signature == "TREE") {
    return decodeV1BTree(group);
  }
  else if (signature == "BTHD") {
    return decodeV2BTree(group,frhp_data);
  }
  else {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unrecognized tree signature '"+std::string(buf,4)+"'";
  }
  return false;
}

bool InputHDF5Stream::decodeFractalHeap(unsigned long long address,int header_message_type,FractalHeapData& frhp_data)
{
  if (address == undef_addr) {
    if (header_message_type != 0x0006) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="fractal heap is not defined";
    }
    return false;
  }
  auto curr_off=fs.tellg();
  fs.seekg(address,std::ios_base::beg);
  unsigned char buf[64];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,5);
  if (fs.gcount() != 5) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error bytes 1-5 in fractal heap";
    return false;
  }
  if (std::string(reinterpret_cast<char *>(buf),4) != "FRHP") {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="not a fractal heap";
    return false;
  }
  if (buf[4] != 0) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to decode fractal heap version "+strutils::itos(static_cast<int>(buf[4]));
    return false;
  }
  if (show_debug) {
    std::cerr << "yay! fractal heap" << std::endl;
  }
  fs.read(cbuf,9);
  if (fs.gcount() != 9) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error bytes 6-14 in fractal heap";
    return false;
  }
  frhp_data.id_len=HDF5::getValue(buf,2);
  if (show_debug) {
    std::cerr << "id_len: " << frhp_data.id_len << std::endl;
  }
  frhp_data.io_filter_size=HDF5::getValue(&buf[2],2);
  if (show_debug) {
    std::cerr << "io_filter_size: " << frhp_data.io_filter_size << std::endl;
  }
  frhp_data.flags=buf[4];
  frhp_data.max_managed_obj_size=HDF5::getValue(&buf[5],4);
  if (show_debug) {
    std::cerr << "max size of managed objects: " << frhp_data.max_managed_obj_size << std::endl;
  }
  fs.read(cbuf,sizes.lengths+sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.lengths+sizes.offsets)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to read huge object data";
    return false;
  }
  fs.read(cbuf,sizes.lengths+sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.lengths+sizes.offsets)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to read free space data";
    return false;
  }
  fs.read(cbuf,sizes.lengths*4);
  if (fs.gcount() != static_cast<int>(sizes.lengths*4)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to read managed space data";
    return false;
  }
  frhp_data.objects.num_managed=HDF5::getValue(&buf[sizes.lengths*3],sizes.lengths);
  if (show_debug) {
    std::cerr << "number of managed objects in heap: " << frhp_data.objects.num_managed << std::endl;
  }
  fs.read(cbuf,sizes.lengths*4);
  if (fs.gcount() != static_cast<int>(sizes.lengths*4)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to read huge/tiny object data";
    return false;
  }
  frhp_data.objects.num_huge=HDF5::getValue(&buf[sizes.lengths],sizes.lengths);
  frhp_data.objects.num_tiny=HDF5::getValue(&buf[sizes.lengths*3],sizes.lengths);
  if (show_debug) {
    std::cerr << "number of huge objects: " << frhp_data.objects.num_huge << std::endl;
    std::cerr << "number of tiny objects: " << frhp_data.objects.num_tiny << std::endl;
  }
  fs.read(cbuf,4+sizes.lengths*2);
  if (fs.gcount() != static_cast<int>(4+sizes.lengths*2)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to read table width and block size data";
    return false;
  }
  frhp_data.table_width=HDF5::getValue(buf,2);
  if (show_debug) {
    std::cerr << "table width=" << frhp_data.table_width << std::endl;
  }
  frhp_data.start_block_size=HDF5::getValue(&buf[2],sizes.lengths);
  if (show_debug) {
    std::cerr << "starting block size: " << frhp_data.start_block_size << std::endl;
  }
  frhp_data.max_dblock_size=HDF5::getValue(&buf[2+sizes.lengths],sizes.lengths);
  if (show_debug) {
    std::cerr << "max direct block size: " << frhp_data.max_dblock_size << std::endl;
  }
  frhp_data.max_dblock_rows=static_cast<int>(log2(HDF5::getValue(&buf[2+sizes.lengths],sizes.lengths))-log2(HDF5::getValue(&buf[2],sizes.lengths)))+2;
  if (show_debug) {
    std::cerr << "max_dblock_rows=" << frhp_data.max_dblock_rows << std::endl;
  }
  frhp_data.max_size=HDF5::getValue(&buf[2+sizes.lengths*2],2);
  if (show_debug) {
    std::cerr << "max_heap_size: " << frhp_data.max_size << std::endl;
  }
  fs.read(cbuf,4+sizes.offsets);
  if (fs.gcount() != static_cast<int>(4+sizes.offsets)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to read block data";
    return false;
  }
  if (show_debug) {
    std::cerr << "start # rows: " << HDF5::getValue(buf,2) << std::endl;
  }
  unsigned long long root_block_addr=HDF5::getValue(&buf[2],sizes.offsets);
  if (show_debug) {
    std::cerr << "address of root block: " << root_block_addr << std::endl;
  }
  frhp_data.nrows=HDF5::getValue(&buf[2+sizes.offsets],2);
  if (show_debug) {
    std::cerr << "nrows=" << frhp_data.nrows << std::endl;
  }
  frhp_data.K= (frhp_data.nrows < frhp_data.max_dblock_rows) ? frhp_data.nrows : frhp_data.max_dblock_rows;
  frhp_data.K*=frhp_data.table_width;
  frhp_data.N=frhp_data.K-(frhp_data.max_dblock_rows*frhp_data.table_width);
  if (frhp_data.N < 0) {
    frhp_data.N=0;
  }
  if (show_debug) {
    std::cerr << "K=" << frhp_data.K << " N=" << frhp_data.N << std::endl;
  }
  frhp_data.curr_row=frhp_data.curr_col=1;
  if (!decodeFractalHeapBlock(root_block_addr,header_message_type,frhp_data)) {
    return false;
  }
  fs.seekg(curr_off,std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decodeFractalHeapBlock(unsigned long long address,int header_message_type,FractalHeapData& frhp_data)
{
  if (address == undef_addr) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="fractal heap block is not defined";
    return false;
  }
  auto curr_off=fs.tellg();
  fs.seekg(address,std::ios_base::beg);
  char buf[32];
  fs.read(buf,5);
  if (fs.gcount() != 5) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error bytes 1-5 in fractal heap block";
    return false;
  }
  auto signature=std::string(buf,4);
  if (signature == "FHDB") {
    if (show_debug) {
	std::cerr << "fractal heap direct block" << std::endl;
    }
    if (buf[4] != 0) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to decode fractal heap direct block version "+strutils::itos(static_cast<int>(buf[4]));
	return false;
    }
    int size_to_read=(frhp_data.curr_row-1)*frhp_data.start_block_size;
    if (size_to_read == 0) {
	size_to_read=frhp_data.start_block_size;
    }
    size_t idum=(frhp_data.max_size+7)/8;
    fs.read(buf,sizes.offsets+idum);
    if (fs.gcount() != static_cast<int>(sizes.offsets+idum)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to read FHDB heap header address and block offset";
	return false;
    }
    size_to_read-=(5+sizes.offsets+idum);
    if (show_debug) {
	std::cerr << "heap header address: " << HDF5::getValue(reinterpret_cast<unsigned char *>(buf),sizes.offsets) << std::endl;
	std::cerr << "block offset: " << HDF5::getValue(&(reinterpret_cast<unsigned char *>(buf))[sizes.offsets],idum) << "  flags: " << static_cast<int>(frhp_data.flags) << std::endl;
    }
    if ( (frhp_data.flags & 0x2) == 0x2) {
	fs.read(buf,4);
	if (fs.gcount() != 4) {
	  if (myerror.length() > 0)
	    myerror+=", ";
	  myerror+="unable to read FHDB checksum";
	  return false;
	}
	size_to_read-=4;
    }
    if (show_debug) {
	std::cerr << "size to read should be: " << size_to_read << "  hdr msg type: " << header_message_type << std::endl;
    }
    auto *buf2=new unsigned char[size_to_read];
    fs.read(reinterpret_cast<char *>(buf2),size_to_read);
    if (fs.gcount() != static_cast<int>(size_to_read)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to read FHDB object data";
	return false;
    }
    if (show_debug) {
	dump(buf2,size_to_read);
    }
    switch (header_message_type) {
	case 6:
	{
	  auto local_off=0;
	  bool is_subgroup=false;
	  while (buf2[local_off] == 1 && local_off < size_to_read) {
	    local_off+=decodeHeaderMessage("",0,6,0,0,&buf2[local_off],&root_group,*(frhp_data.dse),is_subgroup);
	  }
	  break;
	}
	case 12:
	{
	  auto local_off=0;
	  while (local_off < size_to_read) {
	    Attribute attr;
	    int n;
	    if (decodeAttribute(&buf2[local_off],attr,n,2)) {
		if (show_debug) {
		  std::cerr << "ATTRIBUTE (1) of '" << frhp_data.dse->key << "' " << &root_group << " " << frhp_data.dse->dataset << std::endl;
		  std::cerr << "  name: " << attr.key << "  dataset: " << frhp_data.dse->dataset << std::endl;
		}
		if (!frhp_data.dse->dataset->attributes.found(attr.key,attr)) {
		  frhp_data.dse->dataset->attributes.insert(attr);
		  if (show_debug) {
		    std::cerr << "added" << std::endl;
		  }
		}
		local_off+=n;
	    }
	    else {
		++local_off;
	    }
	  }
	  break;
	}
	default:
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="unable to decode FHDB object data for message type "+strutils::itos(header_message_type);
	  return false;
    }
    delete[] buf2;
    frhp_data.curr_col++;
    if (frhp_data.curr_col > frhp_data.table_width) {
	frhp_data.curr_col=1;
	++frhp_data.curr_row;
    }
  }
  else if (signature == "FHIB") {
    if (show_debug) {
	std::cerr << "fractal heap indirect block" << std::endl;
    }
    if (buf[4] != 0) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to decode fractal heap indirect block version "+strutils::itos(static_cast<int>(buf[4]));
	return false;
    }
    fs.read(buf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to read FHIB heap header address";
	return false;
    }
    if (show_debug) {
	std::cerr << "heap header address: " << HDF5::getValue(reinterpret_cast<unsigned char *>(buf),sizes.offsets) << std::endl;
    }
    size_t idum=(frhp_data.max_size+7)/8;
    fs.read(buf,idum);
    if (fs.gcount() != static_cast<int>(idum)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to FHIB block offset";
	return false;
    }
    if (show_debug) {
	std::cerr << "block offset: " << HDF5::getValue(reinterpret_cast<unsigned char *>(buf),idum) << std::endl;
    }
    for (int n=0; n < frhp_data.K; ++n) {
	fs.read(buf,sizes.offsets);
	if (fs.gcount() != static_cast<int>(sizes.offsets)) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="unable to read FHIB child direct block "+strutils::itos(n);
	  return false;
	}
	unsigned long long block_addr=HDF5::getValue(reinterpret_cast<unsigned char *>(buf),sizes.offsets);
	if (show_debug) {
	  std::cerr << "child direct block " << n << " addr: " << block_addr << std::endl;
	}
	if (block_addr != undef_addr) {
	  decodeFractalHeapBlock(block_addr,header_message_type,frhp_data);
	}
	if (frhp_data.io_filter_size > 0) {
	  fs.read(buf,sizes.offsets+sizes.lengths);
	  if (fs.gcount() != static_cast<int>(sizes.offsets+sizes.lengths)) {
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to read FHIB filter data for direct block "+strutils::itos(n);
	    return false;
	  }
	}
    }
  }
  else {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="not a fractal heap block";
    return false;
  }
  fs.seekg(curr_off,std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decodeHeaderMessages(int ohdr_version,size_t header_size,std::string ident,Group *group,DatasetEntry *dse_in,unsigned char flags)
{
  std::unique_ptr<unsigned char[]> buf;
  size_t hdr_off=0;
  int type,len;
  DatasetEntry dse;
  bool is_subgroup=false;

  buf.reset(new unsigned char[header_size]);
  if (dse_in == nullptr) {
    if (group == nullptr) {
	if (!root_group.datasets.found(ident,dse)) {
	  dse.key=ident;
	  dse.dataset.reset(new Dataset);
	}
    }
    else {
	if (group != nullptr && !group->datasets.found(ident,dse)) {
	  dse.key=ident;
	  dse.dataset.reset(new Dataset);
	}
    }
  }
  else {
    dse=*dse_in;
    if (show_debug) {
	std::cerr << "ready to read header messages at offset " << fs.tellg() << " size: " << header_size << " ident: '" << ident << "' GROUP: " << group << " flags: " << static_cast<int>(flags) << std::endl;
    }
  }
  fs.read(reinterpret_cast<char *>(buf.get()),header_size);
  if (fs.gcount() != static_cast<int>(header_size)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error while trying to get header messages";
    return false;
  }
  if (std::string(reinterpret_cast<char *>(buf.get()),4) == "OCHK") {
    hdr_off+=4;
    header_size-=4;
  }
  if (show_debug) {
    std::cerr << "object header version: " << ohdr_version << std::endl;
  }
  while ( (hdr_off+4) < header_size) {
    if (show_debug) {
	std::cerr << "hdr_off: " << hdr_off << " header size: " << header_size << std::endl;
    }
    if (sb_version == 0 || sb_version == 1) {
	type=HDF5::getValue(&buf[hdr_off],2);
	len=HDF5::getValue(&buf[hdr_off+2],2);
	if (show_debug) {
	  std::cerr << "flags: " << static_cast<int>(buf[hdr_off+4]) << std::endl;
	}
	hdr_off+=8;
    }
    else if (sb_version == 2) {
	type=static_cast<int>(buf[hdr_off]);
	len=HDF5::getValue(&buf[hdr_off+1],2);
	if (show_debug) {
	  std::cerr << "flags: " << static_cast<int>(buf[hdr_off+3]) << std::endl;
	}
	hdr_off+=4;
	if ( (flags & 0x4) == 0x4) {
	  hdr_off+=2;
	}
    }
    else {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to read header messages for version "+strutils::itos(sb_version);
	return false;
    }
    if (show_debug) {
	long long off=fs.tellg();
	std::cerr << "HEADER MESSAGE TYPE: " << type << " " << " " << len << " " << hdr_off << " header size: " << header_size << " file offset: " << off-header_size << " " << time(NULL) << std::endl;
	dump(buf.get(),hdr_off+len);
    }
    hdr_off+=decodeHeaderMessage(ident,ohdr_version,type,len,flags,&buf[hdr_off],group,dse,is_subgroup);
  }
  buf=nullptr;
  if (!is_subgroup) {
    if (group == nullptr) {
	group=&root_group;
    }
    if (show_debug) {
	std::cerr << "-- NOT SUBGROUP '" << ident << "' " << group << std::endl;
    }
    if (!group->datasets.found(ident,dse)) {
	dse.key=ident;
	if (show_debug) {
	  std::cerr << "  ADDING DATASET '" << ident << "' TO " << group << std::endl;
	}
	group->datasets.insert(dse);
    }
    if (show_debug) {
	std::cerr << "  HERE " << dse.dataset << " '" << dse.key << "' '" << ident << "'" << std::endl;
    }
  }
  else {
    if (dse_in == nullptr) {
	dse.dataset=nullptr;
    }
  }
  return true;
}

int InputHDF5Stream::decodeHeaderMessage(std::string ident,int ohdr_version,int type,int length,unsigned char flags,unsigned char *buffer,Group *group,DatasetEntry& dse,bool& is_subgroup)
{
  unsigned char *buf2=NULL;
  unsigned long long curr_off,ldum;
  int n,idum,nvals;
  Attribute attr;
  FractalHeapData *frhp_data;
  SymbolTableEntry *ste;
  ReferenceEntry re;
  short dimensionality;

  switch (type) {
    case 0x0000:
	return length;
    case 0x0001:
// Dataspace
	if (show_debug) {
	  std::cerr << "DATASPACE" << std::endl;
	}
	HDF5::decodeDataspace(buffer,sizes.lengths,dse.dataset->dataspace,show_debug);
	return length;
    case 0x0002:
// Link info
 	if (buffer[0] != 0) {
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to decode link info for version number "+strutils::itos(static_cast<int>(buffer[0]));
	    exit(1);
	}
	if (show_debug) {
	  std::cerr << "Link Info:" << std::endl;
	  std::cerr << "  version: " << static_cast<int>(buffer[0]) << " flags: " << static_cast<int>(buffer[1]) << std::endl;
	}
	curr_off=2;
	if (show_debug) {
	  std::cerr << "curr_off is " << curr_off << std::endl;
	}
	if ( (buffer[1] & 0x1) == 0x1) {
	  if (show_debug) {
	    std::cerr << "max creation index: " << HDF5::getValue(&buffer[curr_off],8) << std::endl;
	  }
	  curr_off+=8;
	}
	if (show_debug) {
	  std::cerr << "fractal heap addr: " << HDF5::getValue(&buffer[curr_off],sizes.offsets) << std::endl;
	}
	frhp_data=new FractalHeapData;
	frhp_data->dse=&dse;
	if (decodeFractalHeap(HDF5::getValue(&buffer[curr_off],sizes.offsets),0x0006,*frhp_data)) {
	  root_group.btree_addr=HDF5::getValue(&buffer[curr_off+sizes.offsets],sizes.offsets);
	  if (show_debug) {
	    std::cerr << "b-tree name index addr: " << root_group.btree_addr << std::endl;
	  }
	  if (!decodeBTree(root_group,frhp_data)) {
	    exit(1);
	  }
	}
	if (show_debug) {
	  if ( (buffer[1] & 0x2) == 0x2) {
	    std::cerr << "b-tree creation order index addr: " << HDF5::getValue(&buffer[curr_off+sizes.offsets*2],sizes.offsets) << std::endl;
	  }
	}
	delete frhp_data;
	return length;
    case 0x0003:
// Datatype
	if (show_debug) {
	  std::cerr << "DATATYPE" << std::endl;
	}
	HDF5::decodeDatatype(buffer,dse.dataset->datatype,show_debug);
	return length;
    case 0x0004:
    {
// Fill value (deprecated)
	if (show_debug) {
	  std::cerr << "FILLVALUE (deprecated) '" << dse.key << "'" << std::endl;
	}
	if (dse.dataset->fillvalue.length == 0) {
	  if ( (dse.dataset->fillvalue.length=HDF5::getValue(&buffer[0],4)) > 0) {
	    dse.dataset->fillvalue.bytes=new unsigned char[dse.dataset->fillvalue.length];
	    std::copy(buffer+4,buffer+4+dse.dataset->fillvalue.length,dse.dataset->fillvalue.bytes);
	  }
	}
	return length;
    }
    case 0x0005:
    {
// Fill value
	if (show_debug) {
	  std::cerr << "FILLVALUE '" << dse.key << "'" << std::endl;
	}
	switch (static_cast<int>(buffer[0])) {
	  case 2:
	    if (static_cast<int>(buffer[3]) == 1) {
		dse.dataset->fillvalue.length=HDF5::getValue(&buffer[4],4);
		if (dse.dataset->fillvalue.length > 0) {
		  dse.dataset->fillvalue.bytes=new unsigned char[dse.dataset->fillvalue.length];
		  std::copy(buffer+8,buffer+8+dse.dataset->fillvalue.length,dse.dataset->fillvalue.bytes);
		}
		else {
		  dse.dataset->fillvalue.bytes=NULL;
		}
	    }
	    else {
		if (myerror.length() > 0) {
		  myerror+=", ";
		}
		myerror+="fill value undefined";
		exit(1);
	    }
	    break;
	  case 3:
	    if ( (buffer[1] & 0x20) == 0x20) {
		dse.dataset->fillvalue.length=HDF5::getValue(&buffer[2],4);
		if (dse.dataset->fillvalue.length > 0) {
		  dse.dataset->fillvalue.bytes=new unsigned char[dse.dataset->fillvalue.length];
		  std::copy(buffer+6,buffer+6+dse.dataset->fillvalue.length,dse.dataset->fillvalue.bytes);
		}
		else {
		  dse.dataset->fillvalue.bytes=NULL;
		}
	    }
	    else {
		dse.dataset->fillvalue.length=0;
		dse.dataset->fillvalue.bytes=NULL;
	    }
	    break;
	  default:
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to decode version "+strutils::itos(static_cast<int>(buffer[0]))+" Fill Value message";
	    exit(1);
	}
	return length;
    }
    case 0x0006:
// Link message
	int link_type;
	curr_off=2;
	link_type=0;
	if ( (buffer[1] & 0x8) == 0x8) {
// Link type
	  link_type=buffer[curr_off++];
	}
	if (link_type != 0) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="unable to decode links that are not hard links";
	  exit(1);
	}
	if ( (buffer[1] & 0x4) == 0x4) {
// Creation order
	  curr_off+=8;
	}
	if ( (buffer[1] & 0x10) == 0x10) {
// Link name character set
	  curr_off++;
	}
// Length of link name
	if ( (buffer[1] & 0x3) == 0) {
	  ldum=buffer[curr_off];
	  curr_off++;
	}
	else if ( (buffer[1] & 0x3) == 1) {
	  ldum=HDF5::getValue(&buffer[curr_off],2);
	  curr_off+=2;
	}
	else if ( (buffer[1] & 0x3) == 2) {
	  ldum=HDF5::getValue(&buffer[curr_off],4);
	  curr_off+=4;
	}
	else if ( (buffer[1] & 0x3) == 3) {
	  ldum=HDF5::getValue(&buffer[curr_off],8);
	  curr_off+=8;
	}
	ste=new SymbolTableEntry;
	ste->linkname.assign(reinterpret_cast<char *>(&buffer[curr_off]),ldum);
	if (show_debug) {
	  std::cerr << "Link name: '" << ste->linkname << "'" << std::endl;
	}
	curr_off+=ldum;
	if (link_type == 0) {
	  ste->objhdr_addr=HDF5::getValue(&buffer[curr_off],sizes.offsets);
	  if (show_debug) {
	    std::cerr << "Address of link object header: " << ste->objhdr_addr << std::endl;
	  }
	  if (!decodeObjectHeader(*ste,&root_group)) {
	    exit(1);
	  }
	  curr_off+=sizes.offsets;
	  if (ref_table == nullptr) {
	    ref_table.reset(new my::map<ReferenceEntry>);
	  }
	  re.key=ste->objhdr_addr;
	  if (!ref_table->found(re.key,re)) {
	    re.name=ste->linkname;
	    ref_table->insert(re);
	  }
	}
	delete ste;
	return curr_off;
	break;
    case 0x0008:
// Layout
	if (show_debug) {
	  std::cerr << "LAYOUT" << std::endl;
	}
	switch (static_cast<int>(buffer[0])) {
	  case 3:
	    switch (static_cast<int>(buffer[1])) {
		case 1:
		  dse.dataset->data.address=HDF5::getValue(&buffer[2],sizes.offsets);
		  dse.dataset->data.sizes.emplace_back(HDF5::getValue(&buffer[2+sizes.offsets],sizes.lengths));
		  if (show_debug) {
		    std::cerr << "CONTIGUOUS DATA at " << dse.dataset->data.address << " of length " << dse.dataset->data.sizes.back() << std::endl;
		  }
		  break;
		case 2:
		  dse.dataset->data.address=HDF5::getValue(&buffer[3],sizes.offsets);
		  dimensionality=buffer[2]-1;
		  if (show_debug) {
		    std::cerr << "CHUNKED DATA of dimensionality " << dimensionality << " at address " << dse.dataset->data.address << std::endl;
		  }
		  for (n=0; n < dimensionality; ++n) {
		    dse.dataset->data.sizes.emplace_back(HDF5::getValue(&buffer[3+sizes.offsets+n*4],4));
		    if (show_debug) {
			std::cerr << "  dim: " << n << " size: " << dse.dataset->data.sizes.back() << std::endl;
		    }
		  }
		  dse.dataset->data.size_of_element=HDF5::getValue(&buffer[3+sizes.offsets+dimensionality*4],4);
		  if (show_debug) {
		    std::cerr << "  dataset element size: " << dse.dataset->data.size_of_element << " at offset " << 3+sizes.offsets+dimensionality*4 << std::endl;
		  }
		  if (dse.dataset->data.address != undef_addr) {
		    decodeV1BTree(dse.dataset->data.address,*dse.dataset);
		  }
		  break;
		default:
		  if (myerror.length() > 0) {
		    myerror+=", ";
		  }
		  myerror+="unable to decode layout class "+strutils::itos(static_cast<int>(buffer[1]));
		  exit(1);
	    }
	    break;
	  default:
	    if (myerror.length() > 0)
		myerror+=", ";
	    myerror+="unable to decode layout version "+strutils::itos(static_cast<int>(buffer[0]));
	    exit(1);
	}
	return length;
    case 0x000a:
// Group info
 	if (buffer[0] != 0) {
	    if (myerror.length() > 0)
		myerror+=", ";
	    myerror+="unable to decode new-style group info for version number "+strutils::itos(static_cast<int>(buffer[0]));
	    exit(1);
	}
	if (show_debug) {
	  std::cerr << "Group Info:" << std::endl;
	}
	curr_off=2;
	if ( (buffer[1] & 0x1) == 0x1) {
	  if (show_debug) {
	    std::cerr << "max # compact links: " << HDF5::getValue(&buffer[curr_off],2) << std::endl;
	    std::cerr << "max # dense links: " << HDF5::getValue(&buffer[curr_off+2],2) << std::endl;
	  }
	  curr_off+=4;
	}
	if ( (buffer[1] & 0x2) == 0x2) {
	  if (show_debug) {
	    std::cerr << "estimated # of entries: " << HDF5::getValue(&buffer[curr_off],2) << std::endl;
	    std::cerr << "estimated link name length: " << HDF5::getValue(&buffer[curr_off+2],2) << std::endl;
	  }
	  curr_off+=4;
	}
	return length;
    case 0x000b:
// Filter pipeline
	switch(buffer[0]) {
	  case 1:
	    curr_off=8;
	    break;
	  case 2:
	    curr_off=2;
	    break;
	  default:
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to decode filter pipeline version "+strutils::itos(buffer[0]);
	    exit(1);
	}
	for (n=0; n < static_cast<int>(buffer[1]); ++n) {
	  dse.dataset->filters.push_front(HDF5::getValue(&buffer[curr_off],2));
	  if (buffer[0] == 1 || dse.dataset->filters.front() >= 256) {
// name length
	    idum=HDF5::getValue(&buffer[curr_off+2],2);
	    curr_off+=2;
	  }
	  else {
	    idum=0;
	  }
	  nvals=HDF5::getValue(&buffer[curr_off+4],2);
	  curr_off+=6+idum+4*nvals;
	  if (buffer[0] == 1 && (nvals % 2) != 0) {
	    curr_off+=4;
	  }
	  if (show_debug) {
	    std::cerr << "filter " << n << " of " << static_cast<int>(buffer[1]) << " id: " << dse.dataset->filters.front() << std::endl;
	  }
	}
	if (show_debug) {
	  std::cerr << "filter pipeline message:" << std::endl;
	  dump(buffer,length);
	}
	return length;
    case 0x000c:
// Attribute
	if (show_debug) {
	  std::cerr << "ATTRIBUTE (2) of '" << dse.key << "' " << group << " " << dse.dataset << std::endl;
	}
	if (!decodeAttribute(buffer,attr,n,ohdr_version)) {
	  exit(1);
	}
	if (show_debug) {
	  std::cerr << "  name: " << attr.key << "  dataset: " << dse.dataset << std::endl;
	  std::cerr << &(dse.dataset->attributes) << std::endl;
	  std::cerr << dse.dataset->attributes.size() << std::endl;
	}
	dse.dataset->attributes.insert(attr);
	if (show_debug) {
	  std::cerr << "added" << std::endl;
	}
	if (n != length) {
	  if (show_debug) {
	    std::cerr << "calculated and returning " << n << " but length from header message was " << length << std::endl;
	  }
	  while (n < length && buffer[n] == 0x0) {
	    ++n;
	  }
	}
//	return length;
return n;
    case 0x0010:
// Object Header Continuation
	curr_off=fs.tellg();
	ldum=HDF5::getValue(buffer,sizes.offsets);
	length=sizes.offsets;
	fs.seekg(ldum,std::ios_base::beg);
	if (show_debug) {
	  std::cerr << "branching to continuation block at " << ldum << " from " << curr_off << std::endl;
	}
	ldum=HDF5::getValue(&buffer[length],sizes.offsets);
	length+=sizes.offsets;
	if (!decodeHeaderMessages(ohdr_version,ldum,ident,group,&dse,flags)) {
	  exit(1);
	}
	fs.seekg(curr_off,std::ios_base::beg);
	if (show_debug) {
	  std::cerr << "done with continuation block, back to " << fs.tellg() << std::endl;
	}
	return length;
    case 0x0011: {
	Group *g;
	GroupEntry ge;
	if (group == nullptr) {
	  g=&root_group;
	}
	else {
	  ge.key=ident;
	  ge.group.reset(new Group);
	  if (show_debug) {
	    std::cerr << "SUBGROUP '" << ident << "' " << ge.group << std::endl;
	  }
	  group->groups.insert(ge);
	  g=ge.group.get();
	}
	ldum=HDF5::getValue(buffer,sizes.offsets);
	if (show_debug) {
	  std::cerr << "v1 Btree address: " << ldum << std::endl;
	}
	g->btree_addr=ldum;
	length=sizes.offsets;
	ldum=HDF5::getValue(&buffer[length],sizes.offsets);
	if (show_debug) {
	  std::cerr << "local heap address: " << ldum << std::endl;
	}
	g->local_heap.addr=ldum;
	length+=sizes.offsets;
	curr_off=fs.tellg();
	fs.seekg(g->local_heap.addr,std::ios_base::beg);
	ldum=8+2*sizes.lengths+sizes.offsets;
	buf2=new unsigned char[ldum];
	fs.read(reinterpret_cast<char *>(buf2),ldum);
	if (fs.gcount() != static_cast<int>(ldum)) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="read error while trying to get local heap header";
	  exit(1);
	}
	g->local_heap.data_start=HDF5::getValue(&buf2[8+2*sizes.lengths],sizes.offsets);
	if (g->local_heap.data_start == 0) {
	  g->local_heap.data_start=g->local_heap.addr+8+2*sizes.lengths+sizes.offsets;
	}
	delete[] buf2;
	if (!decodeBTree(*g,NULL)) {
	    exit(1);
	}
	fs.seekg(curr_off,std::ios_base::beg);
	is_subgroup=true;
	return length;
    }
    case 0x0012:
// Modification time
	if (show_debug) {
	  std::cerr << "WARNING: ignoring object modification time" << std::endl;
	}
	return length;
    case 0x0015:
// Attribute info
	n=2;
	if ( (buffer[1] & 0x1) == 0x1) {
	  n+=2;
	}
	ldum=HDF5::getValue(&buffer[n],sizes.offsets);
	if (ldum == undef_addr) {
	  return length;
	}
	if (show_debug) {
	  dump(buffer,n+sizes.offsets*2);
	  std::cerr << ldum << " " << HDF5::getValue(&buffer[n+sizes.offsets],sizes.offsets) << std::endl;
	  std::cerr << "****************" << std::endl;
	}
	frhp_data=new FractalHeapData;
	frhp_data->dse=&dse;
	if (!decodeFractalHeap(ldum,0x000c,*frhp_data)) {
	  if (show_debug) {
	    std::cerr << "compact link" << std::endl;
	  }
	}
	if (show_debug) {
	  std::cerr << "****************" << std::endl;
	}
	delete frhp_data;
	return length;
    default:
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unknown message header type "+strutils::itos(type)+" of length "+strutils::itos(length);
	exit(1);
  }
}

bool InputHDF5Stream::decodeObjectHeader(SymbolTableEntry& ste,Group *group)
{
  unsigned long long start_off,curr_off;
  int version,num_msgs,ref_cnt;
  size_t hdr_size;
  unsigned char flags;

  start_off=fs.tellg();
  fs.seekg(ste.objhdr_addr,std::ios_base::beg);
  unsigned char buf[12];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,4);
  if (fs.gcount() != 4) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to decode first 4 bytes of object header";
    return false;
  }
  if (std::string(reinterpret_cast<char *>(buf),4) == "OHDR") {
    fs.read(cbuf,2);
    if (fs.gcount() != 2) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to get version for OHDR";
	return false;
    }
    version=static_cast<int>(buf[0]);
    if (version == 2) {
	flags=buf[1];
	if ( (flags & 0x20) == 0x20) {
	  fs.read(cbuf,4);
	  if (fs.gcount() != 4) {
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to read OHDR access time";
	    return false;
	  }
	  if (show_debug) {
	    std::cerr << "access time: " << HDF5::getValue(buf,4) << std::endl;
	  }
	  fs.read(cbuf,4);
	  if (fs.gcount() != 4) {
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to read OHDR modification time";
	    return false;
	  }
	  if (show_debug) {
	    std::cerr << "mod time: " << HDF5::getValue(buf,4) << std::endl;
	  }
	  fs.read(cbuf,4);
	  if (fs.gcount() != 4) {
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to read OHDR change time";
	    return false;
	  }
	  if (show_debug) {
	    std::cerr << "change time: " << HDF5::getValue(buf,4) << std::endl;
	  }
	  fs.read(cbuf,4);
	  if (fs.gcount() != 4) {
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to read OHDR birth time";
	    return false;
	  }
	  if (show_debug) {
	    std::cerr << "birth time: " << HDF5::getValue(buf,4) << std::endl;
	  }
	}
	if ( (flags & 0x10) == 0x10) {
	  fs.read(cbuf,4);
	  if (fs.gcount() != 4) {
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to read OHDR attribute data";
	    return false;
	  }
	  if (show_debug) {
	    std::cerr << "max # compact attributes: " << HDF5::getValue(buf,2) << std::endl;
	    std::cerr << "min # dense attributes: " << HDF5::getValue(&buf[2],2) << std::endl;
	  }
	}
	hdr_size=pow(2.,static_cast<int>(flags & 0x3));
	fs.read(cbuf,hdr_size);
	if (fs.gcount() != static_cast<int>(hdr_size)) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="unable to read OHDR size of chunk #0";
	  return false;
	}
	hdr_size=HDF5::getValue(buf,hdr_size);
	if (show_debug) {
	  std::cerr << "flags=" << static_cast<int>(flags) << " " << static_cast<int>(flags & 0x3) << " " << pow(2.,static_cast<int>(flags & 0x3)) << " " << hdr_size << std::endl;
	}
	curr_off=fs.tellg();
	if (!decodeHeaderMessages(2,hdr_size,ste.linkname,group,NULL,flags)) {
	  return false;
	}
	fs.seekg(curr_off+hdr_size,std::ios_base::beg);
    }
    else {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to decode OHDR at addr="+strutils::lltos(ste.objhdr_addr);
	return false;
    }
  }
  else {
    version=static_cast<int>(buf[0]);
    if (version == 1) {
	num_msgs=HDF5::getValue(&buf[2],2);
	fs.read(cbuf,8);
	if (fs.gcount() != 8) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="unable to decode next 8 bytes of object header";
	  return false;
	}
	ref_cnt=HDF5::getValue(buf,4);
	hdr_size=HDF5::getValue(&buf[4],4);
	if (show_debug) {
	  std::cerr << "object header version=" << version << " num_msgs=" << num_msgs << " ref_cnt=" << ref_cnt << " hdr_size=" << hdr_size << std::endl;
	}
	fs.seekg(4,std::ios_base::cur);
	curr_off=fs.tellg();
	if (!decodeHeaderMessages(1,hdr_size,ste.linkname,group,NULL,0)) {
	  return false;
	}
	fs.seekg(curr_off+hdr_size,std::ios_base::beg);
    }
    else {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="expected version 1 but got version "+strutils::itos(version);
	return false;
    }
  }
  fs.seekg(start_off,std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decodeSuperblock(unsigned long long& objhdr_addr)
{
  unsigned char buf[12];
  char *cbuf=reinterpret_cast<char *>(buf);
// check for 0x894844460d0a1a0a in first eight bytes (\211HDF\r\n\032\n)
  fs.read(cbuf,12);
  if (fs.gcount() != 12) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error in first 12 bytes";
    return false;
  }
  unsigned long long magic;
  getBits(buf,magic,0,64);
  if (magic != 0x894844460d0a1a0a) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="not an HDF5 file";
    return false;
  }
  sb_version=static_cast<int>(buf[8]);
  if (sb_version == 0 || sb_version == 1) {
    if (show_debug) {
	std::cerr << "sb_version=" << sb_version << std::endl;
    }
    fs.read(cbuf,12);
    if (fs.gcount() != 12) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error in second 12 bytes of V0/1 superblock";
	return false;
    }
    sizes.offsets=static_cast<int>(buf[1]);
    createMask(undef_addr,sizes.offsets*8);
    sizes.lengths=static_cast<int>(buf[2]);
    group_K.leaf=HDF5::getValue(&buf[4],2);
    group_K.internal=HDF5::getValue(&buf[6],2);
    if (show_debug) {
	std::cerr << sizes.offsets << " " << sizes.lengths << " " << group_K.leaf << " " << group_K.internal << std::endl;
    }
    if (sb_version == 1) {
	fs.read(cbuf,4);
	if (fs.gcount() != 4) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="read error for superblock V1 additions";
	  return false;
	}
    }
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error for V0/1 superblock base address";
	return false;
    }
    base_addr=HDF5::getValue(buf,sizes.offsets);
    if (show_debug) {
	std::cerr << "base address: " << base_addr << std::endl;
    }
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error for V0/1 superblock file free-space address";
	return false;
    }
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error for V0/1 superblock end-of-file address";
	return false;
    }
    eof_addr=HDF5::getValue(buf,sizes.offsets);
    if (show_debug) {
	std::cerr << "eof address: " << eof_addr << std::endl;
    }
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error for V0/1 superblock driver information block address";
	return false;
    }
  }
  else if (sb_version == 2) {
    sizes.offsets=static_cast<int>(buf[9]);
    if (show_debug) {
	std::cerr << "size of offsets: " << sizes.offsets << std::endl;
    }
    createMask(undef_addr,sizes.offsets*8);
    sizes.lengths=static_cast<int>(buf[10]);
    if (show_debug) {
	std::cerr << "size of lengths: " << sizes.lengths << std::endl;
    }
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error for V2 superblock base address";
	return false;
    }
    base_addr=HDF5::getValue(buf,sizes.offsets);
    if (show_debug) {
	std::cerr << "base addr: " << base_addr << std::endl;
    }
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error for V2 superblock extension address";
	return false;
    }
    unsigned long long ext_addr=HDF5::getValue(buf,sizes.offsets);
    if (show_debug) {
	std::cerr << "ext addr: " << ext_addr << std::endl;
    }
    if (ext_addr != undef_addr) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to handle superblock extension";
	return false;
    }
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error for V2 superblock end-of-file address";
	return false;
    }
    eof_addr=HDF5::getValue(buf,sizes.offsets);
    if (show_debug) {
	std::cerr << "eof addr: " << eof_addr << std::endl;
    }
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error for V2 superblock root group object header address";
	return false;
    }
    objhdr_addr=HDF5::getValue(buf,sizes.offsets);
    if (show_debug) {
	std::cerr << "root group object header addr=" << objhdr_addr << std::endl;
    }
    fs.read(cbuf,4);
    if (fs.gcount() != 4) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error for V2 superblock checksum";
	return false;
    }
  }
  else {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to decode superblock version "+strutils::itos(sb_version);
    return false;
  }
  if (sizes.lengths > 16 || sizes.offsets > 16) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="the size of lengths and/or offsets is out of range";
    return false;
  }
  return true;
}

bool InputHDF5Stream::decodeSymbolTableEntry(SymbolTableEntry& ste,Group *group)
{
  if (show_debug) {
    std::cerr << "SYMBOL TABLE ENTRY " << fs.tellg() << std::endl;
  }
  unsigned char buf[16];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.offsets)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error for symbol table entry link name offset";
    return false;
  }
  ste.linkname_off=HDF5::getValue(buf,sizes.offsets);
  if (show_debug) {
    std::cerr << "link name offset: " << ste.linkname_off << std::endl;
  }
  if (group != NULL) {
    if (show_debug) {
	std::cerr << "  local heap data start: " << group->local_heap.data_start << std::endl;
    }
    ste.linkname="";
    auto curr_off=fs.tellg();
    fs.seekg(group->local_heap.data_start+ste.linkname_off,std::ios_base::beg);
    while (1) {
	fs.read(cbuf,1);
	if (fs.gcount() != 1) {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="read error while getting link name";
	  return false;
	}
	if (buf[0] == 0) {
	  break;
	}
	else {
	  ste.linkname+=std::string(reinterpret_cast<char *>(buf),1);
	}
    }
    fs.seekg(curr_off,std::ios_base::beg);
    if (show_debug) {
	std::cerr << "  link name is '" << ste.linkname << "'" << std::endl;
    }
  }
  fs.read(cbuf,sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.offsets)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error for symbol table entry object header address";
    return false;
  }
  ste.objhdr_addr=HDF5::getValue(buf,sizes.offsets);
  if (show_debug) {
    std::cerr << "object header address: " << ste.objhdr_addr << std::endl;
  }
  fs.read(cbuf,8);
  if (fs.gcount() != 8) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error for symbol table entry cache_type";
    return false;
  }
  ste.cache_type=HDF5::getValue(buf,4);
  fs.read(reinterpret_cast<char *>(ste.scratchpad),16);
  if (fs.gcount() != 16) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error for symbol table entry scratchpad";
    return false;
  }
  if (show_debug) {
    std::cerr << "-- DONE: SYMBOL TABLE ENTRY" << std::endl;
  }
  return true;
}

bool InputHDF5Stream::decodeSymbolTableNode(unsigned long long address,Group& group)
{
  unsigned long long curr_off=fs.tellg();
  fs.seekg(address,std::ios_base::beg);
  unsigned char buf[64];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,8);
  if (fs.gcount() != 8) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="error reading first 8 bytes of symbol table node";
    return false;
  }
  if (std::string(cbuf,4) != "SNOD") {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="bad signature '"+std::string(cbuf,4)+"' for symbol table node";
    return false;
  }
  size_t num_symbols=HDF5::getValue(&buf[6],2);
  SymbolTableEntry ste;
  for (size_t n=0; n < num_symbols; ++n) {
    if (!decodeSymbolTableEntry(ste,&group)) {
	return false;
    }
    if (!decodeObjectHeader(ste,&group)) {
	return false;
    }
  }
  fs.seekg(curr_off,std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decodeV1BTree(Group& group)
{
  auto curr_off=fs.tellg();
  unsigned char buf[8];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,4);
  if (fs.gcount() != 4) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error bytes 5-8 in V1 tree";
    return false;
  }
  group.tree.node_type=static_cast<int>(buf[0]);
  if (group.tree.node_type != 0) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="can't decode chunk node tree as group node tree";
    return false;
  }
  group.tree.node_level=static_cast<int>(buf[1]);
  size_t num_children=HDF5::getValue(&buf[2],2);
  if (show_debug) {
    std::cerr << "V1 Tree GROUP node_type=" << group.tree.node_type << " node_level=" << group.tree.node_level << " num_children=" << num_children << std::endl;
  }
  fs.read(cbuf,sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.offsets)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error for address of left sibling";
    return false;
  }
  group.tree.left_sibling_addr=HDF5::getValue(buf,sizes.offsets);
  if (show_debug) {
    std::cerr << "address of left sibling: " << group.tree.left_sibling_addr << std::endl;
  }
  fs.read(cbuf,sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.offsets)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error for address of right sibling";
    return false;
  }
  group.tree.right_sibling_addr=HDF5::getValue(buf,sizes.offsets);
  if (show_debug) {
    std::cerr << "address of right sibling: " << group.tree.right_sibling_addr << std::endl;
  }
  fs.read(cbuf,sizes.lengths);
  if (fs.gcount() != static_cast<int>(sizes.lengths)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error for throwaway key";
    return false;
  }
  for (size_t n=0; n < num_children; ++n) {
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error for address of child "+strutils::itos(n);
	return false;
    }
    long long chld_p=HDF5::getValue(buf,sizes.offsets);
    group.tree.child_pointers.emplace_back(chld_p);
    fs.read(cbuf,sizes.lengths);
    if (fs.gcount() != static_cast<int>(sizes.lengths)) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="read error for key "+strutils::itos(n);
	return false;
    }
    group.tree.keys.emplace_back(HDF5::getValue(buf,sizes.lengths));
    if (!decodeSymbolTableNode(chld_p,group)) {
	return false;
    }
  }
  fs.seekg(curr_off,std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decodeV1BTree(unsigned long long address,Dataset& dataset)
{
  unsigned long long ldum;
  if (address == undef_addr) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="tree is not defined";
    return false;
  }
  auto curr_off=fs.tellg();
  fs.seekg(address,std::ios_base::beg);
  std::streamsize num_to_read=8+sizes.offsets*2;
  unsigned char *buf=new unsigned char[num_to_read];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,num_to_read);
  if (fs.gcount() != num_to_read) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="error reading V1 B-tree";
    return false;
  }
  if (std::string(cbuf,4) != "TREE") {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="not a V1 B-tree";
    return false;
  }
  auto node_type=static_cast<int>(buf[4]);
  if (node_type != 1) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="can't decode group node tree as chunk node tree";
    return false;
  }
  auto node_level=static_cast<int>(buf[5]);
  size_t num_children=HDF5::getValue(&buf[6],2);
  if (show_debug) {
    std::cerr << "V1 Tree CHUNK node_type=" << node_type << " node_level=" << node_level << " num_children=" << num_children << std::endl;
  }
  ldum=HDF5::getValue(&buf[8],sizes.offsets);
  if (show_debug) {
    std::cerr << "address of left sibling: " << ldum << std::endl;
  }
  ldum=HDF5::getValue(&buf[8+sizes.offsets],sizes.offsets);
  if (show_debug) {
    std::cerr << "address of right sibling: " << ldum << std::endl;
  }
  delete[] buf;
  num_to_read=num_children*(8+8*dataset.data.sizes.size()+sizes.offsets*2);
  buf=new unsigned char[num_to_read];
  fs.read(reinterpret_cast<char *>(buf),num_to_read);
  if (fs.gcount() != num_to_read) {
    if (!myerror.empty()) {
	myerror+=", ";
    }
    myerror+="unable to decode V1 B-tree";
    return false;
  }
  auto boff=0;
  for (size_t n=0; n < num_children; ++n) {
    size_t chunk_size=HDF5::getValue(&buf[boff],4);
    boff+=8;
    if (show_debug) {
	std::cerr << "chunk size: " << chunk_size;
    }
    size_t chunk_len=1;
    std::vector<size_t> offsets;
    for (size_t m=0; m < dataset.data.sizes.size(); ++m) {
	chunk_len*=dataset.data.sizes[m];
	offsets.emplace_back(HDF5::getValue(&buf[boff],8));
	boff+=8;
	if (show_debug) {
	  std::cerr << " dimension " << m << " offset: " << offsets.back();
	}
    }
    chunk_len*=dataset.data.size_of_element;
    ldum=HDF5::getValue(&buf[boff],sizes.offsets);
    boff+=sizes.offsets;
    if (ldum != 0) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="datatype offset for child "+strutils::itos(n)+" is NOT zero";
	return false;
    }
    ldum=HDF5::getValue(&buf[boff],sizes.offsets);
    boff+=sizes.offsets;
    if (show_debug) {
	std::cerr << " pointer: " << ldum << " chunk length: " << chunk_len << " #filters: " << dataset.filters.size() << " " << time(NULL) << std::endl;
    }
/*
    char cbuf[4];
    fs.seekg(ldum,std::ios_base::beg);
    fs.read(cbuf,4);
    if (fs.gcount() != 4) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="error reading test bytes for child "+strutils::itos(n);
	return false;
    }
    if (std::string(cbuf,4) == "TREE") {
*/
if (node_level == 1) {
	if (show_debug) {
	  std::cerr << "ANOTHER TREE" << std::endl;
	}
	decodeV1BTree(ldum,dataset);
    }
    else {
	dataset.data.chunks.emplace_back(ldum,chunk_size,chunk_len,offsets);
    }
  }
  fs.seekg(curr_off,std::ios_base::beg);
  delete[] buf;
  return true;
}

bool InputHDF5Stream::decodeV2BTree(Group& group,FractalHeapData *frhp_data)
{
  unsigned long long curr_off;

  curr_off=fs.tellg();
  unsigned char buf[64];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,12);
  if (fs.gcount() != 12) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="read error bytes 5-16 in V2 tree";
    return false;
  }
  if (show_debug) {
    std::cerr << "V2 BTree:" << std::endl;
    std::cerr << "version: " << static_cast<int>(buf[0]) << " type: " << static_cast<int>(buf[1]) << " node size: " << HDF5::getValue(&buf[2],4) << " record size: " << HDF5::getValue(&buf[6],2) << " depth: " << HDF5::getValue(&buf[8],2) << " split %: " << static_cast<int>(buf[10]) << " merge %: " << static_cast<int>(buf[11]) << std::endl;
  }
  fs.read(cbuf,sizes.offsets+2+sizes.lengths);
  if (fs.gcount() != static_cast<int>(sizes.offsets+2+sizes.lengths)) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to read V2 tree root node data";
    return false;
  }
  if (show_debug) {
    std::cerr << "root address: " << HDF5::getValue(buf,sizes.offsets) << " # recs in root node: " << HDF5::getValue(&buf[sizes.offsets],2) << " # recs in tree: " << HDF5::getValue(&buf[sizes.offsets+2],sizes.lengths) << std::endl;
  }
  if (!decodeV2BTreeNode(HDF5::getValue(buf,sizes.offsets),HDF5::getValue(&buf[sizes.offsets],2),frhp_data)) {
    return false;
  }
  fs.seekg(curr_off,std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decodeV2BTreeNode(unsigned long long address,int num_records,FractalHeapData *frhp_data)
{
  int n,max_size,len;

  fs.seekg(address,std::ios_base::beg);
  unsigned char buf[64];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,4);
  if (fs.gcount() != 4) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to read v2 btree node signature";
    return false;
  }
  auto signature=std::string(cbuf,4);
  if (signature == "BTLF") {
    fs.read(cbuf,2);
    if (fs.gcount() != 2) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to read v2 btree version and type";
	return false;
    }
    if (buf[0] != 0) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to decode v2 btree node version "+strutils::itos(static_cast<int>(buf[0]));
	return false;
    }
    switch (buf[1]) {
	case 5:
	{
	  for (n=0; n < num_records; ++n) {
	    fs.read(cbuf,11);
	    if (fs.gcount() != 11) {
		if (myerror.length() > 0) {
		  myerror+=", ";
		}
		myerror+="unable to read v2 btree record #"+strutils::itos(n);
		return false;
	    }
	    if ( (buf[4] & 0xc0) != 0) {
		if (myerror.length() > 0) {
		  myerror+=", ";
		}
		myerror+="unable to decode version "+strutils::itos(buf[4] & 0xc0)+" IDs";
		return false;
	    }
	    switch (buf[4] & 0x30) {
		case 0:
		{
		  max_size= (frhp_data->max_dblock_size < frhp_data->max_managed_obj_size) ? frhp_data->max_dblock_size : frhp_data->max_managed_obj_size;
		  len=1;
		  while (max_size/256 > 0) {
		    ++len;
		    max_size/=256;
		  }
		  if (show_debug) {
		    std::cerr << "BTLF rec #" << n << "  offset: " << HDF5::getValue(&buf[5],frhp_data->max_size/8) << "  length: " << HDF5::getValue(&buf[5+frhp_data->max_size/8],len) << std::endl;
		  }
		  break;
		}
		default:
		{
		  if (myerror.length() > 0) {
		    myerror+=", ";
		  }
		  myerror+="unable to decode type "+strutils::itos(buf[4] & 0x30)+" IDs";
		  return false;
		}
	    }
	  }
	  break;
	}
	default:
	{
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="unable to decode v2 btree node type "+strutils::itos(static_cast<int>(buf[1]));
	  return false;
	}
    }
    return true;
  }
  else if (signature == "BTIN") {
if (show_debug) {
std::cerr << "BTIN decode not defined!" << std::endl;
}
    return true;
  }
  else {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unable to decode v2 btree node with signature '"+std::string(cbuf,4)+"'";
    return false;
  }
}

void InputHDF5Stream::printData(Dataset& dataset)
{
  unsigned long long curr_off;
  unsigned char *buf;
  DataValue value;

  if (dataset.data.address == undef_addr) {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="Data not defined";
    return;
  }
  curr_off=fs.tellg();
  fs.seekg(dataset.data.address,std::ios_base::beg);
  char test[4];
  fs.read(test,4);
  if (fs.gcount() != 4) {
    std::cout << "Data read error";
  }
  else {
    if (std::string(test,4) == "TREE") {
// chunked data
//	if (show_debug) {
if (1) {
	  std::cerr << "CHUNKED DATA! " << dataset.data.chunks.size() << std::endl;
	  for (size_t n=0; n < dataset.data.sizes.size(); ++n) {
	    std::cerr << n << " " << dataset.data.sizes[n] << std::endl;
	  }
	  for (size_t n=0; n < dataset.data.chunks.size(); ++n) {
	    std::cerr << n << " " << dataset.datatype.class_ << std::endl;
	    dump(dataset.data.chunks[n].buffer.get(),dataset.data.chunks[n].length);
	    for (size_t m=0; m < dataset.data.chunks[n].length; m+=dataset.data.size_of_element) {
		value.set(fs,&dataset.data.chunks[n].buffer[m],sizes.offsets,sizes.lengths,dataset.datatype,dataset.dataspace,show_debug);
		value.print(std::cerr,ref_table);
		std::cerr << std::endl;
	    }
	  }
	}
    }
    else {
// contiguous data
	if (show_debug) {
	  std::cerr << "CONTIGUOUS DATA!" << std::endl;
	}
	if (dataset.dataspace.dimensionality == 0) {
	  buf=new unsigned char[dataset.data.sizes[0]];
	  std::copy(test,test+4,buf);
	  fs.read(reinterpret_cast<char *>(buf),dataset.data.sizes[0]-4);
	  if (fs.gcount() != static_cast<int>(dataset.data.sizes[0]-4)) {
	    std::cout << "Data read error";
	  }
	  else {
	    HDF5::printDataValue(dataset.datatype,buf);
	  }
	  delete[] buf;
	}
	else {
	  std::cout << "Unable to read dimensional contiguous data";
	}
    }
  }
  fs.seekg(curr_off,std::ios_base::beg);
}

void InputHDF5Stream::printAGroupTree(Group& group)
{
  static Group *root=&group;
  static std::string indent="";
  GroupEntry ge;
  DatasetEntry dse;
  Attribute attr;

  if (indent.length() == 0) {
    std::cout << "HDF5 File Group Tree:" << std::endl;
    std::cout << "/" << std::endl;
  }
  indent+="  ";
  for (auto& key : group.groups.keys()) {
    std::cout << indent << key << "/" << std::endl;
    group.groups.found(key,ge);
    printAGroupTree(*ge.group);
    for (auto& key2 : ge.group->datasets.keys()) {
	ge.group->datasets.found(key2,dse);
	std::cout << indent;
	if (dse.dataset->datatype.class_ < 0) {
	  std::cout << "  Group: " << key2 << std::endl;
	}
	else {
	  std::cout << "  Dataset: " << key2 << std::endl;
	  std::cout << indent << "    Datatype: " << HDF5::datatypeClassToString(dse.dataset->datatype)<< std::endl;
	}
	for (auto& key3 : dse.dataset->attributes.keys()) {
	  dse.dataset->attributes.found(key3,attr);
	  std::cout << indent << "    ";
	  HDF5::printAttribute(attr,ref_table);
	  std::cout << std::endl;
	}
    }
  }
  strutils::chop(indent,2);
  if (&group == root) {
    if (group.datasets.found("",dse)) {
	for (auto& key : dse.dataset->attributes.keys()) {
	  dse.dataset->attributes.found(key,attr);
	  std::cout << indent << "  ";
	  HDF5::printAttribute(attr,ref_table);
	  std::cout << std::endl;
	}
    }
    for (auto& key : group.datasets.keys()) {
	group.datasets.found(key,dse);
	std::cout << indent;
	if (dse.dataset->datatype.class_ < 0) {
	  std::cout << "  Group: " << key << std::endl;
	}
	else {
	  std::cout << "  Dataset: " << key << std::endl;
	  std::cout << indent << "    Datatype: " << HDF5::datatypeClassToString(dse.dataset->datatype) << std::endl;
	}
	for (auto& key2 : dse.dataset->attributes.keys()) {
	  dse.dataset->attributes.found(key2,attr);
	  std::cout << indent << "    ";
	  HDF5::printAttribute(attr,ref_table);
	  std::cout << std::endl;
	}
    }
  }
}

namespace HDF5 {

bool decodeFixedPointNumberArray(const unsigned char *buffer,size_t chunk_number,const InputHDF5Stream::Datatype& datatype,const std::vector<size_t>& dimensions,const InputHDF5Stream::Dataset& dataset,void **values,size_t num_values,size_t& type)
//bool decodeFixedPointNumberArray(const unsigned char *buffer,const InputHDF5Stream::Datatype& datatype,int size_of_element,void **values,size_t num_values,size_t& type,size_t& index,size_t chunk_length)
{
  unsigned char *buf=const_cast<unsigned char *>(buffer);
  short off=HDF5::getValue(&datatype.properties[0],2);
  short bit_length=HDF5::getValue(&datatype.properties[2],2);
  short byte_order;
  getBits(datatype.bit_fields,byte_order,7,1);
  size_t nvals=dataset.data.chunks[chunk_number].length/dataset.data.size_of_element;
  if (off == 0 && (bit_length % 8) == 0) {
    auto idx=0;
    for (size_t n=0; n < dataset.data.chunks[chunk_number].offsets.size(); ++n) {
	size_t mult=1;
	for (size_t m=n+1; m < dimensions.size(); ++m) {
	  mult*=dimensions[m];
	}
	idx+=dataset.data.chunks[chunk_number].offsets[n]*mult;
    }
    auto byte_length=bit_length/8;
    switch (byte_length) {
	case 1:
	{
	  if (*values == nullptr) {
	    *values=new unsigned char[num_values];
	  }
	  if (byte_order == 0) {
	    for (size_t n=0; n < nvals; ++n) {
		if (n > 0 && (n % dataset.data.sizes.back()) == 0) {
		  idx+=dataset.data.sizes.back();
		}
		(reinterpret_cast<unsigned char *>(*values))[idx]=HDF5::getValue(buf,byte_length);
		buf+=dataset.data.size_of_element;
		++idx;
	    }
	  }
	  else {
	    getBits(buf,&(reinterpret_cast<unsigned char *>(*values)[idx]),0,bit_length,0,nvals);
	  }
	  break;
	}
	case 2:
	{
	  if (*values == nullptr) {
	    *values=new short[num_values];
	    type=DataArray::short_;
	  }
	  if (byte_order == 0) {
	    for (size_t n=0; n < nvals; ++n) {
		if (n > 0 && (n % dataset.data.sizes.back()) == 0) {
		  idx+=dataset.data.sizes.back();
		}
		(reinterpret_cast<short *>(*values))[idx]=HDF5::getValue(buf,byte_length);
		buf+=dataset.data.size_of_element;
		++idx;
	    }
	  }
	  else {
	    getBits(buf,&(reinterpret_cast<short *>(*values)[idx]),0,bit_length,0,nvals);
	  }
	  break;
	}
	case 4:
	{
	  if (*values == nullptr) {
	    *values=new int[num_values];
	    type=DataArray::int_;
	  }
	  if (byte_order == 0) {
	    for (size_t n=0; n < nvals; ++n) {
		if (n > 0 && (n % dataset.data.sizes.back()) == 0) {
		  idx+=dataset.data.sizes.back();
		}
		(reinterpret_cast<int *>(*values))[idx]=HDF5::getValue(buf,byte_length);
		buf+=dataset.data.size_of_element;
		++idx;
	    }
	  }
	  else {
	    getBits(buf,&(reinterpret_cast<int *>(*values)[idx]),0,bit_length,0,nvals);
	  }
	  break;
	}
	case 8:
	{
	  if (*values == nullptr) {
	    *values=new long long[num_values];
	    type=DataArray::long_long_;
	  }
	  if (byte_order == 0) {
	    for (size_t n=0; n < nvals; ++n) {
		if (n > 0 && (n % dataset.data.sizes.back()) == 0) {
		  idx+=dataset.data.sizes.back();
		}
		(reinterpret_cast<long long *>(*values))[idx]=HDF5::getValue(buf,byte_length);
		buf+=dataset.data.size_of_element;
		++idx;
	    }
	  }
	  else {
	    getBits(buf,&(reinterpret_cast<long long *>(*values)[idx]),0,bit_length,0,nvals);
	  }
	  break;
	}
    }
    return true;
  }
  else {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="bad offset "+strutils::itos(off)+" or bit length "+strutils::itos(bit_length)+" when trying to fill data array";
    return false;
  }
}

bool decodeFloatingPointNumberArray(const unsigned char *buffer,size_t chunk_number,const InputHDF5Stream::Datatype& datatype,const std::vector<size_t>& dimensions,const InputHDF5Stream::Dataset& dataset,void **values,size_t num_values,size_t& type)
{
  unsigned char *buf=const_cast<unsigned char *>(buffer);
  short bit_length=HDF5::getValue(&datatype.properties[2],2);
  short byte_order[2];
  getBits(datatype.bit_fields,byte_order[0],1,1);
  getBits(datatype.bit_fields,byte_order[1],7,1);
  size_t nvals=dataset.data.chunks[chunk_number].length/dataset.data.size_of_element;
if (nvals > num_values) {
num_values=nvals;
}
  if (byte_order[0] == 0 && byte_order[1] == 0) {
// little-endian
    if (!system_is_big_endian()) {
	auto idx=0;
	for (size_t n=0; n < dataset.data.chunks[chunk_number].offsets.size(); ++n) {
	  size_t mult=1;
	  for (size_t m=n+1; m < dimensions.size(); ++m) {
	    mult*=dimensions[m];
	  }
	  idx+=dataset.data.chunks[chunk_number].offsets[n]*mult;
	}
	if (bit_length == 32 && datatype.properties[5] == 8 && datatype.properties[6] == 0 && datatype.properties[4] == 23) {
// IEEE single precision
	  if (*values == nullptr) {
	    *values=new float[num_values];
	    type=DataArray::float_;
	  }
	  for (size_t n=0; n < nvals; ++n) {
	    if (nvals != num_values && n > 0 && (n % dataset.data.sizes.back()) == 0) {
		idx+=dataset.data.sizes.back();
	    }
	    (reinterpret_cast<float *>(*values))[idx]=(reinterpret_cast<float *>(buf))[0];
	    buf+=dataset.data.size_of_element;
	    ++idx;
	  }
	}
	else if (bit_length == 64 && datatype.properties[5] == 11 && datatype.properties[6] == 0 && datatype.properties[4] == 52) {
// IEEE double-precision
	  if (*values == nullptr) {
	    *values=new double[num_values];
	    type=DataArray::double_;
	  }
	  for (size_t n=0; n < nvals; ++n) {
	    if (n > 0 && (n % dataset.data.sizes.back()) == 0) {
		idx+=dataset.data.sizes.back();
	    }
	    (reinterpret_cast<double *>(*values))[idx]=(reinterpret_cast<double *>(buf))[0];
	    buf+=dataset.data.size_of_element;
	    ++idx;
	  }
	}
	else {
	  if (myerror.length() > 0) {
	    myerror+=", ";
	  }
	  myerror+="can't decode float 0A";
	  return false;
	}
    }
    else {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="can't decode float A";
	return false;
    }
  }
  else if (byte_order[0] == 0 && byte_order[1] == 1) {
// big-endian
    if (system_is_big_endian()) {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="can't decode float B";
	return false;
    }
    else {
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="can't decode float C";
	return false;
    }
  }
  else {
    if (myerror.length() > 0) {
	myerror+=", ";
    }
    myerror+="unknown byte order for data value";
    return false;
  }
  return true;
}

bool decodeStringArray(const unsigned char *buffer,const InputHDF5Stream::Datatype& datatype,int size_of_element,void **values,size_t num_values,size_t& type,size_t& index,size_t chunk_length)
{
  unsigned char *buf=const_cast<unsigned char *>(buffer);
  if (*values == nullptr) {
    *values=new std::string[num_values];
    type=DataArray::string_;
  }
  size_t nvals=chunk_length/size_of_element;
  size_t end=index+nvals;
  if (end > num_values) {
    end=num_values;
  }
  for (; index < end; ++index,buf+=size_of_element) {
    ((reinterpret_cast<std::string *>(*values))[index]).assign(reinterpret_cast<char *>(buf),size_of_element);
  }
  return true;
}

bool castValue(const InputHDF5Stream::DataValue& data_value,int idx,void **array_values,int num_array_values,size_t& type,int size_of_element)
{
  switch (data_value.class_) {
    case 0:
	switch (data_value.precision) {
	  case 16:
	    if (*array_values == nullptr) {
		*array_values=new short[num_array_values];
		type=DataArray::short_;
	    }
	    (reinterpret_cast<short *>(*array_values))[idx]=*(reinterpret_cast<short *>(data_value.value));
	    break;
	  case 32:
	    if (*array_values == nullptr) {
		*array_values=new int[num_array_values];
		type=DataArray::int_;
	    }
	    (reinterpret_cast<int *>(*array_values))[idx]=*(reinterpret_cast<int *>(data_value.value));
	    break;
	  case 64:
	    if (*array_values == nullptr) {
		*array_values=new long long[num_array_values];
		type=DataArray::long_long_;
	    }
	    (reinterpret_cast<long long *>(*array_values))[idx]=*(reinterpret_cast<long long *>(data_value.value));
	    break;
	  default:
	    return false;
	}
	return true;
    case 1:
	if (size_of_element <= 4) {
	  if (*array_values == nullptr) {
	    *array_values=new float[num_array_values];
	    type=DataArray::float_;
	  }
	  (reinterpret_cast<float *>(*array_values))[idx]=*(reinterpret_cast<float *>(data_value.value));
	}
	else {
	  if (*array_values == nullptr) {
	    *array_values=new double[num_array_values];
	    type=DataArray::double_;
	  }
	  (reinterpret_cast<double *>(*array_values))[idx]=*(reinterpret_cast<double *>(data_value.value));
	}
	return true;
    case 3:
	if (*array_values == nullptr) {
	  *array_values=new std::string[num_array_values];
	  type=DataArray::string_;
	}
	(reinterpret_cast<std::string *>(*array_values))[idx]=reinterpret_cast<char *>(data_value.value);
	return true;
  }
  return false;
}

DataArray& DataArray::operator=(const DataArray& source)
{
  if (this == &source) {
    return *this;
  }
  return *this;
}

bool DataArray::fill(InputHDF5Stream& istream,InputHDF5Stream::Dataset& dataset,size_t compound_member_index)
{
  clear();
  if (dataset.data.address == istream.getUndefinedAddress()) {
    return false;
  }
  auto fs=istream.file_stream();
  auto curr_off=fs->tellg();
  fs->seekg(dataset.data.address,std::ios_base::beg);
  char test[4];
  fs->read(test,4);
  if (fs->gcount() != 4) {
    std::cout << "Data read error";
  }
  else {
    InputHDF5Stream::CompoundDatatype cdtype;
    if (dataset.datatype.class_ == 6) {
	decodeCompoundDatatype(dataset.datatype,cdtype,istream.debugIsOn());
    }
    if (std::string(test,4) == "TREE") {
// chunked data
	dimensions.resize(dataset.data.sizes.size(),0);
	for (size_t n=0; n < dataset.data.chunks.size(); ++n) {
	  if (dataset.data.chunks[n].buffer == nullptr) {
	    if (!dataset.data.chunks[n].fill(*fs,dataset,false)) {
		exit(1);
	    }
	  }
	  num_values+=dataset.data.chunks[n].length/dataset.data.size_of_element;
	  for (size_t m=0; m < dataset.data.sizes.size(); ++m) {
	    auto d=dataset.data.chunks[n].offsets[m]+dataset.data.sizes[m];
	    if (d > dimensions[m]) {
		dimensions[m]=d;
	    }
	  }
	}
if (dataset.dataspace.dimensionality == 1 && static_cast<size_t>(num_values) > dataset.dataspace.sizes[0]) {
num_values=dataset.dataspace.sizes[0];
}
	for (size_t n=0,idx=0; n < dataset.data.chunks.size(); ++n) {
	  switch (dataset.datatype.class_) {
	    case 0:
	    {
		decodeFixedPointNumberArray(dataset.data.chunks[n].buffer.get(),n,dataset.datatype,dimensions,dataset,&values,num_values,type);
//		decodeFixedPointNumberArray(dataset.data.chunks[n].buffer.get(),dataset.datatype,dataset.data.size_of_element,&values,num_values,type,idx,dataset.data.chunks[n].length);
		break;
	    }
	    case 1:
	    {
		decodeFloatingPointNumberArray(dataset.data.chunks[n].buffer.get(),n,dataset.datatype,dimensions,dataset,&values,num_values,type);
		break;
	    }
	    case 3:
	    {
		if (dataset.dataspace.dimensionality == 2) {
		  decodeStringArray(dataset.data.chunks[n].buffer.get(),dataset.datatype,dataset.dataspace.sizes.back(),&values,dataset.dataspace.sizes.front(),type,idx,dataset.data.chunks[n].length);
		  num_values=dataset.dataspace.sizes.front();
		}
		else {
		  decodeStringArray(dataset.data.chunks[n].buffer.get(),dataset.datatype,dataset.data.size_of_element,&values,num_values,type,idx,dataset.data.chunks[n].length);
		}
		break;
	    }
	    case 6:
	    {
		int off=0;
		for (size_t m=0; m < compound_member_index; ++m) {
		  off+=cdtype.members[m].datatype.size;
		}
		switch (cdtype.members[compound_member_index].datatype.class_) {
		  case 0:
		  {
		    decodeFixedPointNumberArray(&dataset.data.chunks[n].buffer[off],n,cdtype.members[compound_member_index].datatype,dimensions,dataset,&values,num_values,type);
//		    decodeFixedPointNumberArray(&dataset.data.chunks[n].buffer[off],cdtype.members[compound_member_index].datatype,dataset.data.size_of_element,&values,num_values,type,idx,dataset.data.chunks[n].length);
		    break;
		  }
		  case 1:
		  {
		    decodeFloatingPointNumberArray(&dataset.data.chunks[n].buffer[off],n,cdtype.members[compound_member_index].datatype,dimensions,dataset,&values,num_values,type);
		    break;
		  }
		  case 3:
		  {
		    decodeStringArray(&dataset.data.chunks[n].buffer[off],cdtype.members[compound_member_index].datatype,dataset.data.size_of_element,&values,num_values,type,idx,dataset.data.chunks[n].length);
		    break;
		  }
		  default:
		  {
std::cerr << "DEFAULT " << cdtype.members[compound_member_index].datatype.class_ << std::endl;
		    InputHDF5Stream::DataValue v;
		    auto nval=0;
		    for (size_t m=0; m < dataset.data.chunks[n].length; m+=dataset.data.size_of_element) {
			v.set(*fs,&dataset.data.chunks[n].buffer[m+off],istream.getSizeOfOffsets(),istream.getSizeOfLengths(),cdtype.members[compound_member_index].datatype,dataset.dataspace,istream.debugIsOn());
std::cerr << idx+nval << " ";
v.print(std::cerr,nullptr);
std::cerr << std::endl;
			if (!castValue(v,idx+nval,&values,num_values,type,cdtype.members[compound_member_index].datatype.size)) {
			  if (myerror.length() > 0) {
			    myerror+=", ";
			  }
			  myerror+="unable to get data for class "+strutils::itos(cdtype.members[compound_member_index].datatype.class_)+" and precision "+strutils::itos(v.precision);
			  return false;
			}
			++nval;
		    }
		    idx+=nval;
		  }
		}
		break;
	    }
	    default:
	    {
		if (myerror.length() > 0) {
		  myerror+=", ";
		}
		myerror+="unable to fill data array for class "+strutils::itos(dataset.datatype.class_);
		return false;
	    }
	  }
	}
    }
    else {
// contiguous data
	num_values=dataset.data.sizes[0]/dataset.datatype.size;
	dataset.data.size_of_element=dataset.datatype.size;
	std::unique_ptr<unsigned char[]> buffer;
	buffer.reset(new unsigned char[dataset.data.sizes[0]]);
	fs->seekg(-4,std::ios_base::cur);
	int nbytes=0;
	fs->read(reinterpret_cast<char *>(buffer.get()),dataset.data.sizes[0]);
	if (fs->gcount() != dataset.data.sizes[0]) {
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="contiguous data read error "+strutils::itos(nbytes);
	    return false;
	}
	InputHDF5Stream::DataValue v;
	auto nval=0;
	for (size_t n=0,m=0; n < num_values; ++n,m+=dataset.datatype.size) {
	  v.set(*fs,&buffer[m],istream.getSizeOfOffsets(),istream.getSizeOfLengths(),dataset.datatype,dataset.dataspace,istream.debugIsOn());
	  if (!castValue(v,nval,&values,num_values,type,dataset.data.size_of_element)) {
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to get data for class "+strutils::itos(dataset.datatype.class_);
	    return false;
	  }
	  ++nval;
	}
    }
  }
  fs->seekg(curr_off,std::ios_base::beg);
  return true;
}

short DataArray::short_value(size_t index) const
{
  if (index < num_values && type == short_) {
    return (reinterpret_cast<short *>(values))[index];
  }
  else {
    return 0x7fff;
  }
}

int DataArray::int_value(size_t index) const
{
  if (index < num_values && type == int_) {
    return (reinterpret_cast<int *>(values))[index];
  }
  else {
    return 0x7fffffff;
  }
}

long long DataArray::long_long_value(size_t index) const
{
  if (index < num_values && type == long_long_) {
    return (reinterpret_cast<long long *>(values))[index];
  }
  else {
    return 0x7fffffffffffffff;
  }
}

float DataArray::float_value(size_t index) const
{
  if (index < num_values && type == float_) {
    return (reinterpret_cast<float *>(values))[index];
  }
  else {
    return 1.e48;
  }
}

double DataArray::double_value(size_t index) const
{
  if (index < num_values && type == double_) {
    return (reinterpret_cast<double *>(values))[index];
  }
  else {
    return 1.e48;
  }
}

std::string DataArray::string_value(size_t index) const
{
  if (index < num_values && type == string_) {
    return (reinterpret_cast<std::string *>(values))[index];
  }
  else {
    return "?????";
  }
}

double DataArray::value(size_t index) const
{
  switch (type) {
    case short_:
    {
	return short_value(index);
    }
    case int_:
    {
	return int_value(index);
    }
    case long_long_:
    {
	return long_long_value(index);
    }
    case float_:
    {
	return float_value(index);
    }
    case double_:
    {
	return double_value(index);
    }
  }
  return 0.;
}

void DataArray::clear()
{
  if (values != nullptr) {
    switch (type) {
	case short_:
	{
	  delete[] reinterpret_cast<short *>(values);
	  break;
	}
	case int_:
	{
	  delete[] reinterpret_cast<int *>(values);
	  break;
	}
	case long_long_:
	{
	  delete[] reinterpret_cast<long long *>(values);
	  break;
	}
	case float_:
	{
	  delete[] reinterpret_cast<float *>(values);
	  break;
	}
	case double_:
	{
	  delete[] reinterpret_cast<double *>(values);
	  break;
	}
	case string_:
	{
	  delete[] reinterpret_cast<std::string *>(values);
	  break;
	}
    }
    values=nullptr;
    num_values=0;
    dimensions.resize(0);
  }
}

void printAttribute(InputHDF5Stream::Attribute& attribute,std::shared_ptr<my::map<InputHDF5Stream::ReferenceEntry>> ref_table)
{
  std::cout << "Attribute: " << attribute.key << "  Value: ";
  attribute.value.print(std::cout,ref_table);
}

void printDataValue(InputHDF5Stream::Datatype& datatype,void *value)
{
  int num_members,n,m,off;
  InputHDF5Stream::Datatype *l_datatype;

  if (value == nullptr) {
    std::cout << "?????";
    return;
  }
  switch (datatype.class_) {
    case 0:
    {
// fixed point numbers (integers)
	switch (HDF5::getValue(&datatype.properties[2],2)) {
	  case 8:
	  {
	    std::cout << static_cast<int>(*(reinterpret_cast<unsigned char *>(value)));
	    break;
	  }
	  case 16:
	  {
	    std::cout << *(reinterpret_cast<short *>(value));
	    break;
	  }
	  case 32:
	  {
	    std::cout << *(reinterpret_cast<int *>(value));
	    break;
	  }
	  case 64:
	  {
	    std::cout << *(reinterpret_cast<long long *>(value));
	    break;
	  }
	  default:
	  {
	    std::cout << "?????";
	  }
	}
	break;
    }
    case 1:
    {
// floating point numbers
	switch (HDF5::getValue(&datatype.properties[2],2)) {
	  case 32:
	  {
	    std::cout << *(reinterpret_cast<float *>(value));
	    break;
	  }
	  case 64:
	  {
	    std::cout << *(reinterpret_cast<double *>(value));
	    break;
	  }
	  default:
	  {
	    std::cout << "????.?";
	  }
	}
	break;
    }
    case 3:
    {
// strings
	std::cout << "\"" << reinterpret_cast<char *>(value) << "\"";
	break;
    }
    case 6:
    {
// compound data types
	num_members=HDF5::getValue(datatype.bit_fields,2);
	switch (datatype.version) {
	  case 1:
	  {
	    off=0;
	    std::cout << num_members << " values: ";
	    for (n=0; n < num_members; ++n) {
		if (n > 0) {
		  std::cout << ", ";
		}
		std::cout << "\"" << reinterpret_cast<char *>(&datatype.properties[off]) << "\": ";
		while (static_cast<int>(datatype.properties[off+7]) != 0) {
		  off+=8;
		}
		off+=8;
		off+=4;
		off+=12;
		for (m=0; m < 4; ++m) {
		  off+=4;
		}
		l_datatype=new InputHDF5Stream::Datatype;
		HDF5::decodeDatatype(&datatype.properties[off],*l_datatype,false);
		HDF5::printDataValue(*l_datatype,value);
		off+=8+l_datatype->prop_len;
		delete l_datatype;
	    }
	    break;
	  }
	  default:
	  {
	    std::cout << "?????";
	  }
	}
	break;
    }
    default:
    {
	std::cout << "?????";
    }
  }
}

bool decodeCompoundDatatype(const InputHDF5Stream::Datatype& datatype,InputHDF5Stream::CompoundDatatype& compound_datatype,bool show_debug)
{
  int n,m,off=0,num_members;
  InputHDF5Stream::CompoundDatatype::Member member;

  num_members=HDF5::getValue(const_cast<unsigned char *>(datatype.bit_fields),2);
  compound_datatype.members.clear();
  compound_datatype.members.reserve(num_members);
  for (n=0; n < num_members; ++n) {
    member.name=reinterpret_cast<char *>(&datatype.properties[off]);
    while (static_cast<int>(datatype.properties[off+7]) != 0) {
	off+=8;
    }
    off+=8;
    member.byte_offset=HDF5::getValue(&datatype.properties[off],4);
    off+=4;
    off+=12;
    for (m=0; m < 4; ++m) {
	off+=4;
    }
    decodeDatatype(&datatype.properties[off],member.datatype,show_debug);
    off+=8+member.datatype.prop_len;
    compound_datatype.members.emplace_back(member);
  }
  return true;
}

bool decodeCompoundDataValue(unsigned char *buffer,InputHDF5Stream::Datatype& datatype,void ***values)
{
  *values=NULL;
return true;
}

bool decodeDataspace(unsigned char *buffer,unsigned long long size_of_lengths,InputHDF5Stream::Dataspace& dataspace,bool show_debug)
{
  int off,flag;

  switch (static_cast<int>(buffer[0])) {
    case 1:
	dataspace.dimensionality=static_cast<short>(buffer[1]);
	off=8;
	if (show_debug) {
	  std::cerr << "version: " << static_cast<int>(buffer[0]) << "  dimensionality: " << dataspace.dimensionality << "  flags: " << static_cast<int>(buffer[2]) << std::endl;
	}
dataspace.sizes.clear();
dataspace.sizes.reserve(dataspace.dimensionality);
	for (int n=0; n < dataspace.dimensionality; ++n) {
	  dataspace.sizes.emplace_back(getValue(&buffer[off],size_of_lengths));
	  if (show_debug) {
	    std::cerr << "  dimension " << n << " size: " << dataspace.sizes[n] << std::endl;
	  }
	  off+=size_of_lengths;
	}
	getBits(buffer,flag,23,1);
	if (flag == 1) {
	  if (show_debug) {
	    std::cerr << "max sizes" << std::endl;
	  }
	  dataspace.max_sizes.clear();
	  dataspace.max_sizes.reserve(dataspace.dimensionality);
	  for (int n=0; n < dataspace.dimensionality; ++n) {
	    dataspace.max_sizes.emplace_back(getValue(&buffer[off],size_of_lengths));
	    if (show_debug) {
		std::cerr << "  dimension " << n << " max size: " << dataspace.max_sizes[n] << std::endl;
	    }
	    off+=size_of_lengths;
	  }
	}
	break;
    case 2:
	if (buffer[3] == 2) {
	  dataspace.dimensionality=-1;
	}
	else {
	  dataspace.dimensionality=static_cast<short>(buffer[1]);
	}
	off=4;
	if (show_debug) {
	  std::cerr << "version: " << static_cast<int>(buffer[0]) << "  dimensionality: " << dataspace.dimensionality << "  flags: " << static_cast<int>(buffer[2]) << "  type: " << static_cast<int>(buffer[3]) << std::endl;
	}
	if (dataspace.dimensionality > 0) {
	  dataspace.sizes.clear();
	  dataspace.sizes.reserve(dataspace.dimensionality);
	  for (int n=0; n < dataspace.dimensionality; ++n) {
	    dataspace.sizes.emplace_back(getValue(&buffer[off],size_of_lengths));
	    if (show_debug) {
		std::cerr << "  dimension " << n << " size: " << dataspace.sizes[n] << std::endl;
	    }
	    off+=size_of_lengths;
	  }
	  getBits(buffer,flag,23,1);
	  if (flag == 1) {
	    if (show_debug) {
		std::cerr << "max sizes" << std::endl;
	    }
dataspace.max_sizes.clear();
dataspace.max_sizes.reserve(dataspace.dimensionality);
	    for (int n=0; n < dataspace.dimensionality; ++n) {
		dataspace.max_sizes.emplace_back(getValue(&buffer[off],size_of_lengths));
		if (show_debug) {
		  std::cerr << "  dimension " << n << " max size: " << dataspace.max_sizes[n] << std::endl;
		}
		off+=size_of_lengths;
	    }
	  }
	}
	break;
    default:
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to decode dataspace version "+strutils::itos(static_cast<int>(buffer[0]));
	return false;
  }
  return true;
}

bool decodeDatatype(unsigned char *buffer,InputHDF5Stream::Datatype& datatype,bool show_debug)
{
  int off;

  getBits(buffer,datatype.version,0,4);
  getBits(buffer,datatype.class_,4,4);
  std::copy(&buffer[1],&buffer[3],datatype.bit_fields);
  datatype.size=HDF5::getValue(&buffer[4],4);
  if (show_debug) {
    std::cerr << "Datatype class and version=" << datatype.class_ << "/" << datatype.version << " size: " << datatype.size << std::endl;
    std::cerr << "Size of datatype element is " << datatype.size << std::endl;
  }
  switch (datatype.class_) {
    case 0:
	datatype.prop_len=4;
	datatype.properties.reset(new unsigned char[datatype.prop_len]);
	std::copy(&buffer[8],&buffer[8]+datatype.prop_len,datatype.properties.get());
	break;
    case 1:
	datatype.prop_len=12;
	datatype.properties.reset(new unsigned char[datatype.prop_len]);
	std::copy(&buffer[8],&buffer[8]+datatype.prop_len,datatype.properties.get());
	break;
    case 2:
	datatype.prop_len=2;
	datatype.properties.reset(new unsigned char[datatype.prop_len]);
	std::copy(&buffer[8],&buffer[8]+datatype.prop_len,datatype.properties.get());
	break;
    case 4:
	datatype.prop_len=4;
	datatype.properties.reset(new unsigned char[datatype.prop_len]);
	std::copy(&buffer[8],&buffer[8]+datatype.prop_len,datatype.properties.get());
	break;
    case 5:
	off=8;
	while (static_cast<int>(buffer[off]) != 0) {
	  off+=8;
	}
	datatype.prop_len=off-8;
	datatype.properties.reset(new unsigned char[datatype.prop_len]);
	std::copy(&buffer[8],&buffer[8]+datatype.prop_len,datatype.properties.get());
	break;
    case 6:
    {
	int num_members=HDF5::getValue(datatype.bit_fields,2);
	off=8;
	switch (datatype.version) {
	  case 1:
	  {
	    if (show_debug) {
		std::cerr << "number of members: " << num_members << std::endl;
	    }
	    for (int n=0; n < num_members; ++n) {
		if (show_debug) {
		  std::cerr << "  name of member " << n << ": '" << &buffer[off] << "' at off " << off << std::endl;
		}
		while (static_cast<int>(buffer[off+7]) != 0) {
		  off+=8;
		}
		off+=8;
		if (show_debug) {
		  std::cerr << "  name ends at off " << off << std::endl;
		  std::cerr << "  byte offset of member within data: " << HDF5::getValue(&buffer[off],4) << std::endl;
		}
		off+=4;
		if (show_debug) {
		  std::cerr << "  dimensionality: " << static_cast<int>(buffer[off]) << std::endl;
		}
		off+=12;
		for (int m=0; m < 4; ++m) {
		  if (show_debug) {
		    std::cerr << "  dim " << m << " size: " << HDF5::getValue(&buffer[off],4) << std::endl;
		  }
		  off+=4;
		}
		if (show_debug) {
		  std::cerr << "  decoding datatype at offset " << off << std::endl;
		}
		InputHDF5Stream::Datatype *l_datatype=new InputHDF5Stream::Datatype;
		decodeDatatype(&buffer[off],*l_datatype,show_debug);
		off+=8+l_datatype->prop_len;
		delete l_datatype;
		if (show_debug) {
		  std::cerr << "  off is " << off << std::endl;
		}
	    }
	    datatype.prop_len=off-8;
	    datatype.properties.reset(new unsigned char[datatype.prop_len]);
	    std::copy(&buffer[8],&buffer[8]+datatype.prop_len,datatype.properties.get());
	    break;
	  }
	  case 3:
	  {
	    if (show_debug) {
		std::cerr << "number of members: " << num_members << std::endl;
	    }
	    for (int n=0; n < num_members; ++n) {
		if (show_debug) {
		  std::cerr << "  name of member " << n << ": '" << &buffer[off] << "' at off " << off << std::endl;
		}
		while (buffer[off] != 0) {
		  ++off;
		}
		++off;
		if (show_debug) {
		  std::cerr << "  name ends at off " << off << std::endl;
		}
		int m=datatype.size;
		auto l=1;
		while (m/256 > 0) {
		  ++l;
		  m/=256;
		}
		if (show_debug) {
		  std::cerr << "  datatype size: " << datatype.size << " length of byte offset: " << l << std::endl;
		  std::cerr << "  byte offset of member within data: " << HDF5::getValue(&buffer[off],l) << std::endl;
		}
		off+=l;
		if (show_debug) {
		  std::cerr << "  decoding datatype at offset " << off << " class/version: " << static_cast<int>(buffer[off]) << std::endl;
		}
		InputHDF5Stream::Datatype *l_datatype=new InputHDF5Stream::Datatype;
		decodeDatatype(&buffer[off],*l_datatype,show_debug);
		off+=8+l_datatype->prop_len;
		delete l_datatype;
		if (show_debug) {
		  std::cerr << "  off is " << off << std::endl;
		}
	    }
	    datatype.prop_len=off-8;
	    datatype.properties.reset(new unsigned char[datatype.prop_len]);
	    std::copy(&buffer[8],&buffer[8]+datatype.prop_len,datatype.properties.get());
	    break;
	  }
	  default:
	    if (myerror.length() > 0) {
		myerror+=", ";
	    }
	    myerror+="unable to decode compound data type version "+strutils::itos(datatype.version);
	    return false;
	}
	break;
    }
    case 8:
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to decode datatype class 8 properties";
	return false;
	break;
    case 9:
	datatype.prop_len=datatype.size-8;
	datatype.properties.reset(new unsigned char[datatype.prop_len]);
	std::copy(&buffer[8],&buffer[8]+datatype.prop_len,datatype.properties.get());
	break;
    case 10:
	if (myerror.length() > 0) {
	  myerror+=", ";
	}
	myerror+="unable to decode datatype class 10 properties";
	return false;
	break;
    default:
	datatype.prop_len=0;
  }
  return true;
}

int getGlobalHeapObject(std::fstream& fs,short size_of_lengths,unsigned long long address,int index,unsigned char **buffer)
{
  int buf_len=0;
  auto curr_off=fs.tellg();
  fs.seekg(address,std::ios_base::beg);
  unsigned char buf[24];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,4);
  if (fs.gcount() != 4) {
    std::cerr << "unable to read global heap collection signature" << std::endl;
    exit(1);
  }
  if (std::string(cbuf,4) != "GCOL") {
    std::cerr << "not a global heap collection" << std::endl;
    exit(1);
  }
  fs.read(cbuf,4);
  if (fs.gcount() != 4) {
    std::cerr << "unable to read global heap collection version" << std::endl;
    exit(1);
  }
  if (buf[0] != 1) {
    std::cerr << "unable to decode global heap collection version "+strutils::itos(buf[0]) << std::endl;
    exit(1);
  }
  fs.read(cbuf,size_of_lengths);
  if (fs.gcount() != static_cast<int>(size_of_lengths)) {
    std::cerr << "unable to read size of global heap collection" << std::endl;
    exit(1);
  }
  unsigned long long gsize=HDF5::getValue(buf,size_of_lengths);
  int off=0;
  while (off < static_cast<int>(gsize)) {
    fs.read(cbuf,8+size_of_lengths);
    if (fs.gcount() != static_cast<int>(8+size_of_lengths)) {
	std::cerr << "unable to read global heap object" << std::endl;
	exit(1);
    }
    int idx=HDF5::getValue(buf,2);
    unsigned long long osize=HDF5::getValue(&buf[8],size_of_lengths);
    auto pad=(osize % 8);
    if (pad > 0) {
	osize+=(8-pad);
    }
    if (idx == index) {
	buf_len=osize;
	*buffer=new unsigned char[buf_len];
	fs.read(reinterpret_cast<char *>(*buffer),buf_len);
	if (fs.gcount() != buf_len) {
	  std::cerr << "unable to read global heap object data" << std::endl;
	  exit(1);
	}
	break;
    }
    else {
	fs.seekg(osize,std::ios_base::cur);
	off+=(8+size_of_lengths+osize);
    }
  }
  fs.seekg(curr_off,std::ios_base::beg);
  return buf_len;
}

unsigned long long getValue(const unsigned char *buffer,int num_bytes)
{
  unsigned long long val=0,m=1;
  int n;

  for (n=0; n < num_bytes; ++n) {
    val+=(static_cast<unsigned long long>(buffer[n]))*m;
    m*=256;
  }
  return val;
}

double decode_data_value(InputHDF5Stream::Datatype& datatype,void *value,double missing_indicator)
{
  if (value == nullptr) {
    return missing_indicator;
  }
  switch (datatype.class_) {
    case 0:
    {
// fixed point numbers (integers)
	switch (HDF5::getValue(&datatype.properties[2],2)) {
	  case 8:
	  {
	    return static_cast<int>(*(reinterpret_cast<unsigned char *>(value)));
	  }
	  case 16:
	  {
	    return *(reinterpret_cast<short *>(value));
	  }
	  case 32:
	  {
	    return *(reinterpret_cast<int *>(value));
	  }
	  case 64:
	  {
	    return *(reinterpret_cast<long long *>(value));
	  }
	  default:
	  {
	    return missing_indicator;
	  }
	}
	break;
    }
    case 1:
    {
// floating point numbers
	switch (HDF5::getValue(&datatype.properties[2],2)) {
	  case 32:
	  {
	    return *(reinterpret_cast<float *>(value));
	  }
	  case 64:
	  {
	    return *(reinterpret_cast<double *>(value));
	  }
	  default:
	  {
	    return missing_indicator;
	  }
	}
	break;
    }
  }
  return missing_indicator;
}

std::string decode_data_value(InputHDF5Stream::Datatype& datatype,void *value,std::string missing_indicator)
{
  if (value == nullptr) {
    return missing_indicator;
  }
  switch (datatype.class_) {
    case 3:
    {
// strings
	missing_indicator.assign(reinterpret_cast<char *>(value),datatype.size);
	break;
    }
  }
  return missing_indicator;
}

std::string datatypeClassToString(const InputHDF5Stream::Datatype& datatype)
{
  switch (datatype.class_) {
    case 0:
	return "Fixed-Point ("+strutils::itos(HDF5::getValue(&datatype.properties[2],2))+")";
    case 1:
	return "Floating-Point ("+strutils::itos(HDF5::getValue(&datatype.properties[2],2))+")";
    case 3:
	return "String";
    case 4:
	return "Bit field";
    case 6:
    {
	std::string s="Compound [";
	InputHDF5Stream::CompoundDatatype cdatatype;
	if (HDF5::decodeCompoundDatatype(datatype,cdatatype,false)) {
	  for (auto& member : cdatatype.members) {
	    if (&member != &cdatatype.members.front()) {
		s+=", ";
	    }
	    s+=datatypeClassToString(member.datatype);
	  }
	}
	s+="]";
	return s;
    }
    default:
	return "Class: "+strutils::itos(datatype.class_);
  }
}

}; // end namespace HDF5
