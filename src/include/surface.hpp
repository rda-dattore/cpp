// FILE: surface.h

#ifndef SURFACE_H
#define  SURFACE_H

#include <iostream>
#include <iodstream.hpp>
#include <mymap.hpp>
#include <observation.hpp>
#include <library.hpp>

class SurfaceObservation : public Observation
{
public:
  enum {dsscanhly_format=1,isd_format,wmssc_format,cpcsumm_format};
  struct SurfaceReport {
    SurfaceReport() : data(),flags(),filled(false) {}

    struct Data {
	Data() : slp(0.),stnp(0.),tdry(0.),tdew(0.),twet(0.),rh(0.),dd(0.),ff(0.) {}

	float slp,stnp,tdry,tdew,twet,rh,dd,ff;
    } data;
    struct Flags {
	Flags() : slp(' '),stnp(' '),tdry(' '),tdew(' '),twet(' '),rh(' '),dd(' '),ff(' ') {}

	unsigned char slp,stnp,tdry,tdew,twet,rh,dd,ff;
    } flags;
    bool filled;
  };
  struct SurfaceAdditionalData {
    SurfaceAdditionalData() : data(),flags(),filled(false) {}

    struct Data {
	Data() : tmax(0.),tmin(0.),pcp_amt(0.),snowd(0.),vis(0.),tot_cloud(0.),cloud_base_hgt(0.),pres_tend(0.) {}

	float tmax,tmin,pcp_amt,snowd,vis,tot_cloud,cloud_base_hgt,pres_tend;
    } data;
    struct Flags {
	Flags() : pcp_amt(' '),pres_tend(' ') {}

	unsigned char pcp_amt,pres_tend;
    } flags;
    bool filled;
  };
  SurfaceObservation() : report_(),addl_data() {}
  SurfaceAdditionalData additional_data() const { return addl_data; }
  DateTime date_time() const { return date_time_; }
  virtual void fill(const unsigned char *stream_buffer,bool fill_header_only)=0;
  ObservationLocation location() const { return location_; }
  virtual void print(std::ostream& outs) const =0;
  virtual void print_header(std::ostream& outs,bool verbose) const =0;
  SurfaceReport report() const { return report_; }

protected:
  SurfaceReport report_;
  SurfaceAdditionalData addl_data;
};

class ADPSurfaceReport : public SurfaceObservation
{
public:
  struct ADPAdditionalData {
    float precipitation_amount_24;
    int present_wx_code,past_wx_code,lowmid_cloud_amount;
    int low_cloud_type,middle_cloud_type,high_cloud_type;
    unsigned char precipitation_time;
  };
  ADPSurfaceReport() : synoptic_date_time_(),receipt_date_time_(),report_type_(),adp_addl_data() {}
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  ADPAdditionalData ADP_additional_data() const { return adp_addl_data; }
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;
  DateTime receipt_date_time() const { return receipt_date_time_; }
  short report_type() const { return report_type_; }
  DateTime synoptic_date_time() const { return synoptic_date_time_; }

private:
  DateTime synoptic_date_time_,receipt_date_time_;
  short report_type_;
  ADPAdditionalData adp_addl_data;
};

class DATSAVSurfaceReport : public SurfaceObservation
{
public:
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;

private:
  short units;
};

class InputDSSCanadianSurfaceHourlyObservationStream : public idstream
{
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
};

class DSSCanadianSurfaceHourlyObservation : public SurfaceObservation
{
public:
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;

private:
};

class InputISDObservationStream : public idstream
{
public:
  InputISDObservationStream() : curr_offset(0) {}
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  off_t current_record_offset() const { return curr_offset; }

private:
  off_t curr_offset;
};

class ISDObservation : public SurfaceObservation
{
public:
  struct AdditionalCodedData {
    struct Data {
	Data() : off(0),len(0),addl_data_handler(nullptr),values() {}

	int off,len;
	void (*addl_data_handler)(char *buf,std::vector<std::string>& vals);
	std::vector<std::string> values;
    };
    AdditionalCodedData() : key(),data(nullptr) {}

    std::string key;
    std::shared_ptr<Data> data;
  };
  ISDObservation();
  AdditionalCodedData& additional_coded_data(std::string addl_data_code) const;
  std::list<std::string> additional_data_codes() const { return addl_data_codes; }
  char ceiling_determination_code() const { return ceiling_determination_code_; }
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;
  std::string report_type() const { return report_type_; }
  char wind_observation_type_code() const { return windobs_type_code; }

private:
  std::string report_type_,qc_proc;
  char ceiling_determination_code_,windobs_type_code;
  std::list<std::string> addl_data_codes;
  my::map<AdditionalCodedData> addl_coded_table;
  friend void decode_ASOS_AWOS_present_weather(char *buf,std::vector<std::string>& vals);
  friend void decode_atmospheric_pressure_change(char *buf,std::vector<std::string>& vals);
  friend void decode_automated_present_weather(char *buf,std::vector<std::string>& vals);
  friend void decode_extreme_air_temperature(char *buf,std::vector<std::string>& vals);
  friend void decode_ground_surface_observation(char *buf,std::vector<std::string>& vals);
  friend void decode_ground_surface_minimum_temperature(char *buf,std::vector<std::string>& vals);
  friend void decode_hourly_liquid_precipitation(char *buf,std::vector<std::string>& vals);
  friend void decode_past_weather(char *buf,std::vector<std::string>& vals);
  friend void decode_present_weather(char *buf,std::vector<std::string>& vals);
  friend void decode_sky_condition2(char *buf,std::vector<std::string>& vals);
  friend void decode_snow_accumulation(char *buf,std::vector<std::string>& vals);
  friend void decode_snow_depth(char *buf,std::vector<std::string>& vals);
};

class InputTD32ObservationStream : public idstream
{
public:
  InputTD32ObservationStream();
  void close();
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);

private:
  unsigned char block[20000];
  size_t block_len,off;
};

class TD32Observation : public SurfaceObservation
{
public:
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;

private:
  std::string type,elem,unit;
};

class InputWMSSCObservationStream : public idstream
{
public:
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);

private:
};

class WMSSCObservation : public SurfaceObservation
{
public:
  struct WMSSCAdditionalData {
    WMSSCAdditionalData() : t_dep(0.),mois(0.),mois_dep(0.),sst(0.),sst_dep(0.),mois_flag(' '),n_pcp_1mm(0),pcp_dep(0),quint_pcp(0),nobs(0),hrs_sun(0),pct_avg_sun(0) {}

    float t_dep,mois,mois_dep,sst,sst_dep;
    unsigned char mois_flag;
    int n_pcp_1mm,pcp_dep,quint_pcp,nobs,hrs_sun,pct_avg_sun;
  };
  WMSSCObservation() : wmssc(),monthly_means(),annual_slp_indicator2(),annual_wmssc_addl_data(),annual_z(0) {}
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  SurfaceAdditionalData annual_additional_data() const { return addl_data; }
  SurfaceReport annual_report() const { return report_; }
  short format() const { return wmssc.format; }
  bool has_additional_data() const { return (static_cast<int>(wmssc.addl_data_indicator) == 1); }
  WMSSCAdditionalData monthly_WMSSC_additional_data(size_t month_number) const;
  SurfaceAdditionalData monthly_additional_data(size_t month_number) const;
  SurfaceReport monthly_report(size_t month_number) const;
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;
  int ship_indicator() const { return static_cast<int>(wmssc.ship_indicator); }
  std::string station_name() const { return wmssc.name; }

private:
  struct WMSSC {
    WMSSC() : data_source(),id_source(),ship_indicator(),addl_data_indicator(),format(0),name() {}

    unsigned char data_source,id_source,ship_indicator,addl_data_indicator;
    short format;
    std::string name;
  } wmssc;
  struct MonthlyMeans {
    MonthlyMeans() : report(),addl_data(),slp_indicator2(),z(0),wmssc_addl_data() {}

    SurfaceReport report;
    SurfaceAdditionalData addl_data;
    unsigned char slp_indicator2;
    int z;
    WMSSCAdditionalData wmssc_addl_data;
  } monthly_means[12];
  unsigned char annual_slp_indicator2;
  WMSSCAdditionalData annual_wmssc_addl_data;
  int annual_z;
};

class InputCPCSummaryObservationStream : public idstream
{
public:
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);

private:
};

class CPCSummaryObservation : public SurfaceObservation
{
public:
  struct CPCDailySummaryData {
    float eprcp,vp,potev,vpd,aptmx,chillm,rad;
    int mxrh,mnrh;
    float slp06,slp12,slp18,slp00;
  };
  struct CPCMonthlySummaryData {
    float tmean,tmax,tmin,hmax,rminl,ctmean,aptmax,wcmin,hmaxapt,wclminl,rpcp,epcp,cpcp,epcpmax,ahs,avp,apet,avpd,trad;
    int amaxrh,aminrh,ihdd,icdd,igdd;
  };
  CPCSummaryObservation() : cpc_dly_summ(),cpc_mly_summ() {}
  CPCDailySummaryData CPC_daily_summary_data() const { return cpc_dly_summ; }
  CPCMonthlySummaryData CPC_monthly_summary_data() const { return cpc_mly_summ; }
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;

private:
  static DSSStationLibrary library;
  CPCDailySummaryData cpc_dly_summ;
  CPCMonthlySummaryData cpc_mly_summ;
};

class InputTD13SurfaceReportStream : public idstream
{
public:
  InputTD13SurfaceReportStream() : file_buf_len(0),file_buf_pos(0) {}
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);

private:
  size_t file_buf_len,file_buf_pos;
};

class TD13SurfaceReport : public SurfaceObservation
{
public:
  TD13SurfaceReport();
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;

private:
};

class InputTDLHourlyObservationStream : public idstream
{
public:
  InputTDLHourlyObservationStream() : curr_offset(0) {}
  int ignore();
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  off_t current_record_offset() const { return curr_offset; }

private:
  off_t curr_offset;
};

class TDLHourlyObservation : public SurfaceObservation
{
public:
  TDLHourlyObservation() : stn_type(9) {}
  void fill(const unsigned char *stream_buffer,bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs,bool verbose) const;

private:
  short stn_type;
};

#endif
