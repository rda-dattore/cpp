#include <regex>
#include <spellchecker_pg.hpp>
#include <PostgreSQL.hpp>
#include <strutils.hpp>

using namespace PostgreSQL;
using std::regex;
using std::regex_search;
using std::string;
using std::unordered_set;
using strutils::chop;
using strutils::is_alpha;
using strutils::is_alphanumeric;
using strutils::is_numeric;
using strutils::occurs;
using strutils::replace_all;
using strutils::split;
using strutils::substitute;
using strutils::to_lower;
using strutils::to_upper;
using strutils::trim;

SpellChecker::SpellChecker() : valids_tables(), misspelled_words_() {
  Server server("rda-db.ucar.edu", "metadata", "metadata", "rdadb");
  if (server) {
    add_to_hash(server, "metautil.word_valids", "word", valids_tables.words);
    add_to_hash(server, "metautil.acronym_valids", "word", valids_tables.
        acronyms);
    add_to_hash(server, "metautil.acronym_valids", "description", valids_tables.
        exact_matches);
    add_to_hash(server, "metautil.place_valids", "word", valids_tables.
        exact_matches);
    add_to_hash(server, "metautil.name_valids", "word", valids_tables.
        exact_matches);
    add_to_hash(server, "metautil.other_exactmatch_valids", "word",
        valids_tables.exact_matches);
    add_to_hash(server, "metautil.unit_valids", "word", valids_tables.units);
    add_to_hash(server, "metautil.non_english_valids", "word", valids_tables.
        words);
    add_to_hash(server, "metautil.file_ext_valids", "word", valids_tables.
        file_exts);
    server.disconnect();
  }
}

SpellChecker::~SpellChecker() {
  valids_tables.words.clear();
  valids_tables.acronyms.clear();
  valids_tables.exact_matches.clear();
  valids_tables.units.clear();
}

int SpellChecker::check(string text) {
  auto check_text = text;
  replace_all(check_text, "\n", " ");
  trim(check_text);
  replace_all(check_text, "\xe2\x80\x90\x00", "-");

  // check the words case-insensitive against the regular valids
  do_spell_check(check_text, "", nullptr, valids_tables.words, true);

  // check the words directly against the acronyms
  if (!misspelled_words_.empty()) {
    fill_text(check_text);
    do_spell_check(check_text, "", spellcheckutils::trim_plural, valids_tables.
        acronyms, false);
  }

  // check the words directly against exact match valids with all punctuation
  //   trimmed
  if (!misspelled_words_.empty()) {
    fill_text(check_text);
    do_spell_check(check_text, "", nullptr, valids_tables.exact_matches, false);
  }

  // check compound (hyphen) words with all punctuation trimmed and
  //   case-insensitive against the regular valids
  if (!misspelled_words_.empty() && check_text.find("-") != string::npos) {
    fill_text(check_text);
    do_spell_check(check_text, "-", nullptr, valids_tables.words, true);
  }

  // check compound (unicode En-dash) words with all punctuation trimmed and
  //   case-insensitive against the regular valids
  if (!misspelled_words_.empty() && check_text.find("\xe2\x80\x93") != string::
      npos) {
    fill_text(check_text);
    do_spell_check(check_text, "\xe2\x80\x93\x00", nullptr, valids_tables.words,
        true);
  }

  // check compound (slash) words (e.g. 'parameters/levels') with all
  //   punctuation trimmed and case-insensitive against the regular valids
  if (!misspelled_words_.empty()) {
    fill_text(check_text);
    do_spell_check(check_text, "/", nullptr, valids_tables.words, true);
  }

  // check compound (slash) words with all punctuation trimmed directly against
  //   acronyms
  if (!misspelled_words_.empty() && check_text.find("/") != string::npos) {
    fill_text(check_text);
    do_spell_check(check_text, "/", nullptr, valids_tables.acronyms, false);
  }

  // check the words directly against units valids with all punctuation trimmed
  if (!misspelled_words_.empty()) {
    check_text = text;
    replace_all(check_text, "\n", " ");
    trim(check_text);
    fill_text(check_text, true);
    do_spell_check(check_text, "", nullptr, valids_tables.units, false);
  }

  // check snake_case words with all punctuation trimmed and case-insensitive
  //   against the regular valids
  if (!misspelled_words_.empty() && check_text.find("_") != string::npos) {
    fill_text(check_text);
    do_spell_check(check_text, "_", nullptr, valids_tables.words, true);
  }
  return misspelled_words_.size();
}

void SpellChecker::add_to_hash(Server& server, string db_table, string column,
    unordered_set<string>& hash) {
  Query query(column, db_table);
  if (query.submit(server) == 0) {
    for (const auto& row : query) {
      auto wordlist = split(row[0]);
      for (const auto& word : wordlist) {
        if (hash.find(word) == hash.end()) {
          hash.emplace(word);
        }
      }
    }
  }
}

bool SpellChecker::check_word(string word) {
  if (word.empty()) {
    return false;
  }
  if (word[0] == '#') {
    word = word.substr(1);
  }

  // ignore numbers
  if (is_numeric(word)) {
    return false;
  }

  // ignore acronyms containing all capital letters and numbers
  if (is_alphanumeric(word) && word == to_upper(word)) {
    return false;
  }

  // ignore floating point numbers
  if (word.find(".") != string::npos && is_numeric(substitute(word, ".", ""))) {
    return false;
  }
  if (word.find(",") != string::npos && is_numeric(substitute(word, ", ",""))) {
    return false;
  }

  // ignore numbers like 9:3
  if (word.find(":") != string::npos && is_numeric(substitute(word, ":", ""))) {
    return false;
  }

  // ignore e.g. 1900s
  if (regex_search(word, regex("s$")) && is_numeric(word.substr(0, word.length()
      -1))) {
    return false;
  }
  if (regex_search(word, regex("st$")) && is_numeric(word.substr(0,
      word.length()-2)) && word[word.length()-3] == '1') {
    return false;
  }
  if (regex_search(word, regex("nd$")) && is_numeric(word.substr(0,
      word.length()-2)) && word[word.length()-3] == '2') {
    return false;
  }
  if (regex_search(word, regex("rd$")) && is_numeric(word.substr(0,
      word.length()-2)) && word[word.length()-3] == '3') {
    return false;
  }
  if (regex_search(word, regex("th$")) && is_numeric(word.substr(0,
      word.length()-2)) && (word[word.length()-3] == '0' || (word[word.length()
      -3] >= '4' && word[word.length()-3] <= '9'))) {
    return false;
  }

  // ignore NG-GDEX dataset IDs
  if (word[0] == 'd' && word.length() == 7 && is_numeric(word.substr(1))) {
    return false;
  }

  // ignore e.g. pre-1950
  if (word.find("pre-") == 0 && is_numeric(word.substr(4))) {
    return false;
  }

  // ignore version numbers e.g. v2.0, 0.x, 1a
  if (word.find("v") == 0 && is_numeric(substitute(word.substr(1), ".", ""))) {
    return false;
  }

  // ignore NCAR Technical notes
  if (regex_search(word,regex("^NCAR/TN-([0-9]){1,}\\+STR$"))) {
    return false;
  }
  if (regex_search(word,regex("\\.x$"))) {
    if (is_numeric(word.substr(0,word.length()-2))) {
      return false;
    }
  }
  if (word.front() >= '0' && word.front() <= '9') {
    size_t n = 1;
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
  if (word.length() == 2 && regex_search(word,regex("[\\.)]$")) && ((word[0] >=
      'a' && word[0] <= 'z') || (word[0] >= 'A' && word[0] <= 'Z'))) {
    return false;
  }

  // ignore references
  if (word.length() == 3 && regex_search(word,regex("^\\(.\\)$")) && ((word[1]
      >= 'a' && word[1] <= 'z') || (word[1] >= 'A' && word[1] <= 'Z') || (word[
      1] >= '1' && word[1] <= '9'))) {
    return false;
  }
  if (word.front() == '(' && word.back() == ')') {
    auto c1 = word[1];
    if (c1 == 'i' || c1 == 'v' || c1 == 'x') {
      auto c2 = word[word.length()-2];
      if (c2 == 'i' || c2 == 'v' || c2 == 'x') {
        return false;
      }
    }
  }
  auto c1 = word.front();
  if (c1 == 'I' || c1 == 'V' || c1 == 'X') {
    auto c2 = word.back();
    if (c2 == 'I' || c2 == 'V' || c2 == 'X') {
      return false;
    }
  }

  // ignore email addresses
  if (occurs(word, "@") == 1 && occurs(word.substr(word.find("@")+1), ".") >=
      1) {
    return false;
  }

  // ignore file extensions
  if (regex_search(word, regex("^\\.([a-zA-Z0-9]){1,10}$"))) {
    return false;
  }

  // ignore file names
  auto idx = word.rfind(".");
  if (idx != string::npos) {
    auto file_ext = word.substr(idx+1);
    if (valids_tables.file_exts.find(file_ext) != valids_tables.file_exts.
        end()) {
      return false;
    }
  }

  // ignore acronyms like TS1.3B.4C
  if (word.find(".") != string::npos) {
    for (const auto& c : word) {
      if (c != '.' && (c < 'A' || c > 'Z') && (c < '0' || c > '9')) {
        return true;
      }
    }
    return false;
  }
  return true;
}

string SpellChecker::clean_word(const string& word) {
  auto cword = spellcheckutils::trim_punctuation(word); // return value
  auto cleaned_word = false;
  if (cword[0] == '"' || cword[0] == '\'') {
    cword = cword.substr(1);
    cleaned_word = true;
  }
  if (cword[0] == '(' && (cword.back() == ')' || cword.find(")") == string::
      npos)) {
    cword = cword.substr(1);
    cleaned_word = true;
  }
  if (cword.back() == ',' || cword.back() == '"' || cword.back() == '\'') {
    chop(cword);
    cleaned_word = true;
  }
  if (regex_search(cword, regex("\\)$")) && !regex_search(cword, regex(
      "\\(s\\)$"))) {
    chop(cword);
    cleaned_word = true;
  }
  if (regex_search(cword, regex("\\)\\.$"))) {
    chop(cword, 2);
    cleaned_word = true;
  }
  if (cword[cword.length()-2] == '-' && cword[cword.length()-1] >= 'A' && cword[
      cword.length()-1] <= 'Z') {
    cword = cword.substr(0, cword.length()-2);
    cleaned_word = true;
  }
  if (cleaned_word) {
    cword = clean_word(cword);
  }
  return cword;
}

void SpellChecker::add_misspelled_word_to_list(string word, unordered_set<
    string>& misspelled_unique_table) {
  auto cw = clean_word(word);
  if (misspelled_unique_table.find(cw) == misspelled_unique_table.end()) {
    misspelled_words_.emplace_back(cw);
    misspelled_unique_table.emplace(cw);
  }
}

void SpellChecker::do_spell_check(const string& text, string separator, string
    (*func)(const string& word), unordered_set<string>& valids_table, bool
    check_as_lower) {
  unordered_set<string> misspelled_unique_table;
  misspelled_words_.clear();
  auto words = split(text);
  auto start = 0;
  auto checking_units = false;
  if (&valids_table == &valids_tables.units) {
    start = 1;
    checking_units = true;
  }
  for (size_t n = start; n < words.size(); ++n) {
    auto key = clean_word(words[n]);
    if (func != nullptr) {
      if (key.back() == '.' && valids_table.find(key) != valids_table.end()) {
        key = "X";
      } else {
        key = func(key);
      }
    }
    if (check_word(key)) {
      if (key.find("non-") == 0) {
        key = key.substr(4);
      }
      if (check_as_lower) {
        key = to_lower(key);
      }
      if (separator.empty()) {
        if (!checking_units) {
          if (!regex_search(key, regex("^\\[http")) && !regex_search(key, regex(
              "^http://")) && !regex_search(key, regex("^https://")) &&
              !regex_search(key, regex("^ftp://")) && !regex_search(key, regex(
              "^\\[mailto:")) && valids_table.find(key) == valids_table.end()) {
            if (regex_search(key, regex("'s$"))) {
              chop(key, 2);
              if (valids_table.find(key) == valids_table.end()) {
                add_misspelled_word_to_list(words[n], misspelled_unique_table);
              }
            } else {
              add_misspelled_word_to_list(words[n], misspelled_unique_table);
            }
          }
        } else {
          if (valids_table.find(key) != valids_table.end()) {
            string pword = "XX";
            if (n > 0) {
              pword = spellcheckutils::trim_front(words[n-1]);
            }
            if ((pword != "et" || key != "al") && !is_numeric(pword)) {
              add_misspelled_word_to_list(words[n], misspelled_unique_table);
            }
          } else {
            if (n > 0) {
              if (key.length() == 1 && is_alpha(key) && key == to_upper(key) &&
                  to_lower(words[n-1]) == "station") {

                // allow for e.g. 'Station P'
              } else if (spellcheckutils::trim_front(to_lower(words[n-1])) ==
                  "version") {

                // allow for e.g. 'version 1e'
              } else {
                add_misspelled_word_to_list(words[n], misspelled_unique_table);
              }
            } else {
              add_misspelled_word_to_list(words[n], misspelled_unique_table);
            }
          }
          ++n;
        }
      } else {
        auto cw_parts = split(key, separator);
        for (size_t m = 0; m < cw_parts.size(); ++m) {
          if (check_word(cw_parts[m]) && valids_table.find(cw_parts[m]) ==
              valids_table.end()) {
            if (regex_search(cw_parts[m], regex("'s$"))) {
              if (valids_table.find(cw_parts[m].substr(0, cw_parts[m].length()
                  -2)) == valids_table.end()) {
                add_misspelled_word_to_list(words[n], misspelled_unique_table);
                m = cw_parts.size();
              }
            } else {
              add_misspelled_word_to_list(words[n], misspelled_unique_table);
              m = cw_parts.size();
            }
          }
        }
      }
    }
  }
}

void SpellChecker::fill_text(string& text, bool include_previous) {
  if (include_previous) {
    auto words = split(text);
    text = "";
    auto it = misspelled_words_.begin();
    auto end = misspelled_words_.end();
    if (words[0] == *it) {
      text = "XX " + *it;
      ++it;
    }
    for (size_t n = 1; n < words.size(); ++n) {
      if (it == end) {
        break;
      }
      if (words[n] == *it || clean_word(words[n]) == *it || spellcheckutils::
          trim_punctuation(words[n]) == *it || spellcheckutils::trim_both(words[
          n]) == *it) {
        text += " " + words[n-1] + " " + *it;
        ++it;
      }
    }
  } else {
    text = "";
    for (const auto& word : misspelled_words_) {
      text += " " + word;
    }
  }
  trim(text);
}

namespace spellcheckutils {

string trim_front(const string& word) {
  auto trimmed_word = word;
  auto n = 0;
  auto c = trimmed_word[n];
  while (n < static_cast<int>(trimmed_word.length()) && (c < '0' || (c > '9' &&
      c < 'A') || (c > 'Z' && c < 'a') || c > 'z')) {
    ++n;
    c = trimmed_word[n];
  }
  return trimmed_word.substr(n);
}

string trim_back(const string& word) {
  auto trimmed_word = word;
  auto c = trimmed_word[trimmed_word.length()-1];
  while (trimmed_word.length() > 0 && (c < '0' || (c > '9' && c < 'A') || (c >
      'Z' && c < 'a') || c > 'z')) {
    chop(trimmed_word);
    c = trimmed_word[trimmed_word.length()-1];
  }
  return trimmed_word;
}

string trim_both(const string& word) { 
  auto trimmed_word = word;
  trimmed_word = trim_front(trimmed_word);
  trimmed_word = trim_back(trimmed_word);
  return trimmed_word;
}

string trim_plural(const string& word) {
  auto trimmed_word = word;
  if (regex_search(trimmed_word, regex("'s$"))) {
    chop(trimmed_word, 2);
  } else if (regex_search(trimmed_word, regex("\xe2\x80\x99s$"))) {
    chop(trimmed_word, 4);
  } else if (regex_search(trimmed_word, regex("s$"))) {
    chop(trimmed_word);
  }
  return trimmed_word;
}

string trim_punctuation(const string& word) {
  auto trimmed_word = word;
  auto c = trimmed_word.back();
  while (c == '.' || c == ',' || c == ':' || c == ';' || c == '?' || c == '!') {
    chop(trimmed_word);
    c = trimmed_word.back();
  }
  return trimmed_word;
}

} // end namespace spellcheckutils
