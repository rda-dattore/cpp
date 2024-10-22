#include <grid.hpp>
#include <bits.hpp>
#include <myexception.hpp>

using std::runtime_error;
using std::string;
using std::to_string;

int InputGRIBStream::peek() {
  unsigned char buffer[16];
  int len = -9;
  while (len == -9) {
    len = find_grib(buffer);
    if (len < 0) {
      return len;
    }
#ifdef __WITH_S3
    if (is3s != nullptr) {
      auto n = is3s->read(&buffer[4], 12);
      if (n > 0) {
        len += n;
      }
    } else {
#endif
      fs.read(reinterpret_cast<char *>(&buffer[4]), 12);
      len += fs.gcount();
#ifdef __WITH_S3
    }
#endif
    if (len != 16) {
      return bfstream::error;
    }
    switch (static_cast<int>(buffer[7])) {
      case 0: {
        len = 4;
        fs.seekg(curr_offset + 4, std::ios_base::beg);
        fs.read(reinterpret_cast<char *>(buffer), 3);
        if (fs.eof()) {
          return bfstream::eof;
        } else if (!fs.good()) {
          return bfstream::error;
        }
        while (buffer[0] != '7' && buffer[1] != '7' && buffer[2] != '7') {
          int llen;
          bits::get(buffer, llen, 0, 24);
          if (llen <= 0) {
            return bfstream::error;
          }
          len += llen;
          fs.seekg(llen - 3, std::ios_base::cur);
          fs.read(reinterpret_cast<char *>(buffer), 3);
          if (fs.eof()) {
            return bfstream::eof;
          } else if (!fs.good()) {
            return bfstream::error;
          }
        }
        len += 4;
        break;
      }
      case 1: {
        bits::get(buffer, len, 32, 24);

        // check for ECMWF large-file
        if (len >= 0x800000) {
          int computed_len = 0;
          int sec_len, flag;

          // PDS length
          bits::get(buffer, sec_len, 64, 24);
          bits::get(buffer, flag, 120, 8);
          computed_len = 8 + sec_len;
          fs.seekg(curr_offset + computed_len, std::ios_base::beg);

          // GDS included?
          if ( (flag & 0x80) == 0x80) {
            fs.read(reinterpret_cast<char *>(buffer), 3);
            bits::get(buffer, sec_len, 0, 24);
            fs.seekg(sec_len - 3, std::ios_base::cur);
            computed_len += sec_len;
          }

          // BMS included?
          if ( (flag & 0x40) == 0x40) {
            fs.read(reinterpret_cast<char *>(buffer), 3);
            bits::get(buffer, sec_len, 0, 24);
            fs.seekg(sec_len - 3, std::ios_base::cur);
            computed_len += sec_len;
          }
          fs.read(reinterpret_cast<char *>(buffer), 3);

          // BDS length
          bits::get(buffer, sec_len, 0, 24);
          if (sec_len < 120) {
            len &= 0x7fffff;
            len *= 120;
            len -= sec_len;
            fs.seekg(curr_offset + len, std::ios_base::beg);
            len += 4;
          } else {
            fs.seekg(sec_len - 3, std::ios_base::cur);
          }
        } else {
#ifdef __WITH_S3
          if (is3s != nullptr) {
            is3s->seek(is3s->tell() + len - 20);
          } else {
#endif
            fs.seekg(len - 20, std::ios_base::cur);
#ifdef __WITH_S3
          }
#endif
        }
#ifdef __WITH_S3
        if (is3s != nullptr) {
          is3s->read(buffer, 4);
        } else {
#endif
          fs.read(reinterpret_cast<char *>(buffer), 4);
#ifdef __WITH_S3
        }
#endif
        if (string(reinterpret_cast<char *>(buffer), 4) != "7777") {
          len = -9;
#ifdef __WITH_S3
          if (is3s != nullptr) {
            is3s->seek(curr_offset + 4);
          } else {
#endif
            fs.seekg(curr_offset + 4, std::ios_base::beg);
#ifdef __WITH_S3
          }
#endif
        }
        break;
      }
      case 2: {
        bits::get(buffer, len, 96, 32);
        break;
      }
      default: {
        return peek();
      }
    }
  }
#ifdef __WITH_S3
  if (is3s != nullptr) {
    is3s->seek(curr_offset);
  } else {
#endif
    fs.seekg(curr_offset, std::ios_base::beg);
#ifdef __WITH_S3
  }
#endif
  return len;
}

int InputGRIBStream::read(unsigned char *buffer, size_t buffer_length) {
  int bytes_read = 0; // return value
  int len = -9;
  while (len == -9) {
    bytes_read = find_grib(buffer);
    if (bytes_read < 0) {
      return bytes_read;
    }
#ifdef __WITH_S3
    if (is3s != nullptr) {
      auto byts = is3s->read(&buffer[4], 12);
      if (byts > 0) {
        bytes_read += byts;
        if (bytes_read != 16) {
          return bfstream::error;
        }
      } else {
        return byts;
      }
    } else {
#endif
      fs.read(reinterpret_cast<char *>(&buffer[4]), 12);
      bytes_read += fs.gcount();
      if (bytes_read != 16) {
        if (fs.eof()) {
          return bfstream::eof;
        } else {
          return bfstream::error;
        }
      }
#ifdef __WITH_S3
    }
#endif
    switch (static_cast<int>(buffer[7])) {
      case 0: {
#ifdef __WITH_S3
        if (is3s != nullptr) {
          auto byts = is3s->read(&buffer[16], 15);
          if (byts > 0) {
            bytes_read += byts;
          }
          if (byts < 0 || bytes_read != 31) {
            return bfstream::eof;
          }
        } else {
#endif
          fs.read(reinterpret_cast<char *>(&buffer[16]), 15);
          bytes_read += fs.gcount();
          if (bytes_read != 31) {
            return bfstream::eof;
          }
#ifdef __WITH_S3
        }
#endif
        len = 28;
        while (buffer[bytes_read-3] != '7' && buffer[bytes_read-2] != '7' &&
            buffer[bytes_read-1] != '7') {
          int llen;
          bits::get(buffer, llen, len * 8, 24);
          if (llen <= 0) {
            return bfstream::error;
          }
          len += llen;
          if (len > static_cast<int>(buffer_length)) {
            throw my::BufferOverflow_Error("input buffer too small.");
          }
#ifdef __WITH_S3
          if (is3s != nullptr) {
            auto byts = is3s->read(&buffer[bytes_read], llen);
            if (byts > 0) {
              bytes_read += byts;
            } else {
              return byts;
            }
          } else {
#endif
            fs.read(reinterpret_cast<char *>(&buffer[bytes_read]), llen);
            bytes_read += fs.gcount();
            if (bytes_read != len+3) {
              if (fs.eof()) {
                return bfstream::eof;
              } else {
                return bfstream::error;
              }
            }
#ifdef __WITH_S3
          }
#endif
        }
#ifdef __WITH_S3
        if (is3s != nullptr) {
          auto byts = is3s->read(&buffer[bytes_read], 1);
          bytes_read += byts;
        } else {
#endif
          fs.read(reinterpret_cast<char *>(&buffer[bytes_read]), 1);
          bytes_read += fs.gcount();
#ifdef __WITH_S3
        }
#endif
        break;
      }
      case 1: {
        bits::get(buffer, len, 32, 24);
        if (len > static_cast<int>(buffer_length)) {
          throw my::BufferOverflow_Error("input buffer too small.");
        }
#ifdef __WITH_S3
        if (is3s != nullptr) {
          auto byts = is3s->read(&buffer[16], len - 16);
          if (byts > 0) {
            bytes_read += byts;
          } else {
            return byts;
          }
          if (bytes_read != len) {
            return bfstream::error;
          }
        } else {
#endif
          fs.read(reinterpret_cast<char *>(&buffer[16]), len - 16);
          bytes_read += fs.gcount();
          if (bytes_read != len) {
            if (fs.eof()) {
              return bfstream::eof;
            } else {
              return bfstream::error;
            }
          }
#ifdef __WITH_S3
        }
#endif

        // check for ECMWF large-file
        if (len >= 0x800000) {
          int computed_len=0, sec_len, flag;

          // PDS length
          bits::get(buffer, sec_len, 64, 24);
          bits::get(buffer, flag, 120, 8);
          computed_len = 8 + sec_len;

          // GDS included
          if ( (flag & 0x80) == 0x80) {
            bits::get(buffer, sec_len, computed_len * 8, 24);
            computed_len += sec_len;
          }

          // BMS included
          if ( (flag & 0x40) == 0x40) {
            bits::get(buffer, sec_len, computed_len * 8, 24);
            computed_len += sec_len;
          }

          // BDS length
          bits::get(buffer, sec_len, computed_len * 8, 24);
          if (sec_len < 120) {
            len &= 0x7fffff;
            len *= 120;
            len -= sec_len;
            len += 4;
            if (len > static_cast<int>(buffer_length)) {
              throw my::BufferOverflow_Error("input buffer too small.");
            }
#ifdef __WITH_S3
            if (is3s != nullptr) {
              auto byts = is3s->read(&buffer[bytes_read], len - bytes_read);
              bytes_read += byts;
            } else {
#endif
              fs.read(reinterpret_cast<char *>(&buffer[bytes_read]), len -
                  bytes_read);
              bytes_read += fs.gcount();
#ifdef __WITH_S3
            }
#endif
          }
        }
        if (string(reinterpret_cast<char *>(&buffer[len-4]), 4) !=
            "7777") {
          len = -9;
          fs.seekg(curr_offset + 4, std::ios_base::beg);
        }
        break;
      }
      case 2: {
        bits::get(buffer, len, 96, 32);
        if (len > static_cast<int>(buffer_length)) {
          throw my::BufferOverflow_Error("input buffer too small.");
        }
#ifdef __WITH_S3
        if (is3s != nullptr) {
          auto byts = is3s->read(&buffer[16], len - 16);
          if (byts > 0) {
            bytes_read += byts;
          } else {
            return byts;
          }
          if (bytes_read != len) {
            return bfstream::error;
          }
        } else {
#endif
          fs.read(reinterpret_cast<char *>(&buffer[16]), len - 16);
          bytes_read += fs.gcount();
          if (bytes_read != len) {
            if (fs.eof()) {
              return bfstream::eof;
            } else {
              return bfstream::error;
            }
          }
#ifdef __WITH_S3
        }
#endif
        if (string(reinterpret_cast<char *>(buffer) + len - 4, 4) != "7777") {
          throw my::NotFound_Error("unable to find GRIB END section where "
              "expected.");
        }
        break;
      }
      default: {
        return read(buffer, buffer_length);
      }
    }
  }
  ++num_read;
  return bytes_read;
}

bool InputGRIBStream::open(string filename) {
  idstream::open(filename);
  if (is_open()) {
    if (ics != nullptr) {
      throw my::BadType_Error("COS-blocking must be removed from GRIB files.");
    }
    return true;
  }
  return false;
}

int InputGRIBStream::find_grib(unsigned char *buffer) {
  auto b = reinterpret_cast<char *>(buffer);
  b[0] = 'X';
#ifdef __WITH_S3
  int stat = 0;
  if (is3s != nullptr) {
    stat = is3s->read(buffer, 4);
  } else {
#endif
    fs.read(b, 4);
#ifdef __WITH_S3
  }
#endif
  while (b[0] != 'G' || b[1] != 'R' || b[2] != 'I' || b[3] != 'B') {
#ifdef __WITH_S3
    if (is3s != nullptr) {
      if (stat == bfstream::eof || stat == bfstream::error) {
        if (num_read == 0) {
          throw my::BadType_Error("file is not a GRIB file.");
        }
        return stat;
      }
    } else {
#endif
      if (fs.eof() || !fs.good()) {
        if (num_read == 0) {
          throw my::BadType_Error("file is not a GRIB file.");
        }
        return (fs.eof()) ? bfstream::eof : bfstream::error;
      }
#ifdef __WITH_S3
    }
#endif
    b[0] = b[1];
    b[1] = b[2];
    b[2] = b[3];
#ifdef __WITH_S3
    if (is3s != nullptr) {
      is3s->read(&buffer[3], 1);
    } else {
#endif
      fs.read(&b[3], 1);
#ifdef __WITH_S3
    }
#endif
  }
#ifdef __WITH_S3
  if (is3s != nullptr) {
    curr_offset = is3s->tell() - 4;
  } else {
#endif
    curr_offset = static_cast<off_t>(fs.tellg()) - 4;
#ifdef __WITH_S3
  }
#endif
  return 4;
}
