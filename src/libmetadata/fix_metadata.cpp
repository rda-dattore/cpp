#include <string>
#include <sys/stat.h>
#include <metadata.hpp>
#include <datetime.hpp>
#include <MySQL.hpp>
#include <bitmap.hpp>
#include <strutils.hpp>
#include <search.hpp>

namespace summarizeMetadata {

namespace fixData {

void summarize_fix_data(std::string caller,std::string user)
{
  std::string dsnum2=strutils::substitute(meta_args.dsnum,".","");
  std::string error;
  my::map<Entry> mssTable;
  Entry e;
  my::map<obsData::SummaryEntry> summaryTable;
  std::list<std::string> summaryTableKeys;
  obsData::SummaryEntry se;

  MySQL::Server server(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  MySQL::Query query("code,format_code","FixML.ds"+dsnum2+"_primaries");
  if (query.submit(server) < 0) {
    metautils::log_error("summarize_fix_data(): "+query.error(),caller,user);
  }
  MySQL::Row row;
  while (query.fetch_row(row)) {
    e.key=row[0];
    e.code=row[1];
    mssTable.insert(e);
  }
  if (server.command("lock tables FixML.ds"+dsnum2+"_locations write",error) < 0)
    metautils::log_error("summarize_fix_data(): "+server.error(),caller,user);
  query.set("mssID_code,classification_code,start_date,end_date,box1d_row,box1d_bitmap","FixML.ds"+dsnum2+"_locations");
  if (query.submit(server) < 0)
    metautils::log_error("summarize_fix_data(): "+query.error(),caller,user);
  while (query.fetch_row(row)) {
    mssTable.found(row[0],e);
    se.key=e.code+"<!>"+row[1]+"<!>"+row[4];
    if (!summaryTable.found(se.key,se)) {
	se.start_date=row[2];
	se.end_date=row[3];
	se.box1d_bitmap=row[5];
	summaryTable.insert(se);
	summaryTableKeys.push_back(se.key);
    }
    else {
	if (row[2] < se.start_date) {
	  se.start_date=row[2];
	}
	if (row[3] > se.end_date) {
	  se.end_date=row[3];
	}
	if (row[5] != se.box1d_bitmap) {
	  se.box1d_bitmap=bitmap::add_box1d_bitmaps(se.box1d_bitmap,row[5]);
	}
	summaryTable.replace(se);
    }
  }
  if (server.command("unlock tables",error) < 0) {
    metautils::log_error("summarize_fix_data(): "+server.error(),caller,user);
  }
  if (server.command("lock tables search.fix_data write",error) < 0) {
    metautils::log_error("summarize_fix_data(): "+server.error(),caller,user);
  }
  server._delete("search.fix_data","dsid = '"+meta_args.dsnum+"'");
  for (const auto& key : summaryTableKeys) {
    auto parts=strutils::split(key,"<!>");
    summaryTable.found(key,se);
    if (server.insert("search.fix_data","'"+meta_args.dsnum+"',"+parts[0]+","+parts[1]+",'"+se.start_date+"','"+se.end_date+"',"+parts[2]+",'"+se.box1d_bitmap+"'") < 0) {
	error=server.error();
	if (!strutils::has_beginning(error,"Duplicate entry")) {
	  metautils::log_error("summarize_fix_data(): "+error,caller,user);
	}
    }
  }
  if (server.command("unlock tables",error) < 0) {
    metautils::log_error("summarize_fix_data(): "+server.error(),caller,user);
  }
  error=summarize_locations("FixML");
  if (!error.empty()) {
    metautils::log_error("summarize_locations() returned '"+error+"'",caller,user);
  }
  server.disconnect();
}

} // end namespace fixData

} // end namespace summarizeMetadata
