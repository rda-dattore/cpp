// FILE: iodstream.cpp

#include <iostream>
#include <iodstream.hpp>

const idstream_iterator& idstream_iterator::operator++()
{
  auto num_bytes=_stream.peek();
  switch (num_bytes) {
    case bfstream::eof:
    {
	if (_stream.number_read() == 0) {
	  std::cerr << "Error reading data stream (maybe the wrong type?)" << std::endl;
	  exit(1);
	}
	else {
	  _is_end=true;
	}
	break;
    }
    case bfstream::error:
    {
	std::cerr << "Error peeking data stream" << std::endl;
	exit(1);
    }
    default:
    {
	if (num_bytes > buffer_capacity) {
	  buffer_capacity=num_bytes;
	  buffer.reset(new unsigned char[buffer_capacity]);
	}
	_stream.read(buffer.get(),buffer_capacity);
    }
  }
  return *this;
}

unsigned char *idstream_iterator::operator*()
{
  if (_is_end) {
    return nullptr;
  }
  else {
    return buffer.get();
  }
}

idstream_iterator idstream::begin()
{
  rewind();
  auto i=idstream_iterator(*this,false);
  ++i;
  return i;
}

void idstream::close()
{
  if (fs.is_open()) {
    fs.close();
    file_name="";
  }
  else if (icosstream != nullptr) {
    icosstream->close();
    icosstream.reset(nullptr);
  }
  else if (irptstream != nullptr) {
    irptstream->close();
    irptstream.reset(nullptr);
  }
  else if (if77_stream != nullptr) {
    if77_stream->close();
    if77_stream.reset(nullptr);
  }
}

idstream_iterator idstream::end()
{
  return idstream_iterator(*this,true);
}

bool idstream::open(std::string filename)
{
  if (is_open()) {
    std::cerr << "Error: currently connected to another file stream" << std::endl;
    exit(1);
  }
  num_read=0;
// check for rptout-blocked first, since rptout files can have COS-blocking on
// them
  irptstream.reset(new irstream);
  if (!irptstream->open(filename)) {
    irptstream.reset(nullptr);
    return false;
  }
  unsigned char test;
  if (irptstream->read(&test,1) < 0) {
    irptstream->close();
    irptstream.reset(nullptr);
// now check to see if file is COS-blocked
    icosstream.reset(new icstream);
    if (!icosstream->open(filename)) {
	icosstream.reset(nullptr);
	return false;
    }
    if (icosstream->read(&test,1) < 0) {
	icosstream->close();
	icosstream.reset(nullptr);
// now check to see if file is F77-blocked
	if77_stream.reset(new if77stream);
	if (!if77_stream->open(filename)) {
	  if77_stream.reset(nullptr);
	  return false;
	}
	if (if77_stream->read(&test,1) < 0) {
	  if77_stream->close();
	  if77_stream.reset(nullptr);
// now check to see if file is VBS-blocked
	  ivstream.reset(new ivbsstream);
	  if (!ivstream->open(filename)) {
	    ivstream.reset(nullptr);
	    return false;
	  }
	  if (ivstream->read(&test,1) < 0) {
	    ivstream->close();
	    ivstream.reset(nullptr);
// file must be plain-binary
	    fs.open(filename.c_str(),std::ios_base::in);
	    if (!fs.is_open()) {
		return false;
	    }
	    file_name=filename;
	  }
	  else {
	    ivstream->rewind();
	  }
	}
	else {
	  if77_stream->rewind();
	}
    }
    else {
	icosstream->rewind();
    }
  }
  else {
    irptstream->rewind();
  }
  return true;
}

void idstream::rewind()
{
  if (fs.is_open()) {
    fs.clear();
    fs.seekg(std::ios_base::beg);
  }
  else if (icosstream != nullptr) {
    icosstream->rewind();
  }
  else if (irptstream != nullptr) {
    irptstream->rewind();
  }
  else if (if77_stream != nullptr) {
    if77_stream->rewind();
  }
  num_read=0;
}

void odstream::close()
{
  if (fs.is_open()) {
    fs.close();
    file_name="";
  }
  else if (ocosstream != nullptr) {
    ocosstream->close();
    ocosstream.reset(nullptr);
  }
  else if (orptstream != nullptr) {
    orptstream->close();
    orptstream.reset(nullptr);
  }
  else if (ocrptstream != nullptr) {
    ocrptstream->close();
    ocrptstream.reset(nullptr);
  }
}

bool odstream::open(std::string filename,size_t blocking_flag)
{
  if (is_open()) {
    std::cerr << "Error: currently connected to another file stream" << std::endl;
    exit(1);
  }
  num_written=0;
  switch (blocking_flag) {
    case binary:
	fs.open(filename.c_str(),std::ios_base::out);
	if (!fs.is_open()) {
	  return false;
	}
	file_name=filename;
	return true;
    case cos:
    {
	ocosstream.reset(new ocstream);
	if (!ocosstream->open(filename)) {
	  ocosstream.reset(nullptr);
	  return false;
	}
	return true;
    }
    case rptout:
    {
	orptstream.reset(new orstream);
	if (!orptstream->open(filename)) {
	  orptstream.reset(nullptr);
	  return false;
	}
	return true;
    }
    case cos_rptout:
    {
	ocrptstream.reset(new ocrstream);
	if (!ocrptstream->open(filename)) {
	  ocrptstream.reset(nullptr);
	  return false;
	}
	return true;
    }
    default:
    {
	return false;
    }
  }
}

int odstream::write(const unsigned char *buffer,size_t num_bytes)
{
  size_t num_out;
  if (fs.is_open()) {
    fs.write(reinterpret_cast<const char *>(buffer),num_bytes);
    if (!fs.good()) {
	num_out=bfstream::error;
    }
    else {
	num_out=num_bytes;
    }
  }
  else if (ocosstream != nullptr) {
    num_out=ocosstream->write(buffer,num_bytes);
  }
  else if (orptstream != nullptr) {
    num_out=orptstream->write(buffer,num_bytes);
  }
  else if (ocrptstream != nullptr) {
    num_out=ocrptstream->write(buffer,num_bytes);
  }
  else {
    std::cerr << "Error: no open output filestream" << std::endl;
    exit(1);
  }
  ++num_written;
  return num_out;
}
