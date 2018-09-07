#include <list>
#include <string>
#include <sstream>
#include <unordered_set>
#include <regex>
#include <sys/stat.h>
#include <metadata.hpp>
#include <datetime.hpp>
#include <MySQL.hpp>
#include <bsort.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bitmap.hpp>
#include <search.hpp>

namespace summarizeMetadata {

namespace gridLevels {

void summarize_grid_levels(std::string database,std::string caller,std::string user)
{
  std::string dsnum2=strutils::substitute(metautils::args.dsnum,".","");
  std::string filetable,file_ID;
  if (database == "GrML") {
    filetable="_primaries";
    file_ID="mssID";
  }
  else if (database == "WGrML") {
    filetable="_webfiles";
    file_ID="webID";
  }
  MySQL::Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"");
  MySQL::LocalQuery query("select distinct p.format_code,g.levelType_codes from "+database+".ds"+dsnum2+"_agrids as g left join "+database+".ds"+dsnum2+filetable+" as p on p.code = g."+file_ID+"_code where !isnull(p.format_code)");
  if (query.submit(server) < 0) {
    metautils::log_error("summarize_grid_levels(): '"+query.error()+"' for '"+query.show()+"'",caller,user);
  }
  my::map<gridLevels::LevelEntry> level_values_table;
  MySQL::Row row;
  while (query.fetch_row(row)) {
    std::vector<size_t> level_values;
    bitmap::uncompress_values(row[1],level_values);
    for (const auto& lval : level_values) {
	gridLevels::LevelEntry le;
	le.key=row[0]+","+strutils::itos(lval);
	if (!level_values_table.found(le.key,le)) {
	  level_values_table.insert(le);
	}
    }
  }
  std::string error;
  if (server.command("lock table "+database+".ds"+dsnum2+"_levels write",error) < 0)
    metautils::log_error("summarize_grid_levels(): "+server.error()+" while locking table "+database+".ds"+dsnum2+"_levels",caller,user);
  server._delete(database+".ds"+dsnum2+"_levels");
  for (const auto& key : level_values_table.keys()) {
    if (server.insert(database+".ds"+dsnum2+"_levels",key) < 0)
	metautils::log_error("summarize_grid_levels(): "+server.error()+" while inserting '"+key+"' into "+database+".ds"+dsnum2+"_levels",caller,user);
  }
  if (server.command("unlock tables",error) < 0)
    metautils::log_error("summarize_grid_levels(): "+server.error()+" while unlocking tables",caller,user);
  server.disconnect();
}

} // end namespace gridLevels

namespace grids {

bool compare_values(const size_t& left,const size_t& right)
{
  if (left <= right) {
    return true;
  }
  else {
    return false;
  }
}

void summarize_grids(std::string database,std::string caller,std::string user,std::string fileID_code)
{
  std::string dsnum2=strutils::substitute(metautils::args.dsnum,".","");
  std::string bitmap,filetable,fileID;
  if (database == "GrML") {
    filetable="_primaries";
    fileID="mssID";
  }
  else if (database == "WGrML") {
    filetable="_webfiles";
    fileID="webID";
  }
  MySQL::Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"");
  MySQL::LocalQuery query;
  if (!fileID_code.empty()) {
    query.set("select f.code,g.timeRange_code,g.gridDefinition_code,g.parameter,g.levelType_codes,g.start_date,g.end_date from "+database+".ds"+dsnum2+"_grids as g left join "+database+".ds"+dsnum2+filetable+" as p on p.code = g."+fileID+"_code left join "+database+".formats as f on f.code = p.format_code where p.code = "+fileID_code);
  }
  else {
    query.set("select distinct f.code,a.timeRange_codes,a.gridDefinition_codes,a.parameter,a.levelType_codes,min(a.start_date),max(a.end_date) from "+database+".ds"+dsnum2+"_agrids as a left join "+database+".ds"+dsnum2+filetable+" as p on p.code = a."+fileID+"_code left join "+database+".formats as f on f.code = p.format_code where !isnull(f.code) group by f.code,a.timeRange_codes,a.gridDefinition_codes,a.parameter,a.levelType_codes");
  }
  if (query.submit(server) < 0) {
    metautils::log_error("summarize_grids(): "+query.error()+" for query '"+query.show()+"'",caller,user);
  }
  std::vector<size_t> time_range_values,grid_definition_values;
  my::map<grids::SummaryEntry> summary_table(99999);
  MySQL::Row row;
  while (query.fetch_row(row)) {
    if (!fileID_code.empty()) {
	time_range_values.clear();
	time_range_values.emplace_back(std::stoi(row[1]));
	grid_definition_values.clear();
	grid_definition_values.emplace_back(std::stoi(row[2]));
    }
    else {
	bitmap::uncompress_values(row[1],time_range_values);
	bitmap::uncompress_values(row[2],grid_definition_values);
    }
    std::vector<size_t> level_values;
    bitmap::uncompress_values(row[4],level_values);
    for (const auto& time_range_value : time_range_values) {
	for (const auto& grid_definition_value : grid_definition_values) {
	  grids::SummaryEntry se;
	  se.key=row[0]+","+strutils::itos(time_range_value)+","+strutils::itos(grid_definition_value)+",'"+row[3]+"'";
	  if (!summary_table.found(se.key,se)) {
	    se.data.reset(new grids::SummaryEntry::Data);
	    se.data->start=std::stoll(row[5]);
	    se.data->end=std::stoll(row[6]);
	    for (const auto& level_value : level_values) {
		grids::LevelCodeEntry le;
		le.key=level_value;
		se.data->level_code_table.insert(le);
	    }
	    summary_table.insert(se);
	  }
	  else {
	    auto date=std::stoll(row[5]);
	    if (date < se.data->start) {
		se.data->start=date;
	    }
	    date=std::stoll(row[6]);
	    if (date > se.data->end) {
		se.data->end=date;
	    }
	    for (const auto& level_value : level_values) {
		grids::LevelCodeEntry le;
		if (!se.data->level_code_table.found(level_value,le)) {
		  grids::LevelCodeEntry le;
		  le.key=level_value;
		  se.data->level_code_table.insert(le);
		}
	    }
	  }
	}
    }
  }
  std::string error;
  if (server.command("lock table "+database+".summary write",error) < 0) {
    metautils::log_error("summarize_grids(): "+server.error(),caller,user);
  }
  if (fileID_code.empty()) {
    server._delete(database+".summary","dsid = '"+metautils::args.dsnum+"'");
  }
  std::vector<size_t> array;
  for (const auto& key : summary_table.keys()) {
    grids::SummaryEntry se;
    summary_table.found(key,se);
    array.clear();
    array.reserve(se.data->level_code_table.size());
    for (const auto& key2 : se.data->level_code_table.keys()) {
	array.emplace_back(key2);
    }
    binary_sort(array,compare_values);
    bitmap::compress_values(array,bitmap);
    auto start=strutils::lltos(se.data->start);
    auto end=strutils::lltos(se.data->end);
    if (server.insert(database+".summary","'"+metautils::args.dsnum+"',"+se.key+",'"+bitmap+"',"+start+","+end) < 0) {
	if (strutils::contains(server.error(),"Duplicate entry")) {
	  auto parts=strutils::split(se.key,",");
	  MySQL::LocalQuery q("levelType_codes",database+".summary","dsid = '"+metautils::args.dsnum+"' and format_code = "+parts[0]+" and timeRange_code = "+parts[1]+" and gridDefinition_code = "+parts[2]+" and parameter = "+parts[3]);
	  if (q.submit(server) < 0) {
	    metautils::log_error("summarize_grids(): "+q.error(),caller,user);
	  }
	  q.fetch_row(row);
	  std::vector<size_t> level_values;
	  bitmap::uncompress_values(row[0],level_values);
	  for (const auto& level_value : level_values) {
	    grids::LevelCodeEntry le;
	    if (!se.data->level_code_table.found(level_value,le)) {
		le.key=level_value;
		se.data->level_code_table.insert(le);
	    }
	  }
	  array.clear();
	  array.reserve(se.data->level_code_table.size());
	  for (const auto& key2 : se.data->level_code_table.keys()) {
	    array.emplace_back(key2);
	  }
	  binary_sort(array,compare_values);
	  bitmap::compress_values(array,bitmap);
	  if (server.insert(database+".summary","'"+metautils::args.dsnum+"',"+se.key+",'"+bitmap+"',"+start+","+end,"update levelType_codes = '"+bitmap+"', start_date = if ("+start+" < start_date,"+start+",start_date), end_date = if ("+end+" > end_date,"+end+",end_date)") < 0) {
	    metautils::log_error("summarize_grids(): "+server.error()+" while trying to insert ('"+metautils::args.dsnum+"',"+se.key+",'"+bitmap+"',"+start+","+end+")",caller,user);
	  }
	}
	else {
	  metautils::log_error("summarize_grids(): "+error+" while trying to insert ('"+metautils::args.dsnum+"',"+se.key+",'"+bitmap+"',"+start+","+end+")",caller,user);
	}
    }
    se.data->level_code_table.clear();
    se.data.reset();
  }
  if (server.command("unlock tables",error) < 0) {
    metautils::log_error("summarize_grids(): "+server.error(),caller,user);
  }
  server.disconnect();
}

void summarize_grid_resolutions(std::string caller,std::string user,std::string mssID_code)
{
  MySQL::Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"");
  MySQL::LocalQuery query("code,definition,defParams","GrML.gridDefinitions");
  if (query.submit(server) < 0) {
    metautils::log_error("summarize_grid_resolutions(): "+query.error(),caller,user);
  }
  my::map<GridDefinitionEntry> grid_definition_table;
  MySQL::Row row;
  while (query.fetch_row(row)) {
    GridDefinitionEntry gde;
    gde.key=row[0];
    gde.definition=row[1];
    gde.def_params=row[2];
    grid_definition_table.insert(gde);
  }
  if (!mssID_code.empty())
    query.set("select distinct gridDefinition_codes from GrML.ds"+strutils::substitute(metautils::args.dsnum,".","")+"_agrids where mssID_code = "+mssID_code);
  else {
    server._delete("search.grid_resolutions","dsid = '"+metautils::args.dsnum+"'");
    query.set("select distinct gridDefinition_codes from GrML.ds"+strutils::substitute(metautils::args.dsnum,".","")+"_agrids");
  }
  if (query.submit(server) < 0) {
    metautils::log_error("summarize_grid_resolutions() REturned error: "+query.error(),caller,user);
  }
  while (query.fetch_row(row)) {
    std::vector<size_t> grid_definition_codes;
    bitmap::uncompress_values(row[0],grid_definition_codes);
    for (const auto& grid_definition_code : grid_definition_codes) {
	GridDefinitionEntry gde;
	grid_definition_table.found(strutils::itos(grid_definition_code),gde);
	short res_type=0;
	double xres=0.,yres=0.,lat1,lat2;
	auto params=strutils::split(gde.def_params,":");
	if (gde.definition == "gaussLatLon") {
	  xres=std::stof(params[6]);
	  lat1=std::stof(params[2]);
	  lat2=std::stof(params[4]);
	  yres=std::stof(params[1]);
	  yres=fabs(lat2-lat1)/yres;
	  res_type=0;
	}
	else if (gde.definition == "lambertConformal" || gde.definition == "polarStereographic") {
	  xres=std::stof(params[7]);
	  yres=std::stof(params[8]);
	  res_type=1;
	}
	else if (std::regex_search(gde.definition,std::regex("^(latLon|mercator)(Cell){0,1}$"))) {
	  xres=std::stof(params[6]);
	  yres=std::stof(params[7]);
	  if (std::regex_search(gde.definition,std::regex("^latLon"))) {
	    res_type=0;
	  }
	  else if (std::regex_search(gde.definition,std::regex("^mercator"))) {
	    res_type=1;
	  }
	}
	else if (gde.definition == "sphericalHarmonics") {
	  res_type=0;
	  auto trunc=params[0];
	  if (trunc == "42") {
	    xres=yres=2.8;
	  }
	  else if (trunc == "63") {
	    xres=yres=1.875;
	  }
	  else if (trunc == "85") {
	    xres=yres=1.4;
	  }
	  else if (trunc == "106") {
	    xres=yres=1.125;
	  }
	  else if (trunc == "159") {
	    xres=yres=0.75;
	  }
	  else if (trunc == "799") {
	    xres=yres=0.225;
	  }
	  else if (trunc == "1279") {
	    xres=yres=0.125;
	  }
	}
	if (floatutils::myequalf(xres,0.) && floatutils::myequalf(yres,0.)) {
	  metautils::log_error("summarize_grid_resolutions(): unknown grid definition '"+gde.definition+"'",caller,user);
	}
	if (yres > xres) {
	  xres=yres;
	}
	auto hres_keyword=searchutils::horizontal_resolution_keyword(xres,res_type);
	if (!hres_keyword.empty()) {
	  if (server.insert("search.grid_resolutions","'"+hres_keyword+"','GCMD','"+metautils::args.dsnum+"','GrML'") < 0) {
	    if (!strutils::contains(server.error(),"Duplicate entry")) {
		metautils::log_error("summarize_grid_resolutions(): "+server.error(),caller,user);
	    }
	  }
	}
	else {
	  metautils::log_warning("summarize_grid_resolutions() issued warning: no grid resolution for "+gde.definition+", "+gde.def_params,caller,user);
	}
    }
  }
  server.disconnect();
}

void aggregate_grids(std::string database,std::string caller,std::string user,std::string fileID_code)
{
  std::string dsnum2=strutils::substitute(metautils::args.dsnum,".","");
  MySQL::Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"");
  MySQL::LocalQuery query;
  if (database == "GrML") {
    if (!fileID_code.empty()) {
	server._delete("GrML.ds"+dsnum2+"_agrids","mssID_code = "+fileID_code);
	server._delete("GrML.ds"+dsnum2+"_agrids2","mssID_code = "+fileID_code);
	server._delete("GrML.ds"+dsnum2+"_grid_definitions","mssID_code = "+fileID_code);
	query.set("select timeRange_code,gridDefinition_code,parameter,levelType_codes,min(start_date),max(end_date) from GrML.ds"+dsnum2+"_grids where mssID_code = "+fileID_code+" group by timeRange_code,gridDefinition_code,parameter,levelType_codes order by parameter,levelType_codes,timeRange_code");
    }
    else {
	server._delete("GrML.ds"+dsnum2+"_agrids_cache");
	query.set("select timeRange_code,gridDefinition_code,parameter,levelType_codes,min(start_date),max(end_date) from GrML.summary where dsid = '"+metautils::args.dsnum+"' group by timeRange_code,gridDefinition_code,parameter,levelType_codes order by parameter,levelType_codes,timeRange_code");
    }
  }
  else if (database == "WGrML") {
    if (!fileID_code.empty()) {
	server._delete("WGrML.ds"+dsnum2+"_agrids","webID_code = "+fileID_code);
	server._delete("WGrML.ds"+dsnum2+"_agrids2","webID_code = "+fileID_code);
	server._delete("WGrML.ds"+dsnum2+"_grid_definitions","webID_code = "+fileID_code);
	query.set("select timeRange_code,gridDefinition_code,parameter,levelType_codes,min(start_date),max(end_date) from WGrML.ds"+dsnum2+"_grids where webID_code = "+fileID_code+" group by timeRange_code,gridDefinition_code,parameter,levelType_codes order by parameter,levelType_codes,timeRange_code");
    }
    else {
	server._delete("WGrML.ds"+dsnum2+"_agrids_cache");
	query.set("select timeRange_code,gridDefinition_code,parameter,levelType_codes,min(start_date),max(end_date) from WGrML.summary where dsid = '"+metautils::args.dsnum+"' group by timeRange_code,gridDefinition_code,parameter,levelType_codes order by parameter,levelType_codes,timeRange_code");
    }
  }
  if (query.submit(server) < 0) {
    metautils::log_error("aggregate_grids(): "+query.error(),caller,user);
  }
//std::cerr << query.show() << std::endl;
  my::map<metautils::SizeTEntry> grid_definition_code_table;
  std::vector<size_t> ds_grid_definition_codes;
  std::unordered_set<size_t> ds_grid_definition_set;
  std::string start,end;
  std::vector<size_t> time_range_codes;
  std::string last_key;
  MySQL::Row row;
  while (query.fetch_row(row)) {
    std::string this_key=row[2]+"','"+row[3];
    if (this_key != last_key) {
	if (!last_key.empty()) {
	  if (!fileID_code.empty()) {
	    std::string tr_bitmap;
	    bitmap::compress_values(time_range_codes,tr_bitmap);
	    if (tr_bitmap.length() > 255) {
		metautils::log_error("aggregate_grids(): bitmap for time ranges is too long - fileID_code: "+fileID_code+" key: \""+last_key+"\" bitmap: '"+tr_bitmap+"'",caller,user);
	    }
	    std::vector<size_t> grid_definition_codes;
	    grid_definition_codes.reserve(grid_definition_code_table.size());
	    for (const auto& key : grid_definition_code_table.keys()) {
		grid_definition_codes.emplace_back(key);
		if (ds_grid_definition_set.find(key) == ds_grid_definition_set.end()) {
		  ds_grid_definition_codes.emplace_back(key);
		  ds_grid_definition_set.emplace(key);
		}
	    }
	    binary_sort(grid_definition_codes,compare_values);
	    std::string grid_definition_bitmap;
	    bitmap::compress_values(grid_definition_codes,grid_definition_bitmap);
	    if (grid_definition_bitmap.length() > 255) {
		metautils::log_error("aggregate_grids(): bitmap for grid definitions is too long",caller,user);
	    }
	    if (server.insert(database+".ds"+dsnum2+"_agrids",fileID_code+",'"+tr_bitmap+"','"+grid_definition_bitmap+"','"+last_key+"',"+start+","+end) < 0) {
		metautils::log_error("aggregate_grids(): "+server.error()+" while trying to insert '"+fileID_code+",'"+tr_bitmap+"','"+grid_definition_bitmap+"','"+last_key+"',"+start+","+end+"'",caller,user);
	    }
	    std::stringstream ss;
	    ss.str("");
	    ss << fileID_code << "," << time_range_codes.front() << "," << time_range_codes.back() << ",'";
	    if (time_range_codes.size() <= 2) {
		ss << "!" << time_range_codes.front();
		if (time_range_codes.size() == 2) {
		  ss << "," << time_range_codes.back();
		}
	    }
	    else {
		ss << tr_bitmap;
	    }
	    ss << "'," << grid_definition_codes.front() << "," << grid_definition_codes.back() << ",'";
	    if (grid_definition_codes.size() <= 2) {
		ss << "!" << grid_definition_codes.front();
		if (grid_definition_codes.size() == 2) {
		  ss << "," << grid_definition_codes.back();
		}
	    }
	    else {
		ss << grid_definition_bitmap;
	    }
	    std::deque<std::string> sp=strutils::split(last_key,"','");
	    std::vector<size_t> l_values;
	    bitmap::uncompress_values(sp[1],l_values);
	    ss << "','" << sp[0] << "'," << l_values.front() << "," << l_values.back() << ",'";
	    if (l_values.size() <= 2) {
		ss << "!" << l_values.front();
		if (l_values.size() == 2) {
		  ss << "," << l_values.back();
		}
	    }
	    else {
		ss << sp[1];
	    }
	    ss << "'," << start << "," << end;
	    if (server.insert(database+".ds"+dsnum2+"_agrids2",ss.str()) < 0) {
		metautils::log_error("aggregate_grids(): "+server.error()+" while trying to insert '"+ss.str()+"' into "+database+".ds"+dsnum2+"_agrids2",caller,user);
	    }
	  }
	  if (server.insert(database+".ds"+dsnum2+"_agrids_cache","parameter,levelType_codes,min_start_date,max_end_date","'"+last_key+"',"+start+","+end,"update min_start_date=if("+start+" < min_start_date,"+start+",min_start_date), max_end_date = if("+end+" > max_end_date,"+end+",max_end_date)") < 0) {
	    metautils::log_error("aggregate_grids(): "+server.error()+" while trying to insert ('"+last_key+"',"+start+","+end+")",caller,user);
	  }
	}
	time_range_codes.clear();
	grid_definition_code_table.clear();
	start="999999999999";
	end="000000000000";
    }
    last_key=this_key;
    time_range_codes.emplace_back(std::stoi(row[0]));
    metautils::SizeTEntry se;
    se.key=std::stoi(row[1]);
    if (!grid_definition_code_table.found(se.key,se)) {
	grid_definition_code_table.insert(se);
    }
    if (row[4] < start) {
	start=row[4];
    }
    if (row[5] > end) {
	end=row[5];
    }
  }
  if (!start.empty()) {
    if (!fileID_code.empty()) {
	std::string tr_bitmap;
	bitmap::compress_values(time_range_codes,tr_bitmap);
	if (tr_bitmap.length() > 255) {
	  metautils::log_error("aggregate_grids(): bitmap for time ranges is too long - fileID_code: "+fileID_code+" key: \""+last_key+"\" bitmap: '"+tr_bitmap+"'",caller,user);
	}
	std::vector<size_t> grid_definition_codes;
	grid_definition_codes.reserve(grid_definition_code_table.size());
	for (const auto& key : grid_definition_code_table.keys()) {
	  grid_definition_codes.emplace_back(key);
	  if (ds_grid_definition_set.find(key) == ds_grid_definition_set.end()) {
	    ds_grid_definition_codes.emplace_back(key);
	    ds_grid_definition_set.emplace(key);
	  }
	}
	binary_sort(grid_definition_codes,compare_values);
	std::string grid_definition_bitmap;
	bitmap::compress_values(grid_definition_codes,grid_definition_bitmap);
	if (grid_definition_bitmap.length() > 255) {
	  metautils::log_error("aggregate_grids(): bitmap for grid definitions is too long",caller,user);
	}
	if (!fileID_code.empty() && server.insert(database+".ds"+dsnum2+"_agrids",fileID_code+",'"+tr_bitmap+"','"+grid_definition_bitmap+"','"+last_key+"',"+start+","+end) < 0) {
	  metautils::log_error("aggregate_grids(): "+server.error()+" while trying to insert '"+fileID_code+",'"+tr_bitmap+"','"+grid_definition_bitmap+"','"+last_key+"',"+start+","+end,caller,user);
	}
	std::stringstream ss;
	ss.str("");
	ss << fileID_code << "," << time_range_codes.front() << "," << time_range_codes.back() << ",'";
	if (time_range_codes.size() <= 2) {
	  ss << "!" << time_range_codes.front();
	  if (time_range_codes.size() == 2) {
	    ss << "," << time_range_codes.back();
	  }
	}
	else {
	  ss << tr_bitmap;
	}
	ss << "'," << grid_definition_codes.front() << "," << grid_definition_codes.back() << ",'";
	if (grid_definition_codes.size() <= 2) {
	  ss << "!" << grid_definition_codes.front();
	  if (grid_definition_codes.size() == 2) {
	    ss << "," << grid_definition_codes.back();
	  }
	}
	else {
	  ss << grid_definition_bitmap;
	}
	std::deque<std::string> sp=strutils::split(last_key,"','");
	std::vector<size_t> l_values;
	bitmap::uncompress_values(sp[1],l_values);
	ss << "','" << sp[0] << "'," << l_values.front() << "," << l_values.back() << ",'";
	if (l_values.size() <= 2) {
	  ss << "!" << l_values.front();
	  if (l_values.size() == 2) {
	    ss << "," << l_values.back();
	  }
	}
	else {
	  ss << sp[1];
	}
	ss << "'," << start << "," << end;
	if (server.insert(database+".ds"+dsnum2+"_agrids2",ss.str()) < 0) {
	  metautils::log_error("aggregate_grids(): "+server.error()+" while trying to insert '"+ss.str()+"' into "+database+".ds"+dsnum2+"_agrids2",caller,user);
	}
    }
    if (server.insert(database+".ds"+dsnum2+"_agrids_cache","parameter,levelType_codes,min_start_date,max_end_date","'"+last_key+"',"+start+","+end,"update min_start_date=if("+start+" < min_start_date,"+start+",min_start_date), max_end_date = if("+end+" > max_end_date,"+end+",max_end_date)") < 0) {
	metautils::log_error("aggregate_grids(): "+server.error()+" while trying to insert ('"+last_key+"',"+start+","+end+")",caller,user);
    }
    binary_sort(ds_grid_definition_codes,compare_values);
    std::string ds_grid_definition_bitmap;
    bitmap::compress_values(ds_grid_definition_codes,ds_grid_definition_bitmap);
    if (!fileID_code.empty() && server.insert(database+".ds"+dsnum2+"_grid_definitions",fileID_code+",'"+ds_grid_definition_bitmap+"'") < 0) {
	if (!std::regex_search(server.error(),std::regex("^Duplicate entry"))) {
	  metautils::log_error("aggregate_grids(): "+server.error()+" while trying to insert ("+fileID_code+",'"+ds_grid_definition_bitmap+"')",caller,user);
	}
    }
  }
  server.disconnect();
}

} // end namespace grids

} // end namespace summarizeMetadata
