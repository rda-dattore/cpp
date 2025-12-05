// FILE: grid.h

#ifndef GRID_H
#define   GRID_H

#include <fstream>
#include <math.h>
#include <list>
#include <deque>
#include <unordered_set>
#ifdef __WITH_JASPER
#include <jasper/jasper.h>
#endif
#include <iodstream.hpp>
#include <datetime.hpp>
#include <netcdf.hpp>
#include <mymap.hpp>

class Grid {
public:
  static const double MISSING_VALUE;
  static const double BAD_VALUE;
  static const bool HEADER_ONLY, FULL_GRID;
  enum class Format{ not_set = 0, grib, tdlgrib, octagonal, latlon, latlon25,
      oldlatlon, slp, on84, cgcm1, ussrslp, navy, dslp, ukslp, tropical,
      jraieeemm, grib2, gpcp };
  enum class Type{ not_set = 0, latitudeLongitude, gaussianLatitudeLongitude,
      polarStereographic, mercator, lambertConformal, sphericalHarmonics,
      staggeredLatitudeLongitude, transverseMercator};

  struct GridDimensions {
    GridDimensions() : x(0), y(0), size(0) { }

    short x, y;
    size_t size;
  };

  struct GridDefinition {
    GridDefinition() : type(Type::not_set), slatitude(0.), slongitude(0.),
        elatitude(0.), elongitude(0.), loincrement(0.), laincrement(0.),
        projection_flag(0xff), num_centers(0), stdparallel1(99.),
        stdparallel2(99.), is_cell(false) { }

    Type type;
    float slatitude, slongitude; // starting latitude, longitude
    union {
      float elatitude; // ending latitude
      float llatitude; // latitude at which grid lengths are valid
      size_t trunc1; // first truncation parameter for spherical harmonics
    };
    union {
      float elongitude; // ending longitude
      float olongitude; // longitude of orientation for grid
      size_t trunc2; // second truncation parameter for spherical harmonics
    };
    union {
      float loincrement; // longitude increment (degrees)
      float dx; // grid length (km) in y-direction
      size_t trunc3; // third truncation parameter for spherical harmonics
    };
    union {
      float laincrement; // latitude increment (degrees)
      size_t num_circles; // number of circles between equator and pole
                          // for a gaussian grid
      float dy; // grid length (km) in x-direction
    };
    size_t projection_flag, num_centers;
    float stdparallel1; // tangent latitude
    union {
      float stdparallel2; // 2nd tangent latitude
      float cmeridian; // central meridian
    };
    bool is_cell;
  };

  struct GridStatistics {
    GridStatistics() : min_val(MISSING_VALUE), max_val(-MISSING_VALUE),
        avg_val(0.), min_i(-1), min_j(-1), max_i(-1), max_j(-1) { }

    double min_val, max_val, avg_val;
    int min_i, min_j, max_i, max_j;
  };

  struct EnsembleData {
    EnsembleData() : fcst_type(), id(), total_size(0) { }

    std::string fcst_type, id;
    short total_size;
  };

  struct GLatEntry {
    GLatEntry() : key(), lats(nullptr) { }
    GLatEntry(const GLatEntry& source) : GLatEntry() { *this = source; }
    ~GLatEntry() {
      if (lats != nullptr) {
        delete[] lats;
        lats = nullptr;
      }
    }
    GLatEntry& operator=(const GLatEntry& source) {
      if (this == &source) {
        return *this;
      }
      key = source.key;
      lats = new float[key * 2];
      for (size_t n = 0; n < key * 2; ++n) {
        lats[n] = source.lats[n];
      }
      return *this;
    }

    size_t key;  // number of latitude circles
    float *lats;
  };

  struct LL_SubsetDefinition {
    struct Latitude {
      Latitude() : south(0.), north(0.), stride(0) { }

      float south, north;
      int stride;
    };
    struct Longitude {
      Longitude() : west(0.), east(0.), stride(0) { }

      float west, east;
      int stride;
    };
    struct Dimension {
      Dimension() : min(0), max(0), num(0) { }

      int min, max, num;
    };

    LL_SubsetDefinition() : latitude(), longitude(), x(), y(),
        crosses_greenwich(false) { }

    Latitude latitude;
    Longitude longitude;
    Dimension x, y;
    bool crosses_greenwich;
  };

  Grid() : m_reference_date_time(), m_forecast_date_time(), m_valid_date_time(),
      dim(), def(), stats(), ensdata(), grid(), m_gridpoints(nullptr),
      num_in_sum(nullptr), _path_to_gauslat_lists() { }
  Grid(const Grid& source) : Grid() { *this = source; }
  virtual ~Grid();
  Grid& operator=(const Grid& source) { return *this; }
  virtual void add(float a);
  virtual void compute_mean();
  virtual size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const = 0;
  virtual void divide_by(double div);
  EnsembleData ensemble_data() const { return ensdata; }
  virtual void fill(const unsigned char *stream_buffer, bool fill_header_only)
      = 0;
  DateTime forecast_date_time() const { return m_forecast_date_time; }
  size_t forecast_time() const { return grid.fcst_time; }
  GridDefinition definition() const { return def; }
  GridDimensions dimensions() const { return dim; }
  short first_level_type() const { return grid.level1_type; }
  double first_level_value() const { return grid.level1; }
  double gridpoint(size_t x_location, size_t y_location) const;
  double gridpoint_at(float latitude, float longitude) const;
  double **gridpoints() const { return m_gridpoints; }
  virtual bool is_averaged_grid() const = 0;
  bool is_filled() const { return grid.filled; }
  bool is_layer() const { return !is_level(); }
  bool is_level() const { return (grid.level2_type < 0 || grid.level2_type ==
      0xff); }
  virtual int latitude_index_of(float latitude, my::map<GLatEntry> *gaus_lats)
      const;
  virtual int latitude_index_north_of(float latitude, my::map<GLatEntry>
      *gaus_lats) const;
  virtual int latitude_index_south_of(float latitude, my::map<GLatEntry>
      *gaus_lats) const;
  int longitude_index_of(float longitude) const;
  int longitude_index_east_of(float longitude) const;
  int longitude_index_west_of(float longitude) const;
  virtual void multiply_by(float mult);
  short number_averaged() const { return grid.nmean; }
  size_t number_missing() const { return grid.num_missing; }
  short parameter() const { return grid.param; }
  std::string path_to_gaussian_latitude_data() const { return
       _path_to_gauslat_lists; }
  double pole_value() const { return grid.pole; }
  virtual void print_ascii(std::ostream& outs) const = 0;
  void print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map = "") const { v_print_header(outs, verbose,
      path_to_parameter_map); }
  virtual void print(std::ostream& outs) const =0;
  virtual void print(std::ostream& outs, float start_latitude, float
      end_latitude, float start_longitude, float end_longitude, float
      lat_increment = 0, float lon_increment = 0, bool full_longitude_strip =
      true) const = 0;
  DateTime reference_date_time() const { return m_reference_date_time; }
  short second_level_type() const { return grid.level2_type; }
  double second_level_value() const { return grid.level2; }
  void set_path_to_gaussian_latitude_data(std::string path_to_gauslat_lists) {
       _path_to_gauslat_lists = path_to_gauslat_lists; }
  short source() const { return grid.src; }
  GridStatistics statistics() const { return stats; }
  virtual void subtract(float sub);
  short type() const { return grid.grid_type; }
  DateTime valid_date_time() const { return m_valid_date_time; }

protected:
  virtual void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const = 0;

  DateTime m_reference_date_time, m_forecast_date_time, m_valid_date_time;
  GridDimensions dim;
  GridDefinition def;
  GridStatistics stats;
  EnsembleData ensdata;
  struct GridData {
    GridData() : grid_type(0), nmean(0), param(0), src(0), num_missing(0),
        level1(0.), level2(0.), level1_type(0), level2_type(-1), fcst_time(0),
        pole(0.), num_in_pole_sum(0), filled(false) { }

    short grid_type, nmean, param, src;
    size_t num_missing;
    double level1, level2;
    short level1_type, level2_type;
    size_t fcst_time;
    double pole;
    int num_in_pole_sum;
    bool filled;
  } grid;
  double **m_gridpoints;
  size_t **num_in_sum;
  std::string _path_to_gauslat_lists;
};

// class Grid needs to be defined for xmlutils.hpp
#include <xmlutils.hpp>

class InputGRIBStream : public idstream {
public:
  InputGRIBStream() : curr_offset(0) { }

  // pure virtual functions from idstream
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

  // other idstream overrides
  bool open(std::string filename);

  // local methods
  off_t current_record_offset() const { return curr_offset; }

private:
  int find_grib(unsigned char *buffer);

  off_t curr_offset;
};

class GRIBMessage {
public:
  struct Offsets {
    Offsets() : is(-1), ids(-1), pds(-1), gds(-1), drs(-1), bms(-1), bds(-1),
        ds(-1) { }

    int is, ids, pds, gds, drs, bms, bds, ds;
  };
  struct Lengths {
    Lengths() : is(0), ids(0), pds(0), pds_supp(0), gds(0), drs(0), bms(0),
        bds(0), ds(0) { }

    int is, ids, pds, pds_supp, gds, drs, bms, bds, ds;
  };

  GRIBMessage() : m_edition(-1), discipline(0), mlength(0), curr_off(-1),
      m_offsets(), m_lengths(), pds_supp(nullptr), gds_included(false),
      bms_included(false), grids() { }
  GRIBMessage(const GRIBMessage& source) : GRIBMessage() { *this = source; }
  virtual ~GRIBMessage() { }
  GRIBMessage& operator=(const GRIBMessage& source);
  void append_grid(const Grid *grid);
  void convert_to_grib1();
  virtual off_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length);
  short edition() const { return m_edition; }
  virtual void fill(const unsigned char *stream_buffer, bool fill_header_only);
  Grid *grid(size_t grid_number) const;
  void initialize(short m_edition, unsigned char *pds_supplement, int
      pds_supplement_length, bool gds_is_included, bool bms_is_included);
  off_t length() const { return mlength; }
  Lengths lengths() const { return m_lengths; }
  size_t number_of_grids() const { return grids.size(); }
  Offsets offsets() const { return m_offsets; }
  void pds_supplement(unsigned char *pds_supplement, size_t&
      pds_supplement_length) const;
  virtual void print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map = "") const;

protected:
  void clear_grids();
  void pack_length(unsigned char *output_buffer) const;
  void unpack_is(const unsigned char *stream_buffer);
  void pack_is(unsigned char *output_buffer, off_t& offset, Grid *grid) const;
  virtual void unpack_pds(const unsigned char *stream_buffer);
  virtual void pack_pds(unsigned char *output_buffer, off_t& offset, Grid
      *grid) const;
  virtual void unpack_gds(const unsigned char *stream_buffer);
  virtual void pack_gds(unsigned char *output_buffer, off_t& offset, Grid
      *grid) const;
  virtual void unpack_bms(const unsigned char *stream_buffer);
  void pack_bms(unsigned char *output_buffer, off_t& offset, Grid *grid) const;
  virtual void unpack_bds(const unsigned char *stream_buffer, bool
      fill_header_only);
  void pack_bds(unsigned char *output_buffer, off_t& offset, Grid *grid) const;
  void unpack_end(const unsigned char *stream_buffer);
  void pack_end(unsigned char *output_buffer, off_t& offset) const;

  short m_edition, discipline;
  off_t mlength, curr_off;
  Offsets m_offsets;
  Lengths m_lengths;
  std::unique_ptr<unsigned char[]> pds_supp;
  bool gds_included, bms_included;
  std::vector<std::shared_ptr<Grid>> grids;
};

class GRIB2Grid;
class GRIB2Message : public GRIBMessage {
public:
#ifdef __WITH_JASPER
  struct JasMatrix {
    JasMatrix() : height(0), width(0), data(nullptr) { }
    JasMatrix(const JasMatrix& source) = delete;
    ~JasMatrix() {
      if (data != nullptr) {
        jas_matrix_destroy(data);
      }
    }
    JasMatrix& operator=(const JasMatrix& source) = delete;

    int height, width;
    jas_matrix_t *data;
  };
#endif

#ifdef __WITH_JASPER
  GRIB2Message() : lus_len(0), jas_matrix() { }
#else
  GRIB2Message() : lus_len(0) { }
#endif
  off_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length);
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map = "") const;
  void quick_fill(const unsigned char *stream_buffer);

protected:
  void unpack_ids(const unsigned char *stream_buffer);
  void pack_ids(unsigned char *output_buffer, off_t& offset, Grid *grid) const;
  void unpack_lus(const unsigned char *stream_buffer);
  void pack_lus(unsigned char *output_buffer, off_t& offset, Grid *grid) const;
  void unpack_pds(const unsigned char *stream_buffer);
  void pack_pds(unsigned char *output_buffer, off_t& offset, Grid *grid) const;
  void unpack_gds(const unsigned char *stream_buffer);
  void pack_gds(unsigned char *output_buffer, off_t& offset, Grid *grid) const;
  void unpack_drs(const unsigned char *stream_buffer);
  void pack_drs(unsigned char *output_buffer, off_t& offset, Grid *grid) const;
  void unpack_ds(const unsigned char *stream_buffer, bool fill_header_only);
  void pack_ds(unsigned char *output_buffer, off_t& offset, Grid *grid) const;

  int lus_len;
#ifdef __WITH_JASPER
  JasMatrix jas_matrix;
#endif
};

class TDLGRIBGrid;
class OctagonalGrid;
class LatLonGrid;
class SLPGrid;
class ON84Grid;
class CGCM1Grid;
class USSRSLPGrid;
class NavyGrid;
class GRIBMessage;
class GRIBGrid : public Grid {
friend class GRIBMessage;

public:
  static const size_t LEVEL_TYPE_LENGTH = 202;
  static const char *LEVEL_TYPE_SHORT_NAME[LEVEL_TYPE_LENGTH];
  static const char *LEVEL_TYPE_UNITS[LEVEL_TYPE_LENGTH];
  static const char *TIME_UNITS[256];
  static const short OCTAGONAL_GRID_PARAMETER_MAP[94];
  static const short NAVY_GRID_PARAMETER_MAP[60];
  static const short ON84_GRID_PARAMETER_MAP[406];

  struct GRIBData {
    GRIBData() : reference_date_time(), valid_date_time(), dim(), def(),
        param_version(0), source(0), process(0), grid_catalog_id(0),
        time_unit(0), time_range(0), sub_center(0), D(0), grid_type(0),
        scan_mode(0), rescomp_flag(0), level_types{ 0, 0 }, param_code(0),
        levels{ 0, 0 }, p1(0), p2(0), num_averaged(0), num_averaged_missing(0),
        pack_width(0), gds_included(false), plist(nullptr), gridpoints(nullptr)
        { }

    DateTime reference_date_time, valid_date_time;
    GridDimensions dim;
    GridDefinition def;
    short param_version, source, process, grid_catalog_id, time_unit,
        time_range, sub_center, D, grid_type, scan_mode, rescomp_flag,
        level_types[2];
    size_t param_code, levels[2], p1, p2, num_averaged, num_averaged_missing,
        pack_width;
    bool gds_included;
    int *plist;
    double **gridpoints;
  };

  GRIBGrid() : plist(nullptr), bitmap(), grib(), map() { }
  GRIBGrid(const GRIBGrid& source) : GRIBGrid() { *this = source; }
  virtual ~GRIBGrid();
  GRIBGrid& operator=(const GRIBGrid& source);
  int binary_scale_factor() const { return grib.E; }
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const { return 0; }
  int decimal_scale_factor() const { return grib.D; }
  void divide_by(double div);
  void fill(const unsigned char* stream_buffer, bool fill_header_only = false);
  void fill_from_grib_data(const GRIBData& source);
  virtual std::string first_level_description(xmlutils::LevelMapper&
      level_mapper) const;
  virtual std::string first_level_units(xmlutils::LevelMapper& level_mapper)
      const;
  unsigned char gds17() const { return grib.rescomp; }
  short grib_catalog_id() const { return grib.grid_catalog_id; }
  bool is_accumulated_grid() const;
  bool is_averaged_grid() const;
  float latitude_at(int index, my::map<Grid::GLatEntry> *gaus_lats) const;
  int latitude_index_of(float latitude, my::map<GLatEntry> *gaus_lats) const;
  int latitude_index_north_of(float latitude, my::map<GLatEntry> *gaus_lats)
      const;
  int latitude_index_south_of(float latitude, my::map<GLatEntry> *gaus_lats)
      const;
  short P1() const { return grib.p1; }
  short P2() const { return grib.p2; }
  short packing_width() const { return grib.pack_width; }
  virtual std::string parameter_cf_keyword(xmlutils::ParameterMapper&
      parameter_mapper) const;
  virtual std::string parameter_description(xmlutils::ParameterMapper&
      parameter_mapper) const;
  virtual std::string parameter_short_name(xmlutils::ParameterMapper&
      parameter_mapper) const;
  short parameter_table_code() const { return grib.table; }
  virtual std::string parameter_units(xmlutils::ParameterMapper&
      parameter_mapper) const;
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude,
      float start_longitude, float end_longitude, float lat_increment = 0,
      float lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;
  short process() const { return grib.process; }
  std::string product_description() const { return grib.prod_descr; }
  void reverse_scan();
  short scan_mode() const { return grib.scan_mode; }
  void set_center_to_dss() { grid.src = 60; grib.sub_center = 1; }
  void set_forecast_time(size_t fcst_time) { grid.fcst_time = fcst_time; }
  void set_scale_and_packing_width(short decimal_scale_factor, short
      maximum_pack_width);
  short sub_center_id() const { return grib.sub_center; }
  short time_range() const { return grib.t_range; }
  short time_unit() const { return grib.time_unit; }
  void operator+=(const GRIBGrid& source);
  void operator-=(const GRIBGrid& source);
  void operator*=(const GRIBGrid& source);
  void operator/=(const GRIBGrid& source);
  GRIBGrid& operator=(const TDLGRIBGrid& source);
  GRIBGrid& operator=(const OctagonalGrid& source);
  GRIBGrid& operator=(const LatLonGrid& source);
  GRIBGrid& operator=(const SLPGrid& source);
  GRIBGrid& operator=(const ON84Grid& source);
  GRIBGrid& operator=(const CGCM1Grid& source);
  GRIBGrid& operator=(const USSRSLPGrid& source);
  GRIBGrid& operator=(const GRIB2Grid& source);

  friend GRIBGrid fabs(const GRIBGrid& source);
  friend GRIBGrid interpolate_gaussian_to_lat_lon(const GRIBGrid& source,
      std::string path_to_gauslat_lists);
  friend GRIBGrid pow(const GRIBGrid& source,double exponent);
  friend GRIBGrid sqrt(const GRIBGrid& source);
  friend GRIBGrid combine_hemispheres(const GRIBGrid& nhgrid, const
      GRIBGrid& shgrid);
  friend GRIBGrid create_subset_grid(const GRIBGrid& source, float
      start_latitude, float end_latitude, float start_longitude, float
      end_longitude);

protected:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;
  void galloc();
  void remap_parameters();
  void resize_bitmap(size_t new_size);
  virtual std::string build_level_search_key() const;
  virtual std::string build_parameter_search_key() const;

  int *plist;
  struct Bitmap {
    Bitmap() : map(), capacity(0), applies(false) { }

    unsigned char *map;
    size_t capacity;
    bool applies;
  } bitmap;

  struct GRIB {
    GRIB() : capacity(), last_src(0), table(0), process(0), grid_catalog_id(0),
        scan_mode(0), time_unit(0), p1(0), p2(0), t_range(0), nmean_missing(0),
        century(0), sub_center(0), pack_width(0), prod_descr(), D(0), E(0),
        rescomp(), sp_lat(0.), sp_lon(0.), map_available(true), map_filled(
        false) { }

    struct Capacity {
	Capacity() : y(0), points(0) { }

	size_t y, points;
    } capacity;

    short last_src, table, process, grid_catalog_id, scan_mode, time_unit, p1,
        p2, t_range, nmean_missing, century, sub_center, pack_width;
    std::string prod_descr;
    int D, E;
    unsigned char rescomp;
    float sp_lat, sp_lon;
    bool map_available, map_filled;
  } grib;

  struct Map {
    Map() : param(nullptr), level_type(nullptr), lvl(nullptr), lvl2(nullptr),
        mult(nullptr) { }

    short *param, *level_type, *lvl, *lvl2;
    float *mult;
  } map;

};

class GRIB2Grid : public GRIBGrid {
friend class GRIB2Message;

public:
  struct StatisticalProcessRange {
    short type, time_increment_type;
    struct {
	short unit;
	int value;
    } period_length, period_time_increment;
  };

  GRIB2Grid() : grib2() { }
  GRIB2Grid(const GRIB2Grid& source) : GRIB2Grid() { *this = source; }
  GRIB2Grid& operator=(const GRIB2Grid& source);
  void compute_mean();
  GRIB2Grid create_subset(double south_latitude, double north_latitude, int
      latitude_stride, double west_longitude, double east_longitude, int
      longitude_stride) const;
  short background_process() const { return grib2.backgen_process; }
  short data_representation() const { return grib2.data_rep; }
  size_t data_type() const { return grib2.data_type; }
  size_t discipline() const { return grib2.discipline; }
  DateTime earliest_valid_date_time() const { return m_valid_date_time; }
  short earth_shape() const { return grib2.earth_shape; }
  virtual std::string first_level_description(xmlutils::LevelMapper&
      level_mapper) const;
  virtual std::string first_level_units(xmlutils::LevelMapper& level_mapper)
      const;
  short forecast_process() const { return grib2.fcstgen_process; }
  short local_table_code() const { return grib2.local_table; }
  const std::vector<StatisticalProcessRange>& statistical_process_ranges()
      const { return grib2.stat_process_ranges; }
  size_t number_missing_from_statistical_process() const { return
      grib2.stat_process_nmissing; }
  size_t number_of_statistical_process_ranges() const { return
      grib2.stat_process_ranges.size(); }
  short parameter_category() const { return grib2.param_cat; }
  virtual std::string parameter_cf_keyword(xmlutils::ParameterMapper&
      parameter_mapper) const;
  virtual std::string parameter_description(xmlutils::ParameterMapper&
      parameter_mapper) const;
  virtual std::string parameter_short_name(xmlutils::ParameterMapper&
      parameter_mapper) const;
  virtual std::string parameter_units(xmlutils::ParameterMapper&
      parameter_mapper) const;
  short product_type() const { return grib2.product_type; }
  DateTime statistical_process_end_date_time() const { return
      grib2.stat_period_end; }
  void set_data_representation(unsigned short data_representation_code);
  void set_statistical_process_ranges(const
      std::vector<StatisticalProcessRange>& source);
  void set_table_versions(short master_table_version, short
      local_table_version) { grib.table = master_table_version;
      grib2.local_table = local_table_version; }
  void operator+=(const GRIB2Grid& source);

#ifdef __WITH_JASPER
  friend void decode_jpeg2000(const unsigned char *jpc_bitstream, size_t
      jpc_bitstream_length, GRIB2Message::JasMatrix& jas_matrix, GRIB2Grid *g2,
      double D, double E);
#endif

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;
  void quick_copy(const GRIB2Grid& source);
  virtual std::string build_level_search_key() const;
  virtual std::string build_parameter_search_key() const;

  struct GRIB2 {
    GRIB2() : discipline(0), data_type(0), stat_process_nmissing(0),
        prod_status(0), local_table(0), time_sig(0), product_type(0),
        param_cat(0), backgen_process(0), fcstgen_process(0), data_rep(0),
        num_coord_vals(0), level2_type(0), lp_width(0), lpi(0), earth_shape(0),
        orig_val_type(0), stat_period_end(), modelv_date(),
        stat_process_ranges(), spatial_process(), complex_pack() { }

    size_t discipline, data_type, stat_process_nmissing;
    short prod_status, local_table, time_sig;
    unsigned short product_type, param_cat, backgen_process, fcstgen_process,
        data_rep, num_coord_vals;
    short level2_type, lp_width, lpi, earth_shape, orig_val_type;
    DateTime stat_period_end, modelv_date;
    std::vector<StatisticalProcessRange> stat_process_ranges;

    struct SpatialProcess {
	SpatialProcess() : stat_process(0), type(-1), num_points(0) { }

	short stat_process, type,num_points;
    } spatial_process;

    struct ComplexPack{
	ComplexPack() : grid_point() { }

	struct GridPoint {
	  GridPoint() : split_method(0), miss_val_mgmt(0), width_ref(0),
              width_pack_width(0), length_incr(0), length_pack_width(0),
              primary_miss_sub(0.), secondary_miss_sub(0.), num_groups(0),
              length_ref(0), last_length(0), ref_vals(), widths(), lengths(),
              pvals(), spatial_diff() { }

	  short split_method, miss_val_mgmt, width_ref, width_pack_width,
              length_incr, length_pack_width;
	  double primary_miss_sub, secondary_miss_sub;
	  int num_groups, length_ref, last_length;
	  std::vector<int> ref_vals, widths, lengths, pvals;

	  struct SpatialDiff {
	    SpatialDiff() : order(0), order_vals_width(0), first_vals() { }

	    short order, order_vals_width;
	    std::vector<int> first_vals;
	  } spatial_diff;

	} grid_point;

    } complex_pack;

  } grib2;

};

class InputOctagonalGridStream : public idstream {
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class OctagonalGrid : public Grid {
public:
  OctagonalGrid();
  OctagonalGrid(const OctagonalGrid& source);
  OctagonalGrid& operator=(const OctagonalGrid& source);
  OctagonalGrid& operator=(const LatLonGrid& source);
  OctagonalGrid& operator=(const NavyGrid& source);
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false);
  bool is_averaged_grid() const;
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude,
      float start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;
  void bessel_interpolation(Grid *source);

  unsigned char start[51], end[51];
};

class InputTropicalGridStream : public idstream {
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class TropicalGrid : public Grid {
public:
  TropicalGrid();
  TropicalGrid(const TropicalGrid& source);
  TropicalGrid& operator=(const TropicalGrid& source);
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false);
  bool is_averaged_grid() const;
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;
  void bessel_interpolation(Grid *source);

  unsigned char start[51], end[51];
};

class InputLatLonGridStream : public idstream {
public:
  InputLatLonGridStream() : res(5.) { }
  InputLatLonGridStream(float resolution) : res(resolution) { }
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  float res;
};

class LatLonGrid : public Grid {
public:
  LatLonGrid(float resolution = 5.);
  LatLonGrid(const LatLonGrid& source);
  LatLonGrid& operator=(const LatLonGrid& source);
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false);
  bool is_averaged_grid() const { return (grid.nmean > 0); }
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;
  LatLonGrid& operator=(const GRIBGrid& source);
  LatLonGrid& operator=(const OctagonalGrid& source);
  LatLonGrid& operator=(const ON84Grid& source);
  void operator+=(const LatLonGrid& source);

  friend void convert_ij_wind_to_xy(LatLonGrid& ucomp, LatLonGrid& vcomp);
  friend void convert_xy_wind_to_ij(LatLonGrid& ucomp, LatLonGrid& vcomp);

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;
};

class InputOldLatLonGridStream : public idstream {
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class OldLatLonGrid : public Grid {
public:
  OldLatLonGrid();
  OldLatLonGrid(const OldLatLonGrid& source);
  OldLatLonGrid& operator=(const OldLatLonGrid& source);
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false);
  bool is_averaged_grid() const { return false; }
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  virtual void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;
};

class InputSLPGridStream : public idstream {
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class UKSLPGrid;
class SLPGrid : public Grid {
public:
  SLPGrid();
  SLPGrid(const SLPGrid& source);
  ~SLPGrid() { --dim.y; }
  SLPGrid& operator=(const SLPGrid& source);
  void compute_mean();
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false);
  bool is_averaged_grid() const { return (grid.nmean > 0); }
  void operator+=(const SLPGrid& source);
  SLPGrid& operator=(const GRIBGrid& source);
  SLPGrid& operator=(const GRIB2Grid& source);
  SLPGrid& operator=(const UKSLPGrid& source);
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;
  void print_csv(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;

  unsigned char imap[15][72];
};

class InputDiamondSLPGridStream : public idstream {
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class DiamondSLPGrid : public Grid {
public:
  DiamondSLPGrid();
  DiamondSLPGrid(const DiamondSLPGrid& source);
  DiamondSLPGrid& operator=(const DiamondSLPGrid& source);
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false);
  bool is_averaged_grid() const { return (grid.nmean > 0); }
  void operator+=(const DiamondSLPGrid& source);
  DiamondSLPGrid& operator=(const GRIBGrid& source);
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;
};

class InputON84GridStream : public idstream {
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class ON84Grid : public Grid {
public:
  static const size_t PARAMETER_SIZE = 471;
  static const char *PARAMETER_SHORT_NAME[PARAMETER_SIZE];
  static const char *PARAMETER_LONG_NAME[PARAMETER_SIZE];
  static const char *PARAMETER_DESCRIPTION[PARAMETER_SIZE];
  static const char *PARAMETER_UNITS[PARAMETER_SIZE];

  ON84Grid() : on84() { }
  ON84Grid(const ON84Grid& source) : ON84Grid() { *this = source; }
  ON84Grid& operator=(const ON84Grid& source);
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void create_global_grid(const ON84Grid& NHGrid, const ON84Grid& SHGrid);
  short F1() const { return on84.f1; }
  short F2() const { return on84.f2; }
  void fill(const unsigned char* stream_buffer, bool fill_header_only = false);
  short generating_program() const { return on84.gen_prog; }
  bool is_averaged_grid() const;
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;
  short run_marker() const { return on84.run_mark; }
  short time_marker() const { return on84.time_mark; }

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;

  struct ON84 {
    ON84() : capacity(0), f1(0), f2(0), time_mark(0), run_mark(0), gen_prog(0)
        { }

    size_t capacity;
    short f1, f2, time_mark, run_mark, gen_prog;
  } on84;
};

class InputQuasiON84GridStream : public idstream {
public:
  InputQuasiON84GridStream() : yr(0), mo(0), dy(0), hr(0), fcst_hr(0.),
      rec_num(0) { }
  int ignore() { return bfstream::error; }
  int peek() { return bfstream::error; }
  int read(unsigned char *buffer, size_t buffer_length);

private:
  int yr, mo, dy, hr;
  float fcst_hr;
  short rec_num;
};

class InputCGCM1GridStream : public idstream {
public:
  InputCGCM1GridStream() : is_binary(false) { }
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  bool is_binary;
};

class CGCM1Grid : public Grid {
public:
  CGCM1Grid();
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false);
  const char *parameter_name() const { return cgcm1.var_name; }
  bool is_averaged_grid() const { return (grid.nmean > 0); }
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;

  struct CGCM1 {
    CGCM1() : capacity(0), tstep(0), pack_dens(0), var_name() { }

    size_t capacity, tstep;
    short pack_dens;
    char var_name[5];
  } cgcm1;
};

class InputTDLGRIBGridStream : public idstream {
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class TDLGRIBMessage : public GRIBMessage {
public:
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print_header(std::ostream& outs, bool verbose) const;

protected:
  void unpack_is(const unsigned char *stream_buffer);
  void unpack_pds(const unsigned char *stream_buffer);
  void unpack_gds(const unsigned char *stream_buffer);
  void unpack_bms(const unsigned char *stream_buffer);
  void unpack_bds(const unsigned char *stream_buffer, bool fill_header_only);
};

class TDLGRIBGrid : public GRIBGrid {
friend class TDLGRIBMessage;

public:
  TDLGRIBGrid() : tdl() { }
  TDLGRIBGrid(const TDLGRIBGrid& source) : TDLGRIBGrid() { *this = source; }
  TDLGRIBGrid& operator=(const TDLGRIBGrid& source);
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false)
      { }
  short model() const { return grib.process; }
  unsigned char *pds() const { return tdl.pds39_a; }
  size_t parameter() const { return tdl.param; }
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;

  struct TDLData {
    TDLData() : param(0), B(0), V(0), T(0), RR(0), O(0), HH(0), seq(0), E(0),
        first_val(0), first_diff(0), pds39_a(pds39) { }

    size_t param;
    short B, V, T, RR, O, HH, seq;
    int E, first_val, first_diff;
    unsigned char pds39[39], *pds39_a;
  } tdl;

};

class InputUSSRSLPGridStream : public idstream {
public:
  InputUSSRSLPGridStream() : ptr_to_lrec(2316) { }
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  unsigned char block[2316];
  short ptr_to_lrec;
};

class USSRSLPGrid : public Grid {
public:
  USSRSLPGrid();
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false);
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  bool is_averaged_grid() const { return false; }
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;
};

class InputNavyGridStream : public idstream {
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class NavyGrid : public Grid {
public:
  NavyGrid() : status(0) { }
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false);
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  bool is_averaged_grid() const { return (m_reference_date_time.day() == 0); }
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;

  short status;
};

class InputCACGridStream : public idstream {
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class InputUKSLPGridStream : public idstream {
public:
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class UKSLPGrid : public Grid {
public:
  UKSLPGrid();
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false);
  bool is_averaged_grid() const { return false; }
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;
};

class InputNOAAOI2SSTGridStream : public idstream {
public:
  int ignore() { return bfstream::error; }
  int peek() { return bfstream::error; }
  int read(unsigned char *buffer, size_t buffer_length);
};

class NOAAOI2SSTGrid : public Grid {
public:
  static const unsigned char MASK[180][360];

  NOAAOI2SSTGrid() : m_version(0) { }
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only = false);
  bool is_averaged_grid() const { return true; }
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;
  size_t version() const { return m_version; }

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;

  size_t m_version;
};

class InputJRAIEEEMMGridStream : public idstream {
public:
  InputJRAIEEEMMGridStream() : file_type(), recln(0), yr(0), mo(0), def_type(
      Grid::Type::not_set), dimx(0), dimy(0), xres(0.), yres(0.) { }
  int ignore() { return bfstream::error; }
  bool open(std::string filename);
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  static const short ANL_CHIPSI_LEVEL_TYPES[120], ANL_ISENTROP_LEVEL_TYPES[175],
      ANL_LAND_LEVEL_TYPES[7], ANL_MDL_LEVEL_TYPES[290], ANL_P_LEVEL_TYPES[138],
      ANL_SNOW_LEVEL_TYPES[1], ANL_Z_LEVEL_TYPES[145],
      FCST_MDL_LEVEL_TYPES[328], FCST_PHY2M_LEVEL_TYPES[60],
      FCST_PHY3M_LEVEL_TYPES[1202], FCST_PHYLAND_LEVEL_TYPES[12],
      GES_MDL_LEVEL_TYPES[328], GES_P_LEVEL_TYPES[153];
  static const float ANL_CHIPSI_LEVEL_VALUES[120],
      ANL_ISENTROP_LEVEL_VALUES[175], ANL_LAND_LEVEL_VALUES[7],
      ANL_MDL_LEVEL_VALUES[290], ANL_P_LEVEL_VALUES[138],
      ANL_SNOW_LEVEL_VALUES[1], ANL_Z_LEVEL_VALUES[145],
      FCST_MDL_LEVEL_VALUES[328], FCST_PHY2M_LEVEL_VALUES[60],
      FCST_PHY3M_LEVEL_VALUES[1202], FCST_PHYLAND_LEVEL_VALUES[12],
      GES_MDL_LEVEL_VALUES[328], GES_P_LEVEL_VALUES[153];
  static const short ANL_CHIPSI_PARAMETERS[120], ANL_ISENTROP_PARAMETERS[175],
      ANL_LAND_PARAMETERS[7], ANL_MDL_PARAMETERS[290], ANL_P_PARAMETERS[138],
      ANL_SNOW_PARAMETERS[1], ANL_Z_PARAMETERS[145], FCST_MDL_PARAMETERS[328],
      FCST_PHY2M_PARAMETERS[60], FCST_PHY3M_PARAMETERS[1202],
      FCST_PHYLAND_PARAMETERS[12], GES_MDL_PARAMETERS[328],
      GES_P_PARAMETERS[153];
  static const char *FCST_MDL_TIME_RANGES[328], *FCST_PHY2M_TIME_RANGES[60],
      *FCST_PHY3M_TIME_RANGES[1202], *FCST_PHYLAND_TIME_RANGES[12];
  std::string file_type;
  size_t recln;
  int yr, mo;
  Grid::Type def_type;
  int dimx, dimy;
  float xres, yres;
};

class JRAIEEEMMGrid : public Grid {
public:
  JRAIEEEMMGrid() : m_time_range() { }
  JRAIEEEMMGrid(const JRAIEEEMMGrid &source) : JRAIEEEMMGrid() { *this =
      source; }
  JRAIEEEMMGrid& operator=(const JRAIEEEMMGrid& source);
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  std::string time_range() const { return m_time_range; }
  bool is_averaged_grid() const { return true; }
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude,
      float start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;

  std::string m_time_range;
};

class InputCMORPH025GridStream : public idstream {
public:
  InputCMORPH025GridStream() : type(0), m_date_time(), parameter(),
      latitude() { } 
  int ignore() { return bfstream::error; }
  bool open(std::string filename);
  DateTime date_time() { return m_date_time; }
  float end_latitude() { return latitude.end; }
  std::string parameter_code() { return parameter.code; }
  std::string parameter_description() { return parameter.description; }
  std::string parameter_units() { return parameter.units; }
  int peek();
  int read(unsigned char *buffer ,size_t buffer_length);
  float start_latitude() { return latitude.start; }

private:
  short type;
  DateTime m_date_time;

  struct ParameterData {
    ParameterData() : code(), description(), units() { }

    std::string code, description, units;
  } parameter;

  struct LatitudeData {
    LatitudeData() : start(0.), end(0.) { }

    float start, end;
  } latitude;

};

class CMORPH025Grid : public Grid {
public:
  enum { CMORPHCombinedType=1, CMORPHAccumulationType };

  CMORPH025Grid();
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  bool is_averaged_grid() const;
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;
  void set_date_time(const DateTime& date_time) { m_reference_date_time =
      m_valid_date_time = date_time; }
  void set_latitudes(float start_latitude, float end_latitude);

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;
};

class InputCMORPH8kmGridStream : public idstream {
public:
  InputCMORPH8kmGridStream() : m_date_time() { }
  int ignore() { return bfstream::error; }
  bool open(std::string filename);
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  static const size_t RECORD_SIZE;
  DateTime m_date_time;
};

class CMORPH8kmGrid : public Grid {
public:
  CMORPH8kmGrid();
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  bool is_averaged_grid() const;
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;
};

class InputGPCPGridStream : public idstream {
public:
  InputGPCPGridStream() : header(nullptr), hdr_size(0), rec_size(0),
      eof_offset(0) { }
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  void read_header();

  std::unique_ptr<unsigned char[]> header;
  size_t hdr_size, rec_size, eof_offset;
};

class GPCPGrid : public Grid {
public:
  GPCPGrid() : x_capacity(0), y_capacity(0), param_name(), param_units(),
      is_empty(true) { }
  size_t copy_to_buffer(unsigned char *output_buffer, const size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  bool is_averaged_grid() const { return true; }
  bool is_empty_grid() const { return is_empty; }
  std::string parameter_name() const { return param_name; }
  std::string parameter_units() const { return param_units; }
  void print(std::ostream& outs) const;
  void print(std::ostream& outs, float start_latitude, float end_latitude, float
      start_longitude, float end_longitude, float lat_increment = 0, float
      lon_increment = 0, bool full_longitude_strip = true) const;
  void print_ascii(std::ostream& outs) const;

private:
  void v_print_header(std::ostream& outs, bool verbose, std::string
      path_to_parameter_map) const;

  int x_capacity, y_capacity;
  std::string param_name, param_units;
  bool is_empty;
};

namespace grib_utils {

struct StringEntry {
  StringEntry() : key() { }

  std::string key;
};

extern std::tuple<std::string, DateTime, DateTime, size_t> grib_product(
    GRIBGrid *grid);
extern std::tuple<std::string, DateTime, DateTime> grib2_product(GRIB2Grid
    *grid);

extern short p2_from_statistical_end_time(const GRIB2Grid& grid);

} // end namespace grib_utils

namespace grid_to_netcdf {

class GridData {
public:
  GridData() : ref_date_time(), cell_methods(), subset_definition(),
      parameter_mapper(), record_flag(0), num_parameters_in_file(1),
      num_parameters_written(0), path_to_gauslat_lists(), wrote_header(false)
      { }

  DateTime ref_date_time;
  std::string cell_methods;
  Grid::LL_SubsetDefinition subset_definition;
  std::shared_ptr<xmlutils::ParameterMapper> parameter_mapper;
  int record_flag;
  size_t num_parameters_in_file, num_parameters_written;
  std::string path_to_gauslat_lists;
  bool wrote_header;
};

struct HouseKeeping {
  HouseKeeping() : unique_variable_table(), include_parameter_set(nullptr) { }

  my::map<NetCDF::UniqueVariableEntry> unique_variable_table;
  std::shared_ptr<std::unordered_set<std::string>> include_parameter_set;
};

extern int convert_grid_to_netcdf(Grid *source_grid, Grid::Format format,
    OutputNetCDFStream *ostream, GridData& grid_data, std::string
    path_to_level_map);

extern void add_ncar_grid_variable_to_netcdf(OutputNetCDFStream *ostream, const
    Grid *source_grid, size_t num_dims, size_t *dim_ids);
extern void convert_grib_file_to_netcdf(std::string filename,
    OutputNetCDFStream& ostream, DateTime& ref_date_time, std::string&
    cell_methods, Grid::LL_SubsetDefinition& subset_definition,
    xmlutils::ParameterMapper& parameter_mapper, std::string path_to_level_map,
    my::map<NetCDF::UniqueVariableEntry>& unique_variable_table, int
    record_flag);
extern void write_netcdf_header_from_grib_file(InputGRIBStream& istream,
    OutputNetCDFStream& ostream, GridData& grid_data, HouseKeeping& hk,
    std::string path_to_level_map);

} // end namespace grid_to_netcdf

namespace grid_conversions {

extern int convert_grid_to_grib1(const Grid *source_grid, Grid::Format format,
    odstream *ostream, size_t grid_number);
extern int convert_grid_to_ll(const Grid *source_grid, Grid::Format format,
    odstream *ostream, size_t grid_number, float resolution);
extern int convert_grid_to_oct(const Grid *source_grid, Grid::Format format,
    odstream *ostream, size_t grid_number);
extern int convert_grid_to_slp(const Grid *source_grid, Grid::Format format,
    odstream *ostream, size_t grid_number);

extern void convert_grid_parameter_string(Grid::Format format, const char
    *string, std::vector<short>& params);

} // end namespace grid_conversions

namespace grid_print {

extern int print(std::string input_filename, Grid::Format format, bool
    headers_only, bool verbose, size_t start, size_t stop, std::string
    path_to_parameter_map = "");
extern int print_ascii(std::string input_filename, Grid::Format format, size_t
    start, size_t stop);

} // end namespace grid_print

#ifdef __WITH_JASPER
extern int encode_jpeg2000(unsigned char *cin, int *pwidth, int *pheight, int
    *pnbits, int *ltype, int *ratio, int *retry, char *outjpc, int *jpclen);
#endif
extern bool operator==(const Grid::GridDefinition& def1, const
    Grid::GridDefinition& def2);
extern bool operator!=(const Grid::GridDefinition& def1, const
    Grid::GridDefinition& def2);
extern bool operator==(const Grid::GridDimensions& dim1, const
    Grid::GridDimensions& dim2);
extern bool operator!=(const Grid::GridDimensions& dim1, const
    Grid::GridDimensions& dim2);

#endif
