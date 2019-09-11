// FILE; citation.h

#ifndef CITATION_H
#define  CITATION_H

#include <MySQL.hpp>
#include <xml.hpp>

namespace citation {

const std::string DATA_CENTER="Research Data Archive at the National Center for Atmospheric Research, Computational and Information Systems Laboratory";

extern void export_to_ris(std::ostream& ofs,std::string dsnum,std::string access_date,MySQL::Server& server);
extern void export_to_bibtex(std::ostream& ofs,std::string dsnum,std::string access_date,MySQL::Server& server);

extern std::string citation(std::string dsnum,std::string style,std::string access_date,MySQL::Server& server,bool htmlized);
extern std::string list_authors(XMLSnippet& xsnip,MySQL::Server& server,size_t max_num_authors,std::string et_al_string,bool last_first_middle_on_first_author_only = true);
extern std::string temporary_citation(std::string dsnum,std::string style,std::string access_date,MySQL::Server& server,bool htmlized);

} // end namespace citation

#endif
