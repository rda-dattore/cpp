#include <fstream>
#include <string>
#include <strutils.hpp>
#include <web/web.hpp>

void QueryString::fill(int method,bool convert_codes)
{
  switch (method) {
    case GET: {
	char *env;
	if ( (env=getenv("QUERY_STRING")) != nullptr) {
	  fill(env);
	}
	break;
    }
    case POST: {
	char *env;
	if ( (env=getenv("CONTENT_LENGTH")) != nullptr) {
	  auto content_length=atoi(env);
	  char *buffer=new char[content_length];
	  std::cin.read(buffer,content_length);
	  if (std::string(buffer,2) == "--") {
	    std::string content(buffer,content_length);
	    auto idx=content.find("\r\n");
	    if (idx == std::string::npos) {
		idx=content.find("\n");
	    }
	    auto boundary=content.substr(0,idx);
	    auto m=0;
	    std::string field_name,filename,name_of_file;
	    auto file_start=0;
	    auto file_end=0;
	    for (size_t n=0; n < content_length-boundary.length(); ++n) {
		if (std::string(&buffer[n],boundary.length()) == boundary) {
		  if (m > 0) {
		    if (filename.empty()) {
			std::string value(&buffer[m],n-m-2);
			if (name_value_table.find(field_name) == name_value_table.end()) {
			  name_value_table.emplace(field_name,std::make_tuple(field_name,std::list<std::string>()));
			}
			(std::get<1>(name_value_table[field_name])).emplace_back(value);
		    }
		    else {
			file_start=m;
			file_end=n-2;
		    }
		  }
		  n+=boundary.length();
		  if (buffer[n] == 0xd && buffer[n+1] == 0xa) {
// Content-disposition line
		    n+=2;
		    m=n;
		    while (m < (content_length-1) && (buffer[m] != 0xd || buffer[m+1] != 0xa)) {
			++m;
		    }
		    std::string content_disposition(&buffer[n],m-n);
		    Pair p;
		    field_name=content_disposition.substr(content_disposition.find("name=\"")+6);
		    field_name=field_name.substr(0,field_name.find("\""));
		    if ( (idx=content_disposition.find("filename")) != std::string::npos) {
			filename=content_disposition.substr(idx+10);
			filename=filename.substr(0,filename.find("\""));
			if (name_value_table.find(field_name) == name_value_table.end()) {
			  name_value_table.emplace(field_name,std::make_tuple(field_name,std::list<std::string>()));
			}
			(std::get<1>(name_value_table[field_name])).emplace_back(filename);
			name_of_file=field_name;
		    }
		    else {
			filename="";
		    }
// blank line or Content-type line
		    n=m+2;
		    m=n;
		    while (m < (content_length-1) && (buffer[m] != 0xd || buffer[m+1] != 0xa)) {
			++m;
		    }
		    std::string line(&buffer[n],m-n);
		    if (!line.empty()) {
// if found Content-type, then skip blank line
			n=m+2;
			m=n;
			while (m < (content_length-1) && (buffer[m] != 0xd || buffer[m+1] != 0xa)) {
			  ++m;
			}
		    }
		    n=m+2;
		    m=n;
		  }
		}
	    }
	    if (file_end > 0) {
// save uploaded file
		filename=value(name_of_file);
		auto directory=value("directory");
		std::ofstream ofs;
		if (!directory.empty()) {
		  if (directory[0] != '/') {
		    directory=std::string(getenv("DOCUMENT_ROOT"))+"/"+directory;
		  }
		  ofs.open((directory+"/"+filename).c_str());
		}
		else {
		  ofs.open(("/usr/local/www/server_root/tmp/"+filename).c_str());
		}
		ofs.write(&buffer[file_start],file_end-file_start);
		ofs.close();
	    }
	  }
	  else {
	    fill(buffer,content_length,convert_codes);
	  }
	  delete[] buffer;
	}
	break;
    }
  }
}

void QueryString::fill(std::string query_string,bool convert_codes)
{
  name_value_table.clear();
  if (query_string.empty()) {
    return;
  }
  raw_string=query_string;
  Pair p;
  if (strutils::has_beginning(query_string,"\"") && strutils::has_ending(query_string,"\"")) {
    strutils::chop(query_string);
    query_string=query_string.substr(1);
  }
  auto nv_pairs=strutils::split(query_string,"&");
  for (size_t n=0; n < nv_pairs.size(); ++n) {
    auto nv_pair=nv_pairs[n];
    if (convert_codes) {
	webutils::cgi::convert_codes(nv_pair);
    }
    strutils::trim(nv_pair);
    auto parts=strutils::split(nv_pair,"=");
    if (parts.size() != 2) {
	if (strutils::contains(nv_pair,"=") && !strutils::contains(nv_pair,"=[")) {
	  nv_pair=nv_pair.substr(0,nv_pair.find("="))+"=["+nv_pair.substr(nv_pair.find("=")+1);
	}
	parts=strutils::split(nv_pair,"=[");
	if (parts.size() != 2) {
	  name_value_table.clear();
	  return;
	}
    }
    auto key=strutils::to_lower(parts[0]);
    if (name_value_table.find(key) == name_value_table.end()) {
	name_value_table.emplace(key,std::make_tuple(parts[0],std::list<std::string>()));
    }
    (std::get<1>(name_value_table[key]).emplace_back(parts[1]));
  }
}

void QueryString::fill(const char * buffer,size_t buffer_length,bool convert_codes)
{
  std::string query_string(buffer,buffer_length);
  fill(query_string,convert_codes);
}

std::list<std::string> QueryString::names()
{
  std::list<std::string> name_list;
  for (const auto& e : name_value_table) {
    name_list.emplace_back(e.first);
  }
  return name_list;
}

std::string QueryString::raw_name(std::string name)
{
  auto it=name_value_table.find(strutils::to_lower(name));
  if (it != name_value_table.end()) {
    return std::get<0>(it->second);
  }
  else {
    return "";
  }
}

bool QueryString::has_value(std::string name) const
{
  return (name_value_table.find(strutils::to_lower(name)) != name_value_table.end());
}

std::list<std::string> QueryString::raw_names()
{
  std::list<std::string> raw_name_list;
  for (const auto& e : name_value_table) {
    raw_name_list.emplace_back(std::get<0>(e.second));
  }
  return raw_name_list;
}

std::list<std::string> QueryString::values(std::string name) const
{
  auto it=name_value_table.find(strutils::to_lower(name));
  if (it != name_value_table.end()) {
    return std::get<1>(it->second);
  }
  else {
    return std::list<std::string>();
  }
}

std::list<std::string> QueryString::values_that_begin_with(std::string name)
{
  std::list<std::string> list;
  for (const auto& e : name_value_table) {
    if (strutils::has_beginning(e.first,strutils::to_lower(name))) {
	auto it=list.end();
	auto values=std::get<1>(e.second);
	list.insert(it,values.begin(),values.end());
    }
  }
  return list;
}

std::string QueryString::value(std::string name,const char *separator) const
{
  std::string value;
  auto list=values(name);
  for (auto& item : list) {
    if (!value.empty()) {
	value+=separator;
    }
    value+=item;
  }
  return value;
}

std::string QueryString::value_that_begins_with(std::string name,const char *separator)
{
  std::list<std::string> list;
  std::string s;

  list=values_that_begin_with(name);
  for (const auto& item : list) {
    if (s.length() > 0) {
	s+=separator;
    }
    s+=item;
  }
  return s;
}
