// FILE: raob.h
//

#ifndef RAOB_H
#define   RAOB_H

#include <iostream>
#include <vector>
#include <memory>
#include <observation.hpp>
#include <bfstream.hpp>
#include <iodstream.hpp>

class Raob : public Observation {
public:
  enum {tsr_format=1, cards_format, ussr_format, adp_format, french_format,
      datsav_format, navyspot_format, kunia_format, ruship_format,
      galapagos_format, argentina_format, mit_format, china_format,
      galapagoshr_format};
  struct RaobLevel {
    RaobLevel() : pressure(0.), temperature(0.), moisture(0.), wind_speed(0.),
        height(0), wind_direction(0), type(0), ucomp(0.), vcomp(0.), time(0.),
        level_quality(' '), time_quality(' '), pressure_quality(' '),
        height_quality(' '), temperature_quality(' '), moisture_quality(' '),
        wind_quality(' ') { }

    float pressure, temperature, moisture, wind_speed;
    int height, wind_direction;
    size_t type;
    float ucomp, vcomp, time;
    char level_quality, time_quality, pressure_quality, height_quality,
        temperature_quality, moisture_quality, wind_quality;
  };
  Raob() : nlev(0),levels() { }
  virtual size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const =0;
  DateTime date_time() const { return date_time_; }
  virtual void fill(const unsigned char *stream_buffer, bool
      fill_header_only)=0;
  RaobLevel level(short level_number) const;
  ObservationLocation location() const { return location_; }
  short number_of_levels() const { return nlev; }
  virtual void print(std::ostream& outs) const =0;
  virtual void print_header(std::ostream& outs, bool verbose) const =0;

protected:
  short nlev;
  std::vector<RaobLevel> levels;
};

class InputTsraobStream : public idstream {
public:
  InputTsraobStream() { }
  InputTsraobStream(const char *filename) { open(filename); }
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class OutputTsraobStream : public odstream {
public:
  OutputTsraobStream() { }
  OutputTsraobStream(const char *filename, size_t blocking_flag);
  int write(const unsigned char *buffer, size_t num_bytes);
};

class CardsRaob;
class USSRRaob;
class FrenchRaob;
class ADPRaob;
class DATSAVRaob;
class NavySpotRaob;
class KuniaRaob;
class GalapagosSounding;
class GalapagosHighResolutionSounding;
class ArgentinaRaob;
class ChinaRaob;
class ChinaWind;
class Tsraob : public Raob {
public:
  struct TsrFlags {
    TsrFlags() : ubits(0), format(0), source(0), height_temperature_status(0),
        wind_status(0), surface_level(0), wind_units(0), moisture_units(0),
        additional_data(false) { }

    short ubits, format, source, height_temperature_status, wind_status,
        surface_level, wind_units, moisture_units;
    bool additional_data;
  };
  Tsraob() : tsr(), rcb() { }
  void operator+=(const Tsraob& source);
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  int deck_number() const { return tsr.deck; }
  bool equals(const Tsraob& source);
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  float first_level_time() const { return tsr.trel[0]; }
  TsrFlags flags() const { return tsr.flags; }
  void initialize(size_t num_levels);
  float last_level_time() const { return tsr.trel[1]; }
  size_t recompute_bits(short level_number) const
  { return (level_number < nlev) ? rcb[level_number] : 0x7fffffff; }
  void run_quality_check(size_t dz_tolerance);
  bool parameter_quality_is_ASCII() const { return tsr.qual_is_ascii; }
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;
  const short* quality_indicators() const { return tsr.ind; }
  void set_date_time(DateTime date_time_to_set) { date_time_=date_time_to_set; }
  void set_flags(TsrFlags flags) { tsr.flags=flags; }
  void set_format_flag(size_t flag) { tsr.format_flag=flag; }
  void set_level(short level_number, RaobLevel level, size_t recompute_bits) {
      levels[level_number]=level; rcb[level_number]=recompute_bits;
  }
  void set_location(ObservationLocation location) { location_=location; }
  void set_quality_indicators(float release_time[2], short
      quality_indicators[9]);
  Tsraob& operator=(const CardsRaob& source);
  Tsraob& operator=(const USSRRaob& source);
  Tsraob& operator=(const FrenchRaob& source);
  Tsraob& operator=(const ADPRaob& source);
  Tsraob& operator=(const DATSAVRaob& source);
  Tsraob& operator=(const NavySpotRaob& source);
  Tsraob& operator=(const KuniaRaob& source);
  Tsraob& operator=(const GalapagosSounding& source);
  Tsraob& operator=(const GalapagosHighResolutionSounding& source);
  Tsraob& operator=(const ArgentinaRaob& source);
  Tsraob& operator=(const ChinaRaob& source);
  Tsraob& operator=(const ChinaWind& source);
  friend bool operator==(const Tsraob& source1, const Tsraob& source2);
  friend bool operator!=(const Tsraob& source1, const Tsraob& source2) {
      return !(source1 == source2);
  }
  friend bool operator<(const Tsraob& source1, const Tsraob& source2);
  friend bool operator>(const Tsraob& source1, const Tsraob& source2);
  friend std::ostream& operator<<(std::ostream& outs, const Tsraob& source) {
      source.print(outs); return outs;
  }

private:
  float computeVirtualTemperature(float p, float t, float m);

  static const float mois_scale[4];
  static const int mois_bias[4];
  static const size_t mois_precision[4];
  struct Tsr {
    Tsr() : flags(), deck(0), ind(), nind(0), trel(), qual_is_ascii(),
        format_flag(1) { }

    TsrFlags flags;
    int deck;
    short ind[9], nind;
    float trel[2];
    bool qual_is_ascii;
    short format_flag;
  } tsr;
  std::vector<size_t> rcb;
};

class InputCardsRaobStream : public idstream {
public:
  InputCardsRaobStream() : first_card(), at_eof(false) { }
  InputCardsRaobStream(const char *filename) : InputCardsRaobStream() {
      open(filename);
  }
  void close();
  int ignore();
  bool open(std::string filename);
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  unsigned char first_card[80];
  bool at_eof;
};

class OutputCardsRaobStream : public odstream {
public:
};

class CardsRaob : public Raob {
public:
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;

private:
};

class InputUSSRRaobStream : public idstream {
public:
  InputUSSRRaobStream() { }
  InputUSSRRaobStream(const char *filename) { open(filename); }
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class OutputUSSRRaobStream : public odstream {
public:
  OutputUSSRRaobStream() { }
  OutputUSSRRaobStream(const char *filename, size_t blocking_flag);
  bool open(std::string filename, iods::Blocking blocking_flag = iods::
      Blocking::cos);
  int write(const unsigned char *buffer, size_t num_bytes);
};

class USSRRaob : public Raob {
public:
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  short moisture_and_wind_type() const { return uraob.mois_wind_type; }
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;
  short sounding_type() const { return uraob.sond_type; }
  friend bool operator==(const USSRRaob& source1, const USSRRaob& source2);
  friend bool operator!=(const USSRRaob& source1, const USSRRaob& source2) {
      return !(source1 == source2);
  }
  friend bool operator<(const USSRRaob& source1, const USSRRaob& source2);
  friend bool operator>(const USSRRaob& source1, const USSRRaob& source2);
  friend std::ostream& operator<<(std::ostream& outs, const USSRRaob& source) {
      source.print(outs); return outs;
  }

private:
  struct {
    short mois_wind_type, sond_type;
    short syn_hour;
    short mrdsq, quad, instru_type;
    short cloud[2][5];
  } uraob;
};

extern int USSRRaobCompare(Raob::RaobLevel& left, Raob::RaobLevel& right);

class ADPRaob : public Raob {
public:
  static const float mand_pres[20];
  static const int mand_hgt[15];
  enum {ASCII_MAX=6440};
  ADPRaob() : curr_raob_ptr(0) { }
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;

private:
  short curr_raob_ptr;
};

extern int ADPCompareHeights(Raob::RaobLevel& left, Raob::RaobLevel& right);
extern int ADPComparePressures(Raob::RaobLevel& left, Raob::RaobLevel& right);

class InputFrenchRaobStream : public idstream {
public:
  InputFrenchRaobStream() : file_buf(nullptr), block_size(0), file_buf_pos(0),
      file_buf_len(0) { }
  InputFrenchRaobStream(const char *filename) : InputFrenchRaobStream() {
      open(filename);
  }
  void close();
  int ignore();
  bool open(std::string filename);
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  void read_from_disk();

  std::unique_ptr<unsigned char[]> file_buf;
  size_t block_size, file_buf_pos;
  int file_buf_len;
};

class OutputFrenchRaobStream : public odstream {
public:

private:
};

class FrenchRaob : public Raob {
public:
  struct FrenchFlags {
    FrenchFlags() : time_deviation(0), measurement_method(0), sounding_type(0)
        { }

    short time_deviation, measurement_method, sounding_type;
  };
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  FrenchFlags flags() const { return fraob.flags; }
  size_t maximum_wind_height() const { return fraob.max_wind_height; }
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;

private:
  struct FRaob {
    FRaob() : flags(), max_wind_height(0) { }

    FrenchFlags flags;
    size_t max_wind_height;
  } fraob;
};

class InputDATSAVRaobStream : public idstream {
public:
  InputDATSAVRaobStream() { }
  InputDATSAVRaobStream(const char *filename) { open(filename); }
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class OutputDATSAVRaobStream : public odstream {
public:
  OutputDATSAVRaobStream() { }
  OutputDATSAVRaobStream(const char *filename) { open(filename); }
  int write(const unsigned char *buffer, size_t num_bytes);

private:
};

class DATSAVRaob : public Raob {
public:
  DATSAVRaob() : draob(), addl_data(nullptr), remarks(nullptr), quality(nullptr)
      { }
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  std::string report_type() const { return draob.type; }
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;

private:
  struct DRaob {
    DRaob() : type(), call(), wind_type(), num_addl(0), num_rmks(0),
        num_reject(0), thermo_code(0), wind_code(0), qual_rate(0), source(0) { }

    std::string type, call;
    char wind_type;
    size_t num_addl, num_rmks, num_reject;
    size_t thermo_code, wind_code, qual_rate, source;
  } draob;
  std::unique_ptr<char[]> addl_data, remarks, quality;
};

extern int DATSAVRaobHeightCompare(Raob::RaobLevel& left, Raob::RaobLevel&
    right);
extern int DATSAVRaobPressureCompare(Raob::RaobLevel& left, Raob::RaobLevel&
    right);

class NavySpotRaob : public Raob {
public:
  static const size_t ua_mand_pres[10];
  static const size_t pibal_mand_pres[12];
  static const int std_z[10];
  NavySpotRaob() : navyraob() { }
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;
  short sounding_type() const { return navyraob.type; }
  short wind_units() const { return navyraob.wind_ind; }

private:
  size_t merge();
  struct NavyRaob {
    NavyRaob() : wind_ind(0), type(0) { }

    short wind_ind, type;
  } navyraob;
};

extern int NavyCompare(Raob::RaobLevel& left, Raob::RaobLevel& right);

class InputKuniaRaobStream : public idstream {
public:
  InputKuniaRaobStream() : offset(0), buf() { }
  InputKuniaRaobStream(const char *filename) : InputKuniaRaobStream() {
      open(filename);
  }
  int ignore();
  bool open(std::string filename);
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  size_t offset;
  unsigned char buf[3000];
};

class KuniaRaob : public Raob {
public:
  static const size_t mand_pres[7];
  static const size_t std_z[7];
  KuniaRaob() : kunia() { levels.resize(7); }
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  short moisture_flag(size_t level_number) {
      return kunia.mois_flag[level_number];
  }
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;

private:
  struct Kunia {
    Kunia() : mois_flag{0, 0, 0, 0, 0, 0, 0} { }

    short mois_flag[7];
  } kunia;
};

class InputRussianShipRaobStream : public idstream {
public:
  InputRussianShipRaobStream() { }
  InputRussianShipRaobStream(const char *filename) { open(filename); }
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
};

class RussianShipRaob : public Raob {
public:
  RussianShipRaob() : octant(0), type(0), device(0), call_sign(0),
      wind_quality2() { }
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;

private:
  short octant, type, device;
  size_t call_sign;
  std::vector<char> wind_quality2;
};

class InputGalapagosSoundingStream : public idstream {
public:
  InputGalapagosSoundingStream() : sline(), date_time() { }
  InputGalapagosSoundingStream(const char *filename) :
      InputGalapagosSoundingStream() {
          open(filename);
      }
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  void set_date(DateTime& date_time_reference);

  std::string sline;
  DateTime date_time;
};

class GalapagosSounding : public Raob {
public:
  GalapagosSounding() { }
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;
};

class GalapagosHighResolutionSounding : public Raob {
public:
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;
};

extern int sortGalapagosByPressure(Raob::RaobLevel& left, Raob::RaobLevel&
    right);

class InputArgentinaRaobStream : public idstream {
public:
  InputArgentinaRaobStream() : last_ID(), last_record() { }
  InputArgentinaRaobStream(const char *filename) : InputArgentinaRaobStream() {
      open(filename);
  }
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  std::string last_ID,last_record;
};

class ArgentinaRaob : public Raob {
public:
  struct SortEntry {
    SortEntry() : level(), rh(0) { }

    RaobLevel level;
    int rh;
  };
  size_t copy_to_buffer(unsigned char *output_buffer ,size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;

private:
  std::vector<int> rh;
};

extern int compareArgentinaPressures(ArgentinaRaob::SortEntry& left,
    ArgentinaRaob::SortEntry& right);

class InputBrazilSoundingStream: public idstream {
public:
  InputBrazilSoundingStream() : sline(), date_time() { }
  InputBrazilSoundingStream(const char *filename) :
      InputBrazilSoundingStream() {
          open(filename);
      }
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);

private:
  std::string sline;
  DateTime date_time;
};

class BrazilSounding : public Raob {
public:
  BrazilSounding() { }
  BrazilSounding(const BrazilSounding& source) { *this=source; }
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;

private:
};

class InputMITRaobStream : public idstream {
public:
  InputMITRaobStream() { }
  InputMITRaobStream(const char *filename) { open(filename); }
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class MITRaob : public Raob {
public:
  MITRaob() : station_ID(0) { }
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;

private:
  size_t station_ID;
};

extern int compareMITLevels(Raob::RaobLevel& left, Raob::RaobLevel& right);

class InputChinaRaobStream : public idstream {
public:
  InputChinaRaobStream() { }
  InputChinaRaobStream(const char *filename) { open(filename); }
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class ChinaRaob : public Raob {
public:
  ChinaRaob() : instrument_type(0) { }
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;

private:
  size_t instrument_type;
};

class InputUADBRaobStream : public idstream {
public:
  InputUADBRaobStream() { }
  InputUADBRaobStream(const char *filename) { open(filename); }
  int ignore();
  int peek();
  int read(unsigned char *buffer, size_t buffer_length);
};

class UADBRaob : public Raob {
public:
  UADBRaob() : id_type(0), obs_type(0) { }
  size_t copy_to_buffer(unsigned char *output_buffer, size_t
      buffer_length) const;
  void fill(const unsigned char *stream_buffer, bool fill_header_only);
  int ID_type() const { return id_type; }
  int observation_type() const { return obs_type; }
  void print(std::ostream& outs) const;
  void print_header(std::ostream& outs, bool verbose) const;

private:
  int id_type, obs_type;
};

#endif
