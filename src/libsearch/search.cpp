#include <iostream>
#include <string>
#include <MySQL.hpp>
#include <search.hpp>
#include <strutils.hpp>

using namespace MySQL;
using std::deque;
using std::string;
using std::to_string;
using strutils::append;
using strutils::chop;
using strutils::is_alpha;
using strutils::has_ending;
using strutils::occurs;
using strutils::replace_all;
using strutils::split;
using strutils::strand;
using strutils::to_lower;
using strutils::trim;

namespace searchutils {

bool is_compound_term(string word, string& separator) {
  if (word.find("-") != string::npos) {
    separator = "-";
    return true;
  } else if (word.find("/") != string::npos) {
    separator = "/";
    return true;
  } else if (word.find(";") != string::npos) {
    separator = "/";
    return true;
  }
  return false;
}

void chop_end(string& word, string ending) {
  if (has_ending(word, ending)) {
    word = word.substr(0, word.length() - ending.length());
  }
}

string root_of_word(string word) {
  auto rword = word;
  if (!is_alpha(rword)) {
    return "";
  }
  chop_end(rword, "is");
  chop_end(rword, "es");
  chop_end(rword, "s");
  chop_end(rword, "ing");
  chop_end(rword, "ity");
  chop_end(rword, "ian");
  chop_end(rword, "ly");
  chop_end(rword, "al");
  chop_end(rword, "ic");
  chop_end(rword, "ed");
  chop_end(rword, "ous");
  auto n = rword.length() - 1;
  while (n > 0 && (rword[n] == rword[n - 1] || (rword[n] >= '0' && rword[n] <=
      '9'))) {
    chop(rword);
    --n;
  }
  return rword;
}

string cleaned_search_word(string& word, bool& ignore) {
  ignore = false;
  trim(word);
  word = to_lower(word);
  replace_all(word, "'", "\\'");

  // ignore full tags
  if (word.find("<") == 0 && has_ending(word, ">")) {
    ignore = true;
    return "";
  }

  // strip tags from word
  while (word.length() > 0 && word.find("<") != string::npos && word.find(">")
      != string::npos) {
    auto idx1 = word.find("<");
    auto idx2 = word.find(">");
    if (idx1 < idx2) {
      auto tag = word.substr(idx1);
      tag = tag.substr(0, tag.find(">") + 1);
      replace_all(word, tag, "");
    } else {
      word = word.substr(idx1);
    }
  }

  // ignore partial tags
  if (word.find("<") == 0) {
    ignore = true;
    return "";
  }
  word = word.substr(word.find(">") + 1);

  // strip punctuation
  if (has_ending(word, ".") && occurs(word, ".") == 1) {
    chop(word);
  }
  while (word.find(",") == 0 || word.find(":") == 0 || word.find("'") == 0 ||
      word.find("\"") == 0 || word.find("\\") == 0) {
    word = word.substr(1);
  }
  while (has_ending(word, ",") || has_ending(word, ":") || has_ending(word, "'")
      || has_ending(word, "\"") || has_ending(word, "\\")) {
    chop(word);
  }

  // strip out special characters
  if (word.find("(") == 0) {
    word = word.substr(1);
  }
  if (has_ending(word, ")")) {
    chop(word);
  }
  if (word.find("(") != string::npos) {
    word += ")";
  }
  auto idx = word.find(")");
  if (idx != string::npos && idx < (word.length() - 1)) {
    word = "(" + word;
  }
  if (word.find("\"") == 0) {
    word = word.substr(1);
  }
  if (word.find("'") == 0) {
    word = word.substr(1);
  }
  if (has_ending(word, "\\'s")) {
    word=word.substr(0, word.length() - 3);
  }

  // strip punctuation again
  if (has_ending(word, ".") && occurs(word, ".") == 1) {
    chop(word);
  }
  while (has_ending(word, ",") || has_ending(word, ":") || has_ending(word, "'")
      || has_ending(word, "\"") || has_ending(word, "\\") || has_ending(word,
      "+")) {
    chop(word);
  }
  if (word.empty()) {
    ignore = true;
    return "";
  }
  auto rword = word;
  int n = rword.length()-1;
  while (n > 0 && rword[n] >= '0' && rword[n] < '9') {
    chop(rword);
    --n;
  }
  if (!rword.empty() && is_alpha(rword)) {
    rword = root_of_word(rword);
  } else {

    // ignore ds numbers
    if (word.length() == 7 && word.find("ds") == 0 && word[5] == '.') {
      ignore = true;
      return "";
    }
    rword = "";
  }
  return rword;
}

string error_message(string sql_error, string table, string word, size_t
     location) {
  std::stringstream error; // return value
  error << sql_error << ", table='" << table << "', word='" << word <<
      "', location=" << location << std::endl;
  return error.str();
}

bool inserted_word_into_search_wordlist(Server& server, const string& table,
    string dsnum, string word, size_t& location, string uflg, string& error) {
  error = "";
  bool ignore;
  auto sword = cleaned_search_word(word, ignore);
  auto iloc = to_string(location);
  if (!ignore) {
    if (server.insert(
          table,
          "word, sword, location, dsid, uflg",
          "'" + word + "', '" + strutils::soundex(sword) + "', " + iloc + ", '"
              + dsnum + "', '" + uflg + "'",
          "update uflg = values(uflg)"
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
                  ", '" + dsnum + "', '" + uflg + "'",
              "update uflg = values(uflg)"
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
    } else {
      word_parts = split("", "");
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
                  ", '" + dsnum + "', '" + uflg + "'",
              "update uflg = values(uflg)"
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
                      + ", '" + dsnum + "', '" + uflg + "'",
                  "update uflg = values(uflg)"
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
                    ", '" + dsnum + "', '" + uflg + "'",
                "update uflg = values(uflg)"
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

bool indexed_variables(Server& server, string dsnum, string& error) {
  error = "";

  // get GCMD variables
  LocalQuery query("select k.path from search.variables as v left join search."
      "gcmd_sciencekeywords as k on k.uuid = v.keyword where v.dsid = '" + dsnum
      + "' and v.vocabulary = 'GCMD'");
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
  query.set("keyword", "search.variables", "dsid = '" + dsnum + "' and "
      "vocabulary = 'CMDMAP'");
  if (query.submit(server) < 0) {
    error = query.error();
    return false;
  }

  // if no CMDMAP variables, get CMDMM variables (specified with metadata
  //     manager)
  if (query.num_rows() == 0) {
    query.set("keyword", "search.variables", "dsid = '" + dsnum + "' and "
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
        dsnum, var, m, uflg, error)) {
      return false;
    }
  }
  server._delete("search.variables_wordlist", "dsid = '" + dsnum + "' and uflg "
      "!= '" + uflg + "'");
  return true;
}

bool indexed_locations(Server& server, string dsnum, string& error) {
  error = "";

  // get locations
  Query query("keyword", "search.locations", "dsid = '" + dsnum + "'");
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
        dsnum, loc, m, uflg, error)) {
      if (error.find("Duplicate entry") == string::npos) {
        return false;
      }
    }
  }
  server._delete("search.locations_wordlist", "dsid = '" + dsnum + "' and uflg "
      "!= '" + uflg + "'");
  return true;
}

string convert_to_expandable_summary(string summary, size_t visible_length,
    size_t& iterator) {
  auto summary_parts = split(summary);
  string expandable_summary;
  if (summary_parts.size() < visible_length) {
    expandable_summary = summary;
    trim(expandable_summary);
    replace_all(expandable_summary, "<p>", "<span>");
    replace_all(expandable_summary, "</p>", "</span><br /><br />");
    if (has_ending(expandable_summary, "\n")) {
      expandable_summary = expandable_summary.substr(0, expandable_summary.
          length() - 13);
    } else {
      expandable_summary = expandable_summary.substr(0, expandable_summary.
          length() - 12);
    }
  } else {
    expandable_summary = "";
    auto in_tag = 0;
    size_t n = 0;
    for (size_t m = 1; n < visible_length; ++n, ++m) {
      if (summary_parts[n].find("</p>") != string::npos || (m < summary_parts.
          size() && summary_parts[m].find("</p>") != string::npos)) {
        break;
      }
      if (!expandable_summary.empty()) {
        expandable_summary += " ";
      }
      expandable_summary += summary_parts[n];
      in_tag += occurs(summary_parts[n], "<") - occurs(summary_parts[n], "</") *
          2 + occurs(summary_parts[n], "</p>") - occurs(summary_parts[n],
          "<p>");
    }
    if (in_tag > 0) {
      while (n < summary_parts.size() && in_tag > 0) {
        expandable_summary += " " + summary_parts[n];
        if (summary_parts[n].find("</") != string::npos) {
          --in_tag;
        }
        ++n;
      }
    }
    replace_all(expandable_summary, "<p>", "<span>");
    replace_all(expandable_summary, "</p>", "</span><br /><br />");
    string summary_remainder = "<span>";
    for (; n < summary_parts.size(); ++n) {
      summary_remainder += " " + summary_parts[n];
    }
    replace_all(summary_remainder, "<p>", "<span>");
    replace_all(summary_remainder, "</p>", "</span><br /><br />");
    trim(summary_remainder);
    if (has_ending(expandable_summary, ("<br /><br />\n"))) {
      expandable_summary = expandable_summary.substr(0, expandable_summary.
          length() - 13);
      summary_remainder = "<br /><br />" + summary_remainder;
    }
    if (summary_remainder.length() > 12) {
      expandable_summary += "<span id=\"D" + to_string(iterator) + "\">"
          "&nbsp;...&nbsp;<a href=\"javascript:swapDivs(" + to_string(iterator)
          + "," + to_string(iterator + 1) + ")\" title=\"Expand\"><img src=\""
          "/images/expand.gif\" width=\"11\" height=\"11\" border=\"0\"></a>"
          "</span></span><span style=\"visibility: hidden; position: absolute; "
          "left: 0px\" id=\"D" + to_string(iterator + 1) + "\">" +
          summary_remainder.substr(0, summary_remainder.length() - 12) + " <a "
          "href=\"javascript:swapDivs(" + to_string(iterator + 1) + "," +
          to_string(iterator) + ")\" title=\"Collapse\"><img src=\"/images/"
          "contract.gif\" width=\"11\" height=\"11\" border=\"0\"></a></span>";
    }
    if (has_ending(expandable_summary, "\n")) {
      expandable_summary = expandable_summary.substr(0, expandable_summary.
          length() - 1);
    }
    iterator += 2;
  }
  return expandable_summary;
}

string horizontal_resolution_keyword(double max_horizontal_resolution, size_t
    resolution_type) {

  // resolution_type = 0 for resolutions in degrees
  // resolution_type = 1 for resolutions in km
  string keyword;
  if ((resolution_type == 0 && max_horizontal_resolution >= 0.01 &&
      max_horizontal_resolution < 0.09) || (resolution_type == 1 &&
      max_horizontal_resolution >= 1. && max_horizontal_resolution < 10.)) {
    keyword = "H : 1 km - < 10 km or approximately .01 degree - < .09 degree";
  } else if ((resolution_type == 0 && max_horizontal_resolution >= 0.09 &&
      max_horizontal_resolution < 0.5) || (resolution_type == 1 &&
      max_horizontal_resolution >= 10. && max_horizontal_resolution < 50.)) {
    keyword = "H : 10 km - < 50 km or approximately .09 degree - < .5 degree";
  } else if ((resolution_type == 0 && max_horizontal_resolution >= 0.5 &&
      max_horizontal_resolution < 1.) || (resolution_type == 1 &&
      max_horizontal_resolution >= 50. && max_horizontal_resolution < 100.)) {
    keyword = "H : 50 km - < 100 km or approximately .5 degree - < 1 degree";
  } else if ((resolution_type == 0 && max_horizontal_resolution >= 1. &&
      max_horizontal_resolution < 2.5) || (resolution_type == 1 &&
      max_horizontal_resolution >= 100. && max_horizontal_resolution < 250.)) {
    keyword = "H : 100 km - < 250 km or approximately 1 degree - < 2.5 degrees";
  } else if ((resolution_type == 0 && max_horizontal_resolution >= 2.5 &&
      max_horizontal_resolution < 5.) || (resolution_type == 1 &&
      max_horizontal_resolution >= 250. && max_horizontal_resolution < 500.)) {
    keyword = "H : 250 km - < 500 km or approximately 2.5 degrees - < 5.0 "
        "degrees";
  } else if ((resolution_type == 0 && max_horizontal_resolution >= 5. &&
      max_horizontal_resolution < 10.) || (resolution_type == 1 &&
      max_horizontal_resolution >= 500. && max_horizontal_resolution < 1000.)) {
    keyword = "H : 500 km - < 1000 km or approximately 5 degrees - < 10 degrees";
  } else if ((resolution_type == 0 && max_horizontal_resolution < 0.00001) ||
     (resolution_type == 1 && max_horizontal_resolution < 0.001)) {
    keyword = "H : < 1 meter";
  } else if ((resolution_type == 0 && max_horizontal_resolution >= 0.00001 &&
      max_horizontal_resolution < 0.0003) || (resolution_type == 1 &&
      max_horizontal_resolution >= 0.001 && max_horizontal_resolution < 0.03)) {
    keyword = "H : 1 meter - < 30 meters";
  } else if ((resolution_type == 0 && max_horizontal_resolution >= 0.0003 &&
      max_horizontal_resolution < 0.001) || (resolution_type == 1 &&
      max_horizontal_resolution >= 0.03 && max_horizontal_resolution < 0.1)) {
    keyword = "H : 30 meters - < 100 meters";
  } else if ((resolution_type == 0 && max_horizontal_resolution >= 0.001 &&
      max_horizontal_resolution < 0.0025) || (resolution_type == 1 &&
      max_horizontal_resolution >= 0.1 && max_horizontal_resolution < 0.25)) {
    keyword = "H : 100 meters - < 250 meters";
  } else if ((resolution_type == 0 && max_horizontal_resolution >= 0.0025 &&
      max_horizontal_resolution < 0.005) || (resolution_type == 1 &&
      max_horizontal_resolution >= 0.25 && max_horizontal_resolution < 0.5)) {
    keyword = "H : 250 meters - < 500 meters";
  } else if ((resolution_type == 0 && max_horizontal_resolution >= 0.005 &&
      max_horizontal_resolution < 0.01) || (resolution_type == 1 &&
      max_horizontal_resolution >= 0.5 && max_horizontal_resolution < 1.)) {
    keyword = "H : 500 meters - < 1 km";
  } else {
    keyword = "H : >= 1000 km or >= 10 degrees";
  }
  return keyword;
}

string time_resolution_keyword(string frequency_type, int number, string unit,
    string statistics) {
  string keyword;
  if (frequency_type == "climatology") {
    if (unit == "hour") {
      keyword = "T : Hourly Climatology";
    } else if (unit == "day") {
      if (number == 5) {
        keyword = "T : Pentad Climatology";
      } else if (number == 7) {
        keyword = "T : Weekly Climatology";
      } else {
        keyword = "T : Daily Climatology";
      }
    } else if (unit == "week") {
      keyword = "T : Weekly Climatology";
    } else if (unit == "month") {
      keyword = "T : Monthly Climatology";
    } else if (unit == "season" || unit == "winter" || unit == "spring" || unit
        == "summer" || unit == "autumn") {
      keyword = "T : Seasonal Climatology";
    } else if (unit == "year") {
      keyword = "T : Annual Climatology";
    } else if (unit == "30-year") {
      keyword = "T : Climate Normal (30-year climatology)";
    }
  } else if (frequency_type == "regular") {
    if (unit == "second") {
      keyword = "T : 1 second - < 1 minute";
    } else if (unit == "minute") {
      keyword = "T : 1 minute - < 1 hour";
    } else if (unit == "hour") {
      keyword = "T : Hourly - < Daily";
    } else if (unit == "day") {
      keyword = "T : Daily - < Weekly";
    } else if (unit == "week") {
      keyword = "T : Weekly - < Monthly";
    } else if (unit == "month" || unit == "season") {
      keyword = "T : Monthly - < Annual";
    } else if (unit == "year" || unit == "winter" || unit == "spring" || unit ==
        "summer" || unit == "autumn") {
      keyword = "T : Annual";
    } else if (unit == "decade") {
      keyword = "T : Decadal";
    }
  } else {
    if (unit == "second") {
      if (number > 1) {
        keyword = "T : < 1 second";
      } else {
        keyword = "T : 1 second - < 1 minute";
      }
    } else if (unit == "minute") {
      if (number > 1) {
        keyword = "T : 1 second - < 1 minute";
      } else {
        keyword = "T : 1 minute - < 1 hour";
      }
    } else if (unit == "hour") {
      if (number > 1) {
        keyword = "T : 1 minute - < 1 hour";
      } else {
        keyword = "T : Hourly - < Daily";
      }
    } else if (unit == "day") {
      if (number > 1) {
        keyword = "T : Hourly - < Daily";
      } else {
        keyword = "T : Daily - < Weekly";
      }
    } else if (unit == "week") {
      if (number > 1) {
        keyword = "T : Daily - < Weekly";
      } else {
        keyword = "T : Weekly - < Monthly";
      }
    } else if (unit == "month") {
      if (number > 1) {
        keyword = "T : Weekly - < Monthly";
      } else {
        keyword = "T : Monthly - < Annual";
      }
    } else if (unit == "season" || unit == "winter" || unit == "spring" || unit
        == "summer" || unit == "autumn") {
      keyword = "T : Monthly - < Annual";
    } else if (unit == "year") {
      if (number > 1) {
        keyword = "T : Monthly - < Annual";
      } else {
        keyword = "T : Annual";
      }
    } else if (unit == "decade") {
      if (number > 1) {
        keyword = "T : Annual";
      } else {
        keyword = "T : Decadal";
      }
    }
  }
  return keyword;
}

} // end namespace searchutils
