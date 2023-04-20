#include <iostream>
#include <fstream>
#include <ftw.h>
#include <string>
#include <strutils.hpp>
#include <web/web.hpp>

using std::cout;
using std::endl;
using std::string;
using std::vector;

vector<string> wget_filelist;

extern "C" int getWgetDirectoryFiles(const char *name, const struct stat64
    *data, int flag, struct FTW *ftw_struct) {
  if (flag == FTW_F) {
    string sname = name;
    if (sname.find("wget") == string::npos) {
        wget_filelist.push_back(sname);
    }
  }
  return 0;
}

void create_wget_script(const vector<string>& filelist, string server, string
    directory, string script_type, std::ofstream *ostream) {
  const string SERVER_NAME = getenv("SERVER_NAME");
  std::ostream *outs;
  if (ostream == nullptr) {
    outs = &cout;
  } else {
    outs = ostream;
  }
  string remark, cert_opt, opts;
  if (script_type == "csh") {
    remark = "#";
    cert_opt = "$cert_opt";
    opts = "$opts";
    *outs << "#! /bin/csh -f" << endl;
    *outs << "#" << endl;
    *outs << "# c-shell script to download selected files from " << SERVER_NAME
        << " using Wget" << endl;
    *outs << "# NOTE: if you want to run under a different shell, make sure "
        "you change" << endl;
    *outs << "#       the 'set' commands according to your shell's syntax" <<
        endl;
    *outs << "# after you save the file, don't forget to make it executable" <<
        endl;
    *outs << "#   i.e. - \"chmod 755 <name_of_script>\"" << endl;
  } else if (script_type == "bat") {
    remark = "::";
    cert_opt = "%cert_opt%";
    opts = "%opts%";
    *outs << ":: DOS batch script to download selected files from " <<
        SERVER_NAME << " using Wget" << endl;
  }
  *outs << remark << endl;
  *outs << remark << " Experienced Wget Users: add additional command-line "
      "flags here" << endl;
  *outs << remark << "   Use the -r (--recursive) option with care" << endl;
  if (script_type == "csh") {
    *outs << "set opts = \"-N\"" << endl;
  } else if (script_type == "bat") {
    *outs << "set opts=-N" << endl;
  }
  *outs << remark << endl;
  if (script_type == "csh") {
    *outs << "set cert_opt = \"\"" << endl;
  } else if (script_type == "bat") {
    *outs << "set cert_opt=" << endl;
  }
  *outs << remark << " If you get a certificate verification error (version 1.10 or higher)," << endl;
  *outs << remark << " uncomment the following line:" << endl;
  if (script_type == "csh") {
    *outs << remark << "set cert_opt = \"--no-check-certificate\"" << endl;
  } else if (script_type == "bat") {
    *outs << remark << "set cert_opt=--no-check-certificate" << endl;
  }
  *outs << remark << endl;
  *outs << remark << " download the file(s)" << endl;
  const vector<string> *flist;
  if (filelist.size() == 0) {
    nftw64(directory.c_str(), getWgetDirectoryFiles, 1, 0);
    flist = &wget_filelist;
  } else {
    flist = &filelist;
  }
  for (const auto& file : *flist) {
    *outs << "wget " << cert_opt << " " << opts;
    if (server.empty()) {
      *outs << " https://" << SERVER_NAME;
    } else {
      *outs << " " << server;
    }
    if (flist == &filelist && directory.length() > 0) {
      *outs << directory;
    }
    *outs << "/" << file << endl;
  }
}

void create_wget_script(const vector<string>& filelist) {
  create_wget_script(filelist, "", "", "csh");
}
