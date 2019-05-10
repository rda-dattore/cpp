#include <metadata.hpp>
#include <strutils.hpp>
#include <utils.hpp>

namespace metautils {

namespace NcTime {

DateTime actual_date_time(double time,const TimeData& time_data,std::string& error)
{
  DateTime date_time;
  if (time_data.units == "seconds") {
if (fabs(time-static_cast<long long>(time)) > 0.01) {
error="can't compute dates from fractional seconds";
}
    if (time >= 0.) {
	date_time=time_data.reference.seconds_added(time,time_data.calendar);
    }
    else {
	date_time=time_data.reference.seconds_subtracted(-time,time_data.calendar);
    }
  }
  else if (time_data.units == "minutes") {
if (fabs(time-static_cast<long long>(time)) > 0.01) {
error="can't compute dates from fractional minutes";
}
    if (time >= 0.) {
	date_time=time_data.reference.minutes_added(time,time_data.calendar);
    }
    else {
	date_time=time_data.reference.minutes_subtracted(-time,time_data.calendar);
    }
  }
  else if (time_data.units == "hours") {
if (fabs(time-static_cast<long long>(time)) > 0.01) {
error="can't compute dates from fractional hours";
}
    if (time >= 0.) {
	date_time=time_data.reference.hours_added(time,time_data.calendar);
    }
    else {
	date_time=time_data.reference.hours_subtracted(-time,time_data.calendar);
    }
  }
  else if (time_data.units == "days") {
    if (time >= 0.) {
	date_time=time_data.reference.days_added(time,time_data.calendar);
    }
    else {
	date_time=time_data.reference.days_subtracted(-time,time_data.calendar);
    }
    auto diff=fabs(time-static_cast<long long>(time));
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
if (fabs(time-static_cast<long long>(time)) > 0.01) {
error="can't compute dates from fractional months";
}
    if (time >= 0.) {
	date_time=time_data.reference.months_added(time);
    }
    else {
	date_time=time_data.reference.months_subtracted(-time);
    }
  }
  else {
    error="don't understand time units in "+time_data.units;
  }
  return date_time;
}

DateTime reference_date_time(std::string units_attribute_value)
{
  DateTime reference_dt;
  auto idx=units_attribute_value.find("since");
  if (idx == std::string::npos) {
    return reference_dt;
  }
  units_attribute_value=units_attribute_value.substr(idx+5);
  strutils::replace_all(units_attribute_value,"T"," ");
  strutils::trim(units_attribute_value);
  if (std::regex_search(units_attribute_value,std::regex("Z$"))) {
    strutils::chop(units_attribute_value);
  }
  auto dt_parts=strutils::split(units_attribute_value);
  auto date_parts=strutils::split(dt_parts[0],"-");
  if (date_parts.size() != 3) {
    return reference_dt;
  }
  auto dt=std::stoll(date_parts[0])*10000000000+std::stoll(date_parts[1])*100000000+std::stoll(date_parts[2])*1000000;
  if (dt_parts.size() > 1) {
    auto time_parts=strutils::split(dt_parts[1],":");
    dt+=std::stoll(time_parts[0])*10000;
    if (time_parts.size() > 1) {
	dt+=std::stoll(time_parts[1])*100;
    }
    if (time_parts.size() > 2) {
	dt+=static_cast<long long>(std::stod(time_parts[2]));
    }
  }
  reference_dt.set(dt);
  return reference_dt;
}

std::string gridded_netcdf_time_range_description(const TimeRangeEntry& tre,const TimeData& time_data,std::string time_method,std::string& error)
{
  std::string time_range;
  if (static_cast<int>(tre.key) < 0) {
    if (!time_method.empty()) {
	auto lmethod=strutils::to_lower(time_method);
	if (strutils::contains(lmethod,"monthly")) {
	  time_range="Monthly ";
	}
	else if (time_data.units == "hours") {
	  if (tre.data->bounded.first_valid_datetime.year() > 0) {
	    auto num_hours=tre.data->instantaneous.first_valid_datetime.hours_since(tre.data->bounded.first_valid_datetime);
	    switch (num_hours) {
		case 1: {
		  time_range="Hourly ";
		}
		default: {
		  time_range=strutils::itos(num_hours)+"-hour ";
		}
	    }
	  }
	  else {
	    time_range="Hourly ";
	  }
	}
	else if (time_data.units == "days") {
	  if (tre.data->bounded.first_valid_datetime.year() > 0) {
	    auto total_days=tre.data->bounded.last_valid_datetime.days_since(tre.data->bounded.first_valid_datetime,time_data.calendar);
	    auto num_days=total_days/tre.data->num_steps;
	    switch (num_days) {
		case 1: {
		  time_range="Daily ";
		  break;
		}
		case 5: {
		  time_range="Pentad ";
		  break;
		}
		case 30:
		case 31: {
		  if (tre.data->bounded.last_valid_datetime.years_since(tre.data->bounded.first_valid_datetime) > 0) {
		    time_range="Monthly ";
		  }
		  break;
		}
		case 365: {
		  time_range="Annual ";
		  break;
		}
		default: {
		  time_range=strutils::itos(num_days)+"-day ";
		}
	    }
	  }
	  else {
	    time_range="Daily ";
	  }
	}
	else if (time_data.units == "months") {
	  time_range="Monthly ";
	}
	else {
	  error="gridded_netcdf_time_range_description(): don't understand time units '"+time_data.units+"' for cell method '"+time_method+"'";
	}
	time_range+=strutils::to_capital(time_method);
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
  else if (!time_method.empty()) {
    auto lmethod=strutils::to_lower(time_method);
    if (tre.key == 0x7fffffff) {
	time_range="All-year Climatology of ";
    }
    else {
	time_range=strutils::itos(tre.key)+"-year Climatology of ";
    }
    if (std::regex_search(lmethod,std::regex("mean over years$"))) {
	auto idx=lmethod.find(" within");
	if (idx != std::string::npos) {
	  auto product=strutils::capitalize(lmethod.substr(0,idx)+"s");
	  switch (tre.data->unit) {
	    case 1: {
		time_range+="Monthly "+product;
		break;
	    }
	    case 2: {
		time_range+="Seasonal "+product;
		break;
	    }
	    case 3: {
		time_range+="Annual "+product;
		break;
	    }
	  }
	}
	else {
	  time_range+="??";
	}
    }
    else {
	time_range+="??";
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
  if (!cell_methods.empty() && strutils::contains(cell_methods,timeid+": ")) {
    auto idx=cell_methods.find(timeid+": ");
    if (idx == std::string::npos) {
	return "";
    }
    if (idx != 0) {
	cell_methods=cell_methods.substr(idx);
    }
    strutils::replace_all(cell_methods,timeid+": ","");
    strutils::trim(cell_methods);
    if ( (idx=cell_methods.find(": ")) != std::string::npos) {
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

std::string write_level_map(const LevelInfo& level_info)
{
  std::string format;
  if (args.data_format == "hdf5nc4") {
    format="netCDF4";
  }
  else {
    format="netCDF";
  }
  LevelMap level_map;
  std::list<std::string> map_contents;
  std::string sdum=unixutils::remote_web_file("https://rda.ucar.edu/metadata/LevelTables/"+format+".ds"+args.dsnum+".xml",directives.temp_path);
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
  std::ofstream ofs((tdir.name()+"/"+format+".ds"+args.dsnum+".xml").c_str());
  if (!ofs.is_open()) {
    return "can't open "+tdir.name()+"/"+format+".ds"+args.dsnum+".xml file for writing netCDF levels";
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
  if (unixutils::rdadata_sync(tdir.name(),".","/data/web/metadata/LevelTables",directives.rdadata_home,herror) < 0) {
    return herror;
  }
  std::stringstream output,error;
  unixutils::mysystem2("/bin/cp "+tdir.name()+"/"+format+".ds"+args.dsnum+".xml "+directives.rdadata_home+"/share/metadata/LevelTables/",output,error);
  return error.str();
}

} // end namespace NcLevel

namespace NcParameter {

std::string write_parameter_map(std::list<std::string>& varlist,my::map<metautils::StringEntry>& var_changes_table,std::string map_type,std::string map_name,bool found_map,std::string& warning)
{
  std::stringstream oss,ess;
  if (varlist.size() > 0) {
    std::list<std::string> map_contents;
    if (found_map) {
	std::ifstream ifs(map_name.c_str());
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
	return "can't create temporary directory for parameter map";
    }
    std::string format;
    if (args.data_format == "hdf5nc4") {
	format="netCDF4";
    }
    else if (args.data_format == "hdf5") {
	format="HDF5";
    }
    else {
	format="netCDF";
    }
    std::ofstream ofs((tdir.name()+"/"+format+".ds"+args.dsnum+".xml").c_str());
    if (!ofs.is_open()) {
        return "can't open parameter map file for output";
    }
    if (!found_map) {
	ofs << "<?xml version=\"1.0\" ?>" << std::endl;
	ofs << "<" << map_type << "Map>" << std::endl;
    }
    else {
	auto no_write=false;
	for (const auto& line : map_contents) {
	  if (strutils::contains(line," code=\"")) {
	    auto parts=strutils::split(line,"\"");
	    metautils::StringEntry se;
	    se.key=parts[1];
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
    for (const auto& item : varlist) {
	auto var_parts=strutils::split(item,"<!>");
	if (map_type == "parameter") {
	  ofs << "  <parameter code=\"" << var_parts[0] << "\">" << std::endl;
	  ofs << "    <shortName>" << var_parts[0] << "</shortName>" << std::endl;
	  if (!var_parts[1].empty()) {
	    ofs << "    <description>" << var_parts[1] << "</description>" << std::endl;
	  }
	  if (!var_parts[2].empty()) {
	    var_parts[2]=std::regex_replace(var_parts[2],std::regex("\\*\\*"),"^");
	    var_parts[2]=std::regex_replace(var_parts[2],std::regex("([a-z]{1,})(-){0,1}([0-9]{1,})"),"$1^$2$3");
	    ofs << "    <units>" << var_parts[2] << "</units>" << std::endl;
	  }
	  if (var_parts.size() > 3 && !var_parts[3].empty()) {
	    ofs << "    <standardName>" << var_parts[3] << "</standardName>" << std::endl;
	  }
	  ofs << "  </parameter>" << std::endl;
	}
	else if (map_type == "dataType") {
	  ofs << "  <dataType code=\"" << var_parts[0] << "\">" << std::endl;
	  ofs << "    <description>" << var_parts[1];
	  if (!var_parts[2].empty()) {
	    ofs << " (" << var_parts[2] << ")";
	  }
	  ofs << "</description>" << std::endl;
	  ofs << "  </dataType>" << std::endl;
	}
    }
    ofs << "</" << map_type << "Map>" << std::endl;
    ofs.close();
    std::string error;
    if (unixutils::rdadata_sync(tdir.name(),".","/data/web/metadata/ParameterTables",directives.rdadata_home,error) < 0) {
        warning="parameter map was not synced - error(s): '"+error+"'";
    }
    unixutils::mysystem2("/bin/cp "+tdir.name()+"/"+format+".ds"+args.dsnum+".xml "+directives.rdadata_home+"/share/metadata/ParameterTables/",oss,ess);
  }
  return ess.str();
}

} // end namespace NcParameter

} // end namespace metautils
