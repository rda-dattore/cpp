// File: hdf.h

#ifndef HDF_H
#define   HDF_H

#include <vector>
#include <deque>
#include <iodstream.hpp>
#include <mymap.hpp>

class HDF4Stream
{
public:
  struct TagEntry {
    TagEntry() : key(),ID(),description() {}

    size_t key;
    std::string ID,description;
  };

  HDF4Stream();
  virtual ~HDF4Stream() {}
  std::string getDataType(int code) const { return data_types[code]; }

protected:
  std::vector<std::string> data_types;
  my::map<TagEntry> tag_table;
};

class InputHDF4Stream : public HDF4Stream, public idstream
{
public:
  struct DataDescriptor {
    DataDescriptor() : key(0),reference_number(0),offset(0),length(0) {}

    size_t key;
    short reference_number;
    int offset,length;
  };
  struct ReferenceEntry {
    ReferenceEntry() : key(0),data_descriptor_table(nullptr) {}

    size_t key;
    std::shared_ptr<my::map<DataDescriptor>> data_descriptor_table;
  };

  InputHDF4Stream() : data_descriptors(),reference_table() {}
  DataDescriptor data_descriptor(size_t index) const { return data_descriptors[index]; }
  int num_descriptors() const { return data_descriptors.size(); }
  int ignore() { return bfstream::error; }
  bool open(const char *filename);
  int peek();
  void printDataDescriptor(const DataDescriptor& data_descriptor,std::string indent = "");
  void printDataDescriptors(size_t tag_number);
  int read(unsigned char *buffer,size_t buffer_length);

protected:
  std::vector<DataDescriptor> data_descriptors;
  my::map<ReferenceEntry> reference_table;
};

class InputHDF5Stream : public idstream
{
public:
  struct ReferenceEntry {
    ReferenceEntry() : key(0),name() {}

    size_t key;
    std::string name;
  };
  struct SymbolTableEntry {
    SymbolTableEntry() : linkname_off(0),objhdr_addr(0),cache_type(0),scratchpad(),linkname() {}

    unsigned long long linkname_off,objhdr_addr;
    int cache_type;
    unsigned char scratchpad[16];
    std::string linkname;
  };
  struct Tree {
    Tree() : node_type(0),node_level(0),left_sibling_addr(0),right_sibling_addr(0),keys(),child_pointers() {}

    int node_type,node_level;
    unsigned long long left_sibling_addr,right_sibling_addr;
    std::list<unsigned long long> keys,child_pointers;
  };
  struct Dataset;
  class Chunk {
  public:
    Chunk() : address(0),buffer(nullptr),size(0),length(0),offsets() {}
    Chunk(const Chunk& source) : Chunk() { *this=source; }
    Chunk(long long addr,size_t sz,size_t len,const std::vector<size_t>& offs);
    ~Chunk();
    Chunk& operator=(const Chunk& source);
    void allocate();
    bool fill(std::fstream& fs,const Dataset& dataset,bool show_debug);

    long long address;
    std::unique_ptr<unsigned char[]> buffer;
    size_t size,length;
    std::vector<size_t> offsets;
  };
  struct Dataspace {
    Dataspace() : dimensionality(0),sizes(),max_sizes() {}

    short dimensionality;
    std::vector<unsigned long long> sizes,max_sizes;
  };
  class Datatype {
  public:
    Datatype() : class_(-1),version(0),bit_fields(),size(0),prop_len(0),properties(nullptr) {}
    Datatype(const Datatype& source) : Datatype() { *this=source; }
    ~Datatype();
    Datatype& operator=(const Datatype& source);

    short class_,version;
    unsigned char bit_fields[3];
    int size,prop_len;
    std::unique_ptr<unsigned char[]> properties;
  };
  struct CompoundDatatype {
    struct Member {
	Member() : name(),byte_offset(0),datatype() {}

	std::string name;
	int byte_offset;
	Datatype datatype;
    };
    CompoundDatatype() : members() {}

    std::vector<Member> members;
  };
  class Data {
  public:
    Data() : address(0xffffffffffffffff),sizes(),chunks(),size_of_element(0) {}

    unsigned long long address;
    std::vector<int> sizes;
    std::vector<Chunk> chunks;
    int size_of_element;
  };
  class FillValue {
  public:
    FillValue() : length(0),bytes(NULL) {}

    int length;
    unsigned char *bytes;
  };
  class DataValue {
  public:
    DataValue() : class_(0),precision(0),size(0),dim_sizes(),value(nullptr),compound() {}
    DataValue(const DataValue& source) : DataValue() { *this=source; }
    ~DataValue() { clear(); }
    DataValue& operator=(const DataValue& source);
    void clear();
    void *get() const { return value; }
    short getClass() const { return class_; }
    short getPrecision() const { return precision; }
    void print(std::ostream& ofs,std::shared_ptr<my::map<ReferenceEntry>> ref_table) const;
    bool set(std::fstream& fs,unsigned char *buffer,short size_of_offsets,short size_of_lengths,const InputHDF5Stream::Datatype& datatype,const InputHDF5Stream::Dataspace& dataspace,bool show_debug);

    short class_,precision;
    int size;
    std::vector<unsigned long long> dim_sizes;
    void *value;
    struct Compound {
	struct Member {
	  Member() : name(),class_(0),precision(0),size(0) {}

	  std::string name;
	  short class_,precision;
	  int size;
	};
	Compound() : members() {}

	std::vector<Member> members;
    } compound;
  };
  struct Attribute {
    Attribute() : key(),value() {}

    std::string key;
    DataValue value;
  };
  struct Dataset {
    Dataset() : datatype(),dataspace(),fillvalue(),attributes(),filters(0),data() {}

    Datatype datatype;
    Dataspace dataspace;
    FillValue fillvalue;
    my::map<Attribute> attributes;
    std::deque<short> filters;
    Data data;
  };
  class DatasetEntry {
  public:
    DatasetEntry() : key(),dataset(nullptr) {}

    std::string key;
    std::shared_ptr<Dataset> dataset;
  };
  class Group;
  class GroupEntry {
  public:
    GroupEntry() : key(),group(nullptr) {}

    std::string key;
    std::shared_ptr<Group> group;
  };
  class Group {
  public:
    Group() : version(0),local_heap(),btree_addr(0),tree(),groups(),datasets() {}

    int version;
    struct LocalHeap {
	LocalHeap() : addr(0),data_start(0) {}

	unsigned long long addr,data_start;
    } local_heap;
    unsigned long long btree_addr;
    Tree tree;
    my::map<GroupEntry> groups;
    my::map<DatasetEntry> datasets;
  };
  struct FractalHeapData {
    FractalHeapData() : dse(nullptr),start_block_size(0),objects(),id_len(0),io_filter_size(0),max_size(0),table_width(0),max_dblock_rows(0),nrows(0),K(0),N(0),max_dblock_size(0),max_managed_obj_size(0),curr_row(0),curr_col(0),flags() {}

    DatasetEntry *dse;
    unsigned long long start_block_size;
    struct Objects {
	Objects() : num_managed(0),num_huge(0),num_tiny(0) {}

	unsigned long long num_managed,num_huge,num_tiny;
    } objects;
    int id_len,io_filter_size,max_size,table_width,max_dblock_rows,nrows,K,N;
    int max_dblock_size,max_managed_obj_size;
    int curr_row,curr_col;
    unsigned char flags;
  };

  InputHDF5Stream() : sb_version(0),sizes(),group_K(),base_addr(0),eof_addr(0),undef_addr(0),root_group(),ref_table(nullptr),show_debug(false) {}
  ~InputHDF5Stream();
  void close();
  bool debugIsOn() const { return show_debug; }
  Attribute getAttribute(std::string xpath);
//  int getData(Dataset& dataset,void **values);
  Dataset *getDataset(std::string xpath);
  std::list<InputHDF5Stream::DatasetEntry> getDatasetsWithAttribute(std::string attribute_path,Group *g = NULL);
  std::fstream *file_stream() { return &fs; }
  std::shared_ptr<my::map<ReferenceEntry>> getReferenceTablePointer() const { return ref_table; }
  short getSizeOfOffsets() const { return sizes.offsets; }
  short getSizeOfLengths() const { return sizes.lengths; }
  unsigned long long getUndefinedAddress() const { return undef_addr; }
  int ignore() { return bfstream::error; }
  bool open(const char *filename);
  int peek();
  void printData(Dataset& dataset);
  void printFillValue(std::string xpath);
  int read(unsigned char *buffer,size_t buffer_length);
  void setDebug(bool show_debug_messages) { show_debug=show_debug_messages; }
  void showFileStructure();

protected:
  void clearGroups(Group& group);
  bool decodeAttribute(unsigned char *buffer,Attribute& attribute,int& length,int ohdr_version);
  bool decodeBTree(Group& group,FractalHeapData *frhp_data);
  bool decodeFractalHeap(unsigned long long address,int header_message_type,FractalHeapData& frhp_data);
  bool decodeFractalHeapBlock(unsigned long long address,int header_message_type,FractalHeapData& frhp_data);
  bool decodeHeaderMessages(int ohdr_version,size_t header_size,std::string ident,Group *group,DatasetEntry *dse_in,unsigned char flags);
  int decodeHeaderMessage(std::string ident,int ohdr_version,int type,int length,unsigned char flags,unsigned char *buffer,Group *group,DatasetEntry& dse,bool& is_subgroup);
  bool decodeObjectHeader(SymbolTableEntry& ste,Group *group);
  bool decodeSuperblock(unsigned long long& objhdr_addr);
  bool decodeSymbolTableEntry(SymbolTableEntry& ste,Group *group);
  bool decodeSymbolTableNode(unsigned long long address,Group& group);
  bool decodeV1BTree(Group& group);
  bool decodeV1BTree(unsigned long long address,Dataset& dataset);
  bool decodeV2BTree(Group& group,FractalHeapData *frhp_data);
  bool decodeV2BTreeNode(unsigned long long address,int num_records,FractalHeapData *frhp_data);
  void printAGroupTree(Group& group);

  int sb_version;
  struct Sizes {
    Sizes() : offsets(0),lengths(0) {}

    size_t offsets,lengths;
  } sizes;
  struct GroupK {
    GroupK() : leaf(0),internal(0) {}

    int leaf,internal;
  } group_K;
  unsigned long long base_addr,eof_addr,undef_addr;
  Group root_group;
  std::shared_ptr<my::map<ReferenceEntry>> ref_table;
  bool show_debug;
};

namespace HDF5 {

class DataArray {
public:
  enum {null_=0,short_,int_,long_long_,float_,double_,string_};
  DataArray() : num_values(0),type(0),values(nullptr),dimensions() {}
  DataArray(const DataArray& source) : DataArray() { *this=source; }
  ~DataArray() { clear(); }
  DataArray& operator=(const DataArray& source);
  bool fill(InputHDF5Stream& istream,InputHDF5Stream::Dataset& dataset,size_t compound_member_index = 0);
  short short_value(size_t index) const;
  int int_value(size_t index) const;
  long long long_long_value(size_t index) const;
  float float_value(size_t index) const;
  double double_value(size_t index) const;
  std::string string_value(size_t index) const;

  size_t num_values,type;
  void *values;
  std::vector<size_t> dimensions;

private:
  void clear();
};

void printAttribute(InputHDF5Stream::Attribute& attribute,std::shared_ptr<my::map<InputHDF5Stream::ReferenceEntry>> ref_table);
void printDataValue(InputHDF5Stream::Datatype& datatype,void *value);

std::string datatypeClassToString(const InputHDF5Stream::Datatype& datatype);

bool decodeCompoundDatatype(const InputHDF5Stream::Datatype& datatype,InputHDF5Stream::CompoundDatatype& compound_datatype,bool show_debug);
bool decodeCompoundDataValue(unsigned char *buffer,InputHDF5Stream::Datatype& datatype,void ***values);
bool decodeDataspace(unsigned char *buffer,unsigned long long size_of_lengths,InputHDF5Stream::Dataspace& dataspace,bool show_debug);
bool decodeDatatype(unsigned char *buffer,InputHDF5Stream::Datatype& datatype,bool show_debug);
//bool decodeFixedPointNumberArray(const unsigned char *buffer,const InputHDF5Stream::Datatype& datatype,int size_of_element,void **values,int num_values,size_t& index,size_t chunk_length);
//bool decodeFloatingPointNumberArray(const unsigned char *buffer,const InputHDF5Stream::Datatype& datatype,int size_of_element,void **values,int& precision,int num_values,size_t& index,size_t chunk_length);

int getGlobalHeapObject(std::fstream& fs,short size_of_lengths,unsigned long long address,int index,unsigned char **buffer);

unsigned long long getValue(const unsigned char *buffer,int num_bytes);

}; // end namespace HDF5

#endif
