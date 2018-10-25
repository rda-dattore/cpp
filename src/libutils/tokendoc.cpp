#include <fstream>
#include <string>
#include <regex>
#include <sys/stat.h>
#include <tokendoc.hpp>
#include <strutils.hpp>

TokenDocument::TokenDocument(std::string filename,size_t indent_length) : capacity(),blocks(),ifs(),repeats(),replacements(),indent("")
{
  clear_all();
  std::string lf("\n");
  struct stat buf;
  if (stat(filename.c_str(),&buf) == 0) {
    capacity=buf.st_size*2;
    std::ifstream ifs;
    ifs.open(filename.c_str());
    if (ifs.is_open()) {
	char line[32768];
	ifs.getline(line,32768);
	while (!ifs.eof()) {
	  if (line[0] == '#') {
	    blocks.emplace_back(build_block(ifs,line));
	  }
	  else if (line[0] != '@') {
	    Block block;
	    block.line=(line+lf);
	    blocks.emplace_back(block);
	  }
	  ifs.getline(line,32768);
	}
	ifs.close();
    }
  }
  set_indent(indent_length);
}

void TokenDocument::add_if(std::string condition)
{
  if (ifs.find(condition) == ifs.end()) {
    ifs.emplace(condition);
  }
}

void TokenDocument::add_repeat(std::string key,std::string entry)
{
  TokenDocument::RepeatEntry re;
  if (!repeats.found(key,re)) {
    re.key=key;
    re.list.reset(new std::vector<std::string>);
    repeats.insert(re);
  }
  re.list->emplace_back(entry);
}

void TokenDocument::add_replacement(std::string key,std::string replacement)
{
  TokenDocument::ReplaceEntry re;
  re.key=key;
  re.replacement.reset(new std::string);
  replacements.insert(re);
  *re.replacement=replacement;
}

void TokenDocument::clear_all()
{
  clear_ifs();
  clear_repeats();
  clear_replacements();
  blocks.clear();
}

std::ostream& operator<<(std::ostream& outs,const TokenDocument& source)
{
  auto s=const_cast<TokenDocument&>(source);
  std::string doc;
  doc.reserve(source.capacity*2);
  for (const auto& block : source.blocks) {
    if (block.type == TokenDocument::BlockType::_line) {
	doc+=source.indent+block.line;
    }
    else {
	doc+=s.block_to_string(block);
    }
  }
  for (const auto& key : s.replacements.keys()) {
    TokenDocument::ReplaceEntry re;
    s.replacements.found(key,re);
    strutils::replace_all(doc,key,*re.replacement);
  }
  outs << doc.substr(0,doc.length()-1);
  return outs;
}

std::string TokenDocument::block_to_string(const TokenDocument::Block& block,std::string alternate_match_string)
{
  std::string s;
  if (block.type == BlockType::_if) {
    if ((block.token[0] == '!' && ifs.find(block.token.substr(1)) == ifs.end()) || ifs.find(block.token) != ifs.end() || (!alternate_match_string.empty() && std::regex_search(alternate_match_string,std::regex(block.token)))) {
	for (auto& b : block.blocks) {
	  if (b.type == BlockType::_line) {
	    s+=indent+b.line;
	  }
	  else {
	    s+=block_to_string(b,alternate_match_string);
	  }
	}
    }
  }
  else if (block.type == BlockType::_else) {
    if (ifs.find(block.token) == ifs.end()) {
	for (auto& b : block.blocks) {
	  if (b.type == BlockType::_line) {
	    s+=indent+b.line;
	  }
	  else {
	    s+=block_to_string(b,alternate_match_string);
	  }
	}
    }
  }
  else if (block.type == BlockType::_repeat) {
    RepeatEntry re;
    if (repeats.found(block.token,re)) {
	std::regex regex("\\[!\\]");
	for (auto& item : *re.list) {
	  std::string repeat_block;
	  for (auto& b : block.blocks) {
	    if (b.type == BlockType::_line) {
		repeat_block+=indent+b.line;
	    }
	    else {
		repeat_block+=block_to_string(b,item);
	    }
	  }
	  if (std::regex_search(item,regex)) {
	    auto rb=repeat_block;
	    auto iparts=strutils::split(item,"<!>");
	    for (auto& part : iparts) {
		auto r=strutils::split(part,"[!]");
		strutils::replace_all(rb,block.token+"."+r[0],r[1]);
	    }
	    s+=rb;
	  }
	  else {
	    s+=strutils::substitute(repeat_block,block.token,item);
	  }
	}
    }
    else if (!alternate_match_string.empty()) {
	auto tparts=strutils::split(block.token,".");
	if (tparts.size() > 1) {
	  auto iparts=strutils::split(alternate_match_string,"<!>");
	  auto regex=std::regex("^"+tparts[1]);
	  for (auto& part : iparts) {
	    std::string repeat_block;
	    if (std::regex_search(part,regex)) {
		for (auto& b : block.blocks) {
		  if (b.type == BlockType::_line) {
		    repeat_block+=indent+b.line;
		  }
		  else {
		    repeat_block+=block_to_string(b,alternate_match_string);
		  }
		}
		auto r=strutils::split(part,"[!]");
		strutils::replace_all(repeat_block,block.token,r[1]);
	    }
	    s+=repeat_block;
	  }
	}
    }
  }
  return s;
}

TokenDocument::Block TokenDocument::build_block(std::ifstream& ifs,std::string line)
{
  Block block;
  if (line.substr(1,2) == "IF") {
    block.type=BlockType::_if;
    block.token=line.substr(3);
    strutils::trim(block.token);
    fill_block(ifs,block,"IF");
  }
  else if (line.substr(1,4) == "ELSE") {
    block.type=BlockType::_else;
    block.token=line.substr(5);
    strutils::trim(block.token);
    fill_block(ifs,block,"ELSE");
  }
  else if (line.substr(1,6) == "REPEAT") {
    block.type=BlockType::_repeat;
    block.token=line.substr(7);
    strutils::trim(block.token);
    fill_block(ifs,block,"REPEAT");
  }
  else if (line[0] != '@') {
    block.line=(line+"\n");
  }
  return block;
}

void TokenDocument::fill_block(std::ifstream& ifs,TokenDocument::Block& block,std::string directive)
{
  std::string lf("\n");
  char line[32768];
  ifs.getline(line,32768);
  while (!ifs.eof()) {
    std::string sline=line;
    if (std::regex_search(sline,std::regex("^#END"+directive)) && std::regex_search(sline,std::regex(block.token))) {
	break;
    }
    else if (line[0] == '#') {
	block.blocks.emplace_back(build_block(ifs,line));
    }
    else if (line[0] != '@') {
	Block b;
	b.line=(line+lf);
	block.blocks.emplace_back(b);
    }
    ifs.getline(line,32768);
  }
}

void TokenDocument::set_indent(size_t indent_length)
{
  indent.resize(indent_length,' ');
}
