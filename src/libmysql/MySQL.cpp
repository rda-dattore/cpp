#include <iostream>
#include <thread>
#include <MySQL.hpp>
#include <strutils.hpp>

namespace MySQL {

Row& Row::operator=(const Row& source)
{
  if (this == &source) {
    return *this;
  }
  columns=source.columns;
  column_list=source.column_list;
  return *this;
}

void Row::clear()
{
  columns.resize(0);
  column_list=nullptr;
}

void Row::fill(MYSQL_ROW& Row,unsigned long *lengths,size_t num_fields,std::unordered_map<std::string,size_t> *p_column_list)
{
  columns.resize(num_fields);
  for (size_t n=0; n < columns.size(); ++n) {
    columns[n].assign(Row[n],lengths[n]);
  }
  column_list=p_column_list;
}

void Row::fill(const std::string& Row,const char *separator,std::unordered_map<std::string,size_t> *p_column_list)
{
  auto cols=strutils::split(Row,separator);
  columns.resize(cols.size());
  for (size_t n=0; n < columns.size(); ++n) {
    columns[n]=cols[n];
  }
  column_list=p_column_list;
}

const std::string& Row::operator[](size_t column_number) const
{
  if (column_number < columns.size()) {
    return columns[column_number];
  }
  else {
    return empty_column;
  }
}

const std::string& Row::operator[](std::string column_name) const
{
  if (column_list != nullptr) {
    auto e=column_list->find(strutils::to_lower(column_name));
    if (e == column_list->end()) {
	return empty_column;
    }
    else {
	return columns[e->second];
    }
  }
  else {
    return empty_column;
  }
}

std::ostream& operator<<(std::ostream& out_stream,const Row& row)
{
  for (size_t n=0; n < row.columns.size(); ++n) {
    if (n > 0) {
	out_stream << row._column_delimiter;
    }
    out_stream << row.columns[n];
  }
  return out_stream;
}

int Server::command(std::string command,std::string& result)
{
  mysql_real_query(&mysql,command.c_str(),command.length());
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
  mysql_real_query(&mysql,s.c_str(),s.length());
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
  if (mysql_query(&mysql,"show databases") > 0) {
    std::cerr << "Error: unable to fill database list because " <<  mysql_error(&mysql) << std::endl;
    exit(1);
  }
  std::list<std::string> list;
  auto dbquery_result=mysql_store_result(&mysql);
  MYSQL_ROW dbrow;
  while ( (dbrow=mysql_fetch_row(dbquery_result))) {
    list.emplace_back(reinterpret_cast<char *>(*dbrow));
  }
  mysql_free_result(dbquery_result);
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
  auto insert_string="insert into "+absolute_table+" values ("+row_specification+")";
  if (!on_duplicate_key.empty()) {
    insert_string+=" on duplicate key "+on_duplicate_key;
  }
  insert_string+=";";
  mysql_real_query(&mysql,insert_string.c_str(),insert_string.length());
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
  std::string insert_string;
  insert_string.reserve(len);
  for (auto& spec : row_specifications) {
    if (insert_string.length() > MAX_QUERY_LENGTH) {
	insert_string="insert into "+absolute_table+" values "+insert_string;
	if (!on_duplicate_key.empty()) {
	  insert_string+=" on duplicate key "+on_duplicate_key;
	}
	insert_string+=";";
	mysql_real_query(&mysql,insert_string.c_str(),insert_string.length());
	_error="";
	if (mysql_errno(&mysql) > 0) {
	  _error=mysql_error(&mysql)+std::string(" - errno: ")+strutils::itos(mysql_errno(&mysql));
	}
	if (!_error.empty()) {
	  return -1;
	}
	insert_string="";
    }
    if (!insert_string.empty()) {
	insert_string+=", ";
    }
    insert_string+="("+spec+")";
  }
  if (!insert_string.empty()) {
    insert_string="insert into "+absolute_table+" values "+insert_string;
    if (!on_duplicate_key.empty()) {
	insert_string+=" on duplicate key "+on_duplicate_key;
    }
    insert_string+=";";
    mysql_real_query(&mysql,insert_string.c_str(),insert_string.length());
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
  auto insert_string="insert into "+absolute_table+" ("+column_list+") values ("+value_list+")";
  if (!on_duplicate_key.empty()) {
    insert_string+=" on duplicate key "+on_duplicate_key;
  }
  insert_string+=";";
  mysql_real_query(&mysql,insert_string.c_str(),insert_string.length());
  _error="";
  if (mysql_errno(&mysql) > 0) {
    _error=mysql_error(&mysql)+std::string(" - errno: ")+strutils::itos(mysql_errno(&mysql))+"\nfailed insert: '"+insert_string+"'";
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
    if (value.is_numeric) {
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
  auto update_string="update "+absolute_table+" set "+column_name_value_pair;
  if (!where_conditions.empty()) {
    update_string+=" where "+where_conditions;
  }
  update_string+=";";
  mysql_real_query(&mysql,update_string.c_str(),update_string.length());
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
  auto update_string="update "+absolute_table+" set "+*it;
  auto end=column_name_value_pairs.end();
  for (++it; it != end; ++it) {
    update_string+=", "+*it;
  }
  if (!where_conditions.empty()) {
    update_string+=" where "+where_conditions;
  }
  update_string+=";";
  mysql_real_query(&mysql,update_string.c_str(),update_string.length());
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

void Query::set(std::string query_specification)
{
  size_t idx=query_specification.find("from");
  if (idx == std::string::npos && query_specification.find("show") != 0) {
    std::cerr << "Error: bad query '" << query_specification << "'" << std::endl;
    exit(1);
  }
  if (idx > 0) {
    auto columns=query_specification.substr(0,idx-1);
    columns=columns.substr(columns.find("select")+7);
    strutils::trim(columns);
    fill_column_list(columns);
  }
  query=query_specification;
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
  mysql_real_query(server.handle(),query.c_str(),query.length());
  _error="";
  size_t num_tries=0;
  while (mysql_errno(server.handle()) == 1205 && num_tries < 3) {
    mysql_real_query(server.handle(),query.c_str(),query.length());
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
  int in_parens=0;
  for (size_t n=0; n < columns.length(); ++n) {
    if (columns[n] == '(') {
	++in_parens;
    }
    else if (columns[n] == ')') {
	--in_parens;
    }
    if (in_parens == 0 && columns[n] == ',') {
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

QueryIterator Query::begin()
{
  auto i=QueryIterator(*this,false);
  ++i;
  return i;
}

QueryIterator Query::end()
{
  return QueryIterator(*this,true);
}

const QueryIterator& QueryIterator::operator++()
{
  if (!_query.fetch_row(row)) {
    _is_end=true;
  }
  return *this;
}

Row& QueryIterator::operator*()
{
  if (_is_end) {
    row=Row();
  }
  return row;
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
  mysql_real_query(server.handle(),query.c_str(),query.length());
  _error="";
  size_t num_tries=0;
  while (mysql_errno(server.handle()) == 1205 && num_tries < 3) {
    mysql_real_query(server.handle(),query.c_str(),query.length());
    ++num_tries;
  }
  if (mysql_errno(server.handle()) > 0) {
    _error=mysql_error(server.handle())+std::string(" - errno: ")+strutils::itos(mysql_errno(server.handle()));
  }
  if (!_error.empty()) {
    return -1;
  }
  RESULT.reset(mysql_store_result(server.handle()));
  _num_rows=mysql_num_rows(RESULT.get());
  num_fields=mysql_num_fields(RESULT.get());
  return 0;
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
    mysql_real_query(server.handle(),query.c_str(),query.length());
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

QueryIterator LocalQuery::begin()
{
  rewind();
  return Query::begin();
}

bool PreparedStatement::bind_parameter(size_t parameter_number,float parameter_specification,bool is_null)
{
  if (parameter_number >= input_bind.lengths.size()) {
    _error="parameter number out of range";
    return false;
  }
  if (input_bind.binds[parameter_number].buffer_type != MYSQL_TYPE_FLOAT) {
    _error="float bind does not match parameter specification";
    return false;
  }
  input_bind.is_nulls[parameter_number]=is_null;
  input_bind.binds[parameter_number].is_null=&input_bind.is_nulls[parameter_number];
  if (is_null) {
    if (input_bind.binds[parameter_number].buffer != nullptr) {
	delete reinterpret_cast<float *>(input_bind.binds[parameter_number].buffer);
    }
    input_bind.binds[parameter_number].buffer=nullptr;
    input_bind.binds[parameter_number].buffer_length=0;
  }
  else {
    input_bind.binds[parameter_number].buffer=new float;
    *(reinterpret_cast<float *>(input_bind.binds[parameter_number].buffer))=parameter_specification;
    input_bind.binds[parameter_number].buffer_length=sizeof(float);
  }
  input_bind.lengths[parameter_number]=sizeof(float);
  input_bind.binds[parameter_number].length=&input_bind.lengths[parameter_number];
  return true;
}

bool PreparedStatement::bind_parameter(size_t parameter_number,int parameter_specification,bool is_null)
{
  if (parameter_number >= input_bind.lengths.size()) {
    _error="parameter number out of range";
    return false;
  }
  if (input_bind.binds[parameter_number].buffer_type != MYSQL_TYPE_LONG) {
    _error="int bind does not match parameter specification";
    return false;
  }
  input_bind.is_nulls[parameter_number]=is_null;
  input_bind.binds[parameter_number].is_null=&input_bind.is_nulls[parameter_number];
  if (is_null) {
    if (input_bind.binds[parameter_number].buffer != nullptr) {
	delete reinterpret_cast<int *>(input_bind.binds[parameter_number].buffer);
    }
    input_bind.binds[parameter_number].buffer=nullptr;
    input_bind.binds[parameter_number].buffer_length=0;
  }
  else {
    input_bind.binds[parameter_number].buffer=new int;
    *(reinterpret_cast<int *>(input_bind.binds[parameter_number].buffer))=parameter_specification;
    input_bind.binds[parameter_number].buffer_length=sizeof(int);
  }
  input_bind.lengths[parameter_number]=sizeof(int);
  input_bind.binds[parameter_number].length=&input_bind.lengths[parameter_number];
  return true;
}

bool PreparedStatement::bind_parameter(size_t parameter_number,std::string parameter_specification,bool is_null)
{
  if (parameter_number >= input_bind.lengths.size()) {
    _error="parameter number out of range";
    return false;
  }
  if (input_bind.binds[parameter_number].buffer_type != MYSQL_TYPE_STRING) {
    _error="string bind does not match parameter specification";
    return false;
  }
  input_bind.is_nulls[parameter_number]=is_null;
  input_bind.binds[parameter_number].is_null=&input_bind.is_nulls[parameter_number];
  if (is_null) {
    if (input_bind.binds[parameter_number].buffer != nullptr) {
	delete[] reinterpret_cast<char *>(input_bind.binds[parameter_number].buffer);
    }
    auto len=0;
    input_bind.lengths[parameter_number]=len;
    input_bind.binds[parameter_number].buffer=nullptr;
    input_bind.binds[parameter_number].buffer_length=len;
  }
  else {
    auto len=parameter_specification.length();
    input_bind.lengths[parameter_number]=len;
    input_bind.binds[parameter_number].buffer=new char[len+1];
    std::copy(parameter_specification.begin(),parameter_specification.end(),reinterpret_cast<char *>(input_bind.binds[parameter_number].buffer));
    reinterpret_cast<char *>(input_bind.binds[parameter_number].buffer)[len]='\0';
    input_bind.binds[parameter_number].buffer_length=len;
  }
  input_bind.binds[parameter_number].length=&input_bind.lengths[parameter_number];
  return true;
}

bool PreparedStatement::bind_parameter(size_t parameter_number,const char *parameter_specification,bool is_null)
{
  return bind_parameter(parameter_number,std::string(parameter_specification),is_null);
}

size_t PreparedStatement::num_rows() const
{
  if (STMT == nullptr) {
    return 0;
  }
  else {
    return mysql_stmt_num_rows(STMT.get());
  }
}

bool PreparedStatement::fetch_row(Row& row) const
{
  if (mysql_stmt_fetch(STMT.get()) == 0) {
    std::stringstream row_ss;
    for (size_t n=0; n < result_bind.field_count; ++n) {
	if (!row_ss.str().empty()) {
	  row_ss << "<!>";
	}
	row_ss << std::string(reinterpret_cast<char *>(result_bind.binds[n].buffer),*result_bind.binds[n].length);
    }
    row.fill(row_ss.str(),"<!>",const_cast<std::unordered_map<std::string,size_t>*>(&column_list));
    return true;
  }
  else {
    return false;
  }
}

void PreparedStatement::set(std::string statement_specification,std::vector<enum_field_types> parameter_types)
{
  statement=statement_specification;
  input_bind.binds.reset(new MYSQL_BIND[parameter_types.size()]);
  input_bind.lengths.resize(parameter_types.size(),0);
  input_bind.is_nulls.resize(parameter_types.size(),0);
  for (size_t n=0; n < parameter_types.size(); ++n) {
    input_bind.binds[n].buffer_type=parameter_types[n];
    input_bind.binds[n].buffer=nullptr;
  }
  STMT.reset(nullptr);
  column_list.clear();
}

int PreparedStatement::submit(Server& server)
{
  if (STMT == nullptr) {
    STMT.reset(mysql_stmt_init(server.handle()));
    if (STMT == nullptr) {
	_error="unable to initialize statement";
	return -1;
    }
  }
  if (mysql_stmt_prepare(STMT.get(),statement.c_str(),statement.length()) != 0) {
    _error="unable to prepare statement - "+std::string(mysql_stmt_error(STMT.get()));
    return -1;
  }
  if (mysql_stmt_param_count(STMT.get()) != input_bind.lengths.size()) {
    _error="bad parameter count - "+strutils::itos(mysql_stmt_param_count(STMT.get()))+"/"+strutils::itos(input_bind.lengths.size())+"\nstatement: "+statement;
    return -1;
  }
  if (mysql_stmt_bind_param(STMT.get(),input_bind.binds.get()) != 0) {
    _error="unable to bind parameter(s)";
    exit(1);
  }
  if (mysql_stmt_execute(STMT.get()) != 0) {
    _error="unable to execute statement";
    exit(1);
  }
  result_bind.field_count=mysql_stmt_field_count(STMT.get());
  if (result_bind.field_count > 0) {
    result_bind.binds.reset(new MYSQL_BIND[result_bind.field_count]);
    result_bind.lengths.resize(result_bind.field_count);
    result_bind.is_nulls.resize(result_bind.field_count);
    result_bind.errors.resize(result_bind.field_count);
    std::vector<char *> fields;
    for (size_t n=0; n < result_bind.field_count; ++n) {
	result_bind.binds[n].buffer_type=MYSQL_TYPE_STRING;
	result_bind.binds[n].buffer_length=255;
	fields.emplace_back(new char[result_bind.binds[n].buffer_length]);
	result_bind.binds[n].buffer=fields[n];
	result_bind.binds[n].length=&result_bind.lengths[n];
	result_bind.binds[n].is_null=&result_bind.is_nulls[n];
	result_bind.binds[n].error=&result_bind.errors[n];
    }
    if (mysql_stmt_bind_result(STMT.get(),result_bind.binds.get()) != 0) {
	_error="unable to bind result set";
	return -1;
    }
    if (mysql_stmt_store_result(STMT.get()) != 0) {
	_error="unable to store the result set";
	return -1;
    }
    if (column_list.size() == 0) {
	auto field_metadata=mysql_stmt_result_metadata(STMT.get());
	auto columns=mysql_fetch_fields(field_metadata);
	for (size_t n=0; n < mysql_num_fields(field_metadata); ++n) {
	  column_list[columns[n].name]=n;
	}
    }
  }
  return 0;
}

PreparedStatementIterator PreparedStatement::begin()
{
  auto i=PreparedStatementIterator(*this,false);
  ++i;
  return i;
}

PreparedStatementIterator PreparedStatement::end()
{
  return PreparedStatementIterator(*this,true);
}

const PreparedStatementIterator& PreparedStatementIterator::operator++()
{
  if (!_pstmt.fetch_row(row)) {
    _is_end=true;
  }
  return *this;
}

Row& PreparedStatementIterator::operator*()
{
  if (_is_end) {
    row=Row();
  }
  return row;
}

bool table_exists(Server& server,std::string absolute_table)
{
  if (!strutils::contains(absolute_table,".")) {
    std::cerr << "Error: bad table specification " << absolute_table << std::endl;
    exit(1);
  }
  auto db_name=absolute_table.substr(0,absolute_table.find("."));
  auto tb_name=absolute_table.substr(absolute_table.find(".")+1);
  LocalQuery query;
  query.set("show tables from "+db_name+" like '"+tb_name+"'");
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
	list.emplace_back(row[0]);
    }
  }
  else {
    error=query.error();
  }
  return list;
}

} // end namespace MySQL
