#ifndef METAHELPERS_H
#define   METAHELPERS_H

#include <PostgreSQL.hpp>
#include <string>
#include <vector>
#include <gridutils.hpp>

namespace html {

struct Node {
  Node() : copy(),value(),node_list() {}

  std::string copy,value;
  std::vector<Node> node_list;
};

extern void wrap(std::string& string,std::string fill,size_t chars);
extern void nodes(std::string string,std::vector<Node>& nodes);
extern void process_node(Node& node,size_t wrap_length);
extern std::string html_text_to_ascii(std::string& element_copy,size_t wrap_length);

} // end namespace html

namespace metacompares {

extern bool compare_grid_definitions(const std::string& left,const std::string& right);
extern bool compare_levels(const std::string& left,const std::string& right);
extern bool compare_time_ranges(const std::string& left,const std::string& right);
extern bool default_compare(const std::string& left,const std::string& right);

} // end namespace metacompares

namespace metatransforms {

extern bool db2grml(std::string dsid, std::string wfile, std::string db, const
    PostgreSQL::DBconfig& db_config, std::string level_map_path, std::string
    target_dir = ".");
extern bool db2obml(std::string dsid, std::string wfile, std::string db, const
    PostgreSQL::DBconfig& db_config, std::string target_dir = ".");

} // end namespace metatransforms

namespace metatranslations {

extern std::string convert_meta_format_to_link(std::string format);
extern std::string convert_meta_grid_definition(std::string grid_definition,std::string definition_parameters,const char *separator);
extern std::string convert_meta_parameter_map(std::string format,std::string map,bool verbose_output);
extern std::string date_time(std::string database_date_time,std::string database_flag,std::string time_zone,std::string time_delimiter = " ");
extern void decode_grid_definition_parameters(std::string definition,std::string def_params,Grid::GridDimensions& grid_dim,Grid::GridDefinition& grid_def);
extern std::string detailed_datatype(xmlutils::DataTypeMapper& data_type_mapper,const std::string& format,const std::string& code);
extern std::string detailed_level(xmlutils::LevelMapper& level_mapper,const std::string& format,const std::string& map,const std::string& type,const std::string& value,bool htmlify_units);
extern std::string detailed_parameter(xmlutils::ParameterMapper& parameter_mapper,const std::string& format,const std::string& code);
extern std::string string_coordinate_to_db(std::string coordinate_value);
extern std::string translate_platform(std::string definition);

} // end namespace metatranslations

#endif
