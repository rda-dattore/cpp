#include <string>
#include <regex>
#include <unordered_set>
#include <thread>
#include <MySQL.hpp>
#include <strutils.hpp>
#include <myexception.hpp>

using std::regex;
using std::regex_search;
using std::stof;
using std::stoi;
using std::string;
using std::stringstream;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using strutils::is_numeric;
using strutils::itos;
using strutils::to_lower;
using strutils::split;
using strutils::trim;

namespace MySQL {

Row& Row::operator=(const Row& source) {
  if (this == &source) {
    return *this;
  }
  columns = source.columns;
  column_list = source.column_list;
  return *this;
}

void Row::clear() {
  columns.resize(0);
  column_list = nullptr;
}

void Row::fill(MYSQL_ROW& Row, unsigned long *lengths, size_t num_fields,
    unordered_map<string, size_t> *p_column_list) {
  columns.resize(num_fields);
  for (size_t n = 0; n < columns.size(); ++n) {
    columns[n].assign(Row[n], lengths[n]);
  }
  column_list = p_column_list;
}

void Row::fill(const string& Row, const char *separator, unordered_map<string,
    size_t> *p_column_list) {
  if (Row.empty()) {
    columns.resize(1);
    columns[0].clear();
  } else {
    auto sp = split(Row, separator);
    columns.resize(sp.size());
    for (size_t n = 0; n < sp.size(); ++n) {
      columns[n] = sp[n];
    }
  }
  column_list = p_column_list;
}

const string& Row::operator[](size_t column_number) const {
  if (column_number < columns.size()) {
    return columns[column_number];
  }
  return empty_column;
}

const string& Row::operator[](string column_name) const {
  if (column_list != nullptr) {
    auto e = column_list->find(to_lower(column_name));
    if (e == column_list->end()) {
      return empty_column;
    }
    return columns[e->second];
  }
  return empty_column;
}

std::ostream& operator<<(std::ostream& out_stream, const Row& row) {
  for (size_t n = 0; n < row.columns.size(); ++n) {
    if (n > 0) {
      out_stream << row._column_delimiter;
    }
    out_stream << row.columns[n];
  }
  return out_stream;
}

int Server::command(string command, string& result) {
  mysql_real_query(&mysql, command.c_str(), command.length());
  m_error.clear();
  if (mysql_errno(&mysql) > 0) {
    m_error = mysql_error(&mysql) + string(" - errno: ") + itos(mysql_errno(
        &mysql));
  }
  if (!m_error.empty()) {
    return -1;
  }
  MYSQL_RES *res = mysql_store_result(&mysql);
  result = "";
  if (res != nullptr) {
    size_t num_fields = 0;
    string separator("|");
    MYSQL_ROW row;
    while ( (row = mysql_fetch_row(res))) {
      if (!result.empty()) {
        result += "\n";
      } else {
        num_fields = mysql_num_fields(res);
      }
      for (size_t n = 0; n < num_fields; ++n) {
        result += separator;
        if (row[n]) {
          result += row[n];
        } else {
          result += "NULL";
        }
      }
      result += separator;
    }
    mysql_free_result(res);
  }
  return 0;
}

void Server::connect(string host, string user, string password, string db, int
    timeout) {
  if (mysql_init(&mysql) == nullptr) {
    return;
  }
  mysql_thread_init();
  if (timeout >= 0) {
    mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, reinterpret_cast<char *>(
        &timeout));
  }
  auto port = 0;
  string sock;
  size_t idx = host.find(":");
  if (idx != string::npos) {
    auto s = host.substr(idx + 1);
    if (is_numeric(s)) {
      port = stoi(s);
    } else {
      sock = s;
    }
    host = host.substr(0, idx);
  }
  mysql_real_connect(&mysql, host.c_str(), user.c_str(), password.c_str(), db.
      c_str(), port, sock.c_str(), 0);
  auto num_tries = 0;
  while (mysql_errno(&mysql) > 0 && num_tries < 3) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    mysql_real_connect(&mysql, host.c_str(), user.c_str(), password.c_str(), db.
        c_str(), port, sock.c_str(), 0);
    ++num_tries;
  }
  m_error.clear();
  if (mysql_errno(&mysql) > 0) {
    m_error = mysql_error(&mysql) + string(" - errno: ") + itos(mysql_errno(
        &mysql));
  }
  if (m_error.empty()) {
    m_connected = true;
  }
}

int Server::_delete(string absolute_table, string where_conditions) {
  string s = "delete from " + absolute_table;
  if (!where_conditions.empty()) {
    s += " where "+where_conditions + ";";
  }
  mysql_real_query(&mysql, s.c_str(), s.length());
  m_error.clear();
  if (mysql_errno(&mysql) > 0) {
    m_error = mysql_error(&mysql) + string(" - errno: ") + itos(mysql_errno(
        &mysql));
  }
  if (!m_error.empty()) {
    return -1;
  }
  return 0;
}

vector<string> Server::db_names() {
  if (mysql_query(&mysql, "show databases") > 0) {
    throw my::BadOperation_Error("unable to fill database list: '" +
        string(mysql_error(&mysql)) + "'");
  }
  vector<string> v;
  auto res = mysql_store_result(&mysql);
  MYSQL_ROW r;
  while ( (r = mysql_fetch_row(res))) {
    v.emplace_back(reinterpret_cast<char *>(*r));
  }
  mysql_free_result(res);
  return v;
}

void Server::disconnect() {
  if (m_connected == true) {
    mysql_close(&mysql);
  }
  m_connected = false;
}

int Server::insert(const string& absolute_table, const string&
    row_specification, const string& on_duplicate_key) {
  auto s = "insert into " + absolute_table + " values (" + row_specification +
      ")";
  if (!on_duplicate_key.empty()) {
    s += " on duplicate key " + on_duplicate_key;
  }
  s += ";";
  mysql_real_query(&mysql, s.c_str(), s.length());
  m_error.clear();
  if (mysql_errno(&mysql) > 0) {
    m_error = mysql_error(&mysql) + string(" - errno: ") + itos(mysql_errno(
        &mysql));
  }
  if (!m_error.empty()) {
    return -1;
  }
  return 0;
}

int Server::insert(const string& absolute_table, vector<string>&
    row_specifications, const string& on_duplicate_key) {
  const int MAX_QUERY_LENGTH = 300000;
  auto len = row_specifications.front().length() * row_specifications.size();
  len = len < MAX_QUERY_LENGTH ? len + len / 2 : MAX_QUERY_LENGTH +
      MAX_QUERY_LENGTH / 2;
  string s;
  s.reserve(len);
  for (auto& spec : row_specifications) {
    if (s.length() > MAX_QUERY_LENGTH) {
      s = "insert into " + absolute_table + " values " + s;
      if (!on_duplicate_key.empty()) {
        s += " on duplicate key " + on_duplicate_key;
      }
      s += ";";
      mysql_real_query(&mysql, s.c_str(), s.length());
      m_error.clear();
      if (mysql_errno(&mysql) > 0) {
        m_error = mysql_error(&mysql) + string(" - errno: ") + itos(mysql_errno(
            &mysql));
      }
      if (!m_error.empty()) {
        return -1;
      }
      s = "";
    }
    if (!s.empty()) {
      s += ", ";
    }
    s += "(" + spec + ")";
  }
  if (!s.empty()) {
    s = "insert into " + absolute_table + " values " + s;
    if (!on_duplicate_key.empty()) {
      s += " on duplicate key " + on_duplicate_key;
    }
    s += ";";
    mysql_real_query(&mysql, s.c_str(), s.length());
    m_error.clear();
    if (mysql_errno(&mysql) > 0) {
      m_error = mysql_error(&mysql) + string(" - errno: ") + itos(mysql_errno(
          &mysql));
    }
    if (!m_error.empty()) {
      return -1;
    }
  }
  return 0;
}

int Server::insert(const string& absolute_table, const string& column_list,
    const string& value_list, const string& on_duplicate_key) {
  static const unordered_set<unsigned int> retry_codes{
      1213
  };
  m_error.clear();
  auto s = "insert into " + absolute_table + " (" + column_list + ") values (" +
      value_list + ")";
  if (!on_duplicate_key.empty()) {
    s += " on duplicate key " + on_duplicate_key;
  }
  s += ";";
  unsigned int err_num = 0xffffffff;
  auto num_tries = 0;
  while (err_num > 0 && num_tries < 3) {
    mysql_real_query(&mysql, s.c_str(), s.length());
    err_num = mysql_errno(&mysql);
    if (err_num > 0) {
      m_error = mysql_error(&mysql) + string(" - errno: ") + itos(err_num) +
          "\nfailed insert: '" + s + "'";
      if (retry_codes.find(err_num) != retry_codes.end()) {
        int sleep = pow(2., ++num_tries);
        std::this_thread::sleep_for(std::chrono::seconds(sleep));
      } else {
        err_num = 0;
      }
    } else {
      m_error.clear();
    }
  }
  if (!m_error.empty()) {
    return -1;
  }
  return 0;
}

int Server::insert(RowInsert& row_insert) {
  string s;
  for (auto& v : row_insert.values) {
    if (!s.empty()) {
      s += ",";
    }
    if (v.is_numeric) {
      s += v.value;
    } else {
      if (v.value.empty() || v.value == "DEFAULT") {
        s += "DEFAULT";
      } else {
        s += "'" + v.value + "'";
      }
    }
  }
  return insert(row_insert.absolute_table, s, row_insert.duplicate_action);
}

int Server::update(string absolute_table, string column_name_value_pair, string
    where_conditions) {
  if (column_name_value_pair.empty()) {
    return -1;
  }
  auto s = "update " + absolute_table + " set " + column_name_value_pair;
  if (!where_conditions.empty()) {
    s += " where " + where_conditions;
  }
  s += ";";
  mysql_real_query(&mysql, s.c_str(), s.length());
  m_error.clear();
  if (mysql_errno(&mysql) > 0) {
    m_error = mysql_error(&mysql) + string(" - errno: ") + itos(mysql_errno(
        &mysql));
  }
  if (!m_error.empty()) {
    return -1;
  }
  return 0;
}

int Server::update(string absolute_table, vector<string>&
    column_name_value_pairs, string where_conditions) {
  if (column_name_value_pairs.empty()) {
    return -1;
  }
  auto it = column_name_value_pairs.begin();
  string s = "update " + absolute_table + " set " + *it;
  auto end=column_name_value_pairs.end();
  for (++it; it != end; ++it) {
    s += ", " + *it;
  }
  if (!where_conditions.empty()) {
    s += " where " + where_conditions;
  }
  s += ";";
  mysql_real_query(&mysql, s.c_str(), s.length());
  m_error.clear();
  if (mysql_errno(&mysql) > 0) {
    m_error = mysql_error(&mysql) + string(" - errno: ") + itos(mysql_errno(
        &mysql));
  }
  if (!m_error.empty()) {
    return -1;
  }
  return 0;
}

bool Query::fetch_row(Row& row) const {
  if (RESULT == nullptr) {
    row.clear();
    return false;
  }
  MYSQL_ROW Row = mysql_fetch_row(RESULT.get());
  if (Row) {
    auto l = mysql_fetch_lengths(RESULT.get());
    row.fill(Row, l, num_fields, const_cast<unordered_map<string, size_t>*>(
        &column_lists.front()));
    return true;
  }
  row.clear();
  return false;
}

void Query::set(string columns, string tables, string where_conditions) {
  fill_column_list(columns);
  query = "select " + columns + " from " + tables;
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
  }
  query = query_specification;
}

int Query::submit(Server& server) {
  if (!server) {
    m_error = "Query error: not connected to server";
    return -1;
  }
  if (RESULT != nullptr) {
    RESULT.reset(nullptr);
  }
  m_thread_id = mysql_thread_id(server.handle());
  mysql_real_query(server.handle(), query.c_str(), query.length());
  m_error.clear();
  auto n = 0;
  while (mysql_errno(server.handle()) == 1205 && n < 3) {
    mysql_real_query(server.handle(), query.c_str(), query.length());
    ++n;
  }
  if (mysql_errno(server.handle()) > 0) {
    m_error = mysql_error(server.handle()) + string(" - errno: ") + itos(
        mysql_errno(server.handle()));
  }
  if (!m_error.empty()) {
    return -1;
  }
  RESULT.reset(mysql_use_result(server.handle()));
  num_fields = mysql_num_fields(RESULT.get());
  return 0;
}

void Query::fill_column_list(string columns) {
  column_lists.emplace_front();
  columns = to_lower(columns);
  size_t idx = columns.find("distinct");
  if (idx != string::npos) {
    columns = columns.substr(idx + 8);
    trim(columns);
  }
  int in_parens = 0;
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
  size_t n = 0;
  auto sp = split(columns, ";");
  for (auto& key : sp) {
    auto idx = key.find(" as ");
    if (idx != string::npos) {
      key = key.substr(idx + 4);
    }
    trim(key);
    column_lists.front()[key] = n++;
  }
}

QueryIterator Query::begin() {
  auto i = QueryIterator(*this, false);
  return ++i;
}

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

ConstQueryIterator Query::begin() const {
  auto i = ConstQueryIterator(*this, false);
  return ++i;
}

ConstQueryIterator Query::end() const {
  return ConstQueryIterator(*this, true);
}

const ConstQueryIterator& ConstQueryIterator::operator++() {
  if (!m_query.fetch_row(row)) {
    m_is_end = true;
  }
  return *this;
}

const Row& ConstQueryIterator::operator*() {
  if (m_is_end) {
    row = Row();
  }
  return row;
}

bool LocalQuery::fetch_row(Row& row) const {
  if (RESULT == nullptr) {
    row.clear();
    return false;
  }
  MYSQL_ROW Row = mysql_fetch_row(RESULT.get());
  if (Row) {
    auto l = mysql_fetch_lengths(RESULT.get());
    row.fill(Row, l, num_fields, const_cast<unordered_map<string, size_t>*>(
        &column_lists.front()));
    return true;
  }
  row.clear();
  return false;
}

int LocalQuery::submit(Server& server) {
  if (!server) {
    m_error = "Query error: not connected to server";
    return -1;
  }
  if (RESULT != nullptr) {
    RESULT.reset(nullptr);
    m_num_rows = num_fields = 0;
  }
  m_thread_id = mysql_thread_id(server.handle());
  mysql_real_query(server.handle(), query.c_str(), query.length());
  m_error.clear();
  auto n = 0;
  while (mysql_errno(server.handle()) == 1205 && n < 3) {
    mysql_real_query(server.handle(), query.c_str(), query.length());
    ++n;
  }
  if (mysql_errno(server.handle()) > 0) {
    m_error = mysql_error(server.handle()) + string(" - errno: ") + itos(
        mysql_errno(server.handle()));
    return -1;
  }
  RESULT.reset(mysql_store_result(server.handle()));
  if (mysql_errno(server.handle()) > 0) {
    m_error = mysql_error(server.handle()) + string(" - errno: ") + itos(
        mysql_errno(server.handle()));
    return -1;
  }
  m_num_rows = mysql_num_rows(RESULT.get());
  num_fields = mysql_num_fields(RESULT.get());
  return 0;
}

int LocalQuery::explain(Server& server) {
  if (!server) {
    m_error = "Query error: not connected to server";
    return -1;
  }
  if (RESULT != nullptr) {
    RESULT.reset(nullptr);
    m_num_rows = num_fields = 0;
  }
  mysql_query(server.handle(), ("explain " + query).c_str());
  m_error.clear();
  auto n = 0;
  while (mysql_errno(server.handle()) == 1205 && n < 3) {
    mysql_real_query(server.handle(), query.c_str(), query.length());
    ++n;
  }
  if (mysql_errno(server.handle()) > 0)
    m_error = mysql_error(server.handle()) + string(" - errno: ") + itos(
        mysql_errno(server.handle()));
  if (!m_error.empty()) {
    return -1;
  }
  RESULT.reset(mysql_store_result(server.handle()));
  m_num_rows = mysql_num_rows(RESULT.get());
  num_fields = mysql_num_fields(RESULT.get());
  return 0;
}

void LocalQuery::rewind() {
  if (RESULT != nullptr) {
    mysql_data_seek(RESULT.get(), 0);
  }
}

void LocalQuery::rewind() const {
  if (RESULT != nullptr) {
    mysql_data_seek(RESULT.get(), 0);
  }
}

QueryIterator LocalQuery::begin() {
  rewind();
  return Query::begin();
}

ConstQueryIterator LocalQuery::begin() const {
  rewind();
  return Query::begin();
}

bool PreparedStatement::bind_parameter(size_t parameter_number, float
    parameter_specification, bool is_null) {
  if (parameter_number >= input_bind.lengths.size()) {
    m_error = "parameter number out of range";
    return false;
  }
  if (input_bind.binds[parameter_number].buffer_type != MYSQL_TYPE_FLOAT) {
    m_error = "float bind does not match parameter specification";
    return false;
  }
  input_bind.is_nulls[parameter_number] = is_null;
#ifdef MYSQL57
  input_bind.binds[parameter_number].is_null = new my_bool;
#endif
#ifdef MYSQL8
  input_bind.binds[parameter_number].is_null = new bool;
#endif
  *input_bind.binds[parameter_number].is_null = is_null;
  if (is_null) {
    if (input_bind.binds[parameter_number].buffer != nullptr) {
      delete reinterpret_cast<float *>(input_bind.binds[parameter_number].
          buffer);
    }
    input_bind.binds[parameter_number].buffer = nullptr;
    input_bind.binds[parameter_number].buffer_length = 0;
  } else {
    input_bind.binds[parameter_number].buffer = new float;
    *(reinterpret_cast<float *>(input_bind.binds[parameter_number].buffer)) =
        parameter_specification;
    input_bind.binds[parameter_number].buffer_length = sizeof(float);
  }
  input_bind.lengths[parameter_number] = sizeof(float);
  input_bind.binds[parameter_number].length = &input_bind.lengths[
      parameter_number];
  return true;
}

bool PreparedStatement::bind_parameter(size_t parameter_number, int
    parameter_specification, bool is_null) {
  if (parameter_number >= input_bind.lengths.size()) {
    m_error = "parameter number out of range";
    return false;
  }
  if (input_bind.binds[parameter_number].buffer_type != MYSQL_TYPE_LONG) {
    m_error = "int bind does not match parameter specification";
    return false;
  }
  input_bind.is_nulls[parameter_number] = is_null;
#ifdef MYSQL57
  input_bind.binds[parameter_number].is_null = new my_bool;
#endif
#ifdef MYSQL8
  input_bind.binds[parameter_number].is_null = new bool;
#endif
  *input_bind.binds[parameter_number].is_null = is_null;
  if (is_null) {
    if (input_bind.binds[parameter_number].buffer != nullptr) {
      delete reinterpret_cast<int *>(input_bind.binds[parameter_number].buffer);
    }
    input_bind.binds[parameter_number].buffer = nullptr;
    input_bind.binds[parameter_number].buffer_length = 0;
  } else {
    input_bind.binds[parameter_number].buffer = new int;
    *(reinterpret_cast<int *>(input_bind.binds[parameter_number].buffer)) =
        parameter_specification;
    input_bind.binds[parameter_number].buffer_length = sizeof(int);
  }
  input_bind.lengths[parameter_number] = sizeof(int);
  input_bind.binds[parameter_number].length = &input_bind.lengths[
      parameter_number];
  return true;
}

bool PreparedStatement::bind_parameter(size_t parameter_number, string
    parameter_specification, bool is_null) {
  if (parameter_number >= input_bind.lengths.size()) {
    m_error = "parameter number out of range";
    return false;
  }
  if (input_bind.binds[parameter_number].buffer_type != MYSQL_TYPE_STRING) {
    m_error = "string bind does not match parameter specification";
    return false;
  }
  input_bind.is_nulls[parameter_number] = is_null;
#ifdef MYSQL57
  input_bind.binds[parameter_number].is_null = new my_bool;
#endif
#ifdef MYSQL8
  input_bind.binds[parameter_number].is_null = new bool;
#endif
  *input_bind.binds[parameter_number].is_null = is_null;
  if (is_null) {
    if (input_bind.binds[parameter_number].buffer != nullptr) {
      delete[] reinterpret_cast<char *>(input_bind.binds[parameter_number].
          buffer);
    }
    input_bind.lengths[parameter_number] = 0;
    input_bind.binds[parameter_number].buffer = nullptr;
    input_bind.binds[parameter_number].buffer_length = 0;
  } else {
    auto len = parameter_specification.length();
    input_bind.lengths[parameter_number] = len;
    input_bind.binds[parameter_number].buffer = new char[len + 1];
    std::copy(parameter_specification.begin(), parameter_specification.end(),
        reinterpret_cast<char *>(input_bind.binds[parameter_number].buffer));
    reinterpret_cast<char *>(input_bind.binds[parameter_number].buffer)[len] =
        '\0';
    input_bind.binds[parameter_number].buffer_length = len;
  }
  input_bind.binds[parameter_number].length = &input_bind.lengths[
      parameter_number];
  return true;
}

bool PreparedStatement::bind_parameter(size_t parameter_number, const char
    *parameter_specification, bool is_null) {
  return bind_parameter(parameter_number, string(parameter_specification),
      is_null);
}

bool PreparedStatement::fetch_row(Row& row) const {
  if (mysql_stmt_fetch(STMT.get()) == 0) {
    stringstream ss;
    for (size_t n = 0; n < result_bind.field_count; ++n) {
      if (n > 0) {
        ss << "<!>";
      }
      ss << string(reinterpret_cast<char *>(result_bind.binds[n].buffer),
          *result_bind.binds[n].length);
    }
    row.fill(ss.str(), "<!>", const_cast<unordered_map<string, size_t>*>(
        &column_list));
    return true;
  }
  return false;
}

size_t PreparedStatement::num_rows() const {
  if (STMT == nullptr) {
    return 0;
  }
  return mysql_stmt_num_rows(STMT.get());
}

void PreparedStatement::rewind() {
  if (STMT != nullptr) {
    mysql_stmt_data_seek(STMT.get(), 0);
  }
}

void PreparedStatement::set(string statement_specification, vector<
    enum_field_types> parameter_types) {
  statement = statement_specification;
  input_bind.binds.reset(new MYSQL_BIND[parameter_types.size()]);
  input_bind.lengths.resize(parameter_types.size(), 0);
  input_bind.is_nulls.resize(parameter_types.size(), 0);
  for (size_t n = 0; n < parameter_types.size(); ++n) {
    input_bind.binds[n].buffer_type = parameter_types[n];
    input_bind.binds[n].buffer = nullptr;
  }
  STMT.reset(nullptr);
  column_list.clear();
}

int PreparedStatement::submit(Server& server) {
  if (STMT == nullptr) {
    STMT.reset(mysql_stmt_init(server.handle()));
    if (STMT == nullptr) {
      m_error = "unable to initialize statement";
      return -1;
    }
  }
  if (mysql_stmt_prepare(STMT.get(), statement.c_str(), statement.length()) !=
      0) {
    m_error = "unable to prepare statement - " + string(mysql_stmt_error(STMT.
        get()));
    return -1;
  }
  if (mysql_stmt_param_count(STMT.get()) != input_bind.lengths.size()) {
    m_error = "bad parameter count - " + itos(mysql_stmt_param_count(STMT.
        get())) + "/" + itos(input_bind.lengths.size()) + "\nstatement: " +
        statement;
    return -1;
  }
  if (mysql_stmt_bind_param(STMT.get(), input_bind.binds.get()) != 0) {
    m_error = "unable to bind parameter(s)";
    exit(1);
  }
  if (mysql_stmt_execute(STMT.get()) != 0) {
    m_error = "unable to execute statement";
    exit(1);
  }
  result_bind.field_count = mysql_stmt_field_count(STMT.get());
  if (result_bind.field_count > 0) {
    auto m = mysql_stmt_result_metadata(STMT.get());
    auto f = mysql_fetch_fields(m);
    if (column_list.empty()) {
      for (size_t n = 0; n < mysql_num_fields(m); ++n) {
        column_list[f[n].name] = n;
      }
    }
    result_bind.binds.reset(new MYSQL_BIND[result_bind.field_count]);
    memset(result_bind.binds.get(), 0, sizeof(MYSQL_BIND) * result_bind.
        field_count);
    result_bind.lengths.resize(result_bind.field_count);
    result_bind.is_nulls.resize(result_bind.field_count);
    result_bind.errors.resize(result_bind.field_count);
    vector<char *> v;
    for (size_t n = 0; n < result_bind.field_count; ++n) {
      result_bind.binds[n].buffer_type = MYSQL_TYPE_STRING;
      switch (f[n].type) {
        case MYSQL_TYPE_BLOB: {
          result_bind.binds[n].buffer_length = 32768;
          break;
        }
        default: {
          result_bind.binds[n].buffer_length = 255;
        }
      }
      v.emplace_back(new char[result_bind.binds[n].buffer_length]);
      result_bind.binds[n].buffer = v[n];
      result_bind.binds[n].length = &result_bind.lengths[n];
#ifdef MYSQL57
      result_bind.binds[n].is_null = new my_bool;
      result_bind.binds[n].error = new my_bool;
#endif
#ifdef MYSQL8
      result_bind.binds[n].is_null = new bool;
      result_bind.binds[n].error = new bool;
#endif
      *result_bind.binds[n].is_null = result_bind.is_nulls[n];
      *result_bind.binds[n].error = result_bind.errors[n];
    }
    if (mysql_stmt_bind_result(STMT.get(), result_bind.binds.get()) != 0) {
      m_error = "unable to bind result set";
      return -1;
    }
    if (mysql_stmt_store_result(STMT.get()) != 0) {
      m_error = "unable to store the result set";
      return -1;
    }
  }
  return 0;
}

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

bool table_exists(Server& server, string absolute_table) {
  if (!regex_search(absolute_table, regex("\\."))) {
    throw my::BadSpecification_Error("bad absolute table specification '" +
        absolute_table + "'");
  }
  auto db = absolute_table.substr(0, absolute_table.find("."));
  auto tbl = absolute_table.substr(absolute_table.find(".") + 1);
  LocalQuery q;
  q.set("show tables from " + db + " like '" + tbl + "'");
  if (q.submit(server) < 0) {
    return false;
  }
  if (q.num_rows() > 0) {
    return true;
  }
  return false;
}

bool field_exists(Server& server, string absolute_table, string field) {
  if (!regex_search(absolute_table, regex("\\."))) {
    throw my::BadSpecification_Error("bad absolute table specification '" +
        absolute_table + "'");
  }
  Query q;
  q.set("show columns from " + absolute_table);
  if (q.submit(server) < 0) {
    return false;
  }
  Row row;
  while (q.fetch_row(row)) {
    if (row[0] == field) {
      return true;
    }
  }
  return false;
}

vector<string> table_names(Server& server, string database, string like_expr,
    string& error) {
  error.clear();
  string s = "show tables from " + database;
  if (!like_expr.empty()) {
    s += " like '" + like_expr + "'";
  }
  Query q(s);
  vector<string> v;
  if (q.submit(server) == 0) {
    Row row;
    while (q.fetch_row(row)) {
      v.emplace_back(row[0]);
    }
  } else {
    error = q.error();
  }
  return v;
}

bool run_prepared_statement(MySQL::Server& server, string
    prepared_statement_text, const vector<enum_field_types>& parameter_types,
    const vector<string>& parameters, MySQL::PreparedStatement&
    prepared_statement, string& error) {
  error.clear();
  if (parameter_types.size() != parameters.size()) {
    error = "numbers of parameter types and parameters don't agree";
    return false;
  }
  prepared_statement.set(prepared_statement_text, parameter_types);
  for (size_t n = 0; n < parameters.size(); ++n) {
    auto b = false;
    switch (parameter_types[n]) {
      case MYSQL_TYPE_STRING: {
        b = prepared_statement.bind_parameter(n, parameters[n], false);
        break;
      }
      case MYSQL_TYPE_LONG: {
        b = prepared_statement.bind_parameter(n, stoi(parameters[n]), false);
        break;
      }
      case MYSQL_TYPE_FLOAT: {
        b = prepared_statement.bind_parameter(n, stof(parameters[n]), false);
        break;
      }
      default: {
        error = "invalid parameter type";
        return false;
      }
    }
    if (!b) {
      error = "bind (" + itos(n) + ") failed - '" + prepared_statement.error() +
          "'";
      return false;
    }
  }
  if (prepared_statement.submit(server) < 0) {
    error = "prepared statement failed - '" + prepared_statement.error() + "'";
    return false;
  }
  return true;
}

} // end namespace MySQL
