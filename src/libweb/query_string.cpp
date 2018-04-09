#include <fstream>
#include <string>
#include <strutils.hpp>
#include <web/web.hpp>

void QueryString::fill(int method,bool convert_codes)
{
  std::ofstream ofs;
  char *env;
  std::string sdum,filename,name_of_file;
  Pair p;

  switch (method) {
    case GET:
    {
	if ( (env=getenv("QUERY_STRING")) != nullptr) {
	  fill(env);
	}
	break;
    }
    case POST:
    {
	if ( (env=getenv("CONTENT_LENGTH")) != nullptr) {
	  auto content_length=atoi(env);
	  char *buffer=new char[content_length];
	  std::cin.read(buffer,content_length);
	  if (std::string(buffer,2) == "--") {
	    sdum.assign(buffer,content_length);
	    auto idx=sdum.find("\r\n");
	    if (idx == std::string::npos) {
		idx=sdum.find("\n");
	    }
	    auto boundary=sdum.substr(0,idx);
	    auto m=0;
	    auto file_start=0;
	    auto file_end=0;
	    for (size_t n=0; n < content_length-boundary.length(); ++n) {
		if (std::string(&buffer[n],boundary.length()) == boundary) {
		  if (m > 0) {
		    if (filename.length() == 0) {
			sdum.assign(&buffer[m],n-m-2);
			if (!name_value_table.found(p.key,p)) {
			  p.data.reset(new Pair::Data);
			  p.data->raw_name=p.key;
			  name_value_table.insert(p);
			}
			p.data->values.emplace_back(sdum);
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
		    sdum.assign(&buffer[n],m-n);
		    p.key=sdum.substr(sdum.find("name=\"")+6);
		    p.key=p.key.substr(0,p.key.find("\""));
		    if ( (idx=sdum.find("filename")) != std::string::npos) {
			filename=sdum.substr(idx+10);
			filename=filename.substr(0,filename.find("\""));
			if (!name_value_table.found(p.key,p)) {
			  p.data.reset(new Pair::Data);
			  p.data->raw_name=p.key;
			  name_value_table.insert(p);
			}
			p.data->values.emplace_back(filename);
			name_of_file=p.key;
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
		    sdum.assign(&buffer[n],m-n);
		    if (sdum.length() > 0) {
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
		sdum=value("directory");
		if (sdum.length() > 0) {
		  if (sdum[0] != '/') {
		    sdum=std::string(getenv("DOCUMENT_ROOT"))+"/"+sdum;
		  }
		  ofs.open((sdum+"/"+filename).c_str());
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
  auto sp=strutils::split(query_string,"&");
  for (size_t n=0; n < sp.size(); ++n) {
    auto sdum=sp[n];
    if (convert_codes) {
	webutils::cgi::convert_codes(sdum);
    }
    strutils::trim(sdum);
    auto spx=strutils::split(sdum,"=");
    if (spx.size() != 2) {
	if (strutils::contains(sdum,"=") && !strutils::contains(sdum,"=[")) {
	  sdum=sdum.substr(0,sdum.find("="))+"=["+sdum.substr(sdum.find("=")+1);
	}
	spx=strutils::split(sdum,"=[");
	if (spx.size() != 2) {
	  name_value_table.clear();
	  return;
	}
    }
    p.key=strutils::to_lower(spx[0]);
    if (!name_value_table.found(p.key,p)) {
	p.data.reset(new Pair::Data);
	p.data->raw_name=spx[0];
	name_value_table.insert(p);
    }
    p.data->values.emplace_back(spx[1]);
  }
}

void QueryString::fill(const char * buffer,size_t buffer_length,bool convert_codes)
{
  std::string query_string(buffer,buffer_length);
  fill(query_string,convert_codes);
}

std::string QueryString::raw_name(std::string name)
{
  Pair p;
  if (name_value_table.found(strutils::to_lower(name),p)) {
    return p.data->raw_name;
  }
  else {
    return "";
  }
}

bool QueryString::has_value(std::string name)
{
  Pair p;
  return name_value_table.found(strutils::to_lower(name),p);
}

std::list<std::string> QueryString::raw_names()
{
  std::list<std::string> raw_name_list;
  Pair p;
  for (auto& key : name_value_table.keys()) {
    name_value_table.found(key,p);
    raw_name_list.emplace_back(p.data->raw_name);
  }
  return raw_name_list;
}

std::list<std::string> QueryString::values(std::string name)
{
  Pair p;
  std::list<std::string>::iterator it,end;

  if (name_value_table.found(strutils::to_lower(name),p)) {
    for (it=p.data->values.begin(),end=p.data->values.end(); it != end; ++it) {
	if (it->length() == 0) {
	  p.data->values.erase(it--);
	}
    }
    return p.data->values;
  }
  else {
    std::list<std::string> l;
    return l;
  }
}

std::list<std::string> QueryString::values_that_begin_with(std::string name)
{
  Pair p;
  std::list<std::string> list;
  std::string sdum;

  for (auto& key : name_value_table.keys()) {
    sdum=key;
    if (strutils::has_beginning(sdum,strutils::to_lower(name))) {
	name_value_table.found(sdum,p);
	auto it=list.end();
	list.insert(it,p.data->values.begin(),p.data->values.end());
    }
  }
  return list;
}

std::string QueryString::value(std::string name,const char *separator)
{
  std::list<std::string> list;
  std::string s;

  list=values(name);
  for (auto& item : list) {
    if (s.length() > 0) {
	s+=separator;
    }
    s+=item;
  }
  return s;
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
