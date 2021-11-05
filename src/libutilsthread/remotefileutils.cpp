#include <iostream>
#include <string>
#include <sstream>
#include <regex>
#include <thread>
#include <unistd.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <datetime.hpp>
#include <strutils.hpp>
#include <utils.hpp>

using std::list;
using std::string;
using std::stringstream;
using strutils::itos;
using strutils::replace_all;
using strutils::substitute;
using strutils::token;
using unixutils::hosts_loaded;

namespace unixutils {

string remote_file(string server_name, string local_tmpdir, string
    remote_filename, string& error) {
  string f; // return value
  auto p = popen(("rsync -aLq " + server_name + ":" + remote_filename + " " +
      local_tmpdir).c_str(), "r");
  auto stat = pclose(p);
  if (stat == 0) {
    f = local_tmpdir + remote_filename.substr(remote_filename.rfind("/"));
  } else {
    error = "rsync returned status " + itos(stat);
  }
  return f;
}

string remote_web_file(string URL, string local_tmpdir) {
  string f; // return value
  auto c = curl_easy_init();
  curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, nullptr);
  f = local_tmpdir + URL.substr(URL.rfind("/"));
  auto fp = fopen(f.c_str(), "w");
  curl_easy_setopt(c, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(c, CURLOPT_URL, URL.c_str());
  auto n = 0;
  while (curl_easy_perform(c) != CURLE_OK && n < 3) {
    std::this_thread::sleep_for(std::chrono::seconds(15));
    ++n;
  }
  fclose(fp);
  if (n == 3) {
    remove(f.c_str());
    f = "";
  }
  return f;
}

int rdadata_sync(string directory, string relative_path, string remote_path,
    string rdadata_home, string& error) {
  int i = 0; // return value
  if (directory.empty()) {
    error = "missing directory";
    return -1;
  }
  if (relative_path.empty()) {
    error = "nothing to copy";
    return -1;
  }
  if (remote_path.empty()) {
    error = "unknown destination";
    return -1;
  }
  list<string> hlst;
  error = "";
  if (!hosts_loaded(hlst, rdadata_home)) {
    error = "rdadata_sync() unable to get list of host names";
    return -1;
  }
  if (setreuid(15968, 15968) != 0) {
    error = "unable to set real and effective uids to 'rdadata'";
    return -1;
  }
  for (const auto& h : hlst) {
    auto e = retry_command("/bin/sh -c 'cd " + directory + "; rsync -rptgD "
        "--relative -e __INNER_QUOTE__ssh -i " + rdadata_home + "/.ssh/" + h +
        "-sync_rdadata_rsa -l rdadata__INNER_QUOTE__ " + relative_path + " " + h
        + ".ucar.edu:" + remote_path + "'", 3);
    if (!e.empty()) {
      if (!error.empty()) {
        error += ", ";
      }
      error += "rsync error on '" + h + "': *'" + e + "'*";
      i = -1;
    }
  }
  return i;
}

int rdadata_unsync(string remote_filename, string local_tmpdir, string
    rdadata_home, string& error) {
// special tokens in remote_filename
//  __HOST__ will be replaced with hosts.getCurrent()
//

  int i = 0; // return value
  error = "";
  list<string> hlst;
  if (!hosts_loaded(hlst, rdadata_home)) {
    error = "rdadata_unsync() unable to get list of host names";
    return -1;
  }
  for (auto& h : hlst) {
    auto f = remote_filename;
    replace_all(f, "__HOST__", h);
    auto idx = f.rfind("/");
    auto p = f.substr(0, idx);
    f = f.substr(idx + 1);
//    auto e = retry_command("/bin/sh -c \"" + rdadata_home + "/bin/" + h +
//        "-sync -d " + f + "\"", 3);
    auto e = retry_command("/bin/sh -c 'rsync -r -e __INNER_QUOTE__ssh -i " +
        rdadata_home + "/.ssh/" + h + "-sync_rdadata_rsa -l "
        "rdadata__INNER_QUOTE__ --delete --include=" + f +
        " __INNER_QUOTE__--exclude=*__INNER_QUOTE__ " + local_tmpdir + "/ " + h
        + ".ucar.edu:" + p + "'", 3);
    if (e.length() > 0) {
      if (!error.empty()) {
        error += ", ";
      }
//      error += h + "-sync error: *'" + e + "'*";
      error += "rsync error on '" + h + "': *'" + e + "'*";
      i = -1;
    }
  }
  return i;
}

int rdadata_sync_from(string remote_filename, string local_filename, string
     rdadata_home, stringstream& ess) {
// special tokens in remote_filename
//  __HOST__ will be replaced with hosts.getCurrent()
//

  int i = 0; //return value
  list<string> hlst;
  if (!hosts_loaded(hlst, rdadata_home)) {
    ess << "rdadata_sync_from() unable to get list of host names" << std::endl;
    return -1;
  }
  for (auto& h : hlst) {
    string f = remote_filename;
    replace_all(f, "__HOST__", h);
    struct stat b;
    stringstream oss;
    if (mysystem2("/bin/sh -c \"" + rdadata_home + "/bin/" + h + "-sync -g " + f
        + " " + local_filename + "\"", oss, ess) == 0 && stat(local_filename
        .c_str(), &b) == 0) {
      break;
    } else {
      i = -1;
    }
  }
  return i;
}

bool exists_on_server(string host_name, string remote_filename, string
     rdadata_home) {
  stringstream oss, ess;
  mysystem2("/bin/sh -c 'rsync --list-only -e __INNER_QUOTE__ssh -i " +
      rdadata_home + "/.ssh/" + token(host_name, ".", 0) + "-sync_rdadata_rsa "
      "-l rdadata__INNER_QUOTE__ " + host_name + ":" + remote_filename + "'",
      oss, ess);
  if (!oss.str().empty()) {
    if (ess.str().empty()) {
      return true;
    }
    return false;
  }
  return false;
}

} // end namespace unixutils
