#include <iostream>
#include <MySQL.hpp>
#include <string.h>

extern "C" {

int mysqlconnect_(char *host,char *username,char *password,char *db)
{
  MySQL::Server *s=new MySQL::Server(host,username,password,db);
  if (*s) {
    return reinterpret_cast<long long>(s);
  }
  else {
    return -1;
  }
}

long long mysqlconnect64_(char *host,char *username,char *password,char *db)
{
  return mysqlconnect_(host,username,password,db);
}

long long mysqlquery_(int *server,char *column_list,char *table_name,char *where_conditions,int *num_rows,int *num_columns,int *status)
{
  auto q=new MySQL::LocalQuery();
  auto s=reinterpret_cast<MySQL::Server *>(*server);
  q->set(column_list,table_name,where_conditions);
  if (q->submit(*s) < 0) {
    *num_rows=0;
    *num_columns=0;
    *status=-1;
  }
  else {
    *num_rows=q->num_rows();
    if (*num_rows > 0) {
	*num_columns=q->length();
    }
    else {
	*num_columns=0;
    }
    *status=0;
  }
  return (long long)q;
}

long long mysqlquery64_(long long *server,char *column_list,char *table_name,char *where_conditions,long long *num_rows,long long *num_columns,long long *status)
{
  int srv=*server;
  int nr,nc,ist;
  long long qstat;
  qstat=mysqlquery_(&srv,column_list,table_name,where_conditions,&nr,&nc,&ist);
  *num_rows=nr;
  *num_columns=nc;
  *status=ist;
  return qstat;
}

void mysqlqueryerror_(int *query)
{
  auto q=reinterpret_cast<MySQL::Query *>(*query);
  std::cerr << q->error() << std::endl;
}

void mysqlqueryerror64_(long long *query)
{
  int q=*query;
  mysqlqueryerror_(&q);
}

void mysqlqueryresult_(int *query,int *row_index,int *num_fields,int *field_length,char *row)
{
  auto q=reinterpret_cast<MySQL::LocalQuery *>(*query);
  q->rewind();
  MySQL::Row _row;
  bool found_row=false;
  for (size_t n=0; n < static_cast<size_t>(*row_index); ++n) {
    found_row=q->fetch_row(_row);
  }
  if (found_row) {
    size_t nf= (*num_fields <= (int)q->length()) ? *num_fields : q->length();
    size_t n=0;
    for (; n < nf; ++n) {
	if (*field_length > static_cast<int>(_row[n].length())) {
	  strcpy(&row[n*(*field_length)],_row[n].c_str());
	  for (size_t m=_row[n].length(); m < static_cast<size_t>(*field_length); ++m) {
	    row[n*(*field_length)+m]=' ';
	  }
	}
	else {
	  std::copy(_row[n].begin(),_row[n].begin()+*field_length,&row[n*(*field_length)]);
	}
    }
    for (; n < static_cast<size_t>(*num_fields); ++n) {
	for (size_t m=0; m < static_cast<size_t>(*field_length); ++m) {
	  row[n*(*field_length)+m]=' ';
	}
    }
  }
  else {
    for (size_t n=0; n < static_cast<size_t>(*num_fields); ++n) {
	for (size_t m=0; m < static_cast<size_t>(*field_length); ++m) {
	  row[n*(*field_length)+m]=' ';
	}
    }
  }
}

void mysqlqueryresult64_(long long *query,long long *row_index,long long *num_fields,long long *field_length,char *row)
{
  int q=static_cast<int>(*query);
  int ri=static_cast<int>(*row_index);
  int nf=static_cast<int>(*num_fields);
  int fl=static_cast<int>(*field_length);
  mysqlqueryresult_(&q,&ri,&nf,&fl,row);
}

int mysqlinsert_(int *server,char *table_name,char *values,int *vlen,char *duplicateKeyCondition)
{

  auto s=reinterpret_cast<MySQL::Server *>(*server);
  int lastInsertID;
  if (s->insert(table_name,std::string(values,*vlen),duplicateKeyCondition) < 0) {
    lastInsertID=-1;
  }
  else {
    lastInsertID=(int)s->last_insert_ID();
  }
  return lastInsertID;
}

long long mysqlinsert64_(long long *server,char *table_name,char *values,long long *vlen,char *duplicateKeyCondition)
{
  int srv=*server,vl=*vlen;
  long long istat=mysqlinsert_(&srv,table_name,values,&vl,duplicateKeyCondition);
  return istat;
}

void mysqlservererror_(int *server)
{
  auto s=reinterpret_cast<MySQL::Server *>(*server);
  std::cerr << s->error() << std::endl;
}

void mysqlservererror64_(long long *server)
{
  int srv=*server;
  mysqlservererror_(&srv);
}

}
