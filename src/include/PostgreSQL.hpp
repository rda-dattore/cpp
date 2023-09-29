#ifndef POSTGRESQL_H
#define  POSTGRESQL_H

#include <libpq-fe.h>
#include <string>
#include <vector>

namespace PostgreSQL {

class Row {
public:
  Row() : columns(), empty_column(), column_delimiter("|") { }
  void clear();
  void delimit(std::string delimiter) { column_delimiter = delimiter; }
  bool empty() const { return columns.empty(); }
  void fill(PGresult *result, size_t num_fields, size_t row_num);
  size_t length() const { return columns.size(); }
  const std::string& operator[](size_t column_number) const;
  friend std::ostream& operator<<(std::ostream& out_stream, const Row& row);

private:
  std::vector<std::string> columns;
  std::string empty_column, column_delimiter;
};

class Server {
public:
  Server() : conn(nullptr), m_error() { }
  Server(const Server& source) = delete;
  ~Server() { disconnect(); }
  Server operator=(const Server& source) = delete;
  operator bool() const { return PQstatus(conn) != CONNECTION_BAD; }
  void connect(std::string host, std::string user, std::string password, std::
      string db, int timeout = -1);
  void disconnect();
  std::string error() const { return m_error; }
  PGconn *handle() { return conn; }
  int insert(const std::string& absolute_table, const std::string& column_list,
      const std::string& value_list, const std::string& on_conflict);

private:
  PGconn *conn;
  std::string m_error;
};

class Query;
class QueryIterator {
public:
  QueryIterator(Query& query, bool is_end) : m_query(query), m_is_end(is_end),
      row() { }
  bool operator !=(const QueryIterator& source) {
    return (m_is_end != source.m_is_end);
  }
  const QueryIterator& operator++();
  Row& operator*();

private:
  Query &m_query;
  bool m_is_end;
  Row row;
};

class Query {
public:
  Query() : result(nullptr), query(), m_error(), m_num_rows(0), m_num_fields(0),
      curr_row(0) { }
  Query(const Query& source) = delete;
  Query(std::string query_specification) : Query() { set(query_specification); }
  virtual ~Query();
  Query operator=(const Query& source) = delete;
  std::string error() const { return m_error; }
  virtual bool fetch_row(Row& row);
  size_t num_rows() const { return m_num_rows; }
  void rewind() { curr_row = 0; }
  void set(std::string query_specification);
  std::string show() const { return query; }

  // range-based support
  QueryIterator begin();
  QueryIterator end();

protected:
  PGresult *result;
  std::string query, m_error;
  size_t m_num_rows, m_num_fields, curr_row;
};

class LocalQuery : public Query {
public:
  LocalQuery() { }
  LocalQuery(const LocalQuery& source) = delete;
  LocalQuery(std::string query_specification) : LocalQuery() {
    set(query_specification);
  }

  // Query overrides
  bool fetch_row(Row& row);
  int submit(Server& server);

private:
};

bool table_exists(Server& server, std::string absolute_table);
bool field_exists(Server& server, std::string absolute_table, std::string
    field);

std::vector<std::string> table_names(Server& server, std::string database, std::
    string like_expr, std::string& error);

} // end namespace PostgreSQL

#endif
