#include <sys/stat.h>
#include <strutils.hpp>
#include <utils.hpp>
#include <xml.hpp>
#include <xmlutils.hpp>

namespace metatranslations {

std::string date_time(std::string database_date_time,std::string database_flag,std::string time_zone,std::string time_delimiter)
{
  auto dt=database_date_time;
  auto flag=std::stoi(database_flag);
  switch (flag) {
    case 1:
    {
	dt=dt.substr(0,4);
	break;
    }
    case 2:
    {
	dt=dt.substr(0,7);
	break;
    }
    case 3:
    {
	dt=dt.substr(0,10);
	break;
    }
    case 4:
    {
	if (!time_delimiter.empty()) {
	  dt=dt.substr(0,10)+time_delimiter+dt.substr(11,2);
	}
	else {
	  dt=dt.substr(0,13);
	}
	dt+=" "+time_zone;
	break;
    }
    case 5:
    {
	if (!time_delimiter.empty()) {
	  dt=dt.substr(0,10)+time_delimiter+dt.substr(11,5);
	}
	else {
	  dt=dt.substr(0,16);
	}
	dt+=" "+time_zone;
	break;
    }
    case 6:
    {
	if (!time_delimiter.empty()) {
	  dt=dt.substr(0,10)+time_delimiter+dt.substr(11);
	}
	else {
	  dt+=" "+time_zone;
	}
	break;
    }
  }
  return dt;
}

std::string detailed_datatype(xmlutils::DataTypeMapper& data_type_mapper,const std::string& data_format,const std::string& code)
{
  auto idx=code.find(":");
  auto detailed_datatype=data_type_mapper.description(data_format,code.substr(0,idx),code.substr(idx+1));
  if (detailed_datatype.empty()) {
    detailed_datatype=data_format+":"+code;
  }
  auto vars=data_type_mapper.variables(data_format,code.substr(0,idx),code.substr(idx+1));
  if (vars.size() > 0) {
    detailed_datatype+=" (may include one or more of the following)<ul>";
    for (auto& var : vars) {
	detailed_datatype+="<li>"+var+"</li>";
    }
    detailed_datatype+="</ul>";
  }
  return detailed_datatype;
}

std::string detailed_level(xmlutils::LevelMapper& level_mapper,const std::string& data_format,const std::string& map,const std::string& type,const std::string& value,bool htmlify_units)
{
  auto detailed_level=level_mapper.description(data_format,type,map);
  std::string units_b,units_t;
  if (strutils::contains(type,"-")) {
    auto sp=strutils::split(type,"-");
    units_b=level_mapper.units(data_format,sp[0],map);
    units_t=level_mapper.units(data_format,sp[1],map);
  }
  else {
    units_b=level_mapper.units(data_format,type,map);
    units_t=units_b;
  }
  if (!units_b.empty() && htmlify_units) {
    units_b=htmlutils::transform_superscripts(units_b);
  }
  if (!units_t.empty() && htmlify_units) {
    units_t=htmlutils::transform_superscripts(units_t);
  }
  if (strutils::contains(value,",")) {
    if (value != "0,0") {
	auto idx=value.find(",");
	detailed_level+=": Bottom="+value.substr(0,idx)+" "+units_b+", Top="+value.substr(idx+1)+" "+units_t;
    }
  }
  else {
    if (value != "0" || !units_b.empty()) {
	detailed_level+=": "+value+" "+units_b;
    }
  }
  return detailed_level;
}

std::string detailed_parameter(xmlutils::ParameterMapper& parameter_mapper,const std::string& data_format,const std::string& code)
{
  auto detailed_parameter=parameter_mapper.description(data_format,code);
  if (detailed_parameter.empty()) {
    detailed_parameter=data_format+":"+code;
  }
  auto units=parameter_mapper.units(data_format,code);
  if (!units.empty()) {
    detailed_parameter+=" <small class=\"units\">("+htmlutils::transform_superscripts(units)+")</small>";
  }
  return detailed_parameter;
}

std::string string_coordinate_to_db(std::string coordinate_value)
{
  std::string s=strutils::substitute(coordinate_value,".","");
  auto idx=coordinate_value.find(".");
  if (idx == std::string::npos) {
    s.append(4,'0');
  }
  else {
    auto x=coordinate_value.length()-idx;
    if (x > 4) {
        s=s.substr(0,idx+4);
    }
    else {
        s.append(5-x,'0');
    }
  }
  return s;
}

} // end namespace metatranslations
