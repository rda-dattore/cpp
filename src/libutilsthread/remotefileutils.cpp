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
  replace_all(URL, "%", "%25");
  curl_easy_setopt(c, CURLOPT_URL, URL.c_str());
  curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0);
  auto n = 0;
  while (curl_easy_perform(c) != CURLE_OK && n < 3) {
    std::this_thread::sleep_for(std::chrono::seconds(15));
    ++n;
  }
  fclose(fp);
  long rcode;
  curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &rcode);
  if (rcode != 200 || n == 3) {
    remove(f.c_str());
    f = "";
  }
  return f;
}

int gdex_upload_dir(string directory, string relative_path, string remote_path,
    string api_key, string& error, string opts) {
  error = "";
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
  if (relative_path == ".") {
    relative_path = "";
  }
  stringstream oss, ess;
  mysystem2("/bin/bash -c 'list=($(ls " + directory + "/" + relative_path +
      ")); for file in ${list[@]}; do /glade/u/home/dattore/bin/gdex_upload " +
      directory + "/" + relative_path + "$file " + remote_path + "/" +
      relative_path + " " + opts + "; done'", oss, ess);
  error = oss.str();
  replace_all(error, "Success.", "");
  if (!error.empty()) {
    return -1;
  }
  return 0;
}

int gdex_unlink(string remote_filepath, string api_key, string& error) {
  int i = 0; // return value
  error = "";
  stringstream oss, ess;
  mysystem2("/bin/bash -c 'curl -k -s -w \" %{http_code}\" -H \"API-Key: " +
      api_key + "\" -d \"path=" + remote_filepath + "\" "
      "\"https://api.gdex.ucar.edu/unlink/\"'", oss, ess);
  auto code = oss.str().substr(oss.str().length() - 3);
  if (code != "200") {
    i = -1;
    error = oss.str().substr(0, oss.str().length() - 4);
  }
  return i;
}

bool exists_on_server(string host_name, string remote_path) {
  stringstream oss, ess;
  if (remote_path.front() != '/' && host_name.back() != '/') {
    host_name += "/";
  }
  mysystem2("/bin/sh -c 'curl -k -I -s -w \"%{http_code}\" \"https://" +
     host_name + remote_path + "\"'", oss, ess);
  if (!oss.str().empty()) {
    auto code = oss.str().substr(oss.str().length() - 3);
    if (code == "200") {
      return true;
    }
  }
  return false;
}

} // end namespace unixutils
