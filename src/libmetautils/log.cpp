#include <regex>
#include <metadata.hpp>
#include <utils.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::regex;
using std::regex_search;
using std::string;
using std::stringstream;

namespace metautils {

void log_info(string message, string caller, string user) {

  std::ofstream log((directives.rdadata_home + "/logs/md/" + caller + "_log").
      c_str(), std::fstream::app);
  log << "user " << user << "/" << directives.host << " at " << dateutils::
      current_date_time().to_string() << " running with '" << args.args_string
      << "':" << endl;
  log << message << endl;
  log.close();
  stringstream oss, ess;
  unixutils::mysystem2("/bin/chmod 664 " + directives.rdadata_home + "/logs/md/"
      + caller + "_log", oss, ess);
}

void log_error(string error, string caller, string user, bool no_exit) {
  size_t idx = 0;
  if (error.substr(0, 2) == "-q") {
    idx = 2;
    error = error.substr(idx);
  }
  if (getenv("QUERY_STRING") != nullptr) {
    cout << "Content-type: text/plain" << endl << endl;
    if (idx == 0) {
	cout << "Error: ";
    }
    cout << error << endl;
  } else {
    if (idx == 0) {
	cerr << "Error: ";
    }
    cerr << error << endl;
  }
  if (!regex_search(error, regex("^Terminating"))) {
    log_info(error, caller, user);
  }
  if (!no_exit) {
    exit(1);
  }
}

void log_error2(string error, string reporter, string caller, string user, bool
    no_exit) {
  if (getenv("QUERY_STRING") != nullptr) {
    cout << "Content-type: text/plain" << endl << endl;
    cout << error << endl;
  } else {
    cerr << reporter << ": " << error << endl;
  }

  // set s = 1 for logging
  // set s = 2 for no logging ("^Terminating" error)
  auto s = 2;
  if (args.dsnum != "999.9" && args.dsnum != "test" && !regex_search(error,
      regex("^Terminating"))) {
    log_info(reporter + ": " + error, caller, user);
    s = 1;
  }
  if (!no_exit) {
    exit(s);
  }
}

void log_error2(string error, string reporter, const UtilityIdentification&
    util_ident, bool no_exit) {
  log_error2(error, reporter, util_ident.m_caller, util_ident.m_user, no_exit);
}

void log_warning(string warning, string caller, string user) {
//  cerr << "Warning: " << warning << endl;
  log_info(warning, caller, user);
}

} // end namespace metautils
