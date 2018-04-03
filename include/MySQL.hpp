#ifndef MYSQL_H
#define   MYSQL_H

#include <memory>
#include <list>
#include <forward_list>
#include <unordered_map>
#include <mysql.h>

namespace MySQL {

class Server;

struct ColumnEntry {
  ColumnEntry() : key(),index() {}

  std::string key;
  size_t index;
};

class Row
{
public:
  Row() : capacity(0),num_columns(0),columns(nullptr),empty_column(),column_list(nullptr) {}
  Row(const Row& source) : Row() { *this=source; }
  ~Row();
  Row& operator=(const Row& source);
  void clear();
  bool empty() const { return (columns == nullptr); }
  void fill(MYSQL_ROW& Row,unsigned long *lengths,size_t num_fields,std::unordered_map<std::string,size_t> *p_column_list);
  void fill(const std::string& Row,const char *separator,std::unordered_map<std::string,size_t> *p_column_list);
  size_t length() const { return num_columns; }
  const std::string& operator[](size_t column_number) const;
  const std::string& operator[](std::string column_name) const;

private:
  void check_alloc(size_t length);

  size_t capacity,num_columns;
  std::string *columns,empty_column;
  std::unordered_map<std::string,size_t> *column_list;
};

class Server
{
public:
  struct Value {
    std::string value;
    bool is_numeric;
  };
  struct RowInsert {
    std::string absolute_table;
    std::list<Value> values;
    std::string duplicate_action;
  };

  Server() : mysql(),_error(),_connected(false) {}
  Server(const Server& source) = delete;
  Server(std::string host,std::string user,std::string password,std::string db,int timeout = -1) : Server() { connect(host,user,password,db,timeout); }
  ~Server() { disconnect(); }
  operator bool() const { return _connected; }
  int command(std::string command,std::string& result);
  void connect(std::string host,std::string user,std::string password,std::string db,int timeout = -1);
  int _delete(std::string absolute_table,std::string where_conditions = "");
  std::list<std::string> db_names();
  void disconnect();
  std::string error() const { return _error; }
  MYSQL *handle() { return &mysql; }
  long long last_insert_ID() const { return mysql_insert_id(reinterpret_cast<st_mysql *>(const_cast<MYSQL *>(&mysql))); }
  int insert(const std::string& absolute_table,const std::string& row_specification,const std::string& on_duplicate_key = "");
  int insert(const std::string& absolute_table,std::list<std::string>& row_specifications,const std::string& on_duplicate_key = "");
  int insert(const std::string& absolute_table,const std::string& column_list,const std::string& value_list,const std::string& on_duplicate_key);
  int insert(RowInsert& row_insert);
  int update(std::string absolute_table,std::string column_name_value_pair,std::string where_conditions = "");
  int update(std::string absolute_table,std::list<std::string>& column_name_value_pairs,std::string where_conditions = "");

private:
  MYSQL mysql;
  std::string _error;
  bool _connected;
};

class Query
{
public:
  Query() : RESULT(nullptr),num_fields(0),_thread_id(0),query(),_error(),column_lists() {}
  Query(const Query& source) : Query() {}
  Query(std::string columns,std::string absolute_table,std::string where_conditions = "") : Query() { set(columns,absolute_table,where_conditions); }
  Query(std::string query_specification) : Query() { set(query_specification); }
  virtual ~Query() {}
  operator bool() const { return (query.length() > 0); }
  std::string error() const { return _error; };
  virtual bool fetch_row(Row& row) const;
  size_t length() const { return num_fields; }
  void set(std::string columns,std::string absolute_tables,std::string where_conditions = "");
  void set(std::string query_specification);
  std::string show() const { return query; }
  virtual int submit(Server& server);
  int thread_ID() const { return _thread_id; }

protected:
  void fill_column_list(std::string columns);

  struct MYSQL_RES_DELETER {
    void operator()(MYSQL_RES *RESULT) {
	mysql_free_result(RESULT);
    }
  };
  std::unique_ptr<MYSQL_RES,MYSQL_RES_DELETER> RESULT;
  int num_fields,_thread_id;
  std::string query,_error;
  std::forward_list<std::unordered_map<std::string,size_t>> column_lists;
};

class LocalQuery : public Query
{
public:
  LocalQuery() : _num_rows(0) {}
  LocalQuery(std::string columns,std::string absolute_table,std::string where_conditions = "") : LocalQuery() { set(columns,absolute_table,where_conditions); }
  LocalQuery(std::string query_specification) : LocalQuery() { set(query_specification); }
  void clear();
  int explain(Server& server);
  bool fetch_row(Row& row) const;
  size_t num_rows() const { return _num_rows; }
  void rewind() { mysql_data_seek(RESULT.get(),0); }
  int submit(Server& server);

private:
  size_t _num_rows;
};

bool table_exists(Server& server,std::string absolute_table);
bool field_exists(Server& server,std::string absolute_table,std::string field);
std::list<std::string> table_names(Server& server,std::string database,std::string like_expr,std::string& error);

extern "C" void mysqlqueryerror_(int *query);
extern "C" void mysqlqueryerror64_(long long *query);
extern "C" void mysqlqueryresult_(int *query,int *row_index,int *num_fields,int *field_length,char *row);
extern "C" void mysqlqueryresult64_(long long *query,long long *row_index,long long *num_fields,long long *field_length,char *row);
extern "C" void mysqlservererror_(int *server);
extern "C" void mysqlservererror64_(long long *server);

extern "C" int mysqlconnect_(char *host,char *username,char *password,char *db);
extern "C" int mysqlinsert_(int *server,char *table_name,char *values,int *vlen,char *duplicate_key_condition);

extern "C" long long mysqlconnect64_(char *host,char *username,char *password,char *db);
extern "C" long long mysqlinsert64_(long long *server,char *table_name,char *values,long long *vlen,char *duplicate_key_condition);
extern "C" long long mysqlquery_(int *server,char *column_list,char *table_name,char *where_conditions,int *num_rows,int *num_columns,int *status);
extern "C" long long mysqlquery64_(long long *server,char *column_list,char *table_name,char *where_conditions,long long *num_rows,long long *num_columns,long long *status);

} // end namespace MySQL

#endif
