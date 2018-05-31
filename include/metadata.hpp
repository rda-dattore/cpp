#ifndef METADATA_H
#define   METADATA_H

#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <mymap.hpp>
#include <grid.hpp>
#include <xml.hpp>
#include <tempfile.hpp>
#include <MySQL.hpp>
#include <satellite.hpp>
#include <datetime.hpp>
#include <tokendoc.hpp>

namespace metautils {

struct Directives {
  Directives() : temp_path(),data_root(),data_root_alias(),host(),web_server(),database_server(),rdadb_username(),rdadb_password(),metadb_username(),metadb_password(),metadata_manager(),server_root(),dss_root(),local_root(),dss_bindir(),rdadata_home(),parameter_map_path(),level_map_path() {}

  std::string temp_path,data_root,data_root_alias;
  std::string host,web_server,database_server,rdadb_username,rdadb_password,metadb_username,metadb_password;
  std::string metadata_manager;
  std::string server_root,dss_root,local_root;
  std::string dss_bindir,rdadata_home,parameter_map_path,level_map_path;
};
struct Args {
  Args() : args_string(),dsnum(),data_format(),reg_key(),path(),filename(),local_name(),member_name(),temp_loc(),update_db(false),update_summary(false),override_primary_check(false),overwrite_only(false),regenerate(false),update_graphics(false),inventory_only(false) {}

  std::string args_string,dsnum;
  std::string data_format,reg_key;
  std::string path,filename,local_name,member_name,temp_loc;
  bool update_db,update_summary,override_primary_check,overwrite_only,regenerate;
  bool update_graphics,inventory_only;
};
struct StringEntry {
  StringEntry() : key() {}

  std::string key;
};
struct SizeTEntry {
  SizeTEntry() : key() {}

  size_t key;
};
struct InformationEntry {
  InformationEntry() : key(),info(),unique_info_table(),marked() {}

  std::string key;
  std::shared_ptr<std::vector<std::string>> info;
  std::shared_ptr<my::map<StringEntry>> unique_info_table;
  std::shared_ptr<bool> marked;
};
struct GrMLQueryStruct {
  struct Lists {
    Lists() : parameter_codes(nullptr),level_codes(nullptr),gindexes(nullptr),file_IDs(nullptr),parameters(nullptr),levels(nullptr),data_formats(nullptr),products(nullptr),grid_definitions(nullptr),processes(nullptr) {}

    std::list<std::string> *parameter_codes,*level_codes;
    std::list<std::string> *gindexes,*file_IDs;
    std::list<std::string> *parameters,*levels;
    std::list<InformationEntry> *data_formats,*products,*grid_definitions,*processes;
  };
  struct Tables {
    Tables() : level_codes(nullptr),parameters(nullptr),levels(nullptr) {}

    my::map<InformationEntry> *level_codes;
    my::map<InformationEntry> *parameters,*levels;
  };
  struct Hashes {
    Hashes() : levels(nullptr),products(nullptr),grid_definitions(nullptr),data_formats(nullptr),groups(nullptr),rda_files(nullptr),metadata_files(nullptr) {}

    my::map<InformationEntry> *levels,*products,*grid_definitions,*data_formats,*groups;
    my::map<InformationEntry> *rda_files,*metadata_files;
  };

  GrMLQueryStruct() : gindex(nullptr),ptfile(nullptr),start_datetime(nullptr),end_datetime(nullptr),product_code(nullptr),grid_definition_code(nullptr),topt_mo(nullptr),lists(),tables(),hashes() {}
  void initialize()
  {
    if (lists.parameters == nullptr) {
	lists.parameters=new std::list<std::string>;
    }
    else {
	lists.parameters->clear();
    }
    if (lists.levels == nullptr) {
	lists.levels=new std::list<std::string>;
    }
    else {
	lists.levels->clear();
    }
    if (lists.data_formats == nullptr) {
	lists.data_formats=new std::list<InformationEntry>;
    }
    else {
	lists.data_formats->clear();
    }
    if (lists.products == nullptr) {
	lists.products=new std::list<InformationEntry>;
    }
    else {
	lists.products->clear();
    }
    if (lists.grid_definitions == nullptr) {
	lists.grid_definitions=new std::list<InformationEntry>;
    }
    else {
	lists.grid_definitions->clear();
    }
    if (lists.processes == nullptr) {
	lists.processes=new std::list<InformationEntry>;
    }
    else {
	lists.processes->clear();
    }
    if (tables.level_codes == nullptr) {
	tables.level_codes=new my::map<InformationEntry>;
    }
    else {
	tables.level_codes->clear();
    }
    if (tables.parameters == nullptr) {
	tables.parameters=new my::map<InformationEntry>;
    }
    else {
	tables.parameters->clear();
    }
    if (tables.levels == nullptr) {
	tables.levels=new my::map<InformationEntry>;
    }
    else {
	tables.levels->clear();
    }
    if (hashes.levels == nullptr) {
	hashes.levels=new my::map<InformationEntry>;
    }
    else {
	hashes.levels->clear();
    }
    if (hashes.groups == nullptr) {
	hashes.groups=new my::map<InformationEntry>;
    }
    else {
	hashes.groups->clear();
    }
  }
  void reset()
  {
    start_datetime.reset();
    end_datetime.reset();
    if (lists.parameter_codes != nullptr) {
	delete lists.parameter_codes;
	lists.parameter_codes=nullptr;
    }
    if (lists.level_codes != nullptr) {
	delete lists.level_codes;
	lists.level_codes=nullptr;
    }
  }

  std::shared_ptr<std::string> gindex,ptfile;
  std::shared_ptr<std::string> start_datetime,end_datetime;
  std::shared_ptr<std::string> product_code,grid_definition_code;
  std::shared_ptr<bool> topt_mo;
  Lists lists;
  Tables tables;
  Hashes hashes;
};

namespace NcTime {

struct Time {
  Time() : t1(0.),t2(0.),times(nullptr),num_times(0) {}

  double t1,t2,*times;
  size_t num_times;
};
struct TimeRange {
  TimeRange() : first_valid_datetime(),last_valid_datetime() {}

  DateTime first_valid_datetime,last_valid_datetime;
};
struct TimeBounds {
  TimeBounds() : t1(0.),t2(0.),diff(0),changed(false) {}

  double t1,t2,diff;
  bool changed;
};
struct TimeData {
  TimeData() : reference(),units(),calendar() {}

  DateTime reference;
  std::string units,calendar;
};
struct TimeRangeEntry {
  struct Data {
    Data() : unit(-1),num_steps(0),instantaneous(),bounded() {}

    int unit,num_steps;
    TimeRange instantaneous,bounded;
  };
  TimeRangeEntry() : key(),data(nullptr) {}

  size_t key;
  std::shared_ptr<Data> data;
};

DateTime actual_date_time(double time,TimeData& time_data,std::string& error);

std::string gridded_netcdf_time_range_description(const TimeRangeEntry& tre,const TimeData& time_data,std::string method,std::string& error);
std::string time_method_from_cell_methods(std::string cell_methods,std::string timeid);

} // end namespace NcTime

namespace NcLevel {

struct LevelInfo {
  LevelInfo() : ID(),description(),units(),write() {}

  std::vector<std::string> ID,description,units;
// have to use this instead of vector<bool>, which has problems
  std::vector<unsigned char> write;
};

std::string write_level_map(const LevelInfo& level_info);

} // end namespace NcLevel

namespace NcParameter {

std::string write_parameter_map(std::list<std::string>& varlist,my::map<metautils::StringEntry>& var_changes_table,std::string map_type,std::string map_name,bool found_map,std::string& warning);

} // end namespace NcParameter

namespace CF {

extern void fill_nc_time_data(std::string units_attribute_value,NcTime::TimeData& time_data,std::string user);

} // end namespace CF

namespace primaryMetadata {

extern void match_web_file_to_hpss_primary(std::string url,std::string& metadata_file);

extern bool expand_file(std::string dirname,std::string filename,std::string *file_format);
extern bool open_file_for_metadata_scanning(void *istream,std::string filename,std::string& error);
extern bool prepare_file_for_metadata_scanning(TempFile& tfile,TempDir& tdir,std::list<std::string> *filelist,std::string& file_format,std::string& error);

} // end namespace primaryMetadata

const std::string tmpdir="/tmp";

extern "C" void cmd_unregister();

extern void check_for_existing_cmd(std::string cmd_type);
extern void clean_parameter_code(std::string& parameter_code,std::string& parameter_map);
extern void cmd_register(std::string cmd,std::string user);
extern void log_error(std::string error,std::string caller,std::string user,bool no_exit = false);
extern void log_info(std::string message,std::string caller,std::string user);
extern void log_warning(std::string warning,std::string caller,std::string user);
extern void obs_per(std::string observation_type_value,size_t num_obs,DateTime start,DateTime end,double& obsper,std::string& unit);

extern std::string clean_id(std::string id);
extern std::string relative_web_filename(std::string url);
extern std::string web_home();

extern std::list<std::string> cmd_databases(std::string caller,std::string user);

extern bool read_config(std::string caller,std::string user,bool restrict_to_user_rdadata = true);

} // end namespace metautils

extern metautils::Directives meta_directives;
extern metautils::Args meta_args;

namespace summarizeMetadata {

struct BoxFlags {
  BoxFlags() : num_y(0),num_x(0),flags(nullptr),npole(),spole() {}
  BoxFlags(const BoxFlags& source) : BoxFlags() { *this=source; }
  ~BoxFlags() {
    clear();
  }
  BoxFlags& operator=(const BoxFlags& source) {
    if (this == &source) {
	return *this;
    }
    clear();
    num_y=source.num_y;
    num_x=source.num_x;
    flags=new unsigned char *[num_y];
    for (size_t n=0; n < num_y; ++n) {
	flags[n]=new unsigned char[num_x];
	for (size_t m=0; m < num_x; ++m) {
	  flags[n][m]=source.flags[n][m];
	}
    }
    npole=source.npole;
    spole=source.spole;
  }
  void clear() {
    if (flags != nullptr) {
	for (size_t n=0; n < num_y; ++n) {
	  delete[] flags[n];
	}
	delete[] flags;
	flags=nullptr;
    }
  }
  void initialize(size_t dim_x,size_t dim_y,unsigned char n_pole,unsigned char s_pole) {
    clear();
    flags=new unsigned char *[dim_y];
    for (size_t n=0; n < dim_y; ++n) {
	flags[n]=new unsigned char[dim_x];
	for (size_t m=0; m < dim_x; flags[n][m++]=0);
    }
    npole=n_pole;
    spole=s_pole;
  }

  size_t num_y,num_x;
  unsigned char **flags;
  unsigned char npole,spole;
};
struct CMDDateRange {
  CMDDateRange() : start(),end(),gindex() {}

  std::string start,end;
  std::string gindex;
};
struct FileEntry {
  FileEntry() : key(),backup_name(),data_format(),units(),metafile_ID(),mod_time(),start(),end(),file_format(),data_size(),group_ID(),annotation(),inv() {}

  std::string key;
  std::string backup_name,data_format,units,metafile_ID,mod_time;
  std::string start,end;
  std::string file_format,data_size;
  std::string group_ID,annotation,inv;
};
struct Entry {
  Entry() : key(),code() {}

  std::string key;
  std::string code;
};
struct ParameterEntry {
  ParameterEntry() : key(),description(),short_name(),count(0),selected(false) {}

  std::string key;
  std::string description,short_name;
  size_t count;
  bool selected;
};

extern std::string string_date_to_ll_string(std::string date);
extern std::string string_ll_to_date_string(std::string ll_date);
extern std::string set_date_time_string(std::string datetime,std::string flag,std::string time_zone,std::string time_prefix = " ");
extern std::string summarize_locations(std::string database);

extern void create_file_list_cache(std::string file_type,std::string caller,std::string user,std::string gindex = "");
extern void create_non_cmd_file_list_cache(std::string file_type,my::map<Entry>& files_with_cmd_table,std::string caller,std::string user);
extern void grids_per(size_t nsteps,DateTime start,DateTime end,double& gridsper,std::string& unit);
extern void summarize_data_formats(std::string caller,std::string user);
extern void summarize_dates(std::string caller,std::string user);
extern void summarize_frequencies(std::string caller,std::string user,std::string mss_ID_code = "");

namespace gridLevels {

struct LevelEntry {
  LevelEntry() : key() {}

  std::string key;
};

void summarize_grid_levels(std::string database,std::string caller,std::string user);

} // end namespace summarizeMetadata::gridLevels

namespace grids {

struct LevelCodeEntry {
  LevelCodeEntry() : key() {}

  size_t key;
};
struct FormatCodeEntry {
  FormatCodeEntry() : key(),code() {}

  std::string key;
  std::string code;
};
struct SummaryEntry {
  struct Data {
    Data() : start(),end(),level_code_table() {}

    long long start,end;
    my::map<LevelCodeEntry> level_code_table;
  };
  SummaryEntry() : key(),data(nullptr) {}

  std::string key;
  std::shared_ptr<Data> data;
};
struct GridDefinitionEntry {
  GridDefinitionEntry() : key(),definition(),def_params() {}

  std::string key;
  std::string definition,def_params;
};

extern void aggregate_grids(std::string database,std::string caller,std::string user,std::string file_ID_code = "");
extern void summarize_grids(std::string database,std::string caller,std::string user,std::string file_ID_code = "");
extern void summarize_grid_resolutions(std::string caller,std::string user,std::string mss_ID_code = "");

extern bool compare_values(const size_t& left,const size_t& right);

} // end namespace summarizeMetadata::grids

namespace obsData {

struct SummaryEntry {
  SummaryEntry() : key(),start_date(),end_date(),box1d_bitmap() {}

  std::string key;
  std::string start_date,end_date;
  std::string box1d_bitmap;
};
struct TypeEntry {
  TypeEntry() : key(),start(),end(),type_table(nullptr),type_table_keys(nullptr),type_table_keys_2(nullptr) {}

  std::string key;
  std::string start,end;
  std::shared_ptr<my::map<TypeEntry>> type_table;
  std::shared_ptr<std::list<std::string>> type_table_keys,type_table_keys_2;
};
struct ParentLocation {
  ParentLocation() : key(),children_table(nullptr),matched_table(nullptr),consolidated_parent_table(nullptr) {}

  std::string key;
  std::shared_ptr<my::map<Entry>> children_table,matched_table,consolidated_parent_table;
};
struct PointEntry {
  PointEntry() : key(),locations() {}

  std::string key;
  std::shared_ptr<std::list<std::string>> locations;
};

extern void check_point(double latp,double lonp,MySQL::Server& server,my::map<Entry>& location_table);
extern void compress_locations(std::list<std::string>& location_list,my::map<ParentLocation>& parent_location_table,std::vector<std::string>& sorted_array,std::string caller,std::string user);

extern bool summarize_obs_data(std::string caller,std::string user);

} // end namespace summarizeMetadata::obsData

namespace fixData {

void summarize_fix_data(std::string caller,std::string user);

} // end namespace summarizeMetadata::fixData

namespace detailedMetadata {

struct UniqueEntry {
  struct Data {
    Data() : start(),end() {}

    std::string start,end;
  };
  UniqueEntry() : key(),data() {}

  std::string key;
  std::shared_ptr<Data> data;
};
struct FrequencyEntry {
  FrequencyEntry() : key() {}

  std::string key;
};
struct LevelEntry {
  struct Range {
    Range() : start(),end() {}

    std::string start,end;
  };
  LevelEntry() : key(),range(),min_start(),max_end(),min_nsteps(),max_nsteps(),min_per(),max_per(),min_units(),max_units(),frequency_table() {}

  size_t key;
  std::shared_ptr<Range> range;
  std::shared_ptr<std::string> min_start,max_end;
  std::shared_ptr<size_t> min_nsteps,max_nsteps;
  std::shared_ptr<size_t> min_per,max_per;
  std::shared_ptr<std::string> min_units,max_units;
  std::shared_ptr<my::map<FrequencyEntry>> frequency_table;
};
struct ProductEntry {
  ProductEntry() : key(),level_table(),code_table() {}

  std::string key;
  std::shared_ptr<my::map<LevelEntry>> level_table;
  std::shared_ptr<my::map<UniqueEntry>> code_table;
};
struct ParameterEntry {
  ParameterEntry() : key(),parameter_code_table(),product_table(),level_table(),level_values() {}

  std::string key;
  std::shared_ptr<my::map<UniqueEntry>> parameter_code_table;
  std::shared_ptr<my::map<ProductEntry>> product_table;
  std::shared_ptr<my::map<LevelEntry>> level_table;
  std::shared_ptr<std::vector<size_t>> level_values;
};
struct ParameterDataEntry {
  ParameterDataEntry() : key(),short_name(),description(),units(),comment() {}

  std::string key;
  std::string short_name,description,units,comment;
};
struct ParameterTableEntry {
  ParameterTableEntry() : key(),parameter_data_table() {}

  std::string key;
  std::shared_ptr<my::map<ParameterDataEntry>> parameter_data_table;
};

extern void add_to_formats(XMLDocument& xdoc,MySQL::Query& query,std::list<std::string>& format_list,std::string& formats);
extern void generate_detailed_metadata_view(std::string caller,std::string user);
extern void generate_group_detailed_metadata_view(std::string group_index,std::string file_type,std::string caller,std::string user);

} // end namespace summarizeMetadata::detailedMetadata

} // end namespace summarizeMetadata

namespace metadata {

struct LevelCodeEntry {
  struct Data {
    Data() : map(),type(),value() {}

    std::string map,type,value;
  };

  LevelCodeEntry() : key(),data() {}

  size_t key;
  std::shared_ptr<Data> data;
};

void close_inventory(std::string filename,TempDir *tdir,std::ofstream& ofs,std::string cmd_type,bool insert_into_db,bool create_cache,std::string caller,std::string user);
void open_inventory(std::string& filename,TempDir **tdir,std::ofstream& ofs,std::string cmd_type,std::string caller,std::string user);

namespace GrML {

struct ParameterEntry {
  ParameterEntry() : key(),start_date_time(),end_date_time(),num_time_steps(0) {}

  std::string key;
  DateTime start_date_time,end_date_time;
  size_t num_time_steps;
};
struct LevelEntry {
  LevelEntry() : key(),units(),parameter_code_table() {}

  std::string key;
  std::string units;
  my::map<ParameterEntry> parameter_code_table;
};
struct GridEntry {
  GridEntry() : key(),level_table(),process_table(),ensemble_table() {}

  std::string key;
  my::map<LevelEntry> level_table;
  my::map<metautils::StringEntry> process_table,ensemble_table;
};

extern void copy_grml(std::string metadata_file,std::string URL,std::string caller,std::string user);
extern void write_grml(my::map<GridEntry>& grid_table,std::string caller,std::string user);

extern int compare_grid_table_keys(const std::string& left,const std::string& right);

} // end namespace metadata::GrML

namespace ObML {

struct DataTypeEntry {
  struct Data {
    Data() : map(),nsteps(0),vdata(nullptr) {}

    struct VerticalData {
	VerticalData() : min_altitude(1.e38),max_altitude(-1.e38),avg_res(0.),avg_nlev(0),res_cnt(0),units() {}

	float min_altitude,max_altitude,avg_res;
	size_t avg_nlev,res_cnt;
	std::string units;
    };

    std::string map;
    size_t nsteps;
    std::shared_ptr<VerticalData> vdata;
  };

  DataTypeEntry() : key(),data(nullptr) {}

  std::string key;
  std::shared_ptr<Data> data;
};
struct IDEntry {
  struct Data {
    Data() : S_lat(),N_lat(),W_lon(),E_lon(),min_lon_bitmap(nullptr),max_lon_bitmap(nullptr),start(),end(),nsteps(),data_types_table() {}

    float S_lat,N_lat,W_lon,E_lon;
//    float min_lon_bitmap[360],max_lon_bitmap[360];
std::unique_ptr<float []> min_lon_bitmap,max_lon_bitmap;
    DateTime start,end;
    size_t nsteps;
    my::map<DataTypeEntry> data_types_table;
  };

  IDEntry() : key(),data(nullptr) {}

  std::string key;
  std::shared_ptr<Data> data;
};
struct PlatformEntry {
  PlatformEntry() : key(),boxflags(nullptr) {}

  std::string key;
  std::shared_ptr<summarizeMetadata::BoxFlags> boxflags;
};
struct ObservationData {
  ObservationData();
  bool add_to_ids(std::string observation_type,IDEntry& ientry,std::string data_type,float lat,float lon,double unique_timestamp,DateTime *start_datetime,DateTime *end_datetime = nullptr);
  bool add_to_platforms(std::string observation_type,std::string platform_type,float lat,float lon);

  size_t num_types;
  std::unordered_map<size_t,std::string> observation_types;
  std::unordered_map<std::string,size_t> observation_indexes;
  std::vector<my::map<IDEntry> *> id_tables;
  std::vector<my::map<PlatformEntry> *> platform_tables;
  std::unordered_map<std::string,std::vector<std::string>> unique_observation_table;
  std::regex unknown_id_re;
  std::unique_ptr<std::unordered_set<std::string>> unknown_ids;
};

extern "C" int list_ancillary_ObML_files(const char *name,const struct stat64 *data,int flag,struct FTW *ftw_struct);

extern void copy_obml(std::string metadata_file,std::string URL,std::string caller,std::string user);
extern void write_obml(ObservationData& obs_data,std::string caller,std::string user);

extern std::string string_coordinate_to_db(std::string coordinate_value);

extern int compareStrings(const std::string& left,const std::string& right);

} // end namespace metadata::ObML

namespace SatML {

struct ScanLine {
  ScanLine() : date_time(),first_coordinate(),last_coordinate(),subpoint(),res(0.),width(0.),subpoint_resolution(0.) {}

  DateTime date_time;
  SatelliteS::EarthCoordinate first_coordinate,last_coordinate,subpoint;
  double res;
  float width,subpoint_resolution;
};
struct ScanLineEntry {
  ScanLineEntry() : key(),sat_ID(),start_date_time(),end_date_time(),scan_line_list() {}

  std::string key;
  std::string sat_ID;
  DateTime start_date_time,end_date_time;
  std::list<ScanLine> scan_line_list;
};
struct Image {
  Image() : date_time(),corners(),center(),xres(0.),yres(0.) {}

  DateTime date_time;
  SatelliteImage::ImageCorners corners;
  SatelliteImage::EarthCoordinate center;
  float xres,yres;
};
struct ImageEntry {
  ImageEntry() : key(),start_date_time(),end_date_time(),image_list() {}

  std::string key;
  DateTime start_date_time,end_date_time;
  std::list<Image> image_list;
};

extern void write_satml(my::map<ScanLineEntry>& scan_line_table,std::list<std::string>& scan_line_table_keys,my::map<ImageEntry>& image_table,std::list<std::string>& image_table_keys,std::string caller,std::string user);

} // end namespace metadata::SatML

namespace FixML {

struct ClassificationEntry {
  ClassificationEntry() : key(),src(),start_datetime(),end_datetime(),start_lat(0.),start_lon(0.),end_lat(0.),end_lon(0.),min_lat(0.),min_lon(0.),max_lat(0.),max_lon(0.),min_pres(0.),max_pres(0.),min_speed(0.),max_speed(0.),nfixes(0),pres_units(),wind_units() {}

  std::string key;
  std::string src;
  DateTime start_datetime,end_datetime;
  float start_lat,start_lon,end_lat,end_lon;
  float min_lat,min_lon,max_lat,max_lon;
  float min_pres,max_pres,min_speed,max_speed;
  size_t nfixes;
  std::string pres_units,wind_units;
};
struct FeatureEntry {
  struct Data {
    Data() : alt_ID(),classification_list() {}

    std::string alt_ID;
    std::list<ClassificationEntry> classification_list;
  };
  FeatureEntry() : key(),data(nullptr) {}

  std::string key;
  std::shared_ptr<Data> data;
};
struct StageEntry {
  struct Data {
    Data() : boxflags(),start(),end() {}

    summarizeMetadata::BoxFlags boxflags;
    DateTime start,end;
  };
  StageEntry() : key(),data(nullptr) {}

  std::string key;
  std::shared_ptr<Data> data;
};

extern "C" int list_ancillary_FixML_files(const char *name,const struct stat64 *data,int flag,struct FTW *ftw_struct);

extern void copy_fixml(std::string metadata_file,std::string URL,std::string caller,std::string user);
extern void write_fixml(my::map<FeatureEntry>& feature_table,my::map<StageEntry>& stage_table,std::string caller,std::string user);

} // end namespace metadata::FixML

} // end namespace metadata

#endif
