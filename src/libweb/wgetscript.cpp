#include <iostream>
#include <fstream>
#include <ftw.h>
#include <string>
#include <list>
#include <strutils.hpp>
#include <web/web.hpp>

std::list<std::string> wget_filelist;
extern "C" int getWgetDirectoryFiles(const char *name,const struct stat64 *data,int flag,struct FTW *ftw_struct)
{
  std::string sname;

  if (flag == FTW_F) {
    sname=name;
    if (!strutils::contains(sname,"wget")) {
        wget_filelist.push_back(sname);
    }
  }
  return 0;
}

void create_wget_script(std::list<std::string>& filelist,std::string directory,std::string script_type,std::ofstream *ostream)
{
  const std::string USER=duser();
  const std::string SERVER_NAME=getenv("SERVER_NAME");
  std::ostream *outs;
  if (ostream == NULL) {
    outs=&std::cout;
  }
  else {
    outs=ostream;
  }
  std::string remark,cert_opt,opts;
  if (script_type == "csh") {
    remark="#";
    cert_opt="$cert_opt";
    opts="$opts";
    *outs << "#! /bin/csh -f" << std::endl;
    *outs << "#" << std::endl;
    *outs << "# c-shell script to download selected files from " << SERVER_NAME << " using Wget" << std::endl;
    *outs << "# NOTE: if you want to run under a different shell, make sure you change" << std::endl;
    *outs << "#       the 'set' commands according to your shell's syntax" << std::endl;
    *outs << "# after you save the file, don't forget to make it executable" << std::endl;
    *outs << "#   i.e. - \"chmod 755 <name_of_script>\"" << std::endl;
  }
  else if (script_type == "bat") {
    remark="::";
    cert_opt="%cert_opt%";
    opts="%opts%";
    *outs << ":: DOS batch script to download selected files from " << SERVER_NAME << " using Wget" << std::endl;
  }
  *outs << remark << std::endl;
  *outs << remark << " Experienced Wget Users: add additional command-line flags here" << std::endl;
  *outs << remark << "   Use the -r (--recursive) option with care" << std::endl;
  *outs << remark << "   Do NOT use the -b (--background) option - simultaneous file downloads" << std::endl;
  *outs << remark << "       can cause your data access to be blocked" << std::endl;
  if (script_type == "csh") {
    *outs << "set opts = \"-N\"" << std::endl;
  }
  else if (script_type == "bat") {
    *outs << "set opts=-N" << std::endl;
  }
  *outs << remark << std::endl;
  if (USER.empty()) {
    *outs << remark << " Replace xxx@xxx.xxx with your email address on the next line" << std::endl;
    if (script_type == "csh") {
	*outs << "set user = \"xxx@xxx.xxx\"" << std::endl;
    }
    else if (script_type == "bat") {
	*outs << "set user=xxx@xxx.xxx" << std::endl;
    }
  }
  *outs << remark << " Replace xxxxxx with your rda.ucar.edu password on the next uncommented line" << std::endl;
  if (script_type == "csh") {
    *outs << "# IMPORTANT NOTE:  If your password uses a special character that has special" << std::endl;
    *outs << "#                  meaning to csh, you should escape it with a backslash" << std::endl;
    *outs << "#                  Example:  set passwd = \"my\\!password\"" << std::endl;
  }
  if (script_type == "csh") {
    *outs << "set passwd = 'xxxxxx'" << std::endl;
    *outs << "set num_chars = `echo \"$passwd\" |awk '{print length($0)}'`" << std::endl;
    *outs << "if ($num_chars == 0) then" << std::endl;
    *outs << "  echo \"You need to set your password before you can continue\"" << std::endl;
    *outs << "  echo \"  see the documentation in the script\"" << std::endl;
    *outs << "  exit" << std::endl;
    *outs << "endif" << std::endl;
    *outs << "@ num = 1" << std::endl;
    *outs << "set newpass = \"\"" << std::endl;
    *outs << "while ($num <= $num_chars)" << std::endl;
    *outs << "  set c = `echo \"$passwd\" |cut -b{$num}-{$num}`" << std::endl;
    *outs << "  if (\"$c\" == \"&\") then" << std::endl;
    *outs << "    set c = \"%26\";" << std::endl;
    *outs << "  else" << std::endl;
    *outs << "    if (\"$c\" == \"?\") then" << std::endl;
    *outs << "      set c = \"%3F\"" << std::endl;
    *outs << "    else" << std::endl;
    *outs << "      if (\"$c\" == \"=\") then" << std::endl;
    *outs << "        set c = \"%3D\"" << std::endl;
    *outs << "      endif" << std::endl;
    *outs << "    endif" << std::endl;
    *outs << "  endif" << std::endl;
    *outs << "  set newpass = \"$newpass$c\"" << std::endl;
    *outs << "  @ num ++" << std::endl;
    *outs << "end" << std::endl;
    *outs << "set passwd = \"$newpass\"" << std::endl;
  }
  else {
    *outs << "set passwd=xxxxxx" << std::endl;
  }
  *outs << remark << std::endl;
  if (script_type == "csh") {
    *outs << "set cert_opt = \"\"" << std::endl;
  }
  else if (script_type == "bat") {
    *outs << "set cert_opt=" << std::endl;
  }
  *outs << remark << " If you get a certificate verification error (version 1.10 or higher)," << std::endl;
  *outs << remark << " uncomment the following line:" << std::endl;
  if (script_type == "csh") {
    *outs << remark << "set cert_opt = \"--no-check-certificate\"" << std::endl;
  }
  else if (script_type == "bat") {
    *outs << remark << "set cert_opt=--no-check-certificate" << std::endl;
  }
  *outs << remark << std::endl;
  if (script_type == "csh") {
    *outs << "if (\"$passwd\" == \"xxxxxx\") then" << std::endl;
    *outs << "  echo \"You need to set your password before you can continue - see the documentation in the script\"" << std::endl;
    *outs << "  exit" << std::endl;
    *outs << "endif" << std::endl;
  }
  else if (script_type == "bat") {
    *outs << "if \"%passwd%\" == \"xxxxxx\" (" << std::endl;
    *outs << "  echo You need to set your password before you can continue - see the documentation in the script" << std::endl;
    *outs << "  exit" << std::endl;
    *outs << ")" << std::endl;
  }
  *outs << remark << std::endl;
  *outs << remark << " authenticate - NOTE: You should only execute this command ONE TIME." << std::endl;
  *outs << remark << " Executing this command for every data file you download may cause" << std::endl;
  *outs << remark << " your download privileges to be suspended." << std::endl;
  *outs << "wget " << cert_opt << " -O auth_status.rda.ucar.edu --save-cookies auth." << SERVER_NAME;
  if (script_type == "csh") {
    *outs << ".$$";
  }
  *outs << " --post-data=\"email=";
  if (!USER.empty()) {
    *outs << USER;
  }
  else {
    *outs << "$user";
  }
  *outs << "&passwd=";
  if (script_type == "csh") {
    *outs << "$passwd";
  }
  else if (script_type == "bat") {
    *outs << "%passwd%";
  }
  *outs << "&action=login\" https://" << SERVER_NAME << "/cgi-bin/login" << std::endl;
  *outs << remark << std::endl;
  *outs << remark << " download the file(s)" << std::endl;
  *outs << remark << " NOTE:  if you get 403 Forbidden errors when downloading the data files, check" << std::endl;
  *outs << remark << "        the contents of the file 'auth_status.rda.ucar.edu'" << std::endl;
  std::list<std::string> *flist;
  if (filelist.size() == 0) {
    nftw64(directory.c_str(),getWgetDirectoryFiles,1,0);
    flist=&wget_filelist;
  }
  else {
    flist=&filelist;
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
    *outs << "/" << file << std::endl;
  }
  *outs << remark << std::endl;
  *outs << remark << " clean up" << std::endl;
  if (script_type == "csh") {
    *outs << "rm";
  }
  else if (script_type == "bat") {
    *outs << "DEL";
  }
  *outs << " auth." << SERVER_NAME;
  if (script_type == "csh") {
    *outs << ".$$";
  }
  *outs << " auth_status." << SERVER_NAME;
  *outs << std::endl;
}

void create_wget_script(std::list<std::string>& filelist)
{
  create_wget_script(filelist,"","csh");
}
