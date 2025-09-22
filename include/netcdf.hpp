#ifndef NETCDF_H
#define   NETCDF_H

#include <fstream>
#include <list>
#include <vector>
#include <unordered_map>
#include <datetime.hpp>
#include <mymap.hpp>

class NetCDF {
public:
  enum class NCType {_NULL = 0, BYTE, CHAR, SHORT, INT, FLOAT, DOUBLE, UBYTE,
      USHORT, UINT, INT64, UINT64};
  static const char *nc_type_str[12];
  static const short nc_type_bytes[12];
  enum class Category {DIMENSION = 10, VARIABLE, ATTRIBUTE};
  static const unsigned char BYTE_NOT_SET;
  static const char CHAR_NOT_SET;
  static const short SHORT_NOT_SET;
  static const int INT_NOT_SET;
  static const float FLOAT_NOT_SET;
  static const double DOUBLE_NOT_SET;

  struct Dimension {
    Dimension() : length(0), name(), is_rec(false) { }

    unsigned long long length;
    std::string name;
    bool is_rec;
  };
  class Attribute {
  public:
    Attribute() : name(), nc_type(), num_values(0), values(nullptr) { }
    Attribute(const Attribute& source) : name(), nc_type(), num_values(0),
        values(nullptr) { *this = source; }
    ~Attribute() { clear_values(); }
    Attribute& operator=(const Attribute& source);
    void clear_values();
    std::string to_string() const { return *(reinterpret_cast<std::string *>(
        values)); }

    std::string name;
    NCType nc_type;
    size_t num_values;
    void *values;
  };
  class DataValue {
  public:
    DataValue() : value(nullptr), nc_type(NCType::_NULL) { }
    DataValue(const DataValue& source) : DataValue() { *this = source; }
    ~DataValue() { clear(); };
    DataValue& operator=(const DataValue& source);
    void clear();
    double get() const;
    void resize(NCType type);
    void set(double source_value);
    NCType type() const { return nc_type; }

  private:
    void *value;
    NCType nc_type;
  };
  struct Variable {
    Variable() : name(), dimids(), attrs(), nc_type(), size(0), offset(0),
        long_name(), standard_name(), units(), _FillValue(), is_rec(false),
        is_coord(false) { }

    std::string name;
    std::vector<unsigned long long> dimids;
    std::vector<Attribute> attrs;
    NCType nc_type;
    size_t size;
    off_t offset;
    std::string long_name, standard_name, units;
    DataValue _FillValue;
    bool is_rec, is_coord;
  };
  struct UniqueVariableStringEntry {
    UniqueVariableStringEntry() : key() { }

    std::string key;
  };
  struct UniqueVariableTimeEntry {
    UniqueVariableTimeEntry() : first_valid_datetime(nullptr),
        last_valid_datetime(nullptr), reference_datetime(nullptr) { }
    void free_memory();

    std::shared_ptr<DateTime> first_valid_datetime, last_valid_datetime,
        reference_datetime;
  };
  struct UniqueVariableLevelEntry {
    UniqueVariableLevelEntry() : key(), value(nullptr), value2(nullptr) { }
    void free_memory();

    std::string key;
    std::shared_ptr<float> value, value2;
  };
  struct UniqueVariableDataEntry {
    UniqueVariableDataEntry() : time_entry(nullptr), record_offset(nullptr),
        record_length(nullptr), level(nullptr) { }
    void free_memory();

    std::shared_ptr<UniqueVariableTimeEntry> time_entry;
    std::shared_ptr<off_t> record_offset, record_length;
    std::shared_ptr<UniqueVariableLevelEntry> level;
  };
  struct UniqueVariableEntry {
    struct Data {
	Data() : description(), units(), lev_description(),
            product_description(), cell_methods(), times(), data(),
            unique_times(), unique_levels(), num_levels(0), dim_ids(),
            num_gridpoints(0), is_layer(false) { }

	std::string description, units, lev_description, product_description,
            cell_methods;
	std::list<UniqueVariableTimeEntry> times;
	std::list<UniqueVariableDataEntry> data;
	my::map<UniqueVariableStringEntry> unique_times, unique_levels;
	int num_levels;
	std::vector<size_t> dim_ids;
	int num_gridpoints;
	bool is_layer;
    };
    UniqueVariableEntry() : key(), data(nullptr) { }
    void free_memory();

    std::string key;
    std::shared_ptr<Data> data;
  };
  class VariableData {
  public:
    VariableData() : num_values(0), values(nullptr), nc_type(NCType::_NULL),
        capacity(0) { }
    VariableData(const VariableData& source) : VariableData() { *this =
        source; }
    ~VariableData() { clear(); }
    VariableData& operator=(const VariableData& source);
    double operator[](size_t index) const;
    double back() const { return (*this)[num_values-1]; }
    void clear();
    bool empty() const { return num_values == 0; }
    double front() const { return (*this)[0]; }
    void *get() const { return values; }
    void resize(int new_size, NCType type);
    void set(size_t index, double value);
    size_t size() const { return num_values; }
    NCType type() const { return nc_type; }

  private:
    size_t num_values;
    void *values;
    NCType nc_type;
    int capacity;
  };
  struct Time {
    Time() : base(), nc_type(), units() { }

    DateTime base;
    NCType nc_type;
    std::string units;
  };

  NetCDF() : file_name(), fs(), _version(0), data_len(0), var_offset_len(0),
      num_recs(0), rec_size(0), dims(), gattrs(), vars() { }
  virtual ~NetCDF() { }
  virtual bool close() = 0;
  std::vector<Dimension>& dimensions() { return dims; }
  std::vector<Attribute>& global_attributes() { return gattrs; }
  bool is_open() const { return (fs.is_open()); }
  size_t num_records() const { return num_recs; }
  virtual bool open(std::string filename) = 0;
  size_t record_size() const { return rec_size; }
  std::vector<Variable>& variables() { return vars; }
  short version() const { return _version; }

protected:
  int find_variable(std::string variable_name) const;

  std::string file_name;
  std::fstream fs;
  short _version, data_len, var_offset_len;
  unsigned long long num_recs, rec_size;
  std::vector<Dimension> dims;
  std::vector<Attribute> gattrs;
  std::vector<Variable> vars;
};

class InputNetCDFStream : public NetCDF {
public:
  InputNetCDFStream() : var_buf(), size_(0), var_indexes() { }
  bool close();
  bool open(std::string filename);
  void print_dimensions() const;
  void print_global_attributes() const;
  void print_header() const;
  void print_variable_data(std::string variable_name,
      std::string string_of_indexes);
  void print_variables() const;
  off_t size() const { return size_; }
  std::vector<double> value_at(std::string variable_name, size_t index);
  const Variable& variable(std::string variable_name) const;
  NCType variable_data(std::string variable_name, VariableData&
      variable_data);
  size_t variable_dimensions(std::string variable_name,
      size_t **address_of_dimension_array) const;
  void variable_value(std::string variable_name, std::string indexes,
      void **value);

private:
  void fill_dimensions();
  void fill_attributes(std::vector<Attribute>& attributes);
  void fill_variables();
  void fill_string(std::string& string_to_fill);
  void print_attribute(const Attribute& attribute, size_t left_margin_spacing)
      const;
  bool print_indexes(const Variable& variable, size_t elem_num) const;

  struct VariableBuffer {
    VariableBuffer() : MAX_BUF_SIZE(4000000), buffer(new char[MAX_BUF_SIZE]),
        buf_size(MAX_BUF_SIZE), first_offset(0) { }

    size_t MAX_BUF_SIZE;
    char *buffer;
    int buf_size;
    long long first_offset;
  } var_buf;
  off_t size_;
  std::unordered_map<std::string, size_t> var_indexes;
};

class OutputNetCDFStream : public NetCDF {
public:
  OutputNetCDFStream() : curr_offset(0), started_record_vars(false),
      non_rec_housekeeping() { }
  ~OutputNetCDFStream();
  void add_dimension(std::string name, size_t length);
  void add_global_attribute(std::string name, std::string value) {
      add_attribute(gattrs, name, NCType::CHAR, 1, &value); }
  void add_global_attribute(std::string name, short value) {
      add_attribute(gattrs, name, NCType::SHORT, 1, &value); }
  void add_global_attribute(std::string name, int value) {
      add_attribute(gattrs, name, NCType::INT, 1, &value); }
  void add_global_attribute(std::string name, float value) {
      add_attribute(gattrs, name, NCType::FLOAT, 1, &value); }
  void add_global_attribute(std::string name, double value) {
      add_attribute(gattrs, name, NCType::DOUBLE, 1, &value); }
  void add_global_attribute(std::string name, NCType nc_type, size_t
      num_values, void *values) { add_attribute(gattrs, name, nc_type,
      num_values, values); }
  void add_record_data(VariableData& variable_data);
  void add_record_data(VariableData& variable_data, size_t num_values);
  void add_variable(std::string name, NCType nc_type) {
      add_variable(name, nc_type, 0, nullptr); }
  void add_variable(std::string name, NCType nc_type, size_t dimension_id) {
      add_variable(name, nc_type, 1, &dimension_id); }
  void add_variable(std::string name, NCType nc_type, size_t num_ids,
      size_t *dimension_ids);
  void add_variable(std::string name, NCType nc_type, const std::vector<
      size_t>& dimension_ids);
  void add_variable_attribute(std::string variable_name,
      std::string attribute_name, unsigned char value);
  void add_variable_attribute(std::string variable_name,
      std::string attribute_name, std::string value);
  void add_variable_attribute(std::string variable_name,
      std::string attribute_name, short value);
  void add_variable_attribute(std::string variable_name,
      std::string attribute_name, int value);
  void add_variable_attribute(std::string variable_name,
      std::string attribute_name, float value);
  void add_variable_attribute(std::string variable_name,
      std::string attribute_name, double value);
  void add_variable_attribute(std::string variable_name,
      std::string attribute_name, NCType nc_type, size_t num_values,
      void *values);
  bool close();
  void initialize_non_record_data(std::string variable_name);
  bool open(std::string filename);
  void set_number_of_records(size_t num_records) { num_recs = num_records; }
  void write_header();
  void write_non_record_data(std::string variable_name, void *data_array);
  void write_partial_non_record_data(void *data_array, int num_values);

private:
  void add_attribute(std::vector<Attribute>& attributes_to_grow, std::string
      name, NCType nc_type, size_t num_values, void *values);
  void put_string(const std::string& string_to_put);
  void put_attributes(const std::vector<Attribute>& attributes_to_put);

  off_t curr_offset;
  bool started_record_vars;
  struct NonRecordHouseKeeping {
    NonRecordHouseKeeping() : index(0), num_values(0), num_values_written(0) { }

    int index, num_values, num_values_written;
  } non_rec_housekeeping;
};

extern bool operator==(const NetCDF::NetCDF::UniqueVariableTimeEntry& te1, const
    NetCDF::NetCDF::UniqueVariableTimeEntry& te2);
extern bool operator!=(const NetCDF::NetCDF::UniqueVariableTimeEntry& te1, const
    NetCDF::NetCDF::UniqueVariableTimeEntry& te2);

#endif
