#ifndef SPELLCHECKER_H
#define  SPELLCHECKER_H

#include <unordered_set>
#include <PostgreSQL.hpp>

class SpellChecker {
public:
  SpellChecker() = delete;
  SpellChecker(PostgreSQL::DBconfig db_config);
  ~SpellChecker();
  int check(std::string text);
  std::vector<std::string> misspelled_words() { return misspelled_words_; }

protected:
  void add_misspelled_word_to_list(std::string word, std::unordered_set<std::
      string>& misspelled_unique_table);
  void add_to_hash(PostgreSQL::Server& server, std::string db_table, std::
      string column, std::unordered_set<std::string>& hash);
  bool check_word(std::string word);
  std::string clean_word(const std::string& word);
  void do_spell_check(const std::string& text, std::string separator, std::
      string (*func)(const std::string& word), std::unordered_set<std::string>&
      valids_table, bool check_as_lower);
  void fill_text(std::string& text, bool include_previous = false);

  struct Valids {
    Valids() : words(), acronyms(), exact_matches(), units(), file_exts() { }

    std::unordered_set<std::string> words, acronyms, exact_matches, units,
        file_exts;
  } valids_tables;
  std::vector<std::string> misspelled_words_;
};

namespace spellcheckutils {

extern std::string trim_front(const std::string& word);
extern std::string trim_back(const std::string& word);
extern std::string trim_both(const std::string& word);
extern std::string trim_plural(const std::string& word);
extern std::string trim_punctuation(const std::string& word);

} // end namespace spellcheckutils

#endif
