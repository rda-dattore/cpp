#include <iostream>
#include <fstream>
#include <regex>
#include <web/web.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <tempfile.hpp>
#include <myerror.hpp>

namespace webutils {

namespace cgi {

bool read_config(Directives& directives)
{
  std::ifstream ifs("/usr/local/decs/bin/conf/cgi.conf");
  if (ifs.is_open()) {
    char line[256];
    ifs.getline(line,256);
    while (!ifs.eof()) {
	auto conf_parts=strutils::split(line);
	if (conf_parts.front() == "dataRoot") {
	  if (conf_parts.size() > 2) {
	    directives.data_root=conf_parts[1];
	    directives.data_root_alias=conf_parts[2];
	  }
	}
	else if (conf_parts.front() == "databaseServer") {
	  if (conf_parts.size() > 1) {
	    directives.database_server=conf_parts[1];
	  }
	}
	else if (conf_parts.front() == "rdadbUsername") {
	  if (conf_parts.size() > 1) {
	    directives.rdadb_username=conf_parts[1];
	  }
	}
	else if (conf_parts.front() == "rdadbPassword") {
	  if (conf_parts.size() > 1) {
	    directives.rdadb_password=conf_parts[1];
	  }
	}
	else if (conf_parts.front() == "metadbUsername") {
	  if (conf_parts.size() > 1) {
	    directives.metadb_username=conf_parts[1];
	  }
	}
	else if (conf_parts.front() == "metadbPassword") {
	  if (conf_parts.size() > 1) {
	    directives.metadb_password=conf_parts[1];
	  }
	}
	else if (conf_parts.front() == "rdadataHome") {
	  if (conf_parts.size() > 1) {
	    directives.rdadata_home=conf_parts[1];
	  }
	}
	ifs.getline(line,256);
    }
    ifs.close();
    return true;
  }
  else {
    return false;
  }
}

void convert_codes(std::string& string)
{
  const size_t NUM_CONVERSIONS=55;
  const char *codes[NUM_CONVERSIONS]={  "+","%20","%21","%22","%23","%24",
        "%25","%26","%27","%28","%29","%2A","%2a","%2B","%2b","%2C","%2c","%2D",
  "%2d","%2E","%2e","%2F","%2f","%3A","%3a","%3B","%3b","%3C","%3c","%3D","%3d",
  "%3E","%3e","%3F","%3f","%40","%5B","%5b","%5C","%5c","%5D","%5d","%5E","%5e",
  "%5F","%5f","%60","%7B","%7b","%7C","%7c","%7D","%7d","%7E","%7e"};
  const char *chars[NUM_CONVERSIONS]={  " ",  " ",  "!", "\"",  "#",  "$",
    "__PCT__",  "&",  "'",  "(",  ")",  "*",  "*",  "+",  "+",  ",",  ",",  "-",
    "-",  ".",  ".",  "/",  "/",  ":",  ":",  ";",  ";",  "<",  "<",  "=",  "=",
    ">",  ">",  "?",  "?",  "@",  "[",  "[", "\\", "\\",  "]",  "]",  "^",  "^",
    "_",  "_",  "`",  "{",  "{",  "|",  "|",  "}",  "}",  "~",  "~"};

  for (size_t n=0; n < NUM_CONVERSIONS; ++n) {
    strutils::replace_all(string,codes[n],chars[n]);
  }
  strutils::replace_all(string,"__PCT__","%");
  strutils::replace_all(string,"%0D%0A","\n");
  strutils::replace_all(string,"%0A","\n");
}

std::string post_data()
{
  std::string _post_data;
  auto env=getenv("CONTENT_LENGTH");
  if (env != nullptr) {
    auto content_length=std::stoi(env);
    std::unique_ptr<char[]> buffer(new char[content_length]);
    std::cin.read(buffer.get(),content_length);
    _post_data.assign(buffer.get(),content_length);
  }
  return _post_data;
}

std::string server_variable(std::string variable_name)
{
  char *env;
  if ( (env=getenv(variable_name.c_str())) != nullptr) {
    return env;
  }
  else {
    return "";
  }
}

} // end namespace cgi

void convert_html_special_characters(std::string& string)
{
  const size_t NUM_CONVERSIONS=46;
  const char *spchars[NUM_CONVERSIONS]={"&#09;","&#10;","&#13;","&#32;","&#33;",
  "&#34;","&quot;","&#35;","&#36;","&#37;","&#38;","&amp;","&#39;","&#40;",
  "&#41;","&42;","&#43","&#44;","&#45;","&#46;","&#47;","&#58;","&#59;"
      "&#60;",     "&lt;","&#61;",       "&#62;",        "&gt;","&#63;","&#64;",
  "&#91;","&#92;","&#93;","&#94;","&#95;","&#96;","&#123;","&#124;","&#125;",
  "&#126;","&#160;","&nbsp;","&nbsp","&#176;", "&deg;",  "&deg"};
  const char *strings[NUM_CONVERSIONS]={"\\tab", "\\lf", "\\cr",    " ",    "!",
     "\"",    "\"",    "#",    "$",    "%",  "and",  "and",    "'",    "(",
      ")",   "*",    "+",   ",",    "-",    ".",    "/",    ":",    ";",
  "less than","less than",    "=","greater than","greater than",    "?",    "@",
      "[",   "\\",    "]",    "^",    "_",    "`",     "{",     "|",     "}",
       "~",     " ",     " ",    " ","degree","degree","degree"};
  for (size_t n=0; n < NUM_CONVERSIONS; ++n) {
    strutils::replace_all(string,spchars[n],strings[n]);
  }
}

std::string url_encode(std::string url)
{
  const size_t NUM_CONVERSIONS=33;
  const char *chars[NUM_CONVERSIONS]={  "!", "\"",  "#",  "$",  "%",  "&",  "'",
    "(",  ")",  "*",  "+",  ",",  "-",  ".",  "/",  ":",  ";",  "<",  "=",  ">",
    "?",  "@",  "[", "\\",  "]",  "^",  "_",  "`",  "{",  "|",  "}",  "~",
         " "};
  const char *codes[NUM_CONVERSIONS]={"%21","%22","%23","%24","%25","%26","%27",
  "%28","%29","%2A","%2B","%2C","%2D","%2E","%2F","%3A","%3B","%3C","%3D","%3E",
  "%3F","%40","%5B","%5C","%5D","%5E","%5F","%60","%7B","%7C","%7D","%7E",
// leave this as last entry because of underscores
  "__PLUS__"};

  for (size_t n=0; n < NUM_CONVERSIONS; ++n) {
    strutils::replace_all(url,chars[n],codes[n]);
  }
  strutils::replace_all(url,"__PLUS__","+");
  return url;
}

void php_execute(std::string php_script,std::ostream& outs)
{
  auto fp=popen(("/usr/bin/php -q "+php_script).c_str(),"r");
  const size_t LINE_LENGTH=32768;
  char line[LINE_LENGTH];
  while (fgets(line,LINE_LENGTH,fp) != NULL) {
    outs << line;
  }
  pclose(fp);
}

std::string php_execute(std::string php_code)
{
  TempFile tfile;
  std::ofstream ofs(tfile.name().c_str());
  if (!std::regex_search(php_code,std::regex("^<\\?php"))) {
    ofs << "<?php" << std::endl;
  }
  ofs << php_code;
  if (!std::regex_search(php_code,std::regex("\\?>$"))) {
    ofs << "?>" << std::endl;
  }
  ofs.close();
  auto fp=popen(("/usr/bin/php -f "+tfile.name()).c_str(),"r");
  const size_t LINE_LENGTH=32768;
  char line[LINE_LENGTH];
  std::string output;
  while (fgets(line,LINE_LENGTH,fp) != NULL) {
    output+=line;
  }
  pclose(fp);
  return output;
}

std::string trim_from_http_request(std::string request,std::string name)
{
  std::string trimmed_request=request;
  name+="=";
  if (strutils::contains(trimmed_request,"?"+name) || strutils::contains(trimmed_request,"&"+name)) {
    auto idx=trimmed_request.find("?"+name);
    if (idx == std::string::npos) {
	name="&"+name;
	idx=trimmed_request.find(name);
    }
    else {
	name="?"+name;
    }
    auto trim_string=trimmed_request.substr(idx+1);
    if (strutils::contains(trim_string,"&")) {
	trim_string=trim_string.substr(0,trim_string.find("&"));
    }
    if (name[0] == '&') {
	trim_string="&"+trim_string;
    }
    strutils::replace_all(trimmed_request,trim_string,"");
  }
  return trimmed_request;
}

} // end namespace webutils
