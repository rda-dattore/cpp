// FILE: xmlutils.h

#include <list>
#include <mymap.hpp>
#include <xml.hpp>
#include <grid.hpp>

#ifndef PARAMETERMAP_H
#define  PARAMETERMAP_H

class mapStructures
{
public:
  struct AttributeEntry {
    AttributeEntry() : key(),value() {}

    std::string key;
    std::string value;
  };

  mapStructures() {}
  virtual ~mapStructures() {}
};

class ParameterMap : public mapStructures
{
public:
  struct PMapEntry {
    PMapEntry() : key(),short_name_(),description_(),units_(),comment_(),standard_name_(),long_name_(),CF(),GCMD(),ISO() {}
    void reset() {
	short_name_="";
	description_="";
	units_="";
	comment_="";
	standard_name_="";
	long_name_="";
	CF.clear();
	GCMD.clear();
	ISO.clear();
    }

    std::string key;
    std::string short_name_,description_,units_,comment_,standard_name_,long_name_;
    my::map<AttributeEntry> CF,GCMD,ISO;
  };
  enum {CF_code=1,GCMD_code,ISO_code};

  ParameterMap() : title_(),parameter_map_table() {}
  ParameterMap(std::string xml_file) : title_(),parameter_map_table() { fill(xml_file); }
  bool fill(std::string xml_file);
  std::string cf_keyword(std::string parameter_code,std::string level_type = "") { return keyword(CF_code,parameter_code,level_type,""); }
  std::string comment(std::string parameter_code);
  std::string description(std::string parameter_code);
  std::string gcmd_keyword(std::string parameter_code,std::string level_type = "") { return keyword(GCMD_code,parameter_code,level_type,""); }
  std::string gcmd_keyword(std::string parameter_code,std::string level_type,std::string level_value) { return keyword(GCMD_code,parameter_code,level_type,level_value); }
  std::string iso_keyword(std::string parameter_code,std::string level_type = "") { return keyword(ISO_code,parameter_code,level_type,""); }
  std::string long_name(std::string parameter_code);
  std::string short_name(std::string parameter_code);
  std::string standard_name(std::string parameter_code);
  std::string title() const { return title_; }
  std::string units(std::string parameter_code);

private:
  std::string keyword(size_t keyword_code,std::string parameter_code,std::string level_type,std::string level_value);

  std::string title_;
  my::map<PMapEntry> parameter_map_table;
};

#endif

#ifndef CONTINUE_XMLUTILS_H
#define   CONTINUE_XMLUTILS_H
class LevelMap
{
public:
  struct LMapEntry {
    LMapEntry() : key(),short_name_(),description_(),units_(),is_layer_(false) {}
    void reset() {
	short_name_="";
	description_="";
	units_="";
	is_layer_=false;
    }

    std::string key;
    std::string short_name_,description_,units_;
    bool is_layer_;
  };

  LevelMap() : level_map_table() {}
  LevelMap(std::string xml_file) : level_map_table() { fill(xml_file); }
  bool fill(std::string xml_file);
  std::string description(std::string level_type);
  size_t size() const { return level_map_table.size(); }
  std::string short_name(std::string level_type);
  std::string units(std::string level_type);
  int is_layer(std::string level_type);

private:
  my::map<LMapEntry> level_map_table;
};

class DataTypeMap : public mapStructures
{
public:
  struct DTMapEntry {
    DTMapEntry() : key(),description_(),gcmd_list(),iso_topic_list(),variable_list() {}
    void reset() {
	description_.clear();
	gcmd_list.clear();
	iso_topic_list.clear();
	variable_list.clear();
    }

    std::string key;
    my::map<AttributeEntry> description_;
    std::list<std::string> gcmd_list,iso_topic_list,variable_list;
  };
  DataTypeMap() : datatype_map_table() { }
  DataTypeMap(const std::string& xml_file) : datatype_map_table() { fill(xml_file); }
  bool fill(const std::string& xml_file);
  std::string description(const std::string& datatype_code,std::string platform_type = "");
  std::list<std::string> gcmd_keywords(const std::string& datatype_code);
  std::list<std::string> iso_topic_keywords(const std::string& datatype_code);
  std::list<std::string> variables(const std::string& datatype_code);

private:
  my::map<DTMapEntry> datatype_map_table;
};

class GridMap {
public:
  struct GMapEntry {
    GMapEntry() : key(),def(),dim(),single_pole_point(false),pole_point_location_() {}

    void reset() {
	def.type=0;
	dim.x=dim.y=0;
	single_pole_point=false;
	pole_point_location_="";
    }

    std::string key;
    Grid::GridDefinition def;
    Grid::GridDimensions dim;
    bool single_pole_point;
    std::string pole_point_location_;
  };

  GridMap() : grid_map_table() {}
  bool fill(const std::string& xml_file);
  Grid::GridDefinition grid_definition(const std::string& grid_code);
  Grid::GridDimensions grid_dimensions(const std::string& grid_code);
  bool is_single_pole_point(const std::string& grid_code);
  std::string pole_point_location(const std::string& grid_code);

private:
  my::map<GMapEntry> grid_map_table;
};

namespace xmlutils {

class ParameterMapEntry {
public:
  ParameterMapEntry() : key(),p(nullptr) {}

  std::string key;
  std::shared_ptr<ParameterMap> p;
};
struct LevelMapEntry {
  LevelMapEntry() : key(),l(nullptr) {}

  std::string key;
  std::shared_ptr<LevelMap> l;
};
struct DataTypeMapEntry {
  DataTypeMapEntry() : key(),d(nullptr) {}

  std::string key;
  std::shared_ptr<DataTypeMap> d;
};
struct GridMapEntry {
  GridMapEntry() : key(),g() {}

  std::string key;
  GridMap g;
};

class ParameterMapper
{
public:
  ParameterMapper() = delete;
  ParameterMapper(std::string path_to_map) : _path_to_map(),parameter_map_table(new my::map<ParameterMapEntry>(99999)) { _path_to_map=path_to_map; }
  ParameterMapper(const ParameterMapper& source) : ParameterMapper("") { *this=source; }
  ~ParameterMapper() { delete parameter_map_table; }
  ParameterMapper& operator=(const ParameterMapper& source) {
    if (this != &source) {
	_path_to_map=source._path_to_map;
	*parameter_map_table=*source.parameter_map_table;
    }
    return *this;
  }
  std::string cf_keyword(std::string data_format,std::string parameter_code,std::string level_type = "");
  std::string comment(std::string data_format,std::string parameter_code);
  std::string description(std::string data_format,std::string parameter_code);
  std::string gcmd_keyword(std::string data_format,std::string parameter_code,std::string level_type = "");
  std::string long_name(std::string data_format,std::string parameter_code);
  void set_path_to_map(std::string path_to_map) { _path_to_map=path_to_map; }
  std::string short_name(std::string data_format,std::string parameter_code);
  size_t size() const { return parameter_map_table->size(); }
  std::string standard_name(std::string data_format,std::string parameter_code);
  std::string title(std::string data_format,std::string format_specialization = "");
  std::string units(std::string data_format,std::string parameter_code);

private:
  ParameterMapEntry entry(std::string data_format,std::string format_specialization);

  std::string _path_to_map;
  my::map<ParameterMapEntry> *parameter_map_table;
};

class LevelMapper
{
public:
  LevelMapper() = delete;
  LevelMapper(std::string path_to_map) : _path_to_map(),level_map_table(new my::map<LevelMapEntry>(99999)) { _path_to_map=path_to_map; }
  LevelMapper(const LevelMapper& source) : LevelMapper("") { *this=source; }
  ~LevelMapper() { delete level_map_table; }
  LevelMapper& operator=(const LevelMapper& source) {
    if (this != &source) {
	_path_to_map=source._path_to_map;
	*level_map_table=*source.level_map_table;
    }
    return *this;
  }
  std::string description(std::string data_format,std::string level_type,std::string format_specialization = "");
  int level_is_a_layer(std::string data_format,std::string level_type,std::string format_specialization = "");
  void set_path_to_map(std::string path_to_map) { _path_to_map=path_to_map; }
  std::string short_name(std::string data_format,std::string level_type,std::string format_specialization = "");
  size_t size() const { return level_map_table->size(); }
  std::string units(std::string data_format,std::string level_type,std::string format_specialization = "");

private:
  LevelMapEntry entry(std::string data_format,std::string format_specialization);

  std::string _path_to_map;
  my::map<LevelMapEntry> *level_map_table;
};

class DataTypeMapper
{
public:
  DataTypeMapper() = delete;
  DataTypeMapper(std::string path_to_map) : _path_to_map(),datatype_map_table(new my::map<DataTypeMapEntry>(99999)) { _path_to_map=path_to_map; }
  DataTypeMapper(const DataTypeMapper& source) : DataTypeMapper("") { *this=source; }
  ~DataTypeMapper() { delete datatype_map_table; }
  DataTypeMapper& operator=(const DataTypeMapper& source) {
    if (this != &source) {
	_path_to_map=source._path_to_map;
	*datatype_map_table=*source.datatype_map_table;
    }
    return *this;
  }
  std::string description(std::string data_format,std::string format_specialization,std::string data_type_code,std::string platform_type = "");
  std::list<std::string> gcmd_keywords(std::string data_format,std::string format_specialization,std::string data_type_code);
  void set_path_to_map(std::string path_to_map) { _path_to_map=path_to_map; }
  size_t size() const { return datatype_map_table->size(); }
  std::list<std::string> variables(std::string data_format,std::string format_specialization,std::string data_type_code);

private:
  DataTypeMapEntry entry(std::string data_format,std::string format_specialization);

  std::string _path_to_map;
  my::map<DataTypeMapEntry> *datatype_map_table;
};

extern void clean_parameter_code(std::string& parameter_code,std::string& format_specialization);

//extern std::string copy_of(XMLElement& e,size_t indent = 0);
extern std::string map_filename(std::string directory,std::string file_base_name,std::string temp_dir_name);
extern std::string to_plain_text(const XMLElement& e,size_t wrap_width = 80,size_t indent_width = 0);

extern std::deque<std::string> split(const std::string& s);

} // end namespace xmlutils

#endif
