#ifndef META_H
#define   META_H

#include <iostream>
#include <list>
#include <vector>
#include <mymap.hpp>
#include <grid.hpp>
#include <xml.hpp>
#include <MySQL.hpp>
#include <tempfile.hpp>
#include <observation.hpp>
#include <satellite.hpp>
#include <tokenDoc.hpp>

namespace metautils {

struct Directives {
  Directives() : tempPath(),data_root(),data_root_alias(),host(),web_server(),database_server(),fallback_database_server(),metadata_manager(),serverRoot(),dssRoot(),localRoot(),dss_bindir() {}

  std::string tempPath,data_root,data_root_alias;
  std::string host,web_server,database_server,fallback_database_server;
  std::string metadata_manager;
  std::string serverRoot,dssRoot,localRoot;
  std::string dss_bindir;
};
struct Args {
  Args() : argsString(),dsnum(),format(),reg_key(),path(),filename(),local_name(),member_name(),temp_loc(),updateDB(false),updateSummary(false),overridePrimaryCheck(false),overwriteOnly(false),regenerate(false),updateGraphics(false),inventoryOnly(false) {}

  std::string argsString,dsnum;
  std::string format,reg_key;
  std::string path,filename,local_name,member_name,temp_loc;
  bool updateDB,updateSummary,overridePrimaryCheck,overwriteOnly,regenerate;
  bool updateGraphics,inventoryOnly;
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
/*
struct ThreadStruct {
  ThreadStruct() : query(),db(),p(nullptr),tid(0),status(0),start(),end(),error() {}

  MySQLQuery query;
  std::string db;
  void **p;
  pthread_t tid;
  int status;
  time_t start,end;
  std::string error;
};
*/
struct GrMLQueryStruct {
  struct Lists {
    Lists() : parameter_codes(nullptr),level_codes(nullptr),gindexes(nullptr),file_IDs(nullptr),parameters(nullptr),levels(nullptr),formats(nullptr),products(nullptr),grid_definitions(nullptr),processes(nullptr) {}

    std::list<std::string> *parameter_codes,*level_codes;
    std::list<std::string> *gindexes,*file_IDs;
    std::list<std::string> *parameters,*levels;
    std::list<InformationEntry> *formats,*products,*grid_definitions,*processes;
  };
  struct Tables {
    Tables() : level_codes(nullptr),parameters(nullptr),levels(nullptr) {}

    my::map<InformationEntry> *level_codes;
    my::map<InformationEntry> *parameters,*levels;
  };
  struct Hashes {
    Hashes() : levels(nullptr),products(nullptr),grid_definitions(nullptr),formats(nullptr),groups(nullptr),rda_files(nullptr),metadata_files(nullptr) {}

    my::map<InformationEntry> *levels,*products,*grid_definitions,*formats,*groups;
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
    if (lists.formats == nullptr) {
	lists.formats=new std::list<InformationEntry>;
    }
    else {
	lists.formats->clear();
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
  TimeRangeEntry() : key(),unit(nullptr),num_steps(nullptr),instantaneous(nullptr),bounded(nullptr) {}

  size_t key;
  int *unit,*num_steps;
  TimeRange *instantaneous,*bounded;
};

DateTime getActualDateTime(double time,TimeData& time_data,std::string& error);

std::string getGriddedNetCDFTimeRangeDescription(const TimeRangeEntry& tre,const TimeData& time_data,std::string method,std::string& error);
std::string getTimeMethodFromCellMethods(std::string cell_methods,std::string timeid);

} // end namespace NcTime

namespace NcLevel {

struct LevelInfo {
  LevelInfo() : ID(),description(),units(),write() {}

  std::vector<std::string> ID,description,units;
// have to use this instead of vector<bool>, which has problems
  std::vector<unsigned char> write;
};

std::string writeLevelMap(std::string dsnum,const LevelInfo& level_info);

} // end namespace NcLevel

namespace NcParameter {

std::string writeParameterMap(std::string dsnum,std::list<std::string>& varlist,my::map<metautils::StringEntry>& var_changes_table,std::string map_type,std::string map_name,bool found_map,std::string& warning);

} // end namespace NcParameter

namespace CF {

extern void fill_NcTimeData(std::string units_attribute_value,NcTime::TimeData& time_data,std::string user);

} // end namespace CF

const std::string tmpdir="/tmp";

extern "C" void cmd_unregister();

extern void addToGindexList(std::string pindex,std::list<std::string>& gindexList,int& min_gindex,int& max_gindex,std::string list_type);
extern void checkForExistingCMD(std::string cmd_type);
extern void cleanParameterCode(std::string& parameter_code,std::string& parameter_map);
extern void cmd_register(std::string cmd,std::string user);
extern void getFromAncillaryTable(MySQL::Server& srv,std::string tableName,std::string whereConditions,MySQL::LocalQuery& query,const std::string& caller,const std::string& user,const std::string& args_string);
extern void getObsPer(std::string observationTypeValue,size_t numObs,DateTime start,DateTime end,double& obsper,std::string& unit);
extern void logError(std::string error,std::string caller,std::string user,std::string argsString,bool no_exit = false);
extern void logInfo(std::string message,std::string caller,std::string user,std::string argsString);
extern void logWarning(std::string warning,std::string caller,std::string user,std::string argsString);
extern void readConfig(std::string caller,std::string user,std::string argsString,bool restrict_to_user_rdadata = true);

extern std::string cleanID(std::string ID);
extern std::string getRelativeWebFilename(std::string URL);
extern std::string getWebHome();

extern std::list<std::string> getCMDDatabases(std::string caller,std::string user,std::string args);

extern bool connectToBackupMetadataServer(MySQL::Server& srv_m);
extern bool connectToBackupRDAServer(MySQL::Server& srv_d);
extern bool connectToMetadataServer(MySQL::Server& srv_m);
extern bool connectToRDAServer(MySQL::Server& srv_d);
extern bool existsOnCastle(std::string filename,struct stat& buf);

extern int queryGridDatabase(std::string database,std::string doc_root,std::string preset,bool set_single_fields,GrMLQueryStruct& grml,std::string is_internal_view,std::string& error,void (*makeLogEntry)(std::string message));

} // end namespace metautils

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
  FileEntry() : key(),backup_name(),format(),units(),metafileID(),mod_time(),start(),end(),file_format(),data_size(),groupID(),annotation(),inv() {}

  std::string key;
  std::string backup_name,format,units,metafileID,mod_time;
  std::string start,end;
  std::string file_format,data_size;
  std::string groupID,annotation,inv;
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

extern std::string LLToDateString(std::string ll_date);
extern std::string dateToLLString(std::string date);
extern std::string setDateTimeString(std::string datetime,std::string flag,std::string time_zone,std::string time_prefix = " ");
extern std::string summarizeLocations(std::string database,std::string dsnum);

extern void createFileListCache(std::string file_type,std::string caller,std::string user,std::string argsString,std::string gindex = "");
extern void createNonCMDFileListCache(std::string file_type,my::map<Entry>& filesWithCMD_table,std::string caller,std::string user,std::string argsString);
extern void getCMDDates(std::string database,std::string dsnum,std::list<CMDDateRange>& range_list,size_t& precision);
extern void getGridsPer(size_t nsteps,DateTime start,DateTime end,double& gridsper,std::string& unit);
extern void insertTimeResolutionKeyword(MySQL::Server& server,std::string database,std::string keyword,std::string dsnum);
extern void summarizeDates(std::string dsnum,std::string caller,std::string user,std::string argsString);
extern void summarizeFrequencies(std::string dsnum,std::string caller,std::string user,std::string argsString,std::string mssID_code = "");
extern void summarizeFrequencies2(std::string dsnum,std::string caller,std::string user,std::string argsString,std::string mssID_code = "");
extern void summarizeFormats(std::string dsnum,std::string caller,std::string user,std::string argsString);

extern int compareCMDDateRangesAsc(const CMDDateRange& left,const CMDDateRange& right);
extern int compareCMDDateRangesDesc(const CMDDateRange& left,const CMDDateRange& right);
extern int compareFileEntries(const FileEntry& left,const FileEntry& right);
extern int compareParameterEntries(const ParameterEntry& left,const ParameterEntry& right);

extern CMDDateRange getCMDDateRange(std::string start,std::string end,std::string gindex,size_t& precision);

namespace gridLevels {

struct LevelEntry {
  LevelEntry() : key() {}

  std::string key;
};

void summarizeGridLevels(std::string dsnum,std::string database,std::string caller,std::string user,std::string args);

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
  GridDefinitionEntry() : key(),definition(),defParams() {}

  std::string key;
  std::string definition,defParams;
};

extern void aggregateGrids(std::string dsnum,std::string database,std::string caller,std::string user,std::string args,std::string fileID_code = "");
extern void summarizeGrids(std::string dsnum,std::string database,std::string caller,std::string user,std::string args,std::string fileID_code = "");
extern void summarizeGridResolutions(std::string dsnum,std::string caller,std::string user,std::string args,std::string mssID_code = "");

extern int compareValues(const size_t& left,const size_t& right);

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

extern void checkPoint(double latp,double lonp,MySQL::Server& server,my::map<Entry>& location_table);
extern void compressLocations(std::list<std::string>& location_list,my::map<ParentLocation>& parent_location_table,std::vector<std::string>& sortedArray,std::string caller,std::string user,std::string args);

extern int compareLocations(const std::string& left,const std::string& right);

extern bool summarizeObsData(std::string dsnum,std::string caller,std::string user,std::string args);

} // end namespace summarizeMetadata::obsData

namespace fixData {

void summarizeFixData(std::string dsnum,std::string caller,std::string user,std::string args);

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
  ParameterDataEntry() : key(),shortName(),description(),units(),comment() {}

  std::string key;
  std::string shortName,description,units,comment;
};
struct ParameterTableEntry {
  ParameterTableEntry() : key(),parameter_data_table() {}

  std::string key;
  std::shared_ptr<my::map<ParameterDataEntry>> parameter_data_table;
};

extern void addToFormats(XMLDocument& xdoc,MySQL::Query& query,std::list<std::string>& format_list,std::string& formats);
extern void generateDetailedFixSummary(std::string dsnum,MySQL::LocalQuery& format_query,std::string file_type,std::string groupIndex,std::ofstream& ofs,std::list<std::string>& format_list,std::string caller,std::string user,std::string args);
extern void generateDetailedGridSummary(std::string dsnum,MySQL::LocalQuery& format_query,std::string file_type,std::string groupIndex,std::ofstream& ofs,std::list<std::string>& format_list,std::string caller,std::string user,std::string args);
extern void generateDetailedMetadataView(std::string dsnum,std::string caller,std::string user,std::string args);
extern void generateDetailedObservationSummary(std::string dsnum,MySQL::LocalQuery& format_query,std::string file_type,std::string groupIndex,std::ofstream& ofs,std::list<std::string>& format_list,std::string caller,std::string user,std::string args);
extern void generateGroupDetailedMetadataView(std::string dsnum,std::string groupIndex,std::string file_type,std::string caller,std::string user,std::string args);
extern void generateParameterCrossReference(std::string dsnum,std::string format,std::string title,std::string htmlFile,std::string caller,std::string user,std::string args);
extern void generateVtable(std::string dsnum,std::string& error,bool test_only = false);

extern int compareParameterDataEntries(const ParameterDataEntry& left,const ParameterDataEntry& right);

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

void closeInventory(std::ofstream& ofs,TempFile *tfile,std::string cmd_type,bool insert_into_db,bool create_cache,std::string caller,std::string user);
void openInventory(std::ofstream& ofs,TempFile **tfile,std::string caller,std::string user);

namespace GrML {

struct ParameterEntry {
  ParameterEntry() : key(),startDateTime(),endDateTime(),numTimeSteps(0) {}

  std::string key;
  DateTime startDateTime,endDateTime;
  size_t numTimeSteps;
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

extern void copyGrML(std::string metadata_file,std::string URL,std::string caller,std::string user);
extern void writeGrML(my::map<GridEntry>& grid_table,std::string caller,std::string user);

extern int compareGridTableKeys(const std::string& left,const std::string& right);

} // end namespace metadata::GrML

namespace ObML {

const size_t NUM_OBS_TYPES=5;
struct ObservationTypes {
  ObservationTypes() : types() {
    types.emplace_back("upper_air");
    types.emplace_back("surface");
    types.emplace_back("underwater");
    types.emplace_back("surface_10-year_climatology");
    types.emplace_back("surface_30-year_climatology");
  }

  std::vector<std::string> types;
};
struct DataTypeEntry {
  struct Data {
    Data() : map(),nsteps(0),min_altitude(1.e38),max_altitude(-1.e38),avg_vres(0.),avg_nlev(0),vres_cnt(0),vunits() {}

    std::string map;
    size_t nsteps;
    float min_altitude,max_altitude,avg_vres;
    size_t avg_nlev,vres_cnt;
    std::string vunits;
  };

  DataTypeEntry() : key(),data(nullptr) {}

  std::string key;
  std::shared_ptr<Data> data;
};
struct IDEntry {
  struct Data {
    Data() : S_lat(),N_lat(),W_lon(),E_lon(),min_lon_bitmap(nullptr),max_lon_bitmap(nullptr),start(),end(),nsteps(),data_types_table(),uniqueTable() {}

    float S_lat,N_lat,W_lon,E_lon;
//    float min_lon_bitmap[360],max_lon_bitmap[360];
std::unique_ptr<float []> min_lon_bitmap,max_lon_bitmap;
    DateTime start,end;
    size_t nsteps;
    my::map<DataTypeEntry> data_types_table,uniqueTable;
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

extern "C" int listAncillaryObMLFiles(const char *name,const struct stat64 *data,int flag,struct FTW *ftw_struct);

extern void copyObML(std::string metadata_file,std::string URL,std::string caller,std::string user);
extern void writeObML(my::map<IDEntry> **ID_table,my::map<PlatformEntry> *platform_table,std::string caller,std::string user);

extern std::string stringCoordinateToDB(std::string coordinate_value);

extern int compareStrings(const std::string& left,const std::string& right);

} // end namespace metadata::ObML

namespace SatML {

struct ScanLine {
  ScanLine() : dateTime(),firstCoord(),lastCoord(),subpoint(),res(0.),width(0.),subpoint_resolution(0.) {}

  DateTime dateTime;
  SatelliteS::EarthCoordinate firstCoord,lastCoord,subpoint;
  double res;
  float width,subpoint_resolution;
};
struct ScanLineEntry {
  ScanLineEntry() : key(),satID(),startDateTime(),endDateTime(),scanLineList() {}

  std::string key;
  std::string satID;
  DateTime startDateTime,endDateTime;
  std::list<ScanLine> scanLineList;
};
struct Image {
  Image() : dateTime(),corners(),center(),xres(0.),yres(0.) {}

  DateTime dateTime;
  SatelliteImage::ImageCorners corners;
  SatelliteImage::EarthCoordinate center;
  float xres,yres;
};
struct ImageEntry {
  ImageEntry() : key(),startDateTime(),endDateTime(),imageList() {}

  std::string key;
  DateTime startDateTime,endDateTime;
  std::list<Image> imageList;
};

extern void writeSatML(my::map<ScanLineEntry>& scan_line_table,std::list<std::string>& scan_line_table_keys,my::map<ImageEntry>& image_table,std::list<std::string>& image_table_keys,std::string caller,std::string user);

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
    Data() : altID(),classificationList() {}

    std::string altID;
    std::list<ClassificationEntry> classificationList;
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

extern "C" int listAncillaryFixMLFiles(const char *name,const struct stat64 *data,int flag,struct FTW *ftw_struct);

extern void copyFixML(std::string metadata_file,std::string URL,std::string caller,std::string user);
extern void writeFixML(my::map<FeatureEntry>& feature_table,my::map<StageEntry>& stage_table,std::string caller,std::string user);

} // end namespace metadata::FixML

extern void writeFinalize(bool isMSSFile,std::string filename,std::string ext,std::string tfile_name,std::ofstream& ofs,std::string caller,std::string user);
extern void writeInitialize(bool& isMSSFile,std::string& filename,std::string ext,std::string tfile_name,std::ofstream& ofs,std::string caller,std::string user);

extern std::string getDetailedDataType(xmlutils::DataTypeMapper& data_type_mapper,const std::string& format,const std::string& code,std::string caller,std::string user,std::string args);
extern std::string getDetailedLevel(xmlutils::LevelMapper& level_mapper,const std::string& format,const std::string& map,const std::string& type,const std::string& value,bool htmlify_units,std::string caller = "",std::string user = "",std::string args = "");
extern std::string getDetailedParameter(xmlutils::ParameterMapper& parameter_mapper,const std::string& format,const std::string& code,std::string caller = "",std::string user = "",std::string args = "");

extern int compareGridDefinitions(const std::string& left,const std::string& right);
extern int compareLevels(const std::string& left,const std::string& right);
extern int compareTimeRanges(const std::string& left,const std::string& right);
extern int defaultCompare(const std::string& left,const std::string& right);

} // end namespace metadata

namespace primaryMetadata {

extern void matchWebFileToMSSPrimary(std::string URL,std::string& metadata_file);

extern bool checkFile(std::string dirname,std::string filename,std::string *file_format);
extern bool openFileForMetadataScanning(void *istream,std::string filename,std::string& error);
extern bool prepareFileForMetadataScanning(TempFile& tfile,TempDir& tdir,std::list<std::string> *filelist,std::string& file_format,std::string& error);

} // end namespace primaryMetadata

namespace metadataExport {

struct Entry {
  Entry() : key(),min_lon(0.),min_lat(0.) {}

  std::string key;
  double min_lon,min_lat;
};
struct PThreadStruct {
  PThreadStruct() : query(),tid(0) {}

  MySQL::LocalQuery query;
  pthread_t tid;
};

extern "C" void *runQuery(void *ts);

extern void addFrequency(my::map<Entry>& frequency_table,size_t frequency,std::string unit);
extern void addToResolutionTable(double lon_res,double lat_res,std::string units,my::map<Entry>& resolution_table);
extern void exportMetadata(std::string format,std::unique_ptr<TokenDocument>& token_doc,std::ostream& ofs,std::string dsnum,size_t initialIndentLength = 0);
extern void exportToDIF(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length);
extern void exportToNative(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length);
extern void exportToOAI_DC(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length);

extern int compareReferences(XMLElement& left,XMLElement& right);

extern std::string getKey(double res,std::string units);

} // end namespace metadataExport

namespace html {

struct Node {
  Node() : copy(),value(),nodes() {}

  std::string copy,value;
  std::list<Node> nodes;
};

extern void wrap(std::string& string,std::string fill,size_t chars);
extern void getNodes(std::string string,std::list<Node>& nodes);
extern void processNode(Node& node,size_t wrapLength);
extern std::string htmlTextToASCII(std::string& elementCopy,size_t wrapLength);

} // end namespace html

extern metautils::Directives directives;
extern metautils::Args args;

extern void decodeGridDefinitionParameters(std::string definition,std::string defParams,Grid::GridDimensions& grid_dim,Grid::GridDefinition& grid_def);
extern void exportToDIF(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,std::string indent);

extern std::string convertGridDefinition(std::string d,XMLElement e);
extern std::string convertMetaFormatToLink(std::string format);
extern std::string convertMetaGridDefinition(std::string gridDefinition,std::string definitionParameters,const char *separator);
extern std::string convertMetaParameterMap(std::string format,std::string map,bool verboseOutput);
extern std::string htmlTextToASCII(std::string& elementCopy,size_t wrapLength);
//extern std::string translateGridDefinition(std::string definition,bool getAbbreviation = false);
extern std::string translatePlatform(std::string definition);

extern bool getSpatialDomainFromGridDefinition(std::string definition,std::string center,double& west_lon,double& south_lat,double& east_lon,double& north_lat);

#endif
