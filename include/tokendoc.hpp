#ifndef TOKENDOC_HPP
#define   TOKENDOC_HPP

#include <string>
#include <deque>
#include <list>
#include <mymap.hpp>

class TokenDocument {
public:
  TokenDocument() = delete;
  TokenDocument(std::string filename,size_t indent_length = 0);
  void add_if(std::string condition);
  void add_repeat(std::string key,std::string entry);
  void add_replacement(std::string key,std::string replacement);
  void clear_all();
  void clear_ifs() { ifs.clear(); }
  void clear_repeats() { repeats.clear(); }
  void clear_replacements() { replacements.clear(); }
  void set_indent(size_t indent_length);

  friend std::ostream& operator<<(std::ostream& outs,const TokenDocument& source);

private:
  enum class BlockType {_line,_if,_else,_repeat};
  struct Block {
    Block() : type(BlockType::_line),token(),blocks(),line() {}

    BlockType type;
    std::string token;
    std::deque<Block> blocks;
    std::string line;
  };
  struct IfEntry {
    IfEntry() : key() {}

    std::string key;
  };
  struct RepeatEntry {
    RepeatEntry() : key(),list(nullptr) {}

    std::string key;
    std::shared_ptr<std::deque<std::string>> list;
  };
  struct ReplaceEntry {
    ReplaceEntry() : key(),replacement() {}

    std::string key;
    std::shared_ptr<std::string> replacement;
  };

  std::string block_to_string(const Block& block,std::string alternate_match_string = "");
  Block build_block(std::ifstream& ifs,std::string line);
  void fill_block(std::ifstream& ifs,Block& block,std::string directive);

  size_t capacity;
  std::deque<Block> blocks;
  my::map<IfEntry> ifs;
  my::map<RepeatEntry> repeats;
  my::map<ReplaceEntry> replacements;
  std::string indent;
};

#endif
