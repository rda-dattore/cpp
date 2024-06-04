#include <vector>
#include <PostgreSQL.hpp>

using std::string;
using std::vector;

namespace PostgreSQL {

void Row::clear() {
  column_names.resize(0);
  column_values.resize(0);
}

void Row::fill(PGresult *result, size_t num_fields, size_t row_num,
    vector<string>& column_list) {
  if (num_fields == 0) {
    num_fields = PQnfields(result);
  }
  if (!column_list.empty()) {
    column_names.resize(num_fields);
  } else {
    column_names.resize(0);
  }
  column_values.resize(num_fields);
  for (size_t n = 0; n < column_values.size(); ++n) {
    if (!column_list.empty()) {
      column_names[n] = column_list[n];
    }
    column_values[n].assign(PQgetvalue(result, row_num, n), PQgetlength(result,
        row_num, n));
  }
}

const string& Row::operator[](size_t column_number) const {
  if (column_number < column_values.size()) {
    return column_values[column_number];
  }
  return empty_column;
}

const string& Row::operator[](string column_name) const {
  if (column_names.empty()) {
    return empty_column;
  }
  for (size_t n = 0; n < column_names.size(); ++n) {
    if (column_names[n] == column_name) {
      return column_values[n];
    }
  }
  return empty_column;
}

std::ostream& operator<<(std::ostream& out_stream, const Row& row) {
  for (size_t n = 0; n < row.column_values.size(); ++n) {
    if (n > 0) {
      out_stream << row.column_delimiter;
    }
    out_stream << row.column_values[n];
  }
  return out_stream;
}

} // end namespace PostgreSQL
