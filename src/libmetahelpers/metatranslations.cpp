#include <sys/stat.h>
#include <strutils.hpp>
#include <utils.hpp>
#include <xml.hpp>
#include <xmlutils.hpp>

namespace metatranslations {

std::string detailed_datatype(xmlutils::DataTypeMapper& data_type_mapper,const std::string& data_format,const std::string& code)
{
  auto idx=code.find(":");
  auto detailedDataType=data_type_mapper.description(data_format,code.substr(0,idx),code.substr(idx+1));
  if (detailedDataType.length() == 0) {
    detailedDataType=data_format+":"+code;
  }
  auto vars=data_type_mapper.variables(data_format,code.substr(0,idx),code.substr(idx+1));
  if (vars.size() > 0) {
    detailedDataType+=" (may include one or more of the following)<ul>";
    for (auto& var : vars) {
	detailedDataType+="<li>"+var+"</li>";
    }
    detailedDataType+="</ul>";
  }
  return detailedDataType;
}

std::string detailed_level(xmlutils::LevelMapper& level_mapper,const std::string& data_format,const std::string& map,const std::string& type,const std::string& value,bool htmlify_units)
{
  std::string detailed_level;
  std::string units_b,units_t;
  std::deque<std::string> sp;
  size_t idx;

  detailed_level=level_mapper.description(data_format,type,map);
  if (strutils::contains(type,"-")) {
    sp=strutils::split(type,"-");
    units_b=level_mapper.units(data_format,sp[0],map);
    units_t=level_mapper.units(data_format,sp[1],map);
  }
  else {
    units_b=level_mapper.units(data_format,type,map);
    units_t=units_b;
  }
  if (units_b.length() > 0 && htmlify_units) {
    units_b=htmlutils::transform_superscripts(units_b);
  }
  if (units_t.length() > 0 && htmlify_units) {
    units_t=htmlutils::transform_superscripts(units_t);
  }
  if (strutils::contains(value,",")) {
    if (value != "0,0") {
	idx=value.find(",");
	detailed_level+=": Bottom="+value.substr(0,idx)+" "+units_b+", Top="+value.substr(idx+1)+" "+units_t;
    }
  }
  else {
    if (value != "0" || units_b.length() > 0) {
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

} // end namespace metatranslations
