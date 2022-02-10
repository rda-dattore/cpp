#include <string>
#include <list>
#include <strutils.hpp>
#include <web/web.hpp>

std::string rda_username()
{
  std::string rda_username_;
  char *env;
  if ( (env=getenv("HTTP_COOKIE")) != NULL) {
    rda_username_=env;
    size_t idx;
    if ( (idx=rda_username_.find("duser=")) != std::string::npos) {
	rda_username_=rda_username_.substr(idx+6);
	if ( (idx=rda_username_.find(";")) != std::string::npos) {
	  rda_username_=rda_username_.substr(0,idx);
	}
	if ( (idx=rda_username_.find(":")) != std::string::npos) {
	  rda_username_=rda_username_.substr(0,idx);
	}
    }
    else {
	rda_username_="";
    }
  }
  return rda_username_;
}

std::string duser()
{
  return rda_username();
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
	accesses.emplace_back("<g>");
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
  for (const auto& access : accesses) {
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
    auto idx=value.find(name+"=");
    while (idx != std::string::npos && idx > 0 && value.substr(idx-2,2) != "; ") {
	idx=value.find(name+"=",idx+1);
    }
    if (idx != std::string::npos) {
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
