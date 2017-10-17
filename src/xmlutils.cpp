#include <regex>
#include <sys/stat.h>
#include <xml.hpp>
#include <xmlutils.hpp>
#include <tempfile.hpp>
#include <strutils.hpp>

std::string ParameterMap::keyword(size_t keyword_code,std::string parameter_code,std::string level_type,std::string level_value)
{
  PMapEntry pme;
  pme.key=parameter_code;
  if (parameter_map_table.found(pme.key,pme)) {
    AttributeEntry ae;
    if (!level_type.empty()) {
	ae.key=level_type;
    }
    else {
	ae.key="x";
    }
    if (!level_value.empty()) {
	ae.key+="[!]"+level_value;
    }
    my::map<AttributeEntry> *t;
    switch (keyword_code) {
	case CF_code:
	{
	  t=&pme.CF;
	  break;
	}
	case GCMD_code:
	{
	  t=&pme.GCMD;
	  break;
	}
	case ISO_code:
	{
	  t=&pme.ISO;
	  break;
	}
	default:
	{
	  return "";
	}
    }
    if (t->found(ae.key,ae)) {
	return ae.value;
    }
    else {
	if (!level_value.empty()) {
	  if (t->found(level_type,ae)) {
	    return ae.value;
	  }
	}
	t->found("x",ae);
	return ae.value;
    }
  }
  else {
    return "";
  }
}

std::string ParameterMap::comment(std::string parameter_code)
{
  PMapEntry pme;
  pme.key=parameter_code;
  if (parameter_map_table.found(pme.key,pme)) {
    return pme.comment_;
  }
  else {
    return "";
  }
}

std::string ParameterMap::description(std::string parameter_code)
{
  PMapEntry pme;
  pme.key=parameter_code;
  if (parameter_map_table.found(pme.key,pme)) {
    return pme.description_;
  }
  else {
    return "";
  }
}

std::string ParameterMap::long_name(std::string parameter_code)
{
  PMapEntry pme;
  pme.key=parameter_code;
  if (parameter_map_table.found(pme.key,pme)) {
    return pme.long_name_;
  }
  else {
    return "";
  }
}

std::string ParameterMap::short_name(std::string parameter_code)
{
  PMapEntry pme;
  pme.key=parameter_code;
  if (parameter_map_table.found(pme.key,pme)) {
    return pme.short_name_;
  }
  else {
    return "";
  }
}

std::string ParameterMap::standard_name(std::string parameter_code)
{
  PMapEntry pme;
  pme.key=parameter_code;
  if (parameter_map_table.found(pme.key,pme)) {
    return pme.standard_name_;
  }
  else {
    return "";
  }
}

std::string ParameterMap::units(std::string parameter_code)
{
  PMapEntry pme;
  pme.key=parameter_code;
  if (parameter_map_table.found(pme.key,pme)) {
    return pme.units_;
  }
  else {
    return "";
  }
}

bool ParameterMap::fill(std::string xml_file)
{
  XMLDocument xdoc(xml_file);
  if (!xdoc) {
    return false;
  }
  auto e=xdoc.element("parameterMap");
  if (e.element_count() == 0) {
    return false;
  }
  parameter_map_table.clear();
  for (const auto& elem : e.element_addresses()) {
    if (elem.p->name() == "parameter") {
	PMapEntry pme;
	pme.reset();
	pme.key=elem.p->attribute_value("code");
	for (const auto& param_elem : elem.p->element_addresses()) {
	  AttributeEntry ae;
	  if (param_elem.p->name() == "shortName") {
	    pme.short_name_=param_elem.p->content();
	  }
	  else if (param_elem.p->name() == "description") {
	    pme.description_=param_elem.p->content();
	  }
	  else if (param_elem.p->name() == "units") {
	    pme.units_=param_elem.p->content();
	  }
	  else if (param_elem.p->name() == "comment") {
	    pme.comment_=param_elem.p->content();
	  }
	  else if (param_elem.p->name() == "standardName") {
	    pme.standard_name_=param_elem.p->content();
	  }
	  else if (param_elem.p->name() == "longName") {
	    pme.long_name_=param_elem.p->content();
	  }
	  else if (param_elem.p->name() == "GCMD") {
	    ae.key=param_elem.p->attribute_value("ifLevelType");
	    if (ae.key.empty()) {
		ae.key="x";
	    }
	    auto if_level_value=param_elem.p->attribute_value("ifLevelValue");
	    if (!if_level_value.empty()) {
		ae.key+="[!]"+if_level_value;
	    }
	    ae.value=param_elem.p->content();
	    pme.GCMD.insert(ae);
	  }
	  else if (param_elem.p->name() == "CF") {
	    ae.key=param_elem.p->attribute_value("ifLevelType");
	    if (ae.key.empty()) {
		ae.key="x";
	    }
	    ae.value=param_elem.p->content();
	    pme.CF.insert(ae);
	  }
	  else if (param_elem.p->name() == "ISO") {
	    ae.key=param_elem.p->attribute_value("ifLevelType");
	    if (ae.key.empty()) {
		ae.key="x";
	    }
	    ae.value=param_elem.p->content();
	    pme.ISO.insert(ae);
	  }
	}
	parameter_map_table.insert(pme);
    }
  }
  e=xdoc.element("parameterMap/title");
  title_=e.content();
  xdoc.close();
  return true;
}

std::string LevelMap::description(std::string level_type)
{
  LMapEntry lme;
  lme.key=level_type;
  if (level_map_table.found(lme.key,lme)) {
    return lme.description_;
  }
  else {
    return "";
  }
}

std::string LevelMap::short_name(std::string level_type)
{
  LMapEntry lme;
  lme.key=level_type;
  if (level_map_table.found(lme.key,lme)) {
    return lme.short_name_;
  }
  else {
    return "";
  }
}

std::string LevelMap::units(std::string level_type)
{
  LMapEntry lme;
  lme.key=level_type;
  if (level_map_table.found(lme.key,lme)) {
    return lme.units_;
  }
  else {
    return "";
  }
}

int LevelMap::is_layer(std::string level_type)
{
  LMapEntry lme;
  lme.key=level_type;
  if (level_map_table.found(lme.key,lme)) {
    return (lme.is_layer_) ? 1 : 0;
  }
  else {
    return -1;
  }
}

bool LevelMap::fill(std::string xml_file)
{
  XMLDocument xdoc(xml_file);
  if (!xdoc) {
    return false;
  }
  auto e=xdoc.element("levelMap");
  if (e.element_count() == 0) {
    return false;
  }
  level_map_table.clear();
  for (const auto& elem : e.element_addresses()) {
    if (elem.p->element_count() > 0) {
	LMapEntry lme;
	lme.reset();
	lme.key=elem.p->attribute_value("code");
	if (elem.p->name() == "layer") {
	  lme.is_layer_=true;
	}
	else {
	  lme.is_layer_=false;
	}
	for (const auto& level_elem : elem.p->element_addresses()) {
	  if (level_elem.p->name() == "shortName") {
	    lme.short_name_=level_elem.p->content();
	  }
	  else if (level_elem.p->name() == "description") {
	    lme.description_=level_elem.p->content();
	  }
	  else if (level_elem.p->name() == "units") {
	    lme.units_=level_elem.p->content();
	  }
	}
	level_map_table.insert(lme);
    }
  }
  xdoc.close();
  return true;
}

bool DataTypeMap::fill(const std::string& xml_file)
{
  XMLDocument xdoc(xml_file);
  if (!xdoc) {
    return false;
  }
  auto e=xdoc.element("dataTypeMap");
  if (e.element_count() == 0) {
    return false;
  }
  datatype_map_table.clear();
  for (const auto& elem : e.element_addresses()) {
    if (elem.p->name() == "dataType") {
	DTMapEntry dtme;
	dtme.reset();
	dtme.key=elem.p->attribute_value("code");
	for (const auto& dt_elem : elem.p->element_addresses()) {
	  AttributeEntry ae;
	  if (dt_elem.p->name() == "description") {
	    ae.key=dt_elem.p->attribute_value("ifPlatformType");
	    if (ae.key.length() == 0) {
		ae.key="x";
	    }
	    ae.value=dt_elem.p->content();
	    dtme.description_.insert(ae);
	  }
	  else if (dt_elem.p->name() == "GCMD") {
	    dtme.GCMD_list.emplace_back(dt_elem.p->content());
	  }
	  else if (dt_elem.p->name() == "ISO") {
	    dtme.ISO_topic_list.emplace_back(dt_elem.p->content());
	  }
	  else if (dt_elem.p->name() == "variable") {
	    dtme.variable_list.emplace_back(dt_elem.p->content());
	  }
	}
	datatype_map_table.insert(dtme);
    }
  }
  xdoc.close();
  return true;
}

std::string DataTypeMap::description(const std::string& datatype_code,std::string platform_type)
{
  DTMapEntry dtme;
  dtme.key=datatype_code;
  if (datatype_map_table.found(dtme.key,dtme)) {
    AttributeEntry ae;
    if (!platform_type.empty()) {
	ae.key=platform_type;
    }
    else {
	ae.key="x";
    }
    if (dtme.description_.found(ae.key,ae)) {
	return ae.value;
    }
    else {
	return "";
    }
  }
  else {
    return "";
  }
}

std::list<std::string> DataTypeMap::GCMD_keywords(const std::string& datatype_code)
{
  std::list<std::string> empty_list;
  DTMapEntry dtme;
  dtme.key=datatype_code;
  if (datatype_map_table.found(dtme.key,dtme)) {
    return dtme.GCMD_list;
  }
  else {
    return empty_list;
  }
}

std::list<std::string> DataTypeMap::ISO_topic_keywords(const std::string& datatype_code)
{
  std::list<std::string> empty_list;
  DTMapEntry dtme;
  dtme.key=datatype_code;
  if (datatype_map_table.found(dtme.key,dtme)) {
    return dtme.ISO_topic_list;
  }
  else {
    return empty_list;
  }
}

std::list<std::string> DataTypeMap::variables(const std::string& datatype_code)
{
  std::list<std::string> empty_list;
  DTMapEntry dtme;
  dtme.key=datatype_code;
  if (datatype_map_table.found(dtme.key,dtme)) {
    return dtme.variable_list;
  }
  else {
    return empty_list;
  }
}

bool GridMap::fill(const std::string& xml_file)
{
  XMLDocument xdoc(xml_file);
  if (!xdoc) {
    return false;
  }
  auto e=xdoc.element("gridMap");
  if (e.element_count() == 0) {
    return false;
  }
  grid_map_table.clear();
  for (const auto& elem : e.element_addresses()) {
    if (elem.p->name() == "grid") {
	GMapEntry gme;
	gme.reset();
	gme.key=elem.p->attribute_value("code");
	for (const auto& g_elem : elem.p->element_addresses()) {
	  if (g_elem.p->name() == "type") {
	    gme.def.type=std::stoi(g_elem.p->content());
	  }
	  else if (g_elem.p->name() == "numX") {
	    gme.dim.x=std::stoi(g_elem.p->content());
	  }
	  else if (g_elem.p->name() == "numY") {
	    gme.dim.y=std::stoi(g_elem.p->content());
	  }
	  else if (g_elem.p->name() == "size") {
	    gme.dim.size=std::stoi(g_elem.p->content());
	  }
	  else if (g_elem.p->name() == "startLat") {
	    gme.def.slatitude=std::stof(g_elem.p->content());
	  }
	  else if (g_elem.p->name() == "startLon") {
	    gme.def.slongitude=std::stof(g_elem.p->content());
	  }
	  else if (g_elem.p->name() == "endLat") {
	    gme.def.elatitude=std::stof(g_elem.p->content());
	  }
	  else if (g_elem.p->name() == "endLon") {
	    gme.def.elongitude=std::stof(g_elem.p->content());
	  }
	  else if (g_elem.p->name() == "xRes") {
	    gme.def.loincrement=std::stof(g_elem.p->content());
	  }
	  else if (g_elem.p->name() == "yRes") {
	    gme.def.laincrement=std::stof(g_elem.p->content());
	  }
	  else if (g_elem.p->name() == "singlePolePoint") {
	    if (g_elem.p->attribute_value("exists") == "true") {
		gme.single_pole_point=true;
	    }
	    gme.pole_point_location_=g_elem.p->attribute_value("location");
	  }
	}
	grid_map_table.insert(gme);
    }
  }
  xdoc.close();
  return true;
}

Grid::GridDefinition GridMap::grid_definition(const std::string& grid_code)
{
  Grid::GridDefinition def;
  GMapEntry gme;
  if (grid_map_table.found(grid_code,gme)) {
    return gme.def;
  }
  else {
    def.type=-1;
    return def;
  }
}

Grid::GridDimensions GridMap::grid_dimensions(const std::string& grid_code)
{
  Grid::GridDimensions dim;
  GMapEntry gme;
  if (grid_map_table.found(grid_code,gme)) {
    return gme.dim;
  }
  else {
    dim.x=dim.y=0;
    dim.size=0;
    return dim;
  }
}

bool GridMap::is_single_pole_point(const std::string& grid_code)
{
  GMapEntry gme;
  if (grid_map_table.found(grid_code,gme)) {
    return gme.single_pole_point;
  }
  else {
    return false;
  }
}

std::string GridMap::pole_point_location(const std::string& grid_code)
{
  GMapEntry gme;
  if (grid_map_table.found(grid_code,gme)) {
    return gme.pole_point_location_;
  }
  else {
    return "";
  }
}

namespace xmlutils {

void clean_parameter_code(std::string& parameter_code,std::string& parameter_map)
{
  if (strutils::contains(parameter_code,"@")) {
    parameter_code=parameter_code.substr(0,parameter_code.find("@"));
  }
  auto idx=parameter_code.find(":");
  if (idx != std::string::npos) {
    parameter_map=parameter_code.substr(0,idx);
    parameter_code=parameter_code.substr(idx+1);
  }
}

std::string map_filename(std::string directory,std::string file_base_name,std::string temp_dir_name)
{
  struct stat buf;
  std::string map_filename;
  if (stat(("/glade/u/home/rdadata/share/metadata/"+directory+"/"+file_base_name+".xml").c_str(),&buf) == 0) {
    map_filename="/glade/u/home/rdadata/share/metadata/"+directory+"/"+file_base_name+".xml";
  }
  else if (stat(("/usr/local/www/server_root/web/metadata/"+directory+"/"+file_base_name+".xml").c_str(),&buf) == 0) {
    map_filename="/usr/local/www/server_root/web/metadata/"+directory+"/"+file_base_name+".xml";
  }
  return map_filename;
}

ParameterMapEntry ParameterMapper::entry(std::string format,std::string parameter_map)
{
  ParameterMapEntry pme;
  pme.key=format;
  if (parameter_map.length() > 0) {
    pme.key+="."+parameter_map;
  }
  if (!parameter_map_table->found(pme.key,pme)) {
    TempDir temp_dir;
    temp_dir.create("/tmp");
    auto map_name=map_filename("ParameterTables",pme.key,temp_dir.name());
    pme.p.reset(new ParameterMap);
    if (pme.p->fill(map_name)) {
	ParameterMapEntry pme2;
	if (!parameter_map_table->found(pme.key,pme2)) {
	  parameter_map_table->insert(pme);
	}
    }
    else {
	pme.key=format;
	if (!parameter_map_table->found(pme.key,pme)) {
	  map_name=map_filename("ParameterTables",pme.key,temp_dir.name());
	  pme.p.reset(new ParameterMap);
	  if (pme.p->fill(map_name)) {
	    ParameterMapEntry pme2;
	    if (!parameter_map_table->found(pme.key,pme2)) {
		parameter_map_table->insert(pme);
	    }
	  }
	}
    }
  }
  return pme;
}

std::string ParameterMapper::CF_keyword(std::string format,std::string parameter_code,std::string level_type)
{
  std::string parameter_map;
  clean_parameter_code(parameter_code,parameter_map);
  return entry(format,parameter_map).p->CF_keyword(parameter_code,level_type);
}

std::string ParameterMapper::comment(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  clean_parameter_code(parameter_code,parameter_map);
  return entry(format,parameter_map).p->comment(parameter_code);
}

std::string ParameterMapper::description(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  clean_parameter_code(parameter_code,parameter_map);
  return entry(format,parameter_map).p->description(parameter_code);
}

std::string ParameterMapper::title(std::string format,std::string parameter_map)
{
  return entry(format,parameter_map).p->title();
}

std::string ParameterMapper::long_name(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  clean_parameter_code(parameter_code,parameter_map);
  return entry(format,parameter_map).p->long_name(parameter_code);
}

std::string ParameterMapper::short_name(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  clean_parameter_code(parameter_code,parameter_map);
  return entry(format,parameter_map).p->short_name(parameter_code);
}

std::string ParameterMapper::standard_name(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  clean_parameter_code(parameter_code,parameter_map);
  return entry(format,parameter_map).p->standard_name(parameter_code);
}

std::string ParameterMapper::units(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  clean_parameter_code(parameter_code,parameter_map);
  return entry(format,parameter_map).p->units(parameter_code);
}

LevelMapEntry LevelMapper::entry(std::string format,std::string level_type,std::string level_map)
{
  LevelMapEntry lme;
  lme.key=format;
  if (level_map.length() > 0) {
    lme.key+="."+level_map;
  }
  if (!level_map_table->found(lme.key,lme)) {
    TempDir temp_dir;
    temp_dir.create("/tmp");
    auto map_name=map_filename("LevelTables",lme.key,temp_dir.name());
    lme.l.reset(new LevelMap);
    if (lme.l->fill(map_name)) {
	LevelMapEntry lme2;
	if (!level_map_table->found(lme.key,lme2)) {
	  level_map_table->insert(lme);
	}
    }
    else {
	lme.key=format;
	if (!level_map_table->found(lme.key,lme)) {
	  map_name=map_filename("LevelTables",lme.key,temp_dir.name());
	  lme.l.reset(new LevelMap);
	  if (lme.l->fill(map_name)) {
	    LevelMapEntry lme2;
	    if (!level_map_table->found(lme.key,lme2)) {
		level_map_table->insert(lme);
	    }
	  }
	}
    }
  }
  return lme;
}

std::string LevelMapper::description(std::string format,std::string level_type,std::string level_map)
{
  if (strutils::contains(level_type,"-")) {
    auto level_types=strutils::split(level_type,"-");
    auto d1=entry(format,level_type,level_map).l->description(level_types[0]);
    auto d2=entry(format,level_type,level_map).l->description(level_types[1]);
    if (d1 == d2) {
	return "Layer between two '"+d1+"'";
    }
    else {
	return "Layer between "+d1+" and "+d2;
    }
  }
  else {
    return entry(format,level_type,level_map).l->description(level_type);
  }
}

std::string LevelMapper::short_name(std::string format,std::string level_type,std::string level_map)
{
  auto level_types=strutils::split(level_type,"-");
  return entry(format,level_type,level_map).l->short_name(level_types[0]);
}

std::string LevelMapper::units(std::string format,std::string level_type,std::string level_map)
{ 
  return entry(format,level_type,level_map).l->units(level_type);
}

int LevelMapper::level_is_a_layer(std::string format,std::string level_type,std::string level_map)
{
  return entry(format,level_type,level_map).l->is_layer(level_type);
}

DataTypeMapEntry DataTypeMapper::entry(std::string format,std::string dsnum)
{
  DataTypeMapEntry dtme;
  dtme.key=format+"."+dsnum;
  if (!datatype_map_table->found(dtme.key,dtme)) {
    TempDir temp_dir;
    temp_dir.create("/tmp");
    auto map_name=map_filename("ParameterTables",dtme.key,temp_dir.name());
    dtme.d.reset(new DataTypeMap);
    if (dtme.d->fill(map_name)) {
	DataTypeMapEntry dtme2;
	if (!datatype_map_table->found(dtme.key,dtme2)) {
	  datatype_map_table->insert(dtme);
	}
    }
    else {
	dtme.key=format;
	if (!datatype_map_table->found(dtme.key,dtme)) {
	  map_name=map_filename("ParameterTables",dtme.key,temp_dir.name());
	  dtme.d.reset(new DataTypeMap);
	  if (dtme.d->fill(map_name)) {
	    DataTypeMapEntry dtme2;
	    if (!datatype_map_table->found(dtme.key,dtme2)) {
		datatype_map_table->insert(dtme);
	    }
	  }
	}
    }
  }
  return dtme;
}

std::string DataTypeMapper::description(std::string format,std::string dsnum,std::string data_type_code,std::string platform_type)
{
  return entry(format,dsnum).d->description(data_type_code,platform_type);
}

std::list<std::string> DataTypeMapper::GCMD_keywords(std::string format,std::string dsnum,std::string data_type_code)
{ 
  return entry(format,dsnum).d->GCMD_keywords(data_type_code);
}

std::list<std::string> DataTypeMapper::variables(std::string format,std::string dsnum,std::string data_type_code)
{ 
  return entry(format,dsnum).d->variables(data_type_code);
}

std::deque<std::string> split(const std::string& s)
{
  std::deque<std::string> parts;
  size_t pos=0;
  auto bindex=s.find("<",pos);
  if (bindex != 0) {
    return parts;
  }
  while (pos < s.length() && bindex != std::string::npos) {
    if ( (bindex-pos) == 0) {
	size_t eindex;
        if ( (eindex=s.find(">",pos)) == std::string::npos) {
          pos=s.length();
        }
        else {
          pos=eindex+1;
        }
        parts.push_back(s.substr(bindex,(pos-bindex)));
        bindex=s.find("<",pos);
    }
    else {
        parts.push_back(s.substr(pos,(bindex-pos)));
        pos=bindex;
    }
  }
  return parts;
}

/*
std::string copy_of(XMLElement& e,size_t indent)
{
  static std::string copyString;
  static int local_indent=-2;
  if (e.name().length() == 0) {
    return "";
  }
  if (local_indent < 0) {
    copyString="";
  }
  local_indent+=2;
  int total_indent=local_indent+indent;
  for (int n=0; n < total_indent; ++n) {
    copyString+=" ";
  }
  copyString+="<"+e.name();
  for (auto& attribute : e.attribute_list() ) {
    copyString+=" "+attribute.name+"=\""+attribute.value+"\"";
  }
  if (e.element_addresses().size() > 0 || e.content().length() > 0) {
    copyString+=">";
    if (e.content().length() > 0) {
	copyString+=e.content();
    }
    else {
	for (auto& address : e.element_addresses() ) {
	  copyString+="\n";
	  copy_of(*address.p,indent);
	}
    }
    if (e.element_count() > 0) {
	copyString+="\n";
	for (int n=0; n < total_indent; ++n) {
	  copyString+=" ";
	}
    }
    int n=copyString.length();
    while (n > 0 && copyString[n-1] == ' ') {
	--n;
    }
    n=copyString.length()-n;
    while (n > total_indent) {
	strutils::chop(copyString);
	--n;
    }
    copyString+="</"+e.name()+">";
  }
  else {
    copyString+=" />";
  }
  local_indent-=2;
  return copyString;
}
*/

std::string to_plain_text(const XMLElement& e,size_t wrap_width,size_t indent_width)
{
  static size_t depth=0;
  ++depth;
  auto ptext=e.to_string();
  auto ptext_l=strutils::to_lower(ptext);
  for (const auto& address : e.element_addresses()) {
    auto iwidth= (depth > 1) ? indent_width : 0;
    if (std::regex_search(ptext_l,std::regex("^<ul[ >]"))) {
	iwidth+=3;
    }
    auto ctext=to_plain_text(*address.p,wrap_width,iwidth);
    --depth;
    strutils::replace_all(ptext,address.p->to_string(),ctext);
  }
  if (std::regex_search(ptext_l,std::regex("^<p[ >]"))) {
    strutils::replace_all(ptext,"</"+ptext.substr(1,1)+">","\n\n");
    ptext=ptext.substr(ptext.find(">")+1);
  }
  else if (std::regex_search(ptext_l,std::regex("^<a (.*)href="))) {
    auto url=ptext.substr(ptext.find("href=")+6);
    size_t idx;
    if ( (idx=url.find("\"")) == std::string::npos) {
	idx=url.find("'");
    }
    url=url.substr(0,idx);
    auto link_text=ptext.substr(ptext.find(">")+1);
    link_text=link_text.substr(0,link_text.find("<"));
    ptext=link_text+" ["+url+"]";
  }
  else if (std::regex_search(ptext_l,std::regex("^<li[ >]"))) {
    ptext=std::string(indent_width,' ')+" * "+ptext.substr(ptext.find(">")+1);
    ptext=ptext.substr(0,ptext.find("<"))+"\n";
  }
  else if (std::regex_search(ptext_l,std::regex("^<ul[ >]"))) {
    ptext="\n"+ptext.substr(ptext.find(">")+1);
    ptext=ptext.substr(0,ptext.find("<"))+"\n";
  }
  else if (std::regex_search(ptext_l,std::regex("^<font[ >]"))) {
    ptext=ptext.substr(ptext.find(">")+1);
    ptext=ptext.substr(0,ptext.find("<"))+"\n";
  }
  if (depth == 1) {
    auto idx1=ptext.find(" ");
    auto idx2=ptext.find(">");
    auto tagname= (idx1 < idx2) ? ptext.substr(1,idx1-1) : ptext.substr(1,idx2-1);
    ptext=ptext.substr(idx2+1);
    strutils::replace_all(ptext,"</"+tagname+">","");
    strutils::trim(ptext);
    strutils::replace_all(ptext,"&nbsp;"," ");
    strutils::replace_all(ptext,"&deg;","degree");
    strutils::replace_all(ptext,"&lt;","less than");
    strutils::replace_all(ptext,"&gt;","greater than");
    std::regex r=std::regex("\\n ");
    while (std::regex_search(ptext,r)) {
	strutils::replace_all(ptext,"\n ","\n");
    }
    ptext=strutils::wrap(ptext,wrap_width,indent_width);
  }
  return ptext;
}

} // end namespace xmlutils
