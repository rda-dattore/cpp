#include <iostream>
#include <string>
#include <list>
#include <web/web.hpp>

void create_python_script(std::list<std::string>& filelist,std::string directory,std::string script_type)
{
  std::string user=duser();
  std::string remark;
  std::string server_name=getenv("SERVER_NAME");
  int n=0;

  if (script_type == "csh") {
    remark="#";
    std::cout << "#! /usr/bin/env python" << std::endl;
    std::cout << "#" << std::endl;
    std::cout << "# python script to download selected files from " << server_name << std::endl;
    std::cout << "# after you save the file, don't forget to make it executable" << std::endl;
    std::cout << "#   i.e. - \"chmod 755 <name_of_script>\"" << std::endl;
  }
  else if (script_type == "bat") {
    remark="::";
    std::cout << ":: python script to download selected files from " << server_name << std::endl;
  }
  std::cout << remark << std::endl;
  std::cout << "import sys" << std::endl;
  std::cout << "import os" << std::endl;
  std::cout << "import urllib2" << std::endl;
  std::cout << "import cookielib" << std::endl;
  std::cout << remark << std::endl;
  std::cout << "if (len(sys.argv) != 2):" << std::endl;
  std::cout << "  print \"usage: \"+sys.argv[0]+\" [-q] password_on_RDA_webserver\"" << std::endl;
  std::cout << "  print \"-q suppresses the progress message for each file that is downloaded\"" << std::endl;
  std::cout << "  sys.exit(1)" << std::endl;
  std::cout << remark << std::endl;
  std::cout << "passwd_idx=1" << std::endl;
  std::cout << "verbose=True" << std::endl;
  std::cout << "if (len(sys.argv) == 3 and sys.argv[1] == \"-q\"):" << std::endl;
  std::cout << "  passwd_idx=2" << std::endl;
  std::cout << "  verbose=False" << std::endl;
  std::cout << remark << std::endl;
  std::cout << "cj=cookielib.MozillaCookieJar()" << std::endl;
  std::cout << "opener=urllib2.build_opener(urllib2.HTTPCookieProcessor(cj))" << std::endl;
  std::cout << remark << std::endl;
  std::cout << remark << " check for existing cookies file and authenticate if necessary" << std::endl;
  std::cout << "do_authentication=False" << std::endl;
  std::cout << "if (os.path.isfile(\"auth.rda.ucar.edu\")):" << std::endl;
  std::cout << "  cj.load(\"auth.rda.ucar.edu\",False,True)" << std::endl;
  std::cout << "  for cookie in cj:" << std::endl;
  std::cout << "    if (cookie.name == \"sess\" and cookie.is_expired()):" << std::endl;
  std::cout << "      do_authentication=True" << std::endl;
  std::cout << "else:" << std::endl;
  std::cout << "  do_authentication=True" << std::endl;
  std::cout << "if (do_authentication):" << std::endl;
  std::cout << "  login=opener.open(\"https://" << server_name << "/cgi-bin/login\",\"email=" << user << "&password=\"+sys.argv[1]+\"&action=login\")" << std::endl;
  std::cout << remark << std::endl;
  std::cout << remark << " save the authentication cookies for future downloads" << std::endl;
  std::cout << remark << " NOTE! - cookies are saved for future sessions because overly-frequent authentication to our server can cause your data access to be blocked" << std::endl;
  std::cout << "  cj.clear_session_cookies()" << std::endl;
  std::cout << "  cj.save(\"auth.rda.ucar.edu\",True,True)" << std::endl;
  std::cout << remark << std::endl;
  std::cout << remark << " download the data file(s)" << std::endl;
  std::cout << "listoffiles=[";
  for (auto file : filelist) {
    if (n > 0)
	std::cout << ",";
    std::cout << "\"" << file << "\"";
    n++;
  }
  std::cout << "]" << std::endl;
  std::cout << "for file in listoffiles:" << std::endl;
  std::cout << "  idx=file.rfind(\"/\")" << std::endl;
  std::cout << "  if (idx > 0):" << std::endl;
  std::cout << "    ofile=file[idx+1:]" << std::endl;
  std::cout << "  else:" << std::endl;
  std::cout << "    ofile=file" << std::endl;
  std::cout << "  if (verbose):" << std::endl;
  std::cout << "    sys.stdout.write(\"downloading \"+ofile+\"...\")" << std::endl;
  std::cout << "    sys.stdout.flush()" << std::endl;
  std::cout << "  infile=opener.open(\"http://" << server_name << directory << "/\"+file)" << std::endl;
  std::cout << "  outfile=open(ofile,\"wb\")" << std::endl;
  std::cout << "  outfile.write(infile.read())" << std::endl;
  std::cout << "  outfile.close()" << std::endl;
  std::cout << "  if (verbose):" << std::endl;
  std::cout << "    sys.stdout.write(\"done.\\n\")" << std::endl;
}

void create_python_script(std::list<std::string>& filelist)
{
  create_python_script(filelist,"","csh");
}
