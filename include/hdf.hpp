// File: hdf.h

#ifndef HDF_H
#define   HDF_H

#include <vector>
#include <deque>
#include <unordered_map>
#include <iodstream.hpp>
#include <mymap.hpp>

class HDF4Stream {
public:
  struct TagEntry {
    TagEntry() : key(), id(), description() { }

    size_t key;
    std::string id, description;
  };

  HDF4Stream();
  virtual ~HDF4Stream() { }
  std::string datatype(int code) const { return data_types[code]; }

protected:
  std::vector<std::string> data_types;
  my::map<TagEntry> tag_table;
};

class InputHDF4Stream : public HDF4Stream, public idstream {
public:
  struct DataDescriptor {
    DataDescriptor() : key(0), reference_number(0), offset(0), length(0) { }

    size_t key;
    short reference_number;
    int offset, length;
  };

  struct ReferenceEntry {
    ReferenceEntry() : key(0), data_descriptor_table(nullptr) { }

    size_t key;
    std::shared_ptr<my::map<DataDescriptor>> data_descriptor_table;
  };

  InputHDF4Stream() : data_descriptors(), reference_table() { }
  DataDescriptor data_descriptor(size_t index) const { return data_descriptors[
      index]; }
  int num_descriptors() const { return data_descriptors.size(); }
  int ignore() { return bfstream::error; }
  bool open(const char *filename);
  int peek();
  void print_data_descriptor(const DataDescriptor& data_descriptor, std::string
      indent = "");
  void print_data_descriptors(size_t tag_number);
  int read(unsigned char *buffer, size_t buffer_length);

protected:
  std::vector<DataDescriptor> data_descriptors;
  my::map<ReferenceEntry> reference_table;
};

class InputHDF5Stream : public idstream {
public:
  struct SymbolTableEntry {
    SymbolTableEntry() : linkname_off(0), objhdr_addr(0), cache_type(0),
        scratchpad(), linkname() { }

    unsigned long long linkname_off, objhdr_addr;
    int cache_type;
    unsigned char scratchpad[16];
    std::string linkname;
  };

  struct Tree {
    Tree() : node_type(0), node_level(0), left_sibling_addr(0),
        right_sibling_addr(0), keys(), child_pointers() { }

    int node_type, node_level;
    unsigned long long left_sibling_addr, right_sibling_addr;
    std::list<unsigned long long> keys, child_pointers;
  };

  struct Dataset;
  class Chunk {
  public:
    Chunk() : address(0), buffer(nullptr), size(0), length(0), offsets() { }
    Chunk(const Chunk& source) : Chunk() { *this = source; }
    Chunk(long long addr, size_t sz, size_t len, const std::vector<size_t>&
        offs);
    ~Chunk();
    Chunk& operator=(const Chunk& source);
    void allocate();
    bool fill(std::fstream& fs, const Dataset& dataset);

    long long address;
    std::unique_ptr<unsigned char[]> buffer;
    size_t size, length;
    std::vector<size_t> offsets;
  };

  struct Dataspace {
    Dataspace() : dimensionality(0), sizes(), max_sizes() { }

    short dimensionality;
    std::vector<unsigned long long> sizes, max_sizes;
  };

  class Datatype {
  public:
    Datatype() : class_(-1), version(0), bit_fields(), size(0), prop_len(0),
        properties(nullptr) { }
    Datatype(const Datatype& source) : Datatype() { *this = source; }
    ~Datatype();
    Datatype& operator=(const Datatype& source);

    short class_, version;
    unsigned char bit_fields[3];
    int size, prop_len;
    std::unique_ptr<unsigned char[]> properties;
  };

  struct CompoundDatatype {
    struct Member {
	Member() : name(), byte_offset(0), datatype() { }

	std::string name;
	size_t byte_offset;
	Datatype datatype;
    };

    CompoundDatatype() : members() { }

    std::vector<Member> members;
  };

  class Data {
  public:
    Data() : address(0xffffffffffffffff), sizes(), chunks(), size_of_element(0)
        { }

    unsigned long long address;
    std::vector<int> sizes;
    std::vector<Chunk> chunks;
    int size_of_element;
  };

  class FillValue {
  public:
    FillValue() : length(0), bytes(nullptr) { }

    int length;
    unsigned char *bytes;
  };

  class DataValue {
  public:
    enum class ArrayType { _NULL = 0, BYTE, SHORT, INT, LONG_LONG, FLOAT,
        DOUBLE, STRING };
    DataValue() : _class_(-1), precision_(0), size(0), dim_sizes(), array(
        nullptr), capacity(0), array_type(ArrayType::_NULL), compound(), vlen()
        { }
    DataValue(const DataValue& source) : DataValue() { *this = source; }
    ~DataValue() { clear(); }
    DataValue& operator=(const DataValue& source);
    void allocate(ArrayType type, size_t count);
    short class_() const { return _class_; }
    void clear();
    void *get() const { return array; }
    short precision() const { return precision_; }
    void print(std::ostream& ofs, std::shared_ptr<std::unordered_map<size_t,
        std::string>> ref_table) const;
    bool set(std::fstream& fs, unsigned char *buffer, short size_of_offsets,
        short size_of_lengths, const InputHDF5Stream::Datatype& datatype, const
         InputHDF5Stream::Dataspace& dataspace);

    short _class_, precision_;
    size_t size;
    std::vector<unsigned long long> dim_sizes;
    void *array;
    size_t capacity;
    ArrayType array_type;
    struct Compound {
	struct Member {
	  Member() : name(), class_(-1), precision_(0), size(0) { }

	  std::string name;
	  short class_, precision_;
	  int size;
	};
	Compound() : members() { }

	std::vector<Member> members;
    } compound;

    struct VariableLength {
	VariableLength() : class_(-1), size(0), buffer(nullptr) { }

	short class_;
	size_t size;
	std::unique_ptr<unsigned char[]> buffer;
    } vlen;

  };

  typedef std::pair<std::string, DataValue> Attribute;
  typedef std::unordered_map<std::string, DataValue> Attributes;

  class Dataset {
  public:
    Dataset() : datatype(), dataspace(), fillvalue(), attributes(), filters(0),
        data() { }
    void free();

    Datatype datatype;
    Dataspace dataspace;
    FillValue fillvalue;
    Attributes attributes;
    std::deque<short> filters;
    Data data;
  };

  class Group;
  struct GroupEntry {
    GroupEntry() : key(), p_g(nullptr) { }

    std::string key;
    std::shared_ptr<Group> p_g;
  };

  struct FindGroup {
    FindGroup() = delete;
    FindGroup(std::string s) : key(s) { }
    bool operator()(const GroupEntry& ge) {
      return ge.key == key;
    }

    std::string key;
  };

  struct DatasetEntry {
    DatasetEntry() : key(), p_ds(nullptr) { }

    std::string key;
    std::shared_ptr<Dataset> p_ds;
  };

  struct FindDataset {
    FindDataset() = delete;
    FindDataset(std::string s) : key(s) { }
    bool operator()(const DatasetEntry& de) {
      return de.key == key;
    }

    std::string key;
  };

  class Group {
  public:
    Group() : version(0), local_heap(), btree_addr(0), tree(), groups(),
        datasets() { }

    int version;
    struct LocalHeap {
	LocalHeap() : addr(0), data_start(0) { }

	unsigned long long addr, data_start;
    } local_heap;
    unsigned long long btree_addr;
    Tree tree;
    std::vector<GroupEntry> groups;
    std::vector<DatasetEntry> datasets;
  };

  struct FractalHeapData {
    struct Space {
      Space() : bytes_free(0), bytes_total(0), bytes_allocated(0),
          next_allocation(0), manager_addr(0) { }

      unsigned long long bytes_free, bytes_total, bytes_allocated,
          next_allocation;
      unsigned long long manager_addr;
    };

    struct Objects {
	Objects() : num_managed(0), num_huge(0), num_tiny(0) { }

	unsigned long long num_managed, num_huge, num_tiny;
    };

    FractalHeapData() : dse(nullptr), start_block_size(0), space(), objects(),
        id_len(0), io_filter_size(0), max_size(0), table_width(0),
        max_dblock_rows(0), nrows(0), K(0), N(0), max_dblock_size(0),
        max_managed_obj_size(0), curr_row(0), curr_col(0), flags() { }

    DatasetEntry *dse;
    unsigned long long start_block_size;
    Space space;
    Objects objects;
    int id_len, io_filter_size, max_size, table_width, max_dblock_rows, nrows,
        K, N;
    int max_dblock_size, max_managed_obj_size;
    int curr_row, curr_col;
    unsigned char flags;
  };

  struct FreeSpaceManager {
    FreeSpaceManager() : total_bytes(0), num_sections(0), num_serialized(0),
        num_unserialized(0), max_size(0), list_addr(0), list_size(0),
        list_asize(0), addr_size(0), space_map() { }

    unsigned long long total_bytes, num_sections, num_serialized,
        num_unserialized, max_size, list_addr, list_size, list_asize;
    size_t addr_size;
    std::unordered_map<unsigned long long, unsigned long long> space_map;
  };

  InputHDF5Stream() : sb_version(0), sizes(), group_K(), base_addr(0), eof_addr(
      0), undef_addr(0), root_group(), ref_table(nullptr), free_space_manager()
      { }
  ~InputHDF5Stream();
  void close();
  Attribute attribute(std::string xpath);
  std::shared_ptr<Dataset> dataset(std::string xpath);
  std::list<DatasetEntry> datasets(Group *g = nullptr);
  std::list<DatasetEntry> datasets_with_attribute(std::string attribute_path,
      Group *g = nullptr);
  std::fstream *file_stream() { return &fs; }
  std::shared_ptr<std::unordered_map<size_t, std::string>>
      reference_table_pointer() const { return ref_table; }
  short size_of_offsets() const { return sizes.offsets; }
  short size_of_lengths() const { return sizes.lengths; }
  unsigned long long undefined_address() const { return undef_addr; }
  int ignore() { return bfstream::error; }
  bool open(const char *filename);
  int peek();
  void print_data(Dataset& dataset);
  void print_fill_value(std::string xpath);
  int read(unsigned char *buffer, size_t buffer_length);
  void show_file_structure();

protected:
  void clear_groups(Group& group);
  bool decode_attribute(unsigned char *buffer, Attribute& attribute, int&
      length, int ohdr_version);
  bool decode_Btree(Group& group, FractalHeapData *frhp_data);
  bool decode_fractal_heap(unsigned long long address, int header_message_type,
      FractalHeapData& frhp_data);
  bool decode_fractal_heap_block(unsigned long long address, int
       header_message_type, FractalHeapData& frhp_data);
  bool decode_free_space_manager(FractalHeapData& frhp_data);
  bool decode_free_space_section_list(FractalHeapData& frhp_data);
  bool decode_header_messages(int ohdr_version, size_t header_size, std::string
      ident, Group *group, DatasetEntry *dse_in, unsigned char flags);
  int decode_header_message(std::string ident, int ohdr_version, int type, int
      length, unsigned char flags, unsigned char *buffer, Group *group,
      DatasetEntry& dse, bool& is_subgroup);
  bool decode_indirect_data(FractalHeapData& frhp_data);
  bool decode_object_header(SymbolTableEntry& ste, Group *group);
  bool decode_superblock(unsigned long long& objhdr_addr);
  bool decode_symbol_table_entry(SymbolTableEntry& ste, Group *group);
  bool decode_symbol_table_node(unsigned long long address, Group& group);
  bool decode_v1_Btree(Group& group);
  bool decode_v1_Btree(unsigned long long address, Dataset& dataset);
  bool decode_v2_Btree(Group& group, FractalHeapData *frhp_data);
  bool decode_v2_Btree_node(unsigned long long address, int num_records,
      FractalHeapData *frhp_data);
  void print_a_group_tree(Group& group);

  int sb_version;
  struct Sizes {
    Sizes() : offsets(0), lengths(0) { }

    size_t offsets, lengths;
  } sizes;
  struct GroupK {
    GroupK() : leaf(0), internal(0) { }

    int leaf, internal;
  } group_K;
  unsigned long long base_addr, eof_addr, undef_addr;
  Group root_group;
  std::shared_ptr<std::unordered_map<size_t, std::string>> ref_table;
  FreeSpaceManager free_space_manager;
};

namespace HDF5 {

class DataArray {
public:
  static const double default_missing_value;
  enum class Type { _NULL = 0, BYTE, SHORT, INT, LONG_LONG, FLOAT, DOUBLE,
      STRING };
  DataArray() : num_values(0), type(Type::_NULL), values(nullptr), dimensions()
      { }
  DataArray(const DataArray& source) : DataArray() { *this = source; }
  DataArray(InputHDF5Stream& istream, InputHDF5Stream::Dataset& dataset, size_t
      compound_member_index = 0) : DataArray() { fill(istream, dataset,
      compound_member_index); }
  ~DataArray() { clear(); }
  DataArray& operator=(const DataArray& source);
  bool fill(InputHDF5Stream& istream, InputHDF5Stream::Dataset& dataset, size_t
      compound_member_index = 0);
  unsigned char byte_value(size_t index) const;
  short short_value(size_t index) const;
  int int_value(size_t index) const;
  long long long_long_value(size_t index) const;
  float float_value(size_t index) const;
  double double_value(size_t index) const;
  std::string string_value(size_t index) const;
  double value(size_t index) const;

  size_t num_values;
  Type type;
  void *values;
  std::vector<size_t> dimensions;

private:
  void clear();
};

void print_attribute(const InputHDF5Stream::Attribute& attribute, std::
    shared_ptr<std::unordered_map<size_t, std::string>> ref_table);
void print_data_value(InputHDF5Stream::Datatype& datatype, void *value);

std::string datatype_class_to_string(const InputHDF5Stream::Datatype& datatype);

bool decode_compound_datatype(const InputHDF5Stream::Datatype& datatype,
    InputHDF5Stream::CompoundDatatype& compound_datatype);
bool decode_compound_data_value(unsigned char *buffer, InputHDF5Stream::
    Datatype& datatype, void ***values);
bool decode_dataspace(unsigned char *buffer, unsigned long long size_of_lengths,
    InputHDF5Stream::Dataspace& dataspace);
bool decode_datatype(unsigned char *buffer, InputHDF5Stream::Datatype&
     datatype);

int global_heap_object(std::fstream& fs, short size_of_lengths, unsigned long
     long address, int index, unsigned char **buffer);

unsigned long long value(const unsigned char *buffer, int num_bytes);

double decode_data_value(const InputHDF5Stream::Datatype& datatype, void *value,
    double missing_indicator);
std::string decode_data_value(const InputHDF5Stream::Datatype& datatype, void
    *value, std::string missing_indicator);

}; // end namespace HDF5

#endif
