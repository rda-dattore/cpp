#include <regex>
#include <strutils.hpp>
#include <utils.hpp>

namespace unixutils {

std::string hsi_command()
{
  std::stringstream oss,ess;
  unixutils::mysystem2("/bin/sh -c \"which hsi\"",oss,ess);
  std::string command;
  if ((!ess.str().empty() || std::regex_search(oss.str(),std::regex("not found")))) {
    if (std::regex_search(unixutils::host_name(),std::regex("^(yslogin|geyser|cheyenne)"))) {
	unixutils::mysystem2("/bin/tcsh -c \"module load ncarenv; which hsi\"",oss,ess);
	if (!oss.str().empty() && !std::regex_search(oss.str(),std::regex("not found"))) {
	  command=oss.str();
	}
    }
    else {
	unixutils::mysystem2("/bin/tcsh -c \"which hsi\"",oss,ess);
	if (ess.str().empty()) {
	  command=oss.str();
	}
    }
  }
  else {
    command=oss.str();
    auto sp=strutils::split(command);
    if (sp.size() > 0) {
	command=sp[0];
    }
  }
  strutils::trim(command);
  return command;
}

} // end namespace unixutils
