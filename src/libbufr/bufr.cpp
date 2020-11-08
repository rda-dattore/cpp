#include <iomanip>
#include <fstream>
#include <complex>
#include <regex>
#include <bufr.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>
#include <myerror.hpp>

const bool BUFRReport::header_only=true,
           BUFRReport::full_report=false;

int InputBUFRStream::peek()
{
return bfstream::error;
}

int InputBUFRStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;
// read a BUFR report from the stream
  if (icosstream != NULL) {
    if ( (bytes_read=icosstream->read(buffer,buffer_length)) <= 0) {
	return bytes_read;
    }
    if (bytes_read == static_cast<int>(buffer_length)) {
	myerror="buffer overflow";
	exit(1);
    }
// check for the presence of 'BUFR' in the first four bytes
    if (buffer[0] != 0x42 || buffer[1] != 0x55 || buffer[2] != 0x46 || buffer[3] != 0x52) {
	return bfstream::error;
    }
  }
  else if (if77_stream != NULL) {
    if ( (bytes_read=if77_stream->read(buffer,buffer_length)) <= 0) {
	return bytes_read;
    }
    if (bytes_read == static_cast<int>(buffer_length)) {
	myerror="buffer overflow";
	exit(1);
    }
// check for the presence of 'BUFR' in the first four bytes
    if (buffer[0] != 0x42 || buffer[1] != 0x55 || buffer[2] != 0x46 || buffer[3] != 0x52) {
	return bfstream::error;
    }
  }
  else {
    fs.read(reinterpret_cast<char *>(buffer),4);
    bytes_read=fs.gcount();
    if (bytes_read < 4) {
	if (bytes_read == 0) {
	  return bfstream::eof;
	}
	else {
	  return bfstream::error;
	}
    }
// check for the presence of 'BUFR' in the first four bytes
/*
    if (num_read == 0 && (buffer[0] != 0x42 || buffer[1] != 0x55 || buffer[2] != 0x46 || buffer[3] != 0x52))
	return bfstream::error;
// otherwise, search the stream for the beginning of the next BUFR report
    else {
*/
	while (buffer[0] != 0x42 || buffer[1] != 0x55 || buffer[2] != 0x46 || buffer[3] != 0x52) {
	  switch (buffer[1]) {
	    case 0x42:
		for (size_t n=0; n < 3; ++n) {
		  buffer[n]=buffer[n+1];
		}
		fs.read(reinterpret_cast<char *>(&buffer[3]),1);
		if (fs.gcount() < 1) {
		  return bfstream::eof;
		}
		bytes_read=4;
		break;
	    default:
		switch (buffer[2]) {
		  case 0x42:
		    for (size_t n=0; n < 2; ++n) {
			buffer[n]=buffer[n+2];
		    }
		    fs.read(reinterpret_cast<char *>(&buffer[2]),2);
		    if (fs.gcount() < 2) {
			return bfstream::eof;
		    }
		    bytes_read=4;
		    break;
		  default:
		    switch (buffer[3]) {
			case 0x42: {
			  buffer[0]=buffer[3];
			  fs.read(reinterpret_cast<char *>(&buffer[1]),3);
			  if (fs.gcount() < 3) {
			    return bfstream::eof;
			  }
			  bytes_read=4;
			  break;
			}
			default: {
			  fs.read(reinterpret_cast<char *>(&buffer[0]),4);
			  if (fs.gcount() < 4) {
			    return bfstream::eof;
			  }
			}
		    }
		}
	  }
	}
/*
    }
*/
    fs.read(reinterpret_cast<char *>(&buffer[4]),4);
    bytes_read+=fs.gcount();
    if (bytes_read != 8) {
	++num_read;
	return bfstream::error;
    }
    size_t msg_len;
    bits::get(buffer,msg_len,32,24);
    if (msg_len > buffer_length) {
	myerror="buffer overflow - message length is "+strutils::itos(msg_len);
	exit(1);
    }
    fs.read(reinterpret_cast<char *>(&buffer[8]),msg_len-8);
    bytes_read+=fs.gcount();
    if (bytes_read != static_cast<int>(msg_len)) {
	return bfstream::error;
    }
  }
  ++num_read;
  return bytes_read;
}

void InputBUFRStream::add_entry_to_table_a(TableAEntry aentry)
{
  if (table_a.found(aentry.key,aentry)) {
    clear_table_a();
    clear_table_d();
  }
  table_a.insert(aentry);
}

void InputBUFRStream::clear_table_d()
{ 
  if (table_d != nullptr) {
    table_d->clear();
  }
}

std::string InputBUFRStream::data_category_description(short data_type)
{
  TableAEntry table_a_entry;
  if (table_a.found(data_type,table_a_entry)) {
    return table_a_entry.description;
  }
  else {
    return "";
  }
}

InputBUFRStream::TableBEntry InputBUFRStream::table_b_entry(short f,short x,short y)
{
  TableBEntry table_b_entry;
  if (table_b != nullptr) {
    table_b_entry.key=make_key(f,x,y);
    return table_b[table_b_entry.key];
  }
  else {
    return table_b_entry;
  }
}

InputBUFRStream::TableDEntry InputBUFRStream::table_d_entry(short f,short x,short y)
{
  TableDEntry table_d_entry;
  if (table_d != nullptr) {
    table_d_entry.key=make_key(f,x,y);
    if (!table_d->found(table_d_entry.key,table_d_entry)) {
	table_d_entry.sequence.clear();
    }
  }
  return table_d_entry;
}

void InputBUFRStream::add_table_d_sequence_descriptor(Descriptor descriptor,Descriptor descriptor_to_add)
{
  TableDEntry table_d_entry;
  if (table_d == nullptr) {
    table_d.reset(new my::map<TableDEntry>(400000));
  }
  table_d_entry.key=make_key(descriptor.f,descriptor.x,descriptor.y);
  if (!table_d->found(table_d_entry.key,table_d_entry)) {
    table_d_entry.sequence.clear();
    table_d_entry.sequence.push_back(descriptor_to_add);
    table_d->insert(table_d_entry);
  }
  else {
    table_d_entry.sequence.push_back(descriptor_to_add);
    table_d->replace(table_d_entry);
  }
}

void InputBUFRStream::define_table_b_element_name(Descriptor descriptor,const char *element_name_string)
{
  TableBEntry table_b_entry;
  if (table_b == nullptr) {
    return;
  }
  table_b_entry.key=make_key(descriptor.f,descriptor.x,descriptor.y);
  table_b[table_b_entry.key].element_name=element_name_string;
  table_b[table_b_entry.key].is_local=true;
  ++table_b_length_;
}

void InputBUFRStream::define_table_b_scale_sign(Descriptor descriptor,char sign)
{
  TableBEntry table_b_entry;
  if (table_b == nullptr) {
    return;
  }
  if (sign != '+' && sign != '-') {
    myerror="attempt to define a bad scale sign '"+std::string(&sign,1)+"' for descriptor "+strutils::itos(descriptor.f)+" "+strutils::itos(descriptor.x)+" "+strutils::itos(descriptor.y);
    exit(1);
  }
  table_b_entry.key=make_key(descriptor.f,descriptor.x,descriptor.y);
  table_b[table_b_entry.key].sign.scale=sign;
  table_b[table_b_entry.key].is_local=true;
}

void InputBUFRStream::define_table_b_scale(Descriptor descriptor,short scale)
{
  TableBEntry table_b_entry;
  if (table_b == nullptr) {
    return;
  }
  table_b_entry.key=make_key(descriptor.f,descriptor.x,descriptor.y);
  table_b[table_b_entry.key].scale=scale;
  if (table_b[table_b_entry.key].sign.scale == '-') {
    table_b[table_b_entry.key].scale*=-1;
  }
  table_b[table_b_entry.key].is_local=true;
}

void InputBUFRStream::define_table_b_reference_value_sign(Descriptor descriptor,char sign)
{
  TableBEntry table_b_entry;
  if (table_b == nullptr) {
    return;
  }
  if (sign != '+' && sign != '-') {
    myerror="attempt to define a bad reference value sign '"+std::string(&sign,1)+"'";
    exit(1);
  }
  table_b_entry.key=make_key(descriptor.f,descriptor.x,descriptor.y);
  table_b[table_b_entry.key].sign.ref_val=sign;
  table_b[table_b_entry.key].is_local=true;
}

void InputBUFRStream::define_table_b_reference_value(Descriptor descriptor,int ref_val)
{
  TableBEntry table_b_entry;
  if (table_b == nullptr) {
    return;
  }
  table_b_entry.key=make_key(descriptor.f,descriptor.x,descriptor.y);
  table_b[table_b_entry.key].ref_val=ref_val;
  if (table_b[table_b_entry.key].sign.ref_val == '-') {
    table_b[table_b_entry.key].ref_val=-table_b[table_b_entry.key].ref_val;
  }
  table_b[table_b_entry.key].is_local=true;
}

void InputBUFRStream::define_table_b_data_width(Descriptor descriptor,short width)
{
  TableBEntry table_b_entry;
  if (table_b == nullptr) {
    return;
  }
  table_b_entry.key=make_key(descriptor.f,descriptor.x,descriptor.y);
  table_b[table_b_entry.key].width=width;
  table_b[table_b_entry.key].is_local=true;
}

void InputBUFRStream::define_table_b_units_name(Descriptor descriptor,const char *units_string)
{
  TableBEntry table_b_entry;
  if (table_b == nullptr) {
    return;
  }
  table_b_entry.key=make_key(descriptor.f,descriptor.x,descriptor.y);
  table_b[table_b_entry.key].units=units_string;
  table_b[table_b_entry.key].is_local=true;
}

void InputBUFRStream::print_table_a()
{
  std::cout << "Table A contains " << table_a.size() << " entries" << std::endl;
  if (table_a.size() > 0) {
    for (short n=0; n < 255; ++n) {
	TableAEntry a_entry;
	if (table_a.found(n,a_entry)) {
	  std::cout << std::setw(5) << n << " :  " << a_entry.description << std::endl;
	}
    }
  }
}

void InputBUFRStream::print_table_b(const char *type_string)
{

  if (table_b == nullptr) {
    return;
  }
  auto print_local=false,print_BUFR=false;
  if (strcmp(type_string,"local") == 0) {
    print_local=true;
  }
  else if (strcmp(type_string,"BUFR") == 0) {
    print_BUFR=true;
  }
  else if (strcmp(type_string,"all") == 0) {
    print_local=print_BUFR=true;
  }
  else {
    myerror="Error in printTableB: bad type_string";
    exit(1);
  }
  std::cout << "Table B contains " << table_b_length_ << " entries";
  if (strcmp(type_string,"all") != 0) {
    std::cout << " (showing only " << type_string << " entries)";
  }
  std::cout << std::endl;
  for (size_t n=0; n < 64; ++n) {
    for (size_t m=0; m < 256; ++m) {
	auto key=make_key(0,n,m);
	if (table_b[key].width > 0) {
	  if ((print_local && print_BUFR) || (print_local && table_b[key].is_local) || (print_BUFR && !table_b[key].is_local)) {
	    std::cout << "f:0 x:" << std::setfill('0') << std::setw(2) << n << " y:" << std::setw(3) << m << std::setfill(' ') << " type:";
	    if (table_b[key].is_local) {
		std::cout << "local";
	    }
	    else {
		std::cout << "BUFR";
	    }
	    std::cout << " width:" << table_b[key].width << " scale:" << table_b[key].scale << " ref_val:" << table_b[key].ref_val;
	    if (!table_b[key].element_name.empty()) {
		std::cout << " element_name:" << table_b[key].element_name;
	    }
	    if (!table_b[key].units.empty()) {
		std::cout << " units:" << table_b[key].units;
	    }
	    std::cout << std::endl;
	  }
	}
    }
  }
}

void InputBUFRStream::print_table_d()
{
  if (table_d == nullptr) {
    return;
  }
  std::cout << "Table D contains " << table_d->size() << " entries" << std::endl;
  for (size_t n=0; n < 64; ++n) {
    for (size_t m=0; m < 256; ++m) {
	TableDEntry table_d_entry;
	table_d_entry.key=make_key(3,n,m);
	if (table_d->found(table_d_entry.key,table_d_entry)) {
	  std::cout << "sequence - f:3 x:" << std::setfill('0') << std::setw(2) << n << " y:" << std::setw(3) << m << std::setfill(' ') << std::endl;
	  for (const auto& s : table_d_entry.sequence) {
	    std::cout << "  f:" << std::setfill('0') << std::setw(1) << static_cast<int>(s.f) << " x:" << std::setw(2) << static_cast<int>(s.x) << " y:" << std::setw(3) << static_cast<int>(s.y) << std::setfill(' ') << std::endl;
	  }
	}
    }
  }
}

void InputBUFRStream::add_table_b_entries(std::ifstream& ifs)
{
  if (table_b == nullptr) {
    return;
  }
  const size_t BUF_LEN=32768;
  std::unique_ptr<char[]> line(new char[BUF_LEN]);
  ifs.getline(&line[0],BUF_LEN);
  while (!ifs.eof()) {
    short f,x,y;
    strutils::strget(&line[0],f,1);
    strutils::strget(&line[2],x,2);
    strutils::strget(&line[5],y,3);
    TableBEntry table_b_entry;
    table_b_entry.key=make_key(f,x,y);
    strutils::strget(&line[9],table_b_entry.width,3);
    strutils::strget(&line[13],table_b_entry.scale,3);
    strutils::strget(&line[17],table_b_entry.ref_val,11);
    table_b_entry.units.assign(&line[29],18);
    strutils::trim(table_b_entry.units);
    table_b_entry.is_local=false;
    if (std::string(line.get()).length() > 48) {
	table_b_entry.element_name.assign(&line[48]);
    }
    else {
	table_b_entry.element_name="";
    }
    table_b[table_b_entry.key]=table_b_entry;
    ++table_b_length_;
    ifs.getline(&line[0],BUF_LEN);
  }
}

void InputBUFRStream::fill_table_b(std::string path_to_bufr_tables,short src,short sub_center)
{
  table_b.reset(new TableBEntry[400000]);
  table_b_length_=0;
  std::ifstream ifs((path_to_bufr_tables+"/TableB").c_str());
  if (!ifs.is_open()) {
    myerror="unable to open Table B file";
    exit(1);
  }
  add_table_b_entries(ifs);
  ifs.close();
  ifs.open((path_to_bufr_tables+"/TableB."+strutils::itos(src)+"-"+strutils::itos(sub_center)).c_str());
  if (ifs.is_open()) {
    add_table_b_entries(ifs);
    ifs.close();
  }
}

void InputBUFRStream::extract_sequences(std::ifstream& ifs)
{
  if (table_d == nullptr) {
    return;
  }
  const size_t BUF_LEN=32768;
  std::unique_ptr<char[]> line(new char[BUF_LEN]);
  ifs.getline(&line[0],BUF_LEN);
  while (!ifs.eof()) {
    Descriptor d;
    strutils::strget(&line[0],d.f,1);
    strutils::strget(&line[1],d.x,2);
    strutils::strget(&line[3],d.y,3);
    TableDEntry table_d_entry;
    table_d_entry.key=make_key(d.f,d.x,d.y);
    size_t offset=7;
    table_d_entry.sequence.clear();
    while (offset < std::string(line.get()).length()) {
	strutils::strget(&line[offset],d.f,1);
	strutils::strget(&line[offset+1],d.x,2);
	strutils::strget(&line[offset+3],d.y,3);
	table_d_entry.sequence.push_back(d);
	offset+=7;
    }
    table_d->insert(table_d_entry);
    ifs.getline(&line[0],BUF_LEN);
  }
}

void InputBUFRStream::fill_table_d(std::string path_to_bufr_tables,short src,short sub_center)
{
  if (table_d == nullptr) {
    table_d.reset(new my::map<TableDEntry>);
  }
  else {
    table_d->clear();
  }
  std::ifstream ifs((path_to_bufr_tables+"/TableD").c_str());
  if (!ifs.is_open()) {
    myerror="unable to open standard Table D file";
    exit(1);
  }
  extract_sequences(ifs);
  ifs.close();
  ifs.open((path_to_bufr_tables+"/TableD."+strutils::itos(src)+"-"+strutils::itos(sub_center)).c_str());
  if (ifs.is_open()) {
    extract_sequences(ifs);
    ifs.close();
  }
}

size_t InputBUFRStream::make_key(short f,short x,short y) const
{
  size_t key=f*100000+x*1000+y;
  if (key > 400000) {
    myerror="bad key "+strutils::itos(key);
    exit(1);
  }
  else {
    return key;
  }
}

BUFRReport::~BUFRReport()
{
  if (os != nullptr) {
    delete[] os;
  }
  if (descriptors.size() > 0) {
    descriptors.resize(0);
  }
  num_descriptors=0;
  if (data_ != nullptr) {
    for (short n=0; n < report.nsubs; ++n) {
	if (data_[n] != nullptr) {
	  void (**f)(Descriptor&,const unsigned char*,const InputBUFRStream::TableBEntry&,size_t,BUFRData**,int,short,bool)=data_handler_.target<void(*)(Descriptor&,const unsigned char*,const InputBUFRStream::TableBEntry&,size_t,BUFRData**,int,short,bool)>();
	  if (f != nullptr && *f == handle_ncep_prepbufr) {
	    delete (reinterpret_cast<NCEPPREPBUFRData **>(data_))[n];
	  }
	  else {
	    delete data_[n];
	  }
	}
    }
    delete[] data_;
  }
}

BUFRReport& BUFRReport::operator=(const BUFRReport& source)
{
  if (this == &source) {
    return *this;
  }
  report=source.report;
  if (os != nullptr) {
    delete[] os;
    os=nullptr;
  }
  if (source.os != nullptr) {
    os=new unsigned char[report.os_length-4];
    memcpy(os,source.os,report.os_length-4);
  }
  num_descriptors=source.num_descriptors;
  if (num_descriptors > descriptors.size()) {
    descriptors.resize(num_descriptors);
  }
  for (size_t n=0; n < num_descriptors; ++n) {
    descriptors[n]=source.descriptors[n];
  }
  return *this;
}

void BUFRReport::fill(InputBUFRStream& istream,const unsigned char *stream_buffer,bool fill_header_only,std::string path_to_bufr_tables,std::string file_of_descriptors)
{
  if (stream_buffer == NULL) {
    myerror="empty file stream";
    exit(1);
  }
  data_present.descriptors.clear();
  data_present.num_operators=0;
  data_present.add_to_descriptors=true;
  data_present.bitmap="";
  data_present.bitmap_index=0;
  unpack_is(stream_buffer);
  unpack_ids(stream_buffer);
  if (report.data_type == 11) {
    if (istream.clear_tables_on_type_11) {
	istream.clear_table_a();
	istream.clear_table_d();
	istream.fill_table_b(path_to_bufr_tables,report.src,report.sub_center);
	istream.clear_tables_on_type_11=false;
    }
  }
  else {
    if (istream.table_b_length() == 0) {
	istream.fill_table_b(path_to_bufr_tables,report.src,report.sub_center);
	istream.fill_table_d(path_to_bufr_tables,report.src,report.sub_center);
    }
    istream.clear_tables_on_type_11=true;
  }
  if (report.os_included) {
    unpack_os_length(stream_buffer);
  }
  unpack_dds(istream,stream_buffer,file_of_descriptors,fill_header_only);
  if (report.os_included) {
    decode_os(stream_buffer);
  }
  unpack_ds(istream,stream_buffer,fill_header_only);
  unpack_end(stream_buffer);
}

void BUFRReport::print() const
{
  print_header(true);
  for (short n=0; n < report.nsubs; ++n) {
    data_[n]->print();
  }
}

void BUFRReport::print_data(bool printed_headers) const
{
  for (short n=0; n < report.nsubs; ++n) {
    if (printed_headers && data_[n]->filled) {
	std::cout << "  Subset: " << n+1 << "  ";
    }
    data_[n]->print();
  }
}

void BUFRReport::print_descriptor_group(std::string indent,size_t index,int num_to_print,InputBUFRStream& istream,bool verbose)
{
  static int width=0;
  if (width == 0) {
    while (num_descriptors > static_cast<size_t>(pow(10.,width))) {
	++width;
    }
  }
  for (int n=0; n < num_to_print; ++n) {
    if (descriptors[index].f == 1) {
	auto np=descriptors[index].x;
	std::cout << strutils::ftos(index+1,width,0,' ') << "  " << indent << std::setfill('0') << std::setw(1) << static_cast<int>(descriptors[index].f) << " " << std::setw(2) << static_cast<int>(descriptors[index].x) << " " << std::setw(3) << static_cast<int>(descriptors[index].y) << std::setfill(' ') << std::endl;
	if (descriptors[index].y == 0) {
	  ++index;
	  std::cout << strutils::ftos(index+1,width,0,' ') << "  " << indent << std::setfill('0') << std::setw(1) << static_cast<int>(descriptors[index].f) << " " << std::setw(2) << static_cast<int>(descriptors[index].x) << " " << std::setw(3) << static_cast<int>(descriptors[index].y) << std::setfill(' ');
	  if (verbose) {
	    auto entry=istream.table_b_entry(descriptors[index]);
	    if (entry.element_name.length() > 0)
		std::cout << " " << entry.element_name;
	  }
	  std::cout << std::endl;
	  --num_to_print;
	}
	++index;
	print_descriptor_group(indent+"  ",index,np,istream,verbose);
	num_to_print-=np;
	index+=np;
    }
    else {
	std::cout << strutils::ftos(index+1,width,0,' ') << "  " << indent << std::setfill('0') << std::setw(1) << static_cast<int>(descriptors[index].f) << " " << std::setw(2) << static_cast<int>(descriptors[index].x) << " " << std::setw(3) << static_cast<int>(descriptors[index].y) << std::setfill(' ');
	if (verbose) {
	  auto entry=istream.table_b_entry(descriptors[index]);
	  if (entry.element_name.length() > 0) {
	    std::cout << " " << entry.element_name;
	  }
	}
	std::cout << std::endl;
	++index;
    }
  }
}

void BUFRReport::print_descriptors(InputBUFRStream& istream,bool verbose)
{
  std::cout << "Total Number of Descriptors: " << num_descriptors << std::endl;
  print_descriptor_group("",0,num_descriptors,istream,verbose);
}

void BUFRReport::print_header(bool verbose) const
{
  if (verbose) {
    std::cout << "BUFR Ed: " << report.edition << "  Table: " << report.table << "  Center: " << std::setw(3) << report.src << "-" << std::setw(2) << report.sub_center << "  Lengths - Total: " << std::setw(5) << report.length << " IDS: " << std::setw(3) << report.ids_length << " OS: " << std::setw(3) << report.os_length << " DDS: " << std::setw(5) << report.dds_length << " DS: " << report.ds_length << std::endl;
    std::cout << "  Date: " << report.datetime.to_string() << "  Data Type: " << std::setw(3) << report.data_type << "-" << std::setw(3) << report.data_subtype;
    if ( (report.dds_flag & 0x80) == 0x80) {
	std::cout << "/OBSERVED,";
    }
    else {
	std::cout << "/OTHER,";
    }
    if ( (report.dds_flag & 0x40) == 0x40) {
	std::cout << "COMPRESSED";
    }
    else {
	std::cout << "NON-COMPRESSED";
    }
    std::cout << "  Table Version: " << std::setw(2) << report.master_version << "-" << std::setw(2) << report.local_version << "  Subsets: " << std::setw(4) << report.nsubs << std::endl;
  }
  else {
    std::cout << "Ed=" << report.edition << " Tbl=" << report.table << " Ctr=" << report.src << "-" << report.sub_center << " Data=" << report.data_type << "-" << report.data_subtype << " Ver=" << report.master_version << "-" << report.local_version << " Date=" << report.datetime.to_string() << std::endl;
  }
}

void BUFRReport::unpack_is(const unsigned char *input_buffer)
{
  if (input_buffer[0] != 0x42 || input_buffer[1] != 0x55 || input_buffer[2] != 0x46 || input_buffer[3] != 0x52) {
    myerror="not a BUFR report";
    exit(1);
  }
  bits::get(input_buffer,report.length,32,24);
  bits::get(input_buffer,report.edition,56,8);
  report.is_length=8;
}

void BUFRReport::unpack_ids(const unsigned char *input_buffer)
{
  size_t off=report.is_length*8;
  bits::get(input_buffer,report.ids_length,off,24);
  bits::get(input_buffer,report.table,off+24,8);
  short yr,mo,dy;
  size_t hr,min,sec=0;
  switch (report.edition) {
    case 2:
    case 3: {
	if (report.edition == 2) {
	  bits::get(input_buffer,report.src,off+32,16);
	  report.sub_center=255;
	}
	else {
	  bits::get(input_buffer,report.sub_center,off+32,8);
	  bits::get(input_buffer,report.src,off+40,8);
	}
	bits::get(input_buffer,report.update,off+48,8);
	short flag;
	bits::get(input_buffer,flag,off+56,8);
	if ( (flag & 0x80) == 0x80) {
	  report.os_included=true;
	}
	else {
	  report.os_included=false;
	  report.os_length=0;
	}
	bits::get(input_buffer,report.data_type,off+64,8);
	bits::get(input_buffer,report.data_subtype,off+72,8);
	bits::get(input_buffer,report.master_version,off+80,8);
	bits::get(input_buffer,report.local_version,off+88,8);
	bits::get(input_buffer,yr,off+96,8);
	if (yr < 30) {
	  yr+=2000;
	}
	else {
	  yr+=1900;
	}
	bits::get(input_buffer,mo,off+104,8);
	bits::get(input_buffer,dy,off+112,8);
	bits::get(input_buffer,hr,off+120,8);
	bits::get(input_buffer,min,off+128,8);
	break;
    }
    case 4: {
	bits::get(input_buffer,report.src,off+32,16);
	bits::get(input_buffer,report.sub_center,off+48,16);
	bits::get(input_buffer,report.update,off+64,8);
	short flag;
	bits::get(input_buffer,flag,off+72,8);
	if ( (flag & 0x80) == 0x80) {
	  report.os_included=true;
	}
	else {
	  report.os_included=false;
	  report.os_length=0;
	}
	bits::get(input_buffer,report.data_type,off+80,8);
	bits::get(input_buffer,report.data_subtype,off+96,8);
	bits::get(input_buffer,report.master_version,off+104,8);
	bits::get(input_buffer,report.local_version,off+112,8);
	bits::get(input_buffer,yr,off+120,16);
	bits::get(input_buffer,mo,off+136,8);
	bits::get(input_buffer,dy,off+144,8);
	bits::get(input_buffer,hr,off+152,8);
	bits::get(input_buffer,min,off+160,8);
	bits::get(input_buffer,sec,off+168,8);
	break;
    }
    default: {
	myerror="unsupported edition: "+strutils::itos(report.edition);
	exit(1);
    }
  }
  auto time=hr*10000+min*100+sec;
  report.datetime.set(yr,mo,dy,time);
}

void BUFRReport::unpack_os_length(const unsigned char *input_buffer)
{
  size_t off=(report.is_length+report.ids_length)*8;
  bits::get(input_buffer,report.os_length,off,24);
}

void BUFRReport::decode_sequence(unsigned char xx,unsigned char yy,InputBUFRStream& istream,std::vector<Descriptor>& descriptor_list,std::vector<Descriptor>::iterator& it)
{
  Descriptor d;
  switch (xx) {
    case 0: {
	switch (yy) {
	  case 2: {
	    d.f=0;
	    d.x=0;
	    d.y=2;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=3;
	    descriptor_list.insert(it,d);
	    break;
	  }
	  case 3: {
	    d.f=0;
	    d.x=0;
	    d.y=10;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=11;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=12;
	    descriptor_list.insert(it,d);
	    break;
	  }
	  case 4: {
	    d.f=3;
	    d.x=0;
	    d.y=3;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=13;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=14;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=15;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=16;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=17;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=18;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=19;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=20;
	    descriptor_list.insert(it,d);
	    break;
	  }
	  case 10: {
	    d.f=3;
	    d.x=0;
	    d.y=3;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=1;
	    d.x=0;
	    d.y=0;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=31;
	    d.y=1;
	    it=descriptor_list.insert(it,d);
	    ++it;
	    d.f=0;
	    d.x=0;
	    d.y=30;
	    descriptor_list.insert(it,d);
	    break;
	  }
	  default: {
	    myerror="no Table D entry for sequence 3 "+strutils::itos(xx)+" "+strutils::itos(yy);
	    exit(1);
	  }
	}
	break;
    }
    default:
    {
	auto entry=istream.table_d_entry(3,xx,yy);
	if (entry.sequence.size() == 0) {
	  myerror="no Table D entry for sequence 3 "+strutils::itos(xx)+" "+strutils::itos(yy);
	  exit(1);
	}
	for (const auto& s : entry.sequence) {
	  it=descriptor_list.insert(it,s);
	  ++it;
	}
	break;
    }
  }
}

size_t BUFRReport::delayed_replication_factor(size_t& offset,InputBUFRStream& istream,const unsigned char *input_buffer,Descriptor descriptor)
{
  size_t rep_factor=0;
  if (descriptor.f != 0 || descriptor.x != 31) {
    myerror="bad delayed replication factor "+strutils::itos(descriptor.f)+" "+strutils::itos(descriptor.x)+" "+strutils::itos(descriptor.y);
    exit(1);
  }
  auto entry=istream.table_b_entry(0,31,descriptor.y);
  bits::get(input_buffer,rep_factor,offset,entry.width);
  offset+=entry.width;
  if ( (report.dds_flag & 0x40) == 0x40) {
    offset+=6;
  }
  return rep_factor;
}

void BUFRReport::unpack_dds(InputBUFRStream& istream,const unsigned char *input_buffer,std::string file_of_descriptors,bool fill_header_only)
{
  size_t off=(report.is_length+report.ids_length+report.os_length)*8;
  bits::get(input_buffer,report.dds_length,off,24);
  if (data_ != nullptr) {
    for (size_t n=0; n < static_cast<size_t>(report.nsubs); ++n) {
	if (data_[n] != nullptr) {
	  delete data_[n];
	}
    }
    delete[] data_;
    data_=nullptr;
  }
  bits::get(input_buffer,report.nsubs,off+32,16);
  data_=new BUFRData *[report.nsubs];
// header information for ECMWF BUFR is in the optional section
  for (short n=0; n < report.nsubs; ++n) {
    void (**f)(Descriptor&,const unsigned char*,const InputBUFRStream::TableBEntry&,size_t,BUFRData**,int,short,bool)=data_handler_.target<void(*)(Descriptor&,const unsigned char*,const InputBUFRStream::TableBEntry&,size_t,BUFRData**,int,short,bool)>();
    if (f != nullptr) {
	if (*f == handle_ncep_prepbufr) {
	  data_[n]=new NCEPPREPBUFRData;
	}
	else if (*f == handle_ncep_adpbufr) {
	  data_[n]=new NCEPADPBUFRData;
	}
	else if (*f == handle_ncep_radiance_bufr) {
	  data_[n]=new NCEPRadianceBUFRData;
	}
	else if (*f == handle_td5900_winds_bufr) {
	  data_[n]=new TD5900WindsBUFRData;
	}
	else if (*f == handle_td5900_surface_bufr) {
	  data_[n]=new TD5900SurfaceBUFRData;
	}
	else if (*f == handle_td5900_moment_bufr) {
	  data_[n]=new TD5900MomentBUFRData;
	}
	else if (*f == handle_ecmwf_bufr) {
	  data_[n]=new ECMWFBUFRData;
	}
    }
    else {
	data_[n]=nullptr;
    }
  }
  bits::get(input_buffer,report.dds_flag,off+48,8);
  std::vector<Descriptor> descriptor_list;
  static std::vector<Descriptor> last_descriptor_list;
  if (file_of_descriptors.length() == 0) {
    auto num_desc=(report.dds_length-7)/2;
    off+=56;
    for (size_t n=0; n < num_desc; ++n) {
	Descriptor descriptor;
	bits::get(input_buffer,descriptor.f,off,2);
	bits::get(input_buffer,descriptor.x,off+2,6);
	bits::get(input_buffer,descriptor.y,off+8,8);
	descriptor_list.emplace_back(descriptor);
	off+=16;
    }
  }
  else {
    std::ifstream ifs(file_of_descriptors.c_str());
    if (ifs.is_open()) {
	char line[256];
	ifs.getline(line,256);
	auto nlines=0;
	while (!ifs.eof()) {
	  ++nlines;
	  std::string sline=line;
	  auto sp=strutils::split(sline);
	  if (sp.size() != 3) {
	    myerror="Error in file of descriptors on line "+strutils::itos(nlines);
	    exit(1);
	  }
	  Descriptor descriptor;
	  descriptor.f=std::stoi(sp[0]);
	  descriptor.x=std::stoi(sp[1]);
	  descriptor.y=std::stoi(sp[2]);
	  descriptor_list.emplace_back(descriptor);
	  ifs.getline(line,256);
	}
	ifs.close();
    }
    else {
	myerror="Error opening file of descriptors";
	exit(1);
    }
  }
// expand sequences
  if (descriptor_list != last_descriptor_list) {
    last_descriptor_list=descriptor_list;
    struct {
	size_t *start,*end,num;
    } rep;
    auto loop_through_descriptors=true;
    while (loop_through_descriptors) {
	loop_through_descriptors=false;
// allocate markers for the replications
	rep.start=new size_t[descriptor_list.size()];
	rep.end=new size_t[descriptor_list.size()];
	rep.num=0;
	size_t n=0;
	auto it=descriptor_list.begin();
	auto end=descriptor_list.end();
	for (; it != end; ++it) {
	  auto descriptor=*it;
	  if (descriptor.f == 1) {
	    rep.start[rep.num]=n;
	    if (descriptor.y == 0) {
		rep.end[rep.num]=n+descriptor.x+1;
	    }
	    else {
		rep.end[rep.num]=n+descriptor.x;
	    }
	    ++rep.num;
	  }
	  else if (descriptor.f == 3) {
	    loop_through_descriptors=true;
	    it=descriptor_list.erase(it);
	    auto num_added=descriptor_list.size();
	    decode_sequence(descriptor.x,descriptor.y,istream,descriptor_list,it);
	    num_added=descriptor_list.size()-num_added-1;
	    for (size_t m=0; m < rep.num; ++m) {
		if (n > rep.start[m] && n <= rep.end[m]) {
		  it=descriptor_list.begin();
		  for (int l=0,lend=rep.start[m]; l < lend; ++l) {
		    ++it;
		  }
		  descriptor=*it;
		  it=descriptor_list.erase(it);
		  descriptor.x+=num_added;
		  descriptor_list.insert(it,descriptor);
		}
	    }
	    break;
	  }
	  ++n;
	}
	delete[] rep.start;
	delete[] rep.end;
	rep.num=0;
    }
    if (descriptor_list.size() > descriptors.size()) {
	descriptors.resize(descriptor_list.size());
    }
    num_descriptors=descriptor_list.size();
    auto n=0;
    for (const auto& d : descriptor_list) {
	descriptors[n++]=d;
    }
  }
}

void BUFRReport::decode_os(const unsigned char *input_buffer)
{
  size_t off=(report.is_length+report.ids_length)*8;
  bits::get(input_buffer,report.os_length,off,24);
  switch (report.src) {
    case 98:
	for (short n=0; n < report.nsubs; ++n) {
	  (reinterpret_cast<ECMWFBUFRData *>(data_[n]))->fill(input_buffer,off);
	}
	break;
  }
}

void BUFRReport::do_substitution(size_t& offset,Descriptor descriptor,InputBUFRStream& istream,const unsigned char *input_buffer,int subset,bool fill_header_only)
{
  std::vector<Descriptor>::iterator it;
  auto end=data_present.descriptors.end();
  if (data_present.bitmap_index == 0) {
    it=data_present.descriptors.begin();
    for (int n=0,m=(data_present.descriptors.size()-data_present.bitmap.length()-data_present.num_operators); n < m; ++n) {
	++it;
    }
  }
  else {
    it=end;
  }
  while (it->f == 2) {
    decode_descriptor(offset,*it,istream,input_buffer,subset,fill_header_only);
    ++it;
  }
  while (data_present.bitmap[data_present.bitmap_index] == '1') {
    ++data_present.bitmap_index;
    ++it;
    while (it->f == 2) {
	decode_descriptor(offset,*it,istream,input_buffer,subset,fill_header_only);
	++it;
    }
  }
  while (it->f == 2) {
    decode_descriptor(offset,*it,istream,input_buffer,subset,fill_header_only);
    ++it;
  }
  if (descriptor.x == 25) {
    ++report.change_width;
  }
  decode_descriptor(offset,*it,istream,input_buffer,subset,fill_header_only);
  if (descriptor.x == 25) {
    --report.change_width;
  }
  ++data_present.bitmap_index;
  ++it;
  while (it != end && it->f == 2 && it->y == 0) {
    decode_descriptor(offset,*it,istream,input_buffer,subset,fill_header_only);
    ++it;
  }
}

void BUFRReport::decode_descriptor(size_t& offset,Descriptor& descriptor,InputBUFRStream& istream,const unsigned char *input_buffer,int subset,bool fill_header_only)
{
  char dum[65];
  Descriptor sequence_descriptor;
  int n;
  short nbinc;
  static InputBUFRStream::TableAEntry aentry;
  InputBUFRStream::TableBEntry bentry;
  std::string sdum;
  double *parray;

  switch (descriptor.f) {
    case 0: {
	bentry=istream.table_b_entry(0,descriptor.x,descriptor.y);
	if (report.local_desc_width > 0) {
	  bentry.width=report.local_desc_width;
	}
	if (bentry.width == 0) {
	  myerror="no Table B entry found for descriptor "+strutils::itos(descriptor.f)+" "+strutils::itos(descriptor.x)+" "+strutils::itos(descriptor.y);
	  exit(1);
	}
	sdum=strutils::to_lower(bentry.units);
	if (!strutils::contains(sdum,"code") && !strutils::contains(sdum,"flag")) {
	  if (!strutils::contains(sdum,"ccitt") && report.local_desc_width == 0) {
	    bentry.width+=report.change_width;
	    bentry.scale+=report.change_scale;
	  }
	  else if (strutils::contains(sdum,"ccitt") && report.change_ccitt_width > 0) {
	    bentry.width=report.change_ccitt_width;
	  }
	  bentry.ref_val*=report.change_ref_val;
	}
	report.local_desc_width=0;
	if (report.dump_descriptors) {
	  std::cout << "DUMP: " << static_cast<int>(descriptor.f) << " " << std::setfill('0') << std::setw(2) << static_cast<int>(descriptor.x) << " " << std::setw(3) << static_cast<int>(descriptor.y) << std::setfill(' ') << " offset: " << offset << " width: " << bentry.width << " value: ";
	  if (!std::regex_search(sdum,std::regex("^ccitt"))) {
	    long long v=0xffffffffffffffff;
	    bits::get(input_buffer,v,offset,bentry.width);
	    if (std::regex_search(sdum,std::regex("^flag"))) {
		std::cout << "0x" << std::hex << static_cast<long long>(bufr_value(v,bentry)) << std::dec;
	    }
	    else {
		std::cout << bufr_value(v,bentry);
	    }
	  }
	  else {
	    n=bentry.width/8;
	    std::unique_ptr<unsigned char []> temp(new unsigned char[n]);
	    bits::get(input_buffer,temp.get(),offset,8,0,n);
	    for (int m=0; m < n; ++m) {
		if (temp[m] == 0xff) {
		  temp[m]=' ';
		}
	    }
	    std::cout << "'";
	    std::cout.write(reinterpret_cast<char *>(temp.get()),n);
	    std::cout << "'";
	    temp=nullptr;
	  }
	  std::cout << std::endl;
	}
	switch (descriptor.x) {
	  case 0: {
// table entries
	    auto num_chars=bentry.width/8;
	    switch (descriptor.y) {
		case 1: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  dum[num_chars]='\0';
		  aentry.key=std::stoi(dum);
		  aentry.description="";
		  break;
		}
		case 2: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  dum[num_chars]='\0';
		  aentry.description+=std::string(dum);
		  strutils::trim(aentry.description);
		  istream.add_entry_to_table_a(aentry);
		  break;
		}
		case 3: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  dum[num_chars]='\0';
		  aentry.description+=std::string(dum);
		  strutils::trim(aentry.description);
		  istream.update_table_a_entry(aentry);
		  break;
		}
		case 10: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  dum[num_chars]='\0';
		  report.def.f=std::stoi(dum);
		  break;
		}
		case 11: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  dum[num_chars]='\0';
		  report.def.x=std::stoi(dum);
		  break;
		}
		case 12: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  dum[num_chars]='\0';
		  report.def.y=std::stoi(dum);
		  break;
		}
		case 13: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  dum[num_chars]='\0';
		  n=num_chars-1;
		  while (n >= 0 && dum[n] == ' ') {
		    dum[n]='\0';
		    --n;
		  }
		  break;
		}
		case 14: {
		  n=strlen(dum);
		  bits::get(input_buffer,&dum[n],offset,8,0,num_chars);
		  n+=num_chars;
		  dum[n]='\0';
		  --n;
		  while (n >= 0 && dum[n] == ' ') {
		    dum[n]='\0';
		    --n;
		  }
		  istream.define_table_b_element_name(report.def,dum);
		  break;
		}
		case 15: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  dum[num_chars]='\0';
		  n=num_chars-1;
		  while (n >= 0 && dum[n] == ' ') {
		    dum[n]='\0';
		    --n;
		  }
		  istream.define_table_b_units_name(report.def,dum);
		  break;
		}
		case 16: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  istream.define_table_b_scale_sign(report.def,dum[0]);
		  break;
		}
		case 17: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  dum[num_chars]='\0';
		  istream.define_table_b_scale(report.def,std::stoi(dum));
		  break;
		}
		case 18: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  istream.define_table_b_reference_value_sign(report.def,dum[0]);
		  break;
		}
		case 19: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  dum[num_chars]='\0';
		  istream.define_table_b_reference_value(report.def,std::stoi(dum));
		  break;
		}
		case 20: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  dum[num_chars]='\0';
		  istream.define_table_b_data_width(report.def,std::stoi(dum));
		  break;
		}
		case 30: {
		  bits::get(input_buffer,dum,offset,8,0,num_chars);
		  strutils::strget(dum,sequence_descriptor.f,1);
		  strutils::strget(&dum[1],sequence_descriptor.x,2);
		  strutils::strget(&dum[3],sequence_descriptor.y,3);
		  istream.add_table_d_sequence_descriptor(report.def,sequence_descriptor);
		  break;
		}
	    }
	    break;
	  }
	  default: {
	    if (descriptor.x == 31 && descriptor.y == 31) {
		if (!data_present.bitmap_started) {
		  data_present.bitmap="";
		  data_present.bitmap_started=true;
		}
		if (subset >= 0) {
		  bits::get(input_buffer,n,offset,1);
		  if (n == 0) {
		    data_present.bitmap+="0";
		  }
		  else {
		    data_present.bitmap+="1";
		  }
		}
		else {
		  unpack_bufr_values(input_buffer,offset,1,0,0,&parray,report.nsubs);
		  if (floatutils::myequalf(parray[0],0.)) {
		    data_present.bitmap+="0";
		  }
		  else {
		    data_present.bitmap+="1";
		  }
		  delete[] parray;
		  bits::get(input_buffer,nbinc,offset+bentry.width,6);
		  offset+=(6+nbinc*report.nsubs);
		}
	    }
	    else if (data_handler_ != nullptr) {
		if (!descriptor.ignore) {
		  data_handler_(descriptor,input_buffer,bentry,offset,data_,subset,report.nsubs,fill_header_only);
		}
		if (subset >= 0) {
		  data_[subset]->filled=!fill_header_only;
		}
		else {
		  bits::get(input_buffer,nbinc,offset+bentry.width,6);
		  offset+=(6+nbinc*report.nsubs);
		}
	    }
	  }
	}
	offset+=bentry.width;
// patch for ECMWF files
	void (**f)(Descriptor&,const unsigned char*,const InputBUFRStream::TableBEntry&,size_t,BUFRData**,int,short,bool)=data_handler_.target<void(*)(Descriptor&,const unsigned char*,const InputBUFRStream::TableBEntry&,size_t,BUFRData**,int,short,bool)>();
	if (f != nullptr && *f == handle_ecmwf_bufr && (reinterpret_cast<ECMWFBUFRData **>(data_))[0]->ecmwf_os.rdb_type == 5 && (reinterpret_cast<ECMWFBUFRData **>(data_))[0]->ecmwf_os.rdb_subtype == 103 && descriptor.x == 20 && descriptor.y == 13) {
	  offset+=15;
	}
	if (data_present.add_to_descriptors) {
	  data_present.descriptors.emplace_back(descriptor);
	}
	break;
    }
    case 2: {
/*
if (descriptor.x >= 21) {
std::cerr << (int)descriptor.f << " " << (int)descriptor.x << " " << (int)descriptor.y << " data present descriptors length = " << data_present.descriptors.length() << " " << data_present.bitmap.length() << " '" << data_present.bitmap << "'" << std::endl;
}
*/
	switch(descriptor.x) {
	  case 1: {
	    if (descriptor.y > 0) {
		report.change_width=report.change_width+descriptor.y-128;
	    }
	    else {
		report.change_width=0;
	    }
	    break;
	  }
	  case 2: {
	    if (descriptor.y > 0) {
		report.change_scale=report.change_scale+descriptor.y-128;
	    }
	    else {
		report.change_scale=0;
	    }
	    break;
	  }
	  case 5: {
	    if (report.dump_descriptors) {
		std::unique_ptr<unsigned char []> temp(new unsigned char[descriptor.y]);
		bits::get(input_buffer,temp.get(),offset,8,0,descriptor.y);
		std::cout << "DUMP: " << static_cast<int>(descriptor.f) << " " << std::setfill('0') << std::setw(2) << static_cast<int>(descriptor.x) << " " << std::setw(3) << static_cast<int>(descriptor.y) << std::setfill(' ') << " offset: " << offset << " width: " << static_cast<int>(descriptor.y) << " value: '";
		std::cout.write(reinterpret_cast<char *>(temp.get()),descriptor.y);
		std::cout << "'" << std::endl;
		temp=nullptr;
	    }
	    offset+=(descriptor.y*8);
	    break;
	  }
	  case 6: {
	    report.local_desc_width=descriptor.y;
	    break;
	  }
	  case 7: {
	    if (descriptor.y > 0) {
		report.change_width=report.change_width+((10.*descriptor.y)+2.)/3.;
		report.change_scale=report.change_scale+descriptor.y;
		report.change_ref_val*=pow(10.,descriptor.y);
	    }
	    else {
		report.change_width=0;
		report.change_scale=0;
		report.change_ref_val=1;
	    }
	    break;
	  }
	  case 8: {
	    report.change_ccitt_width=descriptor.y*8;
	    break;
	  }
	  case 22: {
	    switch (descriptor.y) {
		default:
		  data_present.add_to_descriptors=false;
		  break;
	    }
	    break;
	  }
	  case 23: {
	    switch (descriptor.y) {
		case 0: {
		  data_present.add_to_descriptors=false;
		  data_present.bitmap_index=0;
		  break;
		}
		case 255: {
		  do_substitution(offset,descriptor,istream,input_buffer,subset,fill_header_only);
		  break;
		}
		default: {
		  myerror="bad operand descriptor "+strutils::itos(descriptor.f)+" "+strutils::itos(descriptor.x)+" "+strutils::itos(descriptor.y);
		  exit(1);
		}
	    }
	    break;
	  }
	  case 24: {
	    switch (descriptor.y) {
		case 0: {
		  data_present.add_to_descriptors=false;
		  data_present.bitmap_index=0;
		  break;
		}
		case 255: {
		  do_substitution(offset,descriptor,istream,input_buffer,subset,fill_header_only);
		  break;
		}
		default: {
		  myerror="bad operand descriptor "+strutils::itos(descriptor.f)+" "+strutils::itos(descriptor.x)+" "+strutils::itos(descriptor.y);
		  exit(1);
		}
	    }
	    break;
	  }
	  case 25: {
	    switch (descriptor.y) {
		case 0: {
		  data_present.add_to_descriptors=false;
		  data_present.bitmap_index=0;
		  break;
		}
		case 255: {
		  do_substitution(offset,descriptor,istream,input_buffer,subset,fill_header_only);
		  break;
		}
		default: {
		  myerror="bad operand descriptor "+strutils::itos(descriptor.f)+" "+strutils::itos(descriptor.x)+" "+strutils::itos(descriptor.y);
		  exit(1);
		}
	    }
	    break;
	  }
	  case 32: {
	    switch (descriptor.y) {
		case 0: {
		  data_present.add_to_descriptors=false;
		  data_present.bitmap_index=0;
		  break;
		}
		case 255: {
		  do_substitution(offset,descriptor,istream,input_buffer,subset,fill_header_only);
		  break;
		}
		default: {
		  myerror="bad operand descriptor "+strutils::itos(descriptor.f)+" "+strutils::itos(descriptor.x)+" "+strutils::itos(descriptor.y);
		  exit(1);
		}
	    }
	    break;
	  }
	  case 35: {
	    switch (descriptor.y) {
		case 0: {
		  data_present.add_to_descriptors=true;
		  data_present.num_operators=0;
		  data_present.descriptors.clear();
		  break;
		}
		default: {
		  myerror="bad operand descriptor "+strutils::itos(descriptor.f)+" "+strutils::itos(descriptor.x)+" "+strutils::itos(descriptor.y);
		  exit(1);
		}
	    }
	    break;
	  }
	  case 36: {
	    switch (descriptor.y) {
		case 0: {
		  data_present.add_to_descriptors=false;
		  break;
		}
		default: {
		  myerror="bad operand descriptor "+strutils::itos(descriptor.f)+" "+strutils::itos(descriptor.x)+" "+strutils::itos(descriptor.y);
		  exit(1);
		}
	    }
	    break;
	  }
	  case 37: {
	    switch (descriptor.y) {
		case 0: {
		  data_present.add_to_descriptors=false;
		  data_present.bitmap_index=0;
		  break;
		}
		case 255: {
		  data_present.add_to_descriptors=true;
		  data_present.num_operators=0;
		  data_present.descriptors.clear();
		  break;
		}
		default: {
		  myerror="bad operand descriptor "+strutils::itos(descriptor.f)+" "+strutils::itos(descriptor.x)+" "+strutils::itos(descriptor.y);
		  exit(1);
		}
	    }
	    break;
	  }
	  default: {
	    myerror="bad operand descriptor "+strutils::itos(descriptor.f)+" "+strutils::itos(descriptor.x)+" "+strutils::itos(descriptor.y);
	    exit(1);
	  }
	}
	if (descriptor.x < 21 && data_present.add_to_descriptors) {
	  data_present.descriptors.emplace_back(descriptor);
	  ++data_present.num_operators;
	}
	break;
    }
    default: {
	myerror="bad value for \"F\"";
	exit(1);
    }
  }
}

void BUFRReport::decode_descriptor_list(size_t& offset,InputBUFRStream& istream,const unsigned char *input_buffer,size_t start_index,size_t num_to_decode,size_t num_reps,int subset,bool fill_header_only)
{
  size_t n,m;
  size_t num_to_replicate,rep_factor;

//std::cerr << "offset=" << offset << " num_reps=" << num_reps << std::endl;
/*
std::cerr << std::hex;
for (n=offset/8-3; n < offset/8+800; n++)
std::cerr << (int)input_buffer[n] << " ";
std::cerr << std::dec << std::endl;
*/
  for (n=0; n < num_reps; ++n) {
    for (m=start_index; m < start_index+num_to_decode; ++m) {
	if (descriptors[m].f != 0 || descriptors[m].x != 31 || descriptors[m].y != 31) {
	  data_present.bitmap_started=false;
	}
	if (descriptors[m].f == 1) {
	  num_to_replicate=descriptors[m].x;
	  if (descriptors[m].y == 0) {
	    rep_factor=delayed_replication_factor(offset,istream,input_buffer,descriptors[++m]);
	    if (data_present.add_to_descriptors) {
		data_present.descriptors.emplace_back(descriptors[m]);
	    }
	    decode_descriptor_list(offset,istream,input_buffer,m+1,num_to_replicate,rep_factor,subset,fill_header_only);
	  }
	  else {
	    decode_descriptor_list(offset,istream,input_buffer,m+1,num_to_replicate,descriptors[m].y,subset,fill_header_only);
	  }
	  m+=num_to_replicate;
	}
	else {
	  decode_descriptor(offset,descriptors[m],istream,input_buffer,subset,fill_header_only);
	}
    }
  }
}

void BUFRReport::unpack_ds(InputBUFRStream& istream,const unsigned char *input_buffer,bool fill_header_only)
{
  size_t off=(report.is_length+report.ids_length+report.os_length+report.dds_length)*8;
  short n;

  bits::get(input_buffer,report.ds_length,off,24);
  off+=32;
  if ( (report.dds_flag & 0x40) == 0x40) {
    decode_descriptor_list(off,istream,input_buffer,0,num_descriptors,1,-1,fill_header_only);
  }
  else {
    for (n=0; n < report.nsubs; ++n) {
	decode_descriptor_list(off,istream,input_buffer,0,num_descriptors,1,n,fill_header_only);
    }
  }
}

void BUFRReport::unpack_end(const unsigned char *input_buffer)
{
  if (strncmp(reinterpret_cast<char *>(const_cast<unsigned char *>(input_buffer))+report.length-4,"7777",4) != 0) {
    myerror="bad END section: "+report.datetime.to_string();
    exit(1);
  }
}

double bufr_value(long long packed_value,short width,short scale,int reference_value)
{
  if (packed_value == static_cast<int>(pow(2.,width)-1)) {
    return -99999.;
  }
  else {
    return (packed_value+reference_value)/pow(10.,scale);
  }
}

double bufr_value(long long packed_value,InputBUFRStream::TableBEntry bentry)
{
  return bufr_value(packed_value,bentry.width,bentry.scale,bentry.ref_val);
}

void unpack_bufr_values(const unsigned char *buffer,size_t offset,short width,short scale,int reference_value,double **packed_values,short num_values)
{
  double r0;
  long long packed=0;
  short nbinc;

  *packed_values=new double[num_values];
  bits::get(buffer,packed,offset,width);
  r0=bufr_value(packed,width,scale,reference_value);
  bits::get(buffer,nbinc,offset+width,6);
  offset+=(width+6);
  for (auto n=0; n < num_values; ++n) {
    if (r0 < -99998.) {
	(*packed_values)[n]=r0;
    }
    else {
	if (nbinc > 0) {
	  bits::get(buffer,packed,offset,nbinc);
	  (*packed_values)[n]=r0+bufr_value(packed,nbinc,scale,0);
	}
	else {
	  (*packed_values)[n]=r0;
	}
    }
    offset+=nbinc;
  }
}
