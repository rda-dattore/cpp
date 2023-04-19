#include <MySQL.hpp>

namespace searchutils {


extern std::string cleaned_search_word(std::string& word, bool& ignore);
extern std::string convert_to_expandable_summary(std::string summary, size_t
    visible_length, size_t& iterator);
extern std::string horizontal_resolution_keyword(double
    max_horizontal_resolution, size_t resolution_type);
extern std::string root_of_word(std::string word);
extern std::string time_resolution_keyword(std::string frequency_type, int
    number, std::string unit, std::string statistics);

extern bool index_locations(MySQL::Server& server, std::string dsnum, std::
    string& error);
extern bool indexed_variables(MySQL::Server& server, std::string dsnum, std::
    string& error);
extern bool inserted_word_into_search_wordlist(MySQL::Server& server, const
    std::string& table, std::string dsnum, std::string word, size_t& location,
    std::string& error);
extern bool is_compound_term(std::string word, std::string& separator);

} // end namespace searchutils
