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
#include <satellite.hpp>
#include <datetime.hpp>
#include <tokendoc.hpp>
#include <myerror.hpp>

namespace metautils {

struct Directives {
  Directives() : temp_path(), data_root(), data_root_alias(), host(),
      web_server(), database_server(), rdadb_username(), rdadb_password(),
      metadb_username(), metadb_password(), metadata_manager(), server_root(),
      decs_root(), local_root(), hpss_root(), decs_bindir(), rdadata_home(),
      parameter_map_path(),level_map_path(), singularity_binds() { }

  std::string temp_path, data_root, data_root_alias;
  std::string host, web_server, database_server, rdadb_username, rdadb_password,
      metadb_username, metadb_password;
  std::string metadata_manager;
  std::string server_root, decs_root, local_root, hpss_root;
  std::string decs_bindir, rdadata_home, parameter_map_path, level_map_path;
  std::vector<std::string> singularity_binds;
};

struct Args {
  Args() : args_string(), dsnum(), data_format(), reg_key(), path(), filename(),
      local_name(), member_name(), temp_loc(), update_db(true), update_summary(
      true), override_primary_check(false), overwrite_only(false), regenerate(
      true), update_graphics(true), inventory_only(false) { }

  std::string args_string, dsnum;
  std::string data_format, reg_key;
  std::string path, filename, local_name, member_name, temp_loc;
  bool update_db, update_summary, override_primary_check, overwrite_only,
      regenerate;
  bool update_graphics, inventory_only;
};

struct StringEntry {
  StringEntry() : key() { }

  std::string key;
};

struct InformationEntry {
  InformationEntry() : key(), info(), unique_info_table(), marked() { }

  std::string key;
  std::shared_ptr<std::vector<std::string>> info;
  std::shared_ptr<my::map<StringEntry>> unique_info_table;
  std::shared_ptr<bool> marked;
};

struct GrMLQueryStruct {
  struct Lists {
    Lists() : parameter_codes(nullptr), level_codes(nullptr), gindexes(nullptr),
        file_IDs(nullptr), parameters(nullptr), levels(nullptr), data_formats(
        nullptr), products(nullptr), grid_definitions(nullptr), processes(
        nullptr) { }

    std::list<std::string> *parameter_codes, *level_codes;
    std::list<std::string> *gindexes, *file_IDs;
    std::list<std::string> *parameters, *levels;
    std::list<InformationEntry> *data_formats, *products, *grid_definitions,
        *processes;
  };

  struct Tables {
    Tables() : level_codes(nullptr), parameters(nullptr), levels(nullptr) { }

    my::map<InformationEntry> *level_codes;
    my::map<InformationEntry> *parameters, *levels;
  };

  struct Hashes {
    Hashes() : levels(nullptr), products(nullptr), grid_definitions(nullptr),
        data_formats(nullptr), groups(nullptr), rda_files(nullptr),
        metadata_files(nullptr) { }

    my::map<InformationEntry> *levels, *products, *grid_definitions,
        *data_formats, *groups;
    my::map<InformationEntry> *rda_files, *metadata_files;
  };

  GrMLQueryStruct() : gindex(nullptr), ptfile(nullptr), start_datetime(nullptr),
      end_datetime(nullptr), product_code(nullptr), grid_definition_code(
      nullptr), topt_mo(nullptr), lists(), tables(), hashes() { }
  void initialize() {
    if (lists.parameters == nullptr) {
	lists.parameters = new std::list<std::string>;
    } else {
	lists.parameters->clear();
    }
    if (lists.levels == nullptr) {
	lists.levels = new std::list<std::string>;
    } else {
	lists.levels->clear();
    }
    if (lists.data_formats == nullptr) {
	lists.data_formats = new std::list<InformationEntry>;
    } else {
	lists.data_formats->clear();
    }
    if (lists.products == nullptr) {
	lists.products = new std::list<InformationEntry>;
    } else {
	lists.products->clear();
    }
    if (lists.grid_definitions == nullptr) {
	lists.grid_definitions = new std::list<InformationEntry>;
    } else {
	lists.grid_definitions->clear();
    }
    if (lists.processes == nullptr) {
	lists.processes = new std::list<InformationEntry>;
    } else {
	lists.processes->clear();
    }
    if (tables.level_codes == nullptr) {
	tables.level_codes = new my::map<InformationEntry>;
    } else {
	tables.level_codes->clear();
    }
    if (tables.parameters == nullptr) {
	tables.parameters = new my::map<InformationEntry>;
    } else {
	tables.parameters->clear();
    }
    if (tables.levels == nullptr) {
	tables.levels = new my::map<InformationEntry>;
    } else {
	tables.levels->clear();
    }
    if (hashes.levels == nullptr) {
	hashes.levels = new my::map<InformationEntry>;
    } else {
	hashes.levels->clear();
    }
    if (hashes.groups == nullptr) {
	hashes.groups = new my::map<InformationEntry>;
    } else {
	hashes.groups->clear();
    }
  }
  void reset() {
    start_datetime.reset();
    end_datetime.reset();
    if (lists.parameter_codes != nullptr) {
	delete lists.parameter_codes;
	lists.parameter_codes = nullptr;
    }
    if (lists.level_codes != nullptr) {
	delete lists.level_codes;
	lists.level_codes = nullptr;
    }
  }

  std::shared_ptr<std::string> gindex, ptfile;
  std::shared_ptr<std::string> start_datetime, end_datetime;
  std::shared_ptr<std::string> product_code, grid_definition_code;
  std::shared_ptr<bool> topt_mo;
  Lists lists;
  Tables tables;
  Hashes hashes;
};

struct UtilityIdentification {
  UtilityIdentification() = delete;
  UtilityIdentification(std::string caller, std::string user) : m_caller(caller),
      m_user( user) { }

  std::string m_caller, m_user;
};

class DatasetChecker {
public:
  static const std::string field_nonexist;
  DatasetChecker() = delete;
  DatasetChecker(std::string dsnum);
  operator bool() const { return (myerror.empty()); }
  std::string field_error(std::string field_name) const;
  bool is_good() const { return (field_errors.size() == 0); }
  void print_field_errors(std::ostream& outs) const;

private:
  std::unordered_map<std::string, std::string> field_errors;
};

namespace NcTime {

struct Time {
  Time() : t1(0.), t2(0.), times(nullptr), num_times(0) { }

  double t1, t2, *times;
  size_t num_times;
};

struct TimeRange {
  TimeRange() : first_valid_datetime(), last_valid_datetime() { }

  DateTime first_valid_datetime, last_valid_datetime;
};

struct TimeBounds {
  TimeBounds() : t1(0.), t2(0.), diff(0), changed(false) { }

  double t1, t2, diff;
  bool changed;
};

struct TimeData {
  TimeData() : reference(), units(), calendar() { }

  DateTime reference;
  std::string units, calendar;
};

struct TimeRangeEntry {
  TimeRangeEntry() : key(), unit(-1), num_steps(0), instantaneous(), bounded()
      { }

  size_t key;
  int unit, num_steps;
  TimeRange instantaneous, bounded;
};

DateTime actual_date_time(double time, const TimeData& time_data, std::string&
    error);
DateTime reference_date_time(std::string units_attribute_value);

std::string gridded_netcdf_time_range_description(const TimeRangeEntry& tre,
    const TimeData& time_data, std::string method, std::string& error);
std::string time_method_from_cell_methods(std::string cell_methods, std::string
    timeid);

} // end namespace metautils::NcTime

namespace NcLevel {

struct LevelInfo {
  LevelInfo() : ID(), description(), units(), write() {}

  std::string ID, description, units;
  bool write;
};

std::string write_level_map(const std::vector<LevelInfo>& level_info);

} // end namespace metautils::NcLevel

namespace NcParameter {

std::string write_parameter_map(std::list<std::string>& varlist, std::
    unordered_set<std::string>& var_changes_table, std::string map_type, std::
    string map_name, bool found_map, std::string& warning);

} // end namespace metautils::NcParameter

namespace CF {

extern void fill_nc_time_data(std::string units_attribute_value, NcTime::
    TimeData& time_data, std::string user);

} // end namespace metautils::CF

namespace primaryMetadata {

extern void match_web_file_to_hpss_primary(std::string url, std::string&
    metadata_file);

extern bool expand_file(std::string dirname, std::string filename, std::string
    *file_format);
extern bool open_file_for_metadata_scanning(void *istream, std::string filename,
    std::string& error);
extern bool prepare_file_for_metadata_scanning(TempFile& tfile, TempDir& tdir,
    std::list<std::string> *filelist, std::string& file_format, std::string&
    error);

} // end namespace metautils::primaryMetadata

const std::string tmpdir = "/tmp";
extern Directives directives;
extern Args args;

extern "C" void cmd_unregister();

extern void check_for_existing_cmd(std::string cmd_type);
extern void clean_parameter_code(std::string& parameter_code, std::string&
    parameter_map);
extern void cmd_register(std::string cmd, std::string user);
extern void log_error(std::string error, std::string caller, std::string user,
    bool no_exit = false);
extern void log_error2(std::string error, std::string reporter, std::string
    caller, std::string user, bool no_exit = false);
extern void log_error2(std::string error, std::string reporter, const
    UtilityIdentification& util_ident, bool no_exit = false);
extern void log_info(std::string message, std::string caller, std::string user);
extern void log_warning(std::string warning, std::string caller, std::string
    user);
extern void obs_per(std::string observation_type_value, size_t num_obs, DateTime
    start, DateTime end, double& obsper, std::string& unit);

extern std::string clean_id(std::string id);
extern std::string relative_web_filename(std::string url);
extern std::string web_home();

/*
** CMD_DATABASE is a tuple:
**    database name
**    data type
*/
typedef std::tuple<std::string, std::string> CMD_DATABASE;
extern std::vector<CMD_DATABASE> cmd_databases(std::string caller, std::string
    user);

extern bool read_config(std::string caller, std::string user, bool
    restrict_to_user_rdadata = true);

} // end namespace metautils

extern metautils::Directives meta_directives;
extern metautils::Args meta_args;

#endif
