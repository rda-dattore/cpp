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

bool connectToMetadataServer(MySQL::Server& srv_m)
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

bool connectToRDAServer(MySQL::Server& srv_d)
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

void readConfig(std::string caller,std::string user,std::string argsString,bool restrict_to_user_rdadata)
{
  std::ifstream ifs;
  char line[256];
  std::deque<std::string> sp;
  struct stat buf;

  directives.host=getHostName();
/*
  if (directives.host.beginsWith("yslogin")) {
    std::cerr << "Error: can't execute on yellowstone login nodes" << std::endl;
    exit(1);
  }
*/
  if (restrict_to_user_rdadata && geteuid() != 15968 && directives.host != "rda-web-prod.ucar.edu" && directives.host != "rda-web-dev.ucar.edu" && geteuid() != 8342) {
    std::cerr << "Error: " << geteuid() << " not authorized " << caller << std::endl;
    exit(1);
  }
  directives.serverRoot="/"+strutils::token(directives.host,".",0);
  if (stat("/local/dss",&buf) == 0) {
    directives.dssRoot="/local/dss";
  }
  else if (stat("/usr/local/dss",&buf) == 0) {
    directives.dssRoot="/usr/local/dss";
  }
  else if (stat("/glade/u/home/rdadata",&buf) == 0) {
    directives.dssRoot="/glade/u/home/rdadata";
  }
  if (directives.dssRoot.length() == 0) {
    std::cerr << "Error locating DSS root directory on " << directives.host << std::endl;
    exit(1);
  }
  directives.dss_bindir=directives.dssRoot+"/bin";
  if (strutils::has_beginning(directives.dssRoot,"/glade") && (strutils::has_beginning(directives.host,"evans"))) {
    directives.dss_bindir+="/dsg_mach";
  }
  if (stat("/local/dss/bin/cmd_util",&buf) == 0) {
    directives.localRoot="/local/dss";
  }
  else if (stat("/usr/local/dss/bin/cmd_util",&buf) == 0) {
    directives.localRoot="/usr/local/dss";
  }
  else if (stat("/ncar/rda/setuid/bin/cmd_util",&buf) == 0) {
    directives.localRoot="/ncar/rda/setuid";
  }
  if (directives.localRoot.length() == 0) {
    std::cerr << "Error locating DSS local directory on " << directives.host << std::endl;
    exit(1);
  }
  ifs.open((directives.dssRoot+"/bin/conf/cmd.conf").c_str());
  if (!ifs.is_open()) {
    std::cerr << "Error opening config file cmd.conf" << std::endl;
    exit(1);
  }
  ifs.getline(line,256);
  while (!ifs.eof()) {
    if (line[0] != '#') {
	sp=strutils::split(line);
	if (sp[0] == "service") {
	  if (sp.size() != 2)
	    logError("configuration error on 'service' line",caller,user,argsString);
	  if (sp[1] != "on")
	    logError("service is currently unavailable",caller,user,argsString);
	}
	else if (sp[0] == "webServer") {
	  if (sp.size() != 2)
	    logError("configuration error on 'webServer' line",caller,user,argsString);
	  directives.web_server=sp[1];
	}
	else if (sp[0] == "databaseServer") {
	  if (sp.size() != 2)
	    logError("configuration error on 'databaseServer' line",caller,user,argsString);
	  directives.database_server=sp[1];
	}
	else if (sp[0] == "fallbackDatabaseServer") {
	  if (sp.size() != 2)
	    logError("configuration error on 'fallbackDatabaseServer' line",caller,user,argsString);
	  directives.fallback_database_server=sp[1];
	}
	else if (sp[0] == "tempPath") {
	  if (sp.size() != 2)
	    logError("configuration error on 'tempPath' line",caller,user,argsString);
	  if (stat(sp[1].c_str(),&buf) == 0)
	    directives.tempPath=sp[1];
	}
	else if (sp[0] == "dataRoot") {
	  if (sp.size() != 3) {
	    logError("configuration error on 'dataRoot' line",caller,user,argsString);
	  }
	  directives.data_root=sp[1];
	  directives.data_root_alias=sp[2];
	}
	else if (sp[0] == "metadataManager") {
	  if (sp.size() != 2) {
	    logError("configuration error on 'metadataManager' line",caller,user,argsString);
	  }
	  directives.metadata_manager=sp[1];
	}
    }
    ifs.getline(line,256);
  }
  ifs.close();
  if (directives.tempPath.length() == 0) {
    logError("unable to set path for temporary files",caller,user,argsString);
  }
}

std::list<std::string> getCMDDatabases(std::string caller,std::string user,std::string argsString)
{
  MySQL::Server server;
  MySQL::Query query;
  std::list<std::string> databases;
  my::map<StringEntry> table;
  StringEntry se;
  MySQL::Row row;

  connectToMetadataServer(server);
  query.set("show databases where `Database` like '%ML' and `Database` not like 'W%'");
  if (query.submit(server) < 0)
    logError("getCMDDatabases returned error: '"+query.error()+"'",caller,user,argsString);
  while (query.fetch_row(row)) {
    databases.push_back(row[0]);
    se.key=row[0];
    table.insert(se);
  }
  query.set("show databases where `Database` like 'W%ML'");
  if (query.submit(server) < 0)
    logError("getCMDDatabases returned error: '"+query.error()+"'",caller,user,argsString);
  while (query.fetch_row(row)) {
    se.key=row[0].substr(1);
    if (!table.found(se.key,se))
	databases.push_back(row[0]);
  }
  server.disconnect();
  return databases;
}

void checkForExistingCMD(std::string cmd_type)
{
  MySQL::Server server;
  MySQL::LocalQuery query;
  MySQL::Row row;
  std::stringstream output,error;
  DateTime archive_date,metadata_date,base(1970,1,1,0,0);
  struct stat buf;
  time_t tm=time(NULL);
  struct tm *t;
  std::string path=args.path,cmd_dir,flag;

  connectToMetadataServer(server);
  if (strutils::has_beginning(args.path,"/FS/DSS") || strutils::has_beginning(args.path,"/DSS")) {
    query.set("date_modified,time_modified","dssdb.mssfile","dsid = 'ds"+args.dsnum+"' and mssfile = '"+args.path+"/"+args.filename+"'");
    cmd_dir="fmd";
  }
  else {
    strutils::replace_all(path,"https://rda.ucar.edu"+getWebHome()+"/","");
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
    if (existsOnRDAWebServer("rda.ucar.edu","/SERVER_ROOT/web/datasets/ds"+args.dsnum+"/metadata/"+cmd_dir+"/"+cmd_file+"."+cmd_type,buf)) {
	archive_date.set(std::stoll(strutils::substitute(row[0],"-",""))*1000000+std::stoll(strutils::substitute(row[1],":","")));
	t=localtime(&tm);
	if (t->tm_isdst > 0) {
	  archive_date.addHours(6);
	}
	else {
	  archive_date.addHours(7);
	}
	metadata_date=base.secondsAdded(buf.st_mtime);
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
	    mysystem2(directives.localRoot+"/bin/scm -d "+args.dsnum+" "+flag+" "+cmd_file+"."+cmd_type,output,error);
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
  connectToRDAServer(server_d);
  std::string file=args.path+"/"+args.filename;
  if (!args.member_name.empty()) {
    file+="..m.."+args.member_name;
  }
  MySQL::LocalQuery query("server","cmd_reg","cmd = '"+cmd+"' and file = '"+file+"'");
  if (query.submit(server_d) < 0) {
    logError("cmd_register returned error: "+query.error(),cmd,user,args.argsString);
  }
  if (query.num_rows() > 0) {
    server_d._delete("cmd_reg","timestamp < date_sub(now(),interval 1 hour)");
    if (query.submit(server_d) < 0) {
	logError("cmd_register returned error: "+query.error(),cmd,user,args.argsString);
    }
    if (query.num_rows() > 0) {
	MySQL::Row row;
	query.fetch_row(row);
	std::cerr << "Terminating:  there is a process currently running on '" << row[0] << "' to extract content metadata for '" << args.path << "/" << args.filename << "'" << std::endl;
	exit(1);
    }
  }
  args.reg_key=strutils::strand(15);
  if (server_d.insert("cmd_reg","regkey,server,cmd,file,timestamp","'"+args.reg_key+"','"+directives.host+"','"+cmd+"','"+file+"','"+getCurrentDateTime().toString("%Y-%m-%d %H:%MM:%SS")+"'","") < 0) {
    logError("error while registering: "+server_d.error(),cmd,user,args.argsString);
  }
  atexit(cmd_unregister);
  server_d.disconnect();
}

extern "C" void cmd_unregister()
{
  MySQL::Server server_d;

  connectToRDAServer(server_d);
  if (args.reg_key.length() > 0)
    server_d._delete("cmd_reg","regkey = '"+args.reg_key+"'");
  server_d.disconnect();
}

void logInfo(std::string message,std::string caller,std::string user,std::string argsString)
{
  std::ofstream log;
  std::stringstream output,error;

  log.open(("/glade/u/home/rdadata/logs/md/"+caller+"_log").c_str(),std::fstream::app);
  log << "user " << user << "/" << directives.host << " at " << getCurrentDateTime().toString() << " running with '" << argsString << "':" << std::endl;
  log << message << std::endl;
  log.close();
  mysystem2("/bin/chmod 664 /glade/u/home/rdadata/logs/md/"+caller+"_log",output,error);
}

void logError(std::string error,std::string caller,std::string user,std::string argsString,bool no_exit)
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
  logInfo(error.substr(idx),caller,user,argsString);
  if (!no_exit) {
    exit(1);
  }
}

void logWarning(std::string warning,std::string caller,std::string user,std::string argsString)
{
//  std::cerr << "Warning: " << warning << std::endl;
  logInfo(warning,caller,user,argsString);
}

/*
void addToGindexList(std::string pindex,std::list<std::string>& gindex_list,int& min_gindex,int& max_gindex,std::string list_type)
{
  MySQLServer server;
  MySQLLocalQuery query;
  MySQLRow row;
  int i_pindex;

  if (!connectToRDAServer(server))
    return;
  if (list_type == "mss")
    query.set("select gindex from dsgroup where dsid = 'ds"+args.dsnum+"' and pindex = "+pindex+" and pmsscnt > 0");
  else if (list_type == "web")
    query.set("select gindex from dsgroup where dsid = 'ds"+args.dsnum+"' and pindex = "+pindex+" and dwebcnt > 0");
  if (query.submit(server) < 0) {
    std::cerr << query.error() << std::endl;
    exit(1);
  }
  if (query.num_rows() == 0) {
    gindex_list.push_back(pindex);
    i_pindex=std::stoi(pindex);
    if (i_pindex < min_gindex)
	min_gindex=i_pindex;
    if (i_pindex > max_gindex)
	max_gindex=i_pindex;
  }
  else {
    while (query.fetch_row(row))
	addToGindexList(row[0],gindex_list,min_gindex,max_gindex,list_type);
  }
}
*/

void getObsPer(std::string observationTypeValue,size_t numObs,DateTime start,DateTime end,double& obsper,std::string& unit)
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
    nyrs=end.getYear()-start.getYear();
    if (nyrs >= climo_len) {
	if (climo_len == 30)
	  nper=((nyrs % climo_len)/10)+1;
	else
	  nper=nyrs/climo_len;
	obsper=numObs/static_cast<float>(nper);
	if (static_cast<int>(numObs) == nper)
	  unit="year";
	else if (numObs/nper == 4)
	  unit="season";
	else if (numObs/nper == 12)
	  unit="month";
	else if (nper > 1 && start.getMonth() != end.getMonth()) {
	  nper=(nper-1)*12+12-start.getMonth()+1;
	  if (static_cast<int>(numObs) == nper)
	    unit="month";
	  else
	    unit="";
	}
	else
	  unit="";
    }
    else {
	if ((end.getMonth()-start.getMonth()) == static_cast<int>(numObs))
	  unit="month";
	else
	  unit="";
    }
    obsper=obsper-climo_len;
  }
  else {
    if (isLeapYear(end.getYear()))
	num_days[2]=29;
    obsper=numObs/static_cast<double>(end.getSecondsSince(start));
    if ((obsper+TOLERANCE) > 1.)
	unit="second";
    else {
	obsper*=60.;
	if ((obsper+TOLERANCE) > 1.)
	  unit="minute";
	else {
	  obsper*=60;
	  if ((obsper+TOLERANCE) > 1.)
	    unit="hour";
	  else {
	    obsper*=24;
	    if ((obsper+TOLERANCE) > 1.)
		unit="day";
	    else {
		obsper*=7;
		if ((obsper+TOLERANCE) > 1.)
		  unit="week";
		else {
		  obsper*=(num_days[end.getMonth()]/7.);
		  if ((obsper+TOLERANCE) > 1.)
		    unit="month";
		  else {
		    obsper*=12;
		    if ((obsper+TOLERANCE) > 1.)
			unit="year";
		    else
			unit="";
		  }
		}
	    }
	  }
	}
    }
    num_days[2]=28;
  }
}

std::string getWebHome()
{
  MySQL::Server server;
  connectToRDAServer(server);
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

std::string getRelativeWebFilename(std::string URL)
{
  strutils::replace_all(URL,"https://rda.ucar.edu","");
  strutils::replace_all(URL,"http://rda.ucar.edu","");
  strutils::replace_all(URL,"http://dss.ucar.edu","");
  if (std::regex_search(URL,std::regex("^/dsszone"))) {
    strutils::replace_all(URL,"/dsszone",directives.data_root_alias);
  }
  return strutils::substitute(URL,directives.data_root_alias+"/ds"+args.dsnum+"/","");
}

std::string cleanID(std::string ID)
{
  std::string cleanID=ID;
  size_t n;

  strutils::trim(cleanID);
  for (n=0; n < cleanID.length(); n++) {
    if (static_cast<int>(cleanID[n]) < 32 || static_cast<int>(cleanID[n]) > 127) {
	if (n > 0)
	  cleanID=cleanID.substr(0,n)+"/"+cleanID.substr(n+1);
	else
	  cleanID="/"+cleanID.substr(1);
    }
  }
  strutils::replace_all(cleanID,"\"","'");
  strutils::replace_all(cleanID,"&","&amp;");
  strutils::replace_all(cleanID,">","&gt;");
  strutils::replace_all(cleanID,"<","&lt;");
  return strutils::to_upper(cleanID);
}

void getFromAncillaryTable(MySQL::Server& srv,std::string tableName,std::string whereConditions,MySQL::LocalQuery& query,const std::string& caller,const std::string& user,const std::string& args_string)
{
// whereConditions must have 'and' specified as 'AND' because it is possible
//   that 'and' is in fields in the database tables
  std::deque<std::string> sp,sp2;
  size_t n;
  std::string columns,values,error,sdum;
  std::string wc=whereConditions;

  strutils::replace_all(wc," &eq; "," = ");
  query.set("code",tableName,wc);
  if (query.submit(srv) < 0)
    logError("getFromAncillaryTable returned error: "+query.error()+" from query: '"+query.show()+"'",caller,user,args_string);
  if (query.num_rows() == 0) {
    sp=strutils::split(whereConditions," AND ");
    for (n=0; n < sp.size(); n++) {
	sp2=strutils::split(sp[n]," = ");
	if (sp2.size() != 2) {
	  logError("getFromAncillaryTable error in whereConditions: "+whereConditions+", "+sp[n],caller,user,args_string);
	}
	sdum=sp2[0];
	strutils::trim(sdum);
	if (columns.length() > 0) {
	  columns+=",";
	}
	columns+=sdum;
	sdum=sp2[1];
	strutils::trim(sdum);
	strutils::replace_all(sdum," &eq; "," = ");
	if (values.length() > 0) {
	  values+=",";
	}
	values+=sdum;
    }
    std::string result;
    if (srv.command("lock table "+tableName+" write",result) < 0) {
	logError("getFromAncillaryTable returned "+srv.error(),caller,user,args_string);
    }
    if (srv.insert(tableName,columns,values,"") < 0) {
	error=srv.error();
	if (!strutils::has_beginning(error,"Duplicate entry")) {
	  logError("getFromAncillaryTable srv error: "+error+" while inserting ("+columns+") values("+values+") into "+tableName,caller,user,args_string);
	}
    }
    if (srv.command("unlock tables",result) < 0) {
	logError("getFromAncillaryTable returned "+srv.error(),caller,user,args_string);
    }
    query.submit(srv);
    if (query.num_rows() == 0)
      logError("getFromAncillaryTable error retrieving code from table "+tableName+" for value(s) ("+columns+") values("+values+")",caller,user,args_string);
  }
}

namespace NcTime {

DateTime getActualDateTime(double time,TimeData& time_data,std::string& error)
{
  DateTime actualDateTime;
  double diff;

  if (time_data.units == "seconds") {
if (fabs(time-static_cast<int>(time)) > 0.01) {
error="can't compute dates from fractional seconds";
}
    actualDateTime=time_data.reference.secondsAdded(time,time_data.calendar);
  }
  else if (time_data.units == "minutes") {
if (fabs(time-static_cast<int>(time)) > 0.01) {
error="can't compute dates from fractional minutes";
}
    actualDateTime=time_data.reference.minutesAdded(time,time_data.calendar);
  }
  else if (time_data.units == "hours") {
if (fabs(time-static_cast<int>(time)) > 0.01) {
error="can't compute dates from fractional hours";
}
    actualDateTime=time_data.reference.hoursAdded(time,time_data.calendar);
  }
  else if (time_data.units == "days") {
    actualDateTime=time_data.reference.daysAdded(time,time_data.calendar);
    if ( (diff=fabs(time-static_cast<int>(time))) > 0.01) {
	diff*=24.;
	if (fabs(diff-lround(diff)) > 0.01) {
	  diff*=60.;
	  if (fabs(diff-lround(diff)) > 0.01) {
	    diff*=60.;
	    if (fabs(diff-lround(diff)) > 0.01) {
		error="can't compute dates from fractional seconds in fractional days";
	    }
	    else {
		actualDateTime.addSeconds(lround(diff),time_data.calendar);
	    }
	  }
	  else {
	    actualDateTime.addMinutes(lround(diff),time_data.calendar);
	  }
	}
	else {
	  actualDateTime.addHours(lround(diff),time_data.calendar);
	}
    }
  }
  else if (time_data.units == "months") {
if (fabs(time-static_cast<int>(time)) > 0.01) {
error="can't compute dates from fractional months";
}
    actualDateTime=time_data.reference.monthsAdded(time);
  }
  else {
    error="don't understand time units in "+time_data.units;
  }
  return actualDateTime;
}

std::string getGriddedNetCDFTimeRangeDescription(const TimeRangeEntry& tre,const TimeData& time_data,std::string time_method,std::string& error)
{
  size_t idx;
  std::string time_range,lmethod;

  if (static_cast<int>(tre.key) < 0) {
    if (time_method.length() > 0) {
	lmethod=strutils::to_lower(time_method);
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
	  error="getGriddedNetCDFTimeRange: don't understand time units '"+time_data.units+"' for cell method '"+time_method+"'";
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
    if ( (idx=time_method.find(" over ")) != std::string::npos) {
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

std::string getTimeMethodFromCellMethods(std::string cell_methods,std::string timeid)
{
  size_t idx,idx2;
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
    if ( (idx=cell_methods.find(timeid+":")) == std::string::npos)
	return "";
    if (idx != 0)
	cell_methods=cell_methods.substr(idx);
    strutils::replace_all(cell_methods,timeid+":","");
    strutils::trim(cell_methods);
    if ( (idx=cell_methods.find(":")) != std::string::npos) {
	if ( (idx2=cell_methods.find(")")) == std::string::npos) {
// no extra information in parentheses
 	  cell_methods=cell_methods.substr(0,idx);
	  cell_methods=cell_methods.substr(0,cell_methods.rfind(" "));
	}
	else
// found extra information so include that in the methods
	  cell_methods=cell_methods.substr(0,idx2+1);
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
    if (!valid_cf_cell_methods_table->found(cell_methods,se))
	cell_methods="!"+cell_methods;
    return cell_methods;
  }
  else
    return "";
}

} // end namespace NcTime

namespace NcLevel {

std::string writeLevelMap(std::string dsnum,const LevelInfo& level_info)
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
  std::string sdum=getRemoteWebFile("https://rda.ucar.edu/metadata/LevelTables/"+format+".ds"+dsnum+".xml",directives.tempPath);
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
  TempFile tfile(directives.tempPath);
  std::ofstream ofs(tfile.name().c_str());
  if (!ofs.is_open()) {
    return "can't open "+tfile.name()+" file for writing netCDF levels";
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
    if (level_info.write[n] == 1 && (map_contents.size() == 0 || (map_contents.size() > 0 && level_map.isLayer(level_info.ID[n]) < 0))) {
	ofs << "  <level code=\"" << level_info.ID[n] << "\">" << std::endl;
	ofs << "    <description>" << level_info.description[n] << "</description>" << std::endl;
	ofs << "    <units>" << level_info.units[n] << "</units>" << std::endl;
	ofs << "  </level>" << std::endl;
    }
  }
  ofs << "</levelMap>" << std::endl;
  ofs.close();
  std::string herror;
  if (hostSync(tfile.name(),"/__HOST__/web/metadata/LevelTables/"+format+".ds"+dsnum+".xml",herror) < 0) {
    return herror;
  }
  std::stringstream output,error;
  mysystem2("/bin/cp "+tfile.name()+" /glade/u/home/rdadata/share/metadata/LevelTables/"+format+".ds"+dsnum+".xml",output,error);
  return error.str();
}

} // end namespace NcLevel

namespace NcParameter {

std::string writeParameterMap(std::string dsnum,std::list<std::string>& varlist,my::map<metautils::StringEntry>& var_changes_table,std::string map_type,std::string map_name,bool found_map,std::string& warning)
{
  std::ifstream ifs;
  std::ofstream ofs;
  char line[32768];
  std::stringstream output,error;
  std::string format,herror;
  std::list<std::string> map_contents;
  std::deque<std::string> sp;
  metautils::StringEntry se;
  TempFile *tfile=NULL;
  bool no_write;

  if (varlist.size() > 0) {
    if (found_map) {
	ifs.open(map_name.c_str());
	ifs.getline(line,32768);
	while (!ifs.eof()) {
	  map_contents.push_back(line);
	  ifs.getline(line,32768);
	}
	ifs.close();
	map_contents.pop_back();
    }
    else {
	tfile=new TempFile(directives.tempPath);
	map_name=tfile->name();
    }
    ofs.open(map_name.c_str());
    if (!ofs.is_open()) {
        return "can't open parameter map file for output";
    }
    if (!found_map) {
	ofs << "<?xml version=\"1.0\" ?>" << std::endl;
	ofs << "<" << map_type << "Map>" << std::endl;
    }
    else {
	no_write=false;
	for (auto& line : map_contents) {
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
    if (hostSync(map_name,"/__HOST__/web/metadata/ParameterTables/"+format+".ds"+dsnum+".xml",herror) < 0) {
        warning="parameter map was not synced - error(s): '"+herror+"'";
    }
    mysystem2("/bin/cp "+map_name+" /glade/u/home/rdadata/share/metadata/ParameterTables/"+format+".ds"+dsnum+".xml",output,error);
    if (tfile != NULL) {
        delete tfile;
    }
  }
  return error.str();
}

} // end namespace NcParameter

namespace CF {

void fill_NcTimeData(std::string units_attribute_value,NcTime::TimeData& time_data,std::string user)
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
	metautils::logError("fill_NcTimeData() returned error: unable to get reference time from units specified as: '"+units_attribute_value+"'","nc2xml",user,args.argsString);
    }
    auto dparts=strutils::split(uparts[0],"-");
    if (dparts.size() != 3) {
        metautils::logError("fill_NcTimeData() returned error: unable to get reference time from units specified as: '"+units_attribute_value+"'","nc2xml",user,args.argsString);
    }
    time_data.reference.setYear(std::stoi(dparts[0]));
    time_data.reference.setMonth(std::stoi(dparts[1]));
    time_data.reference.setDay(std::stoi(dparts[2]));
    if (uparts.size() > 1) {
	auto tparts=strutils::split(uparts[1],":");
	switch (tparts.size()) {
	  case 1:
	  {
	    time_data.reference.setTime(std::stoi(tparts[0])*10000);
	    break;
	  }
	  case 2:
	  {
	    time_data.reference.setTime(std::stoi(tparts[0])*10000+std::stoi(tparts[1])*100);
	    break;
	  }
	  case 3:
	  {
	    time_data.reference.setTime(std::stoi(tparts[0])*10000+std::stoi(tparts[1])*100+static_cast<int>(std::stof(tparts[2])));
	    break;
	  }
	}
    }
  }
  else {
    metautils::logError("fill_NcTimeData() returned error: unable to get CF time from time variable units","nc2xml",user,args.argsString);
  }
}

} // end namespace CF

} // end namespace metautils

namespace primaryMetadata {

bool checkFile(std::string dirname,std::string filename,std::string *file_format)
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
  getBits(buffer,idum,0,32);
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
  struct stat buf;

  metautils::connectToRDAServer(server);
  query.set("select m.mssfile from wfile as w left join mssfile as m on m.mssid = w.mssid where w.dsid = 'ds"+args.dsnum+"' and w.wfile = '"+metautils::getRelativeWebFilename(URL)+"' and w.property = 'A' and w.type = 'D' and m.data_size = w.data_size");
  if (query.submit(server) < 0) {
    std::cerr << query.error() << std::endl;
    exit(1);
  }
  if (query.fetch_row(row) && row[0].length() > 0) {
    metadata_file=row[0];
    strutils::replace_all(metadata_file,"/FS/DSS/","");
    strutils::replace_all(metadata_file,"/DSS/","");
    strutils::replace_all(metadata_file,"/","%");
    if (existsOnRDAWebServer("rda.ucar.edu","/SERVER_ROOT/web/datasets/ds"+args.dsnum+"/metadata/fmd/"+metadata_file+".GrML",buf))
	metadata_file+=".GrML";
    else if (existsOnRDAWebServer("rda.ucar.edu","/SERVER_ROOT/web/datasets/ds"+args.dsnum+"/metadata/fmd/"+metadata_file+".ObML",buf))
	metadata_file+=".ObML";
    else if (existsOnRDAWebServer("rda.ucar.edu","/SERVER_ROOT/web/datasets/ds"+args.dsnum+"/metadata/fmd/"+metadata_file+".SatML",buf))
	metadata_file+=".SatML";
    else
	metadata_file=row[0];
  }
  else
    metadata_file="";
  server.disconnect();
}

} // end namespace primaryMetadata
