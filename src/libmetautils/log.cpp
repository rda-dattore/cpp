#include <metadata.hpp>
#include <utils.hpp>

namespace metautils {

void log_info(std::string message,std::string caller,std::string user)
{
  std::ofstream log;
  std::stringstream output,error;

  log.open((directives.rdadata_home+"/logs/md/"+caller+"_log").c_str(),std::fstream::app);
  log << "user " << user << "/" << directives.host << " at " << dateutils::current_date_time().to_string() << " running with '" << args.args_string << "':" << std::endl;
  log << message << std::endl;
  log.close();
  unixutils::mysystem2("/bin/chmod 664 "+directives.rdadata_home+"/logs/md/"+caller+"_log",output,error);
}

void log_error(std::string error,std::string caller,std::string user,bool no_exit)
{
  size_t idx=0;
  if (error.substr(0,2) == "-q") {
    idx=2;
  }
  if (getenv("QUERY_STRING") != nullptr) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    if (idx == 0) {
	std::cout << "Error: ";
    }
    std::cout << error.substr(idx) << std::endl;
  }
  else {
    if (idx == 0) {
	std::cerr << "Error: ";
    }
    std::cerr << error.substr(idx) << std::endl;
  }
  log_info(error.substr(idx),caller,user);
  if (!no_exit) {
    exit(1);
  }
}

void log_warning(std::string warning,std::string caller,std::string user)
{
//  std::cerr << "Warning: " << warning << std::endl;
  log_info(warning,caller,user);
}

} // end namespace metautils
