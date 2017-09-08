#include <regex>
#include <spellchecker.hpp>
#include <MySQL.hpp>
#include <strutils.hpp>

SpellChecker::SpellChecker() : valids_tables(),misspelled_words_()
{
  MySQL::Server server("rda-db.ucar.edu","metadata","metadata","");
  if (server) {
    add_to_hash(server,"metautil.word_valids","word",valids_tables.words);
    add_to_hash(server,"metautil.acronym_valids","word",valids_tables.acronyms);
    add_to_hash(server,"metautil.acronym_valids","description",valids_tables.exact_matches);
    add_to_hash(server,"metautil.place_valids","word",valids_tables.exact_matches);
    add_to_hash(server,"metautil.name_valids","word",valids_tables.exact_matches);
    add_to_hash(server,"metautil.other_exactmatch_valids","word",valids_tables.exact_matches);
    add_to_hash(server,"metautil.unit_valids","word",valids_tables.units);
    add_to_hash(server,"metautil.non_english_valids","word",valids_tables.words);
    server.disconnect();
  }
}

SpellChecker::~SpellChecker()
{
  valids_tables.words.clear();
  valids_tables.acronyms.clear();
  valids_tables.exact_matches.clear();
  valids_tables.units.clear();
}

int SpellChecker::check(std::string text)
{
  auto check_text=text;
  strutils::replace_all(check_text,"\n"," ");
  strutils::trim(check_text);
// check the words against the valids
  do_spell_check(check_text,"",NULL,valids_tables.words,true);
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check the words directly against the acronyms
    do_spell_check(check_text,"",SpellCheckerFunctions::trim_plural,valids_tables.acronyms,false);
  }
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check the words directly against exact match valids
    do_spell_check(check_text,"",NULL,valids_tables.exact_matches,false);
  }
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check words with punctuation trimmed from the front directly against valids
    do_spell_check(check_text,"-",SpellCheckerFunctions::trim_front,valids_tables.words,true);
  }
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check words with punctuation trimmed from the back directly against valids
    do_spell_check(check_text,"-",SpellCheckerFunctions::trim_back,valids_tables.words,true);
  }
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check words with all punctuation trimmed directly against valids
    do_spell_check(check_text,"-",SpellCheckerFunctions::trim_both,valids_tables.words,true);
  }
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check words with punctuation trimmed from the front directly against acronyms
    do_spell_check(check_text,"/",SpellCheckerFunctions::trim_front,valids_tables.acronyms,false);
  }
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check words with punctuation trimmed from the back directly against acronyms
    do_spell_check(check_text,"/",SpellCheckerFunctions::trim_back,valids_tables.acronyms,false);
  }
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check words with all punctuation trimmed directly against acronyms
    do_spell_check(check_text,"/",SpellCheckerFunctions::trim_both,valids_tables.acronyms,false);
  }
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check the words directly against exact match valids, front punctuation trimmed
    do_spell_check(check_text,"",SpellCheckerFunctions::trim_front,valids_tables.exact_matches,false);
  }
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check the words directly against exact match valids, back punctuation trimmed
    do_spell_check(check_text,"",SpellCheckerFunctions::trim_back,valids_tables.exact_matches,false);
  }
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check the words directly against exact match valids, all punctuation trimmed
    do_spell_check(check_text,"",SpellCheckerFunctions::trim_both,valids_tables.exact_matches,false);
  }
  if (misspelled_words_.size() > 0) {
    check_text=text;
    strutils::replace_all(check_text,"\n"," ");
    strutils::trim(check_text);
    fill_text(check_text,true);
// check the words directly against units valids, all punctuation trimmed
    do_spell_check(check_text,"",SpellCheckerFunctions::trim_both,valids_tables.units,false);
  }
  if (misspelled_words_.size() > 0) {
    fill_text(check_text);
// check words like 'parameters/levels'
    do_spell_check(check_text,"/",SpellCheckerFunctions::trim_both,valids_tables.words,true);
  }
  return misspelled_words_.size();
}

void SpellChecker::add_to_hash(MySQL::Server& server,std::string db_table,std::string column,my::map<WordEntry>& hash)
{
  MySQL::Query query(column,db_table);
  if (query.submit(server) == 0) {
    MySQL::Row row;
    while (query.fetch_row(row)) {
	auto wordlist=strutils::split(row[0]);
	for (const auto& word : wordlist) {
	  WordEntry we;
	  if (!hash.found(word,we)) {
	    we.key=word;
	    hash.insert(we);
	  }
	}
    }
  }
}

bool SpellChecker::check_word(const std::string& word)
{
// ignore numbers
  if (strutils::is_numeric(word)) {
    return false;
  }
// ignore acronyms containing all capital letters and numbers
  if (strutils::is_alphanumeric(word) && word == strutils::to_upper(word)) {
    return false;
  }
// ignore floating point numbers
  if (strutils::contains(word,".") && strutils::is_numeric(strutils::substitute(word,".",""))) {
    return false;
  }
  if (strutils::contains(word,",") && strutils::is_numeric(strutils::substitute(word,",",""))) {
    return false;
  }
// ignore numbers like 9:3
  if (strutils::contains(word,":") && strutils::is_numeric(strutils::substitute(word,":",""))) {
    return false;
  }
// ignore e.g. 1900s
  if (std::regex_search(word,std::regex("s$")) && strutils::is_numeric(word.substr(0,word.length()-1))) {
    return false;
  }
  if (std::regex_search(word,std::regex("st$")) && strutils::is_numeric(word.substr(0,word.length()-2)) && word[word.length()-3] == '1') {
    return false;
  }
  if (std::regex_search(word,std::regex("nd$")) && strutils::is_numeric(word.substr(0,word.length()-2)) && word[word.length()-3] == '2') {
    return false;
  }
  if (std::regex_search(word,std::regex("rd$")) && strutils::is_numeric(word.substr(0,word.length()-2)) && word[word.length()-3] == '3') {
    return false;
  }
  if (std::regex_search(word,std::regex("th$")) && strutils::is_numeric(word.substr(0,word.length()-2)) && (word[word.length()-3] == '0' || (word[word.length()-3] >= '4' && word[word.length()-3] <= '9'))) {
    return false;
  }
// ignore RDA dataset numbers
  if (std::regex_search(word,std::regex("^ds")) && word.length() == 7 && word[5] == '.' && strutils::is_numeric(strutils::substitute(strutils::substitute(word,".",""),"ds",""))) {
    return false;
  }
// ignore e.g. pre-1950
  if (std::regex_search(word,std::regex("^pre-")) && strutils::is_numeric(word.substr(4))) {
    return false;
  }
// ignore version numbers e.g. v2.0, 0.x, 1a
  if (std::regex_search(word,std::regex("^v")) && strutils::is_numeric(strutils::substitute(word.substr(1),".",""))) {
    return false;
  }
// ignore NCAR Technical notes
  if (std::regex_search(word,std::regex("^NCAR/TN-([0-9]){1,}\\+STR$"))) {
    return false;
  }
  if (std::regex_search(word,std::regex("\\.x$"))) {
    if (strutils::is_numeric(word.substr(0,word.length()-2))) {
	return false;
    }
  }
  if (word.front() >= '0' && word.front() <= '9') {
    size_t n=1;
    while (n < word.length() && word[n] >= '0' && word[n] <= '9') {
	++n;
    }
    if (word[n] >= 'a' && word[n] <= 'z') {
	++n;
	while (n < word.length() && word[n] >= 'a' && word[n] <= 'z') {
	  ++n;
	}
	if (n == word.length()) {
	  return false;
	}
    }
  }
// ignore itemizations
  if (word.length() == 2 && std::regex_search(word,std::regex("\\.$")) && ((word[0] >= 'a' && word[0] <= 'z') || (word[0] >= 'A' && word[0] <= 'Z'))) {
    return false;
  }
  if (word.front() == '(' && word.back() == ')') {
    auto c1=word[1];
    if (c1 == 'i' || c1 == 'v' || c1 == 'x') {
	auto c2=word[word.length()-2];
	if (c2 == 'i' || c2 == 'v' || c2 == 'x') {
	  return false;
	}
    }
  }
  auto c1=word.front();
  if (c1 == 'I' || c1 == 'V' || c1 == 'X') {
    auto c2=word.back();
    if (c2 == 'I' || c2 == 'V' || c2 == 'X') {
	return false;
    }
  }
// ignore email addresses
  if (strutils::occurs(word,"@") == 1 && strutils::occurs(word.substr(word.find("@")+1),".") >= 1) {
    return false;
  }
// ignore file extensions
  if (std::regex_search(word,std::regex("^\\.([a-zA-Z0-9]){1,10}$"))) {
    return false;
  }
// ignore acronyms like TS1.3B.4C
  if (strutils::contains(word,".")) {
    for (const auto& c : word) {
	if (c != '.' && (c < 'A' || c > 'Z') && (c < '0' || c > '9')) {
	  return true;
	}
    }
    return false;
  }
  return true;
}

std::string SpellChecker::clean_word(const std::string& word)
{
  auto cleaned_word=word;
  if (std::regex_search(cleaned_word,std::regex("^\\("))) {
    cleaned_word=cleaned_word.substr(1);
  }
  if (std::regex_search(cleaned_word,std::regex(",$"))) {
    strutils::chop(cleaned_word);
  }
  if (std::regex_search(cleaned_word,std::regex("\\)$")) && !std::regex_search(cleaned_word,std::regex("\\(s\\)$"))) {
    strutils::chop(cleaned_word);
  }
  if (std::regex_search(cleaned_word,std::regex("\\)\\.$"))) {
    strutils::chop(cleaned_word,2);
  }
  if (cleaned_word[cleaned_word.length()-2] == '-' && cleaned_word[cleaned_word.length()-1] >= 'A' && cleaned_word[cleaned_word.length()-1] <= 'Z') {
    cleaned_word=cleaned_word.substr(0,cleaned_word.length()-2);
  }
  return cleaned_word;
}

void SpellChecker::add_misspelled_word_to_list(std::string word,my::map<WordEntry>& misspelled_unique_table)
{
  auto cw=clean_word(word);
  WordEntry we;
  if (!misspelled_unique_table.found(cw,we)) {
    misspelled_words_.emplace_back(cw);
    we.key=cw;
    misspelled_unique_table.insert(we);
  }
}

void SpellChecker::do_spell_check(const std::string& text,std::string separator,std::string (*func)(const std::string& word),my::map<WordEntry>& valids_table,bool check_as_lower)
{
  my::map<WordEntry> misspelled_unique_table;
  misspelled_words_.clear();
  auto words=strutils::split(text);
  auto start=0;
  auto checking_units=false;
  if (&valids_table == &valids_tables.units) {
    start=1;
    checking_units=true;
  }
  for (size_t n=start; n < words.size(); ++n) {
    WordEntry we;
    we.key=words[n];
    if (func != NULL) {
	we.key=func(we.key);
    }
    if (check_word(we.key)) {
	if (std::regex_search(we.key,std::regex("^non-"))) {
	  we.key=we.key.substr(4);
	}
	if (check_as_lower) {
	  we.key=strutils::to_lower(we.key);
	}
	if (separator.length() == 0) {
	  if (!checking_units) {
	    if (!std::regex_search(we.key,std::regex("^\\[http")) && !std::regex_search(we.key,std::regex("^http://")) && !std::regex_search(we.key,std::regex("^https://")) && !std::regex_search(we.key,std::regex("^ftp://")) && !std::regex_search(we.key,std::regex("^\\[mailto:")) && !valids_table.found(we.key,we)) {
		if (std::regex_search(we.key,std::regex("'s$"))) {
		  strutils::chop(we.key,2);
		  if (!valids_table.found(we.key,we)) {
		    add_misspelled_word_to_list(words[n],misspelled_unique_table);
		  }
		}
		else {
		  add_misspelled_word_to_list(words[n],misspelled_unique_table);
		}
	    }
	  }
	  else {
	    if (valids_table.found(we.key,we)) {
		if (n == 0 || !strutils::is_numeric(SpellCheckerFunctions::trim_front(words[n-1]))) {
		  add_misspelled_word_to_list(words[n],misspelled_unique_table);
		}
	    }
	    else {
		if (n > 0) {
		  if (we.key.length() == 1 && strutils::is_alpha(we.key) && we.key == strutils::to_upper(we.key) && strutils::to_lower(words[n-1]) == "station") {
// allow for e.g. 'Station P'
		  }
		  else if (SpellCheckerFunctions::trim_front(strutils::to_lower(words[n-1])) == "version") {
// allow for e.g. 'version 1e'
		  }
		  else {
		    add_misspelled_word_to_list(words[n],misspelled_unique_table);
		  }
		}
		else {
		  add_misspelled_word_to_list(words[n],misspelled_unique_table);
		}
	    }
	    ++n;
	  }
	}
	else {
	  auto cw_parts=strutils::split(we.key,separator);
	  for (size_t m=0; m < cw_parts.size(); ++m) {
	    if (check_word(cw_parts[m]) && !valids_table.found(cw_parts[m],we)) {
		if (std::regex_search(cw_parts[m],std::regex("'s$"))) {
		  if (!valids_table.found(cw_parts[m].substr(0,cw_parts[m].length()-2),we)) {
		    add_misspelled_word_to_list(words[n],misspelled_unique_table);
		    m=cw_parts.size();
		  }
		}
		else {
		  add_misspelled_word_to_list(words[n],misspelled_unique_table);
		  m=cw_parts.size();
		}
            }
          }
	}
    }
  }
}

void SpellChecker::fill_text(std::string& text,bool include_previous)
{
  if (include_previous) {
    auto words=strutils::split(text);
    text="";
    auto it=misspelled_words_.begin();
    auto end=misspelled_words_.end();
    if (words[0] == *it) {
	text="XX "+*it;
	it++;
    }
    for (size_t n=1; n < words.size(); ++n) {
	if (it == end) {
	  break;
	}
	if (words[n] == *it || clean_word(words[n]) == *it || SpellCheckerFunctions::trim_punctuation(words[n]) == *it || SpellCheckerFunctions::trim_both(words[n]) == *it) {
	  text+=" "+words[n-1]+" "+*it;
	  ++it;
	}
    }
  }
  else {
    text="";
    for (const auto& word : misspelled_words_)
    text+=" "+word;
  }
  strutils::trim(text);
}

namespace SpellCheckerFunctions {

std::string trim_front(const std::string& word)
{
  auto trimmed_word=word;
  auto n=0;
  auto c=trimmed_word[n];
  while (n < static_cast<int>(trimmed_word.length()) && (c < '0' || (c > '9' && c < 'A') || (c > 'Z' && c < 'a') || c > 'z')) {
    ++n;
    c=trimmed_word[n];
  }
  return trimmed_word.substr(n);
}

std::string trim_back(const std::string& word)
{
  auto trimmed_word=word;
  auto c=trimmed_word[trimmed_word.length()-1];
  while (trimmed_word.length() > 0 && (c < '0' || (c > '9' && c < 'A') || (c > 'Z' && c < 'a') || c > 'z')) {
    strutils::chop(trimmed_word);
    c=trimmed_word[trimmed_word.length()-1];
  }
  return trimmed_word;
}

std::string trim_both(const std::string& word)
{ 
  auto trimmed_word=word;
  trimmed_word=trim_front(trimmed_word);
  trimmed_word=trim_back(trimmed_word);
  return trimmed_word;
}

std::string trim_plural(const std::string& word)
{
  auto trimmed_word=word;
  if (std::regex_search(trimmed_word,std::regex("s$"))) {
    strutils::chop(trimmed_word);
  }
  else if (std::regex_search(trimmed_word,std::regex("'s$"))) {
    strutils::chop(trimmed_word,2);
  }
  return trimmed_word;
}

std::string trim_punctuation(const std::string& word)
{
  auto trimmed_word=word;
  auto c=trimmed_word.back();
  while (c == '.' || c == ',' || c == ':' || c == ';' || c == '?' || c == '!') {
    strutils::chop(trimmed_word);
    c=trimmed_word.back();
  }
  return trimmed_word;
}

} // end namespace SpellCheckerFunctions
