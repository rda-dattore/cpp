#ifndef CONFIG_H
#define   CONFIG_H

#include <metadata.hpp>
#include <web/login.hpp>
#include <web/web.hpp>

namespace login {

extern bool read_config(Directives& directives);

} // end namespace login

namespace metautils {

extern bool read_config(std::string caller,std::string user,std::string args_string,bool restrict_to_user_rdadata = true);

} // end namespace metautils

namespace unixutils {

bool load_hosts(std::list<std::string>& hosts,std::string rdadata_home);

} // end namespace unixutils

namespace webutils {

extern bool read_config(Directives& directives);

} // end namespace webutils

#endif
