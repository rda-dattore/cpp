#include <PostgreSQL.hpp>
#include <strutils.hpp>
#include <myexception.hpp>

using std::string;
using strutils::split;
using strutils::to_lower;
using strutils::trim;

namespace PostgreSQL {

QueryIterator Query::begin() {
  auto i = QueryIterator(*this, false);
  return ++i;
};

QueryIterator Query::end() {
  return QueryIterator(*this, true);
}

const QueryIterator& QueryIterator::operator++() {
  if (!m_query.fetch_row(row)) {
    m_is_end = true;
  }
  return *this;
}

Row& QueryIterator::operator*() {
  if (m_is_end) {
    row = Row();
  }
  return row;
}

bool Query::fetch_row(Row& row) {
  auto result = PQgetResult(server_conn);
  if (result == nullptr) {
    return false;
  }
  if (PQntuples(result) == 0 && PQresultStatus(result) == PGRES_TUPLES_OK) {
    while (result != nullptr) {
      PQclear(result);
      result = PQgetResult(server_conn);
    }
    return false;
  }
  row.fill(result, 0, 0, column_list);
  PQclear(result);
  return true;
}

void Query::set(string columns, string absolute_table, string
    where_conditions) {
  fill_column_list(columns);
  auto p = split_tablename(absolute_table);
  query = "select " + columns + " from " + postgres_ready(p.first) + "." + p.
      second;
  if (!where_conditions.empty()) {
    query += " where " + where_conditions;
  }
}

void Query::set(string query_specification) {
  size_t idx = query_specification.find("from");
  if (idx == string::npos && query_specification.find("show") != 0) {
    throw my::BadSpecification_Error("bad query '" + query_specification + "'");
  }
  if (idx > 0) {
    auto columns = query_specification.substr(0, idx - 1);
    columns = columns.substr(columns.find("select") + 7);
    trim(columns);
    fill_column_list(columns);
  } else {
    column_list.clear();
  }
  query = query_specification;
}

int Query::submit(Server& server) {
  if (!server) {
    m_error = "Query error: not connected to server";
    return -1;
  }
  server_conn = server.connection();
  if (PQsendQuery(server_conn, query.c_str()) != 1) {
    m_error = PQerrorMessage(server_conn);
    return -1;
  }
  if (PQsetSingleRowMode(server_conn) != 1) {
    m_error = "Unable to set single row mode";
    return -1;
  }
  return 0;
}

void Query::fill_column_list(string columns) {
  column_list.clear();
  if (columns == "*") {
    return;
  }
  columns = to_lower(columns);
  auto idx = columns.find("distinct");
  if (idx != string::npos) {
    columns = columns.substr(idx + 8);
    trim(columns);
  }
  auto in_parens = 0;
  for (size_t n = 0; n < columns.length(); ++n) {
    if (columns[n] == '(') {
      ++in_parens;
    } else if (columns[n] == ')') {
      --in_parens;
    }
    if (in_parens == 0 && columns[n] == ',') {
      columns = columns.substr(0, n) + ";" + columns.substr(n + 1);
    }
  }
  auto sp = split(columns, ";");
  for (auto& col_name : sp) {
    auto idx = col_name.find(" as ");
    if (idx != string::npos) {
      col_name = col_name.substr(idx + 4);
    }
    trim(col_name);
    column_list.emplace_back(col_name);
  }
}

bool LocalQuery::fetch_row(Row& row) {
  if (result == nullptr || curr_row >= m_num_rows) {
    row.clear();
    return false;
  }
  row.fill(result.get(), m_num_fields, curr_row++, column_list);
  return true;
}

int LocalQuery::submit(Server& server) {
  if (!server) {
    m_error = "Query error: not connected to server";
    return -1;
  }
  if (result != nullptr) {
    PQclear(result.release());
    m_num_rows = m_num_fields = 0;
    curr_row = 0;
  }
  result.reset(PQexec(server.connection(), query.c_str()));
  auto status = PQresultStatus(result.get());
  if (status == PGRES_TUPLES_OK) {
    m_num_rows = PQntuples(result.get());
    m_num_fields = PQnfields(result.get());
  } else {
    m_error = PQresultErrorMessage(result.get());
    return -1;
  }
  return 0;
}

} // end namespace PostgreSQL
