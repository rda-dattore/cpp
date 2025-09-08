#ifndef UTILS_HPP
#define   UTILS_HPP

#include <iostream>
#include <fstream>
#include <cstdint>
#include <list>
#include <deque>
#include <vector>

namespace charconversions {

extern int decode_overpunch(const char *buf,size_t buf_length,size_t overpunch_index);

extern void ascii_to_dpc(char *dest,char *src,size_t num);
extern void ascii_to_ebcdic(char *dest,char *src,size_t num);
extern void dpc_to_ascii(char *dest,char *src,size_t num);
extern void ebcdic_to_ascii(char *dest,char *src,size_t num);
extern void ebcd_to_ascii(char *dest,char *src,size_t num);
extern void ebcd_to_ebcdic(char *dest,char *src,size_t num);
extern void ibcd_to_ascii(char *dest,char *src,size_t num);
extern void ibcd_to_ebcdic(char *dest,char *src,size_t num);
extern void unk_bcd_to_ascii(char *dest,char *src,size_t num);

} // end namespace charconversions

namespace filelock {

extern int lock(const std::string& filename,std::string contents = "");
extern int unlock(const std::string& filename);

extern std::string build_lock_name(const std::string& filename);

extern bool is_locked(const std::string& filename);
extern bool open_lock_file(const std::string& filename,std::ifstream& ifs);

} // end namespace filelock

namespace floatutils {

extern double cdcconv(const unsigned char *buf,size_t off,size_t sign_representation);
extern double crayconv(const unsigned char *buf,size_t off);
extern double cyberconv(const unsigned char *buf,size_t off);
extern double cyberconv64(const unsigned char *buf,size_t off);
extern double ibmconv(const unsigned char *buf,size_t off);
extern double ibm36conv(const unsigned char *buf,size_t off);

extern size_t ibmconv(double native_real);
extern size_t precision(double value);

extern long long cdcconv(double native_real,size_t sign_representation);

extern bool myequalf(double a,double b);
extern bool myequalf(double a,double b,double tol);

} // end namespace floatutils

namespace geoutils {

extern int convert_box_to_center_lat_lon(size_t box_size,size_t lat_index,size_t lon_index,double& lat,double& lon);

extern double arc_distance(double lat1,double lon1,double lat2,double lon2);
extern double gc_distance(double lat1,double lon1,double lat2,double lon2);

extern void convert_lat_lon_to_box(size_t box_size,double lat,double lon,size_t& lat_index,size_t& lon_index);
extern void lon_index_bounds_from_bitmap(const char *bitmap,size_t& min_lon_index,size_t& max_lon_index);

} // end namespace geoutils

namespace htmlutils {

extern std::string convert_html_summary_to_ascii(std::string summary,size_t wrap_length,size_t indent_length);
extern std::string transform_superscripts(std::string s);
extern std::string unicode_escape_to_html(std::string u);

} // end namespace htmlutils

namespace metcalcs {

extern double compute_relative_humidity(double temperature,double dew_point_temperature);
extern double compute_dew_point_temperature(double dry_bulb_temperature,double wet_bulb_temperature,double station_pressure);
extern double compute_standard_height_from_pressure(double pressure);
extern double compute_height_from_pressure_and_temperature(double pressure,double temperature);
extern double stdp2z(double p);

extern void compute_wind_direction_and_speed_from_u_and_v(double u,double v,int& wind_direction,double& wind_speed);
extern void uv2wind(double u,double v,int& dd,double& ff);

} // end namespace metcalcs

namespace miscutils {

extern size_t min_byte_width(size_t value);

extern std::string this_function_label(std::string function_name);

} // end namespace miscutils

namespace unixutils {

enum class DumpType { bytes = 0, bits };

extern const char *cat(std::string filename);

extern int gdex_unlink(std::string remote_filepath, std::string api_key,
    std::string& error);
extern int gdex_upload_dir(std::string directory, std::string relative_path,
    std::string remote_path, std::string api_key, std::string& error);
extern int mysystem2(std::string command,std::stringstream& output,std::stringstream& error);

extern long long little_endian(unsigned char *buffer,size_t num_bytes);

extern std::string host_name();
extern std::string hsi_command();
extern std::string remote_file(std::string server_name,std::string local_tmpdir,std::string remote_filename,std::string& error);
extern std::string remote_web_file(std::string url,std::string local_tmpdir);
extern std::string retry_command(std::string command,int num_retries);
extern std::string unix_args_string(int argc,char **argv,char separator = ':');

extern bool exists_on_server(std::string host_name, std::string remote_path);
extern bool hosts_loaded(std::list<std::string>& hosts,std::string rdadata_home);
extern bool tar(std::string tarfile,std::list<std::string>& filenames);

extern void dump(const unsigned char *buffer,int length,DumpType = DumpType::bytes);
extern void open_output(std::ofstream& ofs, std::string filename);
extern void sendmail(std::string to_list,std::string from,std::string bcc_list,std::string subject,std::string body);
extern void untar(std::string dirname,std::string filename);

inline bool system_is_big_endian()
{
  static bool is_big_endian;
  static bool checked=false;
  if (!checked) {
    union {
	uint32_t i;
	char c[4];
    } test;
    test.i=0x04030201;
    is_big_endian=(test.c[0] == 4);
    checked=true;
  }
  return is_big_endian;
}

} // end namespace unixutils

namespace xtox {

extern int otoi(int octal);

extern long long htoi(std::string hex_string);
extern long long itoo(int ival);
extern long long ltoo(long long ival);
extern long long otoll(long long octal);

extern char *itoh(int ival);
extern char *lltoh(long long llval);

} // end namespace xtox

extern long long checksum(const unsigned char *buf,size_t num_words,size_t word_size,long long& sum);

extern bool within_polygon(std::string polygon,std::string point);

#endif
