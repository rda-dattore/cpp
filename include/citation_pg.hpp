// FILE citation.hpp

#ifndef CITATION_H
#define  CITATION_H

#include <PostgreSQL.hpp>
#include <xml.hpp>

namespace citation {

const std::string DATA_CENTER = "Research Data Archive at the National Center "
    "for Atmospheric Research, Computational and Information Systems "
    "Laboratory";
const std::string DOI_URL_BASE = "https://doi.org";

extern void export_to_bibtex(std::ostream& ofs, std::string dsnum, std::string
    access_date, PostgreSQL::Server& server);
extern void export_to_ris(std::ostream& ofs, std::string dsnum, std::string
    access_date, PostgreSQL::Server& server);

extern std::string add_citation_style_changer(std::string style);
extern std::string agu_citation(std::string dsnum, std::string access_date,
    XMLSnippet& xsnip, PostgreSQL:: Server& server, const PostgreSQL::Row& row,
    bool htmlized);
extern std::string ams_citation(std::string dsnum, std::string access_date,
    XMLSnippet& xsnip, PostgreSQL:: Server& server, const PostgreSQL::Row& row,
    bool htmlized);
extern std::string citation(std::string dsnum, std::string style, std::string
    access_date, PostgreSQL::Server& server, bool htmlized);
extern std::string copernicus_citation(std::string dsnum, std::string
    access_date, XMLSnippet& xsnip, PostgreSQL:: Server& server, const
    PostgreSQL::Row& row, bool htmlized);
extern std::string esip_citation(std::string dsnum, std::string access_date,
    XMLSnippet& xsnip, PostgreSQL:: Server& server, const PostgreSQL::Row& row,
    bool htmlized);
extern std::string formatted_access_date(std::string access_date, bool embedded,
    bool htmlized);
extern std::string list_authors(XMLSnippet& xsnip, PostgreSQL::Server& server,
    size_t max_num_authors, std::string et_al_string, bool
    last_first_middle_on_first_author_only = true);
extern std::string temporary_citation(std::string dsnum, std::string style,
    std::string access_date, PostgreSQL::Server& server, bool htmlized);

extern bool open_ds_overview(XMLDocument& xdoc, std::string dsnum);

} // end namespace citation

#endif
