#include <iostream>
#include <netcdf.hpp>
#include <strutils.hpp>
#include <bits.hpp>
#include <xmlutils.hpp>
#include <tempfile.hpp>
#include <myerror.hpp>

const char *netCDFStream::nc_type[7]={"null","byte","char","short","int","float","double"};
const short netCDFStream::nc_size[7]={0,1,1,2,4,4,8};

netCDFStream::Attribute& netCDFStream::Attribute::operator=(const Attribute& source)
{
  if (this == &source) {
    return *this;
  }
  name=source.name;
  clear_values();
  nc_type=source.nc_type;
  num_values=source.num_values;
  if (source.values != nullptr) {
    switch (nc_type) {
	case NcType::BYTE:
	{
	    values=new unsigned char[num_values];
	    for (size_t n=0; n < num_values; ++n) {
	      (reinterpret_cast<unsigned char *>(values))[n]=(reinterpret_cast<unsigned char *>(source.values))[n];
	    }
	    break;
	}
	case NcType::CHAR:
	{
	    values=new std::string;
	    *(reinterpret_cast<std::string *>(values))=*(reinterpret_cast<std::string *>(source.values));
	    break;
	}
	case NcType::SHORT:
	{
	    values=new short[num_values];
	    for (size_t n=0; n < num_values; ++n) {
	      (reinterpret_cast<short *>(values))[n]=(reinterpret_cast<short *>(source.values))[n];
	    }
	    break;
	}
	case NcType::INT:
	{
	    values=new int[num_values];
	    for (size_t n=0; n < num_values; ++n) {
	      (reinterpret_cast<int *>(values))[n]=(reinterpret_cast<int *>(source.values))[n];
	    }
	    break;
	}
	case NcType::FLOAT:
	{
	    values=new float[num_values];
	    for (size_t n=0; n < num_values; ++n) {
	      (reinterpret_cast<float *>(values))[n]=(reinterpret_cast<float *>(source.values))[n];
	    }
	    break;
	}
	case NcType::DOUBLE:
	{
	    values=new double[num_values];
	    for (size_t n=0; n < num_values; ++n) {
	      (reinterpret_cast<double *>(values))[n]=(reinterpret_cast<double *>(source.values))[n];
	    }
	    break;
	}
	default: {}
    }
  }
  return *this;
}

void netCDFStream::Attribute::clear_values()
{
  if (values != nullptr) {
    switch (nc_type) {
	case NcType::BYTE:
	{
	    delete[] reinterpret_cast<unsigned char *>(values);
	    break;
	}
	case NcType::CHAR:
	{
	    delete reinterpret_cast<std::string *>(values);
	    break;
	}
	case NcType::SHORT:
	{
	    delete reinterpret_cast<short *>(values);
	    break;
	}
	case NcType::INT:
	{
	    delete reinterpret_cast<int *>(values);
	    break;
	}
	case NcType::FLOAT:
	{
	    delete reinterpret_cast<float *>(values);
	    break;
	}
	case NcType::DOUBLE:
	{
	    delete reinterpret_cast<double *>(values);
	    break;
	}
	default: {}
    }
  }
}

void netCDFStream::UniqueVariableTimeEntry::free_memory()
{
  first_valid_datetime.reset();
  last_valid_datetime.reset();
  reference_datetime.reset();
}

void netCDFStream::UniqueVariableLevelEntry::free_memory()
{
  value.reset();
  value2.reset();
}

void netCDFStream::UniqueVariableDataEntry::free_memory()
{
  if (time_entry != nullptr) {
    time_entry->free_memory();
    time_entry.reset();
  }
  record_offset.reset();
  record_length.reset();
  if (level != nullptr) {
    level->free_memory();
    level.reset();
  }
}

void netCDFStream::UniqueVariableEntry::free_memory()
{
  for (auto& t : data->times) {
    t.free_memory();
  }
  for (auto& d : data->data) {
    d.free_memory();
  }
  data.reset();
}

netCDFStream::DataValue& netCDFStream::DataValue::operator=(const DataValue& source) {
  if (this == &source) {
    return *this;
  }
  resize(source.nc_type);
  nc_type=source.nc_type;
  set(source.get());
  return *this;
}

void netCDFStream::DataValue::clear()
{
  if (value != nullptr) {
    switch (nc_type) {
	case NcType::BYTE:
	{
	    delete reinterpret_cast<unsigned char *>(value);
	    break;
	}
	case NcType::CHAR:
	{
	    delete reinterpret_cast<char *>(value);
	    break;
	}
	case NcType::SHORT:
	{
	    delete reinterpret_cast<short *>(value);
	    break;
	}
	case NcType::INT:
	{
	    delete reinterpret_cast<int *>(value);
	    break;
	}
	case NcType::FLOAT:
	{
	    delete reinterpret_cast<float *>(value);
	    break;
	}
	case NcType::DOUBLE:
	{
	    delete reinterpret_cast<double *>(value);
	    break;
	}
	default: {}
    }
    value=nullptr;
    nc_type=NcType::_NULL;
  }
}

double netCDFStream::DataValue::get() const
{
  switch (nc_type) {
    case NcType::BYTE:
    {
	return *(reinterpret_cast<unsigned char *>(value));
    }
    case NcType::CHAR:
    {
	return *(reinterpret_cast<char *>(value));
    }
    case NcType::SHORT:
    {
	return *(reinterpret_cast<short *>(value));
    }
    case NcType::INT:
    {
	return *(reinterpret_cast<int *>(value));
    }
    case NcType::FLOAT:
    {
	return *(reinterpret_cast<float *>(value));
    }
    case NcType::DOUBLE:
    {
	return *(reinterpret_cast<double *>(value));
    }
    default:
    {
	return 3.4e38;
    }
  }
}

void netCDFStream::DataValue::resize(NcType type)
{
  if (type == nc_type) {
    return;
  }
  if (value != nullptr) {
    clear();
  }
  nc_type=type;
  switch (nc_type) {
    case NcType::BYTE:
    {
	value=new unsigned char;
	*(reinterpret_cast<unsigned char *>(value))=0;
	break;
    }
    case NcType::CHAR:
    {
	value=new char;
	*(reinterpret_cast<char *>(value))=0;
	break;
    }
    case NcType::SHORT:
    {
	value=new short;
	*(reinterpret_cast<short *>(value))=0;
	break;
    }
    case NcType::INT:
    {
	value=new int;
	*(reinterpret_cast<int *>(value))=0;
	break;
    }
    case NcType::FLOAT:
    {
	value=new float;
	*(reinterpret_cast<float *>(value))=0.;
	break;
    }
    case NcType::DOUBLE:
    {
	value=new double;
	*(reinterpret_cast<double *>(value))=0.;
	break;
    }
    default: {}
  };
}

void netCDFStream::DataValue::set(double source_value)
{
  switch (nc_type) {
    case NcType::BYTE:
    {
	*(reinterpret_cast<unsigned char *>(value))=source_value;
	break;
    }
    case NcType::CHAR:
    {
	*(reinterpret_cast<char *>(value))=source_value;
	break;
    }
    case NcType::SHORT:
    {
	*(reinterpret_cast<short *>(value))=source_value;
	break;
    }
    case NcType::INT:
    {
	*(reinterpret_cast<int *>(value))=source_value;
	break;
    }
    case NcType::FLOAT:
    {
	*(reinterpret_cast<float *>(value))=source_value;
	break;
    }
    case NcType::DOUBLE:
    {
	*(reinterpret_cast<double *>(value))=source_value;
	break;
    }
    default: {}
  }
}

netCDFStream::VariableData& netCDFStream::VariableData::operator=(const VariableData& source)
{
  if (this == &source) {
    return *this;
  }
  resize(source.num_values,source.nc_type);
  num_values=source.num_values;
  nc_type=source.nc_type;
  for (size_t n=0; n < num_values; ++n) {
    set(n,source[n]);
  }
  return *this;
}

double netCDFStream::VariableData::operator[](size_t index) const
{
  if (index < num_values) {
    switch (nc_type) {
	case NcType::BYTE:
	{
	    return (reinterpret_cast<unsigned char *>(values))[index];
	}
	case NcType::CHAR:
	{
	    return (reinterpret_cast<char *>(values))[index];
	}
	case NcType::SHORT:
	{
	    return (reinterpret_cast<short *>(values))[index];
	}
	case NcType::INT:
	{
	    return (reinterpret_cast<int *>(values))[index];
	}
	case NcType::FLOAT:
	{
	    return (reinterpret_cast<float *>(values))[index];
	}
	case NcType::DOUBLE:
	{
	    return (reinterpret_cast<double *>(values))[index];
	}
	default: {}
    }
  }
  return 3.4e38;
}

void netCDFStream::VariableData::clear()
{
  if (values != nullptr) {
    switch (nc_type) {
	case NcType::BYTE:
	{
	    delete[] reinterpret_cast<unsigned char *>(values);
	    break;
	}
	case NcType::CHAR:
	{
	    delete[] reinterpret_cast<char *>(values);
	    break;
	}
	case NcType::SHORT:
	{
	    delete[] reinterpret_cast<short *>(values);
	    break;
	}
	case NcType::INT:
	{
	    delete[] reinterpret_cast<int *>(values);
	    break;
	}
	case NcType::FLOAT:
	{
	    delete[] reinterpret_cast<float *>(values);
	    break;
	}
	case NcType::DOUBLE:
	{
	    delete[] reinterpret_cast<double *>((values));
	    break;
	}
	default: {}
    };
    values=nullptr;
    nc_type=NcType::_NULL;
  }
}

void netCDFStream::VariableData::resize(int new_size,NcType type)
{
  num_values=new_size;
  if (type == nc_type && new_size <= capacity) {
    return;
  }
  if (values != nullptr) {
    clear();
  }
  capacity=new_size;
  nc_type=type;
  switch (nc_type) {
    case NcType::BYTE:
    {
	values=new unsigned char[capacity];
	break;
    }
    case NcType::CHAR:
    {
	values=new char[capacity];
	break;
    }
    case NcType::SHORT:
    {
	values=new short[capacity];
	break;
    }
    case NcType::INT:
    {
	values=new int[capacity];
	break;
    }
    case NcType::FLOAT:
    {
	values=new float[capacity];
	break;
    }
    case NcType::DOUBLE:
    {
	values=new double[capacity];
	break;
    }
    default: {}
  }
}

void netCDFStream::VariableData::set(size_t index,double value)
{
  if (index < num_values) {
    switch (nc_type) {
	case NcType::BYTE:
	{
	    (reinterpret_cast<unsigned char *>(values))[index]=value;
	    break;
	}
	case NcType::CHAR:
	{
	    (reinterpret_cast<char *>(values))[index]=value;
	    break;
	}
	case NcType::SHORT:
	{
	    (reinterpret_cast<short *>(values))[index]=value;
	    break;
	}
	case NcType::INT:
	{
	    (reinterpret_cast<int *>(values))[index]=value;
	    break;
	}
	case NcType::FLOAT:
	{
	    (reinterpret_cast<float *>(values))[index]=value;
	    break;
	}
	case NcType::DOUBLE:
	{
	    (reinterpret_cast<double *>(values))[index]=value;
	    break;
	}
	default: {}
    }
  }
}

int netCDFStream::find_variable(std::string variable_name) const
{
  int n=0;
  while (n < static_cast<int>(vars.size()) && vars[n].name != variable_name) {
    ++n;
  }
  if (n == static_cast<int>(vars.size())) {
    n=-1;
  }
  return n;
}

bool InputNetCDFStream::open(std::string filename)
{
  if (fs.is_open()) {
    std::cerr << "Error: there is already an open InputNetCDFStream" << std::endl;
    exit(1);
  }
  file_name=filename;
  fs.open(file_name.c_str(),std::ios_base::in);
  if (!fs.is_open()) {
    myerror="unable to open file or file not found";
    return false;
  }
  rec_size=0;
  char magic[4];
  fs.read(magic,4);
  if (fs.gcount() != 4) {
    myerror="unable to read first four bytes in file";
    return false;
  }
  if (magic[0] != 'C' || magic[1] != 'D' || magic[2] != 'F') {
    myerror="did not find 'CDF' in first three bytes of file";
    return false;
  }
  _version=magic[3];
  fs.read(magic,4);
  bits::get(reinterpret_cast<unsigned char *>(magic),num_recs,0,32);
  fill_dimensions();
  fill_attributes(gattrs);
  fill_variables();
  return true;
}

bool InputNetCDFStream::close()
{
  if (!fs.is_open()) {
    myerror="no open input stream";
    return false;
  }
  fs.close();
  fs.clear();
  dims.clear();
  gattrs.clear();
  vars.clear();
  return true;
}

void InputNetCDFStream::print_dimensions() const
{
  std::cout << "Dimensions (" << dims.size() << "):" << std::endl;
  for (size_t n=0; n < dims.size(); ++n) {
    std::cout << "  " << dims[n].name << " = ";
    if (dims[n].is_rec) {
	std::cout << num_recs << " (record dimension)";
    }
    else {
	std::cout << dims[n].length;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

void InputNetCDFStream::print_global_attributes() const
{
  std::cout << "Global Attributes (" << gattrs.size() << "):" << std::endl;
  for (const auto& attr : gattrs) {
    print_attribute(attr,2);
  }
  std::cout << std::endl;
}

void InputNetCDFStream::print_header() const
{
  std::cout.setf(std::ios::fixed);
  std::cout << "Version: " << _version << std::endl << std::endl;
  print_dimensions();
  print_global_attributes();
  print_variables();
}

netCDFStream::NcType InputNetCDFStream::variable_data(std::string variable_name,VariableData& variable_data)
{
  auto var_index=find_variable(variable_name);
  if (var_index < 0) {
    return NcType::_NULL;
  }
  fs.seekg(vars[var_index].offset,std::ios_base::beg);
  size_t num_values;
  switch (vars[var_index].nc_type) {
    case NcType::BYTE:
    case NcType::CHAR:
    {
	if (vars[var_index].is_rec) {
	  num_values=1;
	}
	else {
	  num_values=dims[vars[var_index].dimids[0]].length;
	}
	for (size_t n=1; n < vars[var_index].dimids.size(); ++n) {
	  num_values*=dims[vars[var_index].dimids[n]].length;
	}
	break;
    }
    default:
    {
	num_values=vars[var_index].size/nc_size[static_cast<int>(vars[var_index].nc_type)];
    }
  }
  size_t num_rec_vals;
  if (vars[var_index].is_rec) {
    num_rec_vals=num_values*num_recs;
    variable_data.resize(num_rec_vals,vars[var_index].nc_type);
    auto off=vars[var_index].offset;
    switch (vars[var_index].nc_type) {
	case NcType::BYTE:
	case NcType::CHAR:
	{
	  for (size_t n=0; n < num_rec_vals; n+=num_values) {
	    fs.read(&((reinterpret_cast<char *>(variable_data.get()))[n]),num_values);
	    off+=rec_size;
	    fs.seekg(off,std::ios_base::beg);
	  }
	  break;
	}
	case NcType::SHORT:
	{
	  auto *tmpbuf=new char[nc_size[static_cast<int>(NcType::SHORT)]*num_values];
	  for (size_t n=0; n < num_rec_vals; n+=num_values) {
	    fs.read(tmpbuf,nc_size[static_cast<int>(NcType::SHORT)]*num_values);
	    bits::get(reinterpret_cast<unsigned char *>(tmpbuf),&((reinterpret_cast<short *>(variable_data.get()))[n]),0,nc_size[static_cast<int>(NcType::SHORT)]*8,0,num_values);
	    off+=rec_size;
	    fs.seekg(off,std::ios_base::beg);
	  }
	  delete[] tmpbuf;
	  break;
	}
	case NcType::INT:
	{
	  auto *tmpbuf=new char[nc_size[static_cast<int>(NcType::INT)]*num_values];
	  for (size_t n=0; n < num_rec_vals; n+=num_values) {
	    fs.read(tmpbuf,nc_size[static_cast<int>(NcType::INT)]*num_values);
	    bits::get(reinterpret_cast<unsigned char *>(tmpbuf),&((reinterpret_cast<int *>(variable_data.get()))[n]),0,nc_size[static_cast<int>(NcType::INT)]*8,0,num_values);
	    off+=rec_size;
	    fs.seekg(off,std::ios_base::beg);
	  }
	  delete[] tmpbuf;
	  break;
	}
	case NcType::FLOAT:
	{
	  auto *tmpbuf=new char[nc_size[static_cast<int>(NcType::FLOAT)]*num_values];
	  for (size_t n=0; n < num_rec_vals; n+=num_values) {
	    fs.read(tmpbuf,nc_size[static_cast<int>(NcType::FLOAT)]*num_values);
	    bits::get(reinterpret_cast<unsigned char *>(tmpbuf),&((reinterpret_cast<int *>(variable_data.get()))[n]),0,nc_size[static_cast<int>(NcType::FLOAT)]*8,0,num_values);
	    off+=rec_size;
	    fs.seekg(off,std::ios_base::beg);
	  }
	  delete[] tmpbuf;
	  break;
	}
	case NcType::DOUBLE:
	{
	  auto *tmpbuf=new char[nc_size[static_cast<int>(NcType::DOUBLE)]*num_values];
	  for (size_t n=0; n < num_rec_vals; n+=num_values) {
	    fs.read(tmpbuf,nc_size[static_cast<int>(NcType::DOUBLE)]*num_values);
	    bits::get(reinterpret_cast<unsigned char *>(tmpbuf),&((reinterpret_cast<long long *>(variable_data.get()))[n]),0,nc_size[static_cast<int>(NcType::DOUBLE)]*8,0,num_values);
	    off+=rec_size;
	    fs.seekg(off,std::ios_base::beg);
	  }
	  delete[] tmpbuf;
	  break;
	}
	default:
	{
	  myerror="variable type "+strutils::itos(static_cast<int>(vars[var_index].nc_type))+" not recognized";
	  exit(1);
	}
    }
  }
  else {
    variable_data.resize(num_values,vars[var_index].nc_type);
    switch (vars[var_index].nc_type) {
	case NcType::CHAR:
	{
	  fs.read(reinterpret_cast<char *>(variable_data.get()),num_values);
	  break;
	}
	case NcType::SHORT:
	{
	  auto *tmpbuf=new char[nc_size[static_cast<int>(NcType::SHORT)]*num_values];
	  fs.read(tmpbuf,nc_size[static_cast<int>(NcType::SHORT)]*num_values);
	  bits::get(reinterpret_cast<unsigned char *>(tmpbuf),reinterpret_cast<short *>(variable_data.get()),0,nc_size[static_cast<int>(NcType::SHORT)]*8,0,num_values);
	  delete[] tmpbuf;
	  break;
	}
	case NcType::INT:
	{
	  auto *tmpbuf=new char[nc_size[static_cast<int>(NcType::INT)]*num_values];
	  fs.read(tmpbuf,nc_size[static_cast<int>(NcType::INT)]*num_values);
	  bits::get(reinterpret_cast<unsigned char *>(tmpbuf),reinterpret_cast<int *>(variable_data.get()),0,nc_size[static_cast<int>(NcType::INT)]*8,0,num_values);
	  delete[] tmpbuf;
	  break;
	}
	case NcType::FLOAT:
	{
	  auto *tmpbuf=new char[nc_size[static_cast<int>(NcType::FLOAT)]*num_values];
	  fs.read(tmpbuf,nc_size[static_cast<int>(NcType::FLOAT)]*num_values);
	  bits::get(reinterpret_cast<unsigned char *>(tmpbuf),reinterpret_cast<int *>(variable_data.get()),0,nc_size[static_cast<int>(NcType::FLOAT)]*8,0,num_values);
	  delete[] tmpbuf;
	  break;
	}
	case NcType::DOUBLE:
	{
	  auto *tmpbuf=new char[nc_size[static_cast<int>(NcType::DOUBLE)]*num_values];
	  fs.read(tmpbuf,nc_size[static_cast<int>(NcType::DOUBLE)]*num_values);
	  bits::get(reinterpret_cast<unsigned char *>(tmpbuf),reinterpret_cast<long long *>(variable_data.get()),0,nc_size[static_cast<int>(NcType::DOUBLE)]*8,0,num_values);
	  delete[] tmpbuf;
	  break;
	}
	default:
	{
	  myerror="variable type "+strutils::itos(static_cast<int>(vars[var_index].nc_type))+" not recognized";
	  exit(1);
	}
    }
  }
  return vars[var_index].nc_type;
}

void InputNetCDFStream::print_variable_data(std::string variable_name,std::string string_of_indexes)
{
  std::cout.setf(std::ios::fixed);
  auto var_index=find_variable(variable_name);
  if (var_index < 0) {
    return;
  }
  size_t *product=nullptr;
  std::deque<std::string> indexes;
  if (string_of_indexes.length() > 0) {
    indexes=strutils::split(string_of_indexes,",");
    if (indexes.size() != vars[var_index].dimids.size()) {
	myerror="number of subscripts given ("+strutils::itos(indexes.size())+") does not match number of variable subscripts ("+strutils::itos(vars[var_index].dimids.size())+")";
	exit(1);
    }
    product=new size_t[indexes.size()];
    product[indexes.size()-1]=dims[vars[var_index].dimids[indexes.size()-1]].length;
    for (int n=indexes.size()-2; n > 0; --n) {
	product[n]=dims[vars[var_index].dimids[n]].length*product[n+1];
    }
  }
  fs.seekg(vars[var_index].offset,std::ios_base::beg);
  std::cout << "Variable: " << vars[var_index].name << std::endl;
  std::cout << "Type: " << netCDFStream::nc_type[static_cast<int>(vars[var_index].nc_type)] << std::endl;
  if (vars[var_index].attrs.size() > 0) {
    std::cout << "Attributes: " << vars[var_index].attrs.size() << std::endl;
    for (size_t n=0; n < vars[var_index].attrs.size(); ++n) {
	print_attribute(vars[var_index].attrs[n],2);
    }
  }
  size_t num_vals=1;
  for (size_t n=0; n < vars[var_index].dimids.size(); ++n) {
    if (!dims[vars[var_index].dimids[n]].is_rec) {
	num_vals*=dims[vars[var_index].dimids[n]].length;
    }
  }
  if (vars[var_index].is_rec) {
    auto num_rec_vals=num_vals*num_recs;
    std::cout << "Number of values: " << num_rec_vals << std::endl;
    auto off=vars[var_index].offset;
    if (string_of_indexes.length() == 0) {
	for (size_t n=0; n < num_rec_vals; n+=num_vals) {
	  for (size_t m=0; m < num_vals; ++m) {
	    if (print_indexes(vars[var_index],n+m)) {
		switch(vars[var_index].nc_type) {
		  case NcType::BYTE:
		  {
		    auto data=new unsigned char;
		    fs.read(reinterpret_cast<char *>(data),1);
		    std::cout << "  " << static_cast<int>(*(reinterpret_cast<unsigned char *>(data))) << std::endl;
		    delete data;
		    break;
		  }
		  case NcType::CHAR:
		  {
		    auto data=new char[num_vals];
		    auto nv=dims[vars[var_index].dimids.back()].length;
		    fs.read(reinterpret_cast<char *>(data),nv);
		    std::cout << "  \"";
		    std::cout.write(reinterpret_cast<char *>(data),nv);
		    std::cout << "\"" << std::endl;
		    m+=nv;
		    if ( (m+nv) > num_vals) {
			m=num_vals;
		    }
		    else {
			m--;
		    }
		    delete data;
		    break;
		  }
		  case NcType::SHORT:
		  {
		    auto data=new short;
		    auto tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::SHORT)]];
		    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::SHORT)]);
		    bits::get(tmpbuf,*(reinterpret_cast<short *>(data)),0,nc_size[static_cast<int>(NcType::SHORT)]*8);
		    std::cout << "  " << *(reinterpret_cast<short *>(data)) << std::endl;
		    delete[] tmpbuf;
		    delete data;
		    break;
		  }
		  case NcType::INT:
		  {
		    auto data=new int;
		    auto tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::INT)]];
		    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::INT)]);
		    bits::get(tmpbuf,*(reinterpret_cast<int *>(data)),0,nc_size[static_cast<int>(NcType::INT)]*8);
		    std::cout << "  " << *(reinterpret_cast<int *>(data)) << std::endl;
		    delete[] tmpbuf;
		    delete data;
		    break;
		  }
		  case NcType::FLOAT:
		  {
		    auto data=new float;
		    auto tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::FLOAT)]];
		    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::FLOAT)]);
		    bits::get(tmpbuf,*(reinterpret_cast<int *>(data)),0,nc_size[static_cast<int>(NcType::FLOAT)]*8);
		    std::cout << "  " << *(reinterpret_cast<float *>(data)) << std::endl;
		    delete[] tmpbuf;
		    delete data;
		    break;
		  }
		  case NcType::DOUBLE:
		  {
		    auto data=new double;
		    auto tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::DOUBLE)]];
		    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::DOUBLE)]);
		    bits::get(tmpbuf,*(reinterpret_cast<long long *>(data)),0,nc_size[static_cast<int>(NcType::DOUBLE)]*8);
		    std::cout << "  " << *(reinterpret_cast<double *>(data)) << std::endl;
		    delete[] tmpbuf;
		    delete data;
		    break;
		  }
		  default:
		  {
		    myerror="variable type "+strutils::itos(static_cast<int>(vars[var_index].nc_type))+" not recognized";
		    exit(1);
		  }
		}
	    }
	  }
	  off+=rec_size;
	  fs.seekg(off,std::ios_base::beg);
	}
    }
    else {
	auto data_index=0;
	if (indexes.size() > 1) {
	  size_t n=1;
	  for (n=1; n < indexes.size()-1; ++n) {
	    size_t m=std::stoi(indexes[n]);
	    if (m >= dims[vars[var_index].dimids[n]].length) {
		myerror="subscript ("+strutils::itos(n)+") out of range";
		exit(1);
	    }
	    data_index+=(m*product[n+1]);
	  }
	  size_t m=std::stoi(indexes[n]);
	  if (m >= dims[vars[var_index].dimids[n]].length) {
	    myerror="subscript ("+strutils::itos(n)+") out of range";
	    exit(1);
	  }
	  data_index+=m;
	}
	size_t m=std::stoi(indexes[0]);
	if (m >= num_recs) {
	  myerror="subscript (0) out of range";
	  exit(1);
	}
	fs.seekg(data_index*nc_size[static_cast<int>(vars[var_index].nc_type)]+m*rec_size,std::ios_base::cur);
	switch(vars[var_index].nc_type) {
	  case NcType::BYTE:
	  {
	    auto data=new unsigned char;
	    fs.read(reinterpret_cast<char *>(data),1);
	    std::cout << "(" << string_of_indexes << ")  " << static_cast<int>(*(reinterpret_cast<unsigned char *>(data))) << std::endl;
	    delete data;
	    break;
	  }
	  case NcType::CHAR:
	  {
	    auto data=new char;
	    fs.read(reinterpret_cast<char *>(data),1);
	    std::cout << "(" << string_of_indexes << ")  " << *(reinterpret_cast<char *>(data)) << std::endl;
	    delete data;
	    break;
	  }
	  case NcType::SHORT:
	  {
	    auto data=new short;
	    auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::SHORT)]];
	    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::SHORT)]);
	    bits::get(tmpbuf,*(reinterpret_cast<short *>(data)),0,nc_size[static_cast<int>(NcType::SHORT)]*8);
	    std::cout << "(" << string_of_indexes << ")  " << *(reinterpret_cast<short *>(data)) << std::endl;
	    delete[] tmpbuf;
	    delete data;
	    break;
	  }
	  case NcType::INT:
	  {
	    auto data=new int;
	    auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::INT)]];
	    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::INT)]);
	    bits::get(tmpbuf,*(reinterpret_cast<int *>(data)),0,nc_size[static_cast<int>(NcType::INT)]*8);
	    std::cout << "(" << string_of_indexes << ")  " << *(reinterpret_cast<int *>(data)) << std::endl;
	    delete[] tmpbuf;
	    delete data;
	    break;
	  }
	  case NcType::FLOAT:
	  {
	    auto data=new float;
	    auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::FLOAT)]];
	    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::FLOAT)]);
	    bits::get(tmpbuf,*(reinterpret_cast<int *>(data)),0,nc_size[static_cast<int>(NcType::FLOAT)]*8);
	    std::cout << "(" << string_of_indexes << ")  " << *(reinterpret_cast<float *>(data)) << std::endl;
	    delete[] tmpbuf;
	    delete data;
	    break;
	  }
	  case NcType::DOUBLE:
	  {
	    auto data=new double;
	    auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::DOUBLE)]];
	    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::DOUBLE)]);
	    bits::get(tmpbuf,*(reinterpret_cast<long long *>(data)),0,nc_size[static_cast<int>(NcType::DOUBLE)]*8);
	    std::cout << "(" << string_of_indexes << ")  " << *(reinterpret_cast<double *>(data)) << std::endl;
	    delete[] tmpbuf;
	    delete data;
	    break;
	  }
	  default:
	  {
	    myerror="variable type "+strutils::itos(static_cast<int>(vars[var_index].nc_type))+" not recognized";
	    exit(1);
	  }
	}
    }
  }
  else {
    size_t len;
    if (vars[var_index].nc_type == NcType::CHAR) {
	len=dims[vars[var_index].dimids.back()].length;
	num_vals/=len;
    }
    else {
	len=1;
    }
    std::cout << "Number of values: " << num_vals << std::endl;
    if (string_of_indexes.length() == 0) {
	for (size_t n=0; n < num_vals; ++n) {
	  if (print_indexes(vars[var_index],n*len)) {
	    switch(vars[var_index].nc_type) {
		case NcType::BYTE:
		{
		  auto data=new unsigned char;
		  fs.read(reinterpret_cast<char *>(data),1);
		  std::cout << "  " << static_cast<int>(*(reinterpret_cast<unsigned char *>(data))) << std::endl;
		  delete data;
		  break;
		}
		case NcType::CHAR:
		{
		  auto data=new char[len];
		  fs.read(reinterpret_cast<char *>(data),len);
		  std::cout << "  \"";
		  std::cout.write(reinterpret_cast<char *>(data),len);
		  std::cout << "\"" << std::endl;
		  delete data;
		  break;
		}
		case NcType::SHORT:
		{
		  auto data=new short;
		  auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::SHORT)]];
		  fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::SHORT)]);
		  bits::get(tmpbuf,*(reinterpret_cast<short *>(data)),0,nc_size[static_cast<int>(NcType::SHORT)]*8);
		  std::cout << "  " << *(reinterpret_cast<short *>(data)) << std::endl;
		  delete[] tmpbuf;
		  delete data;
		  break;
		}
		case NcType::INT:
		{
		  auto data=new int;
		  auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::INT)]];
		  fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::INT)]);
		  bits::get(tmpbuf,*(reinterpret_cast<int *>(data)),0,nc_size[static_cast<int>(NcType::INT)]*8);
		  std::cout << "  " << *(reinterpret_cast<int *>(data)) << std::endl;
		  delete[] tmpbuf;
		  delete data;
		  break;
		}
		case NcType::FLOAT:
		{
		  auto data=new float;
		  auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::FLOAT)]];
		  fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::FLOAT)]);
		  bits::get(tmpbuf,*(reinterpret_cast<int *>(data)),0,nc_size[static_cast<int>(NcType::FLOAT)]*8);
		  std::cout << "  " << *(reinterpret_cast<float *>(data)) << std::endl;
		  delete[] tmpbuf;
		  delete data;
		  break;
		}
		case NcType::DOUBLE:
		{
		  auto data=new double;
		  auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::DOUBLE)]];
		  fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::DOUBLE)]);
		  bits::get(tmpbuf,*(reinterpret_cast<long long *>(data)),0,nc_size[static_cast<int>(NcType::DOUBLE)]*8);
		  std::cout << "  " << *(reinterpret_cast<double *>(data)) << std::endl;
		  delete[] tmpbuf;
		  delete data;
		  break;
		}
		default:
		{
		  myerror="variable type "+strutils::itos(static_cast<int>(vars[var_index].nc_type))+" not recognized";
		  exit(1);
		}
	    }
	  }
	}
    }
    else {
	auto data_index=0;
	size_t n;
	for (n=0; n < indexes.size()-1; ++n) {
	  size_t m=std::stoi(indexes[n]);
	  if (m >= dims[vars[var_index].dimids[n]].length) {
	    myerror="subscript ("+strutils::itos(n)+") out of range";
	    exit(1);
	  }
	  data_index+=(m*product[n+1]);
	}
	size_t m=std::stoi(indexes[n]);
	if (m >= dims[vars[var_index].dimids[n]].length) {
	  myerror="subscript ("+strutils::itos(n)+") out of range";
	  exit(1);
	}
	data_index+=m;
	fs.seekg(data_index*nc_size[static_cast<int>(vars[var_index].nc_type)],std::ios_base::cur);
	switch(vars[var_index].nc_type) {
	  case NcType::BYTE:
	  {
	    auto data=new unsigned char;
	    fs.read(reinterpret_cast<char *>(data),1);
	    std::cout << "(" << string_of_indexes << ")  " << static_cast<int>(*(reinterpret_cast<unsigned char *>(data))) << std::endl;
	    delete data;
	    break;
	  }
	  case NcType::CHAR:
	  {
	    auto data=new char;
	    fs.read(reinterpret_cast<char *>(data),1);
	    std::cout << "(" << string_of_indexes << ")  " << *(reinterpret_cast<char *>(data)) << std::endl;
	    delete data;
	    break;
	  }
	  case NcType::SHORT:
	  {
	    auto data=new short;
	    auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::SHORT)]];
	    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::SHORT)]);
	    bits::get(tmpbuf,*(reinterpret_cast<short *>(data)),0,nc_size[static_cast<int>(NcType::SHORT)]*8);
	    std::cout << "(" << string_of_indexes << ")  " << *(reinterpret_cast<short *>(data)) << std::endl;
	    delete[] tmpbuf;
	    delete data;
	    break;
	  }
	  case NcType::INT:
	  {
	    auto data=new int;
	    auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::INT)]];
	    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::INT)]);
	    bits::get(tmpbuf,*(reinterpret_cast<int *>(data)),0,nc_size[static_cast<int>(NcType::INT)]*8);
	    std::cout << "(" << string_of_indexes << ")  " << *(reinterpret_cast<int *>(data)) << std::endl;
	    delete[] tmpbuf;
	    delete data;
	    break;
	  }
	  case NcType::FLOAT:
	  {
	    auto data=new float;
	    auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::FLOAT)]];
	    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::FLOAT)]);
	    bits::get(tmpbuf,*(reinterpret_cast<int *>(data)),0,nc_size[static_cast<int>(NcType::FLOAT)]*8);
	    std::cout << "(" << string_of_indexes << ")  " << *(reinterpret_cast<float *>(data)) << std::endl;
	    delete[] tmpbuf;
	    delete data;
	    break;
	  }
	  case NcType::DOUBLE:
	  {
	    auto data=new double;
	    auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::DOUBLE)]];
	    fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::DOUBLE)]);
	    bits::get(tmpbuf,*(reinterpret_cast<long long *>(data)),0,nc_size[static_cast<int>(NcType::DOUBLE)]*8);
	    std::cout << "(" << string_of_indexes << ")  " << *(reinterpret_cast<double *>(data)) << std::endl;
	    delete[] tmpbuf;
	    delete data;
	    break;
	  }
	  default:
	  {
	    myerror="variable type "+strutils::itos(static_cast<int>(vars[var_index].nc_type))+" not recognized";
	    exit(1);
	  }
  	}
    }
  }
  std::cout << std::endl;
}

void InputNetCDFStream::print_variables() const
{
  std::cout << "Variables (" << vars.size() << "):" << std::endl;
  for (size_t n=0; n < vars.size(); ++n) {
    std::cout << "  " << nc_type[static_cast<int>(vars[n].nc_type)] << " " << vars[n].name;
    if (vars[n].dimids.size() > 0) {
	std::cout << "(";
	for (size_t m=0; m < vars[n].dimids.size(); ++m) {
	  if (m > 0) {
	    std::cout << "," << dims[vars[n].dimids[m]].name;
	  }
	  else {
	    std::cout << dims[vars[n].dimids[m]].name;
	  }
	}
	std::cout << ")";
    }
    if (vars[n].is_coord) {
	std::cout << " (coordinate variable)";
    }
    std::cout << std::endl;
    for (size_t m=0; m < vars[n].attrs.size(); ++m) {
	print_attribute(vars[n].attrs[m],4);
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

void InputNetCDFStream::fill_dimensions()
{
  auto *tmpbuf=new unsigned char[8];
  fs.read(reinterpret_cast<char *>(tmpbuf),8);
  int check;
  bits::get(tmpbuf,check,0,32);
  size_t num_dims;
  bits::get(tmpbuf,num_dims,32,32);
  if ((check == 0 && num_dims != 0) || (check != 0 && check != static_cast<int>(Type::DIMENSION))) {
    myerror="Error while filling dimensions";
    exit(1);
  }
  dims.resize(num_dims);
  for (size_t n=0; n < num_dims; ++n) {
    fill_string(dims[n].name);
    fs.read(reinterpret_cast<char *>(tmpbuf),4);
    bits::get(tmpbuf,dims[n].length,0,32);
    if (dims[n].length == 0) {
	dims[n].is_rec=true;
    }
    else {
	dims[n].is_rec=false;
    }
  }
  delete[] tmpbuf;
}

void InputNetCDFStream::fill_attributes(std::vector<Attribute>& attributes)
{
  auto *tmpbuf=new unsigned char[8];
  fs.read(reinterpret_cast<char *>(tmpbuf),8);
  int check;
  bits::get(tmpbuf,check,0,32);
  size_t num_attr;
  bits::get(tmpbuf,num_attr,32,32);
  if ((check == 0 && num_attr != 0) || (check != 0 && check != static_cast<int>(Type::ATTRIBUTE))) {
    myerror="Error while filling attributes - check: "+strutils::itos(check)+"  num_attr: "+strutils::itos(num_attr);
    exit(1);
  }
  attributes.resize(num_attr);
  for (size_t n=0; n < num_attr; ++n) {
    fill_string(attributes[n].name);
    fs.read(reinterpret_cast<char *>(tmpbuf),4);
    int i;
    bits::get(tmpbuf,i,0,32);
    attributes[n].nc_type=static_cast<NcType>(i);
    switch (attributes[n].nc_type) {
	case NcType::BYTE:
	{
	  fs.read(reinterpret_cast<char *>(tmpbuf),4);
	  bits::get(tmpbuf,attributes[n].num_values,0,32);
	  attributes[n].values=new unsigned char[attributes[n].num_values];
	  fs.read(reinterpret_cast<char *>(attributes[n].values),1);
	  if ( (attributes[n].num_values % 4) != 0) {
	    fs.seekg(4-(attributes[n].num_values % 4),std::ios_base::cur);
	  }
	  break;
	}
	case NcType::CHAR:
	{
	  attributes[n].num_values=1;
	  attributes[n].values=new std::string;
	  fill_string(*(reinterpret_cast<std::string *>(attributes[n].values)));
	  break;
	}
	case NcType::SHORT:
	{
	  fs.read(reinterpret_cast<char *>(tmpbuf),4);
	  bits::get(tmpbuf,attributes[n].num_values,0,32);
	  attributes[n].values=new short[attributes[n].num_values];
	  auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::SHORT)]*attributes[n].num_values];
	  fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::SHORT)]*attributes[n].num_values);
	  bits::get(tmpbuf,reinterpret_cast<short *>(attributes[n].values),0,nc_size[static_cast<int>(NcType::SHORT)]*8,0,attributes[n].num_values);
	  if ( ((attributes[n].num_values*2) % 4) != 0) {
	    fs.seekg(4-((attributes[n].num_values*2) % 4),std::ios_base::cur);
	  }
	  delete[] tmpbuf;
	  break;
	}
	case NcType::INT:
	{
	  fs.read(reinterpret_cast<char *>(tmpbuf),4);
	  bits::get(tmpbuf,attributes[n].num_values,0,32);
	  attributes[n].values=new int[attributes[n].num_values];
	  auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::INT)]*attributes[n].num_values];
	  fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::INT)]*attributes[n].num_values);
	  bits::get(tmpbuf,reinterpret_cast<int *>(attributes[n].values),0,nc_size[static_cast<int>(NcType::INT)]*8,0,attributes[n].num_values);
	  delete[] tmpbuf;
	  break;
	}
	case NcType::FLOAT:
	{
	  fs.read(reinterpret_cast<char *>(tmpbuf),4);
	  bits::get(tmpbuf,attributes[n].num_values,0,32);
	  attributes[n].values=new float[attributes[n].num_values];
	  auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::FLOAT)]*attributes[n].num_values];
	  fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::FLOAT)]*attributes[n].num_values);
	  bits::get(tmpbuf,reinterpret_cast<int *>(attributes[n].values),0,nc_size[static_cast<int>(NcType::FLOAT)]*8,0,attributes[n].num_values);
	  delete[] tmpbuf;
	  break;
	}
	case NcType::DOUBLE:
	{
	  fs.read(reinterpret_cast<char *>(tmpbuf),4);
	  bits::get(tmpbuf,attributes[n].num_values,0,32);
	  attributes[n].values=new double[attributes[n].num_values];
	  auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::DOUBLE)]*attributes[n].num_values];
	  fs.read(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::DOUBLE)]*attributes[n].num_values);
	  bits::get(tmpbuf,reinterpret_cast<long long *>(attributes[n].values),0,nc_size[static_cast<int>(NcType::DOUBLE)]*8,0,attributes[n].num_values);
	  delete[] tmpbuf;
	  break;
	}
	default:
	{
	  myerror="fill_attributes error: attribute type "+strutils::itos(static_cast<int>(attributes[n].nc_type))+" not recognized";
	  exit(1);
	}
    }
  }
  delete[] tmpbuf;
}

void InputNetCDFStream::fill_variables()
{
  auto *tmpbuf=new unsigned char[8];
  fs.read(reinterpret_cast<char *>(tmpbuf),8);
  int check;
  bits::get(tmpbuf,check,0,32);
  size_t num_vars;
  bits::get(tmpbuf,num_vars,32,32);
  if ((check == 0 && num_vars != 0) || (check != 0 && check != static_cast<int>(Type::VARIABLE))) {
    myerror="Error while filling variables";
    exit(1);
  }
  vars.resize(num_vars);
  for (size_t n=0; n < num_vars; ++n) {
    fill_string(vars[n].name);
    fs.read(reinterpret_cast<char *>(tmpbuf),4);
    size_t num_dims;
    bits::get(tmpbuf,num_dims,0,32);
    vars[n].dimids.resize(num_dims);
    for (size_t m=0; m < num_dims; ++m) {
	fs.read(reinterpret_cast<char *>(tmpbuf),4);
	size_t dimid;
	bits::get(tmpbuf,dimid,0,32);
	vars[n].dimids[m]=dimid;
    }
    if (num_dims == 1 && vars[n].name == dims[vars[n].dimids[0]].name) {
	vars[n].is_coord=true;
    }
    else {
	vars[n].is_coord=false;
    }
    fill_attributes(vars[n].attrs);
    fs.read(reinterpret_cast<char *>(tmpbuf),8);
    int i;
    bits::get(tmpbuf,i,0,32);
    vars[n].nc_type=static_cast<NcType>(i);
    bits::get(tmpbuf,vars[n].size,32,32);
    if (vars[n].dimids.size() > 0 && dims[vars[n].dimids[0]].is_rec) {
	vars[n].is_rec=true;
	rec_size+=vars[n].size;
    }
    else {
	vars[n].is_rec=false;
    }
    if (_version == 1) {
	fs.read(reinterpret_cast<char *>(tmpbuf),4);
	bits::get(tmpbuf,vars[n].offset,0,32);
    }
    else if (_version == 2) {
	fs.read(reinterpret_cast<char *>(tmpbuf),8);
	bits::get(tmpbuf,vars[n].offset,0,64);
    }
    else {
	myerror="unrecognized version number "+strutils::itos(_version);
	exit(1);
    }
  }
  delete[] tmpbuf;
  while ( (rec_size % 4) != 0) {
    ++rec_size;
  }
}

void InputNetCDFStream::fill_string(std::string& string_to_fill)
{
  auto *tmpbuf=new unsigned char[4];
  fs.read(reinterpret_cast<char *>(tmpbuf),4);
  size_t str_len;
  bits::get(tmpbuf,str_len,0,32);
  delete[] tmpbuf;
  auto *s=new char[str_len+1];
  fs.read(s,str_len);
  s[str_len]='\0';
  string_to_fill=s;
  delete[] s;
  if ( (str_len % 4) != 0) {
    fs.seekg(4-(str_len % 4),std::ios_base::cur);
  }
}

void InputNetCDFStream::print_attribute(const Attribute& attribute,size_t left_margin_spacing) const
{
  for (size_t n=0; n < left_margin_spacing; ++n) {
    std::cout << " ";
  }
  std::cout << attribute.name << ": ";
  switch (attribute.nc_type) {
    case NcType::BYTE:
    {
	for (size_t n=0; n < attribute.num_values; ++n) {
	  if (n > 0) {
	    std::cout << ", " << static_cast<int>((reinterpret_cast<unsigned char *>(attribute.values))[n]);
	  }
	  else {
	    std::cout << static_cast<int>((reinterpret_cast<unsigned char *>(attribute.values))[n]);
	  }
	}
	std::cout << " (byte)" << std::endl;
	break;
    }
    case NcType::CHAR:
    {
	std::cout << *(reinterpret_cast<std::string *>(attribute.values)) << " (char)" << std::endl;
	break;
    }
    case NcType::SHORT:
    {
	for (size_t n=0; n < attribute.num_values; ++n) {
	  if (n > 0) {
	    std::cout << ", " << (reinterpret_cast<short *>(attribute.values))[n];
	  }
	  else {
	    std::cout << (reinterpret_cast<short *>(attribute.values))[n];
	  }
	}
	std::cout << " (short)" << std::endl;
	break;
    }
    case NcType::INT:
    {
	for (size_t n=0; n < attribute.num_values; ++n) {
	  if (n > 0) {
	    std::cout << ", " << (reinterpret_cast<int *>(attribute.values))[n];
	  }
	  else {
	    std::cout << (reinterpret_cast<int *>(attribute.values))[n];
	  }
	}
	std::cout << " (int)" << std::endl;
      break;
    }
    case NcType::FLOAT:
    {
	for (size_t n=0; n < attribute.num_values; ++n) {
	  if (n > 0) {
	    std::cout << ", " << (reinterpret_cast<float *>(attribute.values))[n];
	  }
	  else {
	    std::cout << (reinterpret_cast<float *>(attribute.values))[n];
	  }
	}
	std::cout << " (float)" << std::endl;
	break;
    }
    case NcType::DOUBLE:
    {
	for (size_t n=0; n < attribute.num_values; ++n) {
	  if (n > 0) {
	    std::cout << ", " << (reinterpret_cast<double *>(attribute.values))[n];
	  }
	  else {
	    std::cout << (reinterpret_cast<double *>(attribute.values))[n];
	  }
	}
	std::cout << " (double)" << std::endl;
	break;
    }
    default:
    {
	myerror="print_attribute error: attribute type "+strutils::itos(static_cast<int>(attribute.nc_type))+" not recognized";
	exit(1);
    }
  }
}

bool InputNetCDFStream::print_indexes(const Variable& variable,size_t elem_num) const
{
  std::string indexes;
  if (variable.dimids.size() == 0) {
    indexes="(0)";
  }
  else {
    auto num_in_div=new size_t[variable.dimids.size()];
    for (size_t n=0; n < variable.dimids.size(); ++n) {
	num_in_div[n]=1;
	size_t len=1;
	for (size_t m=n; m < variable.dimids.size(); ++m) {
	  if (dims[variable.dimids[m]].is_rec) {
/*
	    if (variable.dimids.size() == 1 && (variable.nc_type == NcType::BYTE || variable.nc_type == NcType::CHAR)) {
		num_in_div[n]*=(num_recs*4);
	    }
	    else {
*/
		num_in_div[n]*=num_recs;
//	    }
	  }
	  else {
	    len*=dims[variable.dimids[m]].length;
/*
	    if (m == (variable.dimids.size()-1) && dims[variable.dimids[0]].is_rec && (variable.nc_type == NcType::BYTE || variable.nc_type == NcType::CHAR)) {
		size_t len_extra;
		if ( (len_extra=(len % 4)) != 0) {
		  len+=4-len_extra;
		}
	    }
*/
	  }
	}
	num_in_div[n]*=len;
	len=1;
	if (dims[variable.dimids[n]].is_rec) {
	  num_in_div[n]/=num_recs;
	}
	else {
	  len*=dims[variable.dimids[n]].length;
/*
	  if (n == (variable.dimids.size()-1) && dims[variable.dimids[0]].is_rec && (variable.nc_type == NcType::BYTE || variable.nc_type == NcType::CHAR)) {
	    size_t len_extra;
	    if ( (len_extra=(len % 4)) != 0) {
		len+=4-len_extra;
	    }
	  }
*/
	}
	num_in_div[n]/=len;
    }
    for (size_t n=0; n < variable.dimids.size(); ++n) {
	if (n > 0) {
	  indexes+=",";
	}
	auto index=elem_num/num_in_div[n];
	if (dims[variable.dimids[n]].is_rec || index < dims[variable.dimids[n]].length) {
	  indexes+=strutils::itos(index);
	}
	else {
	  return false;
	}
	elem_num-=(index*num_in_div[n]);
    }
    delete[] num_in_div;
  }
  if (indexes.length() > 0) {
    std::cout << "(" << indexes << ")";
    return true;
  }
  else {
    return false;
  }
}

size_t InputNetCDFStream::variable_dimensions(std::string variable_name,size_t **address_of_dimension_array) const
{
  auto var_index=find_variable(variable_name);
  if (var_index < 0) {
    return 0;
  }
  *address_of_dimension_array=new size_t[vars[var_index].dimids.size()];
  for (size_t n=0; n < vars[var_index].dimids.size(); ++n) {
    if (dims[vars[var_index].dimids[n]].is_rec) {
	(*address_of_dimension_array)[n]=num_recs;
    }
    else {
	(*address_of_dimension_array)[n]=dims[vars[var_index].dimids[n]].length;
    }
  }
  return vars[var_index].dimids.size();
}

void InputNetCDFStream::variable_value(std::string variable_name,std::string indexes,void **value)
{
}

OutputNetCDFStream::~OutputNetCDFStream()
{
  if (fs.is_open()) {
    close();
  }
}


bool OutputNetCDFStream::open(std::string filename)
{
  if (fs.is_open()) {
    std::cerr << "Error: there is already an open OutputNetCDFStream" << std::endl;
    exit(1);
  }
  file_name=filename;
  fs.open(file_name.c_str(),std::ios_base::out);
  if (!fs.is_open()) {
    myerror="unable to open file";
    return false;
  }
  curr_offset=0;
  rec_size=0;
  num_recs=0;
  started_record_vars=false;
  non_rec_housekeeping.index=-1;
  non_rec_housekeeping.num_values=0;
  non_rec_housekeeping.num_values_written=0;
  return true;
}

bool OutputNetCDFStream::close()
{
  if (!fs.is_open()) {
    myerror="no open output stream";
    return false;
  }
  if (non_rec_housekeeping.index >= 0) {
    myerror="close error: non-record variable "+vars[non_rec_housekeeping.index].name+" not completed";
    exit(1);
  }
  dims.clear();
  gattrs.clear();
  vars.clear();
  size_t num_fill=0;
  if (num_recs > 0) {
    if ( (num_recs % rec_size) != 0) {
	myerror="partial record in output file - "+strutils::itos(num_recs)+"/"+strutils::itos(rec_size);
	fs.close();
	fs.clear();
	return false;
    }
    num_fill=4-(num_recs % 4);
    num_recs/=rec_size;
  }
  char null_byte=0;
  for (size_t n=0; n < num_fill; ++n) {
    fs.write(&null_byte,1);
  }
  fs.seekg(4,std::ios_base::beg);
  unsigned char tmpbuf[4];
  bits::set(tmpbuf,num_recs,0,32);
  fs.write(reinterpret_cast<char *>(tmpbuf),4);
  fs.close();
  fs.clear();
  return true;
}

void OutputNetCDFStream::add_dimension(std::string name,size_t length)
{
  size_t n=dims.size();
  if (length == 0) {
    for (size_t m=0; m < n; ++m) {
	if (dims[m].is_rec) {
	  myerror="a record dimension has already been specified - "+std::string(name)+" cannot be a record dimension";
	  exit(1);
	}
    }
  }
  dims.resize(n+1);
  dims[n].name=name;
  dims[n].length=length;
  if (dims[n].length == 0) {
    dims[n].is_rec=true;
  }
  else {
    dims[n].is_rec=false;
  }
}

void OutputNetCDFStream::add_record_data(VariableData& variable_data)
{
  add_record_data(variable_data,variable_data.size());
}

void OutputNetCDFStream::add_record_data(VariableData& variable_data,size_t num_values)
{
  if (!started_record_vars) {
    off_t offset=curr_offset;
    for (size_t n=0; n < vars.size(); ++n) {
	if (vars[n].is_rec) {
	  fs.seekg(vars[n].offset,std::ios_base::beg);
	  auto *tmpbuf=new unsigned char[4];
	  bits::set(tmpbuf,offset,0,32);
	  fs.write(reinterpret_cast<char *>(tmpbuf),4);
	  delete[] tmpbuf;
	  offset+=vars[n].size;
	}
    }
    started_record_vars=true;
    fs.seekg(curr_offset,std::ios_base::beg);
  }
  size_t bytes_written=0;
  switch(variable_data.type()) {
    case NcType::CHAR:
    {
	fs.write(reinterpret_cast<char *>(variable_data.get()),num_values);
	bytes_written=nc_size[static_cast<int>(variable_data.type())]*num_values;
	auto fill=num_values;
	char fill_char=0x0;
	while ( (fill % 4) != 0) {
	  fs.write(&fill_char,1);
	  ++fill;
	  ++bytes_written;
	}
	break;
    }
    case NcType::BYTE:
    {
	fs.write(reinterpret_cast<char *>(variable_data.get()),num_values);
	bytes_written=nc_size[static_cast<int>(variable_data.type())]*num_values;
	auto fill=num_values;
	char fill_byte=0x81;
	while ( (fill % 4) != 0) {
	  fs.write(&fill_byte,1);
	  ++fill;
	  ++bytes_written;
	}
	break;
    }
    case NcType::SHORT:
    {
	auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(variable_data.type())]*num_values];
	bits::set(tmpbuf,reinterpret_cast<short *>(variable_data.get()),0,nc_size[static_cast<int>(NcType::SHORT)]*8,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(variable_data.type())]*num_values);
	bytes_written=nc_size[static_cast<int>(variable_data.type())]*num_values;
	auto fill=num_values*2;
	bits::set(tmpbuf,0x8001,0,16);
	while ( (fill % 4) != 0) {
	  fs.write(reinterpret_cast<char *>(tmpbuf),2);
	  fill+=2;
	  bytes_written+=2;
	}
	delete[] tmpbuf;
	break;
    }
    case NcType::INT:
    {
	auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(variable_data.type())]*num_values];
	bits::set(tmpbuf,reinterpret_cast<int *>(variable_data.get()),0,nc_size[static_cast<int>(NcType::INT)]*8,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(variable_data.type())]*num_values);
	bytes_written=nc_size[static_cast<int>(variable_data.type())]*num_values;
	delete[] tmpbuf;
	break;
    }
    case NcType::FLOAT:
    {
	auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(variable_data.type())]*num_values];
	bits::set(tmpbuf,reinterpret_cast<int *>(variable_data.get()),0,nc_size[static_cast<int>(NcType::FLOAT)]*8,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(variable_data.type())]*num_values);
	bytes_written=nc_size[static_cast<int>(variable_data.type())]*num_values;
	delete[] tmpbuf;
	break;
    }
    case NcType::DOUBLE:
    {
	auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(variable_data.type())]*num_values];
	bits::set(tmpbuf,reinterpret_cast<long long *>(variable_data.get()),0,nc_size[static_cast<int>(NcType::DOUBLE)]*8,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(variable_data.type())]*num_values);
	bytes_written=nc_size[static_cast<int>(variable_data.type())]*num_values;
	delete[] tmpbuf;
	break;
    }
    default: {}
  }
  curr_offset+=bytes_written;
  num_recs+=bytes_written;
  size_t fill;
  if ( (num_recs % rec_size) == 0 && (fill=(rec_size % 4)) != 0) {
    char fill_char=0x0;
    while (fill < 4) {
	fs.write(&fill_char,1);
	++fill;
    }
  }
}

void OutputNetCDFStream::add_variable(std::string name,NcType nc_type,size_t num_ids,size_t *dimension_ids)
{
  size_t n=vars.size();
  vars.resize(n+1);
  vars[n].name=name;
  vars[n].nc_type=nc_type;
  vars[n].dimids.resize(num_ids);
  size_t size=1;
  for (size_t m=0; m < num_ids; ++m) {
    vars[n].dimids[m]=dimension_ids[m];
    if (dims[dimension_ids[m]].length > 0) {
	size*=dims[dimension_ids[m]].length;
    }
  }
  size*=nc_size[static_cast<int>(nc_type)];
  if (nc_type == NcType::BYTE || nc_type == NcType::CHAR || nc_type == NcType::SHORT) {
    while ( (size % 4) != 0) {
	++size;
    }
  }
  vars[n].size=size;
  if (dimension_ids != NULL && dims[dimension_ids[0]].length == 0) {
    vars[n].is_rec=true;
    rec_size+=size;
  }
  else {
    vars[n].is_rec=false;
  }
}

void OutputNetCDFStream::add_variable(std::string name,NcType nc_type,const std::vector<size_t>& dimension_ids)
{
  size_t n=vars.size();
  vars.resize(n+1);
  vars[n].name=name;
  vars[n].nc_type=nc_type;
  vars[n].dimids.resize(dimension_ids.size());
  size_t size=1;
  for (size_t m=0; m < dimension_ids.size(); ++m) {
    vars[n].dimids[m]=dimension_ids[m];
    if (dims[dimension_ids[m]].length > 0) {
	size*=dims[dimension_ids[m]].length;
    }
  }
  size*=nc_size[static_cast<int>(nc_type)];
  if (nc_type == NcType::BYTE || nc_type == NcType::CHAR || nc_type == NcType::SHORT) {
  while ( (size % 4) != 0) {
    size++;
  }
}
  vars[n].size=size;
  if (dimension_ids.size() > 0 && dims[dimension_ids[0]].length == 0) {
    vars[n].is_rec=true;
    rec_size+=size;
  }
  else {
    vars[n].is_rec=false;
  }
}

void OutputNetCDFStream::add_variable_attribute(std::string variable_name,std::string attribute_name,std::string value)
{
  int n;
  if ( (n=find_variable(variable_name)) >= 0) {
    add_attribute(vars[n].attrs,attribute_name,NcType::CHAR,1,&value);
  }
}

void OutputNetCDFStream::add_variable_attribute(std::string variable_name,std::string attribute_name,short value)
{
  int n;
  if ( (n=find_variable(variable_name)) >= 0) {
    add_attribute(vars[n].attrs,attribute_name,NcType::SHORT,1,&value);
  }
}

void OutputNetCDFStream::add_variable_attribute(std::string variable_name,std::string attribute_name,int value)
{
  int n;
  if ( (n=find_variable(variable_name)) >= 0) {
    add_attribute(vars[n].attrs,attribute_name,NcType::INT,1,&value);
  }
}

void OutputNetCDFStream::add_variable_attribute(std::string variable_name,std::string attribute_name,float value)
{
  int n;
  if ( (n=find_variable(variable_name)) >= 0) {
    add_attribute(vars[n].attrs,attribute_name,NcType::FLOAT,1,&value);
  }
}

void OutputNetCDFStream::add_variable_attribute(std::string variable_name,std::string attribute_name,double value)
{
  int n;
  if ( (n=find_variable(variable_name)) >= 0) {
    add_attribute(vars[n].attrs,attribute_name,NcType::DOUBLE,1,&value);
  }
}

void OutputNetCDFStream::add_variable_attribute(std::string variable_name,std::string attribute_name,NcType nc_type,size_t num_values,void *values)
{
  int n;
  if ( (n=find_variable(variable_name)) >= 0) {
    add_attribute(vars[n].attrs,attribute_name,nc_type,num_values,values);
  }
}

void OutputNetCDFStream::write_header()
{
  char tmpbuf[8]={'C','D','F','\1','\0','\0','\0','\0'};
// magic number
  fs.write(tmpbuf,8);
  curr_offset+=8;
// write dimensions
  int flag=static_cast<int>(Type::DIMENSION);
  bits::set(reinterpret_cast<unsigned char *>(tmpbuf),flag,0,32);
  fs.write(tmpbuf,4);
  curr_offset+=4;
  size_t dim_size=dims.size();
  bits::set(reinterpret_cast<unsigned char *>(tmpbuf),dim_size,0,32);
  fs.write(tmpbuf,4);
  curr_offset+=4;
  for (size_t n=0; n < dims.size(); ++n) {
    put_string(dims[n].name);
    bits::set(reinterpret_cast<unsigned char *>(tmpbuf),dims[n].length,0,32);
    fs.write(tmpbuf,4);
    curr_offset+=4;
  }
// write global attributes
  put_attributes(gattrs);
// write variables
  flag=static_cast<int>(Type::VARIABLE);
  bits::set(reinterpret_cast<unsigned char *>(tmpbuf),flag,0,32);
  fs.write(tmpbuf,4);
  curr_offset+=4;
  auto var_size=vars.size();
  bits::set(reinterpret_cast<unsigned char *>(tmpbuf),var_size,0,32);
  fs.write(tmpbuf,4);
  curr_offset+=4;
  for (size_t n=0; n < vars.size(); ++n) {
    put_string(vars[n].name);
    auto dim_size=vars[n].dimids.size();
    bits::set(reinterpret_cast<unsigned char *>(tmpbuf),dim_size,0,32);
    fs.write(tmpbuf,4);
    curr_offset+=4;
    if (vars[n].dimids.size() > 0) {
	for (size_t m=0; m < vars[n].dimids.size(); ++m) {
	  bits::set(reinterpret_cast<unsigned char *>(tmpbuf),vars[n].dimids[m],0,32);
	  fs.write(tmpbuf,4);
	}
	curr_offset+=(4*vars[n].dimids.size());
    }
    put_attributes(vars[n].attrs);
    auto i=static_cast<int>(vars[n].nc_type);
    bits::set(reinterpret_cast<unsigned char *>(tmpbuf),i,0,32);
    fs.write(tmpbuf,4);
    curr_offset+=4;
    bits::set(reinterpret_cast<unsigned char *>(tmpbuf),vars[n].size,0,32);
    fs.write(tmpbuf,4);
    curr_offset+=4;
// store the current offset in the variable offset so that this location can be
// found and changed later
    vars[n].offset=curr_offset;
    bits::set(reinterpret_cast<unsigned char *>(tmpbuf),vars[n].offset,0,32);
    fs.write(tmpbuf,4);
    curr_offset+=4;
  }
}

void OutputNetCDFStream::write_non_record_data(std::string variable_name,void *data_array)
{
  if (started_record_vars) {
    myerror="cannot write non-record variables after record variables";
    exit(1);
  }
  auto index=find_variable(variable_name);
  if (index < 0)
    return;
  if (vars[index].is_rec) {
    myerror="record variable '"+vars[index].name+"' cannot be written with OutputNetCDFStream::write_non_record_data()";
    exit(1);
  }
  auto num_values=vars[index].size/nc_size[static_cast<int>(vars[index].nc_type)];
  fs.seekg(vars[index].offset,std::ios_base::beg);
  auto *tmpbuf=new unsigned char[4];
  bits::set(tmpbuf,curr_offset,0,32);
  fs.write(reinterpret_cast<char *>(tmpbuf),4);
  delete[] tmpbuf;
  fs.seekg(curr_offset,std::ios_base::beg);
  switch (vars[index].nc_type) {
    case NcType::BYTE:
    case NcType::CHAR:
    {
	fs.write(reinterpret_cast<char *>(data_array),num_values);
	break;
    }
    case NcType::SHORT:
    {
	auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::SHORT)]*num_values];
	bits::set(tmpbuf,reinterpret_cast<short *>(data_array),0,nc_size[static_cast<int>(NcType::SHORT)]*8,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::SHORT)]*num_values);
	delete[] tmpbuf;
	break;
    }
    case NcType::INT:
    {
	auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::INT)]*num_values];
	bits::set(tmpbuf,reinterpret_cast<int *>(data_array),0,nc_size[static_cast<int>(NcType::INT)]*8,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::INT)]*num_values);
	delete[] tmpbuf;
	break;
    }
    case NcType::FLOAT:
    {
	auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::FLOAT)]*num_values];
	bits::set(tmpbuf,reinterpret_cast<int *>(data_array),0,32,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::FLOAT)]*num_values);
	delete[] tmpbuf;
	break;
    }
    case NcType::DOUBLE:
    {
	auto *tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::DOUBLE)]*num_values];
	bits::set(tmpbuf,reinterpret_cast<long long *>(data_array),0,64,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::DOUBLE)]*num_values);
	delete[] tmpbuf;
	break;
    }
    default: {}
  }
  curr_offset+=(nc_size[static_cast<int>(vars[index].nc_type)]*num_values);
}

void OutputNetCDFStream::initialize_non_record_data(std::string variable_name)
{
  if (started_record_vars) {
    myerror="cannot write non-record variables after record variables";
    exit(1);
  }
  if (non_rec_housekeeping.index >= 0) {
    myerror="initialize error; previous non-record variable was not completed";
    exit(1);
  }
  non_rec_housekeeping.index=find_variable(variable_name);
  if (non_rec_housekeeping.index < 0) {
    return;
  }
  if (vars[non_rec_housekeeping.index].is_rec) {
    myerror="record variable '"+vars[non_rec_housekeeping.index].name+"' cannot be initialized with OutputNetCDFStream::initialize_non_record_data()";
    exit(1);
  }
  non_rec_housekeeping.num_values=vars[non_rec_housekeeping.index].size/nc_size[static_cast<int>(vars[non_rec_housekeeping.index].nc_type)];
  non_rec_housekeeping.num_values_written=0;
  fs.seekg(vars[non_rec_housekeeping.index].offset,std::ios_base::beg);
  auto *tmpbuf=new char[4];
  bits::set(reinterpret_cast<unsigned char *>(tmpbuf),curr_offset,0,32);
  fs.write(tmpbuf,4);
  delete[] tmpbuf;
  fs.seekg(curr_offset,std::ios_base::beg);
}

void OutputNetCDFStream::write_partial_non_record_data(void *data_array,int num_values)
{
  if (non_rec_housekeeping.index < 0) {
    myerror="error in write_partial_non_record_data: not initialized or variable already completed - filename: "+file_name;
    exit(1);
  }
  switch (vars[non_rec_housekeeping.index].nc_type) {
    case NcType::BYTE:
    case NcType::CHAR:
    {
	fs.write(reinterpret_cast<char *>(data_array),num_values);
	break;
    }
    case NcType::SHORT:
    {
	auto tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::SHORT)]*num_values];
	bits::set(tmpbuf,reinterpret_cast<short *>(data_array),0,nc_size[static_cast<int>(NcType::SHORT)]*8,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::SHORT)]*num_values);
	delete[] tmpbuf;
	break;
    }
    case NcType::INT:
    {
	auto tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::INT)]*num_values];
	bits::set(tmpbuf,reinterpret_cast<int *>(data_array),0,nc_size[static_cast<int>(NcType::INT)]*8,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::INT)]*num_values);
	delete[] tmpbuf;
	break;
    }
    case NcType::FLOAT:
    {
	auto tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::FLOAT)]*num_values];
	bits::set(tmpbuf,reinterpret_cast<int *>(data_array),0,32,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::FLOAT)]*num_values);
	delete[] tmpbuf;
	break;
    }
    case NcType::DOUBLE:
    {
	auto tmpbuf=new unsigned char[nc_size[static_cast<int>(NcType::DOUBLE)]*num_values];
	bits::set(tmpbuf,reinterpret_cast<long long *>(data_array),0,64,0,num_values);
	fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::DOUBLE)]*num_values);
	delete[] tmpbuf;
	break;
    }
    default: {}
  }
  curr_offset+=(nc_size[static_cast<int>(vars[non_rec_housekeeping.index].nc_type)]*num_values);
  non_rec_housekeeping.num_values_written+=num_values;
  if (non_rec_housekeeping.num_values_written > non_rec_housekeeping.num_values) {
    myerror="error in write_partial_non_record_data: too many data values written";
    exit(1);
  }
  else if (non_rec_housekeeping.num_values_written == non_rec_housekeeping.num_values) {
    non_rec_housekeeping.index=-1;
    non_rec_housekeeping.num_values=0;
    non_rec_housekeeping.num_values_written=0;
  }
}

void OutputNetCDFStream::add_attribute(std::vector<Attribute>& attributes_to_grow,std::string name,NcType nc_type,size_t num_values,void *values)
{
  size_t n=attributes_to_grow.size();
  size_t m;

  attributes_to_grow.resize(n+1);
  attributes_to_grow[n].name=name;
  attributes_to_grow[n].nc_type=nc_type;
  attributes_to_grow[n].num_values=num_values;
  switch (nc_type) {
    case NcType::CHAR:
    {
	attributes_to_grow[n].values=new std::string;
	(*reinterpret_cast<std::string *>(attributes_to_grow[n].values))=(*reinterpret_cast<std::string *>(values));
	break;
    }
    case NcType::SHORT:
    {
	attributes_to_grow[n].values=new short[num_values];
	for (m=0; m < num_values; m++)
	  (reinterpret_cast<short *>(attributes_to_grow[n].values))[m]=(reinterpret_cast<short *>(values))[m];
	break;
    }
    case NcType::INT:
    {
	(attributes_to_grow[n].values)=new int[num_values];
	for (m=0; m < num_values; m++)
	  (reinterpret_cast<int *>(attributes_to_grow[n].values))[m]=(reinterpret_cast<int *>(values))[m];
	break;
    }
    case NcType::FLOAT:
    {
	(attributes_to_grow[n].values)=new float[num_values];
	for (m=0; m < num_values; m++)
	  (reinterpret_cast<float *>(attributes_to_grow[n].values))[m]=(reinterpret_cast<float *>(values))[m];
	break;
    }
    case NcType::DOUBLE:
    {
	(attributes_to_grow[n].values)=new double[num_values];
	for (m=0; m < num_values; m++)
	  (reinterpret_cast<double *>(attributes_to_grow[n].values))[m]=(reinterpret_cast<double *>(values))[m];
	break;
    }
    default: {}
  }
}

void OutputNetCDFStream::put_string(const std::string& string_to_put)
{
  size_t str_len=string_to_put.length();
  char tmpbuf[4],zero=0;

  bits::set(tmpbuf,str_len,0,32);
  fs.write(tmpbuf,4);
  curr_offset+=4;
  fs.write(string_to_put.c_str(),str_len);
  curr_offset+=str_len;
  while ( (str_len % 4) != 0) {
    fs.write(&zero,1);
    curr_offset++;
    str_len++;
  }
}

void OutputNetCDFStream::put_attributes(const std::vector<Attribute>& attributes_to_put)
{
  unsigned char tmpbuf[8];
  int flag=0;
  size_t n,m;
  char zero[]={0,0,0};
  int dum;
  void *vdum;

  if (attributes_to_put.size() > 0) {
    flag=static_cast<int>(Type::ATTRIBUTE);
  }
  bits::set(tmpbuf,flag,0,32);
  fs.write(reinterpret_cast<char *>(tmpbuf),4);
  curr_offset+=4;
  dum=attributes_to_put.size();
  bits::set(tmpbuf,dum,0,32);
  fs.write(reinterpret_cast<char *>(tmpbuf),4);
  curr_offset+=4;
  for (n=0; n < attributes_to_put.size(); n++) {
    put_string(attributes_to_put[n].name);
    dum=static_cast<int>(attributes_to_put[n].nc_type);
    bits::set(tmpbuf,dum,0,32);
    fs.write(reinterpret_cast<char *>(tmpbuf),4);
    curr_offset+=4;
    switch (attributes_to_put[n].nc_type) {
	case NcType::BYTE:
	{
	  dum=attributes_to_put[n].num_values;
	  bits::set(tmpbuf,dum,0,32);
	  fs.write(reinterpret_cast<char *>(tmpbuf),4);
	  curr_offset+=4;
	  dum=(reinterpret_cast<unsigned char *>(attributes_to_put[n].values))[0];
	  bits::set(tmpbuf,dum,0,nc_size[static_cast<int>(NcType::BYTE)]*8);
	  fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::BYTE)]);
	  curr_offset+=nc_size[static_cast<int>(NcType::BYTE)];
	  if ( (attributes_to_put[n].num_values % 4) != 0) {
	    fs.write(zero,(attributes_to_put[n].num_values % 4));
	    curr_offset+=(nc_size[static_cast<int>(NcType::BYTE)]*(attributes_to_put[n].num_values % 4));
	  }
	  break;
	}
	case NcType::CHAR:
	{
	  put_string(*(reinterpret_cast<std::string *>(attributes_to_put[n].values)));
	  break;
	}
	case NcType::SHORT:
	{
	  dum=attributes_to_put[n].num_values;
	  bits::set(tmpbuf,dum,0,32);
	  fs.write(reinterpret_cast<char *>(tmpbuf),4);
	  curr_offset+=4;
	  vdum=new short;
	  for (m=0; m < attributes_to_put[n].num_values; m++) {
	    *(reinterpret_cast<short *>(vdum))=(reinterpret_cast<short *>(attributes_to_put[n].values))[m];
	    bits::set(tmpbuf,*(reinterpret_cast<short *>(vdum)),0,nc_size[static_cast<int>(NcType::SHORT)]*8);
	    fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::SHORT)]);
	  }
	  curr_offset+=(nc_size[static_cast<int>(NcType::SHORT)]*attributes_to_put[n].num_values);
	  if ( (m=curr_offset % 4) != 0) {
	    fs.write(zero,m);
	    curr_offset+=m;
	  }
	  delete reinterpret_cast<short *>(vdum);
	  break;
	}
	case NcType::INT:
	{
	  dum=attributes_to_put[n].num_values;
	  bits::set(tmpbuf,dum,0,32);
	  fs.write(reinterpret_cast<char *>(tmpbuf),4);
	  curr_offset+=4;
	  vdum=new int;
	  for (m=0; m < attributes_to_put[n].num_values; m++) {
	    *(reinterpret_cast<int *>(vdum))=(reinterpret_cast<int *>(attributes_to_put[n].values))[m];
	    bits::set(tmpbuf,*(reinterpret_cast<int *>(vdum)),0,nc_size[static_cast<int>(NcType::INT)]*8);
	    fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::INT)]);
	  }
	  curr_offset+=(nc_size[static_cast<int>(NcType::INT)]*attributes_to_put[n].num_values);
	  delete reinterpret_cast<int *>(vdum);
	  break;
	}
	case NcType::FLOAT:
	{
	  dum=attributes_to_put[n].num_values;
	  bits::set(tmpbuf,dum,0,32);
	  fs.write(reinterpret_cast<char *>(tmpbuf),4);
	  curr_offset+=4;
	  vdum=new float;
	  for (m=0; m < attributes_to_put[n].num_values; m++) {
	    *(reinterpret_cast<float *>(vdum))=(reinterpret_cast<float *>(attributes_to_put[n].values))[m];
	    bits::set(tmpbuf,*(reinterpret_cast<int *>(vdum)),0,nc_size[static_cast<int>(NcType::FLOAT)]*8);
	    fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::FLOAT)]);
	  }
	  curr_offset+=(nc_size[static_cast<int>(NcType::FLOAT)]*attributes_to_put[n].num_values);
	  delete reinterpret_cast<float *>(vdum);
	  break;
	}
	case NcType::DOUBLE:
	{
	  dum=attributes_to_put[n].num_values;
	  bits::set(tmpbuf,dum,0,32);
	  fs.write(reinterpret_cast<char *>(tmpbuf),4);
	  curr_offset+=4;
	  vdum=new double;
	  for (m=0; m < attributes_to_put[n].num_values; m++) {
	    *(reinterpret_cast<double *>(vdum))=(reinterpret_cast<double *>(attributes_to_put[n].values))[m];
	    bits::set(tmpbuf,*(reinterpret_cast<long long *>(vdum)),0,nc_size[static_cast<int>(NcType::DOUBLE)]*8);
	    fs.write(reinterpret_cast<char *>(tmpbuf),nc_size[static_cast<int>(NcType::DOUBLE)]);
	  }
	  curr_offset+=(nc_size[static_cast<int>(NcType::DOUBLE)]*attributes_to_put[n].num_values);
	  delete reinterpret_cast<double *>(vdum);
	  break;
	}
	default:
	{
	  myerror="put_attributes error: attribute type "+strutils::itos(static_cast<int>(attributes_to_put[n].nc_type))+" not recognized";
	  exit(1);
	}
    }
  }
}

bool operator==(const netCDFStream::UniqueVariableTimeEntry& te1,const netCDFStream::UniqueVariableTimeEntry& te2)
{
  if (te1.first_valid_datetime == nullptr || te2.first_valid_datetime == nullptr || te1.last_valid_datetime == nullptr || te2.last_valid_datetime == nullptr || te1.reference_datetime == nullptr || te2.reference_datetime == nullptr) {
    if (te1.first_valid_datetime == nullptr && te2.first_valid_datetime == nullptr && te1.last_valid_datetime == nullptr && te2.last_valid_datetime == nullptr && te1.reference_datetime == nullptr && te2.reference_datetime == nullptr) {
	return true;
    }
    else {
	return false;
    }
  }
  else {
    if (*te1.first_valid_datetime == *te2.first_valid_datetime && *te1.last_valid_datetime == *te2.last_valid_datetime && *te1.reference_datetime == *te2.reference_datetime) {
	return true;
    }
    else {
	return false;
    }
  }
}

bool operator!=(const netCDFStream::UniqueVariableTimeEntry& te1,const netCDFStream::UniqueVariableTimeEntry& te2)
{
  return !(te1 == te2);
}
