#include <iostream>
#include <string>
#include <PostgreSQL.hpp>
#include <search_pg.hpp>
#include <strutils.hpp>

using namespace PostgreSQL;
using std::deque;
using std::string;
using std::to_string;
using strutils::append;
using strutils::ds_aliases;
using strutils::replace_all;
using strutils::split;
using strutils::strand;
using strutils::to_sql_tuple_string;

namespace searchutils {

bool inserted_word_into_search_wordlist(Server& server, const string& table,
    string dsid, string word, size_t& location, string uflg, string& error) {
  error = "";
  bool ignore;
  auto sword = cleaned_search_word(word, ignore);
  auto iloc = to_string(location);
  if (!ignore) {
    if (server.insert(
          table,
          "word, sword, location, dsid, uflg",
          "'" + word + "', '" + strutils::soundex(sword) + "', " + iloc + ", '"
              + dsid + "', '" + uflg + "'",
          "(word, location, dsid) do update set sword = excluded.sword, uflg = "
              "excluded.uflg"
          ) < 0) {
      error = error_message(server.error(), table, word, location);
      return false;
    }
    LocalQuery query("select a, b from search.cross_reference where a = '" +
        word + "' union select b, a from search.cross_reference where b = '" +
        word + "'");
    if (query.submit(server) < 0) {
      error = error_message(query.error(), table, word, location);
      return false;
    }
    for (const auto& row : query) {
      auto word = row[1];
      sword = cleaned_search_word(word, ignore);
      if (!ignore) {
        if (server.insert(
              table,
              "word, sword, location, dsid, uflg",
              "'" + word + "', '" + strutils::soundex(sword) + "', " + iloc +
                  ", '" + dsid + "', '" + uflg + "'",
              "(word, location, dsid) do update set sword = excluded.sword, "
                  "uflg = excluded.uflg"
              ) < 0) {
          error = error_message(server.error(), table, word, location);
          return false;
        }
      }
    }
    deque<string> word_parts;
    if (word.find("-") != string::npos) {
      word_parts = split(word, "-");
    } else if (word.find("/") != string::npos) {
      word_parts = split(word, "/");
    }
    string cword;
    for (size_t m = 0; m < word_parts.size(); ++m) {
      cword += word_parts[m];
      if (word_parts[m].empty()) {
        cword = "ignore" + cword;
      }
      auto word = word_parts[m];
      sword = cleaned_search_word(word, ignore);
      if (!ignore) {
        if (server.insert(
              table,
              "word, sword, location, dsid, uflg",
              "'" + word + "', '" + strutils::soundex(sword) + "', " + iloc +
                  ", '" + dsid + "', '" + uflg + "'",
              "(word, location, dsid) do update set sword = excluded.sword, "
                  "uflg = excluded.uflg"
              ) < 0) {
          error = error_message(server.error(), table, word, location);
          return false;
        }
        query.set("select a, b from search.cross_reference where a = '" + word +
            "' union select b, a from search.cross_reference where b = '" + word
            + "'");
        if (query.submit(server) < 0) {
          error = error_message(query.error(), table, word, location);
          return false;
        }
        for (const auto& row : query) {
          auto word = row[1];
          sword = cleaned_search_word(word, ignore);
          if (!ignore) {
            if (server.insert(
                  table,
                  "word, sword, location, dsid, uflg",
                  "'" + word + "', '" + strutils::soundex(sword) + "', " + iloc
                      + ", '" + dsid + "', '" + uflg + "'",
                  "(word, location, dsid) do update set sword = excluded."
                      "sword, uflg = excluded.uflg"
                  ) < 0) {
              error = error_message(server.error(), table, word, location);
              return false;
            }
          }
        }
      }
    }
    if (!word_parts.empty()) {
      if (!strutils::has_beginning(cword, "ignore")) {
        sword = cleaned_search_word(cword, ignore);
        if (!ignore) {
          if (server.insert(
                table,
                "word, sword, location, dsid, uflg",
                "'" + cword + "', '" + strutils::soundex(sword) + "', " + iloc +
                    ", '" + dsid + "', '" + uflg + "'",
                "(word, location, dsid) do update set sword = excluded.sword, "
                    "uflg = excluded.uflg"
                ) < 0) {
            error = error_message(server.error(), table, word, location);
            return false;
          }
        }
      }
    }
    ++location;
  }
  return true;
}

bool indexed_variables(Server& server, string dsid, string& error) {
  error = "";
  auto ds_set = to_sql_tuple_string(ds_aliases(dsid));

  // get GCMD variables
  LocalQuery query("select k.path from search.variables as v left join search."
      "gcmd_sciencekeywords as k on k.uuid = v.keyword where v.dsid in " +
      ds_set + " and v.vocabulary = 'GCMD'");
  if (query.submit(server) < 0) {
    error = query.error();
    return false;
  }
  string varlist;
  for (const auto& row : query) {
    auto keyword = row[0];
    replace_all(keyword, "EARTH SCIENCE > ", "");
    append(varlist, keyword, " ");
  }

  // get CMDMAP variables (mapped from content metadata)
  query.set("keyword", "search.variables", "dsid in " + ds_set + " and "
      "vocabulary = 'CMDMAP'");
  if (query.submit(server) < 0) {
    error = query.error();
    return false;
  }

  // if no CMDMAP variables, get CMDMM variables (specified with metadata
  //     manager)
  if (query.num_rows() == 0) {
    query.set("keyword", "search.variables", "dsid in " + ds_set + " and "
        "vocabulary = 'CMDMM'");
    if (query.submit(server) < 0) {
      error = query.error();
      return false;
    }
  }
  for (const auto& row : query) {
    append(varlist, row[0], " ");
  }
  auto vars = split(varlist);
  size_t m = 0;
  auto uflg = strand(3);
  for (const auto& var : vars) {
    if (!inserted_word_into_search_wordlist(server, "search.variables_wordlist",
        dsid, var, m, uflg, error)) {
      return false;
    }
  }
  server._delete("search.variables_wordlist", "dsid in " + ds_set + " and uflg "
      "!= '" + uflg + "'");
  return true;
}

bool indexed_locations(Server& server, string dsid, string& error) {
  error = "";
  auto ds_set = to_sql_tuple_string(ds_aliases(dsid));

  // get locations
  Query query("keyword", "search.locations", "dsid in " + ds_set);
  if (query.submit(server) < 0) {
    error = query.error();
    return false;
  }
  string loclist;
  for (const auto& row : query) {
    append(loclist, row[0], " ");
  }
  auto locs = split(loclist);
  size_t m = 0;
  auto uflg = strand(3);
  for (const auto& loc : locs) {
    if (!inserted_word_into_search_wordlist(server, "search.locations_wordlist",
        dsid, loc, m, uflg, error)) {
      if (error.find("Duplicate entry") == string::npos) {
        return false;
      }
    }
  }
  server._delete("search.locations_wordlist", "dsid in " + ds_set + " and uflg "
      "!= '" + uflg + "'");
  server.disconnect();
  return true;
}

} // end namespace searchutils
