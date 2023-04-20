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
using std::string;
using std::stoll;
using std::stringstream;
using std::unordered_set;
using std::vector;
using strutils::occurs;
using strutils::replace_all;
using strutils::split;

struct StringEntry {
  StringEntry() : key() { }

  string key;
};

vector<string> curl_filelist;

extern "C" int find_curl_directory_files(const char *name, const struct stat64
    *data, int flag,struct FTW *ftw_struct) {
  if (flag == FTW_F) {
    string sname = name;
    if (sname.find("wget") == string::npos) {
      curl_filelist.push_back(sname);
    }
  }
  return 0;
}

void print_curl_head(string script_type, string& remark, string& cert_opt,
    string& opts, std::ostream *outs) {
  string server_name = getenv("SERVER_NAME");
  string user = duser();
  if (script_type == "csh") {
    remark = "#";
    opts = "$opts";
    *outs << "#! /bin/csh -f" << endl;
    *outs << "#" << endl;
    *outs << "# c-shell script to download selected files from <server_name> "
        "using curl" << endl;
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
    *outs << ":: DOS batch script to download selected files from "
        "<server_name> using curl" << endl;
  }
  *outs << remark << " you can add cURL options here (progress bars, etc.)" <<
      endl;
  *outs << "set opts = \"\"" << endl;
  *outs << remark << endl;
  if (user.empty()) {
    *outs << remark << " Replace \"xxx@xxx.xxx\" with your email address" <<
         endl;
    *outs << "set user = \"xxx@xxx.xxx\"" << endl;
  }
  *outs << remark << " Replace \"xxxxxx\" with your rda.ucar.edu password on "
      "the next uncommented line" << endl;
  if (script_type == "csh") {
    *outs << "# IMPORTANT NOTE:  If your password uses a special character "
        "that has special meaning" << endl;
    *outs << "#                  to csh, you should escape it with a backslash"
        << endl;
    *outs << "#                  Example:  set passwd = \"my\\!password\"" <<
        endl;
  }
  if (script_type == "csh") {
    *outs << "set passwd = 'xxxxxx'" << endl;
    *outs << "set num_chars = `echo \"$passwd\" |awk '{print length($0)}'`" <<
        endl;
    *outs << "@ num = 1" << endl;
    *outs << "set newpass = \"\"" << endl;
    *outs << "while ($num <= $num_chars)" << endl;
    *outs << "  set c = `echo \"$passwd\" |cut -b{$num}-{$num}`" << endl;
    *outs << "  if (\"$c\" == \"&\") then" << endl;
    *outs << "    set c = \"%26\";" << endl;
    *outs << "  else" << endl;
    *outs << "    if (\"$c\" == \"?\") then" << endl;
    *outs << "      set c = \"%3F\"" << endl;
    *outs << "    else" << endl;
    *outs << "      if (\"$c\" == \"=\") then" << endl;
    *outs << "        set c = \"%3D\"" << endl;
    *outs << "      endif" << endl;
    *outs << "    endif" << endl;
    *outs << "  endif" << endl;
    *outs << "  set newpass = \"$newpass$c\"" << endl;
    *outs << "  @ num ++" << endl;
    *outs << "end" << endl;
    *outs << "set passwd = \"$newpass\"" << endl;
  } else {
    *outs << "set passwd = \"xxxxxx\"" << endl;
  }
  *outs << remark << endl;
  if (script_type == "csh") {
    *outs << "if (\"$passwd\" == \"xxxxxx\") then" << endl;
  } else {
    *outs << "if ($passwd == \"xxxxxx\") then" << endl;
  }
  *outs << "  echo \"You need to set your password before you can continue\"" <<
      endl;
  *outs << "  echo \"  see the documentation in the script\"" << endl;
  *outs << "  exit" << endl;
  *outs << "endif" << endl;
  *outs << remark << endl;
  *outs << remark << " authenticate - NOTE: You should only execute this "
      "command ONE TIME." << endl;
  *outs << remark << " Executing this command for every data file you download "
      "may cause" << endl;
  *outs << remark << " your download privileges to be suspended." << endl;
  *outs << "curl -o auth_status." << server_name << " -k -s -c auth." <<
      server_name;
  if (script_type == "csh") {
    *outs << ".$$";
  }
  *outs << " -d \"email=";
  if (!user.empty()) {
    *outs << user;
  } else {
    *outs << "$user";
  }
  *outs << "&passwd=";
  if (script_type == "csh") {
    *outs << "$passwd";
  } else if (script_type == "bat") {
    *outs << "%passwd%";
  }
  *outs << "&action=login\" https://" << server_name << "/cgi-bin/login" <<
      endl;
}

void print_curl_tail(string script_type, string remark, std::ostream *outs) {
  string server_name = getenv("SERVER_NAME");
  *outs << remark << endl;
  *outs << remark << " clean up" << endl;
  if (script_type == "csh") {
    *outs << "rm";
  } else if (script_type == "bat") {
    *outs << "DEL";
  }
  *outs << " auth." << server_name;
  if (script_type == "csh") {
    *outs << ".$$";
  }
  *outs << " auth_status." << server_name;
  *outs << endl;
}

void create_curl_script(const vector<string>& filelist, string server, string
    directory, string dsnum, string script_type, string parameters, string
    level, string product, string start_date, string end_date) {
  const string SERVER_NAME = getenv("SERVER_NAME");
  if (server.empty()) {
    server = SERVER_NAME;
  }
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
  cout << remark << " NOTE:  if you get 403 Forbidden errors when downloading "
      "the data files, check" << endl;
  cout << remark << "        the contents of the file 'auth_status." <<
      SERVER_NAME << "'" << endl;
  for (auto& file : filelist) {
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
      system(("/usr/local/www/server_root/web/cgi-bin/datasets/inventory " +
          dsnum + " GrML " + file + " 2>&1 1>/dev/null").c_str());
    }
    if (stat(inv_file.c_str(), &buf) == 0 && (!parameters.empty() || !level.
        empty() || !product.empty() || (!start_date.empty() && !end_date.
        empty()))) {
      unordered_set<string> parameter_set, level_set, product_set;
      vector<long long> range_start, range_end;
      auto num_lines = 0, num_matches = 0;
      auto last_match = -1;
      std::ifstream ifs(inv_file.c_str());
      if (ifs.is_open()) {
        char line[32768];
        ifs.getline(line, 32768);
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
          ifs.getline(line,32768);
          if (!ifs.eof())
            sline=line;
        }
        ifs.getline(line,32768);
        long long start = -1, end = 0;
        while (!ifs.eof()) {
          sline = line;
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
                if (byte_pos != (end + 1)) {
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
        cout << "curl " << opts << " -k -b auth." << SERVER_NAME;
        if (script_type == "csh")
          cout << ".$$";
        cout << " -r ";
        for (size_t n = 0; n < range_start.size(); ++n) {
          if (n > 0) {
            cout << ",";
          }
          cout << range_start[n] << "-" << range_end[n];
        }
        replace_all(local_file,".tar","");
        cout << " https://" << server << directory << "/" << file << " -o " <<
            local_file << ".subset" << endl;
      } else {
        cout << "curl " << opts << " -k -b auth." << SERVER_NAME;
        if (script_type == "csh") {
          cout << ".$$";
        }
        cout << " https://" << server << directory << "/" << file << " -o " <<
            local_file << endl;
      }
    } else {
      cout << "curl " << opts << " -k -b auth." << SERVER_NAME;
      if (script_type == "csh") {
        cout << ".$$";
      }
      cout << " https://" << server << directory << "/" << file << " -o " <<
          local_file << endl;
    }
  }
  print_curl_tail(script_type, remark, &cout);
}

void create_curl_script(const vector<string>& filelist, string server, string
    directory, string script_type, std::ofstream *ostream) {
  const string SERVER_NAME = getenv("SERVER_NAME");
  if (server.empty()) {
    server = SERVER_NAME;
  }
  std::ostream *outs;
  if (ostream == NULL) {
    outs = &cout;
  } else {
    outs = ostream;
  }
  string remark, cert_opt, opts;
  print_curl_head(script_type, remark, cert_opt, opts, outs);
  *outs << remark << endl;
  *outs << remark << " download the file(s)" << endl;
  *outs << remark << " NOTE:  if you get 403 Forbidden errors when downloading "
      "the data files, check" << endl;
  *outs << remark << "        the contents of the file 'auth_status." <<
      SERVER_NAME << "'" << endl;
  const vector<string> *flist;
  if (filelist.size() == 0) {
    nftw64(directory.c_str(), find_curl_directory_files, 1, 0);
    flist = &curl_filelist;
  } else {
    flist = &filelist;
  }
  for (auto& file : *flist) {
    *outs << "curl " << cert_opt << " " << opts << " -b auth." << SERVER_NAME;
    if (script_type == "csh") {
      *outs << ".$$";
    }
    *outs << " https://" << server;
    if (flist == &filelist && !directory.empty()) {
      *outs << directory;
    }
    *outs << "/" << file << " -o " << file << endl;
  }
  print_curl_tail(script_type, remark, outs);
}

void create_python_script(const vector<string>& filelist, string server, string
    directory, string script_type) {
  const string SERVER_NAME = getenv("SERVER_NAME");
  if (server.empty()) {
    server = SERVER_NAME;
  }
  string user = duser();
  string remark;
  if (script_type == "csh") {
    remark = "#";
    cout << "#! /usr/bin/env python" << endl;
    cout << "#" << endl;
    cout << "# python script to download selected files from " << SERVER_NAME <<
        endl;
    cout << "# after you save the file, don't forget to make it executable" <<
        endl;
    cout << "#   i.e. - \"chmod 755 <name_of_script>\"" << endl;
  } else if (script_type == "bat") {
    remark = "::";
    cout << ":: python script to download selected files from " << SERVER_NAME
        << endl;
  }
  cout << remark << endl;
  cout << "import sys" << endl;
  cout << "import os" << endl;
  cout << "import urllib2" << endl;
  cout << "import cookielib" << endl;
  cout << remark << endl;
  cout << "if (len(sys.argv) != 2):" << endl;
  cout << "  print \"usage: \"+sys.argv[0]+\" [-q] password_on_RDA_webserver\""
      << endl;
  cout << "  print \"-q suppresses the progress message for each file that is "
      "downloaded\"" << endl;
  cout << "  sys.exit(1)" << endl;
  cout << remark << endl;
  cout << "passwd_idx=1" << endl;
  cout << "verbose=True" << endl;
  cout << "if (len(sys.argv) == 3 and sys.argv[1] == \"-q\"):" << endl;
  cout << "  passwd_idx=2" << endl;
  cout << "  verbose=False" << endl;
  cout << remark << endl;
  cout << "cj=cookielib.MozillaCookieJar()" << endl;
  cout << "opener=urllib2.build_opener(urllib2.HTTPCookieProcessor(cj))" <<
      endl;
  cout << remark << endl;
  cout << remark << " check for existing cookies file and authenticate if "
      "necessary" << endl;
  cout << "do_authentication=False" << endl;
  cout << "if (os.path.isfile(\"auth.rda.ucar.edu\")):" << endl;
  cout << "  cj.load(\"auth.rda.ucar.edu\",False,True)" << endl;
  cout << "  for cookie in cj:" << endl;
  cout << "    if (cookie.name == \"sess\" and cookie.is_expired()):" << endl;
  cout << "      do_authentication=True" << endl;
  cout << "else:" << endl;
  cout << "  do_authentication=True" << endl;
  cout << "if (do_authentication):" << endl;
  cout << "  login=opener.open(\"https://" << SERVER_NAME << "/cgi-bin/login\","
      "\"email=" << user << "&password=\"+sys.argv[1]+\"&action=login\")" <<
      endl;
  cout << remark << endl;
  cout << remark << " save the authentication cookies for future downloads" <<
      endl;
  cout << remark << " NOTE! - cookies are saved for future sessions because "
      "overly-frequent authentication to our server can cause your data access "
      "to be blocked" << endl;
  cout << "  cj.clear_session_cookies()" << endl;
  cout << "  cj.save(\"auth.rda.ucar.edu\",True,True)" << endl;
  cout << remark << endl;
  cout << remark << " download the data file(s)" << endl;
  cout << "listoffiles=[";
  auto n = 0;
  for (auto file : filelist) {
    if (n > 0) {
      cout << ",";
    }
    cout << "\"" << file << "\"";
    ++n;
  }
  cout << "]" << endl;
  cout << "for file in listoffiles:" << endl;
  cout << "  idx=file.rfind(\"/\")" << endl;
  cout << "  if (idx > 0):" << endl;
  cout << "    ofile=file[idx+1:]" << endl;
  cout << "  else:" << endl;
  cout << "    ofile=file" << endl;
  cout << "  if (verbose):" << endl;
  cout << "    sys.stdout.write(\"downloading \"+ofile+\"...\")" << endl;
  cout << "    sys.stdout.flush()" << endl;
  cout << "  infile=opener.open(\"http://" << server << directory << "/\"+file)"
      << endl;
  cout << "  outfile=open(ofile,\"wb\")" << endl;
  cout << "  outfile.write(infile.read())" << endl;
  cout << "  outfile.close()" << endl;
  cout << "  if (verbose):" << endl;
  cout << "    sys.stdout.write(\"done.\\n\")" << endl;
}

void create_python_script(const vector<string>& filelist) {
  create_python_script(filelist, "", "", "csh");
}

vector<string> wget_filelist;

extern "C" int getWgetDirectoryFiles(const char *name, const struct stat64
    *data, int flag, struct FTW *ftw_struct) {
  if (flag == FTW_F) {
    string sname = name;
    if (sname.find("wget") == string::npos) {
        wget_filelist.emplace_back(sname);
    }
  }
  return 0;
}

void create_wget_script(const vector<string>& filelist, string server, string
    directory, string script_type, std::ofstream *ostream) {
  const string USER = duser();
  const string SERVER_NAME = getenv("SERVER_NAME");
  if (server.empty()) {
    server = SERVER_NAME;
  }
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
    opts= " $opts";
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
  } else {
    *outs << "Unsupported script type." << endl;
    return;
  }
  *outs << remark << endl;
  *outs << remark << " Experienced Wget Users: add additional command-line "
      "flags here" << endl;
  *outs << remark << "   Use the -r (--recursive) option with care" << endl;
  *outs << remark << "   Do NOT use the -b (--background) option - "
      "simultaneous file downloads" << endl;
  *outs << remark << "       can cause your data access to be blocked" << endl;
  if (script_type == "csh") {
    *outs << "set opts = \"-N\"" << endl;
  } else if (script_type == "bat") {
    *outs << "set opts=-N" << endl;
  }
  *outs << remark << endl;
  if (USER.empty()) {
    *outs << remark << " Replace xxx@xxx.xxx with your email address on the "
        "next line" << endl;
    if (script_type == "csh") {
      *outs << "set user = \"xxx@xxx.xxx\"" << endl;
    } else if (script_type == "bat") {
      *outs << "set user=xxx@xxx.xxx" << endl;
    }
  }
  *outs << remark << " Replace xxxxxx with your rda.ucar.edu password on the "
      "next uncommented line" << endl;
  if (script_type == "csh") {
    *outs << "# IMPORTANT NOTE:  If your password uses a special character "
        "that has special" << endl;
    *outs << "#                  meaning to csh, you should escape it with a "
        "backslash" << endl;
    *outs << "#                  Example:  set passwd = \"my\\!password\"" <<
        endl;
  }
  if (script_type == "csh") {
    *outs << "set passwd = 'xxxxxx'" << endl;
    *outs << "set num_chars = `echo \"$passwd\" |awk '{print length($0)}'`" <<
        endl;
    *outs << "if ($num_chars == 0) then" << endl;
    *outs << "  echo \"You need to set your password before you can continue\""
        << endl;
    *outs << "  echo \"  see the documentation in the script\"" << endl;
    *outs << "  exit" << endl;
    *outs << "endif" << endl;
    *outs << "@ num = 1" << endl;
    *outs << "set newpass = \"\"" << endl;
    *outs << "while ($num <= $num_chars)" << endl;
    *outs << "  set c = `echo \"$passwd\" |cut -b{$num}-{$num}`" << endl;
    *outs << "  if (\"$c\" == \"&\") then" << endl;
    *outs << "    set c = \"%26\";" << endl;
    *outs << "  else" << endl;
    *outs << "    if (\"$c\" == \"?\") then" << endl;
    *outs << "      set c = \"%3F\"" << endl;
    *outs << "    else" << endl;
    *outs << "      if (\"$c\" == \"=\") then" << endl;
    *outs << "        set c = \"%3D\"" << endl;
    *outs << "      endif" << endl;
    *outs << "    endif" << endl;
    *outs << "  endif" << endl;
    *outs << "  set newpass = \"$newpass$c\"" << endl;
    *outs << "  @ num ++" << endl;
    *outs << "end" << endl;
    *outs << "set passwd = \"$newpass\"" << endl;
  } else {
    *outs << "set passwd=xxxxxx" << endl;
  }
  *outs << remark << endl;
  if (script_type == "csh") {
    *outs << "set cert_opt = \"\"" << endl;
  } else if (script_type == "bat") {
    *outs << "set cert_opt=" << endl;
  }
  *outs << remark << " If you get a certificate verification error (version "
      "1.10 or higher)," << endl;
  *outs << remark << " uncomment the following line:" << endl;
  if (script_type == "csh") {
    *outs << remark << "set cert_opt = \"--no-check-certificate\"" << endl;
  } else if (script_type == "bat") {
    *outs << remark << "set cert_opt=--no-check-certificate" << endl;
  }
  *outs << remark << endl;
  if (script_type == "csh") {
    *outs << "if (\"$passwd\" == \"xxxxxx\") then" << endl;
    *outs << "  echo \"You need to set your password before you can continue - "
        "see the documentation in the script\"" << endl;
    *outs << "  exit" << endl;
    *outs << "endif" << endl;
  } else if (script_type == "bat") {
    *outs << "if \"%passwd%\" == \"xxxxxx\" (" << endl;
    *outs << "  echo You need to set your password before you can continue - "
        "see the documentation in the script" << endl;
    *outs << "  exit" << endl;
    *outs << ")" << endl;
  }
  *outs << remark << endl;
  *outs << remark << " authenticate - NOTE: You should only execute this "
      "command ONE TIME." << endl;
  *outs << remark << " Executing this command for every data file you download "
      "may cause" << endl;
  *outs << remark << " your download privileges to be suspended." << endl;
  *outs << "wget " << cert_opt << " -O auth_status.rda.ucar.edu --save-cookies "
      "auth." << SERVER_NAME;
  if (script_type == "csh") {
    *outs << ".$$";
  }
  *outs << " --post-data=\"email=";
  if (!USER.empty()) {
    *outs << USER;
  } else {
    *outs << "$user";
  }
  *outs << "&passwd=";
  if (script_type == "csh") {
    *outs << "$passwd";
  } else if (script_type == "bat") {
    *outs << "%passwd%";
  }
  *outs << "&action=login\" https://" << SERVER_NAME << "/cgi-bin/login" <<
      endl;
  *outs << remark << endl;
  *outs << remark << " download the file(s)" << endl;
  *outs << remark << " NOTE:  if you get 403 Forbidden errors when downloading "
      "the data files, check" << endl;
  *outs << remark << "        the contents of the file 'auth_status.rda.ucar."
      "edu'" << endl;
  const vector<string> *flist;
  if (filelist.empty()) {
    nftw64(directory.c_str(), getWgetDirectoryFiles, 1, 0);
    flist = &wget_filelist;
  } else {
    flist = &filelist;
  }
  for (const auto& file : *flist) {
    *outs << "wget " << cert_opt << " " << opts << " --load-cookies auth." <<
        SERVER_NAME;
    if (script_type == "csh") {
      *outs << ".$$";
    }
    if (server.empty()) {
      *outs << " https://" << SERVER_NAME;
    } else {
      *outs << " " << server;
    }
    if (flist == &filelist && !directory.empty()) {
      *outs << directory;
    }
    *outs << "/" << file << endl;
  }
  *outs << remark << endl;
  *outs << remark << " clean up" << endl;
  if (script_type == "csh") {
    *outs << "rm";
  } else if (script_type == "bat") {
    *outs << "DEL";
  }
  *outs << " auth." << SERVER_NAME;
  if (script_type == "csh") {
    *outs << ".$$";
  }
  *outs << " auth_status." << SERVER_NAME;
  *outs << endl;
}

void create_wget_script(const vector<string>& filelist) {
  create_wget_script(filelist, "", "", "csh");
}
