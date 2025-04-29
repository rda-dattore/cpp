#include <PostgreSQL.hpp>
#include <strutils.hpp>
#include <myexception.hpp>

using std::string;
using std::to_string;
using std::vector;
using strutils::append;
using strutils::strand;

namespace PostgreSQL {

PreparedStatementIterator PreparedStatement::begin() {
  auto i = PreparedStatementIterator(*this, false);
  return ++i;
}

PreparedStatementIterator PreparedStatement::end() {
  return PreparedStatementIterator(*this, true);
}

const PreparedStatementIterator& PreparedStatementIterator::operator++() {
  if (!m_pstmt.fetch_row(row)) {
    m_is_end = true;
  }
  return *this;
}

Row& PreparedStatementIterator::operator*() {
  if (m_is_end) {
    row = Row();
  }
  return row;
}

bool PreparedStatement::bind_parameter(size_t parameter_index, string
    parameter_value, bool is_null) {
  if (param_values == nullptr) {
    m_error = "Prepared statement is not ready.";
    return false;
  }
  if (parameter_index >= nparams) {
    m_error = "Parameter index exceeds number of parameters (" + to_string(
        nparams) + ") in prepared statement.";
    return false;
  }
  if (param_values[parameter_index] != nullptr) {
    m_error = "Parameter value " + to_string(parameter_index) + " is already "
        "bound.";
    return false;
  }
  auto len = parameter_value.length();
  param_values[parameter_index] = new char[len + 1];
  std::copy(parameter_value.begin(), parameter_value.end(), param_values[
      parameter_index]);
  param_values[parameter_index][len] = '\0';
  return true;
}

bool PreparedStatement::fetch_row(Row& row) {
  if (result == nullptr || curr_row >= m_num_rows) {
    row.clear();
    return false;
  }
  vector<string> column_list;
  row.fill(result.get(), m_num_fields, curr_row++, column_list);
  return true;
}

void PreparedStatement::set(PGconn *conn, string query_specification, size_t
    num_params) {
  if (!m_name.empty()) {
    throw my::BadOperation_Error("Prepared statement '" + m_name + "' already "
        "exists.");
  }
  auto name = strand(10);
  result.reset(PQprepare(conn, name.c_str(), query_specification.c_str(),
      num_params, nullptr));
  auto status = PQresultStatus(result.get());
  if (status == PGRES_COMMAND_OK) {
    m_error.clear();
    is_ready = true;
    m_name = name;
    nparams = num_params;
    param_values.reset(new char *[nparams]);
    for (size_t n = 0; n < nparams; ++n) {
      param_values[n] = nullptr;
    }
  } else {
    m_error = PQresultErrorMessage(result.get());
  }
  curr_row = 0;
}

int PreparedStatement::submit(Server& server) {
  m_error.clear();
  if (!is_ready) {
    m_error = "Prepared statement is not ready.";
    return -1;
  }
  auto param_formats = new int[nparams];
  for (size_t n = 0; n < nparams; ++n) {
    if (param_values[n] == nullptr) {
      append(m_error, "Parameter [" + to_string(n) + "] is not bound.", "\n");
    } else {
      param_formats[n] = 0;
    }
  }
  if (!m_error.empty()) {
    delete[] param_formats;
    return -1;
  }
  result.reset(PQexecPrepared(server.connection(), m_name.c_str(), nparams,
      param_values.get(), nullptr, param_formats, 0));
  delete[] param_formats;
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
