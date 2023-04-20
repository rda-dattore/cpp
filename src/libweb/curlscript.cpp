#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <ftw.h>
#include <sys/stat.h>
#include <strutils.hpp>
#include <web/web.hpp>

using std::cout;
using std::endl;
using std::stoll;
using std::string;
using std::stringstream;
using std::unordered_set;
using std::vector;
using strutils::occurs;
using strutils::replace_all;
using strutils::split;

vector<string> curl_filelist;

extern "C" int find_curl_directory_files(const char *name, const struct stat64
    *data, int flag, struct FTW *ftw_struct) {
  if (flag == FTW_F) {
    string sname = name;
    if (sname.find("wget") == string::npos) {
      curl_filelist.emplace_back(sname);
    }
  }
  return 0;
}

void print_curl_head(string script_type, string& remark, string& cert_opt,
    string& opts, std::ostream *outs) {
  const string SERVER_NAME = getenv("SERVER_NAME");
  if (script_type == "csh") {
    remark = "#";
    opts = "$opts";
    *outs << "#! /bin/csh -f" << endl;
    *outs << "#" << endl;
    *outs << "# c-shell script to download selected files from " << SERVER_NAME
        << " using curl" << endl;
    *outs << "# NOTE: if you want to run under a different shell, make sure "
        "you change" << endl;
    *outs << "#       the 'set' commands according to your shell's syntax" <<
         endl;
    *outs << "# after you save the file, don't forget to make it executable" <<
         endl;
    *outs << "#   i.e. - \"chmod 755 <name_of_script>\"" << endl;
    *outs << "#" << endl;
  } else if (script_type == "bat") {
    remark = "::";
    opts = "%opts%";
    *outs << ":: DOS batch script to download selected files from " <<
        SERVER_NAME;
  }
  *outs << remark << " you can add cURL options here (progress bars, etc.)" <<
      endl;
  *outs << "set opts = \"\"" << endl;
}

void create_curl_script(const vector<string>& filelist, string server, string
    directory, string dsnum, string script_type, string parameters, string
    level, string product, string start_date, string end_date) {
  const string SERVER_NAME = getenv("SERVER_NAME");
  unordered_set<string> selected_parameters_set;
  if (!parameters.empty()) {
    auto sp = split(parameters, "[!]");
    for (const auto& p : sp) {
      if (!p.empty()) {
        auto sp2 = split(p, ",");
        for (const auto& p2 : sp2) {
          selected_parameters_set.emplace(p2);
        }
      }
    }
  }
  unordered_set<string> selected_levels_set;
  if (!level.empty()) {
    auto sp = split(level, "[!]");
    for (const auto& p : sp) {
      if (!p.empty()) {
        auto sp2 = split(p, ",");
        for (const auto& p2 : sp2) {
          selected_levels_set.emplace(p2);
        }
      }
    }
  }
  unordered_set<string> selected_products_set;
  if (!product.empty()) {
    auto sp = split(product, ",");
    for (const auto& p : sp) {
      selected_products_set.emplace(p);
    }
  }
  if (!start_date.empty() && !end_date.empty()) {
    start_date = dateutils::string_date_to_ll_string(start_date);
    end_date = dateutils::string_date_to_ll_string(end_date);
  }
  string remark, cert_opt, opts;
  print_curl_head(script_type, remark, cert_opt, opts, &cout);
  cout << remark << endl;
  cout << remark << " download the file(s)" << endl;
  for (const auto& file : filelist) {
    auto local_file = file;
    auto idx = local_file.find("/");
    while (idx != string::npos) {
      local_file = local_file.substr(idx + 1);
      idx = local_file.find("/");
    }
    string inv_file = "/usr/local/www/server_root/web/datasets/ds" + dsnum +
        "/metadata/inv/" + file + ".GrML_inv";
    struct stat buf;
    if (stat(inv_file.c_str(), &buf) != 0) {
      stringstream iss;
      system(("/usr/local/www/server_root/web/cgi-bin/datasets/inventory " +
          dsnum + " GrML " + file + " 2>&1 1>/dev/null").c_str());
    }
    if (stat(inv_file.c_str(), &buf) == 0 && (!parameters.empty() || !level.
        empty() || !product.empty() || (!start_date.empty() && !end_date.
        empty()))) {
      unordered_set<string> parameter_set, level_set, product_set;
      vector<long long> range_start, range_end;
      long long last_match = -1;
      auto num_lines = 0;
      auto num_matches = 0;
      std::ifstream ifs(inv_file.c_str());
      if (ifs.is_open()) {
        char line[32768];
        ifs.getline(line,32768);
        string sline = line;
        while (!ifs.eof() && sline != "-----") {
          auto sp = split(sline, "<!>");
          if (sp[0] == "P") {
            if (selected_parameters_set.find(sp[2]) != selected_parameters_set.
                end()) {
              parameter_set.emplace(sp[1]);
            }
          } else if (sp[0] == "L") {
            string key;
            if (occurs(sp[2], ":") == 2) {
              auto sp2 = split(sp[2], ":");
              key = sp2[0] + ":" + sp2[2] + "," + sp2[1];
            } else {
              key = sp[2];
            }
            if (selected_levels_set.find(key) != selected_levels_set.end()) {
              level_set.emplace(sp[1]);
            }
          } else if (sp[0] == "U") {
            if (selected_products_set.find(sp[2]) != selected_products_set.
                end()) {
              product_set.emplace(sp[1]);
            }
          }
          ifs.getline(line, 32768);
          if (!ifs.eof()) {
            sline = line;
          }
        }
        ifs.getline(line, 32768);
        auto start = -1;
        auto end = 0;
        while (!ifs.eof()) {
          string sline = line;
          ++num_lines;
          auto sp = split(sline, "|");
          auto matched = true;
          if (!parameter_set.empty() && parameter_set.find(sp[6]) ==
              parameter_set.end()) {
            matched = false;
          }
          if (matched && !level.empty() && level_set.find(sp[5]) == level_set.
              end()) {
            matched = false;
          }
          if (matched && !product.empty() && product_set.find(sp[3]) ==
              product_set.end()) {
            matched = false;
          }
          if (matched && !start_date.empty() && !end_date.empty() && (sp[2] <
              start_date || sp[2] > end_date)) {
            matched = false;
          }
          if (matched) {
            auto byte_pos = stoll(sp[0]);
            if (byte_pos != last_match) {
              if (start < 0) {
                start = byte_pos;
                end = start + stoll(sp[1]) - 1;
              } else {
                if (byte_pos != (end+1)) {
                  range_start.emplace_back(start);
                  range_end.emplace_back(end);
                  start = byte_pos;
                }
                end = byte_pos + stoll(sp[1]) - 1;
              }
              last_match = byte_pos;
            }
            ++num_matches;
          }
          ifs.getline(line, 32768);
        }
        ifs.close();
        range_start.emplace_back(start);
        range_end.emplace_back(end);
      }
      if (!range_start.empty() && num_matches < num_lines) {
        cout << "curl " << opts << " -k -r ";
        for (size_t n = 0; n < range_start.size(); ++n) {
          if (n > 0) {
            cout << ",";
          }
          cout << range_start[n] << "-" << range_end[n];
        }
        replace_all(local_file,".tar","");
        if (server.empty()) {
          cout << " https://" << SERVER_NAME;
        } else {
          cout << " " << server;
        }
        if (!directory.empty()) {
          cout << directory;
        }
        cout << "/" << file << " -o " << local_file << ".subset" << endl;
      } else {
        cout << "curl " << opts << " -k";
        if (server.empty()) {
          cout << " https://" << SERVER_NAME;
        } else {
          cout << " " << server;
        }
        if (!directory.empty()) {
          cout << directory;
        }
        cout << "/" << file << " -o " << local_file << endl;
      }
    } else {
      cout << "curl " << opts << " -k";
      if (server.empty()) {
        cout << " https://" << SERVER_NAME;
      } else {
        cout << " " << server;
      }
      if (!directory.empty()) {
        cout << directory;
      }
      cout << "/" << file << " -o " << local_file << endl;
    }
  }
}

void create_curl_script(const vector<string>& filelist, string server, string
    directory, string script_type, std::ofstream *ostream) {
  const string SERVER_NAME = getenv("SERVER_NAME");
  std::ostream *outs;
  if (ostream == nullptr) {
    outs = &cout;
  } else {
    outs = ostream;
  }
  string remark, cert_opt, opts;
  print_curl_head(script_type, remark, cert_opt, opts, outs);
  *outs << remark << endl;
  *outs << remark << " download the file(s)" << endl;
  *outs << remark << " NOTE:  if you get 403 Forbidden errors when downloading the data files, check" << endl;
  *outs << remark << "        the contents of the file 'auth_status.rda.ucar.edu'" << endl;
  const vector<string> *flist;
  if (filelist.size() == 0) {
    nftw64(directory.c_str(), find_curl_directory_files, 1, 0);
    flist = &curl_filelist;
  } else {
    flist = &filelist;
  }
  for (const auto& file : *flist) {
    *outs << "curl " << cert_opt << " " << opts;
    if (server.empty()) {
      *outs << " https://" << SERVER_NAME;
    } else {
      *outs << " " << server;
    }
    if (flist == &filelist && !directory.empty()) {
      *outs << directory;
    }
    *outs << "/" << file << " -o " << file << endl;
  }
}
