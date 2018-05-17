#include <fstream>
#include <string>
#include <sys/stat.h>
#include <regex>
#include <sstream>
#include <metadata.hpp>
#include <metahelpers.hpp>
#include <datetime.hpp>
#include <MySQL.hpp>
#include <bsort.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bitmap.hpp>
#include <search.hpp>
#include <xml.hpp>
#include <gridutils.hpp>

namespace summarizeMetadata {

namespace detailedMetadata {

bool compare_parameter_data_entries(const ParameterDataEntry& left,const ParameterDataEntry& right)
{
  if (left.key == right.key) {
    return true;
  }
  else {
    if (!strutils::contains(left.key,".") && !strutils::contains(right.key,".")) {
	if (strutils::is_numeric(left.key) && strutils::is_numeric(right.key)) {
	  auto l=std::stoi(left.key);
	  auto r=std::stoi(right.key);
	  if (l < r) {
	    return true;
	  }
	  else {
	    return false;
	  }
	}
	else {
	  if (left.key < right.key) {
	    return true;
	  }
	  else {
	    return false;
	  }
	}
    }
    else {
	auto left_parts=strutils::split(left.key,".");
	auto right_parts=strutils::split(right.key,".");
	bool status=true;
	if (left_parts.size() != right_parts.size()) {
	  if (left.key < right.key) {
	    status=true;
	  }
	  else {
	    status=false;
	  }
	}
	else {
	  for (size_t n=0; n < left_parts.size(); n++) {
	    auto l=std::stoi(left_parts[n]);
	    auto r=std::stoi(right_parts[n]);
	    if (l < r) {
		status=true;
		n=left_parts.size();
	    }
	    else if (l > r) {
		status=false;
		n=left_parts.size();
	    }
	  }
	}
	return status;
    }
  }
}

bool compare_levels(std::string& left,std::string& right)
{
  auto left_parts=strutils::split(left,"<!>");
  auto lv=left_parts[0];
  while (lv.length() < 3) {
    lv.insert(0,"0");
  }
  auto right_parts=strutils::split(right,"<!>");
  auto rv=right_parts[0];
  while (rv.length() < 3) {
    rv.insert(0,"0");
  }
  if (lv <= rv) {
    return true;
  }
  else {
    return false;
  }
}

bool compare_layers(std::string& left,std::string& right)
{
  auto left_parts=strutils::split(left,"<!>");
  auto l_parts=strutils::split(left_parts[0],"-");
  auto lv=l_parts[0];
  while (lv.length() < 3) {
    lv.insert(0,"0");
  }
  auto lv2=l_parts[1];
  while (lv2.length() < 3) {
    lv2.insert(0,"0");
  }
  auto right_parts=strutils::split(right,"<!>");
  auto r_parts=strutils::split(right_parts[0],"-");
  auto rv=r_parts[0];
  while (rv.length() < 3) {
    rv.insert(0,"0");
  }
  auto rv2=r_parts[1];
  while (rv2.length() < 3) {
    rv2.insert(0,"0");
  }
  if (lv < rv) {
    return true;
  }
  else if (lv > rv) {
    return false;
  }
  else {
    if (lv2 <= rv2) {
	return true;
    }
    else {
	return false;
    }
  }
}

void fill_level_code_table(my::map<metadata::LevelCodeEntry>& level_code_table)
{
  MySQL::Server server(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  MySQL::LocalQuery query("code,map,type,value","GrML.levels");
  if (query.submit(server) < 0) {
    std::cerr << query.error() << std::endl;
    exit(1);
  }
  MySQL::Row row;
  while (query.fetch_row(row)) {
    metadata::LevelCodeEntry lce;
    lce.key=std::stoi(row[0]);
    lce.data.reset(new metadata::LevelCodeEntry::Data);
    lce.data->map=row[1];
    lce.data->type=row[2];
    lce.data->value=row[3];
    level_code_table.insert(lce);
  }
  server.disconnect();
}

void generate_parameter_cross_reference(std::string format,std::string title,std::string html_file,std::string caller,std::string user)
{
  TempDir tdir;
  if (!tdir.create(meta_directives.temp_path)) {
    metautils::log_error("generate_parameter_cross_reference(): unable to create temporary directory",caller,user);
  }
// create the directory tree in the temp directory
  std::stringstream oss,ess;
  if (unixutils::mysystem2("/bin/mkdir -p "+tdir.name()+"/metadata",oss,ess) < 0) {
    metautils::log_error("generate_parameter_cross_reference(): unable to create temporary directory tree",caller,user);
  }
  MySQL::Server server(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  MySQL::LocalQuery query("select distinct parameter from GrML.summary as s left join GrML.formats as f on f.code = s.format_code where s.dsid = '"+meta_args.dsnum+"' and f.format = '"+format+"'");
  if (query.submit(server) < 0) {
    std::cerr << query.error() << std::endl;
    exit(1);
  }
  if (query.num_rows() > 0) {
    my::map<ParameterTableEntry> parameter_table;
    xmlutils::ParameterMapper parameter_mapper(meta_directives.parameter_map_path);
    MySQL::Row row;
    while (query.fetch_row(row)) {
	auto code=row[0];
	std::string map;
	xmlutils::clean_parameter_code(code,map);
	ParameterTableEntry pte;
	pte.key=parameter_mapper.title(format,map);
	if (!parameter_table.found(pte.key,pte)) {
	  pte.parameter_data_table.reset(new my::map<ParameterDataEntry>);
	  parameter_table.insert(pte);
	}
	ParameterDataEntry pde;
	pde.key=code;
	if (!pte.parameter_data_table->found(pde.key,pde)) {
	  pde.short_name=parameter_mapper.short_name(format,row[0]);
	  pde.description=parameter_mapper.description(format,row[0]);
	  pde.units=parameter_mapper.units(format,row[0]);
	  pde.comment=parameter_mapper.comment(format,row[0]);
	  pte.parameter_data_table->insert(pde);
	}
    }
    std::ofstream ofs((tdir.name()+"/metadata/"+html_file).c_str());
    if (!ofs.is_open()) {
	metautils::log_error("generate_parameter_cross_reference(): unable to open html file for output",caller,user);
    }
    std::ofstream ofs2((tdir.name()+"/metadata/"+strutils::substitute(html_file,".html",".xml")).c_str());
    if (!ofs2.is_open()) {
	metautils::log_error("generate_parameter_cross_reference(): unable to open xml file for output",caller,user);
    }
    ofs2 << "<?xml version=\"1.0\" ?>" << std::endl;
    ofs << "<style id=\"detail\">" << std::endl;
    ofs << "  @import url(/css/transform.css);" << std::endl;
    ofs << "</style>" << std::endl;
    ofs2 << "<parameterTables>" << std::endl;
    for (const auto& key : parameter_table.keys()) {
	ParameterTableEntry pte;
	parameter_table.found(key,pte);
	std::vector<ParameterDataEntry> pdearray;
	pdearray.reserve(pte.parameter_data_table->size());
	auto show_short_name_column=false;
	auto show_comment_column=false;
	for (const auto key2 : pte.parameter_data_table->keys()) {
	  ParameterDataEntry pde;
	  pte.parameter_data_table->found(key2,pde);
	  if (!show_short_name_column && !pde.short_name.empty()) {
	    show_short_name_column=true;
	  }
	  if (!show_comment_column && !pde.comment.empty()) {
	    show_comment_column=true;
	  }
	  pdearray.emplace_back(pde);
	}
	binary_sort(pdearray,compare_parameter_data_entries);
	ofs << "<p>The following " << pte.parameter_data_table->size() << " parameters from " << key << " are included in this dataset:<center><table class=\"insert\" cellspacing=\"1\" cellpadding=\"5\" border=\"0\"><tr class=\"bg2\" valign=\"bottom\"><th align=\"center\">";
	ofs2 << "  <parameterTable>" << std::endl;
	ofs2 << "    <description>" << key << "</description>" << std::endl;
	if (format == "WMO_GRIB2") {
	  ofs << "Discipline</th><th align=\"center\">Category</th><th align=\"center\">Code";
	}
	else {
	  ofs << "Parameter Code";
	}
	ofs << "</th>";
	if (show_short_name_column) {
	  ofs << "<th align=\"center\">Short Name</th>";
	}
	ofs << "<th align=\"left\">Description</th><th align=\"center\">Units</th>";
	if (show_comment_column) {
	  ofs << "<th align=\"left\">Comments</th>";
	}
	ofs << "</tr>" << std::endl;
	for (size_t n=0; n < pte.parameter_data_table->size(); n++) {
	  ofs << "<tr class=\"bg" << (n % 2) << "\"><td align=\"center\">";
	  ofs2 << "    <parameter>" << std::endl;
	  if (format == "WMO_GRIB2") {
	    auto parameter_parts=strutils::split(pdearray[n].key,".");
	    ofs << parameter_parts[0] << "</td><td align=\"center\">" << parameter_parts[1] << "</td><td align=\"center\">" << parameter_parts[2];
	    ofs2 << "      <discipline>" << parameter_parts[0] << "</discipline>" << std::endl;
	    ofs2 << "      <category>" << parameter_parts[1] << "</category>" << std::endl;
	    ofs2 << "      <code>" << parameter_parts[2] << "</code>" << std::endl;
	  }
	  else {
	    ofs << pdearray[n].key;
	    ofs2 << "      <code>" << pdearray[n].key << "</code>" << std::endl;
	  }
	  ofs << "</td>";
	  if (show_short_name_column) {
	    ofs << "<td align=\"center\">" << pdearray[n].short_name << "</td>";
	    ofs2 << "      <shortName>" << pdearray[n].short_name << "</shortName>" << std::endl;
	  }
	  ofs << "<td align=\"left\">" << pdearray[n].description << "</td><td align=\"center\">" << htmlutils::transform_superscripts(pdearray[n].units) << "</td>";
	  ofs2 << "      <description>" << pdearray[n].description << "</description>" << std::endl;
	  ofs2 << "      <units>" << pdearray[n].units << "</units>" << std::endl;
	  if (show_comment_column) {
	    ofs << "<td align=\"left\">" << pdearray[n].comment << "</td>";
	    ofs2 << "      <comments>" << pdearray[n].comment << "</comments>" << std::endl;
	  }
	  ofs << "</tr>" << std::endl;
	  ofs2 << "    </parameter>" << std::endl;
	}
	ofs << "</table></center></p>" << std::endl;
	ofs2 << "  </parameterTable>" << std::endl;
    }
    ofs2 << "</parameterTables>" << std::endl;
    ofs.close();
    ofs2.close();
    std::string error;
    if (unixutils::rdadata_sync(tdir.name(),"metadata/","/data/web/datasets/ds"+meta_args.dsnum,meta_directives.rdadata_home,error) < 0) {
	metautils::log_warning("generate_parameter_cross_reference() couldn't sync cross-references - rdadata_sync error(s): '"+error+"'",caller,user);
    }
  }
  else {
// remove a parameter table if it exists and there are no parameters for this
//   format
    std::string error;
    if (unixutils::rdadata_unsync("/__HOST__/web/datasets/ds"+meta_args.dsnum+"/metadata/"+html_file,meta_directives.rdadata_home,error) < 0) {
	metautils::log_warning("generate_parameter_cross_reference() tried to but couldn't delete '"+html_file+"' - error: '"+error+"'",caller,user);
    }
    if (unixutils::rdadata_unsync("/__HOST__/web/datasets/ds"+meta_args.dsnum+"/metadata/"+strutils::substitute(html_file,".html",".xml"),meta_directives.rdadata_home,error) < 0) {
	metautils::log_warning("generate_parameter_cross_reference() tried to but couldn't delete '"+strutils::substitute(html_file,".html",".xml")+"' - error: '"+error+"'",caller,user);
    }
  }
  server.disconnect();
}

void generate_level_cross_reference(std::string format,std::string title,std::string html_file,std::string caller,std::string user)
{
  my::map<metadata::LevelCodeEntry> level_code_table;
  fill_level_code_table(level_code_table);
  MySQL::Server server(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  MySQL::LocalQuery query("select distinct levelType_codes from GrML.summary as s left join GrML.formats as f on f.code = s.format_code where s.dsid = '"+meta_args.dsnum+"' and f.format = '"+format+"'");
  if (query.submit(server) < 0) {
    std::cerr << query.error() << std::endl;
    exit(1);
  }
  std::list<std::string> levels,layers;
  if (query.num_rows() > 0) {
    xmlutils::LevelMapper level_mapper(meta_directives.level_map_path);
    my::map<metautils::StringEntry> unique_level_type_table;
    MySQL::Row row;
    while (query.fetch_row(row)) {
	std::vector<size_t> level_values;
	bitmap::uncompress_values(row[0],level_values);
	for (auto& level_value : level_values) {
	  metadata::LevelCodeEntry lce;
	  level_code_table.found(level_value,lce);
	  metautils::StringEntry se;
	  if (!unique_level_type_table.found(lce.data->type,se)) {
	    if (strutils::contains(lce.data->type,"-")) {
		auto layer_types=strutils::split(lce.data->type,"-");
		layers.emplace_back(lce.data->type+"<!>"+level_mapper.description(format,lce.data->type,lce.data->map)+"<!>"+htmlutils::transform_superscripts(level_mapper.units(format,layer_types[0],lce.data->map))+"<!>"+htmlutils::transform_superscripts(level_mapper.units(format,layer_types[1],lce.data->map)));
	    }
	    else {
		levels.emplace_back(lce.data->type+"<!>"+level_mapper.description(format,lce.data->type,lce.data->map)+"<!>"+htmlutils::transform_superscripts(level_mapper.units(format,lce.data->type,lce.data->map)));
	    }
	    se.key=lce.data->type;
	    unique_level_type_table.insert(se);
	  }
	}
    }
  }
  if (levels.size() > 0 || layers.size() > 0) {
    TempDir tdir;
    if (!tdir.create(meta_directives.temp_path)) {
	metautils::log_error("generate_level_cross_reference(): unable to create temporary directory",caller,user);
    }
// create the directory tree in the temp directory
    std::stringstream oss,ess;
    if (unixutils::mysystem2("/bin/mkdir -p "+tdir.name()+"/metadata",oss,ess) < 0) {
	metautils::log_error("generate_level_cross_reference(): unable to create temporary directory tree",caller,user);
    }
    std::ofstream ofs((tdir.name()+"/metadata/"+html_file).c_str());
    if (!ofs.is_open()) {
	metautils::log_error("generate_level_cross_reference(): unable to open html file for output",caller,user);
    }
    ofs << "<style id=\"detail\">" << std::endl;
    ofs << "  @import url(/css/transform.css);" << std::endl;
    ofs << "</style>" << std::endl;
    auto n=0;
    if (levels.size() > 0) {
	ofs << "<p>The following " << levels.size() << " " << strutils::to_capital(format) << " levels are included in this dataset:<center><table class=\"insert\" cellspacing=\"1\" cellpadding=\"5\" border=\"0\"><tr class=\"bg2\" valign=\"bottom\"><th align=\"center\">Code</th><th align=\"left\">Description</th><th align=\"center\">Units</th></tr>" << std::endl;
	levels.sort(compare_levels);
	for (const auto& level : levels) {
	  auto level_parts=strutils::split(level,"<!>");
	  ofs << "<tr class=\"bg" << (n % 2) << "\"><td align=\"center\">" << level_parts[0] << "</td><td align=\"left\">" << level_parts[1] << "</td><td align=\"center\">" << level_parts[2] << "</td></tr>" << std::endl;
	  ++n;
	}
	ofs << "</table></center></p>" << std::endl;
    }
    if (layers.size() > 0) {
	ofs << "<p>The following " << layers.size() << " " << strutils::to_capital(format) << " layers are included in this dataset:<center><table class=\"insert\" cellspacing=\"1\" cellpadding=\"5\" border=\"0\"><tr class=\"bg2\" valign=\"bottom\"><th align=\"center\">Code<br />(Bottom)</th><th align=\"center\">Code<br />(Top)</th><th align=\"left\">Description</th><th align=\"center\">Units<br />(Bottom)</th><th align=\"center\">Units<br />(Top)</th></tr>" << std::endl;
	layers.sort(compare_layers);
	for (const auto& layer : layers) {
	  auto layer_parts=strutils::split(layer,"<!>");
	  auto layer_codes=strutils::split(layer_parts[0],"-");
	  ofs << "<tr class=\"bg" << (n % 2) << "\"><td align=\"center\">" << layer_codes[0] << "</td><td align=\"center\">" << layer_codes[1] << "</td><td align=\"left\">" << layer_parts[1] << "</td><td align=\"center\">" << layer_parts[2] << "</td><td align=\"center\">" << layer_parts[3] << "</td></tr>" << std::endl;
	  ++n;
	}
    }
    ofs.close();
    std::string error;
    if (unixutils::rdadata_sync(tdir.name(),"metadata/","/data/web/datasets/ds"+meta_args.dsnum,meta_directives.rdadata_home,error) < 0)
	metautils::log_warning("generate_level_cross_reference() couldn't sync '"+html_file+"' - rdadata_sync error(s): '"+error+"'",caller,user);
  }
  else {
// remove the level table if it exists and there are no levels for this format
    std::string error;
    if (unixutils::rdadata_unsync("/__HOST__/web/datasets/ds"+meta_args.dsnum+"/metadata/"+html_file,meta_directives.rdadata_home,error) < 0) {
	metautils::log_warning("generate_level_cross_reference() tried to but couldn't delete '"+html_file+"' - error: '"+error+"'",caller,user);
    }
  }
  server.disconnect();
}

void replace_token(std::string& source,std::string token,std::string new_s)
{
  std::stringstream ss;
  ss << "\\{" << token;
  while (std::regex_search(source,std::regex(ss.str()))) {
    token=source.substr(source.find("{"+token));
    token=token.substr(0,token.find("}"));
    std::deque<std::string> tparts=strutils::split(token,"_");
    if (tparts.size() > 1) {
	if (tparts[1] == "I3") {
	  new_s.insert(0,3-new_s.length(),'0');
	}
    }
    token+="}";
    strutils::replace_all(source,token,new_s);
  }
}

struct VtableEntry {
  struct Data {
    Data() : parameter_table(),metgrid_units(),metgrid_description(),grib1(),grib2() {}

    std::string parameter_table,metgrid_units,metgrid_description;
    struct GRIBData {
	GRIBData() : param_code(),level_type_code(),top_code(),bottom_code(),unique_levels() {}

	std::string param_code,level_type_code,top_code,bottom_code;
	my::map<metautils::StringEntry> unique_levels;
    } grib1,grib2;
  };
  VtableEntry() : key(),data(nullptr) {}

  std::string key;
  std::shared_ptr<Data> data;
};
struct RequiredsEntry {
  struct Data {
    Data() : timeRanges_map() {}

    my::map<metautils::StringEntry> timeRanges_map;
  };
  RequiredsEntry() : key(),data(nullptr) {}

  std::string key;
  std::shared_ptr<Data> data;
};
struct TimeRangeEntry {
  TimeRangeEntry() : key(),num_occurrences(nullptr) {}

  std::string key;
  std::shared_ptr<size_t> num_occurrences;
};
void generate_vtable(std::string dsnum,std::string& error,bool test_only)
{
  MySQL::LocalQuery query;
  MySQL::Row row;
  my::map<metadata::LevelCodeEntry> level_hash;
  metadata::LevelCodeEntry lce;
  XMLDocument vdoc;
  std::vector<size_t> lvals;
  XMLElement e;
  my::map<VtableEntry> vtable_map;
  VtableEntry vte;
  my::map<RequiredsEntry> requireds_map;
  RequiredsEntry re;
  my::map<metautils::StringEntry> already_checked_map;
  metautils::StringEntry se;
  bool found_grib1=false,found_grib2=false;

  MySQL::Server server(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  query.set("map,type,value,code","GrML.levels");
  if (query.submit(server) < 0) {
    error="Unable to build the level hash";
    return;
  }
  while (query.fetch_row(row)) {
    lce.key=std::stoi(row[3]);
    lce.data.reset(new metadata::LevelCodeEntry::Data);
    lce.data->map=row[0];
    lce.data->type=row[1];
    lce.data->value=row[2];
    level_hash.insert(lce);
  }
  vdoc.open("/glade/u/home/rdadata/share/metadata/WRF.xml");
  if (!vdoc.is_open()) {
    error="WRF.xml is missing or unable to open";
    return;
  }
  std::list<XMLElement> elist=vdoc.element_list("formats/format@name=WPS_requireds/metgrid_name");
  for (auto& e : elist) {
    re.key=e.content();
    re.data.reset(new RequiredsEntry::Data);
    requireds_map.insert(re);
  }
  query.set("select s.parameter,t.timeRange,s.levelType_codes from GrML.summary as s left join GrML.formats as f on f.code = s.format_code left join GrML.timeRanges as t on t.code = s.timeRange_code where s.dsid = '"+dsnum+"' and f.format = 'WMO_GRIB1' and (t.timeRange = 'Analysis' or t.timeRange like '6-hour Forecast')");
  if (query.submit(server) == 0 && query.num_rows() > 0) {
    already_checked_map.clear();
    while (query.fetch_row(row)) {
	size_t idx=row[0].find(":");
	std::string pmap=row[0].substr(0,idx);
	std::string pcode=row[0].substr(idx+1);
	bitmap::uncompress_values(row[2],lvals);
	for (auto& lval : lvals) {
	  se.key=row[0]+"<!>"+strutils::itos(lval);
	  if (!already_checked_map.found(se.key,se)) {
	    already_checked_map.insert(se);
	    level_hash.found(lval,lce);
	    std::deque<std::string> sp=strutils::split(lce.data->value,",");
	    if (sp.size() > 1) {
		e=vdoc.element("formats/format@name=WMO_GRIB1/parameter@code="+pcode+"/table@value="+pmap+"/layer@type="+lce.data->type+"@top="+sp[1]+"@bottom="+sp[0]);
		if (e.name().empty()) {
		  e=vdoc.element("formats/format@name=WMO_GRIB1/parameter@code="+pcode+"/table@value="+pmap+"/layer@type="+lce.data->type+"@top=*@bottom=*");
		  if (e.name().empty()) {
		    e=vdoc.element("formats/format@name=WMO_GRIB1/parameter@code="+pcode+"/layer@type="+lce.data->type+"@top="+sp[1]+"@bottom="+sp[0]);
		    if (e.name().empty()) {
			e=vdoc.element("formats/format@name=WMO_GRIB1/parameter@code="+pcode+"/layer@type="+lce.data->type+"@top=*@bottom=*");
		    }
		  }
		}
	    }
	    else {
		e=vdoc.element("formats/format@name=WMO_GRIB1/parameter@code="+pcode+"/table@value="+pmap+"/level@type="+lce.data->type+"@value="+sp[0]);
		if (e.name().empty()) {
		  e=vdoc.element("formats/format@name=WMO_GRIB1/parameter@code="+pcode+"/table@value="+pmap+"/level@type="+lce.data->type+"@value=*");
		  if (e.name().empty()) {
		    e=vdoc.element("formats/format@name=WMO_GRIB1/parameter@code="+pcode+"/level@type="+lce.data->type+"@value="+sp[0]);
		    if (e.name().empty()) {
			e=vdoc.element("formats/format@name=WMO_GRIB1/parameter@code="+pcode+"/level@type="+lce.data->type+"@value=*");
			if (!e.name().empty()) {
			  sp[0]="*";
			}
		    }
		  }
		  else {
		    sp[0]="*";
		  }
		}
	    }
	    if (!e.name().empty()) {
		vte.key=e.element("metgrid_name").content();
		if (vte.key != "null") {
		  if (requireds_map.found(vte.key,re) && !re.data->timeRanges_map.found(row[1],se)) {
		    se.key=row[1];
		    re.data->timeRanges_map.insert(se);
		  }
		  if (sp.size() > 1) {
		    replace_token(vte.key,"TOP",sp[1]);
		    replace_token(vte.key,"BOTTOM",sp[0]);
		  }
		  else {
		    replace_token(vte.key,"VALUE",sp[0]);
		  }
		  if (!vtable_map.found(vte.key,vte)) {
		    if (sp.size() == 1 || sp[0] != sp[1]) {
			vte.data.reset(new VtableEntry::Data);
			vte.data->parameter_table=pmap;
			vte.data->metgrid_units=e.element("metgrid_units").content();
			vte.data->metgrid_description=e.element("metgrid_description").content();
			if (sp.size() > 1) {
			  replace_token(vte.data->metgrid_description,"TOP",sp[1]);
			  replace_token(vte.data->metgrid_description,"BOTTOM",sp[0]);
			}
			else {
			  replace_token(vte.data->metgrid_description,"VALUE",sp[0]);
			}
			vte.data->grib1.param_code=pcode;
			vte.data->grib1.level_type_code=lce.data->type;
			if (sp.size() > 1) {
			  vte.data->grib1.bottom_code=sp[0];
			  vte.data->grib1.top_code=sp[1];
			}
			else {
			  vte.data->grib1.top_code=sp[0];
			  vte.data->grib1.bottom_code="";
			}
			se.key=lce.data->type+"<!>"+lce.data->value;
			vte.data->grib1.unique_levels.insert(se);
			vtable_map.insert(vte);
			found_grib1=true;
			if (std::regex_search(vte.key,std::regex("^S[TM][0-9][0-9][0-9][0-9][0-9][0-9]"))) {
			  vte.key=vte.key.substr(0,2);
			  if (!vtable_map.found(vte.key,vte)) {
			    vte.data.reset();
			    vtable_map.insert(vte);
			  }
			}
		    }
		  }
		  else {
		    se.key=lce.data->type+"<!>"+lce.data->value;
		    if (!vte.data->grib1.unique_levels.found(se.key,se)) {
			vte.data->grib1.unique_levels.insert(se);
		    }
		  }
		}
	    }
	  }
	}
    }
  }
  query.set("select s.parameter,t.timeRange,s.levelType_codes from GrML.summary as s left join GrML.formats as f on f.code = s.format_code left join GrML.timeRanges as t on t.code = s.timeRange_code where s.dsid = '"+dsnum+"' and f.format = 'WMO_GRIB2' and (t.timeRange = 'Analysis' or t.timeRange like '6-hour Forecast')");
  if (query.submit(server) == 0 && query.num_rows() > 0) {
    already_checked_map.clear();
    while (query.fetch_row(row)) {
	size_t idx=row[0].find(":");
	std::string pmap=row[0].substr(0,idx);
	std::string pcode=row[0].substr(idx+1);
	bitmap::uncompress_values(row[2],lvals);
	for (auto& lval : lvals) {
	  se.key=row[0]+"<!>"+strutils::itos(lval);
//	  if (!already_checked_map.found(se.key,se)) {
//	    already_checked_map.insert(se);
	    level_hash.found(lval,lce);
	    std::deque<std::string> sp=strutils::split(lce.data->value,",");
	    if (sp.size() > 1) {
		if (std::stoi(pcode.substr(pcode.rfind(".")+1)) < 192) {
		  elist=vdoc.element_list("formats/format@name=WMO_GRIB2/parameter@code="+pcode+"/layer@type="+lce.data->type+"@top="+sp[1]+"@bottom="+sp[0]);
		  if (elist.size() == 0) {
		    elist=vdoc.element_list("formats/format@name=WMO_GRIB2/parameter@code="+pcode+"/layer@type="+lce.data->type+"@top=*@bottom=*");
		  }
		}
		else {
		  elist=vdoc.element_list("formats/format@name=WMO_GRIB2/parameter@code="+pcode+"/table@value="+pmap+"/layer@type="+lce.data->type+"@top="+sp[1]+"@bottom="+sp[0]);
		  if (elist.size() == 0) {
		    elist=vdoc.element_list("formats/format@name=WMO_GRIB2/parameter@code="+pcode+"/table@value="+pmap+"/layer@type="+lce.data->type+"@top=*@bottom=*");
		  }
		}
	    }
	    else {
		if (std::stoi(pcode.substr(pcode.rfind(".")+1)) < 192) {
		  elist=vdoc.element_list("formats/format@name=WMO_GRIB2/parameter@code="+pcode+"/level@type="+lce.data->type+"@value="+sp[0]);
		  if (elist.size() == 0) {
		    elist=vdoc.element_list("formats/format@name=WMO_GRIB2/parameter@code="+pcode+"/level@type="+lce.data->type+"@value=*");
		    if (elist.size() == 1 && !std::regex_search(elist.front().element("metgrid_name").content(),std::regex("\\{"))) {
			sp[0]="*";
		    }
		  }
		}
		else {
		  elist=vdoc.element_list("formats/format@name=WMO_GRIB2/parameter@code="+pcode+"/table@value="+pmap+"/level@type="+lce.data->type+"@value="+sp[0]);
		  if (elist.size() == 0) {
		    elist=vdoc.element_list("formats/format@name=WMO_GRIB2/parameter@code="+pcode+"/table@value="+pmap+"/level@type="+lce.data->type+"@value=*");
		    if (elist.size() == 1 && !std::regex_search(elist.front().element("metgrid_name").content(),std::regex("\\{"))) {
			sp[0]="*";
		    }
		  }
		}
	    }
	    for (auto& e : elist) {
// make this GRIB2 level type compatible with GRIB1 level type 112, which
//   was units of cm
		if (lce.data->type == "106" || lce.data->type == "106-106") {
		  if (std::regex_search(sp[0],std::regex("^0\\."))) {
		    sp[0]=sp[0].substr(2)+"0";
		  }
		  else if (sp[0] != "0") {
		    sp[0]+="00";
		  }
		  if (sp.size() > 1) {
		    if (std::regex_search(sp[1],std::regex("^0\\."))) {
			sp[1]=sp[1].substr(2)+"0";
		    }
		    else if (sp[1] != "0") {
			sp[1]+="00";
		    }
		  }
		}
		vte.key=e.element("metgrid_name").content();
		if (vte.key != "null") {
		  if (sp.size() > 1) {
		    replace_token(vte.key,"TOP",sp[1]);
		    replace_token(vte.key,"BOTTOM",sp[0]);
		  }
		  else {
		    replace_token(vte.key,"VALUE",sp[0]);
		  }
		  if (!vtable_map.found(vte.key,vte)) {
		    if (sp.size() == 1 || sp[0] != sp[1]) {
			vte.data.reset(new VtableEntry::Data);
			vte.data->metgrid_units=e.element("metgrid_units").content();
			vte.data->metgrid_description=e.element("metgrid_description").content();
			if (sp.size() > 1) {
			  replace_token(vte.data->metgrid_description,"TOP",sp[1]);
			  replace_token(vte.data->metgrid_description,"BOTTOM",sp[0]);
			}
			else {
			  replace_token(vte.data->metgrid_description,"VALUE",sp[0]);
			}
			vte.data->grib2.param_code=pcode;
			vte.data->grib2.level_type_code=lce.data->type.substr(0,lce.data->type.find("-"));
			if (sp.size() > 1) {
			  vte.data->grib2.bottom_code=sp[0];
			  vte.data->grib2.top_code=sp[1];
			}
			else {
			  vte.data->grib2.top_code=sp[0];
			  vte.data->grib2.bottom_code="";
			}
			se.key=lce.data->type+"<!>"+lce.data->value;
			vte.data->grib2.unique_levels.insert(se);
			vtable_map.insert(vte);
			found_grib2=true;
			if (std::regex_search(vte.key,std::regex("^S[TM][0-9][0-9][0-9][0-9][0-9][0-9]"))) {
			  vte.key=vte.key.substr(0,2);
			  if (!vtable_map.found(vte.key,vte)) {
			    vte.data.reset();
			    vtable_map.insert(vte);
			  }
			}
		    }
		  }
		  else {
		    if (vte.data->grib2.param_code.empty()) {
			vte.data->grib2.param_code=pcode;
			vte.data->grib2.level_type_code=lce.data->type.substr(0,lce.data->type.find("-"));
			found_grib2=true;
		    }
		    se.key=lce.data->type+"<!>"+lce.data->value;
		    if (!vte.data->grib2.unique_levels.found(se.key,se)) {
			vte.data->grib2.unique_levels.insert(se);
		    }
		  }
		  if (requireds_map.found(vte.key,re) && !re.data->timeRanges_map.found(row[1],se)) {
		    se.key=row[1];
		    re.data->timeRanges_map.insert(se);
		  }
		}
	    }
//	  }
	}
    }
  }
// verify that Vtable should be created
  if (vtable_map.size() == 0) {
    error="No data suitable for WRF initialization were found";
    return;
  }
for (auto& key : vtable_map.keys()) {
std::cerr << "found " << key << std::endl;
}
std::cerr << "total found: " << vtable_map.size() << std::endl;
  for (auto& key : requireds_map.keys()) {
std::cerr << "required " << key << " " << vtable_map.found(key,vte) << std::endl;
    if (!vtable_map.found(key,vte)) {
	elist=vdoc.element_list("formats/format@name=WPS_alternates/metgrid_name@alternateFor="+key);
	std::list<XMLElement>::iterator it=elist.begin();
	for (auto& e : elist) {
std::cerr << "looking for alternate for " << key << " testing '" << e.content() << "'" << std::endl;
	  if (vtable_map.found(e.content(),vte)) {
	    break;
	  }
	  ++it;
	}
	if (it == elist.end()) {
	  found_grib1=found_grib2=false;
	  break;
	}
std::cerr << "found alternate for " << key << " '" << it->content() << "'" << std::endl;
    }
    if (vte.data != nullptr) {
	if (vte.data->grib1.param_code.empty()) {
	  found_grib1=false;
	}
	if (vte.data->grib2.param_code.empty()) {
	  found_grib2=false;
	}
	if (std::regex_search(vte.key,std::regex(":P$"))) {
	  if (vte.data->grib1.unique_levels.size() < 5) {
	    found_grib1=false;
	  }
	  if (vte.data->grib2.unique_levels.size() < 5) {
	    found_grib2=false;
	  }
	}
	if (!found_grib1 && !found_grib2) {
	  break;
	}
    }
  }
  if (found_grib1 || found_grib2) {
std::cerr << "should write table" << std::endl;
    if (!test_only) {
	TempDir tdir;
	if (!tdir.create(meta_directives.temp_path)) {
	  error="Unable to create temporary directory";
	  return;
	}
// create the directory tree in the temp directory
	std::stringstream oss,ess;
	if (unixutils::mysystem2("/bin/mkdir -p "+tdir.name()+"/metadata",oss,ess) < 0) {
	  error="Unable to create temporary directory tree - '"+ess.str()+"'";
	  return;
	}
	std::ofstream ofs((tdir.name()+"/metadata/Vtable.RDA_ds"+dsnum).c_str());
	if (!ofs.is_open()) {
	  error="Unable to open temporary file for output";
	  return;
	}
	ofs << "GRIB1| Level| From |  To  | metgrid  | metgrid | metgrid                                 |";
	if (found_grib2) {
	  ofs << "GRIB2|GRIB2|GRIB2|GRIB2|";
	}
	ofs << std::endl;
	ofs << "Param| Type |Level1|Level2| Name     | Units   | Description                             |";
	if (found_grib2) {
	  ofs << "Discp|Catgy|Param|Level|";
	}
	ofs << std::endl;
	ofs << "-----+------+------+------+----------+---------+-----------------------------------------+";
	if (found_grib2) {
	  ofs << "-----------------------+";
	}
	ofs << std::endl;
	for (auto key : vtable_map.keys()) {
	  vtable_map.found(key,vte);
	  if (vte.data != nullptr) {
	    key=key.substr(0,key.find(":"));
	    key.insert(key.length(),9-key.length(),' ');
	    if (vte.data->metgrid_units.length() < 8) {
		vte.data->metgrid_units.insert(vte.data->metgrid_units.length(),8-vte.data->metgrid_units.length(),' ');
	    }
	    if (vte.data->metgrid_description.length() < 40) {
		vte.data->metgrid_description.insert(vte.data->metgrid_description.length(),40-vte.data->metgrid_description.length(),' ');
	    }
	    if (found_grib1) {
		if (vte.data->grib1.param_code.empty()) {
		  vte.data->grib1.param_code="0";
		}
		if (vte.data->grib1.level_type_code.empty()) {
		  vte.data->grib1.level_type_code="0";
		}
		if (vte.data->grib1.top_code.empty()) {
		  vte.data->grib1.top_code="0";
		}
		ofs << std::string(4-vte.data->grib1.param_code.length(),' ') << vte.data->grib1.param_code << " | " << std::string(3-vte.data->grib1.level_type_code.length(),' ') << vte.data->grib1.level_type_code << "  | " << std::string(3-vte.data->grib1.top_code.length(),' ') << vte.data->grib1.top_code << "  | " << std::string(3-vte.data->grib1.bottom_code.length(),' ') << vte.data->grib1.bottom_code << "  | ";
	    }
	    else {
		ofs << "     |   0  | " << std::string(3-vte.data->grib2.top_code.length(),' ') << vte.data->grib2.top_code << "  | " << std::string(3-vte.data->grib2.bottom_code.length(),' ') << vte.data->grib2.bottom_code << "  | ";
	    }
	    ofs << key << "| " << vte.data->metgrid_units.substr(0,8) << "| " << vte.data->metgrid_description.substr(0,40) << "|";
	    if (found_grib2) {
		if (!vte.data->grib2.param_code.empty()) {
		  std::deque<std::string> sp=strutils::split(vte.data->grib2.param_code,".");
		  ofs << " " << std::string(2-sp[0].length(),' ') << sp[0] << "  | " << std::string(2-sp[1].length(),' ') << sp[1] << "  | " << std::string(3-sp[2].length(),' ') << sp[2] << " | " << std::string(3-vte.data->grib2.level_type_code.length(),' ') << vte.data->grib2.level_type_code << " |";
		}
		else {
		  ofs << "     |     |     |     |";
		}
	    }
	    ofs << std::endl;
	  }
	}
	ofs << "-----+------+------+------+----------+---------+-----------------------------------------+";
	if (found_grib2) {
	  ofs << "-----------------------+";
	}
	ofs << std::endl;
	ofs << "#" << std::endl;
	ofs << "#  Vtable for NCAR/Research Data Archive dataset " << dsnum << std::endl;
	ofs << "#  http://rda.ucar.edu/datasets/ds" << dsnum << "/" << std::endl;
	ofs << "#  rdahelp@ucar.edu" << std::endl;
	ofs << "#" << std::endl;
	ofs.close();
	if (unixutils::rdadata_sync(tdir.name(),"metadata/","/data/web/datasets/ds"+dsnum,meta_directives.rdadata_home,error) < 0) {
	  error="Unable to sync 'Vtable.RDA_ds"+dsnum+"' - hostSync error(s): '"+error;
	}
    }
  }
  else {
    std::stringstream ess;
    for (auto& key : requireds_map.keys()) {
	if (requireds_map[key].data->timeRanges_map.size() == 0) {
	  ess << "  " << key << std::endl;
	}
    }
    error="Some fields required by WRF's WPS could not be found:\n"+ess.str();
  }
  my::map<TimeRangeEntry> tr_map;
  TimeRangeEntry tre;
  for (auto& key : requireds_map.keys()) {
    requireds_map.found(key,re);
std::cerr << key << " " << re.data->timeRanges_map.size() << std::endl;
/*
    for (auto& tr : re.data->timeRanges_map.keys()) {
    }
*/
  }
}

void addToFormats(XMLDocument& xdoc,MySQL::Query& query,std::list<std::string>& format_list,std::string& formats)
{
  MySQL::Row row;
  while (query.fetch_row(row)) {
    format_list.emplace_back(row[0]);
    auto e=xdoc.element("formatReferences/format@name="+row[0]);
    auto link=e.attribute_value("href");
    formats+="<li>";
    if (!link.empty()) {
	formats+="<a target=\"_format\" href=\""+link+"\">";
    }
    formats+=strutils::to_capital(strutils::substitute(row[0],"proprietary_","Dataset-specific "));
    if (!link.empty()) {
	formats+="</a>";
    }
    auto format_type=e.attribute_value("type");
    if (!format_type.empty()) {
	formats+=" ("+format_type+")";
    }
    formats+="</li>";
  }
}

struct VariableSummaryEntry {
  struct Data {
    Data() : start(),end() {}

    std::string start,end;
  };
  VariableSummaryEntry() : key(),data() {}

  std::string key;
  std::shared_ptr<Data> data;
};
struct LevelSummaryEntry {
  struct Data {
    Data() : map(),type(),value(),variable_table() {}

    std::string map,type,value;
    my::map<VariableSummaryEntry> variable_table;
  };
  LevelSummaryEntry() : key(),description(),data() {}

  size_t key;
  std::shared_ptr<std::string> description;
  std::shared_ptr<Data> data;
};
struct CombinedLevelSummaryEntry {
  struct Data {
    Data() : map_list(),variable_table() {}

    std::list<std::string> map_list;
    my::map<VariableSummaryEntry> variable_table;
  };
  CombinedLevelSummaryEntry() : key(),data() {}

  std::string key;
  std::shared_ptr<Data> data;
};
struct HashEntry {
  HashEntry() : key(),description() {}

  size_t key;
  std::shared_ptr<std::string> description;
};
my::map<HashEntry> product_hash;
struct HashEntry2 {
  HashEntry2() : key(),description() {}

  std::string key;
  std::shared_ptr<std::string> description;
};
struct GridSummaryEntry {
  GridSummaryEntry() : key(),description(),start(),end() {}

  size_t key;
  std::shared_ptr<std::string> description;
  std::shared_ptr<std::string> start,end;
};
struct ProductSummaryEntry {
  ProductSummaryEntry() : key(),description(),grid_table() {}

  size_t key;
  std::shared_ptr<std::string> description;
  std::shared_ptr<my::map<GridSummaryEntry>> grid_table;
};
void generate_detailed_grid_summary(MySQL::LocalQuery& format_query,std::string file_type,std::string group_index,std::ofstream& ofs,std::list<std::string>& format_list,std::string caller,std::string user)
{
  std::ofstream ofs_p,ofs_l;
  std::string dsnum2=strutils::substitute(meta_args.dsnum,".","");
  MySQL::Row row,row2;
  std::string db,table,dtable,ID_type;
  my::map<ParameterEntry> detailed_parameter_table(99999);
  my::map<LevelSummaryEntry> detailed_level_table(9999);
  LevelSummaryEntry lse;
  MySQL::LocalQuery query;
  std::string sdum,code;
  size_t cindex,ncols;
  size_t idx;
  ParameterEntry pe;
  UniqueEntry ue;
  my::map<metadata::LevelCodeEntry> level_code_table(99999);
  metadata::LevelCodeEntry lce;
  LevelEntry le;
  std::vector<size_t> level_values,product_values,grid_values;
  my::map<HashEntry2> parameter_description_hash;
  HashEntry2 he2;
  xmlutils::ParameterMapper parameter_mapper(meta_directives.parameter_map_path);
  xmlutils::LevelMapper level_mapper(meta_directives.level_map_path);
std::string output,error;
  VariableSummaryEntry vse;
  my::map<HashEntry> grid_definition_hash,level_description_hash;
  HashEntry he;
  my::map<ProductSummaryEntry> product_table;
  ProductSummaryEntry pse;
  std::vector<ProductSummaryEntry> psarray;
  GridSummaryEntry gse;
  std::vector<GridSummaryEntry> gsarray;
  std::vector<std::string> product_view_format_list;

  lce.data=nullptr;
  TempDir tdir;
  if (!tdir.create(meta_directives.temp_path)) {
    metautils::log_error("generate_detailed_grid_summary(): unable to create temporary directory",caller,user);
  }
// create the directory tree in the temp directory
  std::stringstream oss,ess;
  if (unixutils::mysystem2("/bin/mkdir -p "+tdir.name()+"/metadata",oss,ess) < 0) {
    metautils::log_error("generate_detailed_grid_summary(): unable to create temporary directory tree - '"+ess.str()+"'",caller,user);
  }
  ofs_p.open((tdir.name()+"/metadata/parameter-detail.html").c_str());
  if (!ofs_p.is_open()) {
    metautils::log_error("generate_detailed_grid_summary(): unable to open output for parameter detail",caller,user);
  }
  ofs_l.open((tdir.name()+"/metadata/level-detail.html").c_str());
  if (!ofs_l.is_open()) {
    metautils::log_error("generate_detailed_grid_summary(): unable to open output for level detail",caller,user);
  }
  if (file_type == "MSS") {
    db="GrML";
    table="_primaries";
    dtable="mssfile";
    ID_type="mss";
    query.set("code,map,type,value","GrML.levels");
  }
  else if (file_type == "Web") {
    db="WGrML";
    table="_webfiles";
    dtable="wfile";
    ID_type="web";
    query.set("code,map,type,value","WGrML.levels");
  }
  fill_level_code_table(level_code_table);
  MySQL::Server server(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  format_query.rewind();
  while (format_query.fetch_row(row)) { 
    if (!group_index.empty()) {
	if (format_query.num_rows() > 1) {
	  if (field_exists(server,db+".ds"+dsnum2+table,"dsid")) {
	    query.set("select a.parameter,a.levelType_codes,min(a.start_date),max(a.end_date) from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on (x."+dtable+" = p."+ID_type+"ID and x.type = p.type and x.dsid = p.dsid) left join "+db+".ds"+dsnum2+"_agrids as a on a."+ID_type+"ID_code = p.code where p.format_code = "+row[1]+" and !isnull(x."+dtable+") and x.gindex = "+group_index+" group by a.parameter,a.levelType_codes");
	  }
	  else {
	    query.set("select a.parameter,a.levelType_codes,min(a.start_date),max(a.end_date) from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on x."+dtable+" = p."+ID_type+"ID left join "+db+".ds"+dsnum2+"_agrids as a on a."+ID_type+"ID_code = p.code where p.format_code = "+row[1]+" and !isnull(x."+dtable+") and x.gindex = "+group_index+" group by a.parameter,a.levelType_codes");
	  }
	}
	else {
	  if (field_exists(server,db+".ds"+dsnum2+table,"dsid")) {
	    query.set("select a.parameter,a.levelType_codes,min(a.start_date),max(a.end_date) from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on (x."+dtable+" = p."+ID_type+"ID and x.type = p.type and x.dsid = p.dsid) left join "+db+".ds"+dsnum2+"_agrids as a on a."+ID_type+"ID_code = p.code where !isnull(x."+dtable+") and x.gindex = "+group_index+" group by a.parameter,a.levelType_codes");
	  }
	  else {
	    query.set("select a.parameter,a.levelType_codes,min(a.start_date),max(a.end_date) from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on x."+dtable+" = p."+ID_type+"ID left join "+db+".ds"+dsnum2+"_agrids as a on a."+ID_type+"ID_code = p.code where !isnull(x."+dtable+") and x.gindex = "+group_index+" group by a.parameter,a.levelType_codes");
	  }
	}
    }
    else {
	if (format_query.num_rows() > 1) {
	  query.set("select a.parameter,a.levelType_codes,min(a.start_date),max(a.end_date) from "+db+".ds"+dsnum2+"_agrids as a left join "+db+".ds"+dsnum2+table+" as p on p.code = a."+ID_type+"ID_code where p.format_code = "+row[1]+" group by a.parameter,a.levelType_codes");
	}
	else {
	  query.set("select parameter,levelType_codes,min(start_date),max(end_date) from "+db+".ds"+dsnum2+"_agrids group by parameter,levelType_codes");
	}
    }
    if (query.submit(server) < 0) {
	metautils::log_error("generate_detailed_grid_summary(): "+query.error(),caller,user);
    }
    while (query.fetch_row(row2)) {
	if (!parameter_description_hash.found(row2[0],he2)) {
	  he2.key=row2[0];
	  he2.description.reset(new std::string);
	  *he2.description=metatranslations::detailed_parameter(parameter_mapper,row[0],row2[0]);
	  parameter_description_hash.insert(he2);
	}
	pe.key=*he2.description;
	if (!detailed_parameter_table.found(*he2.description,pe)) {
	  pe.key=*he2.description;
	  pe.parameter_code_table.reset(new my::map<UniqueEntry>(99999));
	  ue.key=row2[0];
	  pe.parameter_code_table->insert(ue);
	  pe.level_table.reset(new my::map<LevelEntry>(99999));
	  pe.level_values.reset(new std::vector<size_t>);
	  bitmap::uncompress_values(row2[1],*pe.level_values);
	  for (auto& level_value : *pe.level_values) {
	    le.key=level_value;
	    le.range.reset(new LevelEntry::Range);
	    le.range->start=row2[2];
	    le.range->end=row2[3];
	    pe.level_table->insert(le);
	    lse.key=level_value;
	    if (!detailed_level_table.found(lse.key,lse)) {
		lse.data.reset(new LevelSummaryEntry::Data);
		level_code_table.found(level_value,lce);
		lse.data->map=lce.data->map;
		lse.data->type=lce.data->type;
		lse.data->value=lce.data->value;
		detailed_level_table.insert(lse);
	    }
	    if (!lse.data->variable_table.found(pe.key,vse)) {
		vse.key=pe.key;
		vse.data.reset(new VariableSummaryEntry::Data);
		vse.data->start=le.range->start;
		vse.data->end=le.range->end;
		lse.data->variable_table.insert(vse);
	    }
	  }
	  detailed_parameter_table.insert(pe);
	}
	else {
	  if (!pe.parameter_code_table->found(row2[0],ue)) {
	    ue.key=row2[0];
	    pe.parameter_code_table->insert(ue);
	  }
	  bitmap::uncompress_values(row2[1],level_values);
	  for (auto& level_value : level_values) {
	    lse.key=level_value;
	    if (!pe.level_table->found(level_value,le)) {
		le.key=level_value;
		le.range.reset(new LevelEntry::Range);
		le.range->start=row2[2];
		le.range->end=row2[3];
		pe.level_table->insert(le);
		pe.level_values->emplace_back(le.key);
		if (!detailed_level_table.found(lse.key,lse)) {
		  lse.data.reset(new LevelSummaryEntry::Data);
		  level_code_table.found(level_value,lce);
		  lse.data->map=lce.data->map;
		  lse.data->type=lce.data->type;
		  lse.data->value=lce.data->value;
		  detailed_level_table.insert(lse);
		}
		if (!lse.data->variable_table.found(pe.key,vse)) {
		  vse.key=pe.key;
		  vse.data.reset(new VariableSummaryEntry::Data);
		  vse.data->start=le.range->start;
		  vse.data->end=le.range->end;
		  lse.data->variable_table.insert(vse);
		}
	    }
	    else {
		if (row2[2] < le.range->start) {
		  le.range->start=row2[2];
		}
		if (row2[3] > le.range->end) {
		  le.range->end=row2[3];
		}
		detailed_level_table.found(lse.key,lse);
		lse.data->variable_table.found(pe.key,vse);
		if (le.range->start < vse.data->start) {
		  vse.data->start=le.range->start;
		}
		if (le.range->end > vse.data->end) {
		  vse.data->end=le.range->end;
		}
	    }
	  }
	}
    }
    for (auto& hkey : parameter_description_hash.keys()) {
	parameter_description_hash.found(hkey,he2);
	he2.description.reset();
    }
    parameter_description_hash.clear();
    ofs_p << "<div id=\""+row[0]+"_anchor\"></div>";
    ofs_l << "<div id=\""+row[0]+"_anchor\"></div>";
    if (format_list.size() > 1) {
	ofs_p << "<table width=\"100%\" border=\"0\"><tr><td>" << std::endl;
	ofs_l << "<table width=\"100%\" border=\"0\"><tr><td>" << std::endl;
	for (auto format : format_list) {
	  if (row[0] != format) {
	    sdum=format;
	    strutils::replace_all(sdum,"proprietary","dataset-specific");
	    ofs_p << "<span class=\"paneltext\">Go to <a href=\"javascript:void(0)\" onClick=\"javascript:scrollTo('" << format << "_anchor')\">" << strutils::to_capital(sdum) << "</a> Summary</span><br>" << std::endl;
	    ofs_l << "<span class=\"paneltext\">Go to <a href=\"javascript:void(0)\" onClick=\"javascript:scrollTo('" << format << "_anchor')\">" << strutils::to_capital(sdum) << "</a> Summary</span><br>" << std::endl;
	  }
	}
	ofs_p << "</td></tr></table>" << std::endl;
	ofs_l << "</td></tr></table>" << std::endl;
    }
    sdum=row[0];
    strutils::replace_all(sdum,"proprietary","dataset-specific");
    ncols=2;
    if (row[0] == "WMO_GRIB1") {
	ncols=3;
    }
    ofs_p << "<table class=\"insert\" width=\"100%\" cellspacing=\"1\" cellpadding=\"5\" border=\"0\"><tr><th class=\"headerRow\" colspan=\"" << ncols << "\" align=\"center\">Summary for Grids in " << strutils::to_capital(sdum) << " Format</th></tr>" << std::endl;
    ofs_l << "<table class=\"insert\" width=\"100%\" cellspacing=\"1\" cellpadding=\"5\" border=\"0\"><tr><th class=\"headerRow\" colspan=\"" << ncols << "\" align=\"center\">Summary for Grids in " << strutils::to_capital(sdum) << " Format</th></tr>" << std::endl;
    if (group_index.empty()) {
	query.set("select distinct t.timeRange,d.definition,d.defParams from "+db+".summary as s left join "+db+".timeRanges as t on t.code = s.timeRange_code left join "+db+".gridDefinitions as d on d.code = s.gridDefinition_code where s.dsid = '"+meta_args.dsnum+"' and format_code = "+row[1]);
    }
    else {
	if (field_exists(server,db+".ds"+dsnum2+table,"dsid")) {
	  query.set("select distinct t.timeRange,d.definition,d.defParams from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on (x."+dtable+" = p."+ID_type+"ID and x.type = p.type and x.dsid = p.dsid) left join "+db+".ds"+dsnum2+"_grids as g on g."+ID_type+"ID_code = p.code left join "+db+".timeRanges as t on t.code = g.timeRange_code left join "+db+".gridDefinitions as d on d.code = g.gridDefinition_code where p.format_code = "+row[1]+" and !isnull(x."+dtable+") and x.gindex = "+group_index);
	}
	else {
	  query.set("select distinct t.timeRange,d.definition,d.defParams from (select p.code from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on x."+dtable+" = p."+ID_type+"ID where p.format_code = "+row[1]+" and !isnull(x."+dtable+") and x.gindex = "+group_index+") as p left join "+db+".ds"+dsnum2+"_grids as g on g."+ID_type+"ID_code = p.code left join "+db+".timeRanges as t on t.code = g.timeRange_code left join "+db+".gridDefinitions as d on d.code = g.gridDefinition_code");
	}
    }
    if (query.submit(server) < 0) {
	metautils::log_error("generate_detailed_grid_summary(): "+query.error(),caller,user);
    }
    ofs_p << "<tr class=\"bg0\"><td align=\"left\" colspan=\"" << ncols << "\"><b>Product and Coverage Information:</b><br>";
    ofs_l << "<tr class=\"bg0\"><td align=\"left\" colspan=\"" << ncols << "\"><b>Product and Coverage Information:</b><br>";
    if (query.num_rows() == 1) {
	query.fetch_row(row2);
	ofs_p << "There is one product and one geographical coverage for all of the grids in this format:<ul class=\"paneltext\"><span class=\"underline\">Product:</span> " << row2[0] << "<br><span class=\"underline\">Geographical Coverage:</span> " << gridutils::convert_grid_definition(row2[1]+"<!>"+row2[2]) << "</ul>";
	ofs_l << "There is one product and one geographical coverage for all of the grids in this format:<ul class=\"paneltext\"><span class=\"underline\">Product:</span> " << row2[0] << "<br><span class=\"underline\">Geographical Coverage:</span> " << gridutils::convert_grid_definition(row2[1]+"<!>"+row2[2]) << "</ul>";
    }
    else {
	ofs_p << "There are multiple products and/or geographical coverages for the grids in this format.  Click a variable name to see the available products and coverages for that variable.";
	ofs_l << "There are multiple products and/or geographical coverages for the grids in this format.  Click a vertical level description to see the available products and coverages for that vertical level.";
	product_view_format_list.emplace_back(row[0]+"<!>"+row[1]);
    }
    ofs_p << "</td></tr>" << std::endl;
    ofs_l << "</td></tr>" << std::endl;
    ofs_p << "<tr class=\"bg2\"><th align=\"left\">Variable</th>";
    ofs_l << "<tr class=\"bg2\"><th align=\"left\">Vertical Level</th>";
    if (row[0] == "WMO_GRIB1") {
	ofs_p << "<th>Parameter<br>Code</th>";
    }
    ofs_p << "<th align=\"left\"><table class=\"paneltext\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\"><tr valign=\"top\" class=\"bg2\"><th align=\"left\">Vertical Levels</th><th width=\"41%\" align=\"left\">Temporal Valid Range</th></tr></table></th></tr>" << std::endl;
    ofs_l << "<th width=\"60%\" align=\"left\"><table class=\"paneltext\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\"><tr valign=\"top\" class=\"bg2\"><th align=\"left\">Variables</th><th width=\"41%\" align=\"left\">Temporal Valid Range</th></tr></table></th></tr>" << std::endl;
    detailed_parameter_table.keysort(
    [](const std::string& left,const std::string& right) -> bool
    {
	return metacompares::default_compare(left,right);
    });
    cindex=0;
    level_description_hash.clear();
    my::map<UniqueEntry> unique_levels;
    for (const auto& pkey : detailed_parameter_table.keys()) {
	unique_levels.clear();
	detailed_parameter_table.found(pkey,pe);
	strutils::replace_all(pe.key,"&lt;","<");
	strutils::replace_all(pe.key,"&gt;",">");
	ofs_p << "<tr class=\"bg" << cindex << "\" valign=\"top\"><td>";
	if (query.num_rows() > 1) {
	  ofs_p << "<a title=\"Products and Coverages\" href=\"javascript:void(0)\" onClick=\"popModalWindowWithURL('/cgi-bin/transform?dsnum=" << meta_args.dsnum << "&view=prodcov&formatCode=" << row[1] << "&ftype=" << strutils::to_lower(file_type);
	  if (!group_index.empty()) {
	    ofs_p << "&gindex=" << group_index;
	  }
	  for (auto& ckey : pe.parameter_code_table->keys()) {
	    ofs_p << "&pcode=" << ckey;
	  }
	  ofs_p << "',950,600)\">" << pe.key << "</a>";
	}
	else {
	  ofs_p << pe.key;
	}
	for (auto& level_value : *pe.level_values) {
	  if (!level_description_hash.found(level_value,he)) {
	    level_code_table.found(level_value,lce);
	    he.key=level_value;
	    he.description.reset(new std::string);
	    *he.description=metatranslations::detailed_level(level_mapper,row[0],lce.data->map,lce.data->type,lce.data->value,true);
	    level_description_hash.insert(he);
	  }
	  ue.key=*he.description;
	  pe.level_table->found(level_value,le);
	  if (!unique_levels.found(ue.key,ue)) {
	    ue.data.reset(new UniqueEntry::Data);
	    ue.data->start=le.range->start;
	    ue.data->end=le.range->end;
	    unique_levels.insert(ue);
	  }
	  else {
	    if (le.range->start < ue.data->start) {
		ue.data->start=le.range->start;
	    }
	    if (le.range->end > ue.data->end) {
		ue.data->end=le.range->end;
	    }
	  }
	  le.range.reset();
	}
	pe.level_table->clear();
	pe.level_table.reset();
	pe.level_values->clear();
	pe.level_values.reset();
	if (unique_levels.size() > 10) {
	  ofs_p << "<br /><span style=\"font-size: 13px; color: #6a6a6a\">(" << unique_levels.size() << " levels; scroll to see all)</span>";
	}
	ofs_p << "</td>";
	if (row[0] == "WMO_GRIB1") {
	  idx=(pe.parameter_code_table->keys()).front().find(":");
	  ofs_p << "<td align=\"center\">" << (pe.parameter_code_table->keys()).front().substr(idx+1) << "</td>";
	}
	ofs_p << "<td>";
	if (unique_levels.size() > 10) {
	  ofs_p << "<div class=\"detail_scroll1\">";
	}
	else {
	  ofs_p << "<div class=\"detail_scroll2\">";
	}
	ofs_p << "<table style=\"font-size: 14px\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\">";
	unique_levels.keysort(
	[](const std::string& left,const std::string& right) -> bool
	{
	  return metacompares::compare_levels(left,right);
	});
	size_t cnt=unique_levels.size();
	size_t n=0;
	for (auto& ukey : unique_levels.keys()) {
	  unique_levels.found(ukey,ue);
	  ofs_p << "<tr valign=\"top\"><td style=\"border-bottom: " << static_cast<int>(n < cnt-1) << "px solid #96a4bf\" align=\"left\">&bull;&nbsp;" << strutils::substitute(ukey,"_"," ") << "</td><td style=\"border-bottom: " << static_cast<int>(n < cnt-1) << "px solid #96a4bf\">&nbsp;&nbsp;</td><td style=\"border-bottom: " << static_cast<int>(n < cnt-1) << "px solid #96a4bf\" width=\"40%\" align=\"left\"><nobr>" << string_ll_to_date_string(ue.data->start) << " to " << string_ll_to_date_string(ue.data->end) << "</nobr></td></tr>";
	  ue.data.reset();
	  ++n;
	}
	ofs_p << "</table></div></td></tr>" << std::endl;
	cindex=1-cindex;
    }
    my::map<CombinedLevelSummaryEntry> combined_level_table;
    CombinedLevelSummaryEntry clse;
    VariableSummaryEntry vse2;
    for (auto& dkey : detailed_level_table.keys()) {
	detailed_level_table.found(dkey,lse);
	clse.key=metatranslations::detailed_level(level_mapper,row[0],lse.data->map,lse.data->type,lse.data->value,true);
	if (!combined_level_table.found(clse.key,clse)) {
	  clse.data.reset(new CombinedLevelSummaryEntry::Data);
	  combined_level_table.insert(clse);
	}
	clse.data->map_list.emplace_back(lse.data->map+"[!]"+lse.data->type+"[!]"+lse.data->value);
	if (clse.data->variable_table.size() == 0) {
	  clse.data->variable_table=lse.data->variable_table;
	}
	else {
	  std::list<std::string> keys_to_remove;
	  for (auto& vkey : lse.data->variable_table.keys()) {
	    if (clse.data->variable_table.found(vkey,vse)) {
		lse.data->variable_table.found(vkey,vse2);
		if (vse2.data->start < vse.data->start) {
		  vse.data->start=vse2.data->start;
		}
		if (vse2.data->end > vse.data->end) {
		  vse.data->end=vse2.data->end;
		}
		vse2.data.reset();
		keys_to_remove.emplace_back(vse2.key);
	    }
	  }
	  for (auto& rkey : keys_to_remove) {
	    lse.data->variable_table.remove(rkey);
	  }
	  keys_to_remove.clear();
	  for (auto& vkey : lse.data->variable_table.keys()) {
	    lse.data->variable_table.found(vkey,vse);
	    clse.data->variable_table.insert(vse);
	    keys_to_remove.emplace_back(vkey);
	  }
	  for (auto& rkey : keys_to_remove) {
	    lse.data->variable_table.remove(rkey);
	  }
	  keys_to_remove.clear();
	}
	lse.data.reset();
    }
    detailed_level_table.clear();
    combined_level_table.keysort(
    [](const std::string& left,const std::string& right) -> bool
    {
	return metacompares::compare_levels(left,right);
    });
    for (auto& ckey : combined_level_table.keys()) {
	combined_level_table.found(ckey,clse);
	clse.data->variable_table.keysort(
	[](const std::string& left,const std::string& right) -> bool
	{
	  return  metacompares::default_compare(left,right);
	});
	ofs_l << "<tr class=\"bg" << cindex << "\" valign=\"top\"><td><table><tr valign=\"bottom\"><td>";
	if (query.num_rows() > 1) {
	  ofs_l << "<a title=\"Products and Coverages\" href=\"javascript:void(0)\" onClick=\"popModalWindowWithURL('/cgi-bin/transform?dsnum=" << meta_args.dsnum << "&view=prodcov&formatCode=" << row[1] << "&ftype=" << strutils::to_lower(file_type);
	  if (!group_index.empty()) {
	    ofs_l << "&gindex=" << group_index;
	  }
	  for (auto& map : clse.data->map_list) {
	    ofs_l << "&map=" << map;
	  }
	  ofs_l << "',950,600)\">" << ckey << "</a>";
	}
	else {
	  ofs_l << ckey;
	}
	if (clse.data->variable_table.size() > 10) {
	  ofs_l << "<br /><span style=\"font-size: 13px; color: #6a6a6a\">(" << clse.data->variable_table.size() << " variables; scroll to see all)</span>";
	}
	ofs_l << "</td></tr></table></td><td>";
	if (clse.data->variable_table.size() > 10) {
	  ofs_l << "<div class=\"detail_scroll1\">";
	}
	else {
	  ofs_l << "<div class=\"detail_scroll2\">";
	}
	ofs_l << "<table style=\"font-size: 14px\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\">";
	auto cnt=clse.data->variable_table.keys().size();
	size_t n=0;
	for (auto& vkey : clse.data->variable_table.keys()) {
	  clse.data->variable_table.found(vkey,vse);
	  ofs_l << "<tr valign=\"top\"><td style=\"border-bottom: " << static_cast<int>(n < cnt-1) << "px solid #96a4bf\" align=\"left\">&bull;&nbsp;" << vkey << "</td><td style=\"border-bottom: " << static_cast<int>(n < cnt-1) << "px solid #96a4bf\">&nbsp;&nbsp;</td><td style=\"border-bottom: " << static_cast<int>(n < cnt-1) << "px solid #96a4bf\" width=\"40%\" align=\"left\"><nobr>" << string_ll_to_date_string(vse.data->start) << " to " << string_ll_to_date_string(vse.data->end) << "</nobr></td></tr>";
	  vse.data.reset();
	  ++n;
	}
	clse.data->variable_table.clear();
	clse.data.reset();
	ofs_l << "</table></div></td></tr>" << std::endl;
	cindex=1-cindex;
    }
    combined_level_table.clear();
    detailed_parameter_table.clear();
    ofs_p << "</table>" << std::endl;
    ofs_l << "</table>" << std::endl;
  }
  ofs_p.close();
  ofs_l.close();
  if (product_view_format_list.size() > 0) {
    query.set("timeRange,code",db+".timeRanges");
    if (query.submit(server) < 0) {
	metautils::log_error("generate_detailed_grid_summary() was not able to build the product hash",caller,user);
    }
    while (query.fetch_row(row2)) {
	he.key=std::stoi(row2[1]);
	he.description.reset(new std::string);
	product_hash.insert(he);
	*he.description=row2[0];
    }
    query.set("definition,defParams,code",db+".gridDefinitions");
    if (query.submit(server) < 0) {
	metautils::log_error("generate_detailed_grid_summary() was not able to build the grid definition hash",caller,user);
    }
    while (query.fetch_row(row2)) {
	he.key=std::stoi(row2[2]);
	he.description.reset(new std::string);
	grid_definition_hash.insert(he);
	*he.description=gridutils::convert_grid_definition(row2[0]+"<!>"+row2[1]);
    }
    ofs_p.open((tdir.name()+"/metadata/product-detail.html").c_str());
    if (!ofs_p.is_open()) {
	metautils::log_error("generate_detailed_grid_summary(): unable to open output for product detail",caller,user);
    }
    format_query.rewind();
    while (format_query.fetch_row(row)) {
	product_table.clear();
	if (!group_index.empty()) {
	}
	else {
	  if (format_query.num_rows() > 1) {
	    query.set("select a.timeRange_codes,a.gridDefinition_codes,min(a.start_date),max(a.end_date) from "+db+".ds"+dsnum2+"_agrids as a left join "+db+".ds"+dsnum2+table+" as p on p.code = a."+ID_type+"ID_code where p.format_code = "+row[1]+" group by a.timeRange_codes,a.gridDefinition_codes");
	  }
	  else {
	    query.set("select timeRange_codes,gridDefinition_codes,min(start_date),max(end_date) from "+db+".ds"+dsnum2+"_agrids group by timeRange_codes,gridDefinition_codes");
	  }
	}
	if (query.submit(server) < 0) {
	  metautils::log_error("generate_detailed_grid_summary(): "+query.error(),caller,user);
	}
	while (query.fetch_row(row2)) {
	  bitmap::uncompress_values(row2[0],product_values);
	  bitmap::uncompress_values(row2[1],grid_values);
	  for (auto& pcode : product_values) {
	    if (!product_table.found(pcode,pse)) {
		pse.key=pcode;
		product_hash.found(pcode,he);
		pse.description=he.description;
		pse.grid_table.reset(new my::map<GridSummaryEntry>);
		product_table.insert(pse);
	    }
	    for (auto& gcode : grid_values) {
		if (!pse.grid_table->found(gcode,gse)) {
		  gse.key=gcode;
		  grid_definition_hash.found(gcode,he);
		  gse.description=he.description;
		  gse.start.reset(new std::string);
		  *gse.start=row2[2];
		  gse.end.reset(new std::string);
		  *gse.end=row2[3];
		  pse.grid_table->insert(gse);
		}
		else {
		  if (row2[2] < *gse.start) {
		    *gse.start=row2[2];
		  }
		  if (row2[3] > *gse.end) {
		    *gse.end=row2[3];
		  }
		}
	    }
	  }
	}
	if (group_index.empty()) {
	  query.set("select a.timeRange_codes,a.gridDefinition_codes,min(a.start_date),max(a.end_date) from "+db+".ds"+dsnum2+"_agrids as a left join "+db+".ds"+dsnum2+"_primaries as p on p.code = a."+ID_type+"ID_code where p.format_code = "+row[1]+" group by a.timeRange_codes,a.gridDefinition_codes");
	}
	ofs_p << "<div id=\""+row[0]+"_anchor\"></div>";
	if (format_list.size() > 1) {
	  ofs_p << "<table width=\"100%\" border=\"0\"><tr><td>" << std::endl;
	  for (const auto& format : format_list) {
	    if (row[0] != format) {
		sdum=format;
		strutils::replace_all(sdum,"proprietary","dataset-specific");
		ofs_p << "<span class=\"paneltext\">Go to <a href=\"javascript:void(0)\" onClick=\"javascript:scrollTo('" << format << "_anchor')\">" << strutils::to_capital(sdum) << "</a> Summary</span><br>" << std::endl;
	    }
	  }
	  ofs_p << "</td></tr></table>" << std::endl;
	}
	sdum=row[0];
	strutils::replace_all(sdum,"proprietary","dataset-specific");
	ofs_p << "<table class=\"insert\" width=\"100%\" cellspacing=\"1\" cellpadding=\"5\" border=\"0\"><tr><th class=\"headerRow\" colspan=\"2\" align=\"center\">Summary for Grids in " << strutils::to_capital(sdum) << " Format</th></tr>" << std::endl;
	if (group_index.empty()) {
	  query.set("select distinct parameter,levelType_codes from "+db+".summary where dsid = '"+meta_args.dsnum+"' and format_code = "+row[1]);
	}
	if (query.submit(server) < 0) {
	  metautils::log_error("generate_detailed_grid_summary(): "+query.error(),caller,user);
	}
	ofs_p << "<tr class=\"bg0\"><td align=\"left\" colspan=\"2\"><b>Parameter and Level Information:</b><br />";
	if (query.num_rows() > 1) {
	  ofs_p << "There are multiple parameters and/or vertical levels for the grids in this format.  Click a product description to see the available parameters and levels for that product.";
	}
	else {
	  query.fetch_row(row2);
	  bitmap::uncompress_values(row2[1],level_values);
	  if (level_values.size() > 1) {
	    ofs_p << "There are multiple parameters and/or vertical levels for the grids in this format.  Click a product description to see the available parameters and levels for that product.";
	  }
	  else {
	    ofs_p << "There is one parameter and one vertical level for all of the grids in this format:<ul class=\"paneltext\"><span class=\"underline\">Parameter:</span> " << row2[0] << "<br><span class=\"underline\">Vertical Level:</span> " << row2[1] << "</ul>";
	  }
	}
	ofs_p << "</td></tr>";
	ofs_p << "<tr class=\"bg2\"><th align=\"left\">Product</th><th align=\"left\"><table class=\"paneltext\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\"><tr valign=\"top\" class=\"bg2\"><th align=\"left\">Geographical Coverage</th><th width=\"41%\" align=\"left\">Temporal Valid Range</th></tr></table></th></tr>" << std::endl;
	psarray.clear();
	psarray.reserve(product_table.size());
	for (auto& key : product_table.keys()) {
	  product_table.found(key,pse);
	  psarray.emplace_back(pse);
	}
	binary_sort(psarray,
	[](const ProductSummaryEntry& left,const ProductSummaryEntry& right) -> bool
	{
	  return metacompares::compare_time_ranges(*left.description,*right.description);
	});
	cindex=0;
	for (auto& ps : psarray) {
	  ofs_p << "<tr class=\"bg" << cindex << "\" valign=\"top\"><td>";
	  if (query.num_rows() > 1) {
	    ofs_p << "<a title=\"Parameters and Vertical Levels\" href=\"javascript:void(0)\" onClick=\"popModalWindowWithURL('/cgi-bin/transform?dsnum=" << meta_args.dsnum << "&view=varlev&formatCode=" << row[1] << "&ftype=" << strutils::to_lower(file_type);
	    if (!group_index.empty()) {
		ofs_p << "&gindex=" << group_index;
	    }
	    ofs_p << "&tcode=" << ps.key << "',950,600)\">" << *ps.description << "</a>";
	  }
	  else {
	    ofs_p << *ps.description;
	  }
	  ofs_p << "</td><td><table style=\"font-size: 14px\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\">";
	  gsarray.clear();
	  gsarray.reserve(ps.grid_table->size());
	  for (auto& gkey : ps.grid_table->keys()) {
	    ps.grid_table->found(gkey,gse);
	    gsarray.emplace_back(gse);
	  }
	  binary_sort(gsarray,
	  [](const GridSummaryEntry& left,const GridSummaryEntry& right) -> bool
	  {
	    return metacompares::compare_grid_definitions(*left.description,*right.description);
	  });
	  auto cnt=ps.grid_table->keys().size();
	  size_t n=0;
	  for (const auto& gs : gsarray) {
	    ofs_p << "<tr valign=\"top\"><td style=\"border-bottom: " << static_cast<int>(n < cnt-1) << "px solid #96a4bf\" align=\"left\">&bull;&nbsp;" << *gs.description << "</td><td style=\"border-bottom: " << static_cast<int>(n < cnt-1) << "px solid #96a4bf\">&nbsp;&nbsp;</td><td style=\"border-bottom: " << static_cast<int>(n < cnt-1) << "px solid #96a4bf\" width=\"40%\" align=\"left\"><nobr>" << string_ll_to_date_string(*gs.start) << " to " << string_ll_to_date_string(*gs.end) << "</nobr></td></tr>";
	    ++n;
	  }
	  ofs_p << "</table></td></tr>";
	  cindex=1-cindex;
	}
    }
    ofs_p.close();
  }
  if (unixutils::rdadata_sync(tdir.name(),"metadata/","/data/web/datasets/ds"+meta_args.dsnum,meta_directives.rdadata_home,error) < 0) {
    metautils::log_warning("generate_detailed_grid_summary() couldn't sync detail files - rdadata_sync error(s): '"+error+"'",caller,user);
  }
  server.disconnect();
  ofs << "<style id=\"detail\">" << std::endl;
  ofs << "  @import url(/css/transform.css);" << std::endl;
  ofs << "</style>" << std::endl;
  ofs << "<style id=\"view_button\">" << std::endl;
  ofs << "div.view_button_on," << std::endl;
  ofs << "div.view_button_off {" << std::endl;
  ofs << "  position: relative;" << std::endl;
  ofs << "  float: left;" << std::endl;
  ofs << "  padding: 5px;" << std::endl;
  ofs << "  border: 2px solid #272264;" << std::endl;
  ofs << "  font-size: 14px;" << std::endl;
  ofs << "  margin-left: 15px;" << std::endl;
  ofs << "}" << std::endl;
  ofs << "div.view_button_on {" << std::endl;
  ofs << "  background-color: #27aae0;" << std::endl;
  ofs << "  color: white;" << std::endl;
  ofs << "  font-weight: bold;" << std::endl;
  ofs << "}" << std::endl;
  ofs << "div.view_button_on:after {" << std::endl;
  ofs << "  content: '';" << std::endl;
  ofs << "  display: block;" << std::endl;
  ofs << "  position: absolute;" << std::endl;
  ofs << "  width: 0px;" << std::endl;
  ofs << "  height: 0px;" << std::endl;
  ofs << "  border-style: solid;" << std::endl;
  ofs << "  border-width: 10px 10px 0px 10px;" << std::endl;
  ofs << "  border-color: #272264 transparent transparent transparent;" << std::endl;
  ofs << "  top: 100%;" << std::endl;
  ofs << "  left: 50%;" << std::endl;
  ofs << "  margin-left: -10px;" << std::endl;
  ofs << "}" << std::endl;
  ofs << "div.view_button_off {" << std::endl;
  ofs << "  background-color: #b8edab;" << std::endl;
  ofs << "}" << std::endl;
  ofs << "div.view_button_off:hover {" << std::endl;
  ofs << "  background-color: #27aae0;" << std::endl;
  ofs << "  color: white;" << std::endl;
  ofs << "  cursor: pointer;" << std::endl;
  ofs << "}" << std::endl;
  ofs << "</style>" << std::endl;
  ofs << "<script id=\"view_script\">" << std::endl;
  ofs << "function changeView(e,v) {" << std::endl;
  ofs << "  if (e.className == 'view_button_off') {" << std::endl;
  ofs << "    var x=document.getElementsByClassName('view_button_on');" << std::endl;
  ofs << "    for (n=0; n < x.length; ++n) {" << std::endl;
  ofs << "        x[n].className='view_button_off';" << std::endl;
  ofs << "    }" << std::endl;
  ofs << "    e.className='view_button_on';" << std::endl;
  ofs << "    getAjaxContent('GET',null,'/datasets/ds" << meta_args.dsnum << "/metadata/'+v,'detail_content');" << std::endl;
  ofs << "  }" << std::endl;
  ofs << "}" << std::endl;
  ofs << "</script>" << std::endl;
  ofs << "<br />" << std::endl;
  ofs << "<center><table width=\"95%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\">" << std::endl;
  ofs << "<tr><td>" << std::endl;
  ofs << "<?php" << std::endl;
  ofs << "  if (isset($_GET[\"view\"])) {" << std::endl;
  ofs << "    $view=$_GET[\"view\"];" << std::endl;
  ofs << "  }" << std::endl;
  ofs << "  else {" << std::endl;
  ofs << "    $view=\"parameter\";" << std::endl;
  ofs << "  }" << std::endl;
  ofs << "  if (strcmp($view,\"parameter\") == 0) {" << std::endl;
  ofs << "?>" << std::endl;
  ofs << "<div class=\"view_button_on\" onClick=\"changeView(this,'parameter-detail.html')\">Parameter View</div>" << std::endl;
  ofs << "<?php" << std::endl;
  ofs << "  }" << std::endl;
  ofs << "  else {" << std::endl;
  ofs << "?>" << std::endl;
  ofs << "<div class=\"view_button_off\" onClick=\"changeView(this,'parameter-detail.html')\">Parameter View</div>" << std::endl;
  ofs << "<?php" << std::endl;
  ofs << "  }" << std::endl;
  ofs << "  if (strcmp($view,\"level\") == 0) {" << std::endl;
  ofs << "?>" << std::endl;
  ofs << "<div class=\"view_button_on\" onClick=\"changeView(this,'level-detail.html')\">Vertical Level View</div>" << std::endl;
  ofs << "<?php" << std::endl;
  ofs << "  }" << std::endl;
  ofs << "  else {" << std::endl;
  ofs << "?>" << std::endl;
  ofs << "<div class=\"view_button_off\" onClick=\"changeView(this,'level-detail.html')\">Vertical Level View</div>" << std::endl;
  ofs << "<?php" << std::endl;
  ofs << "  }" << std::endl;
  if (product_view_format_list.size() > 0) {
    ofs << "  if (strcmp($view,\"product\") == 0) {" << std::endl;
    ofs << "?>" << std::endl;
    ofs << "<div class=\"view_button_on\" onClick=\"changeView(this,'product-detail.html')\">Product View</div>" << std::endl;
    ofs << "<?php" << std::endl;
    ofs << "  }" << std::endl;
    ofs << "  else {" << std::endl;
    ofs << "?>" << std::endl;
    ofs << "<div class=\"view_button_off\" onClick=\"changeView(this,'product-detail.html')\">Product View</div>" << std::endl;
    ofs << "<?php" << std::endl;
    ofs << "  }" << std::endl;
  }
  ofs << "?>" << std::endl;
  ofs << "<div id=\"detail_content\" style=\"margin-top: 5%\"><center><img src=\"/images/loader.gif\" /><br ><span style=\"color: #a0a0a0\">Loading...</span></center></div>" << std::endl;
  ofs << "</td></tr>" << std::endl;
  ofs << "</table></center>" << std::endl;
  ofs << "<img src=\"/images/transpace.gif\" width=\"1\" height=\"0\" onLoad=\"getAjaxContent('GET',null,'/datasets/ds" << meta_args.dsnum << "/metadata/<?php echo $view; ?>-detail.html','detail_content')\" />" << std::endl;
}

void generate_detailed_observation_summary(MySQL::LocalQuery& format_query,std::string file_type,std::string group_index,std::ofstream& ofs,std::list<std::string>& format_list,std::string caller,std::string user)
{
  std::string dsnum2=strutils::substitute(meta_args.dsnum,".","");
  MySQL::Row row,row2;
  MySQL::Query query;
  obsData::TypeEntry te,te2;
  xmlutils::DataTypeMapper data_type_mapper(meta_directives.parameter_map_path);

  std::string db,table,dtable,ID_type;
  if (file_type == "MSS") {
    db="ObML";
    table="_primaries";
    dtable="mssfile";
    ID_type="mss";
  }
  else if (file_type == "Web") {
    db="WObML";
    table="_webfiles";
    dtable="wfile";
    ID_type="web";
  }
  MySQL::Server server(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  format_query.rewind();
  my::map<obsData::TypeEntry> platform_table;
  while (format_query.fetch_row(row)) {
    platform_table.clear();
    auto data_format=row[0];
    ofs << "<br><center><table class=\"insert\" width=\"95%\" cellspacing=\"1\" cellpadding=\"5\" border=\"0\"><tr><th class=\"headerRow\" colspan=\"4\" align=\"center\">Summary for Platform Observations in " << strutils::to_capital(strutils::substitute(data_format,"proprietary_","")) << " Format</th></tr>" << std::endl;
    auto cindex=0;
    ofs << "<tr class=\"bg2\" valign=\"top\"><th align=\"left\">Observing Platform</th><th align=\"left\">Observation Type<ul class=\"paneltext\"><li>Data Types</li></ul></th><td align=\"left\"><b>Average Frequency of Data</b><br>(varies by individual platform ID and data type)</td><td align=\"left\"><b>Temporal/Geographical Coverage</b><br>(each dot represents a 3&deg; box containing one or more observations)</td></tr>" << std::endl;
    if (group_index.empty()) {
	if (MySQL::table_exists(server,db+".ds"+dsnum2+"_dataTypes2")) {
	  query.set("select distinct platformType,obsType,dataType,min(start_date),max(end_date) from "+db+".ds"+dsnum2+"_dataTypes2 as d left join "+db+".ds"+dsnum2+"_dataTypesList as l on l.code = d.dataType_code left join "+db+".ds"+dsnum2+table+" as p on p.code = d."+ID_type+"ID_code left join "+db+".platformTypes as t on t.code = l.platformType_code left join "+db+".obsTypes as o on o.code = l.observationType_code where p.format_code = "+row[1]+" group by platformType,obsType,dataType order by platformType,obsType,dataType");
	}
	else {
	  query.set("select distinct platformType,obsType,dataType,min(start_date),max(end_date) from "+db+".ds"+dsnum2+"_dataTypes as d left join "+db+".ds"+dsnum2+table+" as p on p.code = d."+ID_type+"ID_code left join "+db+".platformTypes as t on t.code = d.platformType_code left join "+db+".obsTypes as o on o.code = d.observationType_code where p.format_code = "+row[1]+" group by platformType,obsType,dataType order by platformType,obsType,dataType");
	}
    }
    else {
	if (MySQL::table_exists(server,db+".ds"+dsnum2+"_dataTypes2")) {
	  query.set("select distinct platformType,obsType,dataType,min(start_date),max(end_date) from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on x."+dtable+" = p."+ID_type+"ID left join "+db+".ds"+dsnum2+"_dataTypes2 as d on d."+ID_type+"ID_code = p.code left join "+db+".ds"+dsnum2+"_dataTypesList as l on l.code = d.dataType_code left join "+db+".platformTypes as t on t.code = l.platformType_code left join "+db+".obsTypes as o on o.code = l.observationType_code where x.gindex = "+group_index+" and p.format_code = "+row[1]+" group by platformType,obsType,dataType order by platformType,obsType,dataType");
	}
	else {
	  query.set("select distinct platformType,obsType,dataType,min(start_date),max(end_date) from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on x."+dtable+" = p."+ID_type+"ID left join "+db+".ds"+dsnum2+"_dataTypes as d on d."+ID_type+"ID_code = p.code left join "+db+".platformTypes as t on t.code = d.platformType_code left join "+db+".obsTypes as o on o.code = d.observationType_code where x.gindex = "+group_index+" and p.format_code = "+row[1]+" group by platformType,obsType,dataType order by platformType,obsType,dataType");
	}
    }
    if (query.submit(server) < 0) {
	metautils::log_error("generate_detailed_observation_summary(): "+query.error()+" for "+query.show(),caller,user);
    }
    while (query.fetch_row(row2)) {
	if (!platform_table.found(row2[0],te)) {
// te is the entry for an individual platform; te.type_table_keys is the list of
//   corresponding observation types
	  te.key=row2[0];
	  te.type_table.reset(new my::map<obsData::TypeEntry>);
	  te.type_table_keys.reset(new std::list<std::string>);
// te2 is the entry for an individual observation type; te2.type_table_keys is
//   the list of corresponding dataTypes (don't need te2.type_table for
//   uniqueness because query guarantees each dataType will be unique
	  te2.key=row2[1];
	  te2.start=row2[3];
	  te2.end=row2[4];
	  te2.type_table_keys.reset(new std::list<std::string>);
	  te2.type_table_keys->emplace_back(row2[2]);
	  te2.type_table_keys_2=nullptr;
	  te.type_table->insert(te2);
	  te.type_table_keys->emplace_back(te2.key);
	  platform_table.insert(te);
	}
	else {
	  if (te.type_table->found(row2[1],te2)) {
	    if (row2[3] < te2.start) {
		te2.start=row2[3];
	    }
	    if (row2[4] > te2.end) {
		te2.end=row2[4];
	    }
	    te2.type_table_keys->emplace_back(row2[2]);
	    te.type_table->replace(te2);
	  }
	  else {
	    te2.key=row2[1];
	    te2.start=row2[3];
	    te2.end=row2[4];
	    te2.type_table_keys.reset(new std::list<std::string>);
	    te2.type_table_keys->emplace_back(row2[2]);
	    te2.type_table_keys_2=nullptr;
	    te.type_table->insert(te2);
	    te.type_table_keys->emplace_back(te2.key);
	  }
	  platform_table.replace(te);
	}
    }
    if (group_index.empty()) {
	if (field_exists(server,db+".ds"+dsnum2+table,"min_obs_per")) {
	  query.set("select any_value(l.platformType),any_value(o.obsType),min(f.min_obs_per),max(f.max_obs_per),f.unit from "+db+".ds"+dsnum2+table+" as p join "+db+".ds"+dsnum2+"_frequencies as f on p.code = f."+ID_type+"ID_code left join "+db+".obsTypes as o on o.code = f.observationType_code left join "+db+".platformTypes as l on l.code = f.platformType_code left join ObML.frequency_sort as s on s.keyword = f.unit where p.format_code = "+row[1]+" group by f.observationType_code,f.platformType_code,f.unit,s.idx order by s.idx");
	}
	else {
	  query.set("select any_value(l.platformType),any_value(o.obsType),min(f.avg_obs_per),max(f.avg_obs_per),f.unit from "+db+".ds"+dsnum2+table+" as p join "+db+".ds"+dsnum2+"_frequencies as f on p.code = f."+ID_type+"ID_code left join "+db+".obsTypes as o on o.code = f.observationType_code left join "+db+".platformTypes as l on l.code = f.platformType_code left join ObML.frequency_sort as s on s.keyword = f.unit where p.format_code = "+row[1]+" group by f.observationType_code,f.platformType_code,f.unit,s.idx order by s.idx");
	}
    }
    else {
	if (field_exists(server,db+".ds"+dsnum2+table,"min_obs_per")) {
	  query.set("select any_value(l.platformType),any_value(o.obsType),min(f.min_obs_per),max(f.max_obs_per),f.unit from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on x."+dtable+" = p."+ID_type+"ID left join "+db+".ds"+dsnum2+"_frequencies as f on p.code = f."+ID_type+"ID_code left join "+db+".obsTypes as o on o.code = f.observationType_code left join "+db+".platformTypes as l on l.code = f.platformType_code left join ObML.frequency_sort as s on s.keyword = f.unit where x.gindex = "+group_index+" and p.format_code = "+row[1]+" group by f.observationType_code,f.platformType_code,f.unit,s.idx order by s.idx");
	}
	else {
	  query.set("select any_value(l.platformType),any_value(o.obsType),min(f.avg_obs_per),max(f.avg_obs_per),f.unit from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on x."+dtable+" = p."+ID_type+"ID left join "+db+".ds"+dsnum2+"_frequencies as f on p.code = f."+ID_type+"ID_code left join "+db+".obsTypes as o on o.code = f.observationType_code left join "+db+".platformTypes as l on l.code = f.platformType_code left join ObML.frequency_sort as s on s.keyword = f.unit where x.gindex = "+group_index+" and p.format_code = "+row[1]+" group by f.observationType_code,f.platformType_code,f.unit,s.idx order by s.idx");
	}
    }
    if (query.submit(server) < 0)
	metautils::log_error("generate_detailed_observation_summary(): "+query.error()+" for "+query.show(),caller,user);
    while (query.fetch_row(row2)) {
	if (platform_table.found(row2[0],te)) {
	  te.type_table->found(row2[1],te2);
	  if (te2.type_table_keys_2 == NULL) {
	    te2.type_table_keys_2.reset(new std::list<std::string>);
	  }
	  if (row2[2] == row2[3]) {
	    te2.type_table_keys_2->emplace_back("<nobr>"+row2[2]+" per "+row2[4]+"</nobr>");
	  }
	  else {
	    te2.type_table_keys_2->emplace_back("<nobr>"+row2[2]+" to "+row2[3]+"</nobr> per "+row2[4]);
	  }
	  te.type_table->replace(te2);
	  platform_table.replace(te);
	}
    }
    my::map<UniqueEntry> unique_data_types;
    for (const auto& key : platform_table.keys()) {
	unique_data_types.clear();
	platform_table.found(key,te);
	auto n=0;
	for (const auto& key2 : *te.type_table_keys) {
	  if (n == 0) {
	    ofs << "<tr class=\"bg" << cindex << "\" valign=\"top\"><td rowspan=\"" << te.type_table->size() << "\">" << strutils::to_capital(te.key) << "</td>";
	  }
	  else {
	    ofs << "<tr class=\"bg" << cindex << "\" valign=\"top\">";
	  }
	  te2.key=key2;
	  te.type_table->found(te2.key,te2);
	  ofs << "<td>" << strutils::to_capital(te2.key) << "<ul class=\"paneltext\">";
	  for (const auto& key3 : *te2.type_table_keys) {
	    UniqueEntry ue;
	    ue.key=metatranslations::detailed_datatype(data_type_mapper,row[0],key3);
	    if (!unique_data_types.found(ue.key,ue)) {
		ofs << "<li>" << ue.key << "</li>";
		unique_data_types.insert(ue);
	    }
	  }
	  ofs << "</ul></td><td><ul class=\"paneltext\">";
	  if (te2.type_table_keys_2 != NULL) {
	    for (const auto& key3 : *te2.type_table_keys_2) {
		ofs << "<li>" << key3 << "</li>";
	    }
	  }
	  ofs << "</ul></td><td align=\"center\">" << string_ll_to_date_string(te2.start) << " to " << string_ll_to_date_string(te2.end);
	  if (group_index.empty() && unixutils::exists_on_server(meta_directives.web_server,"/data/web/datasets/ds"+meta_args.dsnum+"/metadata/"+te2.key+"."+te.key+".kml",meta_directives.rdadata_home)) {
	    ofs << "&nbsp;<a href=\"/datasets/ds" << meta_args.dsnum << "/metadata/" << te2.key << "." << te.key << ".kml\"><img src=\"/images/kml.gif\" width=\"36\" height=\"14\" hspace=\"3\" border=\"0\" title=\"See stations plotted in Google Earth\"></a>";
	  }
	  ofs << "<br><img src=\"/datasets/ds" << meta_args.dsnum << "/metadata/spatial_coverage." << strutils::substitute(data_format," ","_") << "_" << ID_type;
	  if (!group_index.empty()) {
	    ofs << "_gindex_" << group_index;
	  }
	  ofs << "_" << key2 << "." << key << ".gif?" << strutils::strand(5) << "\"></td></tr>" << std::endl;
	  te2.type_table_keys->clear();
	  te2.type_table_keys.reset();
	  ++n;
	}
	te.type_table->clear();
	te.type_table.reset();
	te.type_table_keys->clear();
	te.type_table_keys.reset();
	cindex=1-cindex;
    }
    ofs << "</table></center>" << std::endl;
  }
  server.disconnect();
}

void generate_detailed_fix_summary(MySQL::LocalQuery& format_query,std::string file_type,std::string group_index,std::ofstream& ofs,std::list<std::string>& format_list,std::string caller,std::string user)
{
  std::string dsnum2=strutils::substitute(meta_args.dsnum,".","");
  MySQL::Row row,row2;
  MySQL::Query query;
  obsData::TypeEntry te,te2;

  std::string db,table,dtable,ID_type;
  if (file_type == "MSS") {
    db="FixML";
    table="_primaries";
    dtable="mssfile";
    ID_type="mss";
  }
  else if (file_type == "Web") {
    db="WFixML";
    table="_webfiles";
    dtable="wfile";
    ID_type="web";
  }
  MySQL::Server server(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  format_query.rewind();
  my::map<obsData::TypeEntry> platform_table;
  while (format_query.fetch_row(row)) {
    platform_table.clear();
    auto format=row[0];
    strutils::replace_all(format,"proprietary_","");
    ofs << "<br><center><table class=\"insert\" width=\"95%\" cellspacing=\"1\" cellpadding=\"5\" border=\"0\"><tr><th class=\"headerRow\" colspan=\"3\" align=\"center\">Summary for Cyclone Fixes in " << strutils::to_capital(format) << " Format</th></tr>" << std::endl;
    auto cindex=0;
    ofs << "<tr class=\"bg2\" valign=\"top\"><th align=\"left\">Cyclone Classification</th><td align=\"left\"><b>Average Frequency of Data</b><br>(may vary by individual cyclone ID)</td><td align=\"left\"><b>Temporal/Geographical Coverage</b><br>(each dot represents a 3&deg; box containing one or more fixes)</td></tr>" << std::endl;
    if (group_index.empty())
	query.set("select distinct classification,min(l.start_date),max(l.end_date) from "+db+".ds"+dsnum2+"_locations as l left join "+db+".ds"+dsnum2+table+" as p on p.code = l."+ID_type+"ID_code left join "+db+".classifications as c on c.code = l.classification_code where p.format_code = "+row[1]+" group by classification order by classification");
    else
	query.set("select distinct classification,min(l.start_date),max(l.end_date) from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on x."+dtable+" = p."+ID_type+"ID left join "+db+".ds"+dsnum2+"_locations as l on l."+ID_type+"ID_code = p.code left join "+db+".classifications as c on c.code = l.classification_code where x.gindex = "+group_index+" and p.format_code = "+row[1]+" group by classification order by classification");
    if (query.submit(server) < 0) {
	metautils::log_error("generate_detailed_fix_summary(): "+query.error()+" for "+query.show(),caller,user);
    }
    while (query.fetch_row(row2)) {
	if (!platform_table.found(row2[0],te)) {
	  te.key=row2[0];
	  te.start=row2[1];
	  te.end=row2[2];
	  te.type_table_keys.reset(new std::list<std::string>);
	  platform_table.insert(te);
	}
	else {
	  if (row2[1] < te.start) {
	    te.start=row2[1];
	  }
	  if (row2[2] > te.end) {
	    te.end=row2[2];
	  }
	  platform_table.replace(te);
	}
    }
    if (group_index.empty()) {
	query.set("select any_value(c.classification),min(f.min_obs_per),max(f.max_obs_per),f.unit from "+db+".ds"+dsnum2+table+" as p left join "+db+".ds"+dsnum2+"_frequencies as f on p.code = f."+ID_type+"ID_code left join "+db+".classifications as c on c.code = f.classification_code left join ObML.frequency_sort as s on s.keyword = f.unit where p.format_code = "+row[1]+" group by f.classification_code,f.unit,s.idx order by s.idx");
    }
    else {
	query.set("select any_value(c.classification),min(f.min_obs_per),max(f.max_obs_per),f.unit from "+db+".ds"+dsnum2+table+" as p left join dssdb."+dtable+" as x on x."+dtable+" = p."+ID_type+"ID left join "+db+".ds"+dsnum2+"_frequencies as f on p.code = f."+ID_type+"ID_code left join "+db+".classifications as c on c.code = f.classification_code left join ObML.frequency_sort as s on s.keyword = f.unit where x.gindex = "+group_index+" and  p.format_code = "+row[1]+" group by f.classification_code,f.unit,s.idx order by s.idx");
    }
    if (query.submit(server) < 0) {
	metautils::log_error("generate_detailed_fix_summary(): "+query.error()+" for "+query.show(),caller,user);
    }
    while (query.fetch_row(row2)) {
	if (platform_table.found(row2[0],te)) {
	  if (row2[1] == row2[2]) {
	    te.type_table_keys->emplace_back("<nobr>"+row2[1]+" per "+row2[3]+"</nobr>");
	  }
	  else {
	    te.type_table_keys->emplace_back("<nobr>"+row2[1]+" to "+row2[2]+"</nobr> per "+row2[3]);
	  }
	}
    }
    for (const auto& key : platform_table.keys()) {
	platform_table.found(key,te);
	ofs << "<tr class=\"bg" << cindex << "\" valign=\"top\"><td>" << strutils::to_capital(te.key) << "</td><td><ul class=\"paneltext\">";
	for (auto& key2 : *te.type_table_keys) {
	  ofs << "<li>" << key2 << "</li>";
	}
	ofs << "</ul></td><td align=\"center\">" << string_ll_to_date_string(te.start) << " to " << string_ll_to_date_string(te.end) << "<br><img src=\"/datasets/ds" << meta_args.dsnum << "/metadata/spatial_coverage." << ID_type;
	if (!group_index.empty()) {
	  ofs << "_gindex_" << group_index;
	}
	ofs << "_" << te.key << ".gif?" << strutils::strand(5) << "\"></td></tr>" << std::endl;
	te.type_table_keys->clear();
	te.type_table_keys.reset();
	cindex=1-cindex;
    }
    ofs << "</table></center>" << std::endl;
  }
  server.disconnect();
}

void generate_detailed_metadata_view(std::string caller,std::string user)
{
  TempDir temp_dir;
  if (!temp_dir.create(meta_directives.temp_path)) {
    metautils::log_error("generate_detailed_metadata_view(): unable to create temporary directory",caller,user);
  }
  std::string xml_file;
  struct stat buf;
  if (stat((meta_directives.server_root+"/web/metadata/FormatReferences.xml").c_str(),&buf) == 0) {
    xml_file=meta_directives.server_root+"/web/metadata/FormatReferences.xml";
  }
  else {
    xml_file=unixutils::remote_web_file("http://rda.ucar.edu/metadata/FormatReferences.xml",temp_dir.name());
  }
  XMLDocument xdoc(xml_file);
  if (!xdoc) {
    metautils::log_error("generate_detailed_metadata_view(): unable to open FormatReferences.xml",caller,user);
  }
  MySQL::Server server_m(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  std::string dsnum2=strutils::substitute(meta_args.dsnum,".","");
  std::string formats,data_types;
  std::list<std::string> format_list;
  auto found_CMD_in_database=false;
  MySQL::LocalQuery GrML_query;
  if (table_exists(server_m,"GrML.ds"+dsnum2+"_primaries")) {
    found_CMD_in_database=true;
    data_types+="<li>Grids</li>";
    GrML_query.set("select distinct f.format,f.code from GrML.formats as f left join GrML.ds"+dsnum2+"_primaries as p on f.code = p.format_code where !isnull(p.format_code)");
    if (GrML_query.submit(server_m) < 0) {
	std::cerr << GrML_query.error() << std::endl;
	exit(1);
    }
    addToFormats(xdoc,GrML_query,format_list,formats);
  }
  MySQL::LocalQuery ObML_query;
  if (table_exists(server_m,"ObML.ds"+dsnum2+"_locations")) {
    found_CMD_in_database=true;
    data_types+="<li>Platform Observations</li>";
    ObML_query.set("select distinct f.format,f.code from ObML.formats as f left join ObML.ds"+dsnum2+"_primaries as p on f.code = p.format_code where !isnull(p.format_code)");
    if (ObML_query.submit(server_m) < 0) {
	std::cerr << ObML_query.error() << std::endl;
	exit(1);
    }
    addToFormats(xdoc,ObML_query,format_list,formats);
  }
  MySQL::LocalQuery FixML_query;
  if (table_exists(server_m,"FixML.ds"+dsnum2+"_primaries")) {
    found_CMD_in_database=true;
    data_types+="<li>Cyclone Fix</li>";
    FixML_query.set("select distinct f.format,f.code from FixML.formats as f left join FixML.ds"+dsnum2+"_primaries as p on f.code = p.format_code where !isnull(p.format_code)");
    if (FixML_query.submit(server_m) < 0) {
	std::cerr << FixML_query.error() << std::endl;
	exit(1);
    }
    addToFormats(xdoc,FixML_query,format_list,formats);
  }
  xdoc.close();
  if (stat((meta_directives.server_root+"/web/datasets/ds"+meta_args.dsnum+"/metadata/dsOverview.xml").c_str(),&buf) == 0) {
    xml_file=meta_directives.server_root+"/web/datasets/ds"+meta_args.dsnum+"/metadata/dsOverview.xml";
  }
  else {
    xml_file=unixutils::remote_web_file("http://rda.ucar.edu/datasets/ds"+meta_args.dsnum+"/metadata/dsOverview.xml",temp_dir.name());
  }
  xdoc.open(xml_file);
  if (!xdoc) {
    if (xml_file.empty()) {
	metautils::log_warning("generate_detailed_metadata_view() returned warning: unable to access dsOverview.xml from the web server",caller,user);
    }
    else {
	metautils::log_warning("generate_detailed_metadata_view() returned warning: unable to open "+xml_file+" - error: '"+xdoc.parse_error()+"'",caller,user);
    }
  }
  else {
    auto e=xdoc.element("dsOverview/title");
    MySQL::Server server_d(meta_directives.database_server,meta_directives.rdadb_username,meta_directives.rdadb_password,"dssdb");
    if (!found_CMD_in_database) {
	auto elist=xdoc.element_list("dsOverview/contentMetadata/dataType");
	for (const auto& element : elist) {
	  data_types+="<li>";
	  auto data_type=element.content();
	  if (data_type == "grid") {
	    data_types+="Grids";
	  }
	  else if (data_type == "platform_observation") {
	    data_types+="Platform Observations";
	  }
	  else {
	    data_types+=strutils::to_capital(data_type);
	  }
	  data_types+="</li>";
	}
	elist=xdoc.element_list("dsOverview/contentMetadata/format");
	for (auto element : elist) {
	  auto format=element.content();
	  strutils::replace_all(format,"proprietary_","");
	  formats+="<li>"+strutils::to_capital(format)+"</li>";
	}
    }
    TempDir tdir;
    if (!tdir.create(meta_directives.temp_path)) {
	metautils::log_error("generate_detailed_metadata_view(): unable to create temporary directory",caller,user);
    }
// create the directory tree in the temp directory
    std::stringstream oss,ess;
    if (unixutils::mysystem2("/bin/mkdir -p "+tdir.name()+"/metadata",oss,ess) < 0) {
	metautils::log_error("generate_detailed_metadata_view(): unable to create temporary directory tree - '"+ess.str()+"'",caller,user);
    }
    std::ofstream ofs((tdir.name()+"/metadata/detailed.html").c_str());
    if (!ofs.is_open()) {
	metautils::log_error("generate_detailed_metadata_view(): unable to open output for detailed.html",caller,user);
    }
    ofs << "<style id=\"detail\">" << std::endl;
    ofs << "  @import url(/css/transform.css);" << std::endl;
    ofs << "</style>" << std::endl;
    ofs << "<?php" << std::endl;
    ofs << "  if ( ($conn=mysql_connect(\"rda-db.ucar.edu\",\"metadata\",\"metadata\"))) {" << std::endl;
    ofs << "    $title=mysql_query(\"select title from search.datasets where dsid = '" << meta_args.dsnum << "'\");" << std::endl;
    ofs << "    $contributors=mysql_query(\"select g.path from search.contributors_new as c left join search.GCMD_providers as g on g.uuid = c.keyword where dsid = '" << meta_args.dsnum << "' order by disp_order\");" << std::endl;
    ofs << "    $projects=mysql_query(\"select g.path from search.projects_new as p left join search.GCMD_projects as g on g.uuid = p.keyword  where dsid = '" << meta_args.dsnum << "'\");" << std::endl;
    ofs << "    $supportedProjects=mysql_query(\"select g.path from search.supportedProjects_new as p left join search.GCMD_projects as g on g.uuid = p.keyword where dsid = '" << meta_args.dsnum << "'\");" << std::endl;
    ofs << "  }" << std::endl;
    ofs << "?>" << std::endl;
    ofs << "<p>The information presented here summarizes the data in the primary (NCAR HPSS) archive of ds"+meta_args.dsnum+".  Some or all of these data may not be directly accessible from our web server.  If you have questions about data access, please contact the dataset specialist named above.</p>" << std::endl;
    ofs << "<br>" << std::endl;
    ofs << "<center><table class=\"insert\" width=\"95%\" cellspacing=\"1\" cellpadding=\"5\" border=\"0\">" << std::endl;
    ofs << "<tr><th class=\"headerRow\" align=\"center\" colspan=\"2\">Overview</th></tr>" << std::endl;
    ofs << "<tr><td class=\"bg0\" align=\"left\" colspan=\"2\"><b>Dataset Title:</b>&nbsp;<?php print mysql_result($title,0,\"title\"); ?></td></tr>" << std::endl;
// citation
    ofs << "<tr><td class=\"bg0\" align=\"left\" colspan=\"2\"><b>Dataset Citation:</b><br /><div id=\"citation\" style=\"margin-bottom: 5px\"><img src=\"/images/transpace.gif\" width=\"1\" height=\"1\" onLoad=\"getAjaxContent('GET',null,'/cgi-bin/datasets/citation?dsnum=" << meta_args.dsnum << "&style=esip','citation')\" /></div><div style=\"display: inline; background-color: #2a70ae; color: white; padding: 1px 8px 1px 8px; font-size: 16px; font-weight: bold; border-radius: 5px 5px 5px 5px; text-align: center; cursor: pointer\" onClick=\"javascript:location='/cgi-bin/datasets/citation?dsnum=" << meta_args.dsnum << "&style=ris'\" title=\"download citation in RIS format\">RIS</div><div style=\"display: inline; background-color: #2a70ae; color: white; width: 60px; padding: 2px 8px 1px 8px; font-size: 16px; font-weight: bold; font-family: serif; border-radius: 5px 5px 5px 5px; text-align: center; cursor: pointer; margin-left: 7px\" onClick=\"location='/cgi-bin/datasets/citation?dsnum=" << meta_args.dsnum << "&style=bibtex'\" title=\"download citation in BibTeX format\">BibTeX</div></td></tr>" << std::endl;
    ofs << "<tr valign=\"top\">" << std::endl;
    if (!data_types.empty()) {
	ofs << "<td width=\"50%\" class=\"bg0\" align=\"left\"><b>Types of data:</b><ul class=\"paneltext\">"+data_types+"</ul></td>" << std::endl;
    }
    if (!formats.empty()) {
	ofs << "<td class=\"bg0\" align=\"left\"><b>Data formats:</b><ul class=\"paneltext\">"+formats+"</ul></td>" << std::endl;
    }
    ofs << "</tr>" << std::endl;
    ofs  << "<tr valign=\"top\"><td class=\"bg0\" colspan=\"2\"><b>Data contributors:</b><ul class=\"paneltext\">" << std::endl;
    ofs << "<?php" << std::endl;
    ofs << "  $num=mysql_numrows($contributors);" << std::endl;
    ofs << "  $n=0;" << std::endl;
    ofs << "  while ($n < $num) {" << std::endl;
    ofs << "    print \"<li>\" . mysql_result($contributors,$n,\"g.path\") . \"</li>\\n\";" << std::endl;
    ofs << "    $n++;" << std::endl;
    ofs << "  }" << std::endl;
    ofs << "?>" << std::endl;
    ofs << "</ul></td></tr>" << std::endl;
    ofs << "<?php" << std::endl;
    ofs << "  if ( ($num=mysql_numrows($projects)) > 0) {" << std::endl;
    ofs << "    print \"<tr valign=\\\"top\\\"><td class=\\\"bg0\\\" colspan=\\\"2\\\"><b>Programs/Experiments that collected the data:</b><ul class=\\\"paneltext\\\">\\n\";" << std::endl;
    ofs << "    $n=0;" << std::endl;
    ofs << "    while ($n < $num) {" << std::endl;
    ofs << "      print \"<li>\" . mysql_result($projects,$n,\"g.path\") . \"</li>\\n\";" << std::endl;
    ofs << "      $n++;" << std::endl;
    ofs << "    }" << std::endl;
    ofs << "    print \"</ul></td></tr>\\n\";" << std::endl;
    ofs << "  }" << std::endl;
    ofs << "?>" << std::endl;
    ofs << "<?php" << std::endl;
    ofs << "  if ( ($num=mysql_numrows($supportedProjects)) > 0) {" << std::endl;
    ofs << "    print \"<tr valign=\\\"top\\\"><td class=\\\"bg0\\\" colspan=\\\"2\\\"><b>Projects that are supported by the data:</b><ul class=\\\"paneltext\\\">\\n\";" << std::endl;
    ofs << "    $n=0;" << std::endl;
    ofs << "    while ($n < $num) {" << std::endl;
    ofs << "      print \"<li>\" . mysql_result($supportedProjects,$n,\"g.path\") . \"</li>\\n\";" << std::endl;
    ofs << "      $n++;" << std::endl;
    ofs << "    }" << std::endl;
    ofs << "    print \"</ul></td></tr>\\n\";" << std::endl;
    ofs << "  }" << std::endl;
    ofs << "?>" << std::endl;
    MySQL::LocalQuery query("primary_size","dataset","dsid = 'ds"+meta_args.dsnum+"'");
    if (query.submit(server_d) < 0) {
	std::cerr << query.error() << std::endl;
	exit(1);
    }
    double volume;
    MySQL::Row row;
    query.fetch_row(row);
    if (!row[0].empty()) {
	volume=std::stoll(row[0]);
    }
    else {
	volume=0.;
    }
    auto n=0;
    while ( (volume/1000.) >= 1.) {
	volume/=1000.;
	++n;
    }
    const int VUNITS_LEN=6;
    const char *vunits[VUNITS_LEN]={"","K","M","G","T","P"};
    if (n >= VUNITS_LEN) {
	metautils::log_error("generate_detailed_metadata_view() - dataset primary size exceeds volume units specification",caller,user);
    }
    ofs << "<tr valign=\"top\"><td class=\"bg0\" colspan=\"2\"><b>Total volume:</b>&nbsp;&nbsp;"+strutils::ftos(volume,5,3,' ')+" "+vunits[n]+"bytes</td></tr>" << std::endl;
    if (!found_CMD_in_database) {
	ofs << "<tr valign=\"top\">" << std::endl;
	auto elist=xdoc.element_list("dsOverview/contentMetadata/temporal");
	ofs << "<td class=\"bg0\"><b>Temporal Range(s):</b><ul>" << std::endl;
	for (const auto& element : elist) {
	  e=element;
	  auto date=e.attribute_value("start");
	  ofs << "<li>"+date+" to ";
	  date=e.attribute_value("end");
	  ofs << date+"</li>" << std::endl;
	}
	ofs << "</ul></td>" << std::endl;
	elist=xdoc.element_list("dsOverview/contentMetadata/detailedVariables/detailedVariable");
	if (elist.size() > 0) {
	  ofs << "<td class=\"bg0\"><b>Detailed Variable List:</b><ul>" << std::endl;
	}
	else {
	  ofs << "<td class=\"bg0\">&nbsp;" << std::endl;
	}
	for (const auto& element : elist) {
	  ofs << "<li>"+element.content();
	  auto units=element.attribute_value("units");
	  if (!units.empty()) {
	    ofs << " <small class=\"units\">("+htmlutils::transform_superscripts(units)+")</small>";
	  }
	  ofs << "</li>" << std::endl;
	}
	if (elist.size() > 0) {
	  ofs << "</ul></td>" << std::endl;
	}
	else {
	  ofs << "</td>" << std::endl;
	}
	ofs << "</tr>" << std::endl;
    }
    ofs << "</table></center>" << std::endl;
    if (GrML_query.num_rows() > 0) {
	generate_detailed_grid_summary(GrML_query,"MSS","",ofs,format_list,caller,user);
    }
    if (ObML_query.num_rows() > 0) {
	generate_detailed_observation_summary(ObML_query,"MSS","",ofs,format_list,caller,user);
    }
    if (FixML_query.num_rows() > 0) {
	generate_detailed_fix_summary(FixML_query,"MSS","",ofs,format_list,caller,user);
    }
    ofs.close();
    std::string error;
    if (unixutils::rdadata_sync(tdir.name(),"metadata/","/data/web/datasets/ds"+meta_args.dsnum,meta_directives.rdadata_home,error) < 0) {
	metautils::log_warning("generate_detailed_metadata_view() couldn't sync 'detailed.html' - rdadata_sync error(s): '"+error+"'",caller,user);
    }
    xdoc.close();
    if (server_d.update("dataset","meta_link = 'Y',version = version+1","dsid = 'ds"+meta_args.dsnum+"'") < 0) {
	metautils::log_warning("generate_detailed_metadata_view() returned warning: "+server_d.error()+" while trying to update dssdb.dataset (1)",caller,user);
    }
    server_d.disconnect();
  }
  server_m.disconnect();
// generate parameter cross-references
  generate_parameter_cross_reference("WMO_GRIB1","GRIB","grib.html",caller,user);
  generate_parameter_cross_reference("WMO_GRIB2","GRIB2","grib2.html",caller,user);
  generate_level_cross_reference("WMO_GRIB2","GRIB2","grib2_levels.html",caller,user);
  generate_parameter_cross_reference("NCEP_ON84","ON84","on84.html",caller,user);
//  generate_vtable(dsnum,caller,user,args);
}

void generate_group_detailed_metadata_view(std::string group_index,std::string file_type,std::string caller,std::string user)
{
  std::ofstream ofs;
  std::string dsnum2=strutils::substitute(meta_args.dsnum,".","");
  MySQL::LocalQuery query,GrML_query,ObML_query,FixML_query;
  MySQL::Row row;
  std::string gtitle,gsummary,output,error;
//  std::list<std::string> formatList;
//  bool foundDetail=false;

  MySQL::Server server_m(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  MySQL::Server server_d(meta_directives.database_server,meta_directives.rdadb_username,meta_directives.rdadb_password,"dssdb");
  if (!group_index.empty() || group_index == "0") {
    return;
  }
while (group_index != "0") {
if (server_d.update("dsgroup","meta_link = 'X'","dsid = 'ds"+meta_args.dsnum+"' and gindex = "+group_index) < 0) {
metautils::log_warning("generate_group_detailed_metadata_view() returned warning: "+server_d.error()+" while trying to update dssdb.dsgroup",caller,user);
}
if (server_d.update("dataset","version = version+1","dsid = 'ds"+meta_args.dsnum+"'") < 0) {
metautils::log_warning("generate_detailed_metadata_view() returned warning: "+server_d.error()+" while trying to update dssdb.dataset (2)",caller,user);
}
query.set("pindex","dsgroup","gindex = "+group_index);
if (query.submit(server_d) < 0) {
metautils::log_error("generate_group_detailed_metadata_view(): "+query.error(),caller,user);
}
if (query.fetch_row(row)) {
group_index=row[0];
}
else
group_index="0";
}
/*
  if (file_type == "MSS") {
    query.set("title,mnote","dsgroup","dsid = 'ds"+dsnum+"' and gindex = "+group_index);
    if (table_exists(server_m,"GrML.ds"+dsnum2+"_primaries")) {
	GrML_query.set("select distinct f.format,f.code from GrML.formats as f left join GrML.ds"+dsnum2+"_primaries as p on f.code = p.format_code left join dssdb.mssfile as m on m.mssfile = p.mssID where !isnull(p.format_code) and m.gindex = "+group_index);
	if (GrML_query.submit(server_m) < 0)
	  metautils::log_error("generate_group_detailed_metadata_view(): "+GrML_query.error(),caller,user,args);
    }
    if (table_exists(server_m,"ObML.ds"+dsnum2+"_primaries")) {
	ObML_query.set("select distinct f.format,f.code from ObML.formats as f left join ObML.ds"+dsnum2+"_primaries as p on f.code = p.format_code left join dssdb.mssfile as m on m.mssfile = p.mssID where !isnull(p.format_code) and m.gindex = "+group_index);
	if (ObML_query.submit(server_m) < 0)
	  metautils::log_error("generate_group_detailed_metadata_view(): "+ObML_query.error(),caller,user,args);
    }
    if (table_exists(server_m,"FixML.ds"+dsnum2+"_primaries")) {
	FixML_query.set("select distinct f.format,f.code from FixML.formats as f left join FixML.ds"+dsnum2+"_primaries as p on f.code = p.format_code left join dssdb.mssfile as m on m.mssfile = p.mssID where !isnull(p.format_code) and m.gindex = "+group_index);
	if (FixML_query.submit(server_m) < 0)
	  metautils::log_error("generate_group_detailed_metadata_view(): "+FixML_query.error(),caller,user,args);
    }
  }
  else if (file_type == "Web") {
    query.set("title,wnote","dsgroup","dsid = 'ds"+dsnum+"' and gindex = "+group_index);
    if (table_exists(server_m,"WGrML.ds"+dsnum2+"_webfiles")) {
	if (field_exists(server_m,"WGrML.ds"+dsnum2+"_webfiles","dsid"))
	  GrML_query.set("select distinct f.format,f.code from WGrML.formats as f left join WGrML.ds"+dsnum2+"_webfiles as w on f.code = w.format_code left join dssdb.wfile as x on (x.wfile = w.webID and x.type = w.type and x.dsid = w.dsid) where !isnull(w.format_code) and x.gindex = "+group_index);
	else
	  GrML_query.set("select distinct f.format,f.code from WGrML.formats as f left join WGrML.ds"+dsnum2+"_webfiles as w on f.code = w.format_code left join dssdb.wfile as x on x.wfile = w.webID where !isnull(w.format_code) and x.gindex = "+group_index);
	if (GrML_query.submit(server_m) < 0)
	  metautils::log_error("generate_group_detailed_metadata_view(): "+GrML_query.error(),caller,user,args);
    }
    if (table_exists(server_m,"WObML.ds"+dsnum2+"_webfiles")) {
	if (field_exists(server_m,"WObML.ds"+dsnum2+"_webfiles","dsid"))
	  ObML_query.set("select distinct f.format,f.code from WObML.formats as f left join WObML.ds"+dsnum2+"_webfiles as w on f.code = w.format_code left join dssdb.wfile as x on (x.wfile = w.webID and x.type = w.type and x.dsid = w.dsid) where !isnull(w.format_code) and x.gindex = "+group_index);
	else
	  ObML_query.set("select distinct f.format,f.code from WObML.formats as f left join WObML.ds"+dsnum2+"_webfiles as w on f.code = w.format_code left join dssdb.wfile as x on x.wfile = w.webID where !isnull(w.format_code) and x.gindex = "+group_index);
	if (ObML_query.submit(server_m) < 0)
	  metautils::log_error("generate_group_detailed_metadata_view(): "+ObML_query.error(),caller,user,args);
    }
    if (table_exists(server_m,"WFixML.ds"+dsnum2+"_webfiles")) {
	if (field_exists(server_m,"WObML.ds"+dsnum2+"_webfiles","dsid"))
	  FixML_query.set("select distinct f.format,f.code from WFixML.formats as f left join WFixML.ds"+dsnum2+"_webfiles as w on f.code = w.format_code left join dssdb.wfile as x on (x.wfile = w.webID and x.type = w.type and x.dsid = w.dsid) where !isnull(w.format_code) and x.gindex = "+group_index);
	else
	  FixML_query.set("select distinct f.format,f.code from WFixML.formats as f left join WFixML.ds"+dsnum2+"_webfiles as w on f.code = w.format_code left join dssdb.wfile as x on x.wfile = w.webID where !isnull(w.format_code) and x.gindex = "+group_index);
	if (FixML_query.submit(server_m) < 0)
	  metautils::log_error("generate_group_detailed_metadata_view(): "+FixML_query.error(),caller,user,args);
    }
  }
  if (query.submit(server_d) < 0)
    metautils::log_error("generate_group_detailed_metadata_view(): "+query.error(),caller,user,args);
  if (!query.fetch_row(row)) {
    metautils::log_error("generate_group_detailed_metadata_view(): no entry in RDADB for group index "+group_index,caller,user,args);
  }
  gtitle=row[0];
  gsummary=row[1];
  ofs.open(tfile->name().toChar());
  ofs << "<html>" << std::endl;
  ofs << "<head>" << std::endl;
  ofs << "<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/transform.css\">" << std::endl;
  ofs << "</head>" << std::endl;
  ofs << "<body>" << std::endl;
  ofs << "<br><center><big>" << gtitle << "</big></center>" << std::endl;
  if (!gsummary.empty())
    ofs << "<center><table><tr><td align=\"left\">" << gsummary << "</td></tr></table></center>" << std::endl;
  if (GrML_query.num_rows() > 0) {
    generate_detailed_grid_summary(dsnum,GrML_query,file_type,group_index,ofs,formatList,caller,user,args);
    foundDetail=true;
  }
  if (ObML_query.num_rows() > 0) {
    generate_detailed_observation_summary(dsnum,ObML_query,file_type,group_index,ofs,formatList,caller,user,args);
    foundDetail=true;
  }
  if (FixML_query.num_rows() > 0) {
    generate_detailed_fix_summary(dsnum,FixML_query,file_type,group_index,ofs,formatList,caller,user,args);
    foundDetail=true;
  }
  ofs << "</body></html>" << std::endl;
  ofs.close();
  if (foundDetail) {
    if (hostSync(tfile->name(),"/__HOST__/web/datasets/ds"+dsnum+"/metadata/"+file_type.toLower()+"_detailed_gindex_"+group_index+".html",error) < 0)
	metautils::log_warning("generate_detailed_metadata_view() couldn't sync '"+file_type.toLower()+"_detailed_gindex_"+group_index+".html' - hostSync error(s): '"+error+"'",caller,user,argsString);
  } 
  if (foundDetail) {
    if (file_type == "MSS") {
	if (server_d.update("dsgroup","meta_link = if (meta_link = 'W' or meta_link = 'B','B','M')","dsid = 'ds"+dsnum+"' and gindex = "+group_index) < 0)
	  metautils::log_warning("generate_group_detailed_metadata_view() returned warning: "+server_d.error(),caller,user,args);
    }
    else if (file_type == "Web") {
	if (server_d.update("dsgroup","meta_link = if (meta_link = 'M' or meta_link = 'B','B','W')","dsid = 'ds"+dsnum+"' and gindex = "+group_index) < 0)
	  metautils::log_warning("generate_group_detailed_metadata_view() returned warning: "+server_d.error(),caller,user,args);
	while (group_index != "0") {
	  query.set("pindex","dsgroup","gindex = "+group_index);
	  if (query.submit(server_d) < 0)
	    metautils::log_error("generate_group_detailed_metadata_view(): "+query.error(),caller,user,args);
	  if (query.fetch_row(row)) {
	    group_index=row[0];
	  }
	  else {
	    group_index="0";
	  }
	  if (group_index != "0") {
	    if (server_d.update("dsgroup","meta_link = 'X'","dsid = 'ds"+dsnum+"' and gindex = "+group_index) < 0)
		metautils::log_warning("generate_group_detailed_metadata_view() returned warning: "+server_d.error(),caller,user,args);
	  }
	}
    }
  }
  if (server_d.update("dataset","version = version+1","dsid = 'ds"+dsnum+"'") < 0)
    metautils::log_warning("generate_detailed_metadata_view() returned warning: "+server_d.error(),caller,user,args);
*/
  server_m.disconnect();
  server_d.disconnect();
}

} // end namespace detailedMetadata

} // end namespace summarizeMetadata
