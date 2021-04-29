#include <regex>
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
    error=error.substr(idx);
  }
  if (getenv("QUERY_STRING") != nullptr) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    if (idx == 0) {
	std::cout << "Error: ";
    }
    std::cout << error << std::endl;
  }
  else {
    if (idx == 0) {
	std::cerr << "Error: ";
    }
    std::cerr << error << std::endl;
  }
  if (!std::regex_search(error,std::regex("^Terminating"))) {
    log_info(error,caller,user);
  }
  if (!no_exit) {
    exit(1);
  }
}

void log_error2(std::string error,std::string reporter,std::string caller,std::string user,bool no_exit)
{
  if (getenv("QUERY_STRING") != nullptr) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    std::cout << error << std::endl;
  }
  else {
    std::cerr << reporter << ": " << error << std::endl;
  }

// set exit_status = 1 for logging
// set exit_status = 2 for no logging ("^Terminating" error)
  auto exit_status=2;
  if (!std::regex_search(error,std::regex("^Terminating"))) {
    log_info(reporter+": "+error,caller,user);
    exit_status=1;
  }
  if (!no_exit) {
    exit(exit_status);
  }
}

void log_warning(std::string warning,std::string caller,std::string user)
{
//  std::cerr << "Warning: " << warning << std::endl;
  log_info(warning,caller,user);
}

} // end namespace metautils
