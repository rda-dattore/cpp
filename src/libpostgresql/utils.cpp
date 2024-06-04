#include <PostgreSQL.hpp>
#include <strutils.hpp>
#include <myexception.hpp>

using std::make_pair;
using std::pair;
using std::string;
using std::vector;
using strutils::replace_all;
using strutils::split;
using strutils::to_lower;

namespace PostgreSQL {

string postgres_ready(string source) {
  if (to_lower(source) != source && source.front() != '"' && source.back() !=
      '"') {
    return "\"" + source + "\"";
  }
  return source;
}

bool table_exists(Server& server, string absolute_table) {
  auto idx = absolute_table.find(".");
  if (idx == string::npos) {
    throw my::BadSpecification_Error("bad absolute table specification '" +
        absolute_table + "'");
  }
  auto schema = absolute_table.substr(0, idx);
  replace_all(schema, "\"", "");
  auto tbl = absolute_table.substr(idx + 1);
  LocalQuery q("select tablename from pg_catalog.pg_tables where schemaname = '"
      + schema + "' and tablename like '" + tbl + "'");
  if (q.submit(server) < 0) {
    return false;
  }
  if (q.num_rows() > 0) {
    return true;
  }
  return false;
}

bool field_exists(Server& server, string absolute_table, string field) {
  auto idx = absolute_table.find(".");
  if (idx == string::npos) {
    throw my::BadSpecification_Error("bad absolute table specification '" +
        absolute_table + "'");
  }
  auto schema = absolute_table.substr(0, idx);
  replace_all(schema, "\"", "");
  auto tbl = absolute_table.substr(idx + 1);
  LocalQuery q("select column_name from information_schema.columns where "
      "table_schema = '" + schema + "' and table_name = '" + tbl + "'");
  if (q.submit(server) < 0) {
    return false;
  }
  for (const auto& r : q) {
    if (r[0] == field) {
      return true;
    }
  }
  return false;
}

vector<string> table_names(Server& server, string schema, string like_expr,
    string& error) {
  vector<string> v; // return value
  error.clear();
  LocalQuery q("select tablename from pg_catalog.pg_tables where schemaname = '"
      + schema + "' and tablename like '" + like_expr + "'");
  if (q.submit(server) == 0) {
    for (const auto& r : q) {
      v.emplace_back(r[0]);
    }
  } else {
    error = q.error();
  }
  return v;
}

pair<string, string> split_tablename(string absolute_table_name) {
  auto sp = split(absolute_table_name, ".");
  if (sp.size() != 2) {
    throw my::BadSpecification_Error("'" + absolute_table_name + "' is not an "
        "absolute table name");
  }
  return make_pair(sp.front(), sp.back());
}

vector<pair<string, char>> unique_constraints(Server& server, string
    absolute_table, string& error) {
  vector<pair<string, char>> v; // return value
  error.clear();
  auto p = split_tablename(absolute_table);
  LocalQuery q("select conkey, contype from pg_catalog.pg_constraint as c left "
      "join pg_catalog.pg_class as l on l.oid = c.conrelid left join "
      "pg_catalog.pg_namespace as n on n.oid = c.connamespace where n.nspname "
      "= '" + p.first + "' and l.relname = '" + p.second + "'");
  if (q.submit(server) == 0) {
    for (const auto& r : q) {
      v.emplace_back(r[0], r[1][0]);
    }
  } else {
    error = q.error();
  }
  return v;
}

} // end namespace PostgreSQL
