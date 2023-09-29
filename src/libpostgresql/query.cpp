#include <PostgreSQL.hpp>
#include <myexception.hpp>

using std::string;

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

Query::~Query() {
  if (result != nullptr) {
    PQclear(result);
  }
}

bool Query::fetch_row(Row& row) {
  return true;
}

void Query::set(string query_specification) {
  size_t idx = query_specification.find("from");
  if (idx == string::npos && query_specification.find("show") != 0) {
    throw my::BadSpecification_Error("bad query '" + query_specification + "'");
  }
  query = query_specification;
}

bool LocalQuery::fetch_row(Row& row) {
  if (result == nullptr || curr_row >= m_num_rows) {
    row.clear();
    return false;
  }
  row.fill(result, m_num_fields, curr_row++);
  return true;
}

int LocalQuery::submit(Server& server) {
  if (!server) {
    m_error = "Query error: not connected to server";
    return -1;
  }
  if (result != nullptr) {
    PQclear(result);
    m_num_rows = m_num_fields = 0;
    curr_row = 0;
  }
  result = PQexec(server.handle(), query.c_str());
  auto status = PQresultStatus(result);
  if (status == PGRES_TUPLES_OK) {
    m_num_rows = PQntuples(result);
    m_num_fields = PQnfields(result);
  } else {
    m_error = PQresultErrorMessage(result);
    return -1;
  }
  return 0;
}

} // end namespace PostgreSQL
