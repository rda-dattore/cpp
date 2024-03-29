#include <iostream>
#include <string>
#include <list>
#include <web/web.hpp>

using std::cout;
using std::endl;
using std::string;
using std::vector;

void create_python_script(const vector<string>& filelist, string server, string
    directory,string script_type) {
  const string SERVER_NAME = getenv("SERVER_NAME");
  string remark;
  if (script_type == "csh") {
    remark="#";
    cout << "#! /usr/bin/env python3" << endl;
    cout << "#" << endl;
    cout << "# python script to download selected files from " << SERVER_NAME <<
        endl;
    cout << "# after you save the file, don't forget to make it executable" <<
        endl;
    cout << "#   i.e. - \"chmod 755 <name_of_script>\"" << endl;
  } else if (script_type == "bat") {
    remark="::";
    cout << ":: python script to download selected files from " << SERVER_NAME
        << endl;
  }
  cout << remark << endl;
  cout << "import requests" << endl;
  cout << remark << endl;
  cout << "files = [" << endl;
  for (const auto& file : filelist) {
    cout << "    \"" << file << "\"," << endl;
  }
  cout << "]" << endl;
  cout << remark << endl;
  cout << remark << " download the data file(s)" << endl;
  cout << "for file in files:" << endl;
  cout << "    idx = file.rfind(\"/\")" << endl;
  cout << "    if (idx > 0):" << endl;
  cout << "        ofile = file[idx+1:]" << endl;
  cout << "    else:" << endl;
  cout << "        ofile = file" << endl;
  cout << endl;
  cout << "    response = requests.get(\"";
  if (server.empty()) {
    cout << "http://" << SERVER_NAME;
  } else {
    cout << server;
  }
  if (!directory.empty()) {
    cout << directory;
  }
  cout << "/\" + file)" << endl;
  cout << "    with open(ofile, \"wb\") as f:" << endl;
  cout << "        f.write(response.content)" << endl;
}

void create_python_script(const vector<string>& filelist) {
  create_python_script(filelist, "", "", "csh");
}
