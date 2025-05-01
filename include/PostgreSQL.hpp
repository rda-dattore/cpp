#ifndef POSTGRESQL_H
#define  POSTGRESQL_H

#include <libpq-fe.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

namespace PostgreSQL {

struct DBconfig {
  DBconfig() : user(), password(), host(), dbname() { }
  DBconfig(std::string user_, std::string password_, std::string host_,
      std::string dbname_) :
      DBconfig() { user = user_; password = password_; host = host_;
          dbname = dbname_; }

  std::string user, password, host, dbname;
};

struct PGresult_deleter {
  void operator()(PGresult *result) {
    if (result != nullptr) {
      PQclear(result);
    }
  }
};

class Row {
public:
  Row() : column_names(), column_values(), empty_column(), column_delimiter(
      "|") { }
  void clear();
  void delimit(std::string delimiter) { column_delimiter = delimiter; }
  bool empty() const { return column_values.empty(); }
  void fill(PGresult *result, size_t num_fields, size_t row_num, std::
      vector<std::string>& column_list);
  size_t length() const { return column_values.size(); }
  const std::string& operator[](size_t column_number) const;
  const std::string& operator[](std::string column_name) const;
  friend std::ostream& operator<<(std::ostream& out_stream, const Row& row);

private:
  std::vector<std::string> column_names, column_values;
  std::string empty_column, column_delimiter;
};

class Server {
public:
  Server() : conn(nullptr), m_error() { }
  Server(const Server& source) = delete;
  Server(std::string host, std::string user, std::string password, std::string
      db, int timeout = -1) :
      Server() { connect(host, user, password, db, timeout); }
  Server(DBconfig db_config, int timeout = -1) :
      Server() { connect(db_config.host, db_config.user, db_config.password,
          db_config.dbname, timeout); }
  Server operator=(const Server& source) = delete;
  operator bool() const { return PQstatus(conn.get()) != CONNECTION_BAD; }
  int command(std::string command, std::string *result = nullptr);
  void connect(std::string host, std::string user, std::string password, std::
      string db, int timeout = -1);
  void connect(DBconfig db_config, int timeout = -1) { connect(db_config.host,
      db_config.user, db_config.password, db_config.dbname, timeout); }
  PGconn *connection() { return conn.get(); }
  int create_index(std::string absolute_table, std::string index_name, std::
      string column_list);
  int _delete(std::string absolute_table, std::string where_conditions = "");
  void disconnect();
  int drop_index(std::string schema_name, std::string index_name);
  int duplicate_table(std::string absolute_table_source, std::string
      absolute_table_target);
  std::string error() const { return m_error; }
  std::vector<std::string> get_index_names(std::string absolute_table);
  int insert(const std::string& absolute_table, const std::string&
      row_specification, const std::string& on_conflict = "");
  int insert(const std::string& absolute_table, const std::string& column_list,
      const std::string& value_list, const std::string& on_conflict);
  std::vector<std::string> schema_names();
  int update(std::string absolute_table, std::string column_name_value_pair,
    std::string where_conditions);
  int update(std::string absolute_table, std::vector<std::string>&
    column_name_value_pairs, std::string where_conditions);

private:
  struct PGconn_deleter {
    void operator()(PGconn *conn) {
      if (conn != nullptr) {
        PQfinish(conn);
      }
    }
  };
  std::unique_ptr<PGconn, PGconn_deleter> conn;
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
  Query() : query(), m_error(), column_list(), server_conn(nullptr) { }
  Query(const Query& source) : Query() { }
  Query(std::string columns, std::string absolute_table, std::string
      where_conditions = "") :
      Query() { set(columns, absolute_table, where_conditions); }
  Query(std::string query_specification) : Query() { set(query_specification); }
  virtual ~Query() { }
  Query operator=(const Query& source) = delete;
  operator bool() const { return (!query.empty()); }
  std::string error() const { return m_error; }
  virtual bool fetch_row(Row& row);
  void set(std::string columns, std::string absolute_table, std::string
      where_conditions = "");
  void set(std::string query_specification);
  std::string show() const { return query; }
  virtual int submit(Server& server);

  // range-based support
  QueryIterator begin();
  QueryIterator end();

protected:
  std::string query, m_error;
  std::vector<std::string> column_list;

  void fill_column_list(std::string columns);

private:
  PGconn *server_conn;
};

class LocalQuery : public Query {
public:
  LocalQuery() : result(nullptr), m_num_rows(0), m_num_fields(0), curr_row(
      0) { }
//  LocalQuery(const LocalQuery& source) = delete;
  LocalQuery(std::string columns, std::string absolute_table, std::string
      where_conditions = "") :
      LocalQuery() { set(columns, absolute_table, where_conditions); }
  LocalQuery(std::string query_specification) : LocalQuery() {
    set(query_specification);
  }

  // Query overrides
  bool fetch_row(Row& row);
  int submit(Server& server);

  // local methods
  size_t num_columns() const { return m_num_fields; }
  size_t num_rows() const { return m_num_rows; }
  void rewind() { curr_row = 0; }

private:
  std::unique_ptr<PGresult, PGresult_deleter> result;
  size_t m_num_rows, m_num_fields, curr_row;
};

class PreparedStatement;
class PreparedStatementIterator {
public:
  PreparedStatementIterator(PreparedStatement& pstmt, bool is_end) : m_pstmt(
      pstmt), m_is_end(is_end), row() { }
  bool operator !=(const PreparedStatementIterator& source) {
    return (m_is_end != source.m_is_end);
  }
  const PreparedStatementIterator& operator++();
  Row& operator*();

private:
  PreparedStatement &m_pstmt;
  bool m_is_end;
  Row row;
};

class PreparedStatement {
public:
  PreparedStatement() : m_name(), m_error(), result(nullptr), param_values(
      nullptr), nparams(0), m_num_rows(0), m_num_fields(0), curr_row(0),
      is_ready(false) { }
  PreparedStatement(const PreparedStatement& source) = delete;
  PreparedStatement operator=(const PreparedStatement& source) = delete;
  operator bool() const { return is_ready; }
  bool bind_parameter(size_t parameter_index, std::string parameter_value, bool
      is_null = false);
  std::string error() const { return m_error; }
  bool fetch_row(Row& row);
  std::string name() const { return m_name; }
  size_t num_fields() const { return m_num_fields; }
  size_t num_rows() const { return m_num_rows; }
  void set(PGconn *conn, std::string query_specification, size_t num_params);
  int submit(Server& server);

  // range-based support
  PreparedStatementIterator begin();
  PreparedStatementIterator end();

private:
  std::string m_name, m_error;
  std::unique_ptr<PGresult, PGresult_deleter> result;
  std::unique_ptr<char *[]> param_values;
  size_t nparams, m_num_rows, m_num_fields, curr_row;
  bool is_ready;
};

class Transaction {
public:
  Transaction() : m_server(nullptr), m_error(), is_started(false) { }
  Transaction(const Transaction& source) = delete;
  ~Transaction() { rollback(); }
  Transaction operator=(const Transaction& source) = delete;
  int commit();
  std::string error() const { return m_error; }
  void get_lock(long long lock_id, size_t timeout_seconds = 30);
  int rollback();
  void start(Server& server);

private:
  Server *m_server;
  std::string m_error;
  bool is_started;
};

std::string postgres_ready(std::string source);

bool table_exists(Server& server, std::string absolute_table);
bool field_exists(Server& server, std::string absolute_table, std::string
    field);

std::vector<std::string> table_names(Server& server, std::string database, std::
    string like_expr, std::string& error);
std::vector<std::pair<std::string, char>> unique_constraints(Server& server,
    std::string absolute_table, std::string& error);

std::pair<std::string, std::string> split_tablename(std::string
      absolute_table_name);

} // end namespace PostgreSQL

#endif
