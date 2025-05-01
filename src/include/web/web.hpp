// FILE: web.h

#ifndef WEB_H
#define   WEB_H

#include <string>
#include <unordered_map>
#include <ftw.h>
#include <mymap.hpp>
#include <tempfile.hpp>
#include <datetime.hpp>

class QueryString
{
public:
  enum {GET=1,POST};
  struct Pair {
    struct Data {
	Data() : raw_name(),values() {}

	std::string raw_name;
	std::list<std::string> values;
    };
    Pair() : key(),data(nullptr) {}

    std::string key;
    std::shared_ptr<Data> data;
  };
  QueryString() : raw_string(),name_value_table() {}
  QueryString(int method) : QueryString() { fill(method); }
  QueryString(const char *buffer,size_t buffer_length) : QueryString() { fill(buffer,buffer_length); }
  operator bool() { return (name_value_table.size() > 0); }
  void fill(int method,bool convert_codes = true);
  void fill(std::string query_string,bool convert_codes = true);
  void fill(const char *buffer,size_t buffer_length,bool convert_codes = true);
  bool has_value(std::string name) const;
  std::list<std::string> names();
  std::string raw_name(std::string name);
  std::list<std::string> raw_names();
  std::string to_string() const { return raw_string; }
  std::string value(std::string name,const char *separator = ",") const;
  std::list<std::string> values(std::string name) const;
  std::string value_that_begins_with(std::string name,const char *separator);
  std::list<std::string> values_that_begin_with(std::string name);

private:
  std::string raw_string;
//  my::map<Pair> name_value_table;
std::unordered_map<std::string,std::tuple<std::string,std::list<std::string>>> name_value_table;
};

class WebPage {
public:
  WebPage() : title_(),specialist_(),url_(),page_type_(),is_index_(false),buffer() {}
  virtual ~WebPage() {}
  void add_to_title(const std::string& more_title) { title_+=more_title; }
  void add_to_url(const std::string& more_url) { url_+=more_url; }
  std::string page_type() const { return page_type_; }
  std::string specialist() const { return specialist_; }
  std::string title() const { return title_; }
  std::string url() const { return url_; }
  std::string contents() const { return buffer; }
  bool is_index() const { return is_index_; }
  void set_is_index(const bool page_is_index) { is_index_=page_is_index; }
  void set_page_type(const std::string& page_type) { page_type_=page_type; }
  void set_specialist(const std::string& specialist) { specialist_=specialist; }
  void set_title(const std::string& title_specification) { title_=title_specification; }
  void set_url(const std::string& url) { url_=url; }
  void wrap();
  void wrap_dev();
  void write(const std::string& string) { buffer+=string; }
  void writeln(const std::string& string);

protected:
  std::string title_,specialist_,url_,page_type_;
  bool is_index_;

private:
  std::string buffer;
};

class WPGFile : public WebPage {
public:
  WPGFile();
  void add_file(const char *filename);
  void wrap();
  void wrap_dev();
  void write(const std::string& string) { ofs.write(string); }
  void writeln(const std::string& string) { ofs.writeln(string); }

private:
  TempFile ofs;
};

extern void clear_cookie(std::string cookie_name, std::string domain, std::
    string path);
extern void create_curl_script(const std::vector<std::string>& filelist, std::
    string server, std::string directory, std::string dsnum, std::string
    script_type, std::string parameters, std::string level, std::string product,
    std::string start_date, std::string end_date);
extern void create_curl_script(const std::vector<std::string>& filelist, std::
    string server, std::string directory, std::string script_type, std::ofstream
    *ofs = nullptr);
extern void create_python_script(const std::vector<std::string>& filelist, std::
    string server, std::string directory, std::string script_type);
extern void create_python_script(const std::vector<std::string>& filelist);
extern void create_wget_script(const std::vector<std::string>& filelist, std::
    string server, std::string directory, std::string script_type, std::ofstream
    *ofs = nullptr);
extern void create_wget_script(const std::vector<std::string>& filelist);
extern void set_cookie(std::string cookie_name, std::string cookie_value, std::
    string domain, std::string path, DateTime *expiration_datetime = nullptr);
extern void web_error(std::string message, bool print_content_type = true);
extern void web_error2(std::string message, std::string status);
extern void weblog_error(std::string error, std::string caller, std::string
    user);
extern void weblog_info(std::string warning, std::string caller, std::string
    user);

extern std::list<std::string> accesses_from_cookie();

extern std::string cookie_value(std::string name);
extern std::string duser();
extern std::string iuser(PostgreSQL::DBconfig db_config);
extern std::string session_ID();
extern std::string twiki_name();

extern bool has_access(std::string acode);

namespace webutils {

namespace cgi {

struct Directives {
  Directives() : data_root(),data_root_alias(),database_server(),rdadb_username(),rdadb_password(),metadb_username(),metadb_password(),rdadata_home() {}

  std::string data_root,data_root_alias;
  std::string database_server,rdadb_username,rdadb_password,metadb_username,metadb_password;
  std::string rdadata_home;
};

extern std::string post_data();
extern std::string server_variable(std::string variable_name);

extern void convert_codes(std::string& string);

extern bool read_config(Directives& directives);

} // end namespace cgi

extern void convert_html_special_characters(std::string& string);
extern void php_execute(std::string php_script,std::ostream& outs);

extern std::string php_execute(std::string php_code);
extern std::string trim_from_http_request(std::string request,std::string name);
extern std::string url_encode(std::string url);

extern bool filled_inventory(std::string dsnum,std::string data_file,std::string type,std::stringstream& inventory);

} // end namespace webutils

#endif
