#include <fstream>
#include <sstream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utime.h>
#include <sys/stat.h>
#include <MySQL.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bitmap.hpp>
#include <xmlutils.hpp>
#include <metahelpers.hpp>
#include <myerror.hpp>

namespace webutils {

bool filled_inventory(std::string dsnum,std::string data_file,std::string type,std::stringstream& inventory)
{
  auto dsnum2=strutils::substitute(dsnum,".","");
  MySQL::Server server("rda-db.ucar.edu","metadata","metadata","");
  if (!server) {
    myerror="database connection error - "+server.error();
    return false;
  }
  MySQL::LocalQuery query("select w.code,w.format_code,f.format,w.uflag from WGrML.ds"+dsnum2+"_webfiles2 as w left join WGrML.formats as f on f.code = w.format_code where webID = '"+data_file+"'");
  MySQL::Row row;
  if (query.submit(server) < 0 || !query.fetch_row(row)) {
    myerror="database error - "+query.error();
    return false;
  }
  auto webid_code=row[0];
  auto data_format_code=row[1];
  auto data_format=row[2];
  auto uflag=row[3]+"\n";
  auto inv_file="/usr/local/www/server_root/web/datasets/ds"+dsnum+"/metadata/inv/"+data_file+"."+type+"_inv";
  auto versions_match=false;
  struct stat buf;
  if (stat((inv_file+".vrsn").c_str(),&buf) == 0) {
    std::stringstream oss,ess;
    unixutils::mysystem2("/bin/tcsh -c \"cat "+inv_file+".vrsn\"",oss,ess);
    versions_match=(oss.str() == uflag);
  }
  if (stat(inv_file.c_str(),&buf) != 0 || !versions_match) {
    query.set("distinct process","WGrML.ds"+dsnum2+"_processes","webID_code = "+webid_code);
    if (query.submit(server) < 0) {
	myerror="database error - "+query.error();
	return false;
    }
    std::unordered_set<std::string> processes;
    for (const auto& res : query) {
	processes.emplace(res[0]);
    }
    query.set("select timeRange_codes,gridDefinition_codes,parameter,levelType_codes from WGrML.ds"+dsnum2+"_agrids2 where webID_code = "+webid_code);
    if (query.submit(server) < 0) {
	myerror="database error - "+query.error();
	return false;
    }
    std::unordered_map<size_t,std::string> time_ranges,grid_definitions,levels;
    std::unordered_map<std::string,std::string> parameters;
    std::unordered_set<std::string> t,g,l;
    std::string inv_query;
    xmlutils::ParameterMapper pmapper("/usr/local/www/server_root/web/metadata/ParameterTables");
    for (const auto& res : query) {
	std::vector<size_t> values;
	if (t.find(res[0]) == t.end()) {
	  bitmap::uncompress_values(res[0],values);
	  for (const auto& code : values) {
	    if (time_ranges.find(code) == time_ranges.end()) {
		MySQL::LocalQuery tr_query("timeRange","WGrML.timeRanges","code = "+strutils::itos(code));
		MySQL::Row row;
		std::string time_range;
		if (tr_query.submit(server) == 0 && tr_query.fetch_row(row)) {
		  time_range=row[0];
		}
		time_ranges.emplace(code,time_range);
	    }
	  }
	  t.emplace(res[0]);
	}
	if (g.find(res[1]) == g.end()) {
	  bitmap::uncompress_values(res[1],values);
	  for (const auto& code : values) {
	    if (grid_definitions.find(code) == grid_definitions.end()) {
		MySQL::LocalQuery gd_query("definition,defParams","WGrML.gridDefinitions","code = "+strutils::itos(code));
		MySQL::Row row;
		std::string grid_definition;
		if (gd_query.submit(server) == 0 && gd_query.fetch_row(row)) {
		  grid_definition=row[0]+","+strutils::substitute(row[1],":",",");
		}
		grid_definitions.emplace(code,grid_definition);
	    }
	  }
	  g.emplace(res[1]);
	}
	if (parameters.find(res[2]) == parameters.end()) {
	  if (!inv_query.empty()) {
	    inv_query+=" union all ";
	  }
	  inv_query+="select byte_offset,byte_length,valid_date,timeRange_code,gridDefinition_code,level_code,'"+res[2]+"',process from IGrML.`ds"+dsnum2+"_inventory_"+data_format_code+"!"+res[2]+"` where webID_code = "+webid_code;
	  parameters.emplace(res[2],pmapper.description(data_format,res[2]));
	}
	if (l.find(res[3]) == l.end()) {
	  bitmap::uncompress_values(res[3],values);
	  for (const auto& code : values) {
	    if (levels.find(code) == levels.end()) {
		MySQL::LocalQuery l_query("map,type,value","WGrML.levels","code = "+strutils::itos(code));
		MySQL::Row row;
		std::string level;
		if (l_query.submit(server) == 0 && l_query.fetch_row(row)) {
		  level=row[0]+","+row[1]+":"+row[2];
		}
		levels.emplace(code,level);
	    }
	  }
	  l.emplace(res[3]);
	}
    }
    auto time_range_compare=
    [](const std::string& left,const std::string& right) -> bool
    {
	return metacompares::compare_time_ranges(left,right);
    };
    std::map<std::string,size_t,decltype(time_range_compare)> sorted_time_ranges(time_range_compare);
    for (const auto& e : time_ranges) {
	sorted_time_ranges.emplace(e.second,e.first);
    }
if (sorted_time_ranges.size() != time_ranges.size()) {
std::cerr << "***inventory time ranges: " << sorted_time_ranges.size() << " " << time_ranges.size() << std::endl;
}
    auto level_compare=
    [](const std::string& left,const std::string& right) -> bool
    {
	return metacompares::compare_levels(left,right);
    };
    std::map<std::string,size_t,decltype(level_compare)> sorted_levels(level_compare);
    xmlutils::LevelMapper lmapper("/usr/local/www/server_root/web/metadata/LevelTables");
    for (auto& e : levels) {
	auto parts=strutils::split(e.second,":");
	auto l_parts=strutils::split(parts.front(),",");
	std::vector<std::string> level_units;
	auto level_values=strutils::split(parts.back(),",");
	if (l_parts.front() != l_parts.back()) {
	  e.second=lmapper.description(data_format,l_parts.back(),l_parts.front());
	  auto level_codes=strutils::split(l_parts.back(),"-");
	  for (const auto& level_code : level_codes) {
	    level_units.emplace_back(lmapper.units(data_format,level_code,l_parts.front()));
	  }
	}
	else {
	  e.second=lmapper.description(data_format,l_parts.front());
	  auto level_codes=strutils::split(l_parts.front(),"-");
	  for (const auto& level_code : level_codes) {
	    level_units.emplace_back(lmapper.units(data_format,level_code));
	  }
	}
	std::string level;
	if (level_values.size() > 1 || level_values[0] != "0" || !level_units[0].empty()) {
	  level=level_values.front();
	  if (!level_units.front().empty()) {
	    level+=" "+level_units.front();
	  }
	  if (level_values.size() > 1) {
	    level+=", "+level_values.back();
	    if (!level_units.back().empty()) {
		level+=" "+level_units.back();
	    }
	  }
	}
	if (!level.empty()) {
	  e.second+=": "+level;
	}
	sorted_levels.emplace(e.second,e.first);
    }
if (sorted_levels.size() != levels.size()) {
std::cerr << "***inventory levels: " << sorted_levels.size() << " " << levels.size() << std::endl;
}
    auto parameter_compare=
    [](const std::string& left,const std::string& right) -> bool
    {
	return (strutils::to_lower(left) < strutils::to_lower(right));
    };
    std::map<std::string,std::string,decltype(parameter_compare)> sorted_parameters(parameter_compare);
    for (const auto& e : parameters) {
	sorted_parameters.emplace(e.second,e.first);
    }
if (sorted_parameters.size() != parameters.size()) {
std::cerr << "***inventory parameters: " << sorted_parameters.size() << " " << parameters.size() << std::endl;
}
    std::stringstream oss,ess;
    if (unixutils::mysystem2("/bin/mkdir -p /usr/local/www/server_root/web/datasets/ds"+dsnum+"/metadata/inv/"+data_file.substr(0,data_file.rfind("/")),oss,ess) < 0) {
	myerror="directory error - "+ess.str();
	return false;
    }
    std::ofstream ofs(inv_file.c_str());
    if (!ofs.is_open()) {
	myerror="unable to create inventory";
	return false;
    }
    for (const auto& e : sorted_time_ranges) {
	ofs << "U<!>" << std::distance(sorted_time_ranges.begin(),sorted_time_ranges.find(e.first)) << "<!>" << e.second << "<!>" << e.first << std::endl;
    }
    for (const auto& e : grid_definitions) {
	ofs << "G<!>" << std::distance(grid_definitions.begin(),grid_definitions.find(e.first)) << "<!>" << e.first << "<!>" << e.second << std::endl;
    }
    for (const auto& e : sorted_levels) {
	ofs << "L<!>" << std::distance(sorted_levels.begin(),sorted_levels.find(e.first)) << "<!>" << e.second << "<!>" << e.first << std::endl;
    }
    for (const auto& e : sorted_parameters) {
	ofs << "P<!>" << std::distance(sorted_parameters.begin(),sorted_parameters.find(e.first)) << "<!>" << data_format_code << "!" << e.second << "<!>" << e.first << std::endl;
    }
    for (const auto& p : processes) {
	ofs << "R<!>" << std::distance(processes.begin(),processes.find(p)) << "<!>" << p << std::endl;
    }
    ofs << "-----" << std::endl;
    query.set(inv_query+" order by byte_offset");
    if (query.submit(server) == 0) {
	for (const auto& res : query) {
	  ofs << res[0] << "|" << res[1] << "|" << res[2] << "|" << std::distance(sorted_time_ranges.begin(),sorted_time_ranges.find(time_ranges[std::stoi(res[3])])) << "|" << std::distance(grid_definitions.begin(),grid_definitions.find(std::stoi(res[4]))) << "|" << std::distance(sorted_levels.begin(),sorted_levels.find(levels[std::stoi(res[5])])) << "|" << std::distance(sorted_parameters.begin(),sorted_parameters.find(parameters[res[6]])) << "|" << std::distance(processes.begin(),processes.find(res[7])) << std::endl;
	}
    }
    ofs.close();
    ofs.open((inv_file+".vrsn").c_str());
    ofs << uflag;
    ofs.close();
    server.disconnect();
  }
  else {
    utime(inv_file.c_str(),nullptr);
  }
  std::stringstream error;
  if (unixutils::mysystem2("/bin/tcsh -c \"cat "+inv_file+"\"",inventory,error) < 0) {
    myerror=error.str();
    return false;
  }
  else {
    return true;
  }
}

} // end namespace webutils
