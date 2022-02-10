#include <iostream>
#include <string>
#include <list>
#include <strutils.hpp>
#include <datetime.hpp>
#include <web/web.hpp>

using std::cout;
using std::endl;
using std::list;
using std::string;
using strutils::split;

string cookie_value(string cookie_name) {
  string s; // return value
  auto *e = getenv("HTTP_COOKIE");
  if (e != nullptr) {
    s = e;
    auto idx = s.find(cookie_name + "=");
    while (idx != string::npos && idx > 1 && s.substr(idx - 2, 2) != "; ") {
      idx = s.find(cookie_name + "=", idx + 1);
    }
    if (idx != string::npos) {
      s = s.substr(idx + cookie_name.length() + 1);
      idx=s.find(";");
      if (idx != string::npos) {
        s = s.substr(0, idx);
      }
      idx=s.find(":");
      if (idx != string::npos) {
        s = s.substr(0, idx);
      }
    } else {
      s = "";
    }
  }
  return s;
}

string duser() {
  return cookie_value("duser");
}

string session_ID() {
  string s = cookie_value("sess");
  if (s.length() != 20) {
    s = "";
  }
  return s;
}

string twiki_name() {
  return cookie_value("twikiname");
}

list<string> accesses_from_cookie() {
  list<string> l; // return value
  string s;
  auto *env = getenv("HTTP_COOKIE");
  if (env != nullptr) {
    s = env;
  }
  if (!s.empty()) {
    auto idx = s.find("duser=");
    if (idx != string::npos) {
      l.emplace_back("<g>");
      s = s.substr(idx + 6);
      idx=s.find(";");
      if (idx != string::npos) {
        s = s.substr(0, idx);
      }
      auto sp = split(s, ":");
      sp.pop_front();
      for (const auto& access : sp) {
        l.emplace_back(access);
      }
    }
  }
  return l;
}

bool has_access(string acode) {
  auto l = accesses_from_cookie();
  for (const auto& e : l) {
    if (e == acode) {
      return true;
    }
  }
  return false;
}

void set_cookie(string cookie_name, string cookie_value, string domain, string
    path, DateTime *expiration_datetime) {
  cout << "Set-Cookie: " << cookie_name << "=" << cookie_value << "; domain=" <<
      domain << "; path=" << path << "; secure;";
  if (expiration_datetime != nullptr) {
    cout << " expires=" << expiration_datetime->to_string("%a, %d-%h-%Y "
        "%HH:%MM:%SS") << " GMT;";
  }
  cout << endl;
}

void clear_cookie(string cookie_name, string domain, string path) {
  cout << "Set-Cookie: " << cookie_name << "=; domain=" << domain << "; path="
      << path << "; expires=Mon, 1-Jan-2007 0:00:00 GMT;" << endl;
}
