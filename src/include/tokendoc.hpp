#ifndef TOKENDOC_HPP
#define   TOKENDOC_HPP

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class TokenDocument {
public:
  using REPEAT_PAIRS = std::vector<std::pair<std::string, std::string>>;

  TokenDocument() = delete;
  TokenDocument(std::string filename, size_t indent_length = 0);
  operator bool() const { return !blocks.empty(); }
  void add_if(std::string condition);
  void add_repeat(std::string key, std::string entry);
  void add_repeat(std::string key, REPEAT_PAIRS repeat_pairs);
  void add_replacement(std::string key, std::string replacement);
  void clear_all();
  void clear_ifs() { ifs.clear(); }
  void clear_repeats() { repeats.clear(); }
  void clear_replacements() { replacements.clear(); }
  void reset(std::string filename, size_t indent_length = 0);
  void set_indent(size_t indent_length);

  friend std::ostream& operator<<(std::ostream& outs, const TokenDocument&
      source);

private:
  static const std::string REPEAT_DELIMITER;
  static const std::string PAIR_DELIMITER;
  enum class BlockType {_line, _if, _else, _repeat};

  struct Block {
    Block() : type(BlockType::_line), token(), blocks(), line() { }

    BlockType type;
    std::string token;
    std::vector<Block> blocks;
    std::string line;
  };

  std::string block_to_string(const Block& block, std::string
      alternate_match_string = "") const;
  std::string else_block_to_string(const Block& block, std::string
      alternate_match_string) const;
  std::string if_block_to_string(const Block& block, std::string
      alternate_match_string) const;
  std::string repeat_block_substrings_to_string(const std::string&
      repeat_item, const std::string& repeat_block, const std::string& token)
      const;
  std::string repeat_block_to_string(const Block& block, std::string
      alternate_match_string) const;
  Block build_block(std::ifstream& ifs,std::string line);
  void fill_block(std::ifstream& ifs, Block& block, std::string directive);

  size_t capacity;
  std::vector<Block> blocks;
  std::unordered_set<std::string> ifs;
  std::unordered_map<std::string, std::vector<std::string>> repeats;
  std::unordered_map<std::string, std::string> replacements;
  std::string indent;
};

class TokenDocuments {
public:
  TokenDocuments() : docs() { }
  void operator=(const TokenDocuments& source) = delete;
  void add(std::string filename, std::string id);
  TokenDocument& operator[](std::string id);

  friend std::ostream& operator<<(std::ostream& outs, const TokenDocuments&
      source);

private:
  std::map<std::string, TokenDocument> docs;
};

#endif
