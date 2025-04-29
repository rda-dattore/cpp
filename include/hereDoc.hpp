#ifndef HEREDOC_HPP
#define   HEREDOC_HPP

#include <string>
#include <deque>
#include <list>
#include <mymap.hpp>

namespace hereDoc {

struct IfEntry {
  IfEntry() : key() {}

  std::string key;
};
struct RepeatEntry {
  RepeatEntry() : key(),list(nullptr) {}

  std::string key;
  std::shared_ptr<std::deque<std::string>> list;
};
struct Tokens {
  Tokens() : replaces(),ifs(),repeats() {}
  void clear() {
    replaces.clear();
    ifs.clear();
    repeats.clear();
  }

  std::list<std::string> replaces;
  my::map<IfEntry> ifs;
  my::map<RepeatEntry> repeats;
};
extern void print(std::string filename,Tokens* tokens = NULL);
extern void print(std::string filename,Tokens* tokens,std::ostream& ofs,size_t indent_length = 0);
extern void print2(std::string filename,Tokens* tokens,std::ostream& ofs,size_t indent_length = 0);

} // end namespace hereDoc

#endif
