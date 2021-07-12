#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sstream>
#include <regex>
#include <thread>
#include <strutils.hpp>
#include <utils.hpp>

namespace mysystem_t {

struct ThreadStruct {
  ThreadStruct() : tid(0),pipe(0),ss(nullptr) {}

  pthread_t tid;
  int pipe;
  std::stringstream *ss;
};

extern "C" void *read_output(void *ts)
{
  ThreadStruct *t=(ThreadStruct *)ts;
  int len;
  char buffer[32768];

  while ( (len=read(t->pipe,buffer,32768)) > 0) {
    *(t->ss) << std::string(buffer,len);
  }
  close(t->pipe);
  return nullptr;
}

extern "C" void *read_error(void *ts)
{
  ThreadStruct *t=(ThreadStruct *)ts;
  int len;
  char buffer[32768];

  while ( (len=read(t->pipe,buffer,32768)) > 0) {
    *(t->ss) << std::string(buffer,len);
  }
  close(t->pipe);
  return nullptr;
}

} // end namespace mysystem_t

namespace unixutils {

std::string retry_command(std::string command,int num_retries)
{
  std::stringstream output,error;
  std::string error_list;
  int nr=0;

  if (mysystem2(command,output,error) != 0) {
// deal with some specific situations
    if (std::regex_search(error.str(),std::regex("^mv: preserving permissions"))) {
	auto  sp=strutils::split(command);
	if (sp[0] == "/bin/mv" && std::regex_search(sp[1],std::regex("^/tmp"))) {
	  error.str("");
	}
    }
    while (!error.str().empty() && nr < num_retries) {
	if (!error_list.empty()) {
	  error_list+=";";
	}
	error_list+="'"+error.str()+"'";
	std::this_thread::sleep_for(std::chrono::seconds(5));
	++nr;
	mysystem2(command,output,error);
    }
  }
  if (nr < num_retries) {
    error_list="";
  }
  return error_list;
}

int mysystem2(std::string command,std::stringstream& output,std::stringstream& error)
{
  output.str("");
  error.str("");
  int pipe_out[2],pipe_err[2];
  pipe(pipe_out);
  pipe(pipe_err);
  auto child_pid=fork();
  if (child_pid < 0) {
    error << "Unable to fork";
    return -1;
  }
  else if (child_pid == 0) {
    std::string new_command;
    char c_quote='\0';
    auto in_quotes=false;
    auto sp=strutils::split(command);
    for (size_t n=0; n < sp.size(); ++n) {
	if ((sp[n][0] == '"' || sp[n][0] == '\'') && !in_quotes) {
	    c_quote=sp[n][0];
	    sp[n]=sp[n].substr(1);
	    in_quotes=true;
	}
	if (in_quotes && sp[n].back() == c_quote) {
	  in_quotes=false;
	  strutils::chop(sp[n]);
	  c_quote='\0';
	}
	if (in_quotes) {
	  new_command+=sp[n]+" ";
	}
	else {
	  new_command+=sp[n]+"<!>";
	}
    }
    strutils::chop(new_command,3);
    sp=strutils::split(new_command,"<!>");
    auto argv=new char *[sp.size()+1];
    auto idx=sp[0].rfind("/");
    if (idx == std::string::npos) {
	argv[0]=new char[sp[0].length()+1];
	strcpy(argv[0],sp[0].c_str());
    }
    else {
	auto arg0=sp[0].substr(idx+1);
	argv[0]=new char[arg0.length()+1];
	strcpy(argv[0],arg0.c_str());
    }
    size_t last=1;
    for (size_t n=last; n < sp.size(); ++n) {
	strutils::replace_all(sp[n],"__INNER_QUOTE__","'");
	if (sp[n] == "<") {
	  if (n == (sp.size()-2)) {
	    auto rstdin=open(sp[n+1].c_str(),O_RDONLY);
	    dup2(rstdin,0);
	    close(rstdin);
	    break;
	  }
	  else {
	    error << "bad input redirection";
	    return -1;
	  }
	}
	else {
	  argv[n]=new char[sp[n].length()+1];
	  strcpy(argv[n],sp[n].c_str());
	}
	++last;
    }
    argv[last]=nullptr;
    close(pipe_out[0]);
    close(pipe_err[0]);
    dup2(pipe_out[1],1);
    dup2(pipe_err[1],2);
    execv(sp[0].c_str(),argv);
    std::cerr << sp[0] << ": command not found" << std::endl;
    exit(1);
  }
  else {
    close(pipe_out[1]);
    close(pipe_err[1]);
    mysystem_t::ThreadStruct tstructs[2];
    tstructs[0].pipe=pipe_out[0];
    tstructs[0].ss=&output;
    pthread_create(&tstructs[0].tid,nullptr,mysystem_t::read_output,reinterpret_cast<void *>(&tstructs[0]));
    tstructs[1].pipe=pipe_err[0];
    tstructs[1].ss=&error;
    pthread_create(&tstructs[1].tid,nullptr,mysystem_t::read_error,reinterpret_cast<void *>(&tstructs[1]));
    pthread_join(tstructs[0].tid,nullptr);
    pthread_join(tstructs[1].tid,nullptr);
    int status;
    auto pid=waitpid(child_pid,&status,0);
    if (pid < 0 || !error.str().empty()) {
	return WEXITSTATUS(status);
    }
    else {
	return WEXITSTATUS(status);
    }
  }
}

} // end namespace unixutils
