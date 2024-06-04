// FILE: sock.h

#ifndef SOCK_H
#define   SOCK_H

#include <stdio.h>
#include <stdlib.h> // provides size_t
#include <string>
#include <list>
#include <math.h>

namespace sockutils {

extern int authenticate_ncar_user(const char *username,const char *password);
extern int connect(std::string server_name, size_t port);
extern int validate_ncar_user(const char *username);

namespace linkchecker {

extern void fill_broken_links(std::string field,std::string baseURL,std::list<std::string>& links);

extern bool linkcheck(std::string url,size_t& status,std::string& response,std::string& error,int timeout = -1);
extern bool linkcheck(std::string url,std::string& error,int timeout = -1);

} // end namespace linkchecker

} // end namespace sockutils

#endif
