// FILE: bufr.h

#ifndef BUFR_H
#define BUFR_H

#include <vector>
#include <functional>
#include <iodstream.hpp>
#include <observation.hpp>
#include <datetime.hpp>
#include <mymap.hpp>
#include <arrayo.hpp>

class BUFR
{
public:
  struct Descriptor {
    Descriptor() : f(),x(),y(),ignore(false) {}

    unsigned char f,x,y;
    bool ignore;
  };
  struct StringEntry {
    StringEntry() : key() {}

    std::string key;
  };
  BUFR() {}
  virtual ~BUFR() {}
  friend bool operator==(const Descriptor& source1,const Descriptor& source2) {
    if (source1.f == source2.f && source1.x == source2.x && source1.y == source2.y) {
	return true;
    }
    else {
	return false;
    }
  }
  friend bool operator!=(const Descriptor& source1,const Descriptor& source2) {
    return !(source1 == source2);
  }
};

class InputBUFRStream : public BUFR, public idstream
{
public:
  struct TableAEntry {
    TableAEntry() : key(),description() {}

    size_t key;
    std::string description;
  };
  struct TableBEntry {
    TableBEntry() : key(),width(0),scale(0),ref_val(0),sign(),element_name(),units(),is_local(false) {}

    size_t key;
    short width,scale;
    int ref_val;
    struct Sign {
	Sign() : scale(0),ref_val(0) {}

	char scale,ref_val;
    } sign;
    std::string element_name,units;
    bool is_local;
  };
  struct TableDEntry {
    TableDEntry() : key(),sequence() {}

    size_t key;
    std::vector<Descriptor> sequence;
  };

  InputBUFRStream() : clear_tables_on_type_11(true),table_a(),table_b(nullptr),table_b_length_(0),table_d(nullptr) {}
  void fill_table_b(std::string path_to_bufr_tables,short src,short sub_center);
  void fill_table_d(std::string path_to_bufr_tables,short src,short sub_center);
  int table_b_length() const { return table_b_length_; }
  int ignore() { return bfstream::error; }
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  void add_entry_to_table_a(TableAEntry aentry);
  void update_table_a_entry(TableAEntry aentry) { table_a.replace(aentry); }
  void clear_table_a() { table_a.clear(); }
  void clear_table_d();
  std::string data_category_description(short data_type);
  void add_table_d_sequence_descriptor(Descriptor sequence,Descriptor descriptor_to_add);
  void define_table_b_data_width(Descriptor descriptor,short width);
  void define_table_b_element_name(Descriptor descriptor,const char *element_name_string);
  void define_table_b_reference_value_sign(Descriptor descriptor,char sign);
  void define_table_b_reference_value(Descriptor descriptor,int ref_val);
  void define_table_b_scale_sign(Descriptor descriptor,char sign);
  void define_table_b_scale(Descriptor descriptor,short scale);
  void define_table_b_units_name(Descriptor descriptor,const char *units_string);
  void print_table_a();
  void print_table_b(const char *type_string);
  void print_table_d();
  TableBEntry table_b_entry(short f,short x,short y);
  TableBEntry table_b_entry(Descriptor descriptor) { return table_b_entry(descriptor.f,descriptor.x,descriptor.y); }
  TableDEntry table_d_entry(short f,short x,short y);
  TableDEntry table_d_entry(Descriptor descriptor) { return table_d_entry(descriptor.f,descriptor.x,descriptor.y); }

  bool clear_tables_on_type_11;

private:
  void add_table_b_entries(std::ifstream& ifs);
  void extract_sequences(std::ifstream& ifs);
  size_t make_key(short f,short x,short y) const;

  my::map<TableAEntry> table_a;
  std::unique_ptr<TableBEntry[]> table_b;
  size_t table_b_length_;
  std::unique_ptr<my::map<TableDEntry>> table_d;
};

class BUFRData : public BUFR
{
public:
  BUFRData() : filled(false) {}
  virtual ~BUFRData() {}
  virtual void print() const=0;

  bool filled;
};

class NCEPPREPBUFRData : public BUFRData
{
public:
  struct Type {
    Type() : prepbufr(0),ON29(255),instr(255) {}

    short prepbufr,ON29;
    short instr;
  };
  struct CategoryGroup {
    CategoryGroup() : code(0),pres(),hgt(),temp(),td(),q(),dd(),ff(),u(),v(),pres_fcst(),hgt_fcst(),temp_fcst(),q_fcst(),u_fcst(),v_fcst(),pres_err(),hgt_err(),temp_err(),q_err(),ff_err(),pres_anal(),hgt_anal(),temp_anal(),q_anal(),u_anal(),v_anal() {}

    short code;
    Array<float> pres,hgt,temp,td,q,dd,ff,u,v;
    Array<float> pres_fcst,hgt_fcst,temp_fcst,q_fcst,u_fcst,v_fcst;
    Array<float> pres_err,hgt_err,temp_err,q_err,ff_err;
    Array<float> pres_anal,hgt_anal,temp_anal,q_anal,u_anal,v_anal;
  };
  NCEPPREPBUFRData() : stnid(),acftid(),satid(),datetime(),lat(0.),elon(0.),elev(-999),hr(-99),dhr(0),type(),cat_groups(),seq_num(255) {}
  void print() const;

  std::string stnid,acftid,satid;
  DateTime datetime;
  float lat,elon;
  int elev;
  short hr,dhr;
  Type type;
  std::vector<CategoryGroup> cat_groups;
  size_t seq_num;
};

class NCEPADPBUFRData : public BUFRData
{
public:
  struct MultiLevelData {
    MultiLevelData() : lev_type(),pres(),z(),gp(),t(),dp(),dd(),ff() {}

    Array<unsigned char> lev_type;
    Array<float> pres,z,gp,t,dp,dd,ff;
  };
  struct SingleLevelData {
    SingleLevelData() : pres(-999.),z(-9999.),gp(-999.),palt(-999.),t(-999.),dp(-999.),rh(-99.),dd(-999.),ff(-999.),prcp{-9.99,-9.99,-9.99,-9.99} {}

    float pres,z,gp,palt,t,dp,rh,dd,ff;
    float prcp[4];
  };
  NCEPADPBUFRData() : wmoid(),acrn(),acftid(),satid(),rpid(),stsn(),datetime(),lat(0.),lon(0.),elev(0),multi_level_data(),single_level_data(),found_raw(false),set_date(false) {}
  void print() const;

  std::string wmoid,acrn,acftid,satid,rpid,stsn;
  DateTime datetime;
  float lat,lon;
  int elev;
  MultiLevelData multi_level_data;
  SingleLevelData single_level_data;
  bool found_raw,set_date;
};

class TD5900
{
public:
  TD5900() : wmoid(),stn_shortname(),stn_type(0),equip_type(0),time_sig(0),avg_period(0),datetime(),lat(0.),lon(0.),freq(0.),elev(0) {}
  virtual ~TD5900() {}

  std::string wmoid,stn_shortname;
  short stn_type,equip_type,time_sig,avg_period;
  DateTime datetime;
  float lat,lon,freq;
  int elev;
};

class TD5900WindsBUFRData : public BUFRData, public TD5900
{
public:
  TD5900WindsBUFRData() : hgt(),dd(),ff(),ff_stddev(),w(),w_stddev(),qual() {}
  void print() const;

  Array<int> hgt,dd;
  Array<float> ff,ff_stddev,w,w_stddev;
  Array<char> qual;
};

class TD5900SurfaceBUFRData : public BUFRData, public TD5900
{
public:
  TD5900SurfaceBUFRData() : stnp(0.),slp(0.),temp(0.),ff(0.),rain_rate(0.),rh(0),dd(0) {}
  void print() const;

  float stnp,slp,temp,ff,rain_rate;
  int rh,dd;
};

class TD5900MomentBUFRData : public BUFRData, public TD5900
{
public:
  struct BeamData {
    BeamData() : mom_0(),snr(),hgt(),navg(),rad_vel(),spec_width(),qual() {}

    Array<short> mom_0,snr;
    Array<int> hgt,navg;
    Array<float> rad_vel,spec_width;
    Array<char> qual;
  };
  TD5900MomentBUFRData() : azimuth(),elevation(),beam_data() {}
  void print() const;

  Array<float> azimuth,elevation;
  Array<BeamData> beam_data;
};

class NCEPRadianceBUFRData : public BUFRData
{
public:
  struct RadianceGroup {
    RadianceGroup() : sat_instr(0),fov_num(0),datetime(),lat(0.),lon(0.),channel_data() {}

    short sat_instr,fov_num;
    DateTime datetime;
    float lat,lon;
    struct ChannelData {
      ChannelData() : num(),bright_temp() {}

      Array<unsigned char> num;
      Array<float> bright_temp;
    } channel_data;
  };
  NCEPRadianceBUFRData() : satid(),elev(0.),radiance_groups() {}
  void print() const;

  std::string satid;
  float elev;
  std::vector<RadianceGroup> radiance_groups;
};

class ECMWFBUFRData : public BUFRData
{
public:
  struct Data {
    Data() : rdb_type(0),rdb_subtype(0),datetime(),lat(0.),lon(0.),ID() {}

    short rdb_type,rdb_subtype;
    DateTime datetime;
    float lat,lon;
    std::string ID;
  };
  ECMWFBUFRData() : ecmwf_os(),wmoid(),shipid(),buoyid(),satid(),acftid(),datetime(),lat(0.),lon(0.) {}
  void fill(const unsigned char *input_buffer,size_t offset);
  void print() const;

  Data ecmwf_os;
  std::string wmoid,shipid,buoyid,satid,acftid;
  DateTime datetime;
  float lat,lon;
};

class BUFRReport : public BUFR
{
public:
  static const bool header_only,full_report;
  static const short seq_len[22][128];

  BUFRReport() : report(),os(nullptr),descriptors(),num_descriptors(0),data_present(),data_handler_(nullptr),data_(nullptr) {}
  BUFRReport(const BUFRReport& source) : BUFRReport() { *this=source; }
  ~BUFRReport();
  BUFRReport& operator=(const BUFRReport& source);
  void dump_descriptors(bool dump_descriptors) { report.dump_descriptors=dump_descriptors; }
  short center() const { return report.src; }
  BUFRData **data() const { return data_; }
  short data_type() const { return report.data_type; }
  short data_subtype() const { return report.data_subtype; }
  DateTime date_time() const { return report.datetime; }
  void fill(InputBUFRStream& istream,const unsigned char *stream_buffer,bool fill_header_only,std::string path_to_bufr_tables,std::string file_of_descriptors = "");
  short number_of_subsets() const { return report.nsubs; }
  void print() const;
  void print_data(bool printed_headers) const;
  void print_descriptors(InputBUFRStream& istream,bool verbose);
  void print_header(bool verbose) const;
  void set_data_handler(void (*data_handler)(Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only)) { data_handler_=data_handler; }
  short sub_center() const { return report.sub_center; }

private:
  void unpack_is(const unsigned char *input_buffer);
  void unpack_ids(const unsigned char *input_buffer);
  void unpack_os_length(const unsigned char *input_buffer);
  void unpack_dds(InputBUFRStream& istream,const unsigned char *input_buffer,std::string file_of_descriptors,bool fill_header_only);
  void decode_os(const unsigned char *input_buffer);
  void unpack_ds(InputBUFRStream& istream,const unsigned char *input_buffer,bool fill_header_only);
  void unpack_end(const unsigned char *input_buffer);
  void do_substitution(size_t& offset,Descriptor descriptor,InputBUFRStream& istream,const unsigned char *input_buffer,int subset,bool fill_header_only);
  void decode_descriptor(size_t& offset,Descriptor& descriptor,InputBUFRStream& istream,const unsigned char *input_buffer,int subset,bool fill_header_only);
  void decode_sequence(unsigned char xx,unsigned char yy,InputBUFRStream& istream,std::vector<Descriptor>& descriptor_list,std::vector<Descriptor>::iterator& it);
  size_t delayed_replication_factor(size_t& offset,InputBUFRStream& istream,const unsigned char *input_buffer,Descriptor descriptor);
  void decode_descriptor_list(size_t& offset,InputBUFRStream& istream,const unsigned char *input_buffer,size_t start_index,size_t num_to_decode,size_t num_reps,int subset,bool fill_header_only);
  void print_descriptor_group(std::string indent,size_t index,int num_to_print,InputBUFRStream& istream,bool verbose);

  struct Report {
    Report() : length(0),is_length(0),ids_length(0),os_length(0),dds_length(0),ds_length(0),edition(0),table(0),src(0),sub_center(0),update(0),data_type(0),data_subtype(0),master_version(0),local_version(0),nsubs(0),datetime(),dds_flag(),os_included(false),def(),local_desc_width(0),change_width(0),change_scale(0),change_ref_val(1),dump_descriptors(false) {}

    size_t length,is_length,ids_length,os_length,dds_length,ds_length;
    short edition,table,src,sub_center,update,data_type,data_subtype,master_version,local_version,nsubs;
    DateTime datetime;
    unsigned char dds_flag;
    bool os_included;
    Descriptor def;
    size_t local_desc_width;
    short change_width,change_scale;
    int change_ref_val;
    bool dump_descriptors;
  } report;
  unsigned char *os;
  std::vector<Descriptor> descriptors;
  size_t num_descriptors;
  struct DataPresent {
    DataPresent() : descriptors(),num_operators(0),add_to_descriptors(false),bitmap_started(false),bitmap(),bitmap_index(0) {}

    std::vector<Descriptor> descriptors;
    size_t num_operators;
    bool add_to_descriptors,bitmap_started;
    std::string bitmap;
    size_t bitmap_index;
  } data_present;
  std::function<void(Descriptor&,const unsigned char*,const InputBUFRStream::TableBEntry&,size_t,BUFRData**,int,short,bool)> data_handler_;
  BUFRData **data_;
};

extern double bufr_value(long long packed_value,short width,short scale,int reference_value);
extern double bufr_value(long long packed_value,InputBUFRStream::TableBEntry bentry);

extern void unpack_bufr_values(const unsigned char *buffer,size_t offset,short width,short scale,int reference_value,double **packed_values,short num_values);

extern std::string prepbufr_id_key(std::string clean_ID,std::string pentry_key,std::string message_type);

extern void handle_ncep_prepbufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only);
extern void handle_ncep_adpbufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only);
extern void handle_td5900_winds_bufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only);
extern void handle_td5900_surface_bufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only);
extern void handle_td5900_moment_bufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only);
extern void handle_ncep_radiance_bufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only);
extern void handle_ecmwf_bufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only);

#endif
