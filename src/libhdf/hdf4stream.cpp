#include <hdf.hpp>
#include <bits.hpp>
#include <myerror.hpp>

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
  te.id="DFTAG_NULL";
  te.description="No data";
  tag_table.insert(te);
  te.key=30;
  te.id="DFTAG_VERSION";
  te.description="Library version number";
  tag_table.insert(te);
  te.key=106;
  te.id="DFTAG_NT";
  te.description="Number type";
  tag_table.insert(te);
  te.key=701;
  te.id="DFTAG_SDD";
  te.description="Scientific data dimension";
  tag_table.insert(te);
  te.key=702;
  te.id="DFTAG_SD";
  te.description="Scientific data";
  tag_table.insert(te);
  te.key=720;
  te.id="DFTAG_NDG";
  te.description="Numeric data group";
  tag_table.insert(te);
  te.key=721;
  te.id="**DUMMY**";
  te.description="Dummy for scientific data - nothing actually written to file";
  tag_table.insert(te);
  te.key=1962;
  te.id="DFTAG_VH";
  te.description="Vdata description";
  tag_table.insert(te);
  te.key=1963;
  te.id="DFTAG_VS";
  te.description="Vdata";
  tag_table.insert(te);
  te.key=1965;
  te.id="DFTAG_VG";
  te.description="Vgroup";
  tag_table.insert(te);
}

bool InputHDF4Stream::open(const char *filename)
{
  if (is_open()) {
    if (!myerror.empty()) {
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
    if (!myerror.empty()) {
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
	return false;
    }
    size_t num_dds;
    bits::get(buf,num_dds,0,16);
    bits::get(buf,next,16,32);
    for (size_t n=0; n < num_dds; ++n) {
	fs.read(cbuf,12);
	if (fs.gcount() != 12) {
	  return false;
	}
	DataDescriptor dd;
	bits::get(buf,dd.key,0,16);
	if (dd.key != 1) {
	  bits::get(buf,dd.reference_number,16,16);
	  bits::get(buf,dd.offset,32,32);
	  bits::get(buf,dd.length,64,32);
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

void InputHDF4Stream::print_data_descriptor(const DataDescriptor& data_descriptor,std::string indent)
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
    std::cout << te.id << "(" << data_descriptor.key << ")/" << data_descriptor.reference_number << " - " << te.description << "  Offset: " << data_descriptor.offset << "  Length: " << data_descriptor.length << std::endl;
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
	  if (!myerror.empty()) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	std::cout << indent;
	bits::get(buffer,idum,0,32);
	std::cout << "  Version: " << idum;
	bits::get(buffer,idum,32,32);
	std::cout << "." << idum;
	bits::get(buffer,idum,64,32);
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
	  if (!myerror.empty()) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	std::cout << indent;
	bits::get(buffer,idum,0,8);
	std::cout << "  version: " << idum;
	bits::get(buffer,idum,8,8);
	std::cout << "  type: " << data_types[idum] << "(" << idum << ")";
	bits::get(buffer,idum,16,8);
	std::cout << "  width: " << idum;
	bits::get(buffer,idum,24,8);
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
	  if (!myerror.empty()) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	std::cout << indent;
	bits::get(buffer,ndims,0,16);
	std::cout << "  #dims: " << ndims;
	nvals=new int[ndims];
	off=16;
	bits::get(buffer,nvals,off,32,0,ndims);
	off+=32*ndims;
	bits::get(buffer,idum,off,16);
	std::cout << "  data_nt_ref: " << idum << std::endl;
	off+=16;
	scale_nt_refs=new short[ndims];
	bits::get(buffer,scale_nt_refs,off,16,0,ndims);
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
	  if (!myerror.empty()) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	vals=new short[data_descriptor.length/2];
	bits::get(buffer,vals,0,16,0,data_descriptor.length/2);
	for (n=0; n < data_descriptor.length/2; n+=2) {
	  std::cout << indent << "  Member: " << n/2 << "  tag/ref#: ";
	  if (tag_table.found(vals[n],te)) {
	    std::cout << te.id << "(" << vals[n] << ")";
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
	  if (!myerror.empty()) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	std::cout << indent;
	bits::get(buffer,idum,0,16);
	std::cout << "  Interlace: " << idum;
	bits::get(buffer,nentries,16,32);
	std::cout << "  #entries: " << nentries;
	bits::get(buffer,idum,48,16);
	std::cout << "  entry len: " << idum;
	bits::get(buffer,nfields,64,16);
	std::cout << "  #fields/entry: " << nfields << std::endl;
	off=80;
	types=new short[nfields];
	sizes=new short[nfields];
	offsets=new short[nfields];
	orders=new short[nfields];
	fldnmlens=new short[nfields];
	fldnms=new char *[nfields];
	bits::get(buffer,types,off,16,0,nfields);
	off+=16*nfields;
	bits::get(buffer,sizes,off,16,0,nfields);
	off+=16*nfields;
	bits::get(buffer,offsets,off,16,0,nfields);
	off+=16*nfields;
	bits::get(buffer,orders,off,16,0,nfields);
	off+=16*nfields;
	for (n=0; n < nfields; ++n) {
	  bits::get(buffer,fldnmlens[n],off,16);
	  off+=16;
	  fldnms[n]=new char[fldnmlens[n]];
	  bits::get(buffer,fldnms[n],off,8,0,fldnmlens[n]);
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
	bits::get(buffer,idum,off,16);
	off+=16;
	std::cout << indent;
	cdum=new char[idum];
	bits::get(buffer,cdum,off,8,0,idum);
	off+=8*idum;
	std::cout << "  Name: '";
	std::cout.write(cdum,idum);
	delete[] cdum;
	bits::get(buffer,idum,off,16);
	off+=16;
	cdum=new char[idum];
	bits::get(buffer,cdum,off,8,0,idum);
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
		if (!myerror.empty()) {
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
		  bits::get(buffer,shdum,off,16);
		  std::cout << shdum;
		  off+=16;
		}
		else if (data_types[types[0]] == "DFNT_UINT16") {
		  bits::get(buffer,idum,off,16);
		  std::cout << idum;
		  off+=16;
		}
		else if (data_types[types[0]] == "DFNT_INT32") {
		  bits::get(buffer,idum,off,32);
		  std::cout << idum;
		  off+=32;
		}
		else if (data_types[types[0]] == "DFNT_FLOAT64") {
		  bits::get(buffer,ldum,off,64);
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
	  if (!myerror.empty()) {
	    myerror+=", ";
	  }
	  myerror+="Bad data descriptor length";
	  exit(1);
	}
	bits::get(buffer,nelements,0,16);
	std::cout << indent << "  #elements: " << nelements << std::endl;
	off=16;
	tags=new short[nelements];
	bits::get(buffer,tags,off,16,0,nelements);
	off+=16*nelements;
	ref_nos=new short[nelements];
	bits::get(buffer,ref_nos,off,16,0,nelements);
	off+=16*nelements;
	for (n=0; n < nelements; ++n) {
	  std::cout << indent << "  Element: " << n << "  ";
	  reference_table.found(ref_nos[n],re);
	  re.data_descriptor_table->found(tags[n],dd);
	  print_data_descriptor(dd,indent+"  ");
	}
	delete[] tags;
	delete[] ref_nos;
	delete[] buffer;
	break;
    }
  }
}

void InputHDF4Stream::print_data_descriptors(size_t tag_number)
{
  for (size_t n=0; n < data_descriptors.size(); ++n) {
    if (data_descriptors[n].key == tag_number) {
	print_data_descriptor(data_descriptors[n]);
    }
  }
}

int InputHDF4Stream::read(unsigned char *buffer,size_t buffer_length)
{
return bfstream::error;
}
