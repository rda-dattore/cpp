#include <iostream>
#include <string>
#include <sstream>
#include <regex>
#include <thread>
#include <unistd.h>
#include <sys/stat.h>
#include <datetime.hpp>
#include <strutils.hpp>
#include <utils.hpp>

namespace unixutils {

std::string remote_file(std::string server_name,std::string local_tmpdir,std::string remote_filename,std::string& error)
{
  FILE *p;
  int status;
  std::string local_filename;

  p=popen(("rsync -aLq "+server_name+":"+remote_filename+" "+local_tmpdir).c_str(),"r");
  status=pclose(p);
  if (status == 0) {
    local_filename=local_tmpdir+remote_filename.substr(remote_filename.rfind("/"));
  }
  else {
    error="rsync returned status "+strutils::itos(status);
  }
  return local_filename;
}

std::string remote_web_file(std::string URL,std::string local_tmpdir)
{
  std::string local_filename;
  size_t num_tries=0;
  while (local_filename.empty() && num_tries < 3) {
    local_filename=local_tmpdir+URL.substr(URL.rfind("/"));
    std::stringstream oss,ess;
    mysystem2("/bin/sh -c \"curl -q -s -w '%{http_code}\\n' -L -o "+local_filename+" "+strutils::substitute(URL,"%","%25")+"\"",oss,ess);
    struct stat buf;
    if (oss.str() != "200\n" || stat(local_filename.c_str(),&buf) != 0 || buf.st_size == 0) {
	mysystem2("/bin/rm -rf "+local_filename,oss,ess);
	std::this_thread::sleep_for(std::chrono::seconds(15));
	local_filename="";
	++num_tries;
    }
  }
  return local_filename;
}

int rdadata_sync(std::string directory,std::string relative_path,std::string remote_path,std::string rdadata_home,std::string& error)
{
  if (directory.empty()) {
    error="missing directory";
    return -1;
  }
  if (relative_path.empty()) {
    error="nothing to copy";
    return -1;
  }
  if (remote_path.empty()) {
    error="unknown destination";
    return -1;
  }
  int status=0;
  std::list<std::string> hosts;
  error="";
  if (!unixutils::hosts_loaded(hosts,rdadata_home)) {
    error="rdadata_sync() unable to get list of host names";
    return -1;
  }
  setreuid(15968,15968);
  for (const auto& host : hosts) {
    auto retry_error=retry_command("/bin/sh -c 'cd "+directory+"; rsync -rptgD --relative -e __INNER_QUOTE__ssh -i "+rdadata_home+"/.ssh/"+host+"-sync_rdadata_rsa -l rdadata__INNER_QUOTE__ "+relative_path+" "+host+".ucar.edu:"+remote_path+"'",3);
    if (!retry_error.empty()) {
	if (!error.empty()) {
	  error+=", ";
	}
	error+="rsync error on '"+host+"': *'"+retry_error+"'*";
	status=-1;
    }
  }
  return status;
}

int rdadata_unsync(std::string remote_filename,std::string rdadata_home,std::string& error)
{
// special tokens in remote_filename
//  __HOST__ will be replaced with hosts.getCurrent()
//
  std::list<std::string> hosts;
  std::string rname;
  std::string retry_error;
  int status=0;

  error="";
  if (!unixutils::hosts_loaded(hosts,rdadata_home)) {
    error="rdadata_unsync() unable to get list of host names";
    return -1;
  }
  for (auto& host : hosts) {
    rname=remote_filename;
    strutils::replace_all(rname,"__HOST__",host);
    retry_error=retry_command("/bin/sh -c \""+rdadata_home+"/bin/"+host+"-sync -d "+rname+"\"",3);
    if (retry_error.length() > 0) {
	if (error.length() > 0) {
	  error+=", ";
	}
	error+=host+"-sync error: *'"+retry_error+"'*";
	status=-1;
    }
  }
  return status;
}

int rdadata_sync_from(std::string remote_filename,std::string local_filename,std::string rdadata_home,std::stringstream& ess)
{
// special tokens in remote_filename
//  __HOST__ will be replaced with hosts.getCurrent()
//
  std::list<std::string> hosts;
  std::stringstream oss;
  int status=0;
  struct stat buf;

  if (!unixutils::hosts_loaded(hosts,rdadata_home)) {
    ess << "rdadata_sync_from() unable to get list of host names" << std::endl;
    return -1;
  }
  for (auto& host : hosts) {
    std::string rname=remote_filename;
    strutils::replace_all(rname,"__HOST__",host);
    if (mysystem2("/bin/sh -c \""+rdadata_home+"/bin/"+host+"-sync -g "+rname+" "+local_filename+"\"",oss,ess) == 0 && stat(local_filename.c_str(),&buf) == 0) {
	break;
    }
    else {
	status=-1;
    }
  }
  return status;
}

bool exists_on_server(std::string host_name,std::string remote_filename,std::string rdadata_home)
{
  std::stringstream output,error;
  mysystem2("/bin/sh -c 'rsync --list-only -e __INNER_QUOTE__ssh -i "+rdadata_home+"/.ssh/"+strutils::token(host_name,".",0)+"-sync_rdadata_rsa -l rdadata__INNER_QUOTE__ "+host_name+":"+remote_filename+"'",output,error);
  if (!output.str().empty()) {
    if (error.str().empty()) {
	return true;
    }
    else {
	return false;
    }
  }
  else {
    return false;
  }
}

} // end namespace unixutils
