// FILE: iodstream.cpp

#include <iostream>
#include <iodstream.hpp>

using std::runtime_error;
using std::string;

const idstream_iterator& idstream_iterator::operator++() {
  auto num_bytes = stream.peek();
  switch (num_bytes) {
    case bfstream::eof: {
      if (stream.number_read() == 0) {
        throw runtime_error("error reading data stream (maybe the wrong "
            "type?)");
      }
      else {
        is_end = true;
      }
      break;
    }
    case bfstream::error: {
      throw runtime_error("error peeking data stream");
    }
    default: {
      if (num_bytes > buffer_capacity) {
        buffer_capacity = num_bytes;
        buffer.reset(new unsigned char[buffer_capacity]);
      }
      stream.read(buffer.get(), buffer_capacity);
    }
  }
  return *this;
}

unsigned char *idstream_iterator::operator*() {
  if (is_end) {
    return nullptr;
  }
  return buffer.get();
}

idstream_iterator idstream::begin() {
  rewind();
  idstream_iterator i(*this, false); // return value
  ++i;
  return i;
}

void idstream::close() {
  if (fs.is_open()) {
    fs.close();
    file_name = "";
  } else if (icosstream != nullptr) {
    icosstream->close();
    icosstream.reset(nullptr);
  } else if (irptstream != nullptr) {
    irptstream->close();
    irptstream.reset(nullptr);
  } else if (if77_stream != nullptr) {
    if77_stream->close();
    if77_stream.reset(nullptr);
  }
}

idstream_iterator idstream::end() {
  return idstream_iterator(*this, true);
}

bool idstream::open(string filename) {
  if (is_open()) {
    throw runtime_error("currently connected to another file stream");
  }
  num_read = 0;

  // check for rptout-blocked first, since rptout files can have COS-blocking on
  //  them
  irptstream.reset(new irstream);
  if (!irptstream->open(filename)) {
    irptstream.reset(nullptr);
    return false;
  }
  unsigned char c;
  if (irptstream->read(&c, 1) < 0) {
    irptstream->close();
    irptstream.reset(nullptr);

    // now check to see if file is COS-blocked
    icosstream.reset(new icstream);
    if (!icosstream->open(filename)) {
      icosstream.reset(nullptr);
      return false;
    }
    if (icosstream->read(&c, 1) < 0) {
      icosstream->close();
      icosstream.reset(nullptr);

      // now check to see if file is F77-blocked
      if77_stream.reset(new if77stream);
      if (!if77_stream->open(filename)) {
        if77_stream.reset(nullptr);
        return false;
      }
      if (if77_stream->read(&c, 1) < 0) {
        if77_stream->close();
        if77_stream.reset(nullptr);

        // now check to see if file is VBS-blocked
        ivstream.reset(new ivbsstream);
        if (!ivstream->open(filename)) {
          ivstream.reset(nullptr);
          return false;
        }
        if (ivstream->read(&c, 1) < 0) {
          ivstream->close();
          ivstream.reset(nullptr);

          // file must be plain-binary
          fs.open(filename.c_str(), std::ios_base::in);
          if (!fs.is_open()) {
            return false;
          }
          file_name = filename;
        } else {
          ivstream->rewind();
        }
      } else {
        if77_stream->rewind();
      }
    } else {
      icosstream->rewind();
    }
  } else {
    irptstream->rewind();
  }
  return true;
}

void idstream::rewind() {
  if (fs.is_open()) {
    fs.clear();
    fs.seekg(std::ios_base::beg);
  } else if (icosstream != nullptr) {
    icosstream->rewind();
  } else if (irptstream != nullptr) {
    irptstream->rewind();
  } else if (if77_stream != nullptr) {
    if77_stream->rewind();
  }
  num_read = 0;
}

void odstream::close() {
  if (fs.is_open()) {
    fs.close();
    file_name = "";
  } else if (ocosstream != nullptr) {
    ocosstream->close();
    ocosstream.reset(nullptr);
  } else if (orptstream != nullptr) {
    orptstream->close();
    orptstream.reset(nullptr);
  } else if (ocrptstream != nullptr) {
    ocrptstream->close();
    ocrptstream.reset(nullptr);
  }
}

bool odstream::open(string filename,size_t blocking_flag) {
  if (is_open()) {
    throw runtime_error("currently connected to another file stream");
  }
  num_written = 0;
  switch (blocking_flag) {
    case binary: {
      fs.open(filename.c_str(), std::ios_base::out);
      if (!fs.is_open()) {
        return false;
      }
      file_name = filename;
      return true;
    }
    case cos: {
      ocosstream.reset(new ocstream);
      if (!ocosstream->open(filename)) {
        ocosstream.reset(nullptr);
        return false;
      }
      return true;
    }
    case rptout: {
      orptstream.reset(new orstream);
      if (!orptstream->open(filename)) {
        orptstream.reset(nullptr);
        return false;
      }
      return true;
    }
    case cos_rptout: {
      ocrptstream.reset(new ocrstream);
      if (!ocrptstream->open(filename)) {
        ocrptstream.reset(nullptr);
        return false;
      }
      return true;
    }
    default: {
      return false;
    }
  }
}

int odstream::write(const unsigned char *buffer, size_t num_bytes) {
  int n; // return value
  if (fs.is_open()) {
    fs.write(reinterpret_cast<const char *>(buffer), num_bytes);
    if (!fs.good()) {
      n = bfstream::error;
    }
    else {
      n = num_bytes;
    }
  }
  else if (ocosstream != nullptr) {
    n = ocosstream->write(buffer, num_bytes);
  }
  else if (orptstream != nullptr) {
    n = orptstream->write(buffer, num_bytes);
  }
  else if (ocrptstream != nullptr) {
    n = ocrptstream->write(buffer, num_bytes);
  }
  else {
    throw runtime_error("no open output filestream");
  }
  ++num_written;
  return n;
}
