#include <PostgreSQL.hpp>
#include <myexception.hpp>

using std::string;
using std::vector;

namespace PostgreSQL {

bool table_exists(Server& server, string absolute_table) {
  auto idx = absolute_table.find(".");
  if (idx == string::npos) {
    throw my::BadSpecification_Error("bad absolute table specification '" +
        absolute_table + "'");
  }
  auto schema = absolute_table.substr(0, idx);
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

} // end namespace PostgreSQL
