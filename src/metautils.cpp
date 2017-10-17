#include <fstream>
#include <string>
#include <sstream>
#include <regex>
#include <sys/stat.h>
#include <unistd.h>
#include <metadata.hpp>
#include <tempfile.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>
#include <grid.hpp>
#include <MySQL.hpp>

namespace metautils {

bool connect_to_metadata_server(MySQL::Server& srv_m)
{
  int num_tries=0;
  while (!srv_m && num_tries < 3) {
    if (strutils::has_beginning(directives.database_server,directives.host)) {
	srv_m.connect("","metadata","metadata","",5);
    }
    else {
	srv_m.connect(directives.database_server,"metadata","metadata","",5);
    }
    if (!srv_m) {
	sleep(5*pow(2.,num_tries));
    }
    ++num_tries;
  }
  if (!srv_m || !table_exists(srv_m,"search.datasets") ||  !table_exists(srv_m,"dssdb.dataset")) {
    if (strutils::has_beginning(directives.fallback_database_server,directives.host)) {
	srv_m.connect("","metadata","metadata","",5);
    }
    else {
	srv_m.connect(directives.fallback_database_server,"metadata","metadata","",5);
    }
    if (!srv_m) {
	return false;
    }
  }
  return true;
}

bool connectToBackupMetadataServer(MySQL::Server& srv_m)
{
  int num_tries=0;

  while (!srv_m && num_tries < 3) {
    if (strutils::has_beginning(directives.fallback_database_server,directives.host)) {
	srv_m.connect("","metadata","metadata","",5);
    }
    else {
	srv_m.connect(directives.fallback_database_server,"metadata","metadata","",5);
    }
    if (!srv_m) {
	sleep(5*pow(2.,num_tries));
    }
    ++num_tries;
  }
  if (!srv_m) {
    return false;
  }
  else {
    return true;
  }
}

bool connect_to_RDADB_server(MySQL::Server& srv_d)
{
  int num_tries=0;

  while (!srv_d && num_tries < 3) {
    if (strutils::has_beginning(directives.database_server,directives.host)) {
	srv_d.connect("","dssdb","dssdb","dssdb",5);
    }
    else {
	srv_d.connect(directives.database_server,"dssdb","dssdb","dssdb",5);
    }
    if (!srv_d) {
	sleep(5*pow(2.,num_tries));
    }
    ++num_tries;
  }
  if (!srv_d || !table_exists(srv_d,"dssdb.dataset")) {
    if (strutils::has_beginning(directives.fallback_database_server,directives.host)) {
	srv_d.connect("","dssdb","dssdb","dssdb",5);
    }
    else {
	srv_d.connect(directives.fallback_database_server,"dssdb","dssdb","dssdb",5);
    }
    if (!srv_d) {
	return false;
    }
  }
  return true;
}

bool connectToBackupRDAServer(MySQL::Server& srv_d)
{
  int num_tries=0;

  while (!srv_d && num_tries < 3) {
    if (strutils::has_beginning(directives.fallback_database_server,directives.host)) {
	srv_d.connect("","dssdb","dssdb","dssdb",5);
    }
    else {
	srv_d.connect(directives.fallback_database_server,"dssdb","dssdb","dssdb",5);
    }
    if (!srv_d) {
	sleep(5*pow(2.,num_tries));
    }
    ++num_tries;
  }
  if (!srv_d) {
    return false;
  }
  else {
    return true;
  }
}

void read_config(std::string caller,std::string user,std::string args_string,bool restrict_to_user_rdadata)
{
  directives.host=host_name();
  if (restrict_to_user_rdadata && geteuid() != 15968 && directives.host != "rda-web-prod.ucar.edu" && directives.host != "rda-web-dev.ucar.edu" && geteuid() != 8342) {
    std::cerr << "Error: " << geteuid() << " not authorized " << caller << std::endl;
    exit(1);
  }
  directives.server_root="/"+strutils::token(directives.host,".",0);
  struct stat buf;
  if (stat("/local/dss",&buf) == 0) {
    directives.dss_root="/local/dss";
  }
  else if (stat("/usr/local/dss",&buf) == 0) {
    directives.dss_root="/usr/local/dss";
  }
  else if (stat("/glade/u/home/rdadata",&buf) == 0) {
    directives.dss_root="/glade/u/home/rdadata";
  }
  if (directives.dss_root.length() == 0) {
    std::cerr << "Error locating DSS root directory on " << directives.host << std::endl;
    exit(1);
  }
  directives.dss_bindir=directives.dss_root+"/bin";
  if (std::regex_search(directives.dss_root,std::regex("^/glade")) && (std::regex_search(directives.host,std::regex("^cheyenne")) || std::regex_search(directives.host,std::regex("^r([0-9]){1,}i([0-9]){1,}n([0-9]){1,}$")))) {
    directives.dss_bindir+="/ch";
  }
  if (stat("/local/dss/bin/cmd_util",&buf) == 0) {
    directives.local_root="/local/dss";
  }
  else if (stat("/usr/local/dss/bin/cmd_util",&buf) == 0) {
    directives.local_root="/usr/local/dss";
  }
  else if (stat("/ncar/rda/setuid/bin/cmd_util",&buf) == 0) {
    directives.local_root="/ncar/rda/setuid";
  }
  if (directives.local_root.empty()) {
    std::cerr << "Error locating DSS local directory on " << directives.host << std::endl;
    exit(1);
  }
  std::ifstream ifs((directives.dss_root+"/bin/conf/cmd.conf").c_str());
  if (!ifs.is_open()) {
    std::cerr << "Error opening config file cmd.conf" << std::endl;
    exit(1);
  }
  char line[256];
  ifs.getline(line,256);
  while (!ifs.eof()) {
    if (line[0] != '#') {
	auto conf_parts=strutils::split(line);
	if (conf_parts[0] == "service") {
	  if (conf_parts.size() != 2)
	    log_error("configuration error on 'service' line",caller,user,args_string);
	  if (conf_parts[1] != "on")
	    log_error("service is currently unavailable",caller,user,args_string);
	}
	else if (conf_parts[0] == "webServer") {
	  if (conf_parts.size() != 2)
	    log_error("configuration error on 'webServer' line",caller,user,args_string);
	  directives.web_server=conf_parts[1];
	}
	else if (conf_parts[0] == "databaseServer") {
	  if (conf_parts.size() != 2)
	    log_error("configuration error on 'databaseServer' line",caller,user,args_string);
	  directives.database_server=conf_parts[1];
	}
	else if (conf_parts[0] == "fallbackDatabaseServer") {
	  if (conf_parts.size() != 2)
	    log_error("configuration error on 'fallbackDatabaseServer' line",caller,user,args_string);
	  directives.fallback_database_server=conf_parts[1];
	}
	else if (conf_parts[0] == "tempPath") {
	  if (conf_parts.size() != 2)
	    log_error("configuration error on 'tempPath' line",caller,user,args_string);
	  if (stat(conf_parts[1].c_str(),&buf) == 0)
	    directives.temp_path=conf_parts[1];
	}
	else if (conf_parts[0] == "dataRoot") {
	  if (conf_parts.size() != 3) {
	    log_error("configuration error on 'dataRoot' line",caller,user,args_string);
	  }
	  directives.data_root=conf_parts[1];
	  directives.data_root_alias=conf_parts[2];
	}
	else if (conf_parts[0] == "metadataManager") {
	  if (conf_parts.size() != 2) {
	    log_error("configuration error on 'metadataManager' line",caller,user,args_string);
	  }
	  directives.metadata_manager=conf_parts[1];
	}
    }
    ifs.getline(line,256);
  }
  ifs.close();
  if (directives.temp_path.empty()) {
    log_error("unable to set path for temporary files",caller,user,args_string);
  }
}

std::list<std::string> CMD_databases(std::string caller,std::string user,std::string args_string)
{
  MySQL::Server server;
  MySQL::Query query;
  std::list<std::string> databases;
  my::map<StringEntry> table;
  StringEntry se;
  MySQL::Row row;

  connect_to_metadata_server(server);
  query.set("show databases where `Database` like '%ML' and `Database` not like 'W%'");
  if (query.submit(server) < 0)
    log_error("CMD_databases() returned error: '"+query.error()+"'",caller,user,args_string);
  while (query.fetch_row(row)) {
    databases.push_back(row[0]);
    se.key=row[0];
    table.insert(se);
  }
  query.set("show databases where `Database` like 'W%ML'");
  if (query.submit(server) < 0)
    log_error("CMD_databases() returned error: '"+query.error()+"'",caller,user,args_string);
  while (query.fetch_row(row)) {
    se.key=row[0].substr(1);
    if (!table.found(se.key,se))
	databases.push_back(row[0]);
  }
  server.disconnect();
  return databases;
}

void check_for_existing_CMD(std::string cmd_type)
{
  MySQL::Server server;
  MySQL::LocalQuery query;
  MySQL::Row row;
  std::stringstream output,error;
  DateTime archive_date,metadata_date,base(1970,1,1,0,0);
  time_t tm=time(NULL);
  struct tm *t;
  std::string path=args.path,cmd_dir,flag;

  connect_to_metadata_server(server);
  if (strutils::has_beginning(args.path,"/FS/DSS") || strutils::has_beginning(args.path,"/DSS")) {
    query.set("date_modified,time_modified","dssdb.mssfile","dsid = 'ds"+args.dsnum+"' and mssfile = '"+args.path+"/"+args.filename+"'");
    cmd_dir="fmd";
  }
  else {
    strutils::replace_all(path,"https://rda.ucar.edu"+web_home()+"/","");
    query.set("date_modified,time_modified","dssdb.wfile","dsid = 'ds"+args.dsnum+"' and wfile = '"+path+"/"+args.filename+"'");
    cmd_dir="wfmd";
  }
  if (query.submit(server) < 0) {
    std::cerr << query.error() << std::endl;
    exit(1);
  }
  if (query.fetch_row(row)) {
    std::string cmd_file=path+"/"+args.filename;
    strutils::replace_all(cmd_file,"/FS/DSS/","");
    strutils::replace_all(cmd_file,"/DSS/","");
    strutils::replace_all(cmd_file,"/","%");
    if (exists_on_server("rda-web-prod.ucar.edu","/SERVER_ROOT/web/datasets/ds"+args.dsnum+"/metadata/"+cmd_dir+"/"+cmd_file+"."+cmd_type)) {
	archive_date.set(std::stoll(strutils::substitute(row[0],"-",""))*1000000+std::stoll(strutils::substitute(row[1],":","")));
	t=localtime(&tm);
	if (t->tm_isdst > 0) {
	  archive_date.add_hours(6);
	}
	else {
	  archive_date.add_hours(7);
	}
	struct stat buf;
	metadata_date=base.seconds_added(buf.st_mtime);
	if (metadata_date >= archive_date) {
	  if (strutils::has_beginning(args.path,"/FS/DSS") || strutils::has_beginning(args.path,"/DSS")) {
	    query.set("select mssID from "+cmd_type+".ds"+strutils::substitute(args.dsnum,".","")+"_primaries where mssID = '"+args.path+"/"+args.filename+"' and start_date != 0");
	  }
	  else {
	    query.set("select webID from W"+cmd_type+".ds"+strutils::substitute(args.dsnum,".","")+"_webfiles where webID = '"+path+"/"+args.filename+"' and start_date != 0");
	  }
	  if (query.submit(server) == 0 && query.num_rows() == 0) {
	    if (cmd_dir == "fmd") {
		flag="-f";
	    }
	    else if (cmd_dir == "wfmd") {
		flag="-wf";
	    }
	    mysystem2(directives.local_root+"/bin/scm -d "+args.dsnum+" "+flag+" "+cmd_file+"."+cmd_type,output,error);
	  }
	  else {
	    std::cerr << "Terminating: the file metadata for '" << cmd_file << "' are already up-to-date" << std::endl;
	  }
	  exit(1);
	}
    }
  }
  server.disconnect();
}

void cmd_register(std::string cmd,std::string user)
{
  MySQL::Server server_d;
  connect_to_RDADB_server(server_d);
  std::string file=args.path+"/"+args.filename;
  if (!args.member_name.empty()) {
    file+="..m.."+args.member_name;
  }
  MySQL::LocalQuery query("server","cmd_reg","cmd = '"+cmd+"' and file = '"+file+"'");
  if (query.submit(server_d) < 0) {
    log_error("cmd_register returned error: "+query.error(),cmd,user,args.args_string);
  }
  if (query.num_rows() > 0) {
    server_d._delete("cmd_reg","timestamp < date_sub(now(),interval 1 hour)");
    if (query.submit(server_d) < 0) {
	log_error("cmd_register returned error: "+query.error(),cmd,user,args.args_string);
    }
    if (query.num_rows() > 0) {
	MySQL::Row row;
	query.fetch_row(row);
	std::cerr << "Terminating:  there is a process currently running on '" << row[0] << "' to extract content metadata for '" << args.path << "/" << args.filename << "'" << std::endl;
	exit(1);
    }
  }
  args.reg_key=strutils::strand(15);
  if (server_d.insert("cmd_reg","regkey,server,cmd,file,timestamp","'"+args.reg_key+"','"+directives.host+"','"+cmd+"','"+file+"','"+current_date_time().to_string("%Y-%m-%d %H:%MM:%SS")+"'","") < 0) {
    log_error("error while registering: "+server_d.error(),cmd,user,args.args_string);
  }
  atexit(cmd_unregister);
  server_d.disconnect();
}

extern "C" void cmd_unregister()
{
  MySQL::Server server_d;

  connect_to_RDADB_server(server_d);
  if (args.reg_key.length() > 0)
    server_d._delete("cmd_reg","regkey = '"+args.reg_key+"'");
  server_d.disconnect();
}

void log_info(std::string message,std::string caller,std::string user,std::string args_string)
{
  std::ofstream log;
  std::stringstream output,error;

  log.open(("/glade/u/home/rdadata/logs/md/"+caller+"_log").c_str(),std::fstream::app);
  log << "user " << user << "/" << directives.host << " at " << current_date_time().to_string() << " running with '" << args_string << "':" << std::endl;
  log << message << std::endl;
  log.close();
  mysystem2("/bin/chmod 664 /glade/u/home/rdadata/logs/md/"+caller+"_log",output,error);
}

void log_error(std::string error,std::string caller,std::string user,std::string args_string,bool no_exit)
{
  size_t idx=0;

  if (strutils::has_beginning(error,"-q")) {
    idx=2;
  }
  if (getenv("QUERY_STRING") != NULL) {
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
    if (idx == 0) {
	std::cout << "Error: ";
    }
    std::cout << error.substr(idx) << std::endl;
  }
  else {
    if (idx == 0) {
	std::cerr << "Error: ";
    }
    std::cerr << error.substr(idx) << std::endl;
  }
  log_info(error.substr(idx),caller,user,args_string);
  if (!no_exit) {
    exit(1);
  }
}

void log_warning(std::string warning,std::string caller,std::string user,std::string args_string)
{
//  std::cerr << "Warning: " << warning << std::endl;
  log_info(warning,caller,user,args_string);
}

void obs_per(std::string observationTypeValue,size_t numObs,DateTime start,DateTime end,double& obsper,std::string& unit)
{
  size_t num_days[]={0,31,28,31,30,31,30,31,31,30,31,30,31};
  const float TOLERANCE=0.15;
  size_t idx;
  int nyrs,climo_len,nper;
  std::string sdum;

  if (strutils::contains(observationTypeValue,"climatology")) {
    if ( (idx=observationTypeValue.find("year")-1) != std::string::npos) {
	while (idx > 0 && observationTypeValue[idx] != '_')
	  idx--;
    }
    sdum=observationTypeValue.substr(idx+1);
    sdum=sdum.substr(0,sdum.find("-year"));
    climo_len=std::stoi(sdum);
    nyrs=end.year()-start.year();
    if (nyrs >= climo_len) {
	if (climo_len == 30) {
	  nper=((nyrs % climo_len)/10)+1;
	}
	else {
	  nper=nyrs/climo_len;
	}
	obsper=numObs/static_cast<float>(nper);
	if (static_cast<int>(numObs) == nper) {
	  unit="year";
	}
	else if (numObs/nper == 4) {
	  unit="season";
	}
	else if (numObs/nper == 12) {
	  unit="month";
	}
	else if (nper > 1 && start.month() != end.month()) {
	  nper=(nper-1)*12+12-start.month()+1;
	  if (static_cast<int>(numObs) == nper) {
	    unit="month";
	  }
	  else {
	    unit="";
	  }
	}
	else {
	  unit="";
	}
    }
    else {
	if ((end.month()-start.month()) == static_cast<int>(numObs)) {
	  unit="month";
	}
	else {
	  unit="";
	}
    }
    obsper=obsper-climo_len;
  }
  else {
    if (is_leap_year(end.year())) {
	num_days[2]=29;
    }
    obsper=numObs/static_cast<double>(end.seconds_since(start));
    if ((obsper+TOLERANCE) > 1.) {
	unit="second";
    }
    else {
	obsper*=60.;
	if ((obsper+TOLERANCE) > 1.) {
	  unit="minute";
	}
	else {
	  obsper*=60;
	  if ((obsper+TOLERANCE) > 1.) {
	    unit="hour";
	  }
	  else {
	    obsper*=24;
	    if ((obsper+TOLERANCE) > 1.) {
		unit="day";
	    }
	    else {
		obsper*=7;
		if ((obsper+TOLERANCE) > 1.) {
		  unit="week";
		}
		else {
		  obsper*=(num_days[end.month()]/7.);
		  if ((obsper+TOLERANCE) > 1.) {
		    unit="month";
		  }
		  else {
		    obsper*=12;
		    if ((obsper+TOLERANCE) > 1.) {
			unit="year";
		    }
		    else {
			unit="";
		    }
		  }
		}
	    }
	  }
	}
    }
    num_days[2]=28;
  }
}

std::string web_home()
{
  MySQL::Server server;
  connect_to_RDADB_server(server);
  MySQL::LocalQuery query;
  query.set("webhome","dataset","dsid = 'ds"+args.dsnum+"'");
  std::string webhome;
  if (query.submit(server) < 0) {
    webhome="";
  }
  MySQL::Row row;
  if (query.num_rows() > 0) {
    query.fetch_row(row);
    webhome=row[0];
  }
  else {
    webhome="";
  }
  server.disconnect();
  strutils::replace_all(webhome,"/glade/data02","");
  return webhome;
}

std::string relative_web_filename(std::string URL)
{
  strutils::replace_all(URL,"https://rda.ucar.edu","");
  strutils::replace_all(URL,"http://rda.ucar.edu","");
  strutils::replace_all(URL,"http://dss.ucar.edu","");
  if (std::regex_search(URL,std::regex("^/dsszone"))) {
    strutils::replace_all(URL,"/dsszone",directives.data_root_alias);
  }
  return strutils::substitute(URL,directives.data_root_alias+"/ds"+args.dsnum+"/","");
}

std::string clean_ID(std::string ID)
{
  strutils::trim(ID);
  for (size_t n=0; n < ID.length(); n++) {
    if (static_cast<int>(ID[n]) < 32 || static_cast<int>(ID[n]) > 127) {
	if (n > 0) {
	  ID=ID.substr(0,n)+"/"+ID.substr(n+1);
	}
	else {
	  ID="/"+ID.substr(1);
	}
    }
  }
  strutils::replace_all(ID,"\"","'");
  strutils::replace_all(ID,"&","&amp;");
  strutils::replace_all(ID,">","&gt;");
  strutils::replace_all(ID,"<","&lt;");
  return strutils::to_upper(ID);
}

namespace NcTime {

DateTime actual_date_time(double time,TimeData& time_data,std::string& error)
{
  DateTime date_time;
  if (time_data.units == "seconds") {
if (fabs(time-static_cast<int>(time)) > 0.01) {
error="can't compute dates from fractional seconds";
}
    date_time=time_data.reference.seconds_added(time,time_data.calendar);
  }
  else if (time_data.units == "minutes") {
if (fabs(time-static_cast<int>(time)) > 0.01) {
error="can't compute dates from fractional minutes";
}
    date_time=time_data.reference.minutes_added(time,time_data.calendar);
  }
  else if (time_data.units == "hours") {
if (fabs(time-static_cast<int>(time)) > 0.01) {
error="can't compute dates from fractional hours";
}
    date_time=time_data.reference.hours_added(time,time_data.calendar);
  }
  else if (time_data.units == "days") {
    date_time=time_data.reference.days_added(time,time_data.calendar);
    auto diff=fabs(time-static_cast<int>(time));
    if (diff > 0.01) {
	diff*=24.;
	if (fabs(diff-lround(diff)) > 0.01) {
	  diff*=60.;
	  if (fabs(diff-lround(diff)) > 0.01) {
	    diff*=60.;
	    if (fabs(diff-lround(diff)) > 0.01) {
		error="can't compute dates from fractional seconds in fractional days";
	    }
	    else {
		date_time.add_seconds(lround(diff),time_data.calendar);
	    }
	  }
	  else {
	    date_time.add_minutes(lround(diff),time_data.calendar);
	  }
	}
	else {
	  date_time.add_hours(lround(diff),time_data.calendar);
	}
    }
  }
  else if (time_data.units == "months") {
if (fabs(time-static_cast<int>(time)) > 0.01) {
error="can't compute dates from fractional months";
}
    date_time=time_data.reference.months_added(time);
  }
  else {
    error="don't understand time units in "+time_data.units;
  }
  return date_time;
}

std::string gridded_NetCDF_time_range_description(const TimeRangeEntry& tre,const TimeData& time_data,std::string time_method,std::string& error)
{
  std::string time_range;
  if (static_cast<int>(tre.key) < 0) {
    if (time_method.length() > 0) {
	auto lmethod=strutils::to_lower(time_method);
	if (strutils::contains(lmethod,"monthly")) {
	  time_range="Monthly ";
	}
	else if (time_data.units == "hours") {
	  time_range="Hourly ";
	}
	else if (time_data.units == "days") {
	  time_range="Daily ";
	}
	else if (time_data.units == "months") {
	  time_range="Monthly ";
	}
	else {
	  error="gridded_NetCDF_time_range_description(): don't understand time units '"+time_data.units+"' for cell method '"+time_method+"'";
	}
	time_range+=time_method;
    }
    else {
	if (time_data.units == "months") {
	  time_range="Monthly Mean";
	}
	else {
	  if (static_cast<int>(tre.key) == -11) {
	    time_range="Climate Model Simulation";
	  }
	  else {
	    if (static_cast<int>(tre.key) == -1) {
		time_range="Analysis";
	    }
	    else {
		time_range=strutils::itos(-static_cast<int>(tre.key)/100)+"-"+time_data.units;
		strutils::chop(time_range);
		time_range+=" Forecast";
	    }
	  }
	}
    }
  }
  else {
    if (tre.key == 0x7fffffff) {
	time_range="All-year Climatology of ";
    }
    else {
	time_range=strutils::itos(tre.key)+"-year Climatology of ";
    }
    auto idx=time_method.find(" over ");
    if (idx != std::string::npos) {
	time_method.insert(idx,"s");
    }
    else {
	time_method+="s";
    }
    switch (*tre.unit) {
	case 1:
	{
	  time_range+="Monthly ";
	  if (time_method.length() > 0) {
	    time_range+=time_method;
	  }
	  else {
	    time_range+="Means";
	  }
	  break;
	}
	case 2:
	{
	  time_range+="Seasonal ";
	  if (time_method.length() > 0) {
	    time_range+=time_method;
	  }
	  else {
	    time_range+="Means";
	  }
	  break;
	}
	case 3:
	{
	  time_range+="Annual ";
	  if (time_method.length() > 0) {
	    time_range+=time_method;
	  }
	  else {
	    time_range+="Means";
	  }
	  break;
	}
    }
  }
  return time_range;
}

std::string time_method_from_cell_methods(std::string cell_methods,std::string timeid)
{
  static my::map<StringEntry> *valid_cf_cell_methods_table=nullptr;
  StringEntry se;

  while (strutils::contains(cell_methods,"  ")) {
    strutils::replace_all(cell_methods,"  "," ");
  }
  strutils::replace_all(cell_methods,"comment: ","");
  strutils::replace_all(cell_methods,"comments: ","");
  strutils::replace_all(cell_methods,"comment:","");
  strutils::replace_all(cell_methods,"comments:","");
  if (cell_methods.length() > 0 && strutils::contains(cell_methods,timeid+":")) {
    auto idx=cell_methods.find(timeid+":");
    if (idx == std::string::npos) {
	return "";
    }
    if (idx != 0) {
	cell_methods=cell_methods.substr(idx);
    }
    strutils::replace_all(cell_methods,timeid+":","");
    strutils::trim(cell_methods);
    if ( (idx=cell_methods.find(":")) != std::string::npos) {
	auto idx2=cell_methods.find(")");
	if (idx2 == std::string::npos) {
// no extra information in parentheses
 	  cell_methods=cell_methods.substr(0,idx);
	  cell_methods=cell_methods.substr(0,cell_methods.rfind(" "));
	}
	else {
// found extra information so include that in the methods
	  cell_methods=cell_methods.substr(0,idx2+1);
	}
    }
    if (valid_cf_cell_methods_table == nullptr) {
	valid_cf_cell_methods_table=new my::map<StringEntry>;
	se.key="point";
	valid_cf_cell_methods_table->insert(se);
	se.key="sum";
	valid_cf_cell_methods_table->insert(se);
	se.key="maximum";
	valid_cf_cell_methods_table->insert(se);
	se.key="median";
	valid_cf_cell_methods_table->insert(se);
	se.key="mid_range";
	valid_cf_cell_methods_table->insert(se);
	se.key="minimum";
	valid_cf_cell_methods_table->insert(se);
	se.key="mean";
	valid_cf_cell_methods_table->insert(se);
	se.key="mode";
	valid_cf_cell_methods_table->insert(se);
	se.key="standard_deviation";
	valid_cf_cell_methods_table->insert(se);
	se.key="variance";
	valid_cf_cell_methods_table->insert(se);
    }
    if (!valid_cf_cell_methods_table->found(cell_methods,se)) {
	cell_methods="!"+cell_methods;
    }
    return cell_methods;
  }
  else {
    return "";
  }
}

} // end namespace NcTime

namespace NcLevel {

std::string write_level_map(std::string dsnum,const LevelInfo& level_info)
{
  std::string format;
  if (args.format == "hdf5nc4") {
    format="netCDF4";
  }
  else {
    format="netCDF";
  }
  LevelMap level_map;
  std::list<std::string> map_contents;
  std::string sdum=remote_web_file("https://rda.ucar.edu/metadata/LevelTables/"+format+".ds"+dsnum+".xml",directives.temp_path);
  if (level_map.fill(sdum)) {
    std::ifstream ifs(sdum.c_str());
    char line[32768];
    ifs.getline(line,32768);
    while (!ifs.eof()) {
	map_contents.emplace_back(line);
	ifs.getline(line,32768);
    }
    ifs.close();
    map_contents.pop_back();
  }
  TempDir tdir;
  if (!tdir.create(directives.temp_path)) {
    return "can't create temporary directory for writing netCDF levels";
  }
  std::ofstream ofs((tdir.name()+"/"+format+".ds"+dsnum+".xml").c_str());
  if (!ofs.is_open()) {
    return "can't open "+tdir.name()+"/"+format+".ds"+dsnum+".xml file for writing netCDF levels";
  }
  if (map_contents.size() > 0) {
    for (auto line : map_contents) {
	ofs << line << std::endl;
    }
  }
  else {
    ofs << "<?xml version=\"1.0\" ?>" << std::endl;
    ofs << "<levelMap>" << std::endl;
  }
  for (size_t n=0; n < level_info.write.size(); ++n) {
    if (level_info.write[n] == 1 && (map_contents.size() == 0 || (map_contents.size() > 0 && level_map.is_layer(level_info.ID[n]) < 0))) {
	ofs << "  <level code=\"" << level_info.ID[n] << "\">" << std::endl;
	ofs << "    <description>" << level_info.description[n] << "</description>" << std::endl;
	ofs << "    <units>" << level_info.units[n] << "</units>" << std::endl;
	ofs << "  </level>" << std::endl;
    }
  }
  ofs << "</levelMap>" << std::endl;
  ofs.close();
  std::string herror;
  if (host_sync(tdir.name(),".","/data/web/metadata/LevelTables",herror) < 0) {
    return herror;
  }
  std::stringstream output,error;
  mysystem2("/bin/cp "+tdir.name()+"/"+format+".ds"+dsnum+".xml /glade/u/home/rdadata/share/metadata/LevelTables/",output,error);
  return error.str();
}

} // end namespace NcLevel

namespace NcParameter {

std::string write_parameter_map(std::string dsnum,std::list<std::string>& varlist,my::map<metautils::StringEntry>& var_changes_table,std::string map_type,std::string map_name,bool found_map,std::string& warning)
{
  std::ifstream ifs;
  std::ofstream ofs;
  char line[32768];
  std::stringstream output,error;
  std::string format,herror;
  std::list<std::string> map_contents;
  std::deque<std::string> sp;
  metautils::StringEntry se;
  bool no_write;

  if (varlist.size() > 0) {
    if (found_map) {
	ifs.open(map_name.c_str());
	ifs.getline(line,32768);
	while (!ifs.eof()) {
	  map_contents.emplace_back(line);
	  ifs.getline(line,32768);
	}
	ifs.close();
	map_contents.pop_back();
    }
    TempDir tdir;
    if (!tdir.create(directives.temp_path)) {
	return "can't create temporary directory for parameter map";
    }
    ofs.open((tdir.name()+"/"+format+".ds"+dsnum+".xml").c_str());
    if (!ofs.is_open()) {
        return "can't open parameter map file for output";
    }
    if (!found_map) {
	ofs << "<?xml version=\"1.0\" ?>" << std::endl;
	ofs << "<" << map_type << "Map>" << std::endl;
    }
    else {
	no_write=false;
	for (const auto& line : map_contents) {
	  if (strutils::contains(line," code=\"")) {
	    sp=strutils::split(line,"\"");
	    se.key=sp[1];
	    if (var_changes_table.found(se.key,se)) {
		no_write=true;
	    }
	  }
	  if (!no_write) {
	    ofs << line << std::endl;
	  }
	  if (strutils::contains(line,"</"+map_type+">")) {
	    no_write=false;
	  }
	}
    }
    for (auto& item : varlist) {
	sp=strutils::split(item,"<!>");
	if (map_type == "parameter") {
	  ofs << "  <parameter code=\"" << sp[0] << "\">" << std::endl;
	  ofs << "    <shortName>" << sp[0] << "</shortName>" << std::endl;
	  if (sp[1].length() > 0) {
	    ofs << "    <description>" << sp[1] << "</description>" << std::endl;
	  }
	  if (sp[2].length() > 0) {
	    ofs << "    <units>" << strutils::substitute(sp[2],"-","^-") << "</units>" << std::endl;
	  }
	  if (sp.size() > 3 && sp[3].length() > 0) {
	    ofs << "    <standardName>" << sp[3] << "</standardName>" << std::endl;
	  }
	  ofs << "  </parameter>" << std::endl;
	}
	else if (map_type == "dataType") {
	  ofs << "  <dataType code=\"" << sp[0] << "\">" << std::endl;
	  ofs << "    <description>" << sp[1];
	  if (sp[2].length() > 0) {
	    ofs << " (" << sp[2] << ")";
	  }
	  ofs << "</description>" << std::endl;
	  ofs << "  </dataType>" << std::endl;
	}
    }
    ofs << "</" << map_type << "Map>" << std::endl;
    ofs.close();
    if (args.format == "hdf5nc4") {
	format="netCDF4";
    }
    else if (args.format == "hdf5") {
	format="HDF5";
    }
    else {
	format="netCDF";
    }
    if (host_sync(tdir.name(),".","/data/web/metadata/ParameterTables",herror) < 0) {
        warning="parameter map was not synced - error(s): '"+herror+"'";
    }
    mysystem2("/bin/cp "+tdir.name()+"/"+format+".ds"+dsnum+".xml /glade/u/home/rdadata/share/metadata/ParameterTables/",output,error);
  }
  return error.str();
}

} // end namespace NcParameter

namespace CF {

void fill_NC_time_data(std::string units_attribute_value,NcTime::TimeData& time_data,std::string user)
{
  if (std::regex_search(units_attribute_value,std::regex("since"))) {
    auto idx=units_attribute_value.find("since");
    time_data.units=strutils::to_lower(units_attribute_value.substr(0,idx));
    strutils::trim(time_data.units);
    units_attribute_value=units_attribute_value.substr(idx+5);
    while (!units_attribute_value.empty() && (units_attribute_value[0] < '0' || units_attribute_value[0] > '9')) {
	units_attribute_value=units_attribute_value.substr(1);
    }
    auto n=units_attribute_value.length()-1;
    while (n > 0 && (units_attribute_value[n] < '0' || units_attribute_value[n] > '9')) {
	--n;
    }
    ++n;
    if (n < units_attribute_value.length()) {
        units_attribute_value=units_attribute_value.substr(0,n);
    }
    strutils::trim(units_attribute_value);
    auto uparts=strutils::split(units_attribute_value);
    if (uparts.size() < 1 || uparts.size() > 3) {
	metautils::log_error("fill_NC_time_data() returned error: unable to get reference time from units specified as: '"+units_attribute_value+"'","nc2xml",user,args.args_string);
    }
    auto dparts=strutils::split(uparts[0],"-");
    if (dparts.size() != 3) {
        metautils::log_error("fill_NC_time_data() returned error: unable to get reference time from units specified as: '"+units_attribute_value+"'","nc2xml",user,args.args_string);
    }
    time_data.reference.set_year(std::stoi(dparts[0]));
    time_data.reference.set_month(std::stoi(dparts[1]));
    time_data.reference.set_day(std::stoi(dparts[2]));
    if (uparts.size() > 1) {
	auto tparts=strutils::split(uparts[1],":");
	switch (tparts.size()) {
	  case 1:
	  {
	    time_data.reference.set_time(std::stoi(tparts[0])*10000);
	    break;
	  }
	  case 2:
	  {
	    time_data.reference.set_time(std::stoi(tparts[0])*10000+std::stoi(tparts[1])*100);
	    break;
	  }
	  case 3:
	  {
	    time_data.reference.set_time(std::stoi(tparts[0])*10000+std::stoi(tparts[1])*100+static_cast<int>(std::stof(tparts[2])));
	    break;
	  }
	}
    }
  }
  else {
    metautils::log_error("fill_NC_time_data() returned error: unable to get CF time from time variable units","nc2xml",user,args.args_string);
  }
}

} // end namespace CF

} // end namespace metautils

namespace primaryMetadata {

bool check_file(std::string dirname,std::string filename,std::string *file_format)
{
  FILE *fp;
  const size_t BUF_LEN=80000;
  unsigned char buffer[BUF_LEN];
  size_t n,m;
  int chksum,sum,idum;

  if (!strutils::has_beginning(args.format,"grib")) {
// check to see if the file is a tar file
    fp=fopen64(filename.c_str(),"r");
    fread(buffer,1,512,fp);
    fclose(fp);
    m=0;
    for (n=148; n < 156; n++) {
	if (((reinterpret_cast<char *>(buffer))[n] >= '0' && (reinterpret_cast<char *>(buffer))[n] <= '9') || (reinterpret_cast<char *>(buffer))[n] == ' ')
	  m++;
	else
	  n=156;
    }
    strutils::strget(&(reinterpret_cast<char *>(buffer))[148],chksum,m);
    sum=256;
    for (n=0; n < 148; n++)
	sum+=static_cast<int>(buffer[n]);
    for (n=156; n < 512; n++)
	sum+=static_cast<int>(buffer[n]);
    if (sum == otoi(chksum)) {
	untar(dirname,filename);
	if (file_format != NULL) {
	  if (file_format->length() > 0)
	    (*file_format)+=".";
	  (*file_format)+="TAR";
	}
	return true;
    }
  }
// check to see if the file is gzipped
  fp=fopen64(filename.c_str(),"r");
  fread(buffer,1,4,fp);
  fclose(fp);
  get_bits(buffer,idum,0,32);
  if (idum == 0x1f8b0808) {
    system(("mv "+filename+" "+filename+".gz; gunzip -f "+filename).c_str());
    if (file_format != NULL) {
	if (file_format->length() > 0)
	  (*file_format)+=".";
	(*file_format)+="GZ";
    }
    return true;
  }
  return false;
}

void matchWebFileToMSSPrimary(std::string URL,std::string& metadata_file)
{
  MySQL::Server server;
  MySQL::Query query;
  MySQL::Row row;

  metautils::connect_to_RDADB_server(server);
  query.set("select m.mssfile from wfile as w left join mssfile as m on m.mssid = w.mssid where w.dsid = 'ds"+args.dsnum+"' and w.wfile = '"+metautils::relative_web_filename(URL)+"' and w.property = 'A' and w.type = 'D' and m.data_size = w.data_size");
  if (query.submit(server) < 0) {
    std::cerr << query.error() << std::endl;
    exit(1);
  }
  if (query.fetch_row(row) && row[0].length() > 0) {
    metadata_file=row[0];
    strutils::replace_all(metadata_file,"/FS/DSS/","");
    strutils::replace_all(metadata_file,"/DSS/","");
    strutils::replace_all(metadata_file,"/","%");
    if (exists_on_server("rda-web-prod.ucar.edu","/data/web/datasets/ds"+args.dsnum+"/metadata/fmd/"+metadata_file+".GrML")) {
	metadata_file+=".GrML";
    }
    else if (exists_on_server("rda-web-prod.ucar.edu","/SERVER_ROOT/web/datasets/ds"+args.dsnum+"/metadata/fmd/"+metadata_file+".ObML")) {
	metadata_file+=".ObML";
    }
    else if (exists_on_server("rda-web-prod.ucar.edu","/SERVER_ROOT/web/datasets/ds"+args.dsnum+"/metadata/fmd/"+metadata_file+".SatML")) {
	metadata_file+=".SatML";
    }
    else {
	metadata_file=row[0];
    }
  }
  else {
    metadata_file="";
  }
  server.disconnect();
}

} // end namespace primaryMetadata
