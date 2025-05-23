#include <algorithm>
#include <zlib.h>
#include <hdf.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>
#include <myerror.hpp>

using miscutils::min_byte_width;
using std::cerr;
using std::copy;
using std::cout;
using std::endl;
using std::find_if;
using std::list;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;
using strutils::append;
using strutils::chop;
using strutils::replace_all;
using strutils::split;
using unixutils::dump;

InputHDF5Stream::~InputHDF5Stream() {
  close();
}

void InputHDF5Stream::close() {
  if (fs.is_open()) {
    fs.close();
  }
  clear_groups(root_group);
  if (ref_table != nullptr) {
    ref_table.reset();
  }
}

InputHDF5Stream::Chunk::Chunk(long long addr, size_t sz, size_t len, const
    vector<size_t>& offs) : address(addr), buffer(nullptr), size(sz), length(
    len), offsets() {
  for (const auto& e : offs) {
    offsets.emplace_back(e);
  }
}

InputHDF5Stream::Chunk::~Chunk() {
  if (buffer != nullptr) {
    buffer.reset();
  }
}

InputHDF5Stream::Chunk& InputHDF5Stream::Chunk::operator=(const Chunk& source) {
  if (this == &source) {
    return *this;
  }
  address = source.address;
  size = source.size;
  length = source.length;
  if (source.buffer != nullptr) {
    allocate();
    copy(source.buffer.get(), source.buffer.get() + length, buffer.get());
  }
  for (const auto& e : source.offsets) {
    offsets.emplace_back(e);
  }
  return *this;
}

void InputHDF5Stream::Chunk::allocate() {
  if (length > 0) {
    buffer.reset(new unsigned char[length]);
  }
}

bool InputHDF5Stream::Chunk::fill(std::fstream& fs, const Dataset& dataset) {
  auto curr_off = fs.tellg();
  fs.seekg(address, std::ios_base::beg);
  if (dataset.filters.empty()) {
    allocate();
    fs.read(reinterpret_cast<char *>(buffer.get()), length);
    if (fs.gcount() != static_cast<int>(length)) {
      append(myerror, "error reading data chunk", ", ");
      return false;
    }
  } else {
#ifdef __DEBUG
    cerr << "applying filters..." << endl;
#endif
    auto buf = new unsigned char[size];
    fs.read(reinterpret_cast<char *>(buf), size);
    if (fs.gcount() != static_cast<int>(size)) {
      append(myerror, "error reading data chunk", ", ");
      return false;
    }
    for (size_t n = 0; n < dataset.filters.size(); ++n) {
      switch (dataset.filters[n]) {
        case 1: {

          // gzip
          allocate();
          auto status = uncompress(buffer.get(), &length, buf, size);
          if (status != 0) {
            append(myerror, "gunzip error: " + to_string(status), ", ");
            exit(1);
          }
/*
#ifdef __DEBUG
          cerr << "--GUNZIPPED-------" << endl;
          dump(buffer.get(), length);
          cerr << "------------------" << endl;
#endif
*/
          break;
        }
        case 2: {

          // shuffle
          if ( (length % dataset.data.size_of_element) != 0) {
            append(myerror, "can't unshuffle buffer size of " + to_string(
                length) + " and data element size of " + to_string(dataset.data.
                size_of_element), ", ");
            return false;
          }
          if (buffer == nullptr) {
            allocate();
            copy(buf, buf + size, buffer.get());
          }
          unsigned char *cbuf = new unsigned char[length];
          copy(buffer.get(), buffer.get() + length, cbuf);
          auto sl = length / dataset.data.size_of_element;
#ifdef __DEBUG
          cerr << "length: " << length << "  size of data element: " <<
              dataset.data.size_of_element << "  sl: " << sl << endl;
#endif
          for (size_t x = 1, y = 0; x < length - 1; ++x) {
            auto z = x % sl;
            if (z == 0) {
              ++y;
            }
            buffer[z * dataset.data.size_of_element + y] = cbuf[x];
          }
          delete[] cbuf;
/*
#ifdef __DEBUG
          cerr << "--UNSHUFFLED------" << endl;
          dump(buffer.get(), length);
          cerr << "------------------" << endl;
#endif
*/
          break;
        }
        case 3: {
          break;
        }
        default: {
          append(myerror, "unable to handle filter ID '" + to_string(dataset.
              filters[n]) + "'", ", ");
          return false;
        }
      }
    }
    delete[] buf;
  }
  fs.seekg(curr_off, std::ios_base::beg);
  return true;
}

InputHDF5Stream::Datatype::~Datatype() {
  if (properties != nullptr) {
    properties = nullptr;
  }
}

InputHDF5Stream::Datatype& InputHDF5Stream::Datatype::operator=(const Datatype&
    source) {
  if (this == &source) {
    return *this;
  }
  class_ = source.class_;
  version = source.version;
  bit_fields[0] = source.bit_fields[0];
  bit_fields[1] = source.bit_fields[1];
  bit_fields[2] = source.bit_fields[2];
  size = source.size;
  prop_len = source.prop_len;
  properties.reset(new unsigned char[prop_len]);
  copy(source.properties.get(), source.properties.get()+prop_len, properties.
      get());
  return *this;
}

InputHDF5Stream::DataValue& InputHDF5Stream::DataValue::operator=(const
    DataValue& source) {
  if (this == &source) {
    return *this;
  }
  _class_ = source._class_;
  precision_ = source.precision_;
  size = source.size;
  dim_sizes = source.dim_sizes;
  switch (_class_) {
    case 0:
    case 4: {
      size_t max_cnt = 1;
      if (dim_sizes.size() == 1) {
        max_cnt *= dim_sizes[0];
      }
      switch (precision_) {
        case 8: {
          array_type = ArrayType::BYTE;
          array = new unsigned char[max_cnt];
          for (size_t cnt = 0; cnt < max_cnt; ++cnt) {
            (reinterpret_cast<unsigned char *>(array))[cnt] = (reinterpret_cast<
                unsigned char *>(source.array))[cnt];
          }
          break;
        }
        case 16: {
          array_type = ArrayType::FLOAT;
          array = new float[max_cnt];
          for (size_t cnt = 0; cnt < max_cnt; ++cnt) {
            (reinterpret_cast<short *>(array))[cnt] = (reinterpret_cast<
                short *>(source.array))[cnt];
          }
          break;
        }
        case 32: {
          array_type = ArrayType::INT;
          array = new int[max_cnt];
          for (size_t cnt = 0; cnt < max_cnt; ++cnt) {
            (reinterpret_cast<int *>(array))[cnt] = (reinterpret_cast<int *>(
                source.array))[cnt];
          }
          break;
        }
        case 64: {
          array_type = ArrayType::LONG_LONG;
          array = new long long[max_cnt];
          for (size_t cnt = 0; cnt < max_cnt; ++cnt) {
            (reinterpret_cast<long long *>(array))[cnt] = (reinterpret_cast<
                long long *>(source.array))[cnt];
          }
          break;
        }
      }
      break;
    }
    case 1: {
      size_t max_cnt = 1;
      if (dim_sizes.size() == 1) {
        max_cnt *= dim_sizes[0];
      }
      switch (precision_) {
        case 32: {
          array_type = ArrayType::FLOAT;
          array = new float[max_cnt];
          for (size_t cnt = 0; cnt < max_cnt; ++cnt) {
            (reinterpret_cast<float *>(array))[cnt] = (reinterpret_cast<
                float *>(source.array))[cnt];
          }
          break;
        }
        case 64: {
          array_type = ArrayType::DOUBLE;
          array = new double[max_cnt];
          for (size_t cnt = 0; cnt < max_cnt; ++cnt) {
            (reinterpret_cast<double *>(array))[cnt] = (reinterpret_cast<
                double *>(source.array))[cnt];
          }
          break;
        }
      }
      break;
    }
    case 3: {
      array_type = ArrayType::STRING;
      array = new char[precision_ + 1];
      copy(reinterpret_cast<char *>(source.array), reinterpret_cast<char *>(
          source.array)+precision_, reinterpret_cast<char *>(array));
      (reinterpret_cast<char *>(array))[precision_] = '\0';
      break;
    }
    case 6: {
      compound.members.resize(source.compound.members.size());
      auto m = 0;
      for (size_t n = 0; n < compound.members.size(); ++n) {
        compound.members[n].name = source.compound.members[n].name;
        compound.members[n].class_ = source.compound.members[n].class_;
        compound.members[n].precision_ = source.compound.members[n].precision_;
        compound.members[n].size = source.compound.members[n].size;
        m += compound.members[n].size;
      }
      if (dim_sizes.size() > 0) {
        m *= dim_sizes[0];
      }
      array_type = ArrayType::BYTE;
      array = new unsigned char[m];
      copy(reinterpret_cast<unsigned char *>(source.array), reinterpret_cast<
          unsigned char *>(source.array) + m, reinterpret_cast<unsigned char *>(
          array));
      break;
    }
    case 9: {
      array_type = ArrayType::BYTE;
      array = new unsigned char[size+1];
      copy(reinterpret_cast<unsigned char *>(source.array), reinterpret_cast<
          unsigned char *>(source.array) + size + 1, reinterpret_cast<unsigned
          char *>(array));
      vlen.class_ = source.vlen.class_;
      if (source.vlen.size > 0) {
        vlen.size = source.vlen.size;
        vlen.buffer.reset(new unsigned char[vlen.size]);
        copy(&source.vlen.buffer[0], &source.vlen.buffer[vlen.size], vlen.
            buffer.get());
      } else {
        vlen.size = 0;
        vlen.buffer.reset(nullptr);
      }
      break;
    }
  }
  return *this;
}

void InputHDF5Stream::DataValue::allocate(ArrayType type, size_t array_length) {
  if (type != array_type || capacity < array_length) {
    clear();
    array_type = type;
    capacity = array_length;
    switch (array_type) {
      case ArrayType::BYTE: {
        array = new unsigned char[capacity];
        break;
      }
      case ArrayType::SHORT: {
        array = new short[capacity];
        break;
      }
      case ArrayType::INT: {
        array = new int[capacity];
        break;
      }
      case ArrayType::LONG_LONG: {
        array = new long long[capacity];
        break;
      }
      case ArrayType::FLOAT: {
        array = new float[capacity];
        break;
      }
      case ArrayType::DOUBLE: {
        array = new double[capacity];
        break;
      }
      case ArrayType::STRING: {
        array = new char[capacity];
        break;
      }
      default: {}
    }
  }
}

void InputHDF5Stream::DataValue::clear() {
  if (array != nullptr) {
    switch (_class_) {
      case 6: {
        if (compound.members.size() > 0) {
          compound.members.clear();
        }
        break;
      }
      case 9: {
        vlen.buffer.reset(nullptr);
        break;
      }
    }
    switch (array_type) {
      case ArrayType::BYTE: {
        delete[] reinterpret_cast<unsigned char *>(array);
        break;
      }
      case ArrayType::SHORT: {
        delete[] reinterpret_cast<short *>(array);
        break;
      }
      case ArrayType::INT: {
        delete[] reinterpret_cast<int *>(array);
        break;
      }
      case ArrayType::LONG_LONG: {
        delete[] reinterpret_cast<long long *>(array);
        break;
      }
      case ArrayType::FLOAT: {
        delete[] reinterpret_cast<float *>(array);
        break;
      }
      case ArrayType::DOUBLE: {
        delete[] reinterpret_cast<double *>(array);
        break;
      }
      case ArrayType::STRING: {
        delete[] reinterpret_cast<char *>(array);
        break;
      }
      default: {}
    }
    array = nullptr;
    array_type = ArrayType::_NULL;
  }
}

void InputHDF5Stream::DataValue::print(std::ostream& ofs, shared_ptr<
    unordered_map<size_t, string>> ref_table) const {
  if (array == nullptr) {
    ofs << "?????0 (" << _class_ << ")";
    return;
  }
  switch (_class_) {
    case 0:
      // fixed point numbers (integers)

    case 4: {
      // bitfield
      size_t max_cnt = 1;
      if (dim_sizes.size() == 1) {
        max_cnt *= dim_sizes[0];
      }
      for (size_t n = 0; n < max_cnt; ++n) {
        if (n > 0) {
          ofs << ", ";
        }
        switch (precision_) {
          case 8: {
            ofs << static_cast<int>((reinterpret_cast<unsigned char *>(array))[
                n]) << "d";
            break;
          }
          case 16: {
            ofs << (reinterpret_cast<short *>(array))[n] << "d";
            break;
          }
          case 32: {
            ofs << (reinterpret_cast<int *>(array))[n] << "d";
            break;
          }
          case 64: {
            ofs << (reinterpret_cast<long long *>(array))[n] << "d";
            break;
          }
          default: {
            ofs << "?????d";
          }
        }
      }
      break;
    }
    case 1: {

      // floating point numbers
      size_t max_cnt = 1;
      if (dim_sizes.size() == 1) {
        max_cnt *= dim_sizes[0];
      }
      ofs.setf(std::ios::fixed);
      for (size_t n = 0; n < max_cnt; ++n) {
        if (n > 0) {
          ofs << ", ";
        }
        switch (precision_) {
          case 32: {
            ofs << (reinterpret_cast<float *>(array))[n] << "f";
            break;
          }
          case 64: {
            ofs << (reinterpret_cast<double *>(array))[n] << "f";
            break;
          }
          default: {
            ofs << "????.?f";
          }
        }
      }
      break;
    }
    case 3: {

      // strings
      ofs << "\"" << reinterpret_cast<char *>(array) << "\"";
      break;
    }
    case 6: {
      DataValue *cval = new DataValue;
      ofs << "(";
      for (size_t n = 0; n < compound.members.size(); ++n) {
        if (n > 0) {
          ofs << ", ";
        }
        ofs << compound.members[n].name;
      }
      ofs << "): ";
      size_t end = 0;
      if (dim_sizes.size() > 0) {
        ofs << "[ ";
        end = dim_sizes[0];
      } else {
        end = 1;
      }
      auto off = 0;
      for (size_t n = 0; n < end; ++n) {
        if (n > 0) {
          ofs << ", ";
        }
        ofs << "(";
        for (size_t m = 0; m < compound.members.size(); ++m) {
          if (m > 0) {
            ofs << ", ";
          }
          cval->_class_ = compound.members[m].class_;
          cval->precision_ = compound.members[m].precision_;
          cval->size = compound.members[m].size;
          cval->array = &(reinterpret_cast<unsigned char *>(array))[off];
          cval->print(ofs, ref_table);
          off+=compound.members[m].size;
        }
        ofs << ")";
      }
      if (dim_sizes.size() > 0) {
        ofs << " ]";
      }
      cval->array = nullptr;
      delete cval;
      break;
    }
    case 7: {

      // reference
      if (ref_table != nullptr) {
        auto key=HDF5::value(reinterpret_cast<unsigned char *>(array), size);
        ofs << (*ref_table)[key];
      } else {
        ofs << HDF5::value(reinterpret_cast<unsigned char *>(array), size) <<
            "R";
      }
      break;
    }
    case 9: {

      // variable-length
      size_t end = 1;
      if (dim_sizes.size() > 0) {
        end = dim_sizes[0];
      }
      auto off = 0;
      switch (vlen.class_) {
        case 3: {
          for (size_t n = 0; n < end; ++n) {
            if (n > 0) {
              ofs << ", ";
            }
            int len;
            bits::get(&vlen.buffer[off], len, 0, 32);
            ofs << "\"" << string(reinterpret_cast<char *>(&vlen.buffer[off +
                4]),len) << "\"" << endl;
            off += 4 + len;
          }
          break;
        }
        case 7: {
          if (dim_sizes.size() > 0) {
            ofs << "[";
          }
          for (size_t n = 0; n < end; ++n) {
            if (n > 0) {
              ofs << ", ";
            }
            if (ref_table != nullptr) {
              auto key=HDF5::value(&vlen.buffer[off+4], precision_);
              ofs << (*ref_table)[key];
            }
            else {
              ofs << "(" << HDF5::value(&vlen.buffer[off + 4], precision_) <<
                  "R";
            }
            off += 4 + precision_;
          }
          if (dim_sizes.size() > 0) {
            ofs << "]";
          }
          break;
        }
      }
      break;
    }
    default: {
      ofs << "?????u (" << _class_ << ")";
    }
  }
}

bool InputHDF5Stream::DataValue::set(std::fstream& fs, unsigned char *buffer,
    short size_of_offsets, short size_of_lengths, const Datatype& datatype,
    const Dataspace& dataspace) {
  _class_ = datatype.class_;
  dim_sizes = dataspace.sizes;
#ifdef __DEBUG
  cerr << "class: " << _class_ << " dim_sizes size: " << dim_sizes.size() <<
      endl;
  for (auto& dim_size : dim_sizes) {
    cerr << "  dim_size: " << dim_size << endl;
  }
#endif
  switch (_class_) {
    case 0:

      // fixed point numbers (integers)
    case 4: {

      // bitfield
      short byte_order[2];
      bits::get(datatype.bit_fields, byte_order[0], 7, 1);
      short lo_pad;
      bits::get(datatype.bit_fields, lo_pad, 6, 1);
      short hi_pad;
      bits::get(datatype.bit_fields, hi_pad, 5, 1);
      short sign;
      if (_class_ == 0) {
        bits::get(datatype.bit_fields, sign, 4, 1);
      }
      auto off = HDF5::value(&datatype.properties[0], 2);
      precision_ = HDF5::value(&datatype.properties[2], 2);
      size_t max_cnt = 1;
      if (dim_sizes.size() == 1) {
        max_cnt *= dim_sizes[0];
      }
      switch (byte_order[0]) {
        case 0: {
          if (off == 0) {
            if ( (precision_ % 8) == 0) {
              auto byte_len = precision_ / 8;
              for (size_t n = 0; n < max_cnt; ++n) {
                switch (byte_len) {
                  case 1: {
                    if (n == 0) {
                      allocate(ArrayType::BYTE, max_cnt);
                    }
                    (reinterpret_cast<unsigned char *>(array))[n] = HDF5::value(
                        &buffer[byte_len*n], 1);
                    break;
                  }
                  case 2: {
                    if (n == 0) {
                      allocate(ArrayType::SHORT, max_cnt);
                    }
                    (reinterpret_cast<short *>(array))[n] = HDF5::value(&buffer[
                        byte_len*n], 2);
                    break;
                  }
                  case 4: {
                    if (n == 0) {
                      allocate(ArrayType::INT, max_cnt);
                    }
                    (reinterpret_cast<int *>(array))[n] = HDF5::value(&buffer[
                        byte_len*n], 4);
                    break;
                  }
                  case 8: {
                    if (n == 0) {
                      allocate(ArrayType::LONG_LONG, max_cnt);
                    }
                    (reinterpret_cast<long long *>(array))[n] = HDF5::value(
                        &buffer[byte_len*n], 8);
                    break;
                  }
                }
              }
              size = byte_len * max_cnt;
            } else {
              append(myerror, "unable to decode little-endian integer with "
                  "precision " + to_string(precision_), ", ");
              return false;
            }
          } else {
            append(myerror, "unable to decode little-endian integer with bit "
                "offset " + to_string(off), ", ");
            return false;
          }
          break;
        }
        case 1: {
          if (off == 0) {
            if ( (precision_ % 8) == 0) {
              auto byte_len = precision_ / 8;
              for (size_t n = 0; n < max_cnt; ++n) {
                switch (byte_len) {
                  case 1: {
                    if (n == 0) {
                      allocate(ArrayType::BYTE, max_cnt);
                    }
                    bits::get(&buffer[byte_len*n], (reinterpret_cast<unsigned
                        char *>(array))[n], 0, precision_);
                    break;
                  }
                  case 2: {
                    if (n == 0) {
                      allocate(ArrayType::SHORT, max_cnt);
                    }
                    bits::get(&buffer[byte_len*n], (reinterpret_cast<short *>(
                        array))[n], 0, precision_);
                    break;
                  }
                  case 4: {
                    if (n == 0) {
                      allocate(ArrayType::INT, max_cnt);
                    }
                    bits::get(&buffer[byte_len*n], (reinterpret_cast<int *>(
                        array))[n], 0, precision_);
                    break;
                  }
                  case 8: {
                    if (n == 0) {
                      allocate(ArrayType::LONG_LONG, max_cnt);
                    }
                    bits::get(&buffer[byte_len*n], (reinterpret_cast<long
                        long *>(array))[n], 0, precision_);
                    break;
                  }
                }
              }
              size = byte_len * max_cnt;
            } else {
              append(myerror, "unable to decode big-endian integer with "
                  "precision " + to_string(precision_), ", ");
              return false;
            }
          } else {
            append(myerror, "unable to decode big-endian integer with bit "
                "offset " + to_string(off), ", ");
            return false;
          }
          break;
        }
      }
      break;
    }
    case 1: {

      // floating-point numbers
      short byte_order[2];
      bits::get(datatype.bit_fields, byte_order[0], 1, 1);
      bits::get(datatype.bit_fields, byte_order[1], 7, 1);
      short lo_pad;
      bits::get(datatype.bit_fields, lo_pad, 6, 1);
      short hi_pad;
      bits::get(datatype.bit_fields, hi_pad, 5, 1);
      short int_pad;
      bits::get(datatype.bit_fields, int_pad, 4, 1);
      short mant_norm;
      bits::get(datatype.bit_fields, mant_norm, 2, 2);
      precision_ = HDF5::value(&datatype.properties[2], 2);
      size_t max_cnt = 1;
      if (dim_sizes.size() == 1) {
        max_cnt *= dim_sizes[0];
      }
      auto sign_offset = precision_ - static_cast<int>(datatype.bit_fields[1]) -
          1;
      auto exp_offset = precision_ - static_cast<int>(datatype.properties[4]) -
          static_cast<int>(datatype.properties[5]);
      auto exp_length = static_cast<int>(datatype.properties[5]);
      auto exp_bias = HDF5::value(&datatype.properties[8], 4);
      auto mant_offset = precision_ - static_cast<int>(datatype.properties[6]) -
          static_cast<int>(datatype.properties[7]);
      auto mant_length = static_cast<int>(datatype.properties[7]);
      size_t byte_len = precision_ / 8;
      switch (precision_) {
        case 32: {
          allocate(ArrayType::FLOAT, max_cnt);
          size = sizeof(float) * max_cnt;
          break;
        }
        case 64: {
          allocate(ArrayType::DOUBLE, max_cnt);
          size = sizeof(double) * max_cnt;
          break;
        }
      }
      for (size_t n = 0; n < max_cnt; ++n) {
        unsigned char v[8];
        auto do_unpack = true;
        if (byte_order[0] == 0 && byte_order[1] == 0) {

          // little-endian
#ifdef __DEBUG
          cerr << "#" << n << " of " << max_cnt << ", floating-point, "
              "little-endian, precision: " << precision_ << endl;
#endif
          if (!unixutils::system_is_big_endian()) {
            switch (precision_) {
              case 32: {
                (reinterpret_cast<float *>(array))[n] = *(reinterpret_cast<
                    float *>(&buffer[byte_len*n]));
                break;
              }
              case 64: {
                (reinterpret_cast<double *>(array))[n] = *(reinterpret_cast<
                    double *>(&buffer[byte_len*n]));
                break;
              }
            }
            do_unpack = false;
          } else {
            auto buffer_offset = byte_len * n + byte_len;
            for (size_t m = 0; m < byte_len; ++m) {
              v[m] = buffer[--buffer_offset];
            }
          }
        } else if (byte_order[0] == 0 && byte_order[1] == 1) {

          // big-endian
#ifdef __DEBUG
          cerr << "#" << n << " of " << max_cnt << ", floating-point, "
              "big-endian" << endl;
#endif
          if (unixutils::system_is_big_endian()) {
            switch (precision_) {
              case 32: {
                (reinterpret_cast<float *>(array))[n] = *(reinterpret_cast<
                    float *>(&buffer[byte_len*n]));
                break;
              }
              case 64: {
                (reinterpret_cast<double *>(array))[n] = *(reinterpret_cast<
                    double *>(&buffer[byte_len*n]));
                break;
              }
              do_unpack = false;
            }
          } else {
            auto start = byte_len * n;
            copy(&buffer[start], &buffer[start+byte_len], v);
          }
        } else {
          append(myerror, "unknown byte order for data value", ", ");
          return false;
        }
        if (do_unpack) {
          if (HDF5::value(&datatype.properties[0], 2) == 0) {
            long long exp = 0;
            bits::get(reinterpret_cast<unsigned char *>(v), exp, exp_offset,
                exp_length);
            long long mant = 0;
            bits::get(reinterpret_cast<unsigned char *>(v), mant, mant_offset,
                mant_length);
            short sign;
            bits::get(reinterpret_cast<unsigned char *>(v), sign, sign_offset,
                1);
            if (mant == 0 && (mant_norm != 2 || exp == 0)) {
              switch (precision_) {
                case 32: {
                  (reinterpret_cast<float *>(array))[n] = 0.;
                  break;
                }
                case 64: {
                  (reinterpret_cast<double *>(array))[n] = 0.;
                  break;
                }
              }
            } else {
              exp -= exp_bias;
              if (mant_norm == 2) {
                switch (precision_) {
                  case 32: {
                    (reinterpret_cast<float *>(array))[n] = pow(-1., sign) * (1
                        + pow(2., -static_cast<double>(datatype.properties[7]))
                        * mant) * pow(2., exp);
                    break;
                  }
                  case 64: {
                    (reinterpret_cast<double *>(array))[n] = pow(-1., sign) * (1
                        + pow(2., -static_cast<double>(datatype.properties[7]))
                        * mant) * pow(2., exp);
                    break;
                  }
                }
              }
            }
          }
        }
      }
      break;
    }
    case 3: {

      // strings
      if (dataspace.dimensionality < 0) {
        precision_ = 0;
      } else {
        precision_ = datatype.size;
      }
      size = precision_;
      allocate(ArrayType::STRING, precision_+1);
      bits::get(buffer, (reinterpret_cast<char *>(array)), 0, 8, 0, precision_);
      (reinterpret_cast<char *>(array))[precision_] = '\0';
      break;
    }
    case 6: {

      // compound
#ifdef __DEBUG
      cerr << "Setting value for compound type - version: " << datatype.version
          << endl;
#endif
      compound.members.resize(HDF5::value(datatype.bit_fields, 2));
#ifdef __DEBUG
      cerr << "  number of members: " << compound.members.size() << " " <<
          datatype.prop_len << endl;
#endif
      std::unique_ptr<Datatype[]> l_datatypes;
      l_datatypes.reset(new Datatype[compound.members.size()]);
      auto total_size = 0;
      switch (datatype.version) {
        case 1: {
          auto off = 0;
          for (size_t n = 0; n < compound.members.size(); ++n) {
            compound.members[n].name = &(reinterpret_cast<char *>(datatype.
                properties.get()))[off];
#ifdef __DEBUG
            cerr << "  name: " << compound.members[n].name << " at offset " <<
                off << endl;
#endif
            auto len = compound.members[n].name.length();
            off += len;
            while ( (len++ % 8) > 0) {
              ++off;
            }
            off += 4;
            if (static_cast<int>(datatype.properties[off]) != 0) {
              append(myerror, "unable to decode compound type version " +
                  to_string(datatype.version) + " with dimensionality " +
                  to_string(static_cast<int>(datatype.properties[off])), ", ");
              exit(1);
            }
            off += 28;
            HDF5::decode_datatype(&datatype.properties[off], l_datatypes[n]);
            compound.members[n].class_ = l_datatypes[n].class_;
            compound.members[n].size = l_datatypes[n].size;
            total_size += l_datatypes[n].size;
            off += 8 + l_datatypes[n].prop_len;
          }
          break;
        }
        case 3: {
          if (dim_sizes.size() > 1) {
            append(myerror, "unable to decode compound type with "
                "dimensionality " + to_string(dim_sizes.size()), ", ");
            exit(1);
          }
          auto off = 0;
          for (size_t n = 0; n < compound.members.size(); ++n) {
            compound.members[n].name = &(reinterpret_cast<char *>(datatype.
                properties.get()))[off];
#ifdef __DEBUG
            cerr << "  name: " << compound.members[n].name << " at offset " <<
                off << endl;
#endif
            while (datatype.properties[off++] != 0) { }
            auto len = 1;
            auto s = datatype.size;
            while ( (s / 256) > 0) {
              ++len;
              s /= 256;
            }
            off += len;
            HDF5::decode_datatype(&datatype.properties[off], l_datatypes[n]);
            compound.members[n].class_ = l_datatypes[n].class_;
            compound.members[n].size = l_datatypes[n].size;
            total_size += l_datatypes[n].size;
            off += 8 + l_datatypes[n].prop_len;
          }
          break;
        }
        default: {
          append(myerror, "unable to decode compound type version " + to_string(
              datatype.version), ", ");
          return false;
        }
      }
      array = (!dim_sizes.empty()) ? new unsigned char[total_size*dim_sizes[0]]
          : new unsigned char[total_size];
#ifdef __DEBUG
      cerr << "num values: " << compound.members.size() << endl;
#endif
      auto off = 0;
      auto voff = off;
      auto end = (!dim_sizes.empty()) ? dim_sizes[0] : 1;
      for (size_t n = 0; n < end; ++n) {
        auto roff = off;
        for (size_t m = 0; m < compound.members.size(); ++m) {
          DataValue l_value;
          l_value.set(fs, &buffer[roff], size_of_offsets, size_of_lengths,
              l_datatypes[m], dataspace);
          if (n == 0) {
            compound.members[m].precision_ = l_value.precision_;
          }
          copy(reinterpret_cast<unsigned char *>(l_value.array),
              reinterpret_cast<unsigned char *>(l_value.array) + l_datatypes[m].
              size, &(reinterpret_cast<unsigned char *>(array))[voff]);
          roff += l_datatypes[m].size;
          voff += l_datatypes[m].size;
        }
        off += datatype.size;
      }
      l_datatypes = nullptr;
      size = off;
      break;
    }
    case 7: {

      // reference
      auto type = ((datatype.bit_fields[0] & 0xf0) >> 4);
      if (type == 0 || type == 1) {
        size = size_of_offsets;
        allocate(ArrayType::BYTE, size);
        copy(buffer, buffer+size, reinterpret_cast<unsigned char *>(array));
      } else {
        append(myerror, "unable to set class 7 value for bits 0-3=" + to_string(
            type), ", ");
        return false;
      }
      break;
    }
    case 9: {

      // variable-length
      short type, pad, charset;
      bits::get(datatype.bit_fields, type, 0, 4);
      bits::get(datatype.bit_fields, pad, 4, 4);
      bits::get(datatype.bit_fields, charset, 8, 4);
#ifdef __DEBUG
      cerr << "decoding data class 9 -  type: " << type << " pad: " << pad <<
          " charset: " << charset << " size: " << datatype.size << " base type "
          "class and version: " << static_cast<int>(datatype.properties[0] &
          0xf) << "/" << static_cast<int>((datatype.properties[0] & 0xf0) >> 4)
          << endl;
#endif
      if (type == 0) {
        size = datatype.size;
        precision_ = size_of_offsets;
        size_t num_pointers = 1;
        if (!dim_sizes.empty()) {      
          num_pointers = dim_sizes.front();
          size *= dim_sizes.front();
        }
        allocate(ArrayType::BYTE, size);
        vlen.class_ = (datatype.properties[0] & 0xf);
        auto element_size = HDF5::value(&datatype.properties[4], 4);

        // patch for strings stored as sequence of fixed-point numbers
        if (vlen.class_ == 0 && datatype.size > 1 && element_size == 1) {
          vlen.class_ = 3;
#ifdef __DEBUG
          cerr << "***BASE CLASS changed from 'fixed-point number' to 'string'"
              << endl;
#endif
        }
        vlen.size = 0;
        auto off = 0;
        vector<int> lengths;
        for (size_t n = 0; n < num_pointers; ++n) {
          lengths.emplace_back(HDF5::value(&buffer[off], 4) * element_size);
          vlen.size += lengths.back();
          off += 8 + precision_;
        }
        vlen.size += 4 * num_pointers;
        vlen.buffer.reset(new unsigned char[vlen.size]);
#ifdef __DEBUG
        cerr << "Variable length (class " << vlen.class_ << ") buffer set to "
            << vlen.size << " bytes" << endl;
#endif
        off = 0;
        auto voff = 0;
        for (size_t n = 0; n < num_pointers; ++n) {
          bits::set(&vlen.buffer[voff], lengths[n], 0, 32);
          unsigned char *buf2 = nullptr;
          auto len = HDF5::global_heap_object(fs, size_of_lengths, HDF5::value(
              &buffer[off+4], precision_), HDF5::value(&buffer[
              off+4+precision_], 4), &buf2);
          copy(&buf2[0], &buf2[lengths[n]], &vlen.buffer[voff+4]);
#ifdef __DEBUG
          cerr << "***LEN=" << len;
          if (vlen.class_ == 7) {
            cerr << " " << HDF5::value(buf2,size_of_offsets);
          }
          cerr << endl;
#endif
          if (len > 0) {
            delete[] buf2;
          }
          off += 8 + precision_;
          voff += 4 + lengths[n];
        }
        copy(&buffer[0], &buffer[off], reinterpret_cast<unsigned char *>(
            array));
      } else {
        append(myerror, "unable to set class 9 value for type " + to_string(
            type), ", ");
        return false;
      }
      break;
    }
    default: {
      append(myerror, "unable to decode data of class " + to_string(datatype.
          class_), ", ");
      return false;
    }
  }
  return true;
}

InputHDF5Stream::Attribute InputHDF5Stream::attribute(string xpath) {
  auto sp = split(xpath, "@");
  if (sp.size() != 2) {
    return Attribute();
  }
  auto name = sp[1];
  sp = split(xpath, "/");
  auto g = &root_group;
  for (size_t n = 1; n < sp.size() - 1; ++n) {
    auto it = find_if(g->groups.begin(), g->groups.end(), FindGroup(sp[n]));
    if (it == g->groups.end()) {
      return Attribute();
    }
    g = it->p_g.get();
  }
  auto key = sp.back();
  replace_all(key, "@" + name, "");
  auto it = find_if(g->datasets.begin(), g->datasets.end(), FindDataset(key));
  if (it == g->datasets.end()) {
    return Attribute();
  }
  return *(it->p_ds->attributes.find(name));
}

void InputHDF5Stream::Dataset::free()
{
  for (auto& chunk : data.chunks) {
    if (chunk.buffer != nullptr) {
      chunk.buffer.reset(nullptr);
    }
  }
}

shared_ptr<InputHDF5Stream::Dataset> InputHDF5Stream::dataset(string xpath) {
  auto g = &root_group;
  auto sp = split(xpath,"/");
  for (size_t n = 1; n < sp.size() - 1; ++n) {
    auto it = find_if(g->groups.begin(), g->groups.end(), FindGroup(sp[n]));
    if (it == g->groups.end()) {
      return nullptr;
    }
    g = it->p_g.get();
  }
  auto it = find_if(g->datasets.begin(), g->datasets.end(), FindDataset(
      sp.back()));
  if (it == g->datasets.end()) {
    return nullptr;
  }
  for (size_t n = 0; n < it->p_ds->data.chunks.size(); ++n) {
    if (it->p_ds->data.chunks[n].buffer == nullptr) {
      if (!it->p_ds->data.chunks[n].fill(fs, *it->p_ds)) {
        exit(1);
      }
    }
  }
  return it->p_ds;
}

list<InputHDF5Stream::DatasetEntry> InputHDF5Stream::datasets_with_attribute(
    string attribute_path, Group *g) {
  list<DatasetEntry> lst; // return value
  auto sp = split(attribute_path, "=");
  auto name = sp[0];
  string value;
  if (sp.size() > 1) {
    value = sp[1];
  }
  if (g == nullptr) {
    g = &root_group;
  }
  for (const auto& e : g->groups) {
    auto l = datasets_with_attribute(attribute_path, e.p_g.get());
    lst.insert(lst.end(), l.begin(), l.end());
  }
  for (const auto& e : g->datasets) {
    for (auto a : e.p_ds->attributes) {
      if (a.first == name) {
        if (value.empty()) {
          lst.emplace_back(e);
        } else {
          auto it = e.p_ds->attributes.find(a.first);
          if (it != e.p_ds->attributes.end()) {
            auto& attribute_value = it->second;
            switch (attribute_value._class_) {
              case 3: {
                if (string(reinterpret_cast<char *>(attribute_value.array)) ==
                    value) {
                  lst.emplace_back(e);
                }
                break;
              }
              case 9: {
                switch (attribute_value.vlen.class_) {
                  case 3: {
                    int len;
                    bits::get(attribute_value.vlen.buffer.get(), len, 0, 32);
                    if (string(reinterpret_cast<char *>(&attribute_value.vlen.
                        buffer[4]),len) == value) {
                      lst.emplace_back(e);
                    }
                    break;
                  }
                  default: {
                    append(myerror, "can't match a variable-length attribute "
                        "value of class " + to_string(attribute_value.vlen.
                        class_) + " (dataset: " + e.key + ", attribute: " + it->
                        first + ", value: '" + value + "')", ", ");
                  }
                }
                break;
              }
              default: {
                append(myerror, "can't match an attribute value of class " +
                    to_string(attribute_value._class_), ", ");
              }
            }
          }
        }
        break;
      }
    }
  }
  return lst;
}

bool InputHDF5Stream::open(const char *filename) {
  if (is_open()) {
    append(myerror, "currently connected to another file stream", ", ");
    return false;
  }
  fs.open(filename,std::ios::in);
  if (!fs.is_open()) {
    append(myerror, "unable to open " + string(filename), ", ");
    return false;
  }
  SymbolTableEntry ste;
  if (!decode_superblock(ste.objhdr_addr)) {
    return false;
  }
  if (sb_version == 0 || sb_version == 1) {
    if (!decode_symbol_table_entry(ste,NULL))
      return false;
  }
  if (!decode_object_header(ste,NULL)) {
    return false;
  }
#ifdef __DEBUG
  cerr << "ROOT GROUP heap address: " << root_group.local_heap.addr << endl;
  cerr << "ROOT GROUP heap data start address: " << root_group.local_heap.data_start << endl;
  cerr << "ROOT GROUP b-tree address: " << root_group.btree_addr << endl;
#endif
  free_space_manager.space_map.reserve(1024);
  return true;
}

int InputHDF5Stream::peek()
{
return bfstream::error;
}

void InputHDF5Stream::print_fill_value(string xpath) {
  auto sp = split(xpath, "/");
  auto g=&root_group;
  for (size_t n = 1; n < sp.size() - 1; ++n) {
    auto it = find_if(g->groups.begin(), g->groups.end(), FindGroup(sp[n]));
    if (it == g->groups.end()) {
      cout << "?????";
      return;
    }
    g = it->p_g.get();
  }
  auto it = find_if(g->datasets.begin(), g->datasets.end(), FindDataset(sp.
      back()));
  if (it == g->datasets.end()) {
    cout << "?????";
    return;
  }
  if (it->p_ds->fillvalue.bytes != nullptr) {
    HDF5::print_data_value(it->p_ds->datatype, it->p_ds->fillvalue.bytes);
  } else {
    cout << "Fillvalue(s) not defined";
  }
}

int InputHDF5Stream::read(unsigned char *buffer, size_t buffer_length) {
return bfstream::error;
}

void InputHDF5Stream::show_file_structure() {
  print_a_group_tree(root_group);
}

void InputHDF5Stream::clear_groups(Group& group) {
  for (auto& e : group.groups) {
    clear_groups(*e.p_g);
    e.p_g.reset();
  }
  group.groups.clear();
  for (auto& e : group.datasets) {
    if (e.p_ds != nullptr) {
      if (e.p_ds->fillvalue.bytes != nullptr) {
        delete[] e.p_ds->fillvalue.bytes;
      }
      e.p_ds.reset();
    }
  }
  group.datasets.clear();
}

bool InputHDF5Stream::decode_attribute(unsigned char *buffer, Attribute&
    attribute, int& length, int ohdr_version) {
  struct {
    int name, datatype, dataspace;
  } l_sizes;
  l_sizes.name = HDF5::value(&buffer[2], 2);
  if (l_sizes.name == 0) {
    return false;
  }
  l_sizes.datatype = HDF5::value(&buffer[4], 2);
  if (l_sizes.datatype == 0) {
    return false;
  }
  l_sizes.dataspace = HDF5::value(&buffer[6], 2);
  if (l_sizes.dataspace == 0) {
    return false;
  }
  length = 8;
#ifdef __DEBUG
  cerr << "decode attribute version " << static_cast<int>(buffer[0]) << " name "
      "size: " << l_sizes.name << " datatype size: " << l_sizes.datatype <<
      " dataspace size: " << l_sizes.dataspace << " ohdr_version: " <<
      ohdr_version << endl;
#endif
  int name_off;
  switch (static_cast<int>(buffer[0])) {
    case 1: {
      name_off = length;
      length += l_sizes.name;
      int mod = l_sizes.name % 8;
      if (mod > 0) {
        length += 8 - mod;
      }
      Datatype datatype;
      if (!HDF5::decode_datatype(&buffer[length], datatype)) {
        return false;
      }
      length += l_sizes.datatype;
      mod = l_sizes.datatype % 8;
      if (mod > 0) {
        length += 8 - mod;
      }
      Dataspace dataspace;
      if (!HDF5::decode_dataspace(&buffer[length], sizes.lengths, dataspace)) {
        return false;
      }
      length += l_sizes.dataspace;
      mod = l_sizes.dataspace % 8;
      if (mod > 0) {
        length += 8 - mod;
      }
      if (!attribute.second.set(fs, &buffer[length], sizes.offsets, sizes.
          lengths, datatype, dataspace)) {
        return false;
      }
      length += attribute.second.size;
      if (ohdr_version == 1 && datatype.class_ == 3) {
        mod = length % 8;
        if (mod != 0) {
          length += 8 - mod;
#ifdef __DEBUG
          cerr << "string length adjusted from " << attribute.second.size <<
              " to " << (attribute.second.size+8-mod) << endl;
#endif
        }
      }
      break;
    }
    case 2: {
      name_off = length;
      length += l_sizes.name;
      Datatype datatype;
      if (!HDF5::decode_datatype(&buffer[length], datatype)) {
        return false;
      }
      length += l_sizes.datatype;
      Dataspace dataspace;
      if (!HDF5::decode_dataspace(&buffer[length], sizes.lengths, dataspace)) {
        return false;
      }
      length += l_sizes.dataspace;
      if (!attribute.second.set(fs, &buffer[length], sizes.offsets, sizes.
          lengths, datatype, dataspace)) {
        return false;
      }
      length += attribute.second.size;
      break;
    }
    case 3: {
      length++;
      name_off = length;
      length += l_sizes.name;
      Datatype datatype;
      if (!HDF5::decode_datatype(&buffer[length], datatype)) {
        return false;
      }
      length += l_sizes.datatype;
#ifdef __DEBUG
      cerr << "C offset: " << length << endl;
#endif
      Dataspace dataspace;
      if (!HDF5::decode_dataspace(&buffer[length], sizes.lengths, dataspace)) {
        return false;
      }
      length += l_sizes.dataspace;
      if (dataspace.dimensionality > 1) {
        append(myerror, "unable to decode attribute with dimensionality " +
            to_string(dataspace.dimensionality), ", ");
        return false;
      }
#ifdef __DEBUG
      cerr << "setting value at offset: " << length << endl;
#endif
      if (!attribute.second.set(fs, &buffer[length], sizes.offsets, sizes.
          lengths, datatype, dataspace)) {
        return false;
      }
      length += attribute.second.size;
      break;
    }
    default: {
      return false;
    }
  }
/*
#ifdef __DEBUG
  cerr << "value set: ";
  attribute.second.print(cerr, ref_table);
  cerr << endl;
#endif
*/
  attribute.first.assign(reinterpret_cast<char *>(&buffer[name_off]), l_sizes.
      name);
  if (attribute.first.back() == '\0') {
    attribute.first.pop_back();
  }
  return true;
}

bool InputHDF5Stream::decode_Btree(Group& group, FractalHeapData *frhp_data) {
#ifdef __DEBUG
  cerr << "seeking " << group.btree_addr << endl;
#endif
  fs.seekg(group.btree_addr, std::ios_base::beg);
  char buf[4];
  fs.read(buf, 4);
  if (fs.gcount() != 4) {
    append(myerror, "read error tree signature", ", ");
    return false;
  }
  auto signature = string(buf, 4);
  if (signature == "TREE") {
    return decode_v1_Btree(group);
  } else if (signature == "BTHD") {
    return decode_v2_Btree(group, frhp_data);
  } else {
    append(myerror, "unrecognized tree signature '" + string(buf, 4) + "'",
        ", ");
  }
  return false;
}

bool InputHDF5Stream::decode_indirect_data(FractalHeapData& frhp_data) {
  auto off_size = min_byte_width(frhp_data.max_size);
  std::streamsize bytes_to_read = off_size + 6;
  unsigned char buf[64];
  char *cbuf = reinterpret_cast<char *>(buf);
  fs.read(cbuf, bytes_to_read);
  if (fs.gcount() != bytes_to_read) {
    append(myerror, "read error for indirect data", ", ");
    return false;
  }
#ifdef __DEBUG
  auto off = HDF5::value(&buf[0], off_size);
  cerr << "    indirect offset: " << off << endl;
#endif
#ifdef __DEBUG
  auto start_row = HDF5::value(&buf[off_size], 2);
  cerr << "    indirect start row: " << start_row << endl;
#endif
#ifdef __DEBUG
  auto start_column = HDF5::value(&buf[off_size + 2], 2);
  cerr << "    indirect start column: " << start_column << endl;
#endif
#ifdef __DEBUG
  auto nblocks = HDF5::value(&buf[off_size + 4], 2);
  cerr << "    indirect number of blocks: " << nblocks << endl;
#endif
  return true;
}

bool InputHDF5Stream::decode_free_space_section_list(FractalHeapData&
    frhp_data) {
  if (free_space_manager.list_addr == undef_addr) {
    append(myerror, "free space section list address is not defined", ", ");
    return false;
  }
  auto curr_off = fs.tellg();
  fs.seekg(free_space_manager.list_addr, std::ios_base::beg);
  unsigned char buf[64];
  char *cbuf = reinterpret_cast<char *>(buf);
  fs.read(cbuf, 5);
  if (fs.gcount() != 5) {
    append(myerror, "read error bytes 1-5 in free space section list", ", ");
    return false;
  }
  if (string(reinterpret_cast<char *>(buf), 4) != "FSSE") {
    append(myerror, "not a free space section list", ", ");
    return false;
  }
  if (buf[4] != 0) {
    append(myerror, "unable to decode free space section list version " +
        to_string(static_cast<int>(buf[4])), ", ");
    return false;
  }
#ifdef __DEBUG
  unsigned char buf2[160];
  char *cbuf2 = reinterpret_cast<char *>(buf2);
  fs.seekg(-5, std::ios_base::cur);
  fs.read(cbuf2, 160);
  dump(buf2, 160);
  fs.seekg(-155, std::ios_base::cur);
#endif
  fs.read(cbuf, sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.offsets)) {
    append(myerror, "read error for free space manager header address", ", ");
    return false;
  }
  auto addr = HDF5::value(&buf[0], sizes.offsets);
#ifdef __DEBUG
  cerr << "free space manager header address (from section list): " << addr <<
      endl;
#endif
  if (addr != frhp_data.space.manager_addr) {
    append(myerror, "free space manager header address in free space secton "
        "list does not match address in fractal heap", ", ");
    exit(1);
  }
  for (size_t sec_num = 0; sec_num < free_space_manager.num_serialized; ) {
#ifdef __DEBUG
    cerr << "set #" << sec_num+1 << " of " << free_space_manager.num_serialized
        << endl;
#endif
    auto nsec_len = min_byte_width(free_space_manager.num_serialized);
    auto sec_size_len = min_byte_width(free_space_manager.max_size);
#ifdef __DEBUG
    cerr << "lengths: # records: " << nsec_len << ", section size: " <<
        sec_size_len << endl;
#endif
    std::streamsize bytes_to_read = nsec_len + sec_size_len;
    fs.read(cbuf, bytes_to_read);
    if (fs.gcount() != bytes_to_read) {
      append(myerror, "read error for set #" + to_string(sec_num+1) + " record "
          "data", ", ");
      return false;
    }
    auto nsec = HDF5::value(&buf[0], nsec_len);
#ifdef __DEBUG
    cerr << "  number of section records: " << nsec << endl;
#endif
    auto sec_size = HDF5::value(&buf[nsec_len], sec_size_len);
#ifdef __DEBUG
    cerr << "  section size for all records (bytes): " << sec_size << endl;
#endif
    for (size_t k = 0; k < nsec; ++k) {
      auto off_len = (free_space_manager.addr_size + 7) / 8;
      bytes_to_read = off_len + 1;
      fs.read(cbuf, bytes_to_read);
      if (fs.gcount() != bytes_to_read) {
        append(myerror, "read error for set #" + to_string(sec_num+1) +
            " record data", ", ");
        return false;
      }
      auto off = HDF5::value(&buf[0], off_len);
#ifdef __DEBUG
      cerr << "  section #" << k << " offset length: " << off_len << ", record "
          "offset (bytes): " << off << endl;
#endif
      auto typ = HDF5::value(&buf[off_len], 1);
#ifdef __DEBUG
      cerr << "  section #" << k << " record type: " << typ << endl;
#endif
      switch (typ) {
        case 1:
        case 3: {
          if (!decode_indirect_data(frhp_data)) {
            exit(1);
          }
          break;
        }
        default: {
#ifdef __DEBUG
          if (free_space_manager.space_map.find(off-22) != free_space_manager.
              space_map.end()) {
            cerr << "WARNING! free space at offset " << off-22 << " is already "
                "marked" << endl;
          }
#endif
          free_space_manager.space_map.emplace(off-22, sec_size);
#ifdef __DEBUG
          cerr << "free space at offset " << off-22 << " of size " << sec_size
              << "  map: " << &free_space_manager.space_map << "  map size: " <<
              free_space_manager.space_map.size() << endl;
#endif
        }
      }
      ++sec_num;
    }
  }
  fs.seekg(curr_off, std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decode_free_space_manager(FractalHeapData& frhp_data) {
  if (frhp_data.space.manager_addr == undef_addr) {
    append(myerror, "free space manager address is not defined", ", ");
    return false;
  }
  auto curr_off = fs.tellg();
  fs.seekg(frhp_data.space.manager_addr, std::ios_base::beg);
  unsigned char buf[64];
  char *cbuf = reinterpret_cast<char *>(buf);
  fs.read(cbuf, 6);
  if (fs.gcount() != 6) {
    append(myerror, "read error bytes 1-6 in free space manager", ", ");
    return false;
  }
  if (string(reinterpret_cast<char *>(buf), 4) != "FSHD") {
    append(myerror, "not a free space manager header", ", ");
    return false;
  }
  if (buf[4] != 0) {
    append(myerror, "unable to decode free space manager header version " +
        to_string(static_cast<int>(buf[4])), ", ");
    return false;
  }
  if (buf[5] != 0) {
    append(myerror, "client ID is not a fractal heap", ", ");
    return false;
  }
  fs.read(cbuf, sizes.lengths * 4 + 2);
  if (fs.gcount() != static_cast<int>(sizes.lengths) * 4 + 2) {
    append(myerror, "read error for space and section data", ", ");
    return false;
  }
  free_space_manager.total_bytes = HDF5::value(&buf[0], sizes.lengths);
#ifdef __DEBUG
  cerr << "total space tracked (bytes): " << free_space_manager.total_bytes <<
      endl;
#endif
  free_space_manager.num_sections = HDF5::value(&buf[sizes.lengths], sizes.
      lengths);
#ifdef __DEBUG
  cerr << "total number of sections: " << free_space_manager.num_sections <<
      endl;
#endif
  free_space_manager.num_serialized = HDF5::value(&buf[sizes.lengths * 2],
      sizes.lengths);
#ifdef __DEBUG
  cerr << "number of serialized sections: " << free_space_manager.num_serialized
      << endl;
#endif
  free_space_manager.num_unserialized = HDF5::value(&buf[sizes.lengths*3],
      sizes.lengths);
#ifdef __DEBUG
  cerr << "number of un-serialized sections: " << free_space_manager.
      num_unserialized << endl;
#endif
  fs.read(cbuf, 6 + sizes.lengths);
  if (fs.gcount() != 6 + static_cast<int>(sizes.lengths)) {
    append(myerror, "read error for size data", ", ");
    return false;
  }
  free_space_manager.addr_size = HDF5::value(&buf[4], 2);
#ifdef __DEBUG
  cerr << "size of address space: " << free_space_manager.addr_size << endl;
#endif
  free_space_manager.max_size = HDF5::value(&buf[6], sizes.lengths);
#ifdef __DEBUG
  cerr << "maximum section size: " << free_space_manager.max_size << endl;
#endif
  fs.read(cbuf, sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.offsets)) {
    append(myerror, "read error for serialized section list address", ", ");
    return false;
  }
  free_space_manager.list_addr = HDF5::value(&buf[0], sizes.offsets);
#ifdef __DEBUG
  cerr << "address of serialized section list: " << free_space_manager.list_addr
      << endl;
#endif
  fs.read(cbuf, sizes.lengths * 2);
  if (fs.gcount() != static_cast<int>(sizes.lengths) * 2) {
    append(myerror, "read error for serialized section list address", ", ");
    return false;
  }
  free_space_manager.list_size = HDF5::value(&buf[0], sizes.lengths);
#ifdef __DEBUG
  cerr << "size of serialized section list (bytes): " << free_space_manager.
      list_size << endl;
#endif
  free_space_manager.list_asize = HDF5::value(&buf[sizes.lengths], sizes.
      lengths);
#ifdef __DEBUG
  cerr << "allocated size of serialized section list (bytes): " <<
      free_space_manager.list_asize << endl;
#endif
  if (!decode_free_space_section_list(frhp_data)) {
    append(myerror, "failed to decode free space section list", ", ");
    exit(1);
  }
  fs.seekg(curr_off, std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decode_fractal_heap(unsigned long long address,int header_message_type,FractalHeapData& frhp_data)
{
  if (address == undef_addr) {
    if (header_message_type != 0x0006) {
      append(myerror, "fractal heap is not defined", ", ");
    }
    return false;
  }
  auto curr_off=fs.tellg();
  fs.seekg(address,std::ios_base::beg);
  unsigned char buf[64];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,5);
  if (fs.gcount() != 5) {
    append(myerror, "read error bytes 1-5 in fractal heap", ", ");
    return false;
  }
  if (string(reinterpret_cast<char *>(buf),4) != "FRHP") {
    append(myerror, "not a fractal heap", ", ");
    return false;
  }
  if (buf[4] != 0) {
    append(myerror, "unable to decode fractal heap version " + to_string(
        static_cast<int>(buf[4])), ", ");
    return false;
  }
#ifdef __DEBUG
  cerr << "yay! fractal heap at address: " << address << endl;
#endif
  fs.read(cbuf,9);
  if (fs.gcount() != 9) {
    append(myerror, "read error bytes 6-14 in fractal heap", ", ");
    return false;
  }
  frhp_data.id_len=HDF5::value(buf,2);
#ifdef __DEBUG
  cerr << "id_len: " << frhp_data.id_len << endl;
#endif
  frhp_data.io_filter_size=HDF5::value(&buf[2],2);
#ifdef __DEBUG
  cerr << "io_filter_size: " << frhp_data.io_filter_size << endl;
#endif
  frhp_data.flags=buf[4];
#ifdef __DEBUG
  cerr << "flags: " << static_cast<int>(frhp_data.flags) << endl;
#endif
  frhp_data.max_managed_obj_size=HDF5::value(&buf[5],4);
#ifdef __DEBUG
  cerr << "max size of managed objects: " << frhp_data.max_managed_obj_size << endl;
#endif
  fs.read(cbuf,sizes.lengths+sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.lengths+sizes.offsets)) {
    append(myerror, "unable to read huge object data", ", ");
    return false;
  }
  fs.read(cbuf,sizes.lengths+sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.lengths+sizes.offsets)) {
    append(myerror, "unable to read free space data", ", ");
    return false;
  }
  frhp_data.space.bytes_free = HDF5::value(&buf[0], sizes.lengths);
#ifdef __DEBUG
  cerr << "free space (bytes): " << frhp_data.space.bytes_free << endl;
#endif
  frhp_data.space.manager_addr = HDF5::value(&buf[sizes.lengths], sizes.
      offsets);
#ifdef __DEBUG
  cerr << "address of free space manager: " << frhp_data.space.manager_addr <<
      endl;
#endif
  fs.read(cbuf,sizes.lengths*4);
  if (fs.gcount() != static_cast<int>(sizes.lengths*4)) {
    append(myerror, "unable to read managed space data", ", ");
    return false;
  }
  frhp_data.space.bytes_total = HDF5::value(&buf[0], sizes.lengths);
#ifdef __DEBUG
  cerr << "total managed space (bytes): " << frhp_data.space.bytes_total <<
      endl;
#endif
  frhp_data.space.bytes_allocated = HDF5::value(&buf[sizes.lengths], sizes.
      lengths);
#ifdef __DEBUG
  cerr << "allocated managed space (bytes): " << frhp_data.space.bytes_allocated
      << endl;
#endif
  frhp_data.space.next_allocation = HDF5::value(&buf[sizes.lengths * 2], sizes.
      lengths);
#ifdef __DEBUG
  cerr << "offset to next allocation: " << frhp_data.space.next_allocation <<
      endl;
#endif
  frhp_data.objects.num_managed=HDF5::value(&buf[sizes.lengths*3],sizes.lengths);
#ifdef __DEBUG
  cerr << "number of managed objects in heap: " << frhp_data.objects.num_managed << endl;
#endif
  fs.read(cbuf,sizes.lengths*4);
  if (fs.gcount() != static_cast<int>(sizes.lengths*4)) {
    append(myerror, "unable to read huge/tiny object data", ", ");
    return false;
  }
  frhp_data.objects.num_huge=HDF5::value(&buf[sizes.lengths],sizes.lengths);
  frhp_data.objects.num_tiny=HDF5::value(&buf[sizes.lengths*3],sizes.lengths);
#ifdef __DEBUG
  cerr << "number of huge objects: " << frhp_data.objects.num_huge << endl;
  cerr << "number of tiny objects: " << frhp_data.objects.num_tiny << endl;
#endif
  fs.read(cbuf,4+sizes.lengths*2);
  if (fs.gcount() != static_cast<int>(4+sizes.lengths*2)) {
    append(myerror, "unable to read table width and block size data", ", ");
    return false;
  }
  frhp_data.table_width=HDF5::value(buf,2);
#ifdef __DEBUG
  cerr << "table width=" << frhp_data.table_width << endl;
#endif
  frhp_data.start_block_size=HDF5::value(&buf[2],sizes.lengths);
#ifdef __DEBUG
  cerr << "starting block size: " << frhp_data.start_block_size << endl;
#endif
  frhp_data.max_dblock_size=HDF5::value(&buf[2+sizes.lengths],sizes.lengths);
#ifdef __DEBUG
  cerr << "max direct block size: " << frhp_data.max_dblock_size << endl;
#endif
  frhp_data.max_dblock_rows=static_cast<int>(log2(HDF5::value(&buf[2+sizes.lengths],sizes.lengths))-log2(HDF5::value(&buf[2],sizes.lengths)))+2;
#ifdef __DEBUG
  cerr << "max_dblock_rows=" << frhp_data.max_dblock_rows << endl;
#endif
  frhp_data.max_size=HDF5::value(&buf[2+sizes.lengths*2],2);
#ifdef __DEBUG
  cerr << "max_heap_size: " << frhp_data.max_size << endl;
#endif
  fs.read(cbuf,4+sizes.offsets);
  if (fs.gcount() != static_cast<int>(4+sizes.offsets)) {
    append(myerror, "unable to read block data", ", ");
    return false;
  }
#ifdef __DEBUG
  cerr << "start # rows: " << HDF5::value(buf,2) << endl;
#endif
  unsigned long long root_block_addr=HDF5::value(&buf[2],sizes.offsets);
#ifdef __DEBUG
  cerr << "address of root block: " << root_block_addr << endl;
#endif
  frhp_data.nrows=HDF5::value(&buf[2+sizes.offsets],2);
#ifdef __DEBUG
  cerr << "nrows=" << frhp_data.nrows << endl;
#endif
  frhp_data.K= (frhp_data.nrows < frhp_data.max_dblock_rows) ? frhp_data.nrows : frhp_data.max_dblock_rows;
  frhp_data.K*=frhp_data.table_width;
  frhp_data.N=frhp_data.K-(frhp_data.max_dblock_rows*frhp_data.table_width);
  if (frhp_data.N < 0) {
    frhp_data.N=0;
  }
#ifdef __DEBUG
  cerr << "K=" << frhp_data.K << " N=" << frhp_data.N << endl;
#endif
  if (frhp_data.space.manager_addr != undef_addr && !decode_free_space_manager(
      frhp_data)) {
    append(myerror, "error decoding free space manager", ", ");
    exit(1);
  }
  frhp_data.curr_row=frhp_data.curr_col=1;
  if (!decode_fractal_heap_block(root_block_addr,header_message_type,frhp_data)) {
    return false;
  }
  fs.seekg(curr_off,std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decode_fractal_heap_block(unsigned long long address, int
    header_message_type, FractalHeapData& frhp_data) {
  if (address == undef_addr) {
    append(myerror, "fractal heap block is not defined", ", ");
    return false;
  }
  auto curr_off = fs.tellg();
  fs.seekg(address, std::ios_base::beg);
  char buf[32];
  fs.read(buf, 5);
  if (fs.gcount() != 5) {
    append(myerror, "read error bytes 1-5 in fractal heap block", ", ");
    return false;
  }
  auto signature=string(buf, 4);
  if (signature == "FHDB") {
#ifdef __DEBUG
    cerr << "fractal heap direct block at address " << address << endl;
#endif 
    if (buf[4] != 0) {
      append(myerror, "unable to decode fractal heap direct block version " +
          to_string(static_cast<int>(buf[4])), ", ");
      return false;
    }
    int size_to_read = (frhp_data.curr_row - 1) * frhp_data.start_block_size;
    if (size_to_read == 0) {
      size_to_read = frhp_data.start_block_size;
    }
    size_t nbyts = (frhp_data.max_size + 7) / 8;
    fs.read(buf, sizes.offsets + nbyts);
    if (fs.gcount() != static_cast<int>(sizes.offsets + nbyts)) {
      append(myerror, "unable to read FHDB heap header address and block "
          "offset", ", ");
      return false;
    }
    size_to_read -= (5 + sizes.offsets + nbyts);
#ifdef __DEBUG
    cerr << "heap header address: " << HDF5::value(reinterpret_cast<unsigned
        char *>(buf), sizes.offsets) << endl;
    cerr << "block offset: " << HDF5::value(&(reinterpret_cast<unsigned char *>(
        buf))[sizes.offsets],nbyts) << "  flags: " << static_cast<int>(
        frhp_data.flags) << endl;
#endif
    if ( (frhp_data.flags & 0x2) == 0x2) {
      fs.read(buf, 4);
      if (fs.gcount() != 4) {
        append(myerror, "unable to read FHDB checksum", ", ");
        return false;
      }
      size_to_read -= 4;
    }
#ifdef __DEBUG
    cerr << "size to read should be: " << size_to_read << "  hdr msg type: " <<
        header_message_type << endl;
#endif
    auto *buf2 = new unsigned char[size_to_read];
    fs.read(reinterpret_cast<char *>(buf2), size_to_read);
    if (fs.gcount() != static_cast<int>(size_to_read)) {
      append(myerror, "unable to read FHDB object data", ", ");
      return false;
    }
#ifdef __DEBUG
    dump(buf2, size_to_read);
#endif
    switch (header_message_type) {
      case 6: {
        auto local_off = 0;
        bool is_subgroup = false;
//        while (buf2[local_off] == 1 && local_off < size_to_read) {
size_to_read -= 6;
while(local_off < size_to_read) {
          local_off += decode_header_message("", 0, 6, 0, 0, &buf2[local_off],
              &root_group, *(frhp_data.dse), is_subgroup);
        }
        break;
      }
      case 12: {
        auto local_off = 0;

        // need enough space at end of block for an attribute name (1
        //   byte), attribute datatype (at least 8 bytes), and attribute
        //   dataspace (at least 8 bytes)
        size_to_read -= 17;
        while (local_off < size_to_read) {
          auto it = free_space_manager.space_map.find(local_off);
          if (it != free_space_manager.space_map.end()) {
#ifdef __DEBUG
            cerr << "skipping " << it->second << " bytes of free space at "
                "offset " << local_off << endl;
#endif
            local_off += it->second;
            if (local_off >= size_to_read) {
              break;
            }
          }
          Attribute attr;
          int n;
          if (decode_attribute(&buf2[local_off], attr, n, 2)) {
#ifdef __DEBUG
            cerr << "ATTRIBUTE (1) of '" << frhp_data.dse->key << "' at: " <<
                local_off << " with length: " << n << " " << &root_group << " "
                << frhp_data.dse->p_ds << endl;
            cerr << "  name: " << attr.first << "  dataset: " << frhp_data.dse->
                p_ds << endl;
#endif
            if (frhp_data.dse->p_ds->attributes.find(attr.first) == frhp_data.
                dse->p_ds->attributes.end()) {
              frhp_data.dse->p_ds->attributes.emplace(attr);
#ifdef __DEBUG
              cerr << "added" << endl;
#endif
            } else {
              frhp_data.dse->p_ds->attributes[attr.first] = attr.second;
#ifdef __DEBUG
              cerr << "replaced" << endl;
#endif
            }
            local_off += n;
          } else {
            ++local_off;
          }
        }
        break;
      }
      default: {
        append(myerror, "unable to decode FHDB object data for message type " +
            to_string(header_message_type), ", ");
        return false;
      }
    }
    delete[] buf2;
    ++frhp_data.curr_col;
    if (frhp_data.curr_col > frhp_data.table_width) {
      frhp_data.curr_col = 1;
      ++frhp_data.curr_row;
    }
  } else if (signature == "FHIB") {
#ifdef __DEBUG
    cerr << "fractal heap indirect block" << endl;
#endif
    if (buf[4] != 0) {
      append(myerror, "unable to decode fractal heap indirect block version " +
          to_string(static_cast<int>(buf[4])), ", ");
      return false;
    }
    fs.read(buf, sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
      append(myerror, "unable to read FHIB heap header address", ", ");
      return false;
    }
#ifdef __DEBUG
    cerr << "heap header address: " << HDF5::value(reinterpret_cast<unsigned
        char *>(buf),sizes.offsets) << endl;
#endif
    size_t nbyts = (frhp_data.max_size + 7) / 8;
    fs.read(buf, nbyts);
    if (fs.gcount() != static_cast<int>(nbyts)) {
      append(myerror, "unable to FHIB block offset", ", ");
      return false;
    }
#ifdef __DEBUG
    cerr << "block offset: " << HDF5::value(reinterpret_cast<unsigned char *>(
        buf), nbyts) << endl;
#endif
    for (int n = 0; n < frhp_data.K; ++n) {
      fs.read(buf, sizes.offsets);
      if (fs.gcount() != static_cast<int>(sizes.offsets)) {
        append(myerror, "unable to read FHIB child direct block " + to_string(
            n), ", ");
        return false;
      }
      unsigned long long block_addr = HDF5::value(reinterpret_cast<unsigned
          char *>(buf), sizes.offsets);
#ifdef __DEBUG
      cerr << "child direct block " << n << " addr: " << block_addr << endl;
#endif
      if (block_addr != undef_addr) {
        decode_fractal_heap_block(block_addr, header_message_type, frhp_data);
      }
      if (frhp_data.io_filter_size > 0) {
        fs.read(buf, sizes.offsets+sizes.lengths);
        if (fs.gcount() != static_cast<int>(sizes.offsets+sizes.lengths)) {
          append(myerror, "unable to read FHIB filter data for direct block " +
              to_string(n), ", ");
          return false;
        }
      }
    }
  } else {
    append(myerror, "not a fractal heap block", ", ");
    return false;
  }
  fs.seekg(curr_off, std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decode_header_messages(int ohdr_version, size_t
    header_size, string ident, Group *group, DatasetEntry *dse_in, unsigned
    char flags) {
  size_t hdr_off = 0;
  std::unique_ptr<unsigned char[]> buf(new unsigned char[header_size]);
  DatasetEntry dse;
  if (dse_in == nullptr) {
    if (group == nullptr) {
      if (find_if(root_group.datasets.begin(), root_group.datasets.end(),
          FindDataset(ident)) == root_group.datasets.end()) {
        dse.key = ident;
        dse.p_ds.reset(new Dataset);
      }
    } else {
      if (group != nullptr && find_if(group->datasets.begin(), group->
          datasets.end(), FindDataset(ident)) == group->datasets.end()) {
        dse.key = ident;
        dse.p_ds.reset(new Dataset);
      }
    }
  } else {
    dse = *dse_in;
#ifdef __DEBUG
    cerr << "ready to read header messages at offset " << fs.tellg() <<
        " size: " << header_size << " ident: '" << ident << "' GROUP: " << group
        << " flags: " << static_cast<int>(flags) << endl;
#endif
  }
  fs.read(reinterpret_cast<char *>(buf.get()), header_size);
  if (fs.gcount() != static_cast<int>(header_size)) {
    append(myerror, "read error while trying to get header messages", ", ");
    return false;
  }
  if (string(reinterpret_cast<char *>(buf.get()), 4) == "OCHK") {
    hdr_off += 4;
    header_size -= 4;
  }
#ifdef __DEBUG
  cerr << "object header version: " << ohdr_version << endl;
#endif
  bool is_subgroup = false;
  while ( (hdr_off + 4) < header_size) {
#ifdef __DEBUG
    cerr << "hdr_off: " << hdr_off << " header size: " << header_size << endl;
#endif
    int hdr_msg_type, hdr_msg_len;
    switch (ohdr_version) {
      case 1: {
        hdr_msg_type = HDF5::value(&buf[hdr_off], 2);
        hdr_msg_len = HDF5::value(&buf[hdr_off + 2], 2);
#ifdef __DEBUG
        cerr << "flags: " << static_cast<int>(buf[hdr_off + 4]) << endl;
#endif
        hdr_off += 8;
        break;
      }
      case 2: {
        hdr_msg_type = static_cast<int>(buf[hdr_off]);
        hdr_msg_len = HDF5::value(&buf[hdr_off+1], 2);
#ifdef __DEBUG
        cerr << "flags: " << static_cast<int>(buf[hdr_off+3]) << endl;
#endif
        hdr_off += 4;
        if ( (flags & 0x4) == 0x4) {
          hdr_off += 2;
        }
        break;
      }
      default: {
        append(myerror, "unable to read header messages for version " +
            to_string(ohdr_version), ", ");
        return false;
      }
    }
#ifdef __DEBUG
    long long off = fs.tellg();
    cerr << "HEADER MESSAGE TYPE: " << hdr_msg_type << " " << " " << hdr_msg_len
        << " " << hdr_off << " header size: " << header_size << " file offset: "
        << off-header_size << " " << time(NULL) << endl;
    dump(buf.get(), hdr_off + hdr_msg_len);
#endif
    hdr_off += decode_header_message(ident, ohdr_version, hdr_msg_type,
        hdr_msg_len, flags, &buf[hdr_off], group, dse, is_subgroup);
  }
  buf = nullptr;
  if (!is_subgroup) {
    if (group == nullptr) {
      group = &root_group;
    }
#ifdef __DEBUG
    cerr << "-- NOT SUBGROUP '" << ident << "' " << group << endl;
#endif
    if (find_if(group->datasets.begin(), group->datasets.end(), FindDataset(
        ident)) == group->datasets.end()) {
      dse.key = ident;
#ifdef __DEBUG
      cerr << "  ADDING DATASET '" << ident << "' TO " << group << endl;
#endif
      group->datasets.emplace_back(dse);
    }
#ifdef __DEBUG
    cerr << "  HERE " << dse.p_ds << " '" << dse.key << "' '" << ident <<
        "'" << endl;
#endif
  } else {
    if (dse_in == nullptr) {
      dse.p_ds.reset();
    }
  }
  return true;
}

int InputHDF5Stream::decode_header_message(string ident, int ohdr_version,
    int type, int length, unsigned char flags, unsigned char *buffer, Group
    *group, DatasetEntry& dse, bool& is_subgroup) {
  unsigned long long curr_off;
  switch (type) {
    case 0x0000: {
      return length;
    }
    case 0x0001: {

      // Dataspace
#ifdef __DEBUG
      cerr << "DATASPACE" << endl;
#endif
      HDF5::decode_dataspace(buffer, sizes.lengths, dse.p_ds->dataspace);
      return length;
    }
    case 0x0002: {

      // Link info
      if (buffer[0] != 0) {
         append(myerror, "unable to decode link info for version number " +
             to_string(static_cast<int>(buffer[0])), ", ");
         exit(1);
      }
#ifdef __DEBUG
      cerr << "Link Info:" << endl;
      cerr << "  version: " << static_cast<int>(buffer[0]) << " flags: " <<
          static_cast<int>(buffer[1]) << endl;
#endif
      curr_off = 2;
#ifdef __DEBUG
      cerr << "curr_off is " << curr_off << endl;
#endif
      if ( (buffer[1] & 0x1) == 0x1) {
#ifdef __DEBUG
        cerr << "max creation index: " << HDF5::value(&buffer[curr_off],8) <<
            endl;
#endif
        curr_off += 8;
      }
#ifdef __DEBUG
      cerr << "fractal heap addr: " << HDF5::value(&buffer[curr_off], sizes.
          offsets) << endl;
#endif
      auto frhp_data = new FractalHeapData;
      frhp_data->dse = &dse;
      free_space_manager.space_map.clear();
      if (decode_fractal_heap(HDF5::value(&buffer[curr_off], sizes.offsets),
          0x0006, *frhp_data)) {
        root_group.btree_addr=HDF5::value(&buffer[curr_off+sizes.offsets],
            sizes.offsets);
#ifdef __DEBUG
        cerr << "b-tree name index addr: " << root_group.btree_addr << endl;
#endif
        if (!decode_Btree(root_group, frhp_data)) {
          exit(1);
        }
      }
#ifdef __DEBUG
      if ( (buffer[1] & 0x2) == 0x2) {
        cerr << "b-tree creation order index addr: " << HDF5::value(&buffer[
            curr_off + sizes.offsets * 2],sizes.offsets) << endl;
      }
#endif
      delete frhp_data;
      return length;
    }
    case 0x0003: {

      // Datatype
#ifdef __DEBUG
      cerr << "DATATYPE" << endl;
#endif
      HDF5::decode_datatype(buffer, dse.p_ds->datatype);
      return length;
    }
    case 0x0004: {

      // Fill value (deprecated)
#ifdef __DEBUG
      cerr << "FILLVALUE (deprecated) '" << dse.key << "'" << endl;
#endif
      if (dse.p_ds->fillvalue.length == 0) {
        if ( (dse.p_ds->fillvalue.length=HDF5::value(&buffer[0], 4)) > 0) {
          dse.p_ds->fillvalue.bytes = new unsigned char[dse.p_ds->fillvalue.
              length];
          copy(buffer+4, buffer+4+dse.p_ds->fillvalue.length, dse.p_ds->
              fillvalue.bytes);
        }
      }
      return length;
    }
    case 0x0005: {

      // Fill value
#ifdef __DEBUG
      cerr << "FILLVALUE '" << dse.key << "'" << endl;
#endif
      switch (static_cast<int>(buffer[0])) {
        case 2: {
          if (static_cast<int>(buffer[3]) == 1) {
            dse.p_ds->fillvalue.length = HDF5::value(&buffer[4], 4);
            if (dse.p_ds->fillvalue.length > 0) {
              dse.p_ds->fillvalue.bytes = new unsigned char[dse.p_ds->fillvalue.
                  length];
              copy(buffer+8, buffer+8+dse.p_ds->fillvalue.length, dse.p_ds->
                  fillvalue.bytes);
            } else {
              dse.p_ds->fillvalue.bytes = nullptr;
            }
/*
          } else {
            append(myerror, "fill value undefined", ", ");
            exit(1);
*/
          }
          break;
        }
        case 3: {
          if ( (buffer[1] & 0x20) == 0x20) {
            dse.p_ds->fillvalue.length = HDF5::value(&buffer[2], 4);
            if (dse.p_ds->fillvalue.length > 0) {
              dse.p_ds->fillvalue.bytes = new unsigned char[dse.p_ds->fillvalue.
                  length];
              copy(buffer+6, buffer+6+dse.p_ds->fillvalue.length, dse.p_ds->
                  fillvalue.bytes);
            } else {
              dse.p_ds->fillvalue.bytes = nullptr;
            }
          } else {
            dse.p_ds->fillvalue.length = 0;
            dse.p_ds->fillvalue.bytes = nullptr;
          }
          break;
        }
        default: {
          append(myerror, "unable to decode version " + to_string(static_cast<
              int>(buffer[0])) + " Fill Value message", ", ");
          exit(1);
        }
      }
      return length;
    }
    case 0x0006: {

      // Link message
#ifdef __DEBUG
      cerr << "Link message: version: " << static_cast<int>(buffer[0]) << " flags: " << static_cast<int>(buffer[1]) << std::endl;
#endif
      int link_type;
      curr_off = 2;
      link_type = 0;
      if ( (buffer[1] & 0x8) == 0x8) {

        // Link type
        link_type = buffer[curr_off++];
      }
      if (link_type != 0) {
        append(myerror, "unable to decode links that are not hard links", ", ");
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
      long long length_of_link_name;
      if ( (buffer[1] & 0x3) == 0) {
        length_of_link_name = buffer[curr_off];
        curr_off++;
      } else if ( (buffer[1] & 0x3) == 1) {
        length_of_link_name = HDF5::value(&buffer[curr_off], 2);
        curr_off += 2;
      } else if ( (buffer[1] & 0x3) == 2) {
        length_of_link_name = HDF5::value(&buffer[curr_off], 4);
        curr_off += 4;
      } else if ( (buffer[1] & 0x3) == 3) {
        length_of_link_name = HDF5::value(&buffer[curr_off], 8);
        curr_off += 8;
      }
      if (length_of_link_name > 0) {
        auto ste = new SymbolTableEntry;
        ste->linkname.assign(reinterpret_cast<char *>(&buffer[curr_off]),
            length_of_link_name);
  #ifdef __DEBUG
        cerr << "Link name: '" << ste->linkname << "', type: " << link_type <<
            endl;
  #endif
        curr_off += length_of_link_name;
        if (link_type == 0) {
          ste->objhdr_addr = HDF5::value(&buffer[curr_off], sizes.offsets);
  #ifdef __DEBUG
          cerr << "Address of link object header: " << ste->objhdr_addr << endl;
  #endif
          if (!decode_object_header(*ste, &root_group)) {
            exit(1);
          }
          curr_off += sizes.offsets;
          if (buffer[0] == 1) {
            if (ref_table == nullptr) {
              ref_table.reset(new unordered_map<size_t, string>);
            }
            if (ref_table->find(ste->objhdr_addr) == ref_table->end()) {
              ref_table->emplace(ste->objhdr_addr, ste->linkname);
            }
          }
        }
        delete ste;
      }
#ifdef __DEBUG
      cerr << "message length: " << curr_off << std::endl;
#endif
      return curr_off;
      break;
    }
    case 0x0008: {

      // Layout
#ifdef __DEBUG
      cerr << "LAYOUT" << endl;
#endif
      switch (static_cast<int>(buffer[0])) {
        case 3: {
          switch (static_cast<int>(buffer[1])) {
            case 1: {
              dse.p_ds->data.address = HDF5::value(&buffer[2], sizes.offsets);
              dse.p_ds->data.sizes.emplace_back(HDF5::value(&buffer[2 + sizes.
                  offsets], sizes.lengths));
#ifdef __DEBUG
              cerr << "CONTIGUOUS DATA at " << dse.p_ds->data.address << " of "
                  "length " << dse.p_ds->data.sizes.back() << endl;
#endif
              break;
            }
            case 2: {
              dse.p_ds->data.address = HDF5::value(&buffer[3], sizes.offsets);
              short dimensionality = buffer[2] - 1;
#ifdef __DEBUG
              cerr << "CHUNKED DATA of dimensionality " << dimensionality <<
                  " at address " << dse.p_ds->data.address << endl;
#endif
              for (short n = 0; n < dimensionality; ++n) {
                dse.p_ds->data.sizes.emplace_back(HDF5::value(&buffer[3 + sizes.
                    offsets + n * 4], 4));
#ifdef __DEBUG
                cerr << "  dim: " << n << " size: " << dse.p_ds->data.sizes.
                    back() << endl;
#endif
              }
              dse.p_ds->data.size_of_element = HDF5::value(&buffer[3 + sizes.
                  offsets + dimensionality * 4], 4);
#ifdef __DEBUG
              cerr << "  dataset element size: " << dse.p_ds->data.
                  size_of_element << " at offset " << 3 + sizes.offsets +
                  dimensionality * 4 << endl;
#endif
              if (dse.p_ds->data.address != undef_addr) {
                decode_v1_Btree(dse.p_ds->data.address, *dse.p_ds);
              }
              break;
            }
            default: {
              append(myerror, "unable to decode layout class " + to_string(
                  static_cast<int>(buffer[1])), ", ");
              exit(1);
            }
          }
          break;
        }
        default: {
          append(myerror, "unable to decode layout version " + to_string(
              static_cast<int>(buffer[0])), ", ");
          exit(1);
        }
      }
      return length;
    }
    case 0x000a: {

       // Group info
       if (buffer[0] != 0) {
          append(myerror, "unable to decode new-style group info for version "
              "number " + to_string(static_cast<int>(buffer[0])), ", ");
          exit(1);
      }
#ifdef __DEBUG
      cerr << "Group Info:" << endl;
#endif
      curr_off = 2;
      if ( (buffer[1] & 0x1) == 0x1) {
#ifdef __DEBUG
        cerr << "max # compact links: " << HDF5::value(&buffer[curr_off], 2) <<
            endl;
        cerr << "max # dense links: " << HDF5::value(&buffer[curr_off+2], 2) <<
            endl;
#endif
        curr_off += 4;
      }
      if ( (buffer[1] & 0x2) == 0x2) {
#ifdef __DEBUG
        cerr << "estimated # of entries: " << HDF5::value(&buffer[curr_off], 2)
            << endl;
        cerr << "estimated link name length: " << HDF5::value(&buffer[curr_off +
            2], 2) << endl;
#endif
        curr_off += 4;
      }
      return length;
    }
    case 0x000b: {

      // Filter pipeline
      switch(buffer[0]) {
        case 1: {
          curr_off = 8;
          break;
        }
        case 2: {
          curr_off = 2;
          break;
        }
        default: {
          append(myerror, "unable to decode filter pipeline version " +
              to_string(buffer[0]), ", ");
          exit(1);
        }
      }
      for (size_t n = 0; n < static_cast<size_t>(buffer[1]); ++n) {
        dse.p_ds->filters.push_front(HDF5::value(&buffer[curr_off], 2));
        size_t name_length;
        if (buffer[0] == 1 || dse.p_ds->filters.front() >= 256) {

          // name length
          name_length=HDF5::value(&buffer[curr_off + 2], 2);
          curr_off += 2;
        } else {
          name_length = 0;
        }
        auto nvals = HDF5::value(&buffer[curr_off + 4], 2);
        curr_off += 6 + name_length + 4 * nvals;
        if (buffer[0] == 1 && (nvals % 2) != 0) {
          curr_off += 4;
        }
#ifdef __DEBUG
        cerr << "filter " << n << " of " << static_cast<int>(buffer[1]) <<
            " id: " << dse.p_ds->filters.front() << endl;
#endif
      }
#ifdef __DEBUG
      cerr << "filter pipeline message:" << endl;
      dump(buffer, length);
#endif
      return length;
    }
    case 0x000c: {

      // Attribute
#ifdef __DEBUG
      cerr << "ATTRIBUTE (2) of '" << dse.key << "' " << group << " " << dse.
          p_ds << endl;
#endif
      Attribute attr;
      int attr_len;
      if (!decode_attribute(buffer, attr, attr_len, ohdr_version)) {
        exit(1);
      }
#ifdef __DEBUG
      cerr << "  name: " << attr.first << "  dataset: " << dse.p_ds << endl;
      cerr << &(dse.p_ds->attributes) << endl;
      cerr << dse.p_ds->attributes.size() << endl;
#endif
      dse.p_ds->attributes.emplace(attr);
#ifdef __DEBUG
      cerr << "added" << endl;
#endif
      if (attr_len != length) {
#ifdef __DEBUG
        cerr << "calculated and returning " << attr_len << " but length from "
            "header message was " << length << endl;
#endif
        while (attr_len < length && buffer[attr_len] == 0x0) {
          ++attr_len;
        }
      }
      return attr_len;
    }
    case 0x0010: {

      // Object Header Continuation
      curr_off = fs.tellg();
      auto ochk_off = HDF5::value(buffer, sizes.offsets);
      length = sizes.offsets;
      fs.seekg(ochk_off, std::ios_base::beg);
#ifdef __DEBUG
      cerr << "branching to continuation block at " << ochk_off << " from " <<
          curr_off << endl;
#endif
      auto ochk_len = HDF5::value(&buffer[length], sizes.offsets);
      length += sizes.offsets;
      if (!decode_header_messages(ohdr_version, ochk_len, ident, group, &dse,
          flags)) {
        exit(1);
      }
      fs.seekg(curr_off, std::ios_base::beg);
#ifdef __DEBUG
      cerr << "done with continuation block, back to " << fs.tellg() << endl;
#endif
      return length;
    }
    case 0x0011: {
      Group *g;
      if (group == nullptr) {
        g = &root_group;
      } else {
        group->groups.emplace_back(GroupEntry());
        group->groups.back().key = ident;
        group->groups.back().p_g.reset(new Group);
#ifdef __DEBUG
        cerr << "SUBGROUP '" << ident << "' " << group->groups.back().p_g <<
            endl;
#endif
        g = group->groups.back().p_g.get();
      }
      auto v1_btree_addr = HDF5::value(buffer, sizes.offsets);
#ifdef __DEBUG
      cerr << "v1 Btree address: " << v1_btree_addr << endl;
#endif
      g->btree_addr = v1_btree_addr;
      length = sizes.offsets;
      auto local_heap_addr = HDF5::value(&buffer[length], sizes.offsets);
#ifdef __DEBUG
      cerr << "local heap address: " << local_heap_addr << endl;
#endif
      g->local_heap.addr = local_heap_addr;
      length += sizes.offsets;
      curr_off = fs.tellg();
      fs.seekg(g->local_heap.addr, std::ios_base::beg);
      auto buf_len = 8 + 2 * sizes.lengths + sizes.offsets;
      auto buf2 = new unsigned char[buf_len];
      fs.read(reinterpret_cast<char *>(buf2), buf_len);
      if (fs.gcount() != static_cast<int>(buf_len)) {
        append(myerror, "read error while trying to get local heap header",
            ", ");
        exit(1);
      }
      g->local_heap.data_start = HDF5::value(&buf2[8+2*sizes.lengths], sizes.
          offsets);
      if (g->local_heap.data_start == 0) {
        g->local_heap.data_start = g->local_heap.addr + 8 + 2 * sizes.lengths +
            sizes.offsets;
      }
      delete[] buf2;
      if (!decode_Btree(*g, nullptr)) {
          exit(1);
      }
      fs.seekg(curr_off, std::ios_base::beg);
      is_subgroup = true;
      return length;
    }
    case 0x0012: {

      // Modification time
#ifdef __DEBUG
      cerr << "WARNING: ignoring object modification time" << endl;
#endif
      return length;
    }
    case 0x0015: {

      // Attribute info
      auto off = 2;
      if ( (buffer[1] & 0x1) == 0x1) {
        off += 2;
      }
      auto frhp_addr = HDF5::value(&buffer[off], sizes.offsets);
      if (frhp_addr == undef_addr) {
        return length;
      }
#ifdef __DEBUG
      dump(buffer, off + sizes.offsets * 2);
      cerr << frhp_addr << " " << HDF5::value(&buffer[off+sizes.offsets], sizes.
          offsets) << endl;
      cerr << "****************" << endl;
#endif
      auto frhp_data = new FractalHeapData;
      frhp_data->dse = &dse;
      free_space_manager.space_map.clear();
      if (!decode_fractal_heap(frhp_addr, 0x000c, *frhp_data)) {
#ifdef __DEBUG
        cerr << "compact link" << endl;
#endif
      }
#ifdef __DEBUG
      cerr << "****************" << endl;
#endif
      delete frhp_data;
      return length;
    }
    case 0x0016: {

      // Reference count
#ifdef __DEBUG
      cerr << "WARNING: ignoring object reference count" << endl;
#endif
      return length;
    }
    default: {
      append(myerror, "unknown message header type " + to_string(type) + " of "
          "length " + to_string(length), ", ");
      exit(1);
    }
  }
}

bool InputHDF5Stream::decode_object_header(SymbolTableEntry& ste,Group *group)
{
  static const string THIS_FUNC=__func__;
  auto start_off=fs.tellg();
  fs.seekg(ste.objhdr_addr,std::ios_base::beg);
  unsigned char buf[12];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,4);
  if (fs.gcount() != 4) {
    append(myerror, THIS_FUNC + "(): unable to decode first 4 bytes of object "
        "header", "\n");
    return false;
  }
  auto signature=string(reinterpret_cast<char *>(cbuf),4);
#ifdef __DEBUG
  cerr << "object header signature: \"" << signature << "\" for '" << ste.linkname << "' " << ste.objhdr_addr << endl;
#endif
  if (signature == "OHDR") {
    fs.read(cbuf,2);
    if (fs.gcount() != 2) {
      append(myerror, THIS_FUNC + "(): unable to get version for OHDR", "\n");
      return false;
    }
    auto version=static_cast<short>(buf[0]);
    if (version == 2) {
      auto flags=buf[1];
      if ( (flags & 0x20) == 0x20) {
        fs.read(cbuf,4);
        if (fs.gcount() != 4) {
          append(myerror, THIS_FUNC + "(): unable to read OHDR access time",
              "\n");
          return false;
        }
#ifdef __DEBUG
        cerr << "access time: " << HDF5::value(buf,4) << endl;
#endif
        fs.read(cbuf,4);
        if (fs.gcount() != 4) {
          append(myerror, THIS_FUNC + "(): unable to read OHDR modification "
              "time", "\n");
          return false;
        }
#ifdef __DEBUG
        cerr << "mod time: " << HDF5::value(buf,4) << endl;
#endif
        fs.read(cbuf,4);
        if (fs.gcount() != 4) {
          append(myerror, THIS_FUNC + "(): unable to read OHDR change time",
              "\n");
          return false;
        }
#ifdef __DEBUG
        cerr << "change time: " << HDF5::value(buf,4) << endl;
#endif
        fs.read(cbuf,4);
        if (fs.gcount() != 4) {
          append(myerror, THIS_FUNC + "(): unable to read OHDR birth time",
              "\n");
          return false;
        }
#ifdef __DEBUG
        cerr << "birth time: " << HDF5::value(buf,4) << endl;
#endif
      }
      if ( (flags & 0x10) == 0x10) {
        fs.read(cbuf,4);
        if (fs.gcount() != 4) {
          append(myerror, THIS_FUNC + "(): unable to read OHDR attribute data",
              "\n");
          return false;
        }
#ifdef __DEBUG
        cerr << "max # compact attributes: " << HDF5::value(buf,2) << endl;
        cerr << "min # dense attributes: " << HDF5::value(&buf[2],2) << endl;
#endif
      }
      size_t hdr_size=pow(2.,static_cast<int>(flags & 0x3));
      fs.read(cbuf,hdr_size);
      if (fs.gcount() != static_cast<int>(hdr_size)) {
        append(myerror, THIS_FUNC + "(): unable to read OHDR size of chunk #0",
            "\n");
        return false;
      }
      hdr_size=HDF5::value(buf,hdr_size);
#ifdef __DEBUG
      cerr << "flags=" << static_cast<int>(flags) << " " << static_cast<int>(flags & 0x3) << " " << pow(2.,static_cast<int>(flags & 0x3)) << " " << hdr_size << endl;
#endif
      long long curr_off = fs.tellg();
      if (!decode_header_messages(2, hdr_size, ste.linkname, group, nullptr,
          flags)) {
        return false;
      }
      fs.seekg(curr_off+hdr_size, std::ios_base::beg);
    } else {
      append(myerror, THIS_FUNC + "(): unable to decode OHDR at addr=" +
          to_string(ste.objhdr_addr), "\n");
      return false;
    }
  } else {
    auto version = static_cast<int>(buf[0]);
    if (version == 1) {
#ifdef __DEBUG
      auto num_msgs = HDF5::value(&buf[2], 2);
#endif
      fs.read(cbuf,8);
      if (fs.gcount() != 8) {
        append(myerror, THIS_FUNC + "(): unable to decode next 8 bytes of "
            "object header", "\n");
        return false;
      }
#ifdef __DEBUG
      auto ref_cnt = HDF5::value(buf, 4);
#endif
      auto hdr_size = HDF5::value(&buf[4], 4);
#ifdef __DEBUG
      cerr << "object header version=" << version << " num_msgs=" << num_msgs << " ref_cnt=" << ref_cnt << " hdr_size=" << hdr_size << endl;
#endif
      fs.seekg(4,std::ios_base::cur);
      long long curr_off=fs.tellg();
      if (!decode_header_messages(1,hdr_size,ste.linkname,group,nullptr,0x0)) {
        return false;
      }
      fs.seekg(curr_off+hdr_size,std::ios_base::beg);
    } else {
      append(myerror, THIS_FUNC + "(): expected version 1 but got version " +
          to_string(version) + " at offset " + to_string(fs.tellg()), "\n");
      return false;
    }
  }
  fs.seekg(start_off, std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decode_superblock(unsigned long long& objhdr_addr) {
  unsigned char buf[12];
  char *cbuf = reinterpret_cast<char *>(buf);

  // check for 0x894844460d0a1a0a in first eight bytes (\211HDF\r\n\032\n)
  fs.read(cbuf, 12);
  if (fs.gcount() != 12) {
    append(myerror, "read error in first 12 bytes", "\n");
    return false;
  }
  unsigned long long magic;
  bits::get(buf, magic, 0, 64);
  if (magic != 0x894844460d0a1a0a) {
    append(myerror, "not an HDF5 file", "\n");
    return false;
  }
  sb_version = static_cast<int>(buf[8]);
  if (sb_version == 0 || sb_version == 1) {
#ifdef __DEBUG
    cerr << "sb_version=" << sb_version << endl;
#endif
    fs.read(cbuf,12);
    if (fs.gcount() != 12) {
      append(myerror, "read error in second 12 bytes of V0/1 superblock", "\n");
      return false;
    }
    sizes.offsets=static_cast<int>(buf[1]);
    bits::create_mask(undef_addr,sizes.offsets*8);
    sizes.lengths=static_cast<int>(buf[2]);
    group_K.leaf=HDF5::value(&buf[4],2);
    group_K.internal=HDF5::value(&buf[6],2);
#ifdef __DEBUG
    cerr << sizes.offsets << " " << sizes.lengths << " " << group_K.leaf << " " << group_K.internal << endl;
#endif
    if (sb_version == 1) {
      fs.read(cbuf,4);
      if (fs.gcount() != 4) {
        append(myerror, "read error for superblock v1 additions", "\n");
        return false;
      }
    }
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
      append(myerror, "read error for V0/1 superblock base address", "\n");
      return false;
    }
    base_addr=HDF5::value(buf,sizes.offsets);
#ifdef __DEBUG
    cerr << "base address: " << base_addr << endl;
#endif
    fs.read(cbuf, sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
      append(myerror, "read error for V0/1 superblock file free-space address",
          "\n");
      return false;
    }
    fs.read(cbuf, sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
      append(myerror, "read error for V0/1 superblock end-of-file address",
          "\n");
      return false;
    }
    eof_addr = HDF5::value(buf, sizes.offsets);
#ifdef __DEBUG
    cerr << "eof address: " << eof_addr << endl;
#endif
    fs.read(cbuf, sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
      append(myerror, "read error for V0/1 superblock driver information block "
          "address", "\n");
      return false;
    }
  } else if (sb_version == 2) {
    sizes.offsets = static_cast<int>(buf[9]);
#ifdef __DEBUG
    cerr << "size of offsets: " << sizes.offsets << endl;
#endif
    bits::create_mask(undef_addr, sizes.offsets*8);
    sizes.lengths = static_cast<int>(buf[10]);
#ifdef __DEBUG
    cerr << "size of lengths: " << sizes.lengths << endl;
#endif
    fs.read(cbuf, sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
      append(myerror, "read error for v2 superblock base address", "\n");
      return false;
    }
    base_addr = HDF5::value(buf, sizes.offsets);
#ifdef __DEBUG
    cerr << "base addr: " << base_addr << endl;
#endif
    fs.read(cbuf, sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
      append(myerror, "read error for v2 superblock extension address", "\n");
      return false;
    }
    unsigned long long ext_addr = HDF5::value(buf, sizes.offsets);
#ifdef __DEBUG
    cerr << "ext addr: " << ext_addr << endl;
#endif
    if (ext_addr != undef_addr) {
      append(myerror, "unable to handle superblock extension", "\n");
      return false;
    }
    fs.read(cbuf, sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
      append(myerror, "read error for v2 superblock end-of-file address", "\n");
      return false;
    }
    eof_addr=HDF5::value(buf,sizes.offsets);
#ifdef __DEBUG
    cerr << "eof addr: " << eof_addr << endl;
#endif
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
      append(myerror, "read error for v2 superblock root group object header "
          "address", "\n");
      return false;
    }
    objhdr_addr=HDF5::value(buf,sizes.offsets);
#ifdef __DEBUG
    cerr << "root group object header addr=" << objhdr_addr << endl;
#endif 
    fs.read(cbuf,4);
    if (fs.gcount() != 4) {
      append(myerror, "read error for v2 superblock checksum", "\n");
      return false;
    }
  } else {
    append(myerror, "unable to decode superblock version " + to_string(
        sb_version), "\n");
    return false;
  }
  if (sizes.lengths > 16 || sizes.offsets > 16) {
    append(myerror, "the size of lengths and/or offsets is out of range", "\n");
    return false;
  }
  return true;
}

bool InputHDF5Stream::decode_symbol_table_entry(SymbolTableEntry& ste,Group *group)
{
#ifdef __DEBUG
  cerr << "SYMBOL TABLE ENTRY " << fs.tellg() << endl;
#endif
  unsigned char buf[16];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.offsets)) {
    append(myerror, "read error for symbol table entry link name offset", ", ");
    return false;
  }
  ste.linkname_off=HDF5::value(buf,sizes.offsets);
#ifdef __DEBUG
  cerr << "link name offset: " << ste.linkname_off << endl;
#endif
  if (group != NULL) {
#ifdef __DEBUG
    cerr << "  local heap data start: " << group->local_heap.data_start << endl;
#endif
    ste.linkname="";
    auto curr_off=fs.tellg();
    fs.seekg(group->local_heap.data_start+ste.linkname_off,std::ios_base::beg);
    while (1) {
      fs.read(cbuf,1);
      if (fs.gcount() != 1) {
        append(myerror, "read error while getting link name", ", ");
        return false;
      }
      if (buf[0] == 0) {
        break;
      } else {
        ste.linkname+=string(reinterpret_cast<char *>(buf),1);
      }
    }
    fs.seekg(curr_off,std::ios_base::beg);
#ifdef __DEBUG
    cerr << "  link name is '" << ste.linkname << "'" << endl;
#endif
  }
  fs.read(cbuf,sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.offsets)) {
    append(myerror, "read error for symbol table entry object header address",
        ", ");
    return false;
  }
  ste.objhdr_addr=HDF5::value(buf,sizes.offsets);
#ifdef __DEBUG
  cerr << "object header address: " << ste.objhdr_addr << endl;
#endif
  fs.read(cbuf,8);
  if (fs.gcount() != 8) {
    append(myerror, "read error for symbol table entry cache_type", ", ");
    return false;
  }
  ste.cache_type=HDF5::value(buf,4);
  fs.read(reinterpret_cast<char *>(ste.scratchpad),16);
  if (fs.gcount() != 16) {
    append(myerror, "read error for symbol table entry scratchpad", ", ");
    return false;
  }
#ifdef __DEBUG
  cerr << "-- DONE: SYMBOL TABLE ENTRY" << endl;
#endif
  return true;
}

bool InputHDF5Stream::decode_symbol_table_node(unsigned long long address,Group& group)
{
  unsigned long long curr_off=fs.tellg();
  fs.seekg(address,std::ios_base::beg);
  unsigned char buf[64];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,8);
  if (fs.gcount() != 8) {
    append(myerror, "error reading first 8 bytes of symbol table node", ", ");
    return false;
  }
  if (string(cbuf,4) != "SNOD") {
    append(myerror, "bad signature '" + string(cbuf, 4) + "' for symbol table "
        "node", ", ");
    return false;
  }
  size_t num_symbols=HDF5::value(&buf[6],2);
  SymbolTableEntry ste;
  for (size_t n=0; n < num_symbols; ++n) {
    if (!decode_symbol_table_entry(ste,&group)) {
      return false;
    }
    if (!decode_object_header(ste,&group)) {
      return false;
    }
  }
  fs.seekg(curr_off,std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decode_v1_Btree(Group& group)
{
  auto curr_off=fs.tellg();
  unsigned char buf[8];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,4);
  if (fs.gcount() != 4) {
    append(myerror, "read error bytes 5-8 in v1 tree", ", ");
    return false;
  }
  group.tree.node_type=static_cast<int>(buf[0]);
  if (group.tree.node_type != 0) {
    append(myerror, "can't decode chunk node tree as group node tree", ", ");
    return false;
  }
  group.tree.node_level=static_cast<int>(buf[1]);
  size_t num_children=HDF5::value(&buf[2],2);
#ifdef __DEBUG
  cerr << "v1 Tree GROUP node_type=" << group.tree.node_type << " node_level=" << group.tree.node_level << " num_children=" << num_children << endl;
#endif
  fs.read(cbuf,sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.offsets)) {
    append(myerror, "read error for address of left sibling", ", ");
    return false;
  }
  group.tree.left_sibling_addr=HDF5::value(buf,sizes.offsets);
#ifdef __DEBUG
  cerr << "address of left sibling: " << group.tree.left_sibling_addr << endl;
#endif
  fs.read(cbuf,sizes.offsets);
  if (fs.gcount() != static_cast<int>(sizes.offsets)) {
    append(myerror, "read error for address of right sibling", ", ");
    return false;
  }
  group.tree.right_sibling_addr=HDF5::value(buf,sizes.offsets);
#ifdef __DEBUG
  cerr << "address of right sibling: " << group.tree.right_sibling_addr << endl;
#endif
  fs.read(cbuf,sizes.lengths);
  if (fs.gcount() != static_cast<int>(sizes.lengths)) {
    append(myerror, "read error for throwaway key", ", ");
    return false;
  }
  for (size_t n=0; n < num_children; ++n) {
    fs.read(cbuf,sizes.offsets);
    if (fs.gcount() != static_cast<int>(sizes.offsets)) {
      append(myerror, "read error for address of child " + to_string(n), ", ");
      return false;
    }
    long long chld_p=HDF5::value(buf,sizes.offsets);
    group.tree.child_pointers.emplace_back(chld_p);
    fs.read(cbuf,sizes.lengths);
    if (fs.gcount() != static_cast<int>(sizes.lengths)) {
      append(myerror, "read error for key " + to_string(n), ", ");
      return false;
    }
    group.tree.keys.emplace_back(HDF5::value(buf,sizes.lengths));
    if (!decode_symbol_table_node(chld_p,group)) {
      return false;
    }
  }
  fs.seekg(curr_off,std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decode_v1_Btree(unsigned long long address,Dataset& dataset)
{
  unsigned long long ldum;
  if (address == undef_addr) {
    append(myerror, "tree is not defined", ", ");
    return false;
  }
  auto curr_off=fs.tellg();
  fs.seekg(address,std::ios_base::beg);
  std::streamsize num_to_read=8+sizes.offsets*2;
  unsigned char *buf=new unsigned char[num_to_read];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,num_to_read);
  if (fs.gcount() != num_to_read) {
    append(myerror, "error reading v1 B-tree", ", ");
    return false;
  }
  if (string(cbuf,4) != "TREE") {
    append(myerror, "not a v1 B-tree", ", ");
    return false;
  }
  auto node_type=static_cast<int>(buf[4]);
  if (node_type != 1) {
    append(myerror, "can't decode group node tree as chunk node tree", ", ");
    return false;
  }
  auto node_level=static_cast<int>(buf[5]);
  size_t num_children=HDF5::value(&buf[6],2);
#ifdef __DEBUG
  cerr << "v1 Tree CHUNK node_type=" << node_type << " node_level=" << node_level << " num_children=" << num_children << endl;
#endif
  ldum=HDF5::value(&buf[8],sizes.offsets);
#ifdef __DEBUG
  cerr << "address of left sibling: " << ldum << endl;
#endif
  ldum=HDF5::value(&buf[8+sizes.offsets],sizes.offsets);
#ifdef __DEBUG
  cerr << "address of right sibling: " << ldum << endl;
#endif
  delete[] buf;
  num_to_read=num_children*(8+8*dataset.data.sizes.size()+sizes.offsets*2);
  buf=new unsigned char[num_to_read];
  fs.read(reinterpret_cast<char *>(buf),num_to_read);
  if (fs.gcount() != num_to_read) {
    append(myerror, "unable to decode v1 B-tree", ", ");
    return false;
  }
  auto boff=0;
  for (size_t n=0; n < num_children; ++n) {
    size_t chunk_size=HDF5::value(&buf[boff],4);
    auto fmask = HDF5::value(&buf[boff+4], 4);
    boff+=8;
#ifdef __DEBUG
    cerr << "chunk size: " << chunk_size;
#endif
    size_t chunk_len=1;
    vector<size_t> offsets;
    for (size_t m=0; m < dataset.data.sizes.size(); ++m) {
      chunk_len*=dataset.data.sizes[m];
      offsets.emplace_back(HDF5::value(&buf[boff],8));
      boff+=8;
#ifdef __DEBUG
      cerr << " dimension " << m << " offset: " << offsets.back();
#endif
    }
    chunk_len*=dataset.data.size_of_element;
    ldum=HDF5::value(&buf[boff],sizes.offsets);
    boff+=sizes.offsets;
    if (ldum != 0) {
      append(myerror, "datatype offset for child " + to_string(n) + " is NOT "
          "zero", ", ");
      return false;
    }
    ldum=HDF5::value(&buf[boff],sizes.offsets);
    boff+=sizes.offsets;
#ifdef __DEBUG
    cerr << " pointer: " << ldum << " chunk length: " << chunk_len << " #filters: " << dataset.filters.size() << " " << time(NULL) << endl;
#endif
/*
    char cbuf[4];
    fs.seekg(ldum, std::ios_base::beg);
    fs.read(cbuf,4);
    if (fs.gcount() != 4) {
      append(myerror, "error reading test bytes for child " + to_string(n),
          ", ");
      return false;
    }
    if (string(cbuf, 4) == "TREE") {
*/
    if (node_level > 0) {
#ifdef __DEBUG
      cerr << "ANOTHER TREE" << endl;
#endif
      decode_v1_Btree(ldum, dataset);
    } else {
      dataset.data.chunks.emplace_back(ldum, chunk_size, chunk_len, offsets);
    }
  }
  fs.seekg(curr_off, std::ios_base::beg);
  delete[] buf;
  return true;
}

bool InputHDF5Stream::decode_v2_Btree(Group& group, FractalHeapData
    *frhp_data) {
  unsigned long long curr_off;

  curr_off=fs.tellg();
  unsigned char buf[64];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,12);
  if (fs.gcount() != 12) {
    append(myerror, "read error bytes 5-16 in v2 tree", ", ");
    return false;
  }
#ifdef __DEBUG
  cerr << "v2 BTree:" << endl;
  cerr << "version: " << static_cast<int>(buf[0]) << " type: " << static_cast<int>(buf[1]) << " node size: " << HDF5::value(&buf[2],4) << " record size: " << HDF5::value(&buf[6],2) << " depth: " << HDF5::value(&buf[8],2) << " split %: " << static_cast<int>(buf[10]) << " merge %: " << static_cast<int>(buf[11]) << endl;
#endif
  fs.read(cbuf,sizes.offsets+2+sizes.lengths);
  if (fs.gcount() != static_cast<int>(sizes.offsets+2+sizes.lengths)) {
    append(myerror, "unable to read v2 tree root node data", ", ");
    return false;
  }
#ifdef __DEBUG
  cerr << "root address: " << HDF5::value(buf,sizes.offsets) << " # recs in root node: " << HDF5::value(&buf[sizes.offsets],2) << " # recs in tree: " << HDF5::value(&buf[sizes.offsets+2],sizes.lengths) << endl;
#endif
  if (!decode_v2_Btree_node(HDF5::value(buf,sizes.offsets),HDF5::value(&buf[sizes.offsets],2),frhp_data)) {
    return false;
  }
  fs.seekg(curr_off,std::ios_base::beg);
  return true;
}

bool InputHDF5Stream::decode_v2_Btree_node(unsigned long long address,int num_records,FractalHeapData *frhp_data)
{
  int n,max_size,len;

  fs.seekg(address,std::ios_base::beg);
  unsigned char buf[64];
  char *cbuf=reinterpret_cast<char *>(buf);
  fs.read(cbuf,4);
  if (fs.gcount() != 4) {
    append(myerror, "unable to read v2 btree node signature", ", ");
    return false;
  }
  auto signature=string(cbuf,4);
  if (signature == "BTLF") {
    fs.read(cbuf,2);
    if (fs.gcount() != 2) {
      append(myerror, "unable to read v2 btree version and type", ", ");
      return false;
    }
    if (buf[0] != 0) {
      append(myerror, "unable to decode v2 btree node version " + to_string(
          static_cast<int>(buf[0])), ", ");
      return false;
    }
    switch (buf[1]) {
      case 5: {
        for (n=0; n < num_records; ++n) {
          fs.read(cbuf,11);
          if (fs.gcount() != 11) {
            append(myerror, "unable to read v2 btree record #" + to_string(n),
                ", ");
            return false;
          }
          if ( (buf[4] & 0xc0) != 0) {
            append(myerror, "unable to decode version " + to_string(buf[4] &
                0xc0) + " IDs", ", ");
            return false;
          }
          switch (buf[4] & 0x30) {
            case 0: {
              max_size= (frhp_data->max_dblock_size < frhp_data->max_managed_obj_size) ? frhp_data->max_dblock_size : frhp_data->max_managed_obj_size;
              len=1;
              while (max_size/256 > 0) {
                ++len;
                max_size/=256;
              }
#ifdef __DEBUG
              cerr << "BTLF rec #" << n << "  offset: " << HDF5::value(&buf[5],frhp_data->max_size/8) << "  length: " << HDF5::value(&buf[5+frhp_data->max_size/8],len) << endl;
#endif
              break;
            }
            default: {
              append(myerror, "unable to decode type " + to_string(buf[4] &
                  0x30) + " IDs", ", ");
              return false;
            }
          }
        }
        break;
      }
      default: {
        append(myerror, "unable to decode v2 btree node type " + to_string(
            static_cast<int>(buf[1])), ", ");
        return false;
      }
    }
    return true;
  } else if (signature == "BTIN") {
#ifdef __DEBUG
    cerr << "BTIN decode not defined!" << endl;
#endif
    return true;
  } else {
    append(myerror, "unable to decode v2 btree node with signature '" + string(
        cbuf, 4) + "'", ", ");
    return false;
  }
}

void InputHDF5Stream::print_data(Dataset& dataset) {
  unsigned long long curr_off;
  unsigned char *buf;
  DataValue value;

  if (dataset.data.address == undef_addr) {
    append(myerror, "Data not defined", ", ");
    return;
  }
  curr_off = fs.tellg();
  fs.seekg(dataset.data.address, std::ios_base::beg);
  char test[4];
  fs.read(test, 4);
  if (fs.gcount() != 4) {
    cout << "Data read error";
  } else {
    if (string(test,4) == "TREE") {

      // chunked data
#ifdef __DEBUG
      cerr << "CHUNKED DATA! " << dataset.data.chunks.size() << endl;
      for (size_t n=0; n < dataset.data.sizes.size(); ++n) {
        cerr << n << " " << dataset.data.sizes[n] << endl;
      }
      for (size_t n=0; n < dataset.data.chunks.size(); ++n) {
        cerr << n << " " << dataset.datatype.class_ << endl;
        dump(dataset.data.chunks[n].buffer.get(),dataset.data.chunks[n].length);
        for (size_t m=0; m < dataset.data.chunks[n].length; m+=dataset.data.size_of_element) {
          value.set(fs,&dataset.data.chunks[n].buffer[m],sizes.offsets,sizes.lengths,dataset.datatype,dataset.dataspace);
          value.print(cerr,ref_table);
          cerr << endl;
        }
      }
#endif
    } else {

      // contiguous data
#ifdef __DEBUG
      cerr << "CONTIGUOUS DATA!" << endl;
#endif
      if (dataset.dataspace.dimensionality == 0) {
        buf=new unsigned char[dataset.data.sizes[0]];
        copy(test,test+4,buf);
        fs.read(reinterpret_cast<char *>(buf),dataset.data.sizes[0]-4);
        if (fs.gcount() != static_cast<int>(dataset.data.sizes[0]-4)) {
          cout << "Data read error";
        } else {
          HDF5::print_data_value(dataset.datatype,buf);
        }
        delete[] buf;
      } else {
        cout << "Unable to read dimensional contiguous data";
      }
    }
  }
  fs.seekg(curr_off,std::ios_base::beg);
}

void InputHDF5Stream::print_a_group_tree(Group& group) {
  static Group *root = &group;
  static string indent = "";
  if (indent.empty()) {
    cout << "HDF5 File Group Tree:" << endl;
    cout << "/" << endl;
  }
  indent += "  ";
  DatasetEntry dse;
  for (const auto& e : group.groups) {
    cout << indent << e.key << "/" << endl;
    print_a_group_tree(*e.p_g);
    for (auto& dse : e.p_g->datasets) {
      cout << indent;
      if (dse.p_ds->datatype.class_ < 0) {
        cout << "  Group: " << dse.key << endl;
      } else {
        cout << "  Dataset: " << dse.key << endl;
        cout << indent << "    Datatype: " << HDF5::datatype_class_to_string(
            dse.p_ds->datatype)<< endl;
      }
      for (const auto& a : dse.p_ds->attributes) {
        cout << indent << "    ";
        HDF5::print_attribute(a, ref_table);
        cout << endl;
      }
    }
  }
  chop(indent, 2);
  if (&group == root) {
    auto it = find_if(group.datasets.begin(), group.datasets.end(),
        FindDataset(""));
    if (it != group.datasets.end()) {
      for (const auto& a : it->p_ds->attributes) {
        cout << indent << "  ";
        HDF5::print_attribute(a, ref_table);
        cout << endl;
      }
    }
    for (const auto& e : group.datasets) {
      cout << indent;
      if (e.p_ds->datatype.class_ < 0) {
        cout << "  Group: " << e.key << endl;
      } else {
        cout << "  Dataset: " << e.key << endl;
        cout << indent << "    Datatype: " << HDF5::datatype_class_to_string(e.
            p_ds->datatype) << endl;
      }
      for (const auto& attr_entry : e.p_ds->attributes) {
        cout << indent << "    ";
        HDF5::print_attribute(attr_entry,ref_table);
        cout << endl;
      }
    }
  }
}
