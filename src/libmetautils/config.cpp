#include <unistd.h>
#include <sys/stat.h>
#include <metadata.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <myerror.hpp>
#include <myexception.hpp>

using std::regex;
using std::regex_search;
using std::string;
using std::unordered_map;
using strutils::itos;
using strutils::split;
using strutils::token;
using strutils::trimmed;

namespace metautils {

const string GLOBAL_CONFIG_HOME = "/glade/u/home/dattore/conf";
const string GLOBAL_DECS_ROOT = "/ncar/rda/setuid";
const string LOCAL_CONFIG_HOME = "/home/dattore/conf";
const string LOCAL_DECS_ROOT = "/usr/local/decs";

bool configuration_error(string directive) {
  myerror = "configuration error on '" + directive + "' line";
  return false;
}

bool read_config(string caller, string user, bool restrict_to_user_rdadata) {
  directives.host = unixutils::host_name();
  if (restrict_to_user_rdadata && geteuid() != 15968 && directives.host !=
      "rda-web-prod.ucar.edu" && directives.host != "rda-web-test.ucar.edu" &&
      directives.host != "rda-web-dev.ucar.edu" && geteuid() != 8342) {
    throw my::NotAuthorized_Error(caller + ": unauthorized uid: " + itos(
        geteuid()));
  }
  directives.server_root = "/" + token(directives.host, ".", 0);
  struct stat buf;
  auto is_singularity = false;
  if (stat("/.singularity.d", &buf) == 0) {
    is_singularity = true;
  }
  if (stat(LOCAL_DECS_ROOT.c_str(), &buf) == 0) {
    directives.decs_root = LOCAL_DECS_ROOT;
  } else if (stat(GLOBAL_DECS_ROOT.c_str(), &buf) == 0) {
    directives.decs_root = GLOBAL_DECS_ROOT;
  } else if (is_singularity) {
    directives.decs_root = "/usr/local";
  }
  if (directives.decs_root.empty()) {
    myerror = "unable to locate decs root directory on " + directives.host;
    return false;
  }
  if (stat("/local/decs/bin/cmd_util_pg", &buf) == 0) {
    directives.local_root = "/local/decs";
  } else if (stat("/usr/local/decs/bin/cmd_util_pg", &buf) == 0) {
    directives.local_root = "/usr/local/decs";
  } else if (stat("/ncar/rda/setuid/bin/cmd_util_pg", &buf) == 0) {
    directives.local_root = "/ncar/rda/setuid";
  } else if (is_singularity) {
    directives.local_root = "/usr/local";
  }
  if (directives.local_root.empty()) {
    myerror = "unable to locate DECS local directory on " + directives.host;
    return false;
  }
  std::ifstream ifs;
  if (stat(LOCAL_CONFIG_HOME.c_str(), &buf) == 0) {
    ifs.open((LOCAL_CONFIG_HOME + "/metautils_pg.conf").c_str());
  } else if (stat(GLOBAL_CONFIG_HOME.c_str(), &buf) == 0) {
    ifs.open((GLOBAL_CONFIG_HOME + "/metautils_pg.conf").c_str());
  }
  if (!ifs.is_open()) {
    throw my::NotFound_Error("unable to open metautils_pg.conf");
  }
  unordered_map<string, string> bin_map;
  char line[256];
  ifs.getline(line,256);
  while (!ifs.eof()) {
    if (line[0] != '#') {
      auto conf_parts = split(trimmed(line));
      if (conf_parts[0] == "service") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        if (conf_parts[1] != "on") {
          myerror = "service is currently unavailable";
          return false;
        }
      } else if (conf_parts[0] == "rdadataHome") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.rdadata_home = conf_parts[1];
      } else if (conf_parts[0] == "rdadataBinSubDirectory") {
        bin_map.emplace(conf_parts[1], conf_parts[2]);
      } else if (conf_parts[0] == "webServer") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.web_server = conf_parts[1];
      } else if (conf_parts[0] == "databaseServer") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.database_server = conf_parts[1];
      } else if (conf_parts[0] == "rdadbUsername") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.rdadb_username = conf_parts[1];
      } else if (conf_parts[0] == "rdadbPassword") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.rdadb_password = conf_parts[1];
      } else if (conf_parts[0] == "metadbUsername") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.metadb_username = conf_parts[1];
      } else if (conf_parts[0] == "metadbPassword") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.metadb_password = conf_parts[1];
      } else if (conf_parts[0] == "wagtailUsername") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.wagtail_username = conf_parts[1];
      } else if (conf_parts[0] == "wagtailPassword") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.wagtail_password = conf_parts[1];
      } else if (conf_parts[0] == "tempPath") {
  std::cerr << conf_parts[0] << " " << conf_parts[1] << std::endl;
  auto status = stat(conf_parts[1], &buf);
  std::cerr << status << std::endl;
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        if (directives.temp_path.empty() && stat(conf_parts[1].c_str(), &buf) ==
            0) {
          directives.temp_path = conf_parts[1];
        }
      } else if (conf_parts[0] == "dataRoot") {
        if (conf_parts.size() != 3) {
          return configuration_error(conf_parts[0]);
        }
        directives.data_root = conf_parts[1];
        directives.data_root_alias = conf_parts[2];
      } else if (conf_parts[0] == "hpssRoot") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.hpss_root = conf_parts[1];
      } else if (conf_parts[0] == "metadataManager") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.metadata_manager = conf_parts[1];
      } else if (conf_parts[0] == "parameterMapPath") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.parameter_map_path = conf_parts[1];
      } else if (conf_parts[0] == "levelMapPath") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.level_map_path = conf_parts[1];
      } else if (conf_parts[0] == "singularityBind") {
        if (conf_parts.size() != 2) {
          return configuration_error(conf_parts[0]);
        }
        directives.singularity_binds.emplace_back(conf_parts[1]);
      }
    }
    ifs.getline(line, 256);
  }
  ifs.close();
  if (directives.temp_path.empty()) {
    throw my::BadOperation_Error("unable to set path for temporary files");
  }
  if (directives.decs_root == GLOBAL_DECS_ROOT) {
    if (is_singularity) {
      directives.decs_root = directives.rdadata_home;
      directives.decs_bindir = directives.decs_root + "/singularity";
    } else {
      for (const auto& e : bin_map) {
        if (regex_search(directives.host, regex(e.first))) {
          directives.decs_root = directives.rdadata_home;
          directives.decs_bindir = directives.decs_root + "/bin/" + e.second;
        }
      }
    }
  } else {
    directives.decs_bindir = directives.decs_root + "/bin";
  }
  if (directives.decs_bindir.empty()) {
    myerror = "'rdadataBinSubDirectory' not set for host '" + directives.host +
        "'";
    return false;
  }
  return true;
}

} // end namespace metautils
