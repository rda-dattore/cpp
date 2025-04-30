#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sstream>
#include <regex>
#include <thread>
#include <strutils.hpp>
#include <utils.hpp>

using std::regex;
using std::regex_search;
using std::string;
using std::stringstream;
using strutils::chop;
using strutils::replace_all;
using strutils::split;

namespace mysystem_t {

struct ThreadStruct {
  ThreadStruct() : tid(0), pipe(0), ss(nullptr) { }

  pthread_t tid;
  int pipe;
  stringstream *ss;
};

extern "C" void *read_output(void *ts) {
  char buffer[32768];
  auto *t = (ThreadStruct *)ts;
  int len;
  while ( (len = read(t->pipe, buffer, 32768)) > 0) {
    *(t->ss) << string(buffer, len);
  }
  close(t->pipe);
  return nullptr;
}

extern "C" void *read_error(void *ts) {
  char buffer[32768];
  auto *t = (ThreadStruct *)ts;
  int len;
  while ( (len = read(t->pipe, buffer, 32768)) > 0) {
    *(t->ss) << string(buffer, len);
  }
  close(t->pipe);
  return nullptr;
}

} // end namespace mysystem_t

namespace unixutils {

string retry_command(string command, int num_retries) {
  string s; // return value
  int n = 0;
  stringstream oss, ess;
  if (mysystem2(command, oss, ess) != 0) {

    // deal with some specific situations
    if (regex_search(ess.str(), regex("^mv: preserving permissions"))) {
      auto sp = split(command);
      if (sp[0] == "/bin/mv" && regex_search(sp[1], regex("^/tmp"))) {
        ess.str("");
      }
    }
    while (!ess.str().empty() && n < num_retries) {
      if (!s.empty()) {
        s += ";";
      }
      s += "'" + ess.str() + "'";
      std::this_thread::sleep_for(std::chrono::seconds(5));
      ++n;
      mysystem2(command, oss, ess);
    }
  }
  if (n < num_retries) {
    s = "";
  }
  return s;
}

int mysystem2(string command, stringstream& output, stringstream& error) {
  output.str("");
  error.str("");
  int pipe_out[2], pipe_err[2];
  if (pipe(pipe_out) != 0) {
    error << "can't create output pipe";
    return -1;
  }
  if (pipe(pipe_err) != 0) {
    error << "can't create error pipe";
    return -1;
  }
  auto c = fork();
  if (c < 0) {
    error << "unable to fork";
    return -1;
  } else if (c == 0) {
    string s;
    char c_quote;
    auto in_quotes = false;
    auto sp = split(command);
    for (size_t n = 0; n < sp.size(); ++n) {
      if ((sp[n][0] == '"' || sp[n][0] == '\'') && !in_quotes) {
        c_quote = sp[n][0];
        sp[n] = sp[n].substr(1);
        in_quotes = true;
      }
      if (in_quotes && sp[n].back() == c_quote) {
        in_quotes = false;
        chop(sp[n]);
      }
      if (in_quotes) {
        s += sp[n] + " ";
      } else {
        s += sp[n] + "<!>";
      }
    }
    auto sp2 = split(s, "<!>");
    sp2.pop_back();
    auto argv = new char *[sp2.size() + 1];
    auto idx = sp2[0].rfind("/");
    if (idx == string::npos) {
      argv[0] = new char[sp2[0].length() + 1];
      strcpy(argv[0], sp2[0].c_str());
    } else {
      auto a = sp2[0].substr(idx + 1);
      argv[0] = new char[a.length() + 1];
      strcpy(argv[0], a.c_str());
    }
    size_t last = 1;
    for (size_t n = last; n < sp2.size(); ++n) {
      replace_all(sp2[n], "__INNER_QUOTE__", "'");
      if (sp2[n] == "<") {
        if (n == (sp2.size() - 2)) {
          auto d = open(sp2[n + 1].c_str(), O_RDONLY);
          dup2(d, 0);
          close(d);
          break;
        } else {
          error << "bad input redirection";
          return -1;
        }
      } else {
        argv[n] = new char[sp2[n].length() + 1];
        strcpy(argv[n], sp2[n].c_str());
      }
      ++last;
    }
    argv[last] = nullptr;
    close(pipe_out[0]);
    close(pipe_err[0]);
    dup2(pipe_out[1], 1);
    dup2(pipe_err[1], 2);
    execv(sp2[0].c_str(), argv);
    std::cerr << sp2[0] << ": command not found";
    exit(1);
  } else {
    close(pipe_out[1]);
    close(pipe_err[1]);
    mysystem_t::ThreadStruct t[2];
    t[0].pipe = pipe_out[0];
    t[0].ss = &output;
    pthread_create(&t[0].tid, nullptr, mysystem_t::read_output,
        reinterpret_cast<void *>(&t[0]));
    t[1].pipe = pipe_err[0];
    t[1].ss = &error;
    pthread_create(&t[1].tid, nullptr, mysystem_t::read_error, reinterpret_cast<
        void *>(&t[1]));
    pthread_join(t[0].tid, nullptr);
    pthread_join(t[1].tid, nullptr);
    int s;
    auto pid = waitpid(c, &s, 0);
    if (pid < 0 || !error.str().empty()) {
      return WEXITSTATUS(s);
    }
    return WEXITSTATUS(s);
  }
}

} // end namespace unixutils
