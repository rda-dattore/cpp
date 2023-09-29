#include <PostgreSQL.hpp>
#include <iostream>

using std::string;

namespace PostgreSQL {

void Row::clear() {
  columns.resize(0);
}

void Row::fill(PGresult *result, size_t num_fields, size_t row_num) {
  columns.resize(num_fields);
  for (size_t n = 0; n < columns.size(); ++n) {
    columns[n].assign(PQgetvalue(result, row_num, n), PQgetlength(result,
        row_num, n));
  }
}

const string& Row::operator[](size_t column_number) const {
  if (column_number < columns.size()) {
    return columns[column_number];
  }
  return empty_column;
}

std::ostream& operator<<(std::ostream& out_stream, const Row& row) {
  for (size_t n = 0; n < row.columns.size(); ++n) {
    if (n > 0) {
      out_stream << row.column_delimiter;
    }
    out_stream << row.columns[n];
  }
  return out_stream;
}

} // end namespace PostgreSQL
