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
      } else {
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
  } else if (ics != nullptr) {
    ics->close();
    ics.reset(nullptr);
  } else if (irs != nullptr) {
    irs->close();
    irs.reset(nullptr);
  } else if (if77s != nullptr) {
    if77s->close();
    if77s.reset(nullptr);
  } else if (is3s != nullptr) {
    is3s->close();
    is3s.reset(nullptr);
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
  if (filename.substr(0, 4) == "http") {

    // file is on an S3 device
    is3s.reset(new is3stream);
    return is3s->open(filename);
  }

  // check for rptout-blocked first, since rptout files can have COS-blocking on
  //  them
  irs.reset(new irstream);
  if (!irs->open(filename)) {
    irs.reset(nullptr);
    return false;
  }
  unsigned char c;
  if (irs->read(&c, 1) < 0) {
    irs->close();
    irs.reset(nullptr);

    // now check to see if file is COS-blocked
    ics.reset(new icstream);
    if (!ics->open(filename)) {
      ics.reset(nullptr);
      return false;
    }
    if (ics->read(&c, 1) < 0) {
      ics->close();
      ics.reset(nullptr);

      // now check to see if file is F77-blocked
      if77s.reset(new if77stream);
      if (!if77s->open(filename)) {
        if77s.reset(nullptr);
        return false;
      }
      if (if77s->read(&c, 1) < 0) {
        if77s->close();
        if77s.reset(nullptr);

        // now check to see if file is VBS-blocked
        ivs.reset(new ivbsstream);
        if (!ivs->open(filename)) {
          ivs.reset(nullptr);
          return false;
        }
        if (ivs->read(&c, 1) < 0) {
          ivs->close();
          ivs.reset(nullptr);

          // file must be plain-binary
          fs.open(filename.c_str(), std::ios_base::in);
          if (!fs.is_open()) {
            return false;
          }
          file_name = filename;
        } else {
          ivs->rewind();
        }
      } else {
        if77s->rewind();
      }
    } else {
      ics->rewind();
    }
  } else {
    irs->rewind();
  }
  return true;
}

void idstream::rewind() {
  if (fs.is_open()) {
    fs.clear();
    fs.seekg(std::ios_base::beg);
  } else if (ics != nullptr) {
    ics->rewind();
  } else if (irs != nullptr) {
    irs->rewind();
  } else if (if77s != nullptr) {
    if77s->rewind();
  }
  num_read = 0;
}

void odstream::close() {
  if (fs.is_open()) {
    fs.close();
    file_name = "";
  } else if (ocs != nullptr) {
    ocs->close();
    ocs.reset(nullptr);
  } else if (ors != nullptr) {
    ors->close();
    ors.reset(nullptr);
  } else if (ocrs != nullptr) {
    ocrs->close();
    ocrs.reset(nullptr);
  }
}

bool odstream::open(string filename, Blocking block_flag) {
  if (is_open()) {
    throw runtime_error("currently connected to another file stream");
  }
  num_written = 0;
  switch (block_flag) {
    case Blocking::binary: {
      fs.open(filename.c_str(), std::ios_base::out);
      if (!fs.is_open()) {
        return false;
      }
      file_name = filename;
      return true;
    }
    case Blocking::cos: {
      ocs.reset(new ocstream);
      if (!ocs->open(filename)) {
        ocs.reset(nullptr);
        return false;
      }
      return true;
    }
    case Blocking::rptout: {
      ors.reset(new orstream);
      if (!ors->open(filename)) {
        ors.reset(nullptr);
        return false;
      }
      return true;
    }
    case Blocking::cos_rptout: {
      ocrs.reset(new ocrstream);
      if (!ocrs->open(filename)) {
        ocrs.reset(nullptr);
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
    } else {
      n = num_bytes;
    }
  } else if (ocs != nullptr) {
    n = ocs->write(buffer, num_bytes);
  } else if (ors != nullptr) {
    n = ors->write(buffer, num_bytes);
  } else if (ocrs != nullptr) {
    n = ocrs->write(buffer, num_bytes);
  } else {
    throw runtime_error("no open output filestream");
  }
  ++num_written;
  return n;
}
