#include <string>
#include <list>
#include <strutils.hpp>
#include <web/web.hpp>
#include <MySQL.hpp>

std::string duser()
{
  std::string duser_;
  char *env;
  if ( (env=getenv("HTTP_COOKIE")) != NULL) {
    duser_=env;
    size_t idx;
    if ( (idx=duser_.find("duser=")) != std::string::npos) {
	duser_=duser_.substr(idx+6);
	if ( (idx=duser_.find(";")) != std::string::npos) {
	  duser_=duser_.substr(0,idx);
	}
	if ( (idx=duser_.find(":")) != std::string::npos) {
	  duser_=duser_.substr(0,idx);
	}
    }
    else {
	duser_="";
    }
  }
  return duser_;
}

std::string iuser()
{
  std::string iuser_;
  char *env;
  if ( (env=getenv("HTTP_COOKIE")) != NULL) {
    iuser_=env;
    size_t idx;
    if ( (idx=iuser_.find("iuser=")) != std::string::npos) {
	iuser_=iuser_.substr(idx+6);
	iuser_=iuser_.substr(0,iuser_.find("@"));
    }
    else {
	iuser_="";
    }
  }
  if (iuser_.empty()) {
    iuser_=duser();
    if (iuser_.length() > 0) {
	MySQL::Server server;
	server.connect("rda-db.ucar.edu","dssdb","dssdb","dssdb");
	if (server) {
	  MySQL::LocalQuery query("acode","ruser_access","email = '"+iuser_+"' and acode = 'i'");
	  if (query.submit(server) != 0 || query.num_rows() == 0) {
	    iuser_="";
	  }
	}
    }
  }
  return iuser_;
}

std::string session_ID()
{
  std::string session_ID_;
  char *env;
  if ( (env=getenv("HTTP_COOKIE")) != NULL) {
    session_ID_=env;
    size_t idx;
    if ( (idx=session_ID_.find("sess=")) != std::string::npos) {
	session_ID_=session_ID_.substr(idx+5);
	if ( (idx=session_ID_.find(";")) != std::string::npos) {
	  session_ID_=session_ID_.substr(0,idx);
	}
	if (session_ID_.length() != 20) {
	  session_ID_="";
	}
    }
    else {
	session_ID_="";
    }
  }
  return session_ID_;
}

std::string twiki_name()
{
  std::string twiki_name_;
  char *env;
  if ( (env=getenv("HTTP_COOKIE")) != NULL) {
    twiki_name_=env;
    size_t idx;
    if ( (idx=twiki_name_.find("twikiname=")) != std::string::npos) {
	twiki_name_=twiki_name_.substr(idx+6);
	if ( (idx=twiki_name_.find(";")) != std::string::npos) {
	  twiki_name_=twiki_name_.substr(0,idx);
	}
	if ( (idx=twiki_name_.find(":")) != std::string::npos) {
	  twiki_name_=twiki_name_.substr(0,idx);
	}
    }
    else {
	twiki_name_="";
    }
  }
  return twiki_name_;
}

std::list<std::string> accesses_from_cookie()
{
  std::list<std::string> accesses;
  char *env;
  std::string cookie;
  if ( (env=getenv("HTTP_COOKIE")) != NULL) {
    cookie=env;
  }
  if (!cookie.empty() > 0) {
    size_t idx;
    if ( (idx=cookie.find("duser=")) != std::string::npos) {
	accesses.push_back("<g>");
	cookie=cookie.substr(idx+6);
	if ( (idx=cookie.find(";")) != std::string::npos) {
	  cookie=cookie.substr(0,idx);
	}
	auto access_list=strutils::split(cookie,":");
	access_list.pop_front();
	for (const auto& access : access_list) {
	  accesses.emplace_back(access);
	}
    }
  }
  return accesses;
}

bool has_access(std::string acode)
{
  auto accesses=accesses_from_cookie();
  for (auto access : accesses) {
    if (access == acode) {
	return true;
    }
  }
  return false;
}

std::string value_from_cookie(std::string name)
{

  std::string value;
  char *env;
  if ( (env=getenv("HTTP_COOKIE")) != NULL) {
    value=env;
    size_t idx;
    if ( (idx=value.find(name+"=")) != std::string::npos) {
	value=value.substr(idx+name.length()+1);
	if ( (idx=value.find(";")) != std::string::npos) {
	  value=value.substr(0,idx);
	}
	if ( (idx=value.find(":")) != std::string::npos) {
	  value=value.substr(0,idx);
	}
    }
    else {
	value="";
    }
  }
  return value;
}
