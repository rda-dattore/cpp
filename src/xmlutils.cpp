#include <regex>
#include <sys/stat.h>
#include <xml.hpp>
#include <xmlutils.hpp>
#include <tempfile.hpp>
#include <strutils.hpp>

std::string ParameterMap::getKeyword(size_t keywordCode,std::string parameterCode,std::string level_type,std::string level_value)
{
  PMapEntry pme;
  AttributeEntry ae;
  my::map<AttributeEntry> *t;

  pme.key=parameterCode;
  if (parameterMapTable.found(pme.key,pme)) {
    if (level_type.length() > 0) {
	ae.key=level_type;
    }
    else {
	ae.key="x";
    }
    if (level_value.length() > 0) {
	ae.key+="[!]"+level_value;
    }
    switch (keywordCode) {
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
	if (level_value.length() > 0) {
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

std::string ParameterMap::getComment(std::string parameterCode)
{
  PMapEntry pme;
  pme.key=parameterCode;
  if (parameterMapTable.found(pme.key,pme)) {
    return pme.comment;
  }
  else {
    return "";
  }
}

std::string ParameterMap::getDescription(std::string parameterCode)
{
  PMapEntry pme;
  pme.key=parameterCode;
  if (parameterMapTable.found(pme.key,pme)) {
    return pme.description;
  }
  else {
    return "";
  }
}

std::string ParameterMap::getLongName(std::string parameterCode)
{
  PMapEntry pme;
  pme.key=parameterCode;
  if (parameterMapTable.found(pme.key,pme)) {
    return pme.long_name;
  }
  else {
    return "";
  }
}

std::string ParameterMap::getShortName(std::string parameterCode)
{
  PMapEntry pme;
  pme.key=parameterCode;
  if (parameterMapTable.found(pme.key,pme)) {
    return pme.shortName;
  }
  else {
    return "";
  }
}

std::string ParameterMap::getStandardName(std::string parameterCode)
{
  PMapEntry pme;
  pme.key=parameterCode;
  if (parameterMapTable.found(pme.key,pme)) {
    return pme.standard_name;
  }
  else {
    return "";
  }
}

std::string ParameterMap::getUnits(std::string parameterCode)
{
  PMapEntry pme;
  pme.key=parameterCode;
  if (parameterMapTable.found(pme.key,pme)) {
    return pme.units;
  }
  else {
    return "";
  }
}

bool ParameterMap::fill(std::string xmlFile)
{
  XMLDocument xdoc(xmlFile);
  if (!xdoc) {
    return false;
  }
  auto e=xdoc.element("parameterMap");
  if (e.element_count() == 0) {
    return false;
  }
  parameterMapTable.clear();
  for (const auto& elem : e.element_addresses()) {
    if (elem.p->name() == "parameter") {
	PMapEntry pme;
	pme.reset();
	pme.key=elem.p->attribute_value("code");
	for (const auto& param_elem : elem.p->element_addresses()) {
	  AttributeEntry ae;
	  if (param_elem.p->name() == "shortName") {
	    pme.shortName=param_elem.p->content();
	  }
	  else if (param_elem.p->name() == "description") {
	    pme.description=param_elem.p->content();
	  }
	  else if (param_elem.p->name() == "units") {
	    pme.units=param_elem.p->content();
	  }
	  else if (param_elem.p->name() == "comment") {
	    pme.comment=param_elem.p->content();
	  }
	  else if (param_elem.p->name() == "standardName") {
	    pme.standard_name=param_elem.p->content();
	  }
	  else if (param_elem.p->name() == "longName") {
	    pme.long_name=param_elem.p->content();
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
	parameterMapTable.insert(pme);
    }
  }
  e=xdoc.element("parameterMap/title");
  title=e.content();
  xdoc.close();
  return true;
}

std::string LevelMap::getDescription(std::string level_type)
{
  LMapEntry lme;
  lme.key=level_type;
  if (levelMapTable.found(lme.key,lme)) {
    return lme.description;
  }
  else {
    return "";
  }
}

std::string LevelMap::getShortName(std::string level_type)
{
  LMapEntry lme;
  lme.key=level_type;
  if (levelMapTable.found(lme.key,lme)) {
    return lme.shortName;
  }
  else {
    return "";
  }
}

std::string LevelMap::getUnits(std::string level_type)
{
  LMapEntry lme;
  lme.key=level_type;
  if (levelMapTable.found(lme.key,lme)) {
    return lme.units;
  }
  else {
    return "";
  }
}

int LevelMap::isLayer(std::string level_type)
{
  LMapEntry lme;
  lme.key=level_type;
  if (levelMapTable.found(lme.key,lme)) {
    return (lme.isLayer) ? 1 : 0;
  }
  else {
    return -1;
  }
}

bool LevelMap::fill(std::string xmlFile)
{
  XMLDocument xdoc(xmlFile);
  if (!xdoc) {
    return false;
  }
  auto e=xdoc.element("levelMap");
  if (e.element_count() == 0) {
    return false;
  }
  levelMapTable.clear();
  for (const auto& elem : e.element_addresses()) {
    if (elem.p->element_count() > 0) {
	LMapEntry lme;
	lme.reset();
	lme.key=elem.p->attribute_value("code");
	if (elem.p->name() == "layer") {
	  lme.isLayer=true;
	}
	else {
	  lme.isLayer=false;
	}
	for (const auto& level_elem : elem.p->element_addresses()) {
	  if (level_elem.p->name() == "shortName") {
	    lme.shortName=level_elem.p->content();
	  }
	  else if (level_elem.p->name() == "description") {
	    lme.description=level_elem.p->content();
	  }
	  else if (level_elem.p->name() == "units") {
	    lme.units=level_elem.p->content();
	  }
	}
	levelMapTable.insert(lme);
    }
  }
  xdoc.close();
  return true;
}

bool DataTypeMap::fill(const std::string& xmlFile)
{
  XMLDocument xdoc(xmlFile);
  if (!xdoc) {
    return false;
  }
  auto e=xdoc.element("dataTypeMap");
  if (e.element_count() == 0) {
    return false;
  }
  dataTypeMapTable.clear();
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
	    dtme.description.insert(ae);
	  }
	  else if (dt_elem.p->name() == "GCMD") {
	    dtme.GCMDList.emplace_back(dt_elem.p->content());
	  }
	  else if (dt_elem.p->name() == "ISO") {
	    dtme.ISOTopicList.emplace_back(dt_elem.p->content());
	  }
	  else if (dt_elem.p->name() == "variable") {
	    dtme.variableList.emplace_back(dt_elem.p->content());
	  }
	}
	dataTypeMapTable.insert(dtme);
    }
  }
  xdoc.close();
  return true;
}

std::string DataTypeMap::getDescription(const std::string& dataTypeCode,std::string platformType)
{
  DTMapEntry dtme;
  dtme.key=dataTypeCode;
  if (dataTypeMapTable.found(dtme.key,dtme)) {
    AttributeEntry ae;
    if (platformType.length() > 0) {
	ae.key=platformType;
    }
    else {
	ae.key="x";
    }
    if (dtme.description.found(ae.key,ae)) {
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

std::list<std::string> DataTypeMap::getGCMDKeywords(const std::string& dataTypeCode)
{
  std::list<std::string> empty_list;
  DTMapEntry dtme;
  dtme.key=dataTypeCode;
  if (dataTypeMapTable.found(dtme.key,dtme)) {
    return dtme.GCMDList;
  }
  else {
    return empty_list;
  }
}

std::list<std::string> DataTypeMap::getISOTopicKeywords(const std::string& dataTypeCode)
{
  std::list<std::string> empty_list;
  DTMapEntry dtme;
  dtme.key=dataTypeCode;
  if (dataTypeMapTable.found(dtme.key,dtme)) {
    return dtme.ISOTopicList;
  }
  else {
    return empty_list;
  }
}

std::list<std::string> DataTypeMap::getVariables(const std::string& dataTypeCode)
{
  std::list<std::string> empty_list;
  DTMapEntry dtme;
  dtme.key=dataTypeCode;
  if (dataTypeMapTable.found(dtme.key,dtme)) {
    return dtme.variableList;
  }
  else {
    return empty_list;
  }
}

bool GridMap::fill(const std::string& xmlFile)
{
  XMLDocument xdoc(xmlFile);
  if (!xdoc) {
    return false;
  }
  auto e=xdoc.element("gridMap");
  if (e.element_count() == 0) {
    return false;
  }
  gridMapTable.clear();
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
		gme.singlePolePoint=true;
	    }
	    gme.polePointLocation=g_elem.p->attribute_value("location");
	  }
	}
	gridMapTable.insert(gme);
    }
  }
  xdoc.close();
  return true;
}

Grid::GridDefinition GridMap::getGridDefinition(const std::string& gridCode)
{
  Grid::GridDefinition def;
  GMapEntry gme;
  if (gridMapTable.found(gridCode,gme))
    return gme.def;
  else {
    def.type=-1;
    return def;
  }
}

Grid::GridDimensions GridMap::getGridDimensions(const std::string& gridCode)
{
  Grid::GridDimensions dim;
  GMapEntry gme;
  if (gridMapTable.found(gridCode,gme))
    return gme.dim;
  else {
    dim.x=dim.y=0;
    dim.size=0;
    return dim;
  }
}

bool GridMap::isSinglePolePoint(const std::string& gridCode)
{
  GMapEntry gme;
  if (gridMapTable.found(gridCode,gme)) {
    return gme.singlePolePoint;
  }
  else {
    return false;
  }
}

std::string GridMap::getPolePointLocation(const std::string& gridCode)
{
  GMapEntry gme;
  if (gridMapTable.found(gridCode,gme)) {
    return gme.polePointLocation;
  }
  else {
    return "";
  }
}

namespace xmlutils {

void cleanParameterCode(std::string& parameter_code,std::string& parameter_map)
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

std::string getMapFilename(std::string directory,std::string file_base_name,std::string temp_dir_name)
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

ParameterMapEntry ParameterMapper::getEntry(std::string format,std::string parameter_map)
{
  ParameterMapEntry pme;
  pme.key=format;
  if (parameter_map.length() > 0) {
    pme.key+="."+parameter_map;
  }
  if (!parameter_map_table->found(pme.key,pme)) {
    TempDir temp_dir;
    temp_dir.create("/tmp");
    auto sdum=getMapFilename("ParameterTables",pme.key,temp_dir.name());
    pme.p.reset(new ParameterMap);
    if (pme.p->fill(sdum)) {
	ParameterMapEntry pme2;
	if (!parameter_map_table->found(pme.key,pme2)) {
	  parameter_map_table->insert(pme);
	}
    }
    else {
	pme.key=format;
	if (!parameter_map_table->found(pme.key,pme)) {
	  sdum=getMapFilename("ParameterTables",pme.key,temp_dir.name());
	  pme.p.reset(new ParameterMap);
	  if (pme.p->fill(sdum)) {
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

std::string ParameterMapper::getCFKeyword(std::string format,std::string parameter_code,std::string level_type)
{
  std::string parameter_map;
  cleanParameterCode(parameter_code,parameter_map);
  return getEntry(format,parameter_map).p->getCFKeyword(parameter_code,level_type);
}

std::string ParameterMapper::getComment(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  cleanParameterCode(parameter_code,parameter_map);
  return getEntry(format,parameter_map).p->getComment(parameter_code);
}

std::string ParameterMapper::getDescription(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  cleanParameterCode(parameter_code,parameter_map);
  return getEntry(format,parameter_map).p->getDescription(parameter_code);
}

std::string ParameterMapper::getTitle(std::string format,std::string parameter_map)
{
  return getEntry(format,parameter_map).p->getTitle();
}

std::string ParameterMapper::getLongName(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  cleanParameterCode(parameter_code,parameter_map);
  return getEntry(format,parameter_map).p->getLongName(parameter_code);
}

std::string ParameterMapper::getShortName(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  cleanParameterCode(parameter_code,parameter_map);
  return getEntry(format,parameter_map).p->getShortName(parameter_code);
}

std::string ParameterMapper::getStandardName(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  cleanParameterCode(parameter_code,parameter_map);
  return getEntry(format,parameter_map).p->getStandardName(parameter_code);
}

std::string ParameterMapper::getUnits(std::string format,std::string parameter_code)
{
  std::string parameter_map;
  cleanParameterCode(parameter_code,parameter_map);
  return getEntry(format,parameter_map).p->getUnits(parameter_code);
}

LevelMapEntry LevelMapper::getEntry(std::string format,std::string level_type,std::string level_map)
{
  LevelMapEntry lme;
  lme.key=format;
  if (level_map.length() > 0) {
    lme.key+="."+level_map;
  }
  if (!level_map_table->found(lme.key,lme)) {
    TempDir temp_dir;
    temp_dir.create("/tmp");
    auto sdum=getMapFilename("LevelTables",lme.key,temp_dir.name());
    lme.l.reset(new LevelMap);
    if (lme.l->fill(sdum)) {
	LevelMapEntry lme2;
	if (!level_map_table->found(lme.key,lme2)) {
	  level_map_table->insert(lme);
	}
    }
    else {
	lme.key=format;
	if (!level_map_table->found(lme.key,lme)) {
	  sdum=getMapFilename("LevelTables",lme.key,temp_dir.name());
	  lme.l.reset(new LevelMap);
	  if (lme.l->fill(sdum)) {
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

std::string LevelMapper::getDescription(std::string format,std::string level_type,std::string level_map)
{
  if (strutils::contains(level_type,"-")) {
    auto sp=strutils::split(level_type,"-");
    auto d1=getEntry(format,level_type,level_map).l->getDescription(sp[0]);
    auto d2=getEntry(format,level_type,level_map).l->getDescription(sp[1]);
    if (d1 == d2) {
	return "Layer between two '"+d1+"'";
    }
    else {
	return "Layer between "+d1+" and "+d2;
    }
  }
  else {
    return getEntry(format,level_type,level_map).l->getDescription(level_type);
  }
}

std::string LevelMapper::getShortName(std::string format,std::string level_type,std::string level_map)
{
  auto sp=strutils::split(level_type,"-");
  return getEntry(format,level_type,level_map).l->getShortName(sp[0]);
}

std::string LevelMapper::getUnits(std::string format,std::string level_type,std::string level_map)
{ 
  return getEntry(format,level_type,level_map).l->getUnits(level_type);
}

int LevelMapper::levelIsLayer(std::string format,std::string level_type,std::string level_map)
{
  return getEntry(format,level_type,level_map).l->isLayer(level_type);
}

DataTypeMapEntry DataTypeMapper::getEntry(std::string format,std::string dsnum)
{
  DataTypeMapEntry dtme;
  dtme.key=format+"."+dsnum;
  if (!data_type_map_table->found(dtme.key,dtme)) {
    TempDir temp_dir;
    temp_dir.create("/tmp");
    auto sdum=getMapFilename("ParameterTables",dtme.key,temp_dir.name());
    dtme.d.reset(new DataTypeMap);
    if (dtme.d->fill(sdum)) {
	DataTypeMapEntry dtme2;
	if (!data_type_map_table->found(dtme.key,dtme2)) {
	  data_type_map_table->insert(dtme);
	}
    }
    else {
	dtme.key=format;
	if (!data_type_map_table->found(dtme.key,dtme)) {
	  sdum=getMapFilename("ParameterTables",dtme.key,temp_dir.name());
	  dtme.d.reset(new DataTypeMap);
	  if (dtme.d->fill(sdum)) {
	    DataTypeMapEntry dtme2;
	    if (!data_type_map_table->found(dtme.key,dtme2)) {
		data_type_map_table->insert(dtme);
	    }
	  }
	}
    }
  }
  return dtme;
}

std::string DataTypeMapper::getDescription(std::string format,std::string dsnum,std::string data_type_code,std::string platform_type)
{
  return getEntry(format,dsnum).d->getDescription(data_type_code,platform_type);
}

std::list<std::string> DataTypeMapper::getGCMDKeywords(std::string format,std::string dsnum,std::string data_type_code)
{ 
  return getEntry(format,dsnum).d->getGCMDKeywords(data_type_code);
}

std::list<std::string> DataTypeMapper::getVariables(std::string format,std::string dsnum,std::string data_type_code)
{ 
  return getEntry(format,dsnum).d->getVariables(data_type_code);
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
