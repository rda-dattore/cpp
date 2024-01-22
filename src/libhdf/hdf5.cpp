#include <hdf.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>
#include <myerror.hpp>

using Attribute = InputHDF5Stream::Attribute;
using Chunk = InputHDF5Stream::Chunk;
using CompoundDatatype = InputHDF5Stream::CompoundDatatype;
using Dataset = InputHDF5Stream::Dataset;
using Dataspace = InputHDF5Stream::Dataspace;
using Datatype = InputHDF5Stream::Datatype;
using DataValue = InputHDF5Stream::DataValue;
using std::cerr;
using std::copy;
using std::cout;
using std::endl;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using strutils::append;

namespace HDF5 {

int data_array_index(size_t chunk_index, const std::deque<size_t>& multipliers,
    const vector<size_t> offsets, const vector<unsigned long long>&
    array_dimensions) {
  int index = 0;
  for (size_t n = 0, end = multipliers.size()-1; n < end; ++n) {
    size_t i = (chunk_index % multipliers[n]) / multipliers[n+1] + offsets[n];
    if (i >= array_dimensions[n]) {
      return -1;
    }
    if (n < array_dimensions.size()-1) {
      index += i * array_dimensions[n+1];
    } else {
      index += i;
    }
  }
  return index;
}

bool decode_class0_array(DataArray& darray, const Dataset& dataset, size_t
    chunk_number, const CompoundDatatype::Member *compound_datatype_member,
    const vector<size_t>& dimensions) {
  unsigned char *buf = const_cast<unsigned char *>(dataset.data.chunks[
      chunk_number].buffer.get());
  if (compound_datatype_member != nullptr) {
    buf += compound_datatype_member->byte_offset;
  }
  auto &datatype = (compound_datatype_member == nullptr) ? dataset.datatype :
      compound_datatype_member->datatype;
  short off = value(&datatype.properties[0], 2);
  short bit_length = value(&datatype.properties[2], 2);
  short byte_order;
  bits::get(datatype.bit_fields, byte_order, 7, 1);
  size_t nvals = dataset.data.chunks[chunk_number].length / dataset.data.
      size_of_element;
  if (off == 0 && (bit_length % 8) == 0) {
    std::deque<size_t> multipliers;
    if (dataset.dataspace.dimensionality > 1) {
      size_t mult = 1;
      multipliers.emplace_front(mult);
      for (int n = dataset.data.sizes.size()-1; n >= 0; --n) {
        mult *= dataset.data.sizes[n];
        multipliers.emplace_front(mult);
      }
    }
    auto byte_length = bit_length / 8;
    switch (byte_length) {
      case 1: {
        if (darray.values == nullptr) {
          darray.values = new unsigned char[darray.num_values];
        }
        if (byte_order == 0) {
          if (dataset.dataspace.dimensionality == 1) {

            // one-dimension array
            for (size_t n = 0, m = dataset.data.chunks[chunk_number].offsets[0];
                n < nvals; ++n, ++m) {
              if (m < darray.num_values) {
                (reinterpret_cast<unsigned char *>(darray.values))[m] = value(
                    buf, byte_length);
              }
              buf += dataset.data.size_of_element;
            }
          } else {

            // multi-dimensional array
            for (size_t n = 0; n < nvals; ++n) {
              auto idx = data_array_index(n, multipliers, dataset.data.chunks[
                  chunk_number].offsets, dataset.dataspace.sizes);
              if (idx >= 0) {
                (reinterpret_cast<unsigned char *>(darray.values))[idx] = value(
                    buf, byte_length);
              }
              buf += dataset.data.size_of_element;
            }
          }
        } else {
          if (dataset.dataspace.dimensionality == 1) {

            // one-dimension array
            auto end = nvals + dataset.data.chunks[chunk_number].offsets[0];
            if (end > darray.num_values) {
              nvals -= end - darray.num_values;
            }
            bits::get(buf, &(reinterpret_cast<unsigned char *>(darray.values)[
                dataset.data.chunks[chunk_number].offsets[0]]), 0, bit_length,
                0, nvals);
          } else {

            // multi-dimensional array
            for (size_t n = 0; n < nvals; ++n) {
              auto idx = data_array_index(n, multipliers, dataset.data.chunks[
                  chunk_number].offsets, dataset.dataspace.sizes);
              if (idx >= 0) {
                bits::get(buf, (reinterpret_cast<unsigned char *>(darray.
                    values))[idx], 0, bit_length);
              }
              buf += dataset.data.size_of_element;
            }
          }
        }
        break;
      }
      case 2: {
        if (darray.values == nullptr) {
          darray.values = new short[darray.num_values];
          short fill_value = decode_data_value(datatype, dataset.fillvalue.
              bytes, DataArray::default_missing_value);
          for (size_t n = 0; n < darray.num_values; ++n) {
            (reinterpret_cast<short *>(darray.values))[n] = fill_value;
          }
          darray.type = DataArray::Type::SHORT;
        }
        if (byte_order == 0) {
          if (dataset.dataspace.dimensionality == 1) {

            // one-dimension array
            for (size_t n = 0, m = dataset.data.chunks[chunk_number].offsets[0];
                n < nvals; ++n, ++m) {
              if (m < darray.num_values) {
                (reinterpret_cast<short *>(darray.values))[m] = value(buf,
                    byte_length);
              }
              buf += dataset.data.size_of_element;
            }
          } else {

            // multi-dimensional array
            for (size_t n = 0; n < nvals; ++n) {
              auto idx = data_array_index(n, multipliers, dataset.data.chunks[
                  chunk_number].offsets, dataset.dataspace.sizes);
              if (idx >= 0) {
                (reinterpret_cast<short *>(darray.values))[idx] = value(buf,
                    byte_length);
              }
              buf += dataset.data.size_of_element;
            }
          }
        } else {
          if (dataset.dataspace.dimensionality == 1) {

            // one-dimension array
            auto end = nvals + dataset.data.chunks[chunk_number].offsets[0];
            if (end > darray.num_values) {
              nvals -= end - darray.num_values;
            }
            bits::get(buf, &(reinterpret_cast<short *>(darray.values)[dataset.
                data.chunks[chunk_number].offsets[0]]), 0, bit_length, 0,
                nvals);
          } else {

            // multi-dimensional array
            for (size_t n = 0; n < nvals; ++n) {
              auto idx = data_array_index(n, multipliers, dataset.data.chunks[
                  chunk_number].offsets, dataset.dataspace.sizes);
              if (idx >= 0) {
                bits::get(buf, (reinterpret_cast<short *>(darray.values))[idx],
                    0, bit_length);
              }
              buf += dataset.data.size_of_element;
            }
          }
        }
        break;
      }
      case 4: {
        if (darray.values == nullptr) {
          darray.values = new int[darray.num_values];
          int fill_value = decode_data_value(datatype, dataset.fillvalue.bytes,
              DataArray::default_missing_value);
          for (size_t n = 0; n < darray.num_values; ++n) {
            (reinterpret_cast<int *>(darray.values))[n] = fill_value;
          }
          darray.type = DataArray::Type::INT;
        }
        if (byte_order == 0) {
          if (dataset.dataspace.dimensionality == 1) {

            // one-dimension array
            for (size_t n = 0, m = dataset.data.chunks[chunk_number].offsets[0];
                n < nvals; ++n, ++m) {
              if (m < darray.num_values) {
                (reinterpret_cast<int *>(darray.values))[m] = value(buf,
                    byte_length);
              }
              buf += dataset.data.size_of_element;
            }
          } else {

            // multi-dimensional array
            for (size_t n = 0; n < nvals; ++n) {
              auto idx = data_array_index(n, multipliers, dataset.data.chunks[
                  chunk_number].offsets, dataset.dataspace.sizes);
              if (idx >= 0) {
                (reinterpret_cast<int *>(darray.values))[idx] = value(buf,
                    byte_length);
              }
              buf += dataset.data.size_of_element;
            }
          }
        } else {
          if (dataset.dataspace.dimensionality == 1) {

            // one-dimension array
            auto end = nvals + dataset.data.chunks[chunk_number].offsets[0];
            if (end > darray.num_values) {
              nvals -= end - darray.num_values;
            }
            bits::get(buf, &(reinterpret_cast<int *>(darray.values)[dataset.
                data.chunks[chunk_number].offsets[0]]), 0, bit_length, 0,
                nvals);
          } else {

            // multi-dimensional array
            for (size_t n = 0; n < nvals; ++n) {
              auto idx = data_array_index(n, multipliers, dataset.data.chunks[
                  chunk_number].offsets, dataset.dataspace.sizes);
              if (idx >= 0) {
                bits::get(buf, (reinterpret_cast<int *>(darray.values))[idx], 0,
                    bit_length);
              }
              buf += dataset.data.size_of_element;
            }
          }
        }
        break;
      }
      case 8: {
        if (darray.values == nullptr) {
          darray.values = new long long[darray.num_values];
          long long fill_value = decode_data_value(datatype, dataset.fillvalue.
              bytes, DataArray::default_missing_value);
          for (size_t n = 0; n < darray.num_values; ++n) {
            (reinterpret_cast<long long *>(darray.values))[n] = fill_value;
          }
          darray.type = DataArray::Type::LONG_LONG;
        }
        if (byte_order == 0) {
          if (dataset.dataspace.dimensionality == 1) {

            // one-dimension array
            for (size_t n = 0, m = dataset.data.chunks[chunk_number].offsets[0];
                n < nvals; ++n, ++m) {
              if (m < darray.num_values) {
                (reinterpret_cast<long long *>(darray.values))[m] = value(buf,
                    byte_length);
              }
              buf += dataset.data.size_of_element;
            }
          } else {

            // multi-dimensional array
            for (size_t n = 0; n < nvals; ++n) {
              auto idx = data_array_index(n, multipliers, dataset.data.chunks[
                  chunk_number].offsets, dataset.dataspace.sizes);
              if (idx >= 0) {
                (reinterpret_cast<long long *>(darray.values))[idx] = value(buf,
                    byte_length);
              }
              buf += dataset.data.size_of_element;
            }
          }
        } else {
          if (dataset.dataspace.dimensionality == 1) {

            // one-dimension array
            auto end = nvals + dataset.data.chunks[chunk_number].offsets[0];
            if (end > darray.num_values) {
              nvals -= end - darray.num_values;
            }
            bits::get(buf, &(reinterpret_cast<long long *>(darray.values)[
                dataset.data.chunks[chunk_number].offsets[0]]), 0, bit_length,
                0, nvals);
          } else {

            // multi-dimensional array
            for (size_t n = 0; n < nvals; ++n) {
              auto idx = data_array_index(n, multipliers, dataset.data.chunks[
                  chunk_number].offsets, dataset.dataspace.sizes);
              if (idx >= 0) {
                bits::get(buf, (reinterpret_cast<long long *>(darray.values))[
                    idx], 0, bit_length);
              }
              buf += dataset.data.size_of_element;
            }
          }
        }
        break;
      }
    }
    return true;
  } else {
    append(myerror, "bad offset " + to_string(off) + " or bit length " +
        to_string(bit_length) + " when trying to fill data array", ", ");
    return false;
  }
}

bool decode_class1_array(DataArray& darray, const Dataset& dataset, size_t
    chunk_number, const CompoundDatatype::Member *compound_datatype_member) {
  unsigned char *buf = const_cast<unsigned char *>(dataset.data.chunks[
      chunk_number].buffer.get());
  if (compound_datatype_member != nullptr) {
    buf += compound_datatype_member->byte_offset;
  }
  auto &datatype = (compound_datatype_member == nullptr) ? dataset.datatype :
      compound_datatype_member->datatype;
  short bit_length = value(&datatype.properties[2], 2);
  short byte_order[2];
  bits::get(datatype.bit_fields, byte_order[0], 1, 1);
  bits::get(datatype.bit_fields, byte_order[1], 7, 1);
  size_t nvals = dataset.data.chunks[chunk_number].length / dataset.data.
      size_of_element;
/*
if (nvals > darray.num_values) {
darray.num_values=nvals;
}
*/
  std::deque<size_t> multipliers;
  if (dataset.dataspace.dimensionality > 1) {
    size_t mult = 1;
    multipliers.emplace_front(mult);
    for (int n = dataset.data.sizes.size()-1; n >= 0; --n) {
      mult *= dataset.data.sizes[n];
      multipliers.emplace_front(mult);
    }
  }
  if (byte_order[0] == 0 && byte_order[1] == 0) {

    // little-endian
    if (!unixutils::system_is_big_endian()) {
      if (bit_length == 32 && datatype.properties[5] == 8 && datatype.
          properties[6] == 0 && datatype.properties[4] == 23) {

        // IEEE single precision
        if (darray.values == nullptr) {
          darray.values = new float[darray.num_values];
          float fill_value = decode_data_value(datatype, dataset.fillvalue.
              bytes, DataArray::default_missing_value);
          for (size_t n = 0; n < darray.num_values; ++n) {
            (reinterpret_cast<float *>(darray.values))[n] = fill_value;
          }
          darray.type = DataArray::Type::FLOAT;
        }
        if (dataset.dataspace.dimensionality == 1) {

          // one-dimension array
          for (size_t n = 0, m = dataset.data.chunks[chunk_number].offsets[0]; n
              < nvals; ++n, ++m) {
            if (m < darray.num_values) {
              (reinterpret_cast<float *>(darray.values))[m] = (reinterpret_cast<
                  float *>(buf))[0];
            }
            buf += dataset.data.size_of_element;
          }
        } else {

          // multi-dimensional array
          for (size_t n = 0; n < nvals; ++n) {
            auto idx = data_array_index(n, multipliers, dataset.data.chunks[
                chunk_number].offsets, dataset.dataspace.sizes);
            if (idx >= 0) {
              (reinterpret_cast<float *>(darray.values))[idx] =
                  (reinterpret_cast<float *>(buf))[0];
            }
            buf += dataset.data.size_of_element;
          }
        }
      } else if (bit_length == 64 && datatype.properties[5] == 11 && datatype.
          properties[6] == 0 && datatype.properties[4] == 52) {

        // IEEE double-precision
        if (darray.values == nullptr) {
          darray.values = new double[darray.num_values];
          double fill_value = decode_data_value(datatype, dataset.fillvalue.
              bytes, DataArray::default_missing_value);
          for (size_t n = 0; n < darray.num_values; ++n) {
            (reinterpret_cast<double *>(darray.values))[n] = fill_value;
          }
          darray.type = DataArray::Type::DOUBLE;
        }
        if (dataset.dataspace.dimensionality == 1) {

          // one-dimensional array
          for (size_t n = 0, m = dataset.data.chunks[chunk_number].offsets[0]; n
              < nvals; ++n, ++m) {
            if (m < darray.num_values) {
              (reinterpret_cast<double *>(darray.values))[m] =
                  (reinterpret_cast<double *>(buf))[0];
            }
            buf += dataset.data.size_of_element;
          }
        } else {

          // multi-dimensional array
          for (size_t n = 0; n < nvals; ++n) {
            auto idx = data_array_index(n, multipliers, dataset.data.chunks[
                chunk_number].offsets, dataset.dataspace.sizes);
            if (idx >= 0) {
              (reinterpret_cast<double *>(darray.values))[idx] =
                  (reinterpret_cast<double *>(buf))[0];
            }
            buf += dataset.data.size_of_element;
          }
        }
      } else {
        append(myerror, "can't decode float 0A", ", ");
        return false;
      }
    } else {
      append(myerror, "can't decode float A", ", ");
      return false;
    }
  } else if (byte_order[0] == 0 && byte_order[1] == 1) {

    // big-endian
    if (unixutils::system_is_big_endian()) {
      append(myerror, "can't decode float B", ", ");
      return false;
    } else {
      append(myerror, "can't decode float C", ", ");
      return false;
    }
  } else {
    append(myerror, "unknown byte order for data value", ", ");
    return false;
  }
  return true;
}

bool decode_string_array(const Chunk& chunk, int buffer_off, int
    size_of_element, void **values, size_t num_values, DataArray::Type& type,
    size_t& index) {
  auto buf = const_cast<unsigned char *>(&chunk.buffer[buffer_off]);
  if (*values == nullptr) {
    *values = new string[num_values];
    for (size_t n = 0; n < num_values; ++n) {
      reinterpret_cast<string *>(*values)[n].assign(size_of_element, ' ');
    }
    type = DataArray::Type::STRING;
  }
  size_t end = index + chunk.length / size_of_element;
  if (end > num_values) {
    end = num_values;
  }
  for (; index < end; ++index, buf += size_of_element) {
    reinterpret_cast<string *>(*values)[index].replace(0, size_of_element,
        reinterpret_cast<char *>(buf), size_of_element);
  }
  return true;
}

bool decode_2D_string_array(const Chunk& chunk, int buffer_off, const Dataset&
    dataset, void **values, DataArray::Type& type) {
  auto index = chunk.offsets.front();
  auto buf = const_cast<unsigned char *>(&chunk.buffer[buffer_off]);
  if (*values == nullptr) {
    *values = new string[dataset.dataspace.sizes.front()];
    for (size_t n = 0; n < dataset.dataspace.sizes.front(); ++n) {
      reinterpret_cast<string *>(*values)[n].assign(dataset.dataspace.sizes.
          back(), ' ');
    }
    type = DataArray::Type::STRING;
  }
  size_t end = index + chunk.length / dataset.data.sizes.back();
  if (end > dataset.dataspace.sizes.front()) {
    end = dataset.dataspace.sizes.front();
  }
  auto num_to_copy = dataset.data.sizes.back();
  while (num_to_copy > 0 && chunk.offsets.back() + num_to_copy > dataset.
      dataspace.sizes.back()) {
    --num_to_copy;
  }
  for (; index < end; ++index, buf += dataset.data.sizes.back()) {
    reinterpret_cast<string *>(*values)[index].replace(chunk.offsets.back(),
        num_to_copy, reinterpret_cast<char *>(buf), num_to_copy);
  }
  return true;
}

size_t cast_data_values(const DataValue& data_value, void **array_values,
    size_t& start_array_index, size_t num_array_values, DataArray::Type& type,
    size_t size_of_element) {
  size_t num_to_cast = data_value.size/size_of_element;
  switch (data_value._class_) {
    case 0: {
      switch (data_value.precision_) {
        case 8: {
          if (*array_values == nullptr) {
            *array_values = new unsigned char[num_array_values];
            type = DataArray::Type::BYTE;
          }
          for (size_t n = 0; n < num_to_cast; ++n) {
            (reinterpret_cast<unsigned char *>(*array_values))[
                start_array_index++] = (reinterpret_cast<unsigned char *>(
                data_value.array))[n];
          }
          break;
        }
        case 16: {
          if (*array_values == nullptr) {
            *array_values = new short[num_array_values];
            type = DataArray::Type::SHORT;
          }
          for (size_t n = 0; n < num_to_cast; ++n) {
            (reinterpret_cast<short *>(*array_values))[start_array_index++] =
                (reinterpret_cast<short *>(data_value.array))[n];
          }
          break;
        }
        case 32: {
          if (*array_values == nullptr) {
            *array_values = new int[num_array_values];
            type = DataArray::Type::INT;
          }
          for (size_t n = 0; n < num_to_cast; ++n) {
            (reinterpret_cast<int *>(*array_values))[start_array_index++] =
                (reinterpret_cast<int *>(data_value.array))[n];
          }
          break;
        }
        case 64: {
          if (*array_values == nullptr) {
            *array_values = new long long[num_array_values];
            type = DataArray::Type::LONG_LONG;
          }
          for (size_t n = 0; n < num_to_cast; ++n) {
            (reinterpret_cast<int *>(*array_values))[start_array_index++] =
                (reinterpret_cast<long long *>(data_value.array))[n];
          }
          break;
        }
        default: {
          num_to_cast = 0;
        }
      }
      break;
    }
    case 1: {
      if (size_of_element <= 4) {
        if (*array_values == nullptr) {
          *array_values = new float[num_array_values];
          type = DataArray::Type::FLOAT;
        }
        for (size_t n = 0; n < num_to_cast; ++n) {
          (reinterpret_cast<float *>(*array_values))[start_array_index++] =
              (reinterpret_cast<float *>(data_value.array))[n];
        }
      } else {
        if (*array_values == nullptr) {
          *array_values = new double[num_array_values];
          type = DataArray::Type::DOUBLE;
        }
        for (size_t n = 0; n < num_to_cast; ++n) {
          (reinterpret_cast<double *>(*array_values))[start_array_index++] =
              (reinterpret_cast<double *>(data_value.array))[n];
        }
      }
      break;
    }
    case 3: {
      if (*array_values == nullptr) {
        *array_values = new string[num_array_values];
        type = DataArray::Type::STRING;
      }
      for (size_t n = 0; n < num_to_cast; ++n) {
        (reinterpret_cast<string *>(*array_values))[start_array_index++].assign(
            &(reinterpret_cast<char *>(data_value.array))[n*size_of_element]);
      }
      break;
    }
    default: {
      num_to_cast = 0;
    }
  }
  return num_to_cast;
}

bool cast_value(const DataValue& data_value, int idx, void **array_values, int
    num_array_values, DataArray::Type& type, int size_of_element) {
  switch (data_value._class_) {
    case 0: {
      switch (data_value.precision_) {
        case 16: {
          if (*array_values == nullptr) {
            *array_values = new short[num_array_values];
            type = DataArray::Type::SHORT;
          }
          (reinterpret_cast<short *>(*array_values))[idx] = *(reinterpret_cast<
              short *>(data_value.array));
          break;
        }
        case 32: {
          if (*array_values == nullptr) {
            *array_values = new int[num_array_values];
            type = DataArray::Type::INT;
          }
          (reinterpret_cast<int *>(*array_values))[idx] = *(reinterpret_cast<
              int *>(data_value.array));
          break;
        }
        case 64: {
          if (*array_values == nullptr) {
            *array_values = new long long[num_array_values];
            type = DataArray::Type::LONG_LONG;
          }
          (reinterpret_cast<long long *>(*array_values))[idx] =
              *(reinterpret_cast<long long *>(data_value.array));
          break;
        }
        default: {
          return false;
        }
      }
      return true;
    }
    case 1: {
      if (size_of_element <= 4) {
        if (*array_values == nullptr) {
          *array_values = new float[num_array_values];
          type = DataArray::Type::FLOAT;
        }
        (reinterpret_cast<float *>(*array_values))[idx] = *(reinterpret_cast<
            float *>(data_value.array));
      } else {
        if (*array_values == nullptr) {
          *array_values = new double[num_array_values];
          type = DataArray::Type::DOUBLE;
        }
        (reinterpret_cast<double *>(*array_values))[idx] = *(reinterpret_cast<
            double *>(data_value.array));
      }
      return true;
    }
    case 3: {
      if (*array_values == nullptr) {
        *array_values = new string[num_array_values];
        type = DataArray::Type::STRING;
      }
      (reinterpret_cast<string *>(*array_values))[idx] = reinterpret_cast<
          char *>(data_value.array);
      return true;
    }
  }
  return false;
}

const double DataArray::default_missing_value = 3.4e38;

DataArray& DataArray::operator=(const DataArray& source) {
  if (this == &source) {
    return *this;
  }
  return *this;
}

bool DataArray::fill(InputHDF5Stream& istream, Dataset& dataset, size_t
    compound_member_index) {
  clear();
  if (dataset.data.address == istream.undefined_address()) {
    append(myerror, "dataset address is undefined", ", ");
    return false;
  }
  auto fs = istream.file_stream();
  auto curr_off = fs->tellg();
  fs->seekg(dataset.data.address, std::ios_base::beg);
  char test[4];
  fs->read(test, 4);
  if (fs->gcount() != 4) {
    cout << "Data read error";
  } else {
    CompoundDatatype cdtype;
    if (dataset.datatype.class_ == 6) {
      decode_compound_datatype(dataset.datatype, cdtype);
    }
    if (string(test, 4) == "TREE") {

      // chunked data
#ifdef __DEBUG
      cerr << "Filling data array from chunked data..." << endl;
#endif
      for (size_t n = 0; n < dataset.data.chunks.size(); ++n) {
        if (dataset.data.chunks[n].buffer == nullptr) {
          if (!dataset.data.chunks[n].fill(*fs, dataset)) {
            exit(1);
          }
        }
      }
      dimensions.resize(dataset.dataspace.sizes.size(), 0);
      for (size_t n = 0; n < dataset.dataspace.sizes.size(); ++n) {
        dimensions[n] = dataset.dataspace.sizes[n];
      }
      num_values = 1;
      for (size_t n = 0; n < dataset.dataspace.sizes.size(); ++n) {
        num_values *= dataset.dataspace.sizes[n];
      }
      for (size_t n=0, idx=0; n < dataset.data.chunks.size(); ++n) {
        switch (dataset.datatype.class_) {
          case 0: {
            decode_class0_array(*this, dataset, n, nullptr, dimensions);
            break;
          }
          case 1: {
            decode_class1_array(*this, dataset, n, nullptr);
            break;
          }
          case 3: {
#ifdef __DEBUG
            cerr << "Decoding string array with dataspace dimensionality of " <<
                dataset.dataspace.dimensionality << "  size of element: " <<
                dataset.dataspace.sizes.back() << " (last dim) or " << dataset.
                data.size_of_element << " (size of element)  # values: " <<
                dataset.dataspace.sizes.front() << " (first dataspace size) or "
                << num_values << " (computed) " << dataset.data.sizes.front() <<
                " " << dataset.data.sizes.back() << endl;
#endif
            if (dataset.dataspace.dimensionality == 2) {
              decode_2D_string_array(dataset.data.chunks[n], 0, dataset,
                  &values, type);
              num_values = dataset.dataspace.sizes.front();
            } else {
              decode_string_array(dataset.data.chunks[n], 0, dataset.data.
                  size_of_element, &values, num_values, type, idx);
            }
            break;
          }
          case 6: {
            int off = 0;
            for (size_t m = 0; m < compound_member_index; ++m) {
              off += cdtype.members[m].datatype.size;
            }
            switch (cdtype.members[compound_member_index].datatype.class_) {
              case 0: {
                decode_class0_array(*this, dataset, n, &cdtype.members[
                    compound_member_index], dimensions);
                break;
              }
              case 1: {
                decode_class1_array(*this, dataset, n, &cdtype.members[
                    compound_member_index]);
                break;
              }
              case 3: {
                decode_string_array(dataset.data.chunks[n], off, dataset.data.
                    size_of_element, &values, num_values, type, idx);
                break;
              }
              default: {
cerr << "DEFAULT " << cdtype.members[compound_member_index].datatype.class_ << endl;
                DataValue v;
                auto nval = 0;
                for (size_t m = 0; m < dataset.data.chunks[n].length; m +=
                    dataset.data.size_of_element) {
                  v.set(*fs, &dataset.data.chunks[n].buffer[m + off], istream.
                      size_of_offsets(), istream.size_of_lengths(), cdtype.
                      members[compound_member_index].datatype, dataset.
                      dataspace);
cerr << idx+nval << " ";
v.print(cerr, nullptr);
cerr << endl;
                  if (!cast_value(v, idx + nval, &values, num_values, type,
                      cdtype.members[compound_member_index].datatype.size)) {
                    append(myerror, "unable to get data for class " + to_string(
                        cdtype.members[compound_member_index].datatype.class_) +
                        " and precision " + to_string(v.precision_), ", ");
                    return false;
                  }
                  ++nval;
                }
                idx += nval;
              }
            }
            break;
          }
          default: {
            append(myerror, "unable to fill data array for class " + to_string(
                dataset.datatype.class_), ", ");
            return false;
          }
        }
      }
    } else {

      // contiguous data
#ifdef __DEBUG
      cerr << "Filling data array from contiguous data..." << endl;
#endif
      num_values = dataset.data.sizes[0] / dataset.datatype.size;
      dataset.data.size_of_element = dataset.datatype.size;
      unique_ptr<unsigned char[]> buffer;
      buffer.reset(new unsigned char[dataset.data.sizes[0]]);
      fs->seekg(-4, std::ios_base::cur);
      int nbytes = 0;
      fs->read(reinterpret_cast<char *>(buffer.get()), dataset.data.sizes[0]);
      if (fs->gcount() != dataset.data.sizes[0]) {
          append(myerror, "contiguous data read error " + to_string(nbytes),
              ", ");
          return false;
      }
      DataValue v;
#ifdef __DEBUG
      cerr << "...array contains " << num_values << " values" << endl;
      cerr << "...datatype size is " << dataset.datatype.size << endl;
      cerr << "...buffer contains " << dataset.data.sizes[0] << " bytes" <<
          endl;
#endif
      size_t start_array_index = 0;
      for (size_t n = 0, m = 0; n < num_values; m+=dataset.datatype.size) {
#ifdef __DEBUG
        cerr << "...setting value #" << n << " from byte " << m << " in buffer"
            << endl;
#endif
        v.set(*fs, &buffer[m], istream.size_of_offsets(), istream.
            size_of_lengths(), dataset.datatype, dataset.dataspace);
        auto nvals = cast_data_values(v, &values, start_array_index, num_values,
            type, dataset.data.size_of_element);
        if (nvals == 0) {
          append(myerror, "unable to get data for class " + to_string(dataset.
              datatype.class_) + " and precision " + to_string(v.precision_),
              ", ");
          return false;
        }
        n += nvals;
      }
    }
  }
  fs->seekg(curr_off, std::ios_base::beg);
  return true;
}

unsigned char DataArray::byte_value(size_t index) const {
  if (index < num_values && type == Type::BYTE) {
    return (reinterpret_cast<unsigned char *>(values))[index];
  } else {
    return 0xff;
  }
}

short DataArray::short_value(size_t index) const {
  if (index < num_values && type == Type::SHORT) {
    return (reinterpret_cast<short *>(values))[index];
  } else {
    return 0x7fff;
  }
}

int DataArray::int_value(size_t index) const {
  if (index < num_values && type == Type::INT) {
    return (reinterpret_cast<int *>(values))[index];
  } else {
    return 0x7fffffff;
  }
}

long long DataArray::long_long_value(size_t index) const {
  if (index < num_values && type == Type::LONG_LONG) {
    return (reinterpret_cast<long long *>(values))[index];
  } else {
    return 0x7fffffffffffffff;
  }
}

float DataArray::float_value(size_t index) const {
  if (index < num_values && type == Type::FLOAT) {
    return (reinterpret_cast<float *>(values))[index];
  } else {
    return 1.e48;
  }
}

double DataArray::double_value(size_t index) const {
  if (index < num_values && type == Type::DOUBLE) {
    return (reinterpret_cast<double *>(values))[index];
  } else {
    return 1.e48;
  }
}

string DataArray::string_value(size_t index) const {
  if (index < num_values && type == Type::STRING) {
    return (reinterpret_cast<string *>(values))[index];
  } else {
    return "?????";
  }
}

double DataArray::value(size_t index) const {
  switch (type) {
    case Type::SHORT: {
      return short_value(index);
    }
    case Type::INT: {
      return int_value(index);
    }
    case Type::LONG_LONG: {
      return long_long_value(index);
    }
    case Type::FLOAT: {
      return float_value(index);
    }
    case Type::DOUBLE: {
      return double_value(index);
    }
    default: {
      return 0.;
    }
  }
}

void DataArray::clear() {
  if (values != nullptr) {
    switch (type) {
      case Type::SHORT: {
        delete[] reinterpret_cast<short *>(values);
        break;
      }
      case Type::INT: {
        delete[] reinterpret_cast<int *>(values);
        break;
      }
      case Type::LONG_LONG: {
        delete[] reinterpret_cast<long long *>(values);
        break;
      }
      case Type::FLOAT: {
        delete[] reinterpret_cast<float *>(values);
        break;
      }
      case Type::DOUBLE: {
        delete[] reinterpret_cast<double *>(values);
        break;
      }
      case Type::STRING: {
        delete[] reinterpret_cast<string *>(values);
        break;
      }
      default: {}
    }
    values = nullptr;
    num_values = 0;
    dimensions.resize(0);
  }
}

size_t decode_compound_datatype_name_and_byte_offset(const unsigned char
    *buffer, const Datatype& datatype, string& name, size_t& byte_offset) {
  size_t bytes_decoded = 0;
  switch (datatype.version) {
    case 1: {
      name = reinterpret_cast<char *>(const_cast<unsigned char *>(buffer));
#ifdef __DEBUG
      cerr << "  name of member : '" << name << "'" << endl;
#endif
      while (static_cast<int>(buffer[bytes_decoded+7]) != 0) {
        bytes_decoded += 8;
      }
      bytes_decoded += 8;
      byte_offset = value(&buffer[bytes_decoded], 4);
#ifdef __DEBUG
      cerr << "  name ends at off " << bytes_decoded << endl;
      cerr << "  byte offset of member within data: " << byte_offset << endl;
#endif
      bytes_decoded += 4;
      bytes_decoded += 12;
      for (size_t n = 0; n < 4; ++n) {
#ifdef __DEBUG
        cerr << "  dim " << n << " size: " << value(&buffer[bytes_decoded], 4)
            << endl;
#endif
        bytes_decoded += 4;
      }
      break;
    }
    case 2: {
      name = reinterpret_cast<char *>(const_cast<unsigned char *>(buffer));
#ifdef __DEBUG
      cerr << "  name of member : '" << name << "'" << endl;
#endif
      while (static_cast<int>(buffer[bytes_decoded+7]) != 0) {
        bytes_decoded += 8;
      }
      bytes_decoded += 8;
      byte_offset = value(&buffer[bytes_decoded], 4);
#ifdef __DEBUG
      cerr << "  name ends at off " << bytes_decoded << endl;
      cerr << "  byte offset of member within data: " << byte_offset << endl;
#endif
      bytes_decoded += 4;
      break;
    }
    case 3: {
      name = reinterpret_cast<char *>(const_cast<unsigned char *>(buffer));
#ifdef __DEBUG
      cerr << "  name of member : '" << name << "'" << endl;
#endif
      while (buffer[bytes_decoded] != 0) {
        ++bytes_decoded;
      }
      ++bytes_decoded;
      int m = datatype.size;
      auto l = 1;
      while ( (m / 256) > 0) {
        ++l;
        m /= 256;
      }
      byte_offset = value(&buffer[bytes_decoded], l);
#ifdef __DEBUG
      cerr << "  name ends at off " << bytes_decoded << endl;
      cerr << "  byte offset of member within data: " << byte_offset << endl;
#endif
      bytes_decoded += l;
      break;
    }
    default: {
      name = "";
      byte_offset = 0xffffffff;
    }
  }
  return bytes_decoded;
}

void print_attribute(const Attribute& attribute, shared_ptr<unordered_map<
    size_t, string>> ref_table) {
  cout << "Attribute: " << attribute.first << "  Value: ";
  attribute.second.print(cout, ref_table);
}

void print_data_value(Datatype& datatype, void *value) {
  if (value == nullptr) {
    cout << "?????";
    return;
  }
  switch (datatype.class_) {
    case 0: {

      // fixed point numbers (integers)
      switch (HDF5::value(&datatype.properties[2], 2)) {
        case 8: {
          cout << static_cast<int>(*(reinterpret_cast<unsigned char *>(value)));
          break;
        }
        case 16: {
          cout << *(reinterpret_cast<short *>(value));
          break;
        }
        case 32: {
          cout << *(reinterpret_cast<int *>(value));
          break;
        }
        case 64: {
          cout << *(reinterpret_cast<long long *>(value));
          break;
        }
        default: {
          cout << "?????";
        }
      }
      break;
    }
    case 1: {

      // floating point numbers
      switch (HDF5::value(&datatype.properties[2], 2)) {
        case 32: {
          cout << *(reinterpret_cast<float *>(value));
          break;
        }
        case 64: {
          cout << *(reinterpret_cast<double *>(value));
          break;
        }
        default: {
          cout << "????.?";
        }
      }
      break;
    }
    case 3: {

      // strings
      cout << "\"" << reinterpret_cast<char *>(value) << "\"";
      break;
    }
    case 6: {

      // compound data types
      auto num_members = HDF5::value(datatype.bit_fields, 2);
      auto off = 0;
      cout << num_members << " values: ";
      for (size_t n = 0; n < num_members; ++n) {
        if (n > 0) {
          cout << ", ";
        }
        string member_name;
        size_t member_byte_offset;
        auto bytes_decoded = decode_compound_datatype_name_and_byte_offset(
            &datatype.properties[off], datatype, member_name,
            member_byte_offset);
        if (bytes_decoded > 0) {
          cout << "\"" << member_name << "\": ";
          off += bytes_decoded;
          auto l_datatype = new Datatype;
          decode_datatype(&datatype.properties[off], *l_datatype);
          print_data_value(*l_datatype, value);
          off += 8 + l_datatype->prop_len;
          delete l_datatype;
        } else {
          cout << "\"?????\": ";
        }
      }
      break;
    }
    default: {
      cout << "?????";
    }
  }
}

bool decode_compound_datatype(const Datatype& datatype, CompoundDatatype&
    compound_datatype) {
  auto num_members = value(const_cast<unsigned char *>(datatype.bit_fields), 2);
  compound_datatype.members.clear();
  compound_datatype.members.reserve(num_members);
  auto off = 0;
  for (size_t n = 0; n < num_members; ++n) {
    CompoundDatatype::Member member;
    off += decode_compound_datatype_name_and_byte_offset(&datatype.properties[
        off], datatype, member.name, member.byte_offset);
/*
    switch (datatype.version) {
      case 1: {
        while (static_cast<int>(datatype.properties[off+7]) != 0) {
          off += 8;
        }
        off += 8;
        member.byte_offset = value(&datatype.properties[off], 4);
        off += 4;
        off += 12;
        for (auto m = 0; m < 4; ++m) {
          off += 4;
        }
        break;
      }
      case 2: {
        while (static_cast<int>(datatype.properties[off+7]) != 0) {
          off += 8;
        }
        off += 8;
        member.byte_offset = value(&datatype.properties[off], 4);
        off += 4;
        break;
      }
      case 3: {
        while (datatype.properties[off] != 0) {
          ++off;
        }
        ++off;
        int m = datatype.size;
        auto l = 1;
        while ( (m / 256) > 0) {
          ++l;
          m /= 256;
        }
        member.byte_offset = value(&datatype.properties[off], l);
        off += l;
        break;
      }
    }
*/
    decode_datatype(&datatype.properties[off], member.datatype);
    off += 8 + member.datatype.prop_len;
    compound_datatype.members.emplace_back(member);
  }
  return true;
}

bool decode_compound_data_value(unsigned char *buffer, Datatype& datatype, void
    ***values) {
  *values = nullptr;
return true;
}

bool decode_dataspace(unsigned char *buffer, unsigned long long size_of_lengths,
    Dataspace& dataspace) {
  switch (static_cast<int>(buffer[0])) {
    case 1: {
      dataspace.dimensionality = static_cast<short>(buffer[1]);
      auto off = 8;
#ifdef __DEBUG
      cerr << "version: " << static_cast<int>(buffer[0]) << "  dimensionality: "
          << dataspace.dimensionality << "  flags: " << static_cast<int>(buffer[
          2]) << endl;
#endif
      dataspace.sizes.clear();
      dataspace.sizes.reserve(dataspace.dimensionality);
      for (int n = 0; n < dataspace.dimensionality; ++n) {
        dataspace.sizes.emplace_back(value(&buffer[off], size_of_lengths));
#ifdef __DEBUG
        cerr << "  dimension " << n << " size: " << dataspace.sizes[n] << endl;
#endif
        off += size_of_lengths;
      }
      int flag;
      bits::get(buffer, flag, 23, 1);
      if (flag == 1) {
#ifdef __DEBUG
        cerr << "max sizes" << endl;
#endif
        dataspace.max_sizes.clear();
        dataspace.max_sizes.reserve(dataspace.dimensionality);
        for (int n = 0; n < dataspace.dimensionality; ++n) {
          dataspace.max_sizes.emplace_back(value(&buffer[off],
              size_of_lengths));
#ifdef __DEBUG
          cerr << "  dimension " << n << " max size: " << dataspace.max_sizes[n]
              << endl;
#endif
          off += size_of_lengths;
        }
      }
      break;
    }
    case 2: {
      if (buffer[3] == 2) {
        dataspace.dimensionality = -1;
      } else {
        dataspace.dimensionality = static_cast<short>(buffer[1]);
      }
      auto off = 4;
#ifdef __DEBUG
      cerr << "version: " << static_cast<int>(buffer[0]) << "  dimensionality: "
          << dataspace.dimensionality << "  flags: " << static_cast<int>(buffer[
          2]) << "  type: " << static_cast<int>(buffer[3]) << endl;
#endif
      if (dataspace.dimensionality > 0) {
        dataspace.sizes.clear();
        dataspace.sizes.reserve(dataspace.dimensionality);
        for (int n = 0; n < dataspace.dimensionality; ++n) {
          dataspace.sizes.emplace_back(value(&buffer[off], size_of_lengths));
#ifdef __DEBUG
          cerr << "  dimension " << n << " size: " << dataspace.sizes[n] <<
              endl;
#endif
          off += size_of_lengths;
        }
        int flag;
        bits::get(buffer, flag, 23, 1);
        if (flag == 1) {
#ifdef __DEBUG
          cerr << "max sizes" << endl;
#endif
          dataspace.max_sizes.clear();
          dataspace.max_sizes.reserve(dataspace.dimensionality);
          for (int n = 0; n < dataspace.dimensionality; ++n) {
            dataspace.max_sizes.emplace_back(value(&buffer[off],
                size_of_lengths));
#ifdef __DEBUG
            cerr << "  dimension " << n << " max size: " << dataspace.max_sizes[
                n] << endl;
#endif
            off += size_of_lengths;
          }
        }
      }
      break;
    }
    default: {
      append(myerror, "unable to decode dataspace version " + to_string(
          static_cast<int>(buffer[0])), ", ");
      return false;
    }
  }
  return true;
}

bool decode_datatype(unsigned char *buffer, Datatype& datatype) {
  bits::get(buffer, datatype.version, 0, 4);
  bits::get(buffer, datatype.class_, 4, 4);
  copy(&buffer[1], &buffer[3], datatype.bit_fields);
  datatype.size = value(&buffer[4], 4);
#ifdef __DEBUG
  cerr << "Datatype class and version=" << datatype.class_ << "/" << datatype.
      version << " size: " << datatype.size << endl;
  cerr << "Size of datatype element is " << datatype.size << endl;
#endif
  switch (datatype.class_) {
    case 0: {
      datatype.prop_len = 4;
      datatype.properties.reset(new unsigned char[datatype.prop_len]);
      copy(&buffer[8], &buffer[8]+datatype.prop_len, datatype.properties.get());
      break;
    }
    case 1: {
      datatype.prop_len = 12;
      datatype.properties.reset(new unsigned char[datatype.prop_len]);
      copy(&buffer[8], &buffer[8]+datatype.prop_len, datatype.properties.get());
      break;
    }
    case 2: {
      datatype.prop_len = 2;
      datatype.properties.reset(new unsigned char[datatype.prop_len]);
      copy(&buffer[8], &buffer[8]+datatype.prop_len, datatype.properties.get());
      break;
    }
    case 3: {
      datatype.prop_len = 0;
      break;
    }
    case 4: {
      datatype.prop_len = 4;
      datatype.properties.reset(new unsigned char[datatype.prop_len]);
      copy(&buffer[8], &buffer[8]+datatype.prop_len, datatype.properties.get());
      break;
    }
    case 5: {
      auto off = 8;
      while (static_cast<int>(buffer[off]) != 0) {
        off += 8;
      }
      datatype.prop_len = off - 8;
      datatype.properties.reset(new unsigned char[datatype.prop_len]);
      copy(&buffer[8], &buffer[8]+datatype.prop_len, datatype.properties.get());
      break;
    }
    case 6: {
      int num_members = value(datatype.bit_fields, 2);
#ifdef __DEBUG
      cerr << "number of members: " << num_members << endl;
#endif
      auto off = 8;
      for (int n = 0; n < num_members; ++n) {
#ifdef __DEBUG
        cerr << "  member #" << n << " at off " << off << endl;
#endif
        string member_name;
        size_t member_byte_offset;
        auto bytes_decoded = decode_compound_datatype_name_and_byte_offset(
            &buffer[off], datatype, member_name, member_byte_offset);
        if (bytes_decoded == 0) {
          append(myerror, "unable to decode compound data type version " +
              to_string(datatype.version), ", ");
          return false;
        }
        off += bytes_decoded;
#ifdef __DEBUG
        short c, v;
        bits::get(&buffer[off], v, 0, 4);
        bits::get(&buffer[off], c, 4, 4);
        cerr << "  decoding datatype at offset " << off << " class/version: " <<
            c << "/" << v << endl;
#endif
        auto *l_datatype = new Datatype;
        decode_datatype(&buffer[off], *l_datatype);
        off += 8 + l_datatype->prop_len;
        delete l_datatype;
#ifdef __DEBUG
        cerr << "  off is " << off << endl;
#endif
/*
        switch (datatype.version) {
          case 1: {
            while (static_cast<int>(buffer[off+7]) != 0) {
              off+=8;
            }
            off+=8;
#ifdef __DEBUG
            cerr << "  name ends at off " << off << endl;
            cerr << "  byte offset of member within data: " << value(&buffer[off], 4) << endl;
#endif
            off+=4;
#ifdef __DEBUG
            cerr << "  dimensionality: " << static_cast<int>(buffer[off]) << endl;
#endif
            off+=12;
            for (int m=0; m < 4; ++m) {
#ifdef __DEBUG
              cerr << "  dim " << m << " size: " << value(&buffer[off], 4) << endl;
#endif
              off+=4;
            }
#ifdef __DEBUG
            cerr << "  decoding datatype at offset " << off << endl;
#endif
            auto *l_datatype = new Datatype;
            decode_datatype(&buffer[off], *l_datatype);
            off+=8+l_datatype->prop_len;
            delete l_datatype;
#ifdef __DEBUG
            cerr << "  off is " << off << endl;
#endif
            break;
          }
          case 3: {
            while (buffer[off] != 0) {
              ++off;
            }
#ifdef __DEBUG
            cerr << "  name ends at off " << off << endl;
#endif
            ++off;
            int m=datatype.size;
            auto l=1;
            while (m/256 > 0) {
              ++l;
              m/=256;
            }
#ifdef __DEBUG
            cerr << "  datatype size: " << datatype.size << " length of byte offset: " << l << endl;
            cerr << "  byte offset of member within data: " << value(&buffer[off], l) << endl;
#endif
            off+=l;
#ifdef __DEBUG
            short c, v;
            bits::get(&buffer[off], v, 0, 4);
            bits::get(&buffer[off], c, 4, 4);
            cerr << "  decoding datatype at offset " << off <<
                " class/version: " << c << "/" << v << endl;
#endif
            auto *l_datatype = new Datatype;
            decode_datatype(&buffer[off], *l_datatype);
            off += 8 + l_datatype->prop_len;
            delete l_datatype;
#ifdef __DEBUG
            cerr << "  off is " << off << endl;
#endif
            break;
          }
          default: {
            append(myerror, "unable to decode compound data type version " +
                to_string(datatype.version), ", ");
            return false;
          }
        }
*/
      }
      datatype.prop_len = off - 8;
      datatype.properties.reset(new unsigned char[datatype.prop_len]);
      copy(&buffer[8], &buffer[8]+datatype.prop_len, datatype.properties.get());
      break;
    }
    case 7: {
      datatype.prop_len = 0;
      break;
    }
    case 8: {
      append(myerror, "unable to decode datatype class 8 properties", ", ");
      return false;
      break;
    }
    case 9: {
      datatype.prop_len = datatype.size - 8;
      datatype.properties.reset(new unsigned char[datatype.prop_len]);
      copy(&buffer[8], &buffer[8]+datatype.prop_len, datatype.properties.get());
      break;
    }
    case 10: {
      short dimensionality = buffer[8];
#ifdef __DEBUG
      cerr << "dimensionality is " << dimensionality << endl;
#endif
      switch (datatype.version) {
        case 2: {
          datatype.prop_len = 4 + dimensionality * 8 + 8;
          datatype.properties.reset(new unsigned char[datatype.prop_len]);
          copy(&buffer[8], &buffer[8]+datatype.prop_len, datatype.properties.
              get());
          break;
        }
        case 3: {
          datatype.prop_len = 1 + dimensionality * 4 + 8;
          datatype.properties.reset(new unsigned char[datatype.prop_len]);
          copy(&buffer[8], &buffer[8]+datatype.prop_len, datatype.properties.
              get());
          break;
        }
      }
      break;
    }
    default: {
      append(myerror, "datatype class " + to_string(datatype.class_) + " is "
          "not defined", ", ");
      return false;
    }
  }
  return true;
}

int global_heap_object(std::fstream& fs, short size_of_lengths, unsigned long
    long address, int index, unsigned char **buffer) {
  int buf_len = 0;
  auto curr_off = fs.tellg();
  fs.seekg(address, std::ios_base::beg);
  unsigned char buf[24];
  char *cbuf = reinterpret_cast<char *>(buf);
  fs.read(cbuf, 4);
  if (fs.gcount() != 4) {
    cerr << "unable to read global heap collection signature at address " <<
        address << endl;
    exit(1);
  }
  if (string(cbuf, 4) != "GCOL") {
    cerr << "not a global heap collection" << endl;
    exit(1);
  }
  fs.read(cbuf, 4);
  if (fs.gcount() != 4) {
    cerr << "unable to read global heap collection version" << endl;
    exit(1);
  }
  if (buf[0] != 1) {
    cerr << "unable to decode global heap collection version " << to_string(buf[
        0]) << endl;
    exit(1);
  }
  fs.read(cbuf, size_of_lengths);
  if (fs.gcount() != static_cast<int>(size_of_lengths)) {
    cerr << "unable to read size of global heap collection" << endl;
    exit(1);
  }
  unsigned long long gsize = value(buf, size_of_lengths);
  int off = 0;
  while (off < static_cast<int>(gsize)) {
    fs.read(cbuf, 8+size_of_lengths);
    if (fs.gcount() != static_cast<int>(8+size_of_lengths)) {
      cerr << "unable to read global heap object" << endl;
      exit(1);
    }
    int idx = value(buf, 2);
    unsigned long long osize = value(&buf[8], size_of_lengths);
    auto pad = osize % 8;
    if (pad > 0) {
      osize += 8 - pad;
    }
    if (idx == index) {
      buf_len = osize;
      *buffer = new unsigned char[buf_len];
      fs.read(reinterpret_cast<char *>(*buffer), buf_len);
      if (fs.gcount() != buf_len) {
        cerr << "unable to read global heap object data" << endl;
        exit(1);
      }
      break;
    } else {
      fs.seekg(osize, std::ios_base::cur);
      off += 8 + size_of_lengths + osize;
    }
  }
  fs.seekg(curr_off, std::ios_base::beg);
  return buf_len;
}

unsigned long long value(const unsigned char *buffer, int num_bytes) {
  unsigned long long val = 0; // return value
  unsigned long long m = 1;
  for (int n = 0; n < num_bytes; ++n) {
    val += static_cast<unsigned long long>(buffer[n]) * m;
    m *= 256;
  }
  return val;
}

double decode_data_value(const Datatype& datatype, void *value, double
    missing_indicator) {
  if (value == nullptr) {
    return missing_indicator;
  }
  switch (datatype.class_) {
    case 0: {

      // fixed point numbers (integers)
      switch (HDF5::value(&datatype.properties[2], 2)) {
        case 8: {
          return static_cast<int>(*(reinterpret_cast<unsigned char *>(value)));
        }
        case 16: {
          return *(reinterpret_cast<short *>(value));
        }
        case 32: {
          return *(reinterpret_cast<int *>(value));
        }
        case 64: {
          return *(reinterpret_cast<long long *>(value));
        }
        default: {
          return missing_indicator;
        }
      }
      break;
    }
    case 1: {

      // floating point numbers
      switch (HDF5::value(&datatype.properties[2], 2)) {
        case 32: {
          return *(reinterpret_cast<float *>(value));
        }
        case 64: {
          return *(reinterpret_cast<double *>(value));
        }
        default: {
          return missing_indicator;
        }
      }
      break;
    }
  }
  return missing_indicator;
}

string decode_data_value(const Datatype& datatype, void *value, string
    missing_indicator) {
  if (value == nullptr) {
    return missing_indicator;
  }
  switch (datatype.class_) {
    case 3: {

      // strings
      missing_indicator.assign(reinterpret_cast<char *>(value), datatype.size);
      break;
    }
  }
  return missing_indicator;
}

string datatype_class_to_string(const Datatype& datatype) {
  switch (datatype.class_) {
    case 0: {
      return "Fixed-Point (" + to_string(value(&datatype.properties[2], 2)) +
          ")";
    }
    case 1: {
      return "Floating-Point (" + to_string(value(&datatype.properties[2], 2)) +
          ")";
    }
    case 3: {
      return "String";
    }
    case 4: {
      return "Bit field";
    }
    case 6: {
      string s = "Compound [";
      CompoundDatatype cdatatype;
      if (decode_compound_datatype(datatype, cdatatype)) {
        for (auto& member : cdatatype.members) {
          if (&member != &cdatatype.members.front()) {
            s += ", ";
          }
          s += "\"" + member.name + "\": " + datatype_class_to_string(member.
              datatype);
        }
      }
      s += "]";
      return s;
    }
    default: {
      return "Class: " + to_string(datatype.class_);
    }
  }
}

}; // end namespace HDF5
