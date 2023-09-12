#ifndef MYSQL_H
#define   MYSQL_H

#include <fstream>
#include <memory>
#include <vector>
#include <forward_list>
#include <unordered_map>
#include <mysql.h>

namespace MySQL {

class Server;

class Row {
public:
  Row() : columns(), empty_column(), column_list(), _column_delimiter('|') { }
  Row(const Row& source) : Row() { *this=source; }
  ~Row() {}
  Row& operator=(const Row& source);
  void clear();
  void delimit(char column_delimiter) { _column_delimiter=column_delimiter; }
  bool empty() const { return (columns.size() == 0); }
  void fill(MYSQL_ROW& Row, unsigned long *lengths, size_t num_fields, std::
      unordered_map<std::string, size_t> *p_column_list);
  void fill(const std::string& Row, const char *separator, std::unordered_map<
      std::string, size_t> *p_column_list);
  size_t length() const { return columns.size(); }
  const std::string& operator[](size_t column_number) const;
  const std::string& operator[](std::string column_name) const;
  friend std::ostream& operator<<(std::ostream& out_stream, const Row& row);

private:
  std::vector<std::string> columns;
  std::string empty_column;
  std::unordered_map<std::string, size_t> *column_list;
  char _column_delimiter;
};

class Server {
public:

  struct Value {
    std::string value;
    bool is_numeric;
  };

  struct RowInsert {
    std::string absolute_table;
    std::vector<Value> values;
    std::string duplicate_action;
  };

  Server() : mysql(), m_error(), m_connected(false) { }
  Server(const Server& source) = delete;
  Server(std::string host, std::string user, std::string password, std::string
      db, int timeout = -1) :
      Server() { connect(host, user, password, db, timeout); }
  ~Server() { disconnect(); }
  operator bool() const { return m_connected == true; }
  int command(std::string command, std::string& result);
  void connect(std::string host, std::string user, std::string password, std::
      string db, int timeout = -1);
  int _delete(std::string absolute_table, std::string where_conditions = "");
  std::vector<std::string> db_names();
  void disconnect();
  std::string error() const { return m_error; }
  MYSQL *handle() { return &mysql; }
  long long last_insert_ID() const {
      return mysql_insert_id(const_cast<MYSQL *>(&mysql));
  }
  int insert(const std::string& absolute_table, const std::string&
      row_specification, const std::string& on_duplicate_key = "");
  int insert(const std::string& absolute_table, std::vector<std::string>&
      row_specifications, const std::string& on_duplicate_key = "");
  int insert(const std::string& absolute_table, const std::string& column_list,
      const std::string& value_list, const std::string& on_duplicate_key);
  int insert(RowInsert& row_insert);
  int update(std::string absolute_table, std::string column_name_value_pair,
      std::string where_conditions = "");
  int update(std::string absolute_table, std::vector<std::string>&
      column_name_value_pairs, std::string where_conditions = "");
  size_t version() { return mysql_get_server_version(&mysql); }

private:
  MYSQL mysql;
  std::string m_error;
  bool m_connected;
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

class ConstQueryIterator {
public:
  ConstQueryIterator(const Query& query, bool is_end) : m_query(query),
      m_is_end(is_end), row() { }
  bool operator !=(const ConstQueryIterator& source) {
    return (m_is_end != source.m_is_end);
  }
  const ConstQueryIterator& operator++();
  const Row& operator*();

private:
  const Query &m_query;
  bool m_is_end;
  Row row;
};

class Query {
public:
  Query() : RESULT(nullptr), num_fields(0), m_thread_id(0), query(), m_error(),
      column_lists() { }
  Query(const Query& source) : Query() { }
  Query(std::string columns, std::string absolute_table, std::string
      where_conditions = "") :
      Query() { set(columns, absolute_table, where_conditions); }
  Query(std::string query_specification) : Query() { set(query_specification); }
  virtual ~Query() { }
  operator bool() const { return (!query.empty()); }
  std::string error() const { return m_error; };
  virtual bool fetch_row(Row& row) const;
  size_t length() const { return num_fields; }
  void set(std::string columns, std::string absolute_tables, std::string
      where_conditions = "");
  void set(std::string query_specification);
  std::string show() const { return query; }
  virtual int submit(Server& server);
  int thread_ID() const { return m_thread_id; }

  // range-based support
  QueryIterator begin();
  ConstQueryIterator begin() const;
  QueryIterator end();
  ConstQueryIterator end() const;

protected:
  struct MYSQL_RES_DELETER {
    void operator()(MYSQL_RES *RESULT) {
	mysql_free_result(RESULT);
    }
  };
  std::unique_ptr<MYSQL_RES, MYSQL_RES_DELETER> RESULT;
  int num_fields, m_thread_id;
  std::string query, m_error;
  std::forward_list<std::unordered_map<std::string, size_t>> column_lists;

  void fill_column_list(std::string columns);
};

class LocalQuery : public Query {
public:
  LocalQuery() : m_num_rows(0) {}
  LocalQuery(std::string columns, std::string absolute_table, std::string
      where_conditions = "") :
      LocalQuery() { set(columns, absolute_table, where_conditions); }
  LocalQuery(std::string query_specification) : LocalQuery() {
    set(query_specification);
  }

  // Query overrides
  bool fetch_row(Row& row) const;
  int submit(Server& server);

  // local methods
  int explain(Server& server);
  size_t num_rows() const { return m_num_rows; }
  void rewind();
  void rewind() const;

  // range-based support
  QueryIterator begin();
  ConstQueryIterator begin() const;

private:
  size_t m_num_rows;
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
  PreparedStatement() : STMT(nullptr), statement(), m_error(), input_bind(),
      result_bind(), column_list() {}
  operator bool() const { return (!statement.empty()); }
  bool bind_parameter(size_t parameter_number, float parameter_specification,
      bool is_null);
  bool bind_parameter(size_t parameter_number, int parameter_specification,
      bool is_null);
  bool bind_parameter(size_t parameter_number, std::string
      parameter_specification, bool is_null);
  bool bind_parameter(size_t parameter_number, const char
      *parameter_specification, bool is_null);
  std::string error() const { return m_error; }
  bool fetch_row(Row& row) const;
  size_t num_rows() const;
  void rewind();
  void set(std::string statement_specification, std::vector<enum_field_types>
      parameter_types);
  std::string show() const { return statement; }
  int submit(Server& server);

  // range-based support
  PreparedStatementIterator begin();
  PreparedStatementIterator end();

private:
  struct MYSQL_STMT_DELETER {
    void operator()(MYSQL_STMT *STMT) {
	mysql_stmt_close(STMT);
    }
  };
  std::unique_ptr<MYSQL_STMT, MYSQL_STMT_DELETER> STMT;
  std::string statement, m_error;
  struct MYSQL_BIND_S {
    MYSQL_BIND_S() : binds(nullptr), lengths(), is_nulls(), errors(),
        field_count(0) {}

    struct MYSQL_BIND_DELETER {
	void operator()(MYSQL_BIND *BIND) {
	  switch (BIND->buffer_type) {
	    case (MYSQL_TYPE_FLOAT): {
		delete reinterpret_cast<float *>(BIND->buffer);
		break;
	    }
	    case (MYSQL_TYPE_LONG): {
		delete reinterpret_cast<int *>(BIND->buffer);
		break;
	    }
	    case (MYSQL_TYPE_STRING): {
		delete[] reinterpret_cast<char *>(BIND->buffer);
		break;
	    }
	    default: {}
	  }
	}
    };
    std::unique_ptr<MYSQL_BIND[], MYSQL_BIND_DELETER> binds;
    std::vector<size_t> lengths;
#ifdef MYSQL57
    std::vector<my_bool> is_nulls, errors;
#endif
#ifdef MYSQL8
    std::vector<bool> is_nulls, errors;
#endif
    size_t field_count;
  } input_bind, result_bind;
  std::unordered_map<std::string, size_t> column_list;
};

bool table_exists(Server& server, std::string absolute_table);
bool field_exists(Server& server, std::string absolute_table, std::string
    field);

std::vector<std::string> table_names(Server& server, std::string database, std::
    string like_expr, std::string& error);

bool run_prepared_statement(MySQL::Server& server, std::string
    prepared_statement_text, const std::vector<enum_field_types>&
    parameter_types, const std::vector<std::string>& parameters, MySQL::
    PreparedStatement& prepared_statement, std::string& error);

extern "C" void mysqlqueryerror_(int *query);
extern "C" void mysqlqueryerror64_(long long *query);
extern "C" void mysqlqueryresult_(int *query, int *row_index, int *num_fields,
    int *field_length, char *row);
extern "C" void mysqlqueryresult64_(long long *query, long long *row_index,
    long long *num_fields,long long *field_length,char *row);
extern "C" void mysqlservererror_(int *server);
extern "C" void mysqlservererror64_(long long *server);

extern "C" int mysqlconnect_(char *host, char *username, char *password, char
    *db);
extern "C" int mysqlinsert_(int *server, char *table_name, char *values, int
    *vlen, char *duplicate_key_condition);

extern "C" long long mysqlconnect64_(char *host, char *username, char *password,
    char *db);
extern "C" long long mysqlinsert64_(long long *server, char *table_name, char
    *values, long long *vlen, char *duplicate_key_condition);
extern "C" long long mysqlquery_(int *server, char *column_list, char
    *table_name, char *where_conditions, int *num_rows, int *num_columns, int
    *status);
extern "C" long long mysqlquery64_(long long *server, char *column_list, char
    *table_name, char *where_conditions, long long *num_rows, long long
    *num_columns, long long *status);

} // end namespace MySQL

#endif
