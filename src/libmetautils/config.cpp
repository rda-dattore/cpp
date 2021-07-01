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

const string GLOBAL_CONFIG_HOME = "/gpfs/u/home/dattore/conf";
const string GLOBAL_DECS_ROOT = "/ncar/rda/setuid";
const string LOCAL_CONFIG_HOME = "/home/dattore/conf";
const string LOCAL_DECS_ROOT = "/usr/local/decs";

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
  if (stat(LOCAL_DECS_ROOT.c_str(), &buf) == 0) {
    directives.decs_root = LOCAL_DECS_ROOT;
  } else if (stat(GLOBAL_DECS_ROOT.c_str(), &buf) == 0) {
    directives.decs_root = GLOBAL_DECS_ROOT;
  } else if (stat("/.singularity.d", &buf) == 0) {
    directives.decs_root = "/usr/local";
  }
  if (directives.decs_root.empty()) {
    myerror = "unable to locate decs root directory on " + directives.host;
    return false;
  }
  if (stat("/local/decs/bin/cmd_util", &buf) == 0) {
    directives.local_root = "/local/decs";
  } else if (stat("/usr/local/decs/bin/cmd_util", &buf) == 0) {
    directives.local_root = "/usr/local/decs";
  } else if (stat("/ncar/rda/setuid/bin/cmd_util", &buf) == 0) {
    directives.local_root = "/ncar/rda/setuid";
  } else if (stat("/.singularity.d", &buf) == 0) {
    directives.local_root = "/usr/local";
  }
  if (directives.local_root.empty()) {
    myerror = "unable to locate DECS local directory on " + directives.host;
    return false;
  }
  std::ifstream ifs;
  if (stat(LOCAL_CONFIG_HOME.c_str(), &buf) == 0) {
    ifs.open((LOCAL_CONFIG_HOME + "/metautils.conf").c_str());
  } else if (stat(GLOBAL_CONFIG_HOME.c_str(), &buf) == 0) {
    ifs.open((GLOBAL_CONFIG_HOME + "/metautils.conf").c_str());
  }
  if (!ifs.is_open()) {
    throw my::NotFound_Error("unable to open metautils.conf");
  }
  unordered_map<string, string> bin_map;
  char line[256];
  ifs.getline(line,256);
  while (!ifs.eof()) {
    if (line[0] != '#') {
      auto conf_parts = split(trimmed(line));
      if (conf_parts[0] == "service") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'service' line";
          return false;
        }
        if (conf_parts[1] != "on") {
          myerror = "service is currently unavailable";
          return false;
        }
      } else if (conf_parts[0] == "rdadataHome") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'rdadataHome' line";
          return false;
        }
        directives.rdadata_home = conf_parts[1];
      } else if (conf_parts[0] == "rdadataBinSubDirectory") {
        bin_map.emplace(conf_parts[1], conf_parts[2]);
      } else if (conf_parts[0] == "webServer") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'webServer' line";
          return false;
        }
        directives.web_server=conf_parts[1];
      } else if (conf_parts[0] == "databaseServer") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'databaseServer' line";
          return false;
        }
        directives.database_server=conf_parts[1];
      } else if (conf_parts[0] == "rdadbUsername") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'rdadbUsername' line";
          return false;
        }
        directives.rdadb_username=conf_parts[1];
      } else if (conf_parts[0] == "rdadbPassword") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'rdadbPassword' line";
          return false;
        }
        directives.rdadb_password=conf_parts[1];
      } else if (conf_parts[0] == "metadbUsername") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'metadbUsername' line";
          return false;
        }
        directives.metadb_username=conf_parts[1];
      } else if (conf_parts[0] == "metadbPassword") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'metadbPassword' line";
          return false;
        }
        directives.metadb_password = conf_parts[1];
      } else if (conf_parts[0] == "tempPath") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'tempPath' line";
          return false;
        }
        if (directives.temp_path.empty()) {
          if (stat(conf_parts[1].c_str(), &buf) == 0) {
            directives.temp_path = conf_parts[1];
          }
        }
      } else if (conf_parts[0] == "dataRoot") {
        if (conf_parts.size() != 3) {
          myerror = "configuration error on 'dataRoot' line";
          return false;
        }
        directives.data_root = conf_parts[1];
        directives.data_root_alias = conf_parts[2];
      } else if (conf_parts[0] == "hpssRoot") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'hpssRoot' line";
          return false;
        }
        directives.hpss_root=conf_parts[1];
      } else if (conf_parts[0] == "metadataManager") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'metadataManager' line";
          return false;
        }
        directives.metadata_manager = conf_parts[1];
      } else if (conf_parts[0] == "parameterMapPath") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'parameterMapPath' line";
          return false;
        }
        directives.parameter_map_path = conf_parts[1];
      } else if (conf_parts[0] == "levelMapPath") {
        if (conf_parts.size() != 2) {
          myerror = "configuration error on 'levelMapPath' line";
          return false;
        }
        directives.level_map_path = conf_parts[1];
      }
    }
    ifs.getline(line, 256);
  }
  ifs.close();
  if (directives.temp_path.empty()) {
    throw my::BadOperation_Error("unable to set path for temporary files");
  }
  if (directives.decs_root == GLOBAL_DECS_ROOT) {
    for (const auto& e : bin_map) {
      if (regex_search(directives.host, regex(e.first))) {
        directives.decs_root = directives.rdadata_home;
        directives.decs_bindir = directives.decs_root + "/bin/" + e.second;
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
