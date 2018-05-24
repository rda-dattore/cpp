#include <string>
#include <list>
#include <strutils.hpp>
#include <web/web.hpp>
#include <MySQL.hpp>

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
