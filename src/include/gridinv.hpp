// FILE: gridinv.h

#ifndef GRIDINV_H
#define   GRIDINV_H

#include <grid.hpp>
#include <datetime.hpp>

class GridInventory
{
public:
  static const bool date_only,full_inventory;
  static const char *format_descr[8];

  GridInventory() : datetime(),inventory_type_(0),time_range_(0),period{0,0},time_unit_(0),number_averaged_(0),parameter_(0),level_type_(0),grid_type_(0),level_id{0,0},nx(0),ny(0),scan_dir(0),format_(0),pole_proj(0),dist_x(0.),dist_y(0.),flat(0.),flon(0.),dsnum(999.9),olon(0.),grid_length_(0),gds_included(false),verbose(true),filename_() {}
  size_t bottom_level() const { return level_id[1]; }
  size_t copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const;
  DateTime end_datetime() const { return datetime[1]; }
  void fill(const unsigned char *buffer,bool fill_date_only);
  void fill_from_grid(const Grid *source, Grid::Format format,float dataset_number,std::string filename);
  std::string filename() const { return filename_; }
  short format() const { return format_; }
  size_t grid_length() const { return grid_length_; }
  short grid_type() const { return grid_type_; }
  short inventory_type() const { return inventory_type_; }
  bool is_averaged_grid() const;
  short level_type() const { return level_type_; }
  short number_averaged() const { return number_averaged_; }
  short parameter() const { return parameter_; }
  short P1() const { return period[0]; }
  short P2() const { return period[1]; }
  DateTime start_datetime() const { return datetime[0]; }
  short time_range() const { return time_range_; }
  short time_unit() const { return time_unit_; }
  const char *time_unit_string() const { return GRIBGrid::TIME_UNITS[time_unit_]; }
  size_t top_level() const { return level_id[0]; }
  void set_verbose_off() { verbose=false; }
  void set_verbose_on() { verbose=true; }
  friend std::ostream& operator<<(std::ostream& out_stream,const GridInventory& source);
  friend bool operator==(const GridInventory& source1,const GridInventory& source2);
  friend bool operator!=(const GridInventory& source1,const GridInventory& source2);
  friend bool operator<(const GridInventory& source1,const GridInventory& source2);
  friend bool operator<=(const GridInventory& source1,const GridInventory& source2);
  friend bool operator>(const GridInventory& source1,const GridInventory& source2);
  friend bool operator>=(const GridInventory& source1,const GridInventory& source2);

private:
  static const short grid_types[8];
  static const size_t fixed_grid_lengths[8];
  DateTime datetime[2];
  short inventory_type_,time_range_,period[2],time_unit_,number_averaged_,parameter_,level_type_,grid_type_;
  size_t level_id[2];
  short nx,ny,scan_dir,format_,pole_proj;
  float dist_x,dist_y,flat,flon,dsnum;
  float olon;
  size_t grid_length_;
  bool gds_included,verbose;
  std::string filename_;
};

namespace gridInventory {

extern int inventory_grids(std::string input_filename,size_t format);

extern void fill_level_info_from_ncar_grids(const Grid *source,short& param,short& ltype,size_t *level_id);
extern void fill_time_info_from_ncar_grids(const Grid *source,DateTime& datetime,short& trange,short& p2,short& tunit);

} // end namespace gridInventory

#endif
