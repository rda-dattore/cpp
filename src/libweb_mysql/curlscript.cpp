#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <ftw.h>
#include <sys/stat.h>
#include <strutils.hpp>
#include <web/web.hpp>
#include <MySQL.hpp>

struct StringEntry {
  StringEntry() : key() {}

  std::string key;
};

std::list<std::string> curl_filelist;
extern "C" int find_curl_directory_files(const char *name,const struct stat64 *data,int flag,struct FTW *ftw_struct)
{
  std::string sname;

  if (flag == FTW_F) {
    sname=name;
    if (!strutils::contains(sname,"wget"))
        curl_filelist.push_back(sname);
  }
  return 0;
}

void print_curl_head(std::string script_type,std::string& remark,std::string& cert_opt,std::string& opts,std::ostream *outs)
{
  std::string server_name=getenv("SERVER_NAME");
  std::string user=duser();

  if (script_type == "csh") {
    remark="#";
    opts="$opts";
    *outs << "#! /bin/csh -f" << std::endl;
    *outs << "#" << std::endl;
    *outs << "# c-shell script to download selected files from <server_name> using curl" << std::endl;
    *outs << "# NOTE: if you want to run under a different shell, make sure you change" << std::endl;
    *outs << "#       the 'set' commands according to your shell's syntax" << std::endl;
    *outs << "# after you save the file, don't forget to make it executable" << std::endl;
    *outs << "#   i.e. - \"chmod 755 <name_of_script>\"" << std::endl;
    *outs << "#" << std::endl;
  }
  else if (script_type == "bat") {
    remark="::";
    opts="%opts%";
    *outs << ":: DOS batch script to download selected files from <server_name> using curl" << std::endl;
  }
  *outs << remark << " you can add cURL options here (progress bars, etc.)" << std::endl;
  *outs << "set opts = \"\"" << std::endl;
  *outs << remark << std::endl;
  if (user.empty()) {
    *outs << remark << " Replace \"xxx@xxx.xxx\" with your email address" << std::endl;
    *outs << "set user = \"xxx@xxx.xxx\"" << std::endl;
  }
  *outs << remark << " Replace \"xxxxxx\" with your rda.ucar.edu password on the next uncommented line" << std::endl;
  if (script_type == "csh") {
    *outs << "# IMPORTANT NOTE:  If your password uses a special character that has special meaning" << std::endl;
    *outs << "#                  to csh, you should escape it with a backslash" << std::endl;
    *outs << "#                  Example:  set passwd = \"my\\!password\"" << std::endl;
  }
  if (script_type == "csh") {
    *outs << "set passwd = 'xxxxxx'" << std::endl;
    *outs << "set num_chars = `echo \"$passwd\" |awk '{print length($0)}'`" << std::endl;
    *outs << "@ num = 1" << std::endl;
    *outs << "set newpass = \"\"" << std::endl;
    *outs << "while ($num <= $num_chars)" << std::endl;
    *outs << "  set c = `echo \"$passwd\" |cut -b{$num}-{$num}`" << std::endl;
    *outs << "  if (\"$c\" == \"&\") then" << std::endl;
    *outs << "    set c = \"%26\";" << std::endl;
    *outs << "  else" << std::endl;
    *outs << "    if (\"$c\" == \"?\") then" << std::endl;
    *outs << "      set c = \"%3F\"" << std::endl;
    *outs << "    else" << std::endl;
    *outs << "      if (\"$c\" == \"=\") then" << std::endl;
    *outs << "        set c = \"%3D\"" << std::endl;
    *outs << "      endif" << std::endl;
    *outs << "    endif" << std::endl;
    *outs << "  endif" << std::endl;
    *outs << "  set newpass = \"$newpass$c\"" << std::endl;
    *outs << "  @ num ++" << std::endl;
    *outs << "end" << std::endl;
    *outs << "set passwd = \"$newpass\"" << std::endl;
  }
  else {
    *outs << "set passwd = \"xxxxxx\"" << std::endl;
  }
  *outs << remark << std::endl;
  if (script_type == "csh") {
    *outs << "if (\"$passwd\" == \"xxxxxx\") then" << std::endl;
  }
  else {
    *outs << "if ($passwd == \"xxxxxx\") then" << std::endl;
  }
  *outs << "  echo \"You need to set your password before you can continue\"" << std::endl;
  *outs << "  echo \"  see the documentation in the script\"" << std::endl;
  *outs << "  exit" << std::endl;
  *outs << "endif" << std::endl;
  *outs << remark << std::endl;
  *outs << remark << " authenticate - NOTE: You should only execute this command ONE TIME." << std::endl;
  *outs << remark << " Executing this command for every data file you download may cause" << std::endl;
  *outs << remark << " your download privileges to be suspended." << std::endl;
  *outs << "curl -o auth_status." << server_name << " -k -s -c auth." << server_name;
  if (script_type == "csh") {
    *outs << ".$$";
  }
  *outs << " -d \"email=";
  if (!user.empty()) {
    *outs << user;
  }
  else {
    *outs << "$user";
  }
  *outs << "&passwd=";
  if (script_type == "csh") {
    *outs << "$passwd";
  }
  else if (script_type == "bat") {
    *outs << "%passwd%";
  }
  *outs << "&action=login\" https://" << server_name << "/cgi-bin/login" << std::endl;
}

void print_curl_tail(std::string script_type,std::string remark,std::ostream *outs)
{
  std::string server_name=getenv("SERVER_NAME");

  *outs << remark << std::endl;
  *outs << remark << " clean up" << std::endl;
  if (script_type == "csh")
    *outs << "rm";
  else if (script_type == "bat")
    *outs << "DEL";
  *outs << " auth." << server_name;
  if (script_type == "csh")
    *outs << ".$$";
  *outs << " auth_status." << server_name;
  *outs << std::endl;
}

void create_curl_script(std::list<std::string>& filelist,std::string directory,std::string dsnum,std::string script_type,std::string parameters,std::string level,std::string product,std::string start_date,std::string end_date)
{
  std::ifstream ifs;
  char line[32768];
  std::string local_file,inv_file;
  struct stat buf;
  my::map<StringEntry> selected_parameters_table,selected_levels_table,selected_products_table;
  my::map<StringEntry> parameter_table,level_table,product_table;
  std::list<long long> range_start,range_end;
  std::list<long long>::iterator it_start,it_end,end_pos;
  StringEntry se;
  std::string sline,sdum;
  std::deque<std::string> sp,spx;
  size_t n,m;
  int idx,last_match;
  long long byte_pos,start,end=0;
  size_t num_lines=0,num_matches=0;
  MySQL::Server server;
  MySQL::Query query;
  MySQL::Row row;
  std::string remark,cert_opt,opts,server_name=getenv("SERVER_NAME");
  bool matched;

  if (!parameters.empty()) {
    sp=strutils::split(parameters,"[!]");
    for (n=0; n < sp.size(); n++) {
	if (!sp[n].empty()) {
	  spx=strutils::split(sp[n],",");
	  for (m=0; m < spx.size(); m++) {
	    se.key=spx[m];
	    if ( (idx=se.key.find("!")) > 0)
		se.key=se.key.substr(idx+1);
	    selected_parameters_table.insert(se);
	  }
	}
    }
  }
  if (!level.empty()) {
    server.connect("rda-db.ucar.edu","metadata","metadata","");
    sp=strutils::split(level,"[!]");
    for (n=0; n < sp.size(); n++) {
	if (!sp[n].empty()) {
	  spx=strutils::split(sp[n],",");
	  for (m=0; m < spx.size(); m++) {
	    query.set("map,type,value","WGrML.levels","code = "+spx[m]);
	    if (query.submit(server) == 0) {
		query.fetch_row(row);
		if (!row[0].empty() > 0) {
		  se.key=row[0]+",";
		}
		else {
		  se.key="";
		}
		se.key+=row[1]+":"+row[2];
		selected_levels_table.insert(se);
	    }
	  }
	}
    }
    server.disconnect();
  }
  if (!product.empty()) {
    server.connect("rda-db.ucar.edu","metadata","metadata","");
    sp=strutils::split(product,",");
    for (n=0; n < sp.size(); n++) {
	query.set("timeRange","WGrML.timeRanges","code = "+sp[n]);
	if (query.submit(server) == 0) {
	  query.fetch_row(row);
	  se.key=row[0];
	  selected_products_table.insert(se);
	}
    }
    server.disconnect();
  }
  if (!start_date.empty() && !end_date.empty()) {
    strutils::replace_all(start_date,"-","");
    strutils::replace_all(start_date,":","");
    strutils::replace_all(start_date," ","");
    strutils::replace_all(end_date,"-","");
    strutils::replace_all(end_date,":","");
    strutils::replace_all(end_date," ","");
  }
  print_curl_head(script_type,remark,cert_opt,opts,&std::cout);
  std::cout << remark << std::endl;
  std::cout << remark << " download the file(s)" << std::endl;
  std::cout << remark << " NOTE:  if you get 403 Forbidden errors when downloading the data files, check" << std::endl;
  std::cout << remark << "        the contents of the file 'auth_status.rda.ucar.edu'" << std::endl;
  for (auto& file : filelist) {
    local_file=file;
    while (strutils::contains(local_file,"/")) {
	local_file=local_file.substr(local_file.find("/")+1);
    }
    inv_file="/usr/local/www/server_root/web/datasets/ds"+dsnum+"/metadata/inv/"+strutils::substitute(file,"/","%")+".GrML_inv";
    TempFile *tfile=nullptr;
    if (stat((inv_file+".gz").c_str(),&buf) == 0) {
	tfile=new TempFile("/usr/local/www/server_root/tmp");
	system(("cp "+inv_file+".gz "+tfile->name()+".gz").c_str());
    }
    if (tfile != nullptr) {
	system(("gunzip -f "+tfile->name()+".gz").c_str());
	inv_file=tfile->name();
    }
//std::cerr << inv_file << std::endl;
    if (stat(inv_file.c_str(),&buf) == 0 && (!parameters.empty() || !level.empty() || !product.empty() || (!start_date.empty() && !end_date.empty()))) {
	parameter_table.clear();
	level_table.clear();
	product_table.clear();
	range_start.clear();
	range_end.clear();
	last_match=-1;
	ifs.open(inv_file.c_str());
	if (ifs.is_open()) {
	  ifs.getline(line,32768);
	  sline=line;
	  while (!ifs.eof() && sline != "-----") {
	    sp=strutils::split(sline,"<!>");
	    if (sp[0] == "P") {
	      if (selected_parameters_table.found(sp[2],se)) {
		  se.key=sp[1];
		  parameter_table.insert(se);
		}
	    }
	    else if (sp[0] == "L") {
		if (strutils::occurs(sp[2],":") == 2) {
		  spx=strutils::split(sp[2],":");
		  sdum=spx[0]+":"+spx[2]+","+spx[1];
		}
		else
		  sdum=sp[2];
		if (selected_levels_table.found(sdum,se)) {
		  se.key=sp[1];
		  level_table.insert(se);
		}
	    }
	    else if (sp[0] == "U") {
		if (selected_products_table.found(sp[2],se)) {
		  se.key=sp[1];
		  product_table.insert(se);
		}
	    }
	    ifs.getline(line,32768);
	    if (!ifs.eof())
		sline=line;
	  }
	  ifs.getline(line,32768);
	  start=-1;
	  while (!ifs.eof()) {
	    sline=line;
	    num_lines++;
	    sp=strutils::split(sline,"|");
	    matched=true;
	    if (parameter_table.size() > 0 && !parameter_table.found(sp[6],se))
{
		matched=false;
//std::cerr << "A " << matched << std::endl;
}
	    if (matched && !level.empty() && !level_table.found(sp[5],se))
{
		matched=false;
//std::cerr << "B " << matched << std::endl;
}
	    if (matched && !product.empty() && !product_table.found(sp[3],se))
{
		matched=false;
//std::cerr << "C " << matched << std::endl;
}
	    if (matched && !start_date.empty() && !end_date.empty() && (sp[2] < start_date || sp[2] > end_date))
{
		matched=false;
//std::cerr << "D " << matched << std::endl;
}
	    if (matched) {
		byte_pos=std::stoll(sp[0]);
		if (byte_pos != last_match) {
		  if (start < 0) {
		    start=byte_pos;
		    end=start+std::stoll(sp[1])-1;
		  }
		  else {
		    if (byte_pos != (end+1)) {
			range_start.push_back(start);
			range_end.push_back(end);
			start=byte_pos;
		    }
		    end=byte_pos+std::stoll(sp[1])-1;
		  }
		  last_match=byte_pos;
		}
		num_matches++;
	    }
	    ifs.getline(line,32768);
	  }
	  ifs.close();
	  range_start.push_back(start);
	  range_end.push_back(end);
	}
	if (range_start.size() > 0 && num_matches < num_lines) {
	  std::cout << "curl " << opts << " -k -b auth." << server_name;
	  if (script_type == "csh")
	    std::cout << ".$$";
	  std::cout << " -r ";
	  n=0;
	  for (it_start=range_start.begin(),it_end=range_end.begin(),end_pos=range_start.end(); it_start != end_pos; ++it_start,++it_end) {
	    if (n > 0)
		std::cout << ",";
	    std::cout << *it_start << "-" << *it_end;
	    n++;
	  }
	  strutils::replace_all(local_file,".tar","");
	  std::cout << " https://" << server_name << directory << "/" << file << " -o " << local_file << ".subset" << std::endl;
	}
	else {
	  std::cout << "curl " << opts << " -k -b auth." << server_name;
	  if (script_type == "csh")
	    std::cout << ".$$";
	  std::cout << " https://" << server_name << directory << "/" << file << " -o " << local_file << std::endl;
	}
    }
    else {
	std::cout << "curl " << opts << " -k -b auth." << server_name;
	if (script_type == "csh")
	  std::cout << ".$$";
	std::cout << " https://" << server_name << directory << "/" << file << " -o " << local_file << std::endl;
    }
    if (tfile != nullptr) {
	delete tfile;
    }
  }
  print_curl_tail(script_type,remark,&std::cout);
}

void create_curl_script(std::list<std::string>& filelist,std::string directory,std::string script_type,std::ofstream *ostream)
{
  std::ostream *outs;
  std::list<std::string> *flist;
  std::string remark,cert_opt,opts,server_name=getenv("SERVER_NAME");

  if (ostream == NULL) {
    outs=&std::cout;
  }
  else {
    outs=ostream;
  }
  print_curl_head(script_type,remark,cert_opt,opts,outs);
  *outs << remark << std::endl;
  *outs << remark << " download the file(s)" << std::endl;
  *outs << remark << " NOTE:  if you get 403 Forbidden errors when downloading the data files, check" << std::endl;
  *outs << remark << "        the contents of the file 'auth_status.rda.ucar.edu'" << std::endl;
  if (filelist.size() == 0) {
    nftw64(directory.c_str(),find_curl_directory_files,1,0);
    flist=&curl_filelist;
  }
  else {
    flist=&filelist;
  }
  for (auto& file : *flist) {
    *outs << "curl " << cert_opt << " " << opts << " -b auth." << server_name;
    if (script_type == "csh") {
	*outs << ".$$";
    }
    *outs << " https://" << server_name;
    if (flist == &filelist && !directory.empty()) {
	*outs << directory;
    }
    *outs << "/" << file << " -o " << file << std::endl;
  }
  print_curl_tail(script_type,remark,outs);
}
