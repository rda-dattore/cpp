#include <fstream>
#include <string>
#include <regex>
#include <sys/stat.h>
#include <tokendoc.hpp>
#include <strutils.hpp>
#include <myexception.hpp>

using std::ifstream;
using std::move;
using std::ostream;
using std::pair;
using std::regex;
using std::regex_search;
using std::smatch;
using std::stod;
using std::stoll;
using std::string;
using std::to_string;
using std::unordered_set;
using std::vector;
using strutils::append;
using strutils::itos;
using strutils::replace_all;
using strutils::split;
using strutils::substitute;
using strutils::trim;

const string TokenDocument::REPEAT_DELIMITER = "<!>";
const string TokenDocument::PAIR_DELIMITER = "[!]";

TokenDocument::TokenDocument(string filename, size_t indent_length) :
    capacity(), blocks(), ifs(), repeats(), replacements(), indent("") {
  reset(filename, indent_length);
}

void TokenDocument::add_if(string condition) {
  if (ifs.find(condition) == ifs.end()) {
    ifs.emplace(condition);
  }
}

void TokenDocument::add_repeat(string key, string entry) {
  if (repeats.find(key) == repeats.end()) {
    repeats.emplace(key, vector<string>());
  }
  repeats[key].emplace_back(entry);
}

void TokenDocument::add_repeat(string key, REPEAT_PAIRS repeat_pairs) {
  string s;
  for (const auto& e : repeat_pairs) {
    append(s, e.first + PAIR_DELIMITER + e.second, REPEAT_DELIMITER);
  }
  add_repeat(key, s);
}

void TokenDocument::add_replacement(string key, string replacement) {
  replacements.emplace(key, replacement);
}

void TokenDocument::clear_all() {
  clear_ifs();
  clear_repeats();
  clear_replacements();
  blocks.clear();
}

ostream& operator<<(ostream& outs, const TokenDocument& source) {
  string doc;
  doc.reserve(source.capacity * 2);
  for (const auto& block : source.blocks) {
    if (block.type == TokenDocument::BlockType::_line) {
      doc += source.indent + block.line;
    } else {
      doc += source.block_to_string(block);
    }
  }
  for (const auto& e : source.replacements) {
    replace_all(doc, e.first, e.second);
  }
  outs << doc.substr(0, doc.length() - 1);
  return outs;
}

string TokenDocument::else_block_to_string(const TokenDocument::Block& block,
    string alternate_match_string) const {
  string s; // return value
  if (ifs.find(block.token) == ifs.end()) {
    for (auto& b : block.blocks) {
      if (b.type == BlockType::_line) {
        s += indent + b.line;
      } else {
        s += block_to_string(b, alternate_match_string);
      }
    }
  }
  return s;
}

string TokenDocument::if_block_to_string(const TokenDocument::Block& block,
    string alternate_match_string) const {
  string s; // return value
  if ((block.token[0] == '!' && ifs.find(block.token.substr(1)) == ifs.end()) ||
      ifs.find(block.token) != ifs.end() || (!alternate_match_string.empty() &&
      regex_search(alternate_match_string, regex(block.token)))) {
    for (auto& b : block.blocks) {
      if (b.type == BlockType::_line) {
        s += indent + b.line;
      } else {
        s += block_to_string(b, alternate_match_string);
      }
    }
  }
  return s;
}

string TokenDocument::repeat_block_substrings_to_string(const string&
    repeat_item, const string& repeat_block, const string& token) const {
  string s = repeat_block; // return value
  auto rb = repeat_block;
  smatch parts;
  auto re_if = regex("!REPIF((.){1,}\\.(.){1,})\n");
  unordered_set<string> rep_ifs;
  while (regex_search(rb, parts, re_if) && parts.ready()) {
    rep_ifs.emplace(parts[1].str());
    rb = parts.suffix();
  }
  auto sp = split(repeat_item, REPEAT_DELIMITER);
  for (const auto& i : rep_ifs) {
    auto key = i.substr(i.find(".") + 1) + PAIR_DELIMITER;
    auto b = false;
    for (const auto& p : sp) {
      if (p.substr(0, key.length()) == key) {
        b = true;
        break;
      }
    }
    if (b) {
      replace_all(s, "!REPIF" + i + "\n", "");
      replace_all(s, "!ENDREPIF" + i + "\n", "");
    } else {
      auto idx1 = s.find("!REPIF" + i + "\n");
      while (idx1 != string::npos) {
        auto idx2 = s.find("!ENDREPIF" + i + "\n") + 10 + i.length();
        s = s.substr(0, idx1) + s.substr(idx2);
        idx1 = s.find("!REPIF" + i + "\n");
      }
    }
  }
  for (auto& p : sp) {
    auto sp2 = split(p, PAIR_DELIMITER);
    replace_all(s, token + "." + sp2[0], sp2[1]);
  }
  return s;
}

double eval(string expression) {
  smatch parts;
  if (regex_search(expression, parts, regex("^.*(\\+|\\-|\\*|\\/|%).*$")) &&
      parts.ready()) {
    auto idx = expression.find(parts[1].str());
    switch (parts[1].str().front()) {
      case '+': {
        return stod(expression.substr(0, idx)) + stod(expression.substr(idx +
            1));
      }
      case '-': {
        return stod(expression.substr(0, idx)) - stod(expression.substr(idx +
            1));
      }
      case '*': {
        return stod(expression.substr(0, idx)) * stod(expression.substr(idx +
            1));
      }
      case '/': {
        auto a = expression.substr(0, idx);
        auto b = expression.substr(idx + 1);
        if (a.find(".") == string::npos && b.find(".") == string::npos) {
          return stoll(a) / stoll(b);
        } else if (a.find(".") == string::npos) {
          return stoll(a) / stod(b);
        } else if (b.find(".") == string::npos) {
          return stod(a) / stoll(b);
        }
        return stod(a) / stod(b);
      }
      case '%': {
        return stoll(expression.substr(0, idx)) % stoll(expression.substr(idx +
            1));
      }
    }
  }
  return stod(expression);
}

string TokenDocument::repeat_block_to_string(const TokenDocument::Block& block,
    string alternate_match_string) const {
  string s; // return value
  auto e = repeats.find(block.token);
  if (e != repeats.end()) {
    auto re_sub = regex("\\[!\\]");
    auto loop_count = 0;
    for (auto& item : e->second) {
      string s2;
      for (auto& b : block.blocks) {
        if (b.type == BlockType::_line) {
          s2 += indent + b.line;
        } else {
          s2 += block_to_string(b, item);
        }
      }
      replace_all(s2, "$LOOP_COUNT", to_string(loop_count));

      // check for expressions denoted by "$(...)"
      auto idx = s2.find("$(");
      if (idx != string::npos) {
        auto e = s2.substr(idx);
        idx = e.find(")");
        if (idx != string::npos) {
          e = e.substr(0, idx + 1);
          auto d = eval(e.substr(2, e.length() - 3));
          if (d == floor(d)) {
            replace_all(s2, e, itos(d));
          } else {
            replace_all(s2, e, to_string(d));
          }
        }
      }
      if (regex_search(item, re_sub)) {
        s += repeat_block_substrings_to_string(item, s2, block.token);
      } else {
        s += substitute(s2, block.token, item);
      }
      ++loop_count;
    }
  } else if (!alternate_match_string.empty()) {
    auto tparts = split(block.token, ".");
    if (tparts.size() > 1) {
      auto iparts = split(alternate_match_string, REPEAT_DELIMITER);
      auto re = regex("^" + tparts[1]);
      for (auto& part : iparts) {
        string s2;
        if (regex_search(part, re)) {
          for (auto& b : block.blocks) {
            if (b.type == BlockType::_line) {
              s2 += indent + b.line;
            } else {
              s2 += block_to_string(b, alternate_match_string);
            }
          }
          auto r = split(part, PAIR_DELIMITER);
          replace_all(s2, block.token, r[1]);
        }
        s += s2;
      }
    }
  }
  return s;
}

string TokenDocument::block_to_string(const TokenDocument::Block& block, string
    alternate_match_string) const {
  if (block.type == BlockType::_if) {
    return if_block_to_string(block, alternate_match_string);
  } else if (block.type == BlockType::_else) {
    return else_block_to_string(block, alternate_match_string);
  } else if (block.type == BlockType::_repeat) {
    return repeat_block_to_string(block, alternate_match_string);
  } else {
    return "";
  }
}

TokenDocument::Block TokenDocument::build_block(ifstream& ifs, string line) {
  Block block;
  if (line.substr(1, 2) == "IF") {
    block.type = BlockType::_if;
    block.token = line.substr(3);
    trim(block.token);
    fill_block(ifs, block, "IF");
  } else if (line.substr(1, 4) == "ELSE") {
    block.type = BlockType::_else;
    block.token = line.substr(5);
    trim(block.token);
    fill_block(ifs, block, "ELSE");
  } else if (line.substr(1, 6) == "REPEAT") {
    block.type = BlockType::_repeat;
    block.token = line.substr(7);
    trim(block.token);
    fill_block(ifs, block, "REPEAT");
  } else if (line[0] != '@') {
    block.line = move(line);
    if (block.line.back() == '\\') {
      block.line.pop_back();
    } else {
      block.line += "\n";
    }
  }
  return block;
}

void TokenDocument::fill_block(ifstream& ifs, TokenDocument::Block& block,
    string directive) {
  char line[32768];
  ifs.getline(line, 32768);
  while (!ifs.eof()) {
    string sline = line;
    if (regex_search(sline, regex("^#END" + directive)) && regex_search(sline,
        regex(block.token))) {
      break;
    } else if (line[0] == '#') {
      block.blocks.emplace_back(build_block(ifs, line));
    } else if (line[0] != '@') {
      Block b;
      b.line = line;
      if (b.line.back() == '\\') {
        b.line.pop_back();
      } else {
        b.line += "\n";
      }
      block.blocks.emplace_back(b);
    }
    ifs.getline(line, 32768);
  }
}

void TokenDocument::reset(string filename, size_t indent_length) {
  clear_all();
  struct stat buf;
  if (stat(filename.c_str(), &buf) == 0) {
    capacity = buf.st_size * 2;
    ifstream ifs;
    ifs.open(filename.c_str());
    if (ifs.is_open()) {
      char line[32768];
      ifs.getline(line, 32768);
      while (!ifs.eof()) {
        if (line[0] == '#') {
          blocks.emplace_back(build_block(ifs, line));
        } else if (line[0] != '@') {
          Block block;
          block.line = line;
          if (block.line.back() == '\\') {
            block.line.pop_back();
          } else {
            block.line += "\n";
          }
          blocks.emplace_back(block);
        }
        ifs.getline(line, 32768);
      }
      ifs.close();
    } else {
      throw my::OpenFailed_Error("file '" + filename + "' cannot be opened");
    }
  } else {
    throw my::NotFound_Error("file '" + filename + "' does not exist");
  }
  set_indent(indent_length);
}

void TokenDocument::set_indent(size_t indent_length) {
  indent.resize(indent_length, ' ');
}

void TokenDocuments::add(string filename, string id) {
  auto iter = docs.find(id);
  if (iter == docs.end()) {
    docs.emplace(id, filename);
  } else {
    iter->second.reset(filename);
  }
}

TokenDocument& TokenDocuments::operator[](string id) {
  auto iter = docs.find(id);
  if (iter == docs.end()) {
    throw my::NotFound_Error("no TokenDocument for id '" + id + "' exists");
  }
  return iter->second;
}

ostream& operator<<(ostream& outs, const TokenDocuments& source) {
  auto n = 0;
  for (const auto& e : source.docs) {
    if (n++ > 0) {
      outs << "\n";
    }
    outs << e.second;
  }
  return outs;
}
