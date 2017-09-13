#include <iostream>
#include <thread>
#include <MySQL.hpp>
#include <strutils.hpp>

namespace MySQL {

Row::~Row()
{
  if (columns != nullptr) {
    delete[] columns;
  }
  column_list=nullptr;
}

Row& Row::operator=(const Row& source)
{
  if (this == &source) {
    return *this;
  }
  check_alloc(source.capacity);
  num_columns=source.num_columns;
  for (size_t n=0; n < num_columns; ++n) {
    columns[n]=source.columns[n];
  }
  column_list=source.column_list;
  return *this;
}

void Row::clear()
{
  if (columns != nullptr) {
    delete[] columns;
  }
  capacity=num_columns=0;
  columns=nullptr;
}

void Row::fill(MYSQL_ROW& Row,unsigned long *lengths,size_t num_fields,std::unordered_map<std::string,size_t> *p_column_list)
{
  check_alloc(num_fields);
  num_columns=num_fields;
  for (size_t n=0; n < num_columns; ++n) {
    columns[n].assign(Row[n],lengths[n]);
  }
  column_list=p_column_list;
}

void Row::fill(const std::string& Row,const char *separator,std::unordered_map<std::string,size_t> *p_column_list)
{
  auto cols=strutils::split(Row,separator);
  num_columns=cols.size();
  check_alloc(num_columns);
  for (size_t n=0; n < num_columns; ++n) {
    columns[n]=cols[n];
  }
  column_list=p_column_list;
}

const std::string& Row::operator[](size_t column_number) const
{
  if (column_number < num_columns) {
    return columns[column_number];
  }
  else {
    return empty_column;
  }
}

const std::string& Row::operator[](std::string column_name) const
{
  auto e=column_list->find(column_name);
  if (e == column_list->end()) {
    return empty_column;
  }
  else {
    return columns[e->second];
  }
}

void Row::check_alloc(size_t length)
{
  if (length > capacity) {
    if (columns != nullptr) {
	delete[] columns;
    }
    capacity=length;
    columns=new std::string[capacity];
  }
}

int Server::command(std::string command,std::string& result)
{
  mysql_query(&mysql,command.c_str());
  _error="";
  if (mysql_errno(&mysql) > 0) {
    _error=mysql_error(&mysql)+std::string(" - errno: ")+strutils::itos(mysql_errno(&mysql));
  }
  if (!_error.empty()) {
    return -1;
  }
  MYSQL_RES *res=mysql_store_result(&mysql);
  MYSQL_ROW row;
  size_t num_fields=0;
  std::string separator("|");
  result="";
  if (res != nullptr) {
    while ( (row=mysql_fetch_row(res))) {
	if (!result.empty()) {
	  result+="\n";
	}
	else {
	  num_fields=mysql_num_fields(res);
	}
	for (size_t n=0; n < num_fields; ++n) {
	  result+=separator;
	  if (row[n]) {
	    result+=row[n];
	  }
	  else {
	    result+="NULL";
	  }
	}
	result+=separator;
    }
    mysql_free_result(res);
  }
  return 0;
}

void Server::connect(std::string host,std::string user,std::string password,std::string db,int timeout)
{
  if (mysql_init(&mysql) == nullptr) {
    return;
  }
  mysql_thread_init();
  if (timeout >= 0) {
    mysql_options(&mysql,MYSQL_OPT_CONNECT_TIMEOUT,reinterpret_cast<char *>(&timeout));
  }
  auto port=0;
  size_t idx;
  if ( (idx=host.find(":")) != std::string::npos) {
    port=std::stoi(host.substr(idx+1));
    host=host.substr(0,idx);
  }
  mysql_real_connect(&mysql,host.c_str(),user.c_str(),password.c_str(),db.c_str(),port,nullptr,0);
  _error="";
  auto num_tries=0;
  while (mysql_errno(&mysql) > 0 && num_tries < 3) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    ++num_tries;
  }
  if (mysql_errno(&mysql) > 0) {
    _error=mysql_error(&mysql)+std::string(" - errno: ")+strutils::itos(mysql_errno(&mysql));
  }
  if (_error.empty()) {
    _connected=true;
  }
}

int Server::_delete(std::string absolute_table,std::string where_conditions)
{
  std::string s="delete from "+absolute_table;
  if (!where_conditions.empty()) {
    s+=" where "+where_conditions+";";
  }
  mysql_query(&mysql,s.c_str());
  _error="";
  if (mysql_errno(&mysql) > 0) {
    _error=mysql_error(&mysql)+std::string(" - errno: ")+strutils::itos(mysql_errno(&mysql));
  }
  if (!_error.empty()) {
    return -1;
  }
  return 0;
}

std::list<std::string> Server::db_names()
{
  MYSQL_RES *dbqueryResult;
  MYSQL_ROW dbrow;
  std::list<std::string> list;

  if (mysql_query(&mysql,"show databases") > 0) {
    std::cerr << "Error: unable to fill database list because " <<  mysql_error(&mysql) << std::endl;
    exit(1);
  }
  dbqueryResult=mysql_store_result(&mysql);
  while ( (dbrow=mysql_fetch_row(dbqueryResult)))
    list.push_back(reinterpret_cast<char *>(*dbrow));
  mysql_free_result(dbqueryResult);
  return list;
}

void Server::disconnect()
{
  if (_connected) {
    mysql_close(&mysql);
  }
  _connected=false;
}

int Server::insert(const std::string& absolute_table,const std::string& row_specification,const std::string& on_duplicate_key)
{
  auto insertString="insert into "+absolute_table+" values ("+row_specification+")";
  if (!on_duplicate_key.empty()) {
    insertString+=" on duplicate key "+on_duplicate_key;
  }
  insertString+=";";
  mysql_query(&mysql,insertString.c_str());
  _error="";
  if (mysql_errno(&mysql) > 0) {
    _error=mysql_error(&mysql)+std::string(" - errno: ")+strutils::itos(mysql_errno(&mysql));
  }
  if (!_error.empty()) {
    return -1;
  }
  else {
    return 0;
  }
}

int Server::insert(const std::string& absolute_table,std::list<std::string>& row_specifications,const std::string& on_duplicate_key)
{
  const int MAX_QUERY_LENGTH=300000;
  auto len=row_specifications.front().length()*row_specifications.size();
  len= (len < MAX_QUERY_LENGTH) ? len+len/2 : MAX_QUERY_LENGTH+MAX_QUERY_LENGTH/2;
  std::string insertString;
  insertString.reserve(len);
  for (auto& spec : row_specifications) {
    if (insertString.length() > MAX_QUERY_LENGTH) {
	insertString="insert into "+absolute_table+" values "+insertString;
	if (!on_duplicate_key.empty()) {
	  insertString+=" on duplicate key "+on_duplicate_key;
	}
	insertString+=";";
	mysql_query(&mysql,insertString.c_str());
	_error="";
	if (mysql_errno(&mysql) > 0) {
	  _error=mysql_error(&mysql)+std::string(" - errno: ")+strutils::itos(mysql_errno(&mysql));
	}
	if (!_error.empty()) {
	  return -1;
	}
	insertString="";
    }
    if (!insertString.empty()) {
	insertString+=", ";
    }
    insertString+="("+spec+")";
  }
  if (!insertString.empty()) {
    insertString="insert into "+absolute_table+" values "+insertString;
    if (!on_duplicate_key.empty()) {
	insertString+=" on duplicate key "+on_duplicate_key;
    }
    insertString+=";";
    mysql_query(&mysql,insertString.c_str());
    _error="";
    if (mysql_errno(&mysql) > 0) {
	_error=mysql_error(&mysql)+std::string(" - errno: ")+strutils::itos(mysql_errno(&mysql));
    }
    if (!_error.empty()) {
	return -1;
    }
  }
  return 0;
}

int Server::insert(const std::string& absolute_table,const std::string& column_list,const std::string& value_list,const std::string& on_duplicate_key)
{
  auto insertString="insert into "+absolute_table+" ("+column_list+") values ("+value_list+")";
  if (!on_duplicate_key.empty()) {
    insertString+=" on duplicate key "+on_duplicate_key;
  }
  insertString+=";";
  mysql_query(&mysql,insertString.c_str());
  _error="";
  if (mysql_errno(&mysql) > 0) {
    _error=mysql_error(&mysql)+std::string(" - errno: ")+strutils::itos(mysql_errno(&mysql));
  }
  if (!_error.empty()) {
    return -1;
  }
  else {
    return 0;
  }
}

int Server::insert(RowInsert& row_insert)
{
  std::string row_specification;
  for (auto& value : row_insert.values) {
    if (!row_specification.empty()) {
	row_specification+=",";
    }
    if (value.isNumeric) {
	row_specification+=value.value;
    }
    else {
	if (value.value.empty() || value.value == "DEFAULT") {
	  row_specification+="DEFAULT";
	}
	else {
	  row_specification+="'"+value.value+"'";
	}
    }
  }
  return insert(row_insert.absolute_table,row_specification,row_insert.duplicate_action);
}

int Server::update(std::string absolute_table,std::string column_name_value_pair,std::string where_conditions)
{
  if (column_name_value_pair.empty()) {
    return -1;
  }
  auto updateString="update "+absolute_table+" set "+column_name_value_pair;
  if (!where_conditions.empty()) {
    updateString+=" where "+where_conditions;
  }
  updateString+=";";
  mysql_query(&mysql,updateString.c_str());
  _error="";
  if (mysql_errno(&mysql) > 0) {
    _error=mysql_error(&mysql)+std::string(" - errno: ")+strutils::itos(mysql_errno(&mysql));
  }
  if (!_error.empty()) {
    return -1;
  }
  return 0;
}

int Server::update(std::string absolute_table,std::list<std::string>& column_name_value_pairs,std::string where_conditions)
{
  if (column_name_value_pairs.empty()) {
    return -1;
  }
  auto it=column_name_value_pairs.begin();
  auto updateString="update "+absolute_table+" set "+*it;
  auto end=column_name_value_pairs.end();
  for (++it; it != end; ++it) {
    updateString+=", "+*it;
  }
  if (!where_conditions.empty()) {
    updateString+=" where "+where_conditions;
  }
  updateString+=";";
  mysql_query(&mysql,updateString.c_str());
  _error="";
  if (mysql_errno(&mysql) > 0) {
    _error=mysql_error(&mysql)+std::string(" - errno: ")+strutils::itos(mysql_errno(&mysql));
  }
  if (!_error.empty()) {
    return -1;
  }
  return 0;
}

bool Query::fetch_row(Row& row) const
{
  if (RESULT == nullptr) {
    row.clear();
    return false;
  }
  MYSQL_ROW Row;
  if ( (Row=mysql_fetch_row(RESULT.get()))) {
    auto lengths=mysql_fetch_lengths(RESULT.get());
    row.fill(Row,lengths,num_fields,const_cast<std::unordered_map<std::string,size_t>*>(&column_lists.front()));
    return true;
  }
  else {
    row.clear();
    return false;
  }
}

void Query::set(std::string columns,std::string tables,std::string where_conditions)
{
  fill_column_list(columns);
  query="select "+columns+" from "+tables;
  if (!where_conditions.empty()) {
    query+=" where "+where_conditions;
  }
}

void Query::set(std::string querySpecification)
{
  size_t idx=querySpecification.find("from");
  if (idx == std::string::npos && querySpecification.find("show") != 0) {
    std::cerr << "Error: bad query '" << querySpecification << "'" << std::endl;
    exit(1);
  }
  if (idx > 0) {
    auto columns=querySpecification.substr(0,idx-1);
    columns=columns.substr(columns.find("select")+7);
    strutils::trim(columns);
    fill_column_list(columns);
  }
  query=querySpecification;
}

int Query::submit(Server& server)
{
  if (!server) {
    _error="Query error: not connected to server";
    return -1;
  }
  if (RESULT != nullptr) {
    RESULT.reset(nullptr);
  }
  _thread_id=mysql_thread_id(server.handle());
  mysql_query(server.handle(),query.c_str());
  _error="";
  size_t num_tries=0;
  while (mysql_errno(server.handle()) == 1205 && num_tries < 3) {
    mysql_query(server.handle(),query.c_str());
    ++num_tries;
  }
  if (mysql_errno(server.handle()) > 0) {
    _error=mysql_error(server.handle())+std::string(" - errno: ")+strutils::itos(mysql_errno(server.handle()));
  }
  if (!_error.empty()) {
    return -1;
  }
  RESULT.reset(mysql_use_result(server.handle()));
  num_fields=mysql_num_fields(RESULT.get());
  return 0;
}

void Query::fill_column_list(std::string columns)
{
  column_lists.emplace_front();
  columns=strutils::to_lower(columns);
  size_t idx;
  if ( (idx=columns.find("distinct")) != std::string::npos) {
    columns=columns.substr(idx+8);
    strutils::trim(columns);
  }
  int inParens=0;
  for (size_t n=0; n < columns.length(); ++n) {
    if (columns[n] == '(') {
	++inParens;
    }
    else if (columns[n] == ')') {
	--inParens;
    }
    if (inParens == 0 && columns[n] == ',') {
	columns=columns.substr(0,n)+";"+columns.substr(n+1);
    }
  }
  size_t n=0;
  auto cparts=strutils::split(columns,";");
  for (auto& key : cparts) {
    if ( (idx=key.find(" as ")) != std::string::npos) {
	key=key.substr(idx+4);
	strutils::trim(key);
    }
    column_lists.front()[key]=n++;
  }
}

bool LocalQuery::fetch_row(Row& row) const
{
  if (RESULT == nullptr) {
    row.clear();
    return false;
  }
  MYSQL_ROW Row;
  if ( (Row=mysql_fetch_row(RESULT.get()))) {
    auto lengths=mysql_fetch_lengths(RESULT.get());
    row.fill(Row,lengths,num_fields,const_cast<std::unordered_map<std::string,size_t>*>(&column_lists.front()));
    return true;
  }
  else {
    row.clear();
    return false;
  }
}

int LocalQuery::explain(Server& server)
{
  if (!server) {
    _error="Query error: not connected to server";
    return -1;
  }
  if (RESULT != nullptr) {
    RESULT.reset(nullptr);
    _num_rows=num_fields=0;
  }
  mysql_query(server.handle(),("explain "+query).c_str());
  _error="";
  size_t num_tries=0;
  while (mysql_errno(server.handle()) == 1205 && num_tries < 3) {
    mysql_query(server.handle(),query.c_str());
    ++num_tries;
  }
  if (mysql_errno(server.handle()) > 0)
    _error=mysql_error(server.handle())+std::string(" - errno: ")+strutils::itos(mysql_errno(server.handle()));
  if (!_error.empty()) {
    return -1;
  }
  RESULT.reset(mysql_store_result(server.handle()));
  _num_rows=mysql_num_rows(RESULT.get());
  num_fields=mysql_num_fields(RESULT.get());
  return 0;
}

int LocalQuery::submit(Server& server)
{
  if (!server) {
    _error="Query error: not connected to server";
    return -1;
  }
  if (RESULT != nullptr) {
    RESULT.reset(nullptr);
    _num_rows=num_fields=0;
  }
  _thread_id=mysql_thread_id(server.handle());
  mysql_query(server.handle(),query.c_str());
  _error="";
  size_t num_tries=0;
  while (mysql_errno(server.handle()) == 1205 && num_tries < 3) {
    mysql_query(server.handle(),query.c_str());
    ++num_tries;
  }
  if (mysql_errno(server.handle()) > 0)
    _error=mysql_error(server.handle())+std::string(" - errno: ")+strutils::itos(mysql_errno(server.handle()));
  if (!_error.empty()) {
    return -1;
  }
  RESULT.reset(mysql_store_result(server.handle()));
  _num_rows=mysql_num_rows(RESULT.get());
  num_fields=mysql_num_fields(RESULT.get());
  return 0;
}

bool table_exists(Server& server,std::string absolute_table)
{
  if (!strutils::contains(absolute_table,".")) {
    std::cerr << "Error: bad table specification " << absolute_table << std::endl;
    exit(1);
  }
  auto dbName=absolute_table.substr(0,absolute_table.find("."));
  auto tbName=absolute_table.substr(absolute_table.find(".")+1);
  LocalQuery query;
  query.set("show tables from "+dbName+" like '"+tbName+"'");
  if (query.submit(server) < 0) {
    return false;
  }
  if (query.num_rows() > 0) {
    return true;
  }
  else {
    return false;
  }
}

bool field_exists(Server& server,std::string absolute_table,std::string field)
{
  if (!strutils::contains(absolute_table,".")) {
    std::cerr << "Error: bad table specification " << absolute_table << std::endl;
    exit(1);
  }
  Query query;
  query.set("show columns from "+absolute_table);
  if (query.submit(server) < 0) {
    return false;
  }
  Row row;
  while (query.fetch_row(row)) {
    if (row[0] == field) {
	return true;
    }
  }
  return false;
}

std::list<std::string> table_names(Server& server,std::string database,std::string like_expr,std::string& error)
{
  Query query;
  if (!like_expr.empty()) {
    query.set("show tables from "+database+" like '"+like_expr+"'");
  }
  else {
    query.set("show tables from "+database);
  }
  error="";
  std::list<std::string> list;
  if (query.submit(server) == 0) {
    Row row;
    while (query.fetch_row(row)) {
	list.push_back(row[0]);
    }
  }
  else {
    error=query.error();
  }
  return list;
}

} // end namespace MySQL
