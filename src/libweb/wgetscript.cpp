#include <iostream>
#include <fstream>
#include <ftw.h>
#include <string>
#include <list>
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

void create_wget_script(vector<string>& filelist, string server, string
    directory, string script_type, std::ofstream *ostream) {
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
    opts = "$opts";
    *outs << "#! /bin/csh -f" << endl;
    *outs << "#" << endl;
    *outs << "# c-shell script to download selected files from " << SERVER_NAME << " using Wget" << endl;
    *outs << "# NOTE: if you want to run under a different shell, make sure you change" << endl;
    *outs << "#       the 'set' commands according to your shell's syntax" << endl;
    *outs << "# after you save the file, don't forget to make it executable" << endl;
    *outs << "#   i.e. - \"chmod 755 <name_of_script>\"" << endl;
  } else if (script_type == "bat") {
    remark = "::";
    cert_opt = "%cert_opt%";
    opts = "%opts%";
    *outs << ":: DOS batch script to download selected files from " << SERVER_NAME << " using Wget" << endl;
  }
  *outs << remark << endl;
  *outs << remark << " Experienced Wget Users: add additional command-line flags here" << endl;
  *outs << remark << "   Use the -r (--recursive) option with care" << endl;
  *outs << remark << "   Do NOT use the -b (--background) option - simultaneous file downloads" << endl;
  *outs << remark << "       can cause your data access to be blocked" << endl;
  if (script_type == "csh") {
    *outs << "set opts = \"-N\"" << endl;
  } else if (script_type == "bat") {
    *outs << "set opts=-N" << endl;
  }
  *outs << remark << endl;
    *outs << remark << " Replace xxx@xxx.xxx with your email address on the next line" << endl;
    if (script_type == "csh") {
      *outs << "set user = \"xxx@xxx.xxx\"" << endl;
    }
    else if (script_type == "bat") {
      *outs << "set user=xxx@xxx.xxx" << endl;
    }
  *outs << remark << " Replace xxxxxx with your rda.ucar.edu password on the next uncommented line" << endl;
  if (script_type == "csh") {
    *outs << "# IMPORTANT NOTE:  If your password uses a special character that has special" << endl;
    *outs << "#                  meaning to csh, you should escape it with a backslash" << endl;
    *outs << "#                  Example:  set passwd = \"my\\!password\"" << endl;
  }
  if (script_type == "csh") {
    *outs << "set passwd = 'xxxxxx'" << endl;
    *outs << "set num_chars = `echo \"$passwd\" |awk '{print length($0)}'`" << endl;
    *outs << "if ($num_chars == 0) then" << endl;
    *outs << "  echo \"You need to set your password before you can continue\"" << endl;
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
  *outs << remark << " If you get a certificate verification error (version 1.10 or higher)," << endl;
  *outs << remark << " uncomment the following line:" << endl;
  if (script_type == "csh") {
    *outs << remark << "set cert_opt = \"--no-check-certificate\"" << endl;
  } else if (script_type == "bat") {
    *outs << remark << "set cert_opt=--no-check-certificate" << endl;
  }
  *outs << remark << endl;
  if (script_type == "csh") {
    *outs << "if (\"$passwd\" == \"xxxxxx\") then" << endl;
    *outs << "  echo \"You need to set your password before you can continue - see the documentation in the script\"" << endl;
    *outs << "  exit" << endl;
    *outs << "endif" << endl;
  } else if (script_type == "bat") {
    *outs << "if \"%passwd%\" == \"xxxxxx\" (" << endl;
    *outs << "  echo You need to set your password before you can continue - see the documentation in the script" << endl;
    *outs << "  exit" << endl;
    *outs << ")" << endl;
  }
  *outs << remark << endl;
  *outs << remark << " authenticate - NOTE: You should only execute this command ONE TIME." << endl;
  *outs << remark << " Executing this command for every data file you download may cause" << endl;
  *outs << remark << " your download privileges to be suspended." << endl;
  *outs << "wget " << cert_opt << " -O auth_status.rda.ucar.edu --save-cookies auth." << SERVER_NAME;
  if (script_type == "csh") {
    *outs << ".$$";
  }
  *outs << " --post-data=\"email=";
    *outs << "$user";
  *outs << "&passwd=";
  if (script_type == "csh") {
    *outs << "$passwd";
  } else if (script_type == "bat") {
    *outs << "%passwd%";
  }
  *outs << "&action=login\" https://" << SERVER_NAME << "/cgi-bin/login" << endl;
  *outs << remark << endl;
  *outs << remark << " download the file(s)" << endl;
  *outs << remark << " NOTE:  if you get 403 Forbidden errors when downloading the data files, check" << endl;
  *outs << remark << "        the contents of the file 'auth_status.rda.ucar.edu'" << endl;
  vector<string> *flist;
  if (filelist.size() == 0) {
    nftw64(directory.c_str(), getWgetDirectoryFiles, 1, 0);
    flist = &wget_filelist;
  } else {
    flist = &filelist;
  }
  for (const auto& file : *flist) {
    *outs << "wget " << cert_opt << " " << opts << " --load-cookies auth." << SERVER_NAME;
    if (script_type == "csh") {
      *outs << ".$$";
    }
    *outs << " https://" << SERVER_NAME;
    if (flist == &filelist && directory.length() > 0) {
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

void create_wget_script(vector<string>& filelist) {
  create_wget_script(filelist, "", "", "csh");
}
