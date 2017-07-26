#ifndef SPELLCHECKER_H
#define  SPELLCHECKER_H

#include <mymap.hpp>
#include <MySQL.hpp>

class SpellChecker
{
public:
  SpellChecker();
  ~SpellChecker();
  int check(std::string text);
  std::list<std::string> misspelled_words() { return misspelled_words_; }

protected:
  struct WordEntry {
    WordEntry() : key() {}

    std::string key;
  };
  void add_misspelled_word_to_list(std::string word,my::map<WordEntry>& misspelled_unique_table);
  void add_to_hash(MySQL::Server& server,std::string db_table,std::string column,my::map<WordEntry>& hash);
  bool check_word(const std::string& word);
  std::string clean_word(const std::string& word);
  void do_spell_check(const std::string& text,std::string separator,std::string (*func)(const std::string& word),my::map<WordEntry>& valids_table,bool check_as_lower);
  void fill_text(std::string& text,bool include_previous = false);

  struct Valids {
    Valids() : words(),acronyms(),exact_matches(),units() {}

    my::map<WordEntry> words,acronyms,exact_matches,units;
  } valids_tables;
  std::list<std::string> misspelled_words_;
};

namespace SpellCheckerFunctions {

extern std::string trim_front(const std::string& word);
extern std::string trim_back(const std::string& word);
extern std::string trim_both(const std::string& word);
extern std::string trim_plural(const std::string& word);
extern std::string trim_punctuation(const std::string& word);

} // end namespace SpellCheckerFunctions

#endif
