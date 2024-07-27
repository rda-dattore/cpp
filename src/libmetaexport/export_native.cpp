#include <fstream>
#include <regex>
#include <sys/stat.h>
#include <metadata_export_pg.hpp>
#include <metadata.hpp>
#include <metahelpers.hpp>
#include <xml.hpp>
#include <PostgreSQL.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <tempfile.hpp>

using namespace PostgreSQL;

namespace metadataExport {

void add_frequency(my::map<Entry>& frequency_table,size_t frequency,std::string unit)
{
  Entry e;
  if (frequency == 1) {
    e.key=strutils::itos(frequency)+"<!>"+unit;
  }
  else {
    if (unit == "minute") {
	e.key=strutils::itos(lround(60./frequency+0.49))+"<!>second";
    }
    else if (unit == "hour") {
	e.key=strutils::itos(lround(60./frequency+0.49))+"<!>minute";
    }
    else if (unit == "day") {
	e.key=strutils::itos(lround(24./frequency+0.49))+"<!>hour";
    }
    else if (unit == "week") {
	e.key=strutils::itos(lround(7./frequency+0.49))+"<!>day";
    }
    else if (unit == "month") {
	e.key=strutils::itos(lround(30./frequency+0.49))+"<!>day";
    }
  }
  if (e.key.length() > 0 && !frequency_table.found(e.key,e)) {
    frequency_table.insert(e);
  }
}

extern "C" void *run_query(void *ts)
{
  PThreadStruct *t=(PThreadStruct *)ts;
  Server tserver(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"rdadb");
  t->query.submit(tserver);
  tserver.disconnect();
  return NULL;
}

bool export_to_native(std::ostream& ofs,std::string dsnum,XMLDocument& xdoc,size_t indent_length)
{
  std::string dsnum2=strutils::substitute(dsnum,".","");
  TempDir temp_dir;
  temp_dir.create("/tmp");
  Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"rdadb");
  std::string indent(indent_length,' ');
  XMLElement e=xdoc.element("dsOverview");
  ofs << indent << "<dsOverview xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << indent << "            xsi:schemaLocation=\"https://rda.ucar.edu/schemas" << std::endl;
  ofs << indent << "                                https://rda.ucar.edu/schemas/dsOverview.xsd\"" << std::endl;
  ofs << indent << "            ID=\"" << e.attribute_value("ID") << "\" type=\"" << e.attribute_value("type") << "\">" << std::endl;
  e=xdoc.element("dsOverview/timeStamp");
  ofs << indent << "  " << e.to_string() << std::endl;
  LocalQuery query("doi","dssdb.dsvrsn","dsid = 'ds"+dsnum+"' and end_date is null");
  Row row;
  if (query.submit(server) == 0 && query.fetch_row(row)) {
    ofs << indent << "  <doi>" << row[0] << "</doi>" << std::endl;
  }
  e=xdoc.element("dsOverview/continuingUpdate");
  ofs << indent << "  " << e.to_string() << std::endl;
  e=xdoc.element("dsOverview/access");
  ofs << indent << "  " << e.to_string() << std::endl;
  e=xdoc.element("dsOverview/title");
  ofs << indent << "  " << e.to_string() << std::endl;
  e=xdoc.element("dsOverview/summary");
  auto summary=e.to_string();
  strutils::replace_all(summary,"\n","\n"+indent+"  ");
  ofs << indent << "  " << summary << std::endl;
  e=xdoc.element("dsOverview/creator");
  auto creator=e.to_string();
  if (strutils::contains(creator,"/SCD/")) {
    strutils::replace_all(creator,"SCD","CISL");
  }
  ofs << indent << "  " << creator << std::endl;
  std::list<XMLElement> elist=xdoc.element_list("dsOverview/contributor");
  for (const auto& element : elist) {
    auto contributor=element.to_string();
    if (strutils::contains(contributor,"/SCD/")) {
	strutils::replace_all(contributor,"SCD","CISL");
    }
    ofs << indent << "  " << contributor << std::endl;
  }
  elist=xdoc.element_list("dsOverview/author");
  for (const auto& element : elist) {
    ofs << indent << "  <author>";
    auto author_type=element.attribute_value("xsi:type");
    if (author_type == "authorPerson" || author_type.empty()) {
	ofs << element.attribute_value("fname");
	auto mname=element.attribute_value("mname");
	if (!mname.empty()) {
	  ofs << " " << mname;
	}
	ofs << " " << element.attribute_value("lname");
    }
    else {
	ofs << element.attribute_value("name");
    }
    ofs << "</author>" << std::endl;
  }
  e=xdoc.element("dsOverview/restrictions");
  if (e.name() == "restrictions") {
    ofs << indent << "  " << e.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/variable");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/contact");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/platform");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/project");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/supportsProject");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  e=xdoc.element("dsOverview/topic");
  ofs << indent << "  " << e.to_string() << std::endl;
  elist=xdoc.element_list("dsOverview/keyword");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/reference");
  for (const auto& element : elist) {
    auto reference=element.to_string();
    strutils::replace_all(reference,"\n","\n"+indent+"  ");
    ofs << indent << "  " << reference << std::endl;
  }
  elist=xdoc.element_list("dsOverview/referenceURL");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/relatedResource");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  elist=xdoc.element_list("dsOverview/relatedDataset");
  for (const auto& element : elist) {
    ofs << indent << "  " << element.to_string() << std::endl;
  }
  e=xdoc.element("dsOverview/publicationDate");
  if (e.name() == "publicationDate") {
    ofs << indent << "  " << e.to_string() << std::endl;
  }
  else {
    query.set("pub_date","search.datasets","dsid = "+dsnum);
    if (query.submit(server) == 0 && query.fetch_row(row)) {
	ofs << indent << "  <publicationDate>" << row[0] << "</publicationDate>" << std::endl;
    }
  }
  auto cmd_databases=metautils::cmd_databases("unknown","unknown");
  auto found_content_metadata=false;
  for (const auto& database : cmd_databases) {
    if (table_exists(server,std::get<0>(database)+".ds"+dsnum2+"_primaries2")) {
	found_content_metadata=true;
    }
  }
  if (found_content_metadata) {
    ofs << indent << "  <contentMetadata>" << std::endl;
    PThreadStruct pts;
    pts.query.set("select min(concat(date_start,' ',time_start)),min(start_flag),max(concat(date_end,' ',time_end)),min(end_flag),time_zone from dssdb.dsperiod where dsid = 'ds"+dsnum+"' group by dsid");
    pthread_create(&pts.tid,NULL,run_query,reinterpret_cast<void *>(&pts));
    PThreadStruct gfts;
    gfts.query.set("nsteps_per,unit","GrML.frequencies","dsid = '"+dsnum+"'");
    pthread_create(&gfts.tid,NULL,run_query,reinterpret_cast<void *>(&gfts));
    PThreadStruct gfmts;
    gfmts.query.set("select distinct f.format from GrML.ds"+dsnum2+"_primaries2 as p left join GrML.formats as f on f.code = p.format_code");
    pthread_create(&gfmts.tid,NULL,run_query,reinterpret_cast<void *>(&gfmts));
    PThreadStruct ofts;
    ofts.query.set("select min(min_obs_per),max(max_obs_per),unit from ObML.ds"+dsnum2+"_frequencies group by unit");
    pthread_create(&ofts.tid,NULL,run_query,reinterpret_cast<void *>(&ofts));
    PThreadStruct ofmts;
    ofmts.query.set("select distinct f.format from ObML.ds"+dsnum2+"_primaries2 as p left join ObML.formats as f on f.code = p.format_code");
    pthread_create(&ofmts.tid,NULL,run_query,reinterpret_cast<void *>(&ofmts));
    PThreadStruct dts;
    dts.query.set("select distinct d.definition, d.def_params from \"WGrML\".summary as s left join \"WGrML\".grid_definitions as d on d.code = s.grid_definition_code where s.dsid = '"+dsnum+"'");
    pthread_create(&dts.tid,NULL,run_query,reinterpret_cast<void *>(&dts));
    pthread_join(pts.tid,NULL);
    pthread_join(gfts.tid,NULL);
    pthread_join(gfmts.tid,NULL);
    pthread_join(ofts.tid,NULL);
    pthread_join(ofmts.tid,NULL);
    pthread_join(dts.tid,NULL);
    if (pts.query.num_rows() > 0) {
	while (pts.query.fetch_row(row)) {
	  ofs << indent << "    <temporal start=\"" << metatranslations::date_time(row[0],row[1],row[4]) << "\" end=\"" << metatranslations::date_time(row[2],row[3],row[4]) << "\" groupID=\"Entire Dataset\" />" << std::endl;
	}
    }
    my::map<Entry> frequency_table;
    std::list<std::string> type_list,format_list;
    for (const auto& database : cmd_databases) {
	auto database_name=std::get<0>(database);
	if (database_name == "GrML") {
	  type_list.emplace_back("grid");
	  if (gfts.query.num_rows() > 0) {
	    while (gfts.query.fetch_row(row)) {
		add_frequency(frequency_table,std::stoi(row[0]),row[1]);
	    }
	  }
	  if (gfmts.query.num_rows() > 0) {
	    while (gfmts.query.fetch_row(row)) {
		format_list.emplace_back(row[0]);
	    }
	  }
	}
	else if (database_name == "ObML") {
	  type_list.emplace_back("platform_observation");
	  if (ofts.query.num_rows() > 0) {
	    while (ofts.query.fetch_row(row)) {
		add_frequency(frequency_table,std::stoi(row[0]),row[2]);
		add_frequency(frequency_table,std::stoi(row[1]),row[2]);
	    }
	  }
	  if (ofmts.query.num_rows() > 0) {
	    while (ofmts.query.fetch_row(row)) {
		format_list.emplace_back(row[0]);
	    }
	  }
	}
    }
    for (const auto& key : frequency_table.keys()) {
	size_t idx=key.find("<!>");
	ofs << indent << "    <temporalFrequency type=\"regular\" number=\"" << key.substr(0,idx) << "\" unit=\"" << key.substr(idx+3) << "\" />" << std::endl;
    }
    for (const auto& type : type_list) {
	ofs << indent << "    <dataType>" << type << "</dataType>" << std::endl;
    }
    std::string format_ref_doc;
    struct stat buf;
    if (stat((metautils::directives.server_root+"/web/metadata/FormatReferences.xml.new").c_str(),&buf) == 0) {
	format_ref_doc=metautils::directives.server_root+"/web/metadata/FormatReferences.xml.new";
    }
    else {
	format_ref_doc=unixutils::remote_web_file("https://rda.ucar.edu/metadata/FormatReferences.xml.new",temp_dir.name());
    }
    XMLDocument fdoc(format_ref_doc);
    for (const auto& format : format_list) {
	e=fdoc.element("formatReferences/format@name="+format+"/href");
	auto href=e.content();
	ofs << indent << "    <format";
	if (!href.empty()) {
	  ofs << " href=\"" << href << "\"";
	}
	ofs << ">" << format << "</format>" << std::endl;
    }
    fdoc.close();
    if (dts.query.num_rows() > 0) {
	while (dts.query.fetch_row(row)) {
	  ofs << indent << "    <geospatialCoverage>" << std::endl;
	  std::deque<std::string> sp=strutils::split(row[1],":");
	  ofs << indent << "      <grid definition=\"" << row[0] << "\" numX=\"" << sp[0] << "\" numY=\"" << sp[1] << "\"";
	  auto grid_def=row[0];
	  if (grid_def == "gaussLatLon") {
	    ofs << " startLon=\"" << sp[3] << "\" startLat=\"" << sp[2] << "\" endLon=\"" << sp[5] << "\" endLat=\"" << sp[4] << "\" xRes=\"" << sp[6] << "\"";
	  }
	  else if (grid_def == "lambertConformal") {
	    ofs << " startLon=\"" << sp[3] << "\" startLat=\"" << sp[2] << "\" projLon=\"" << sp[5] << "\" resLat=\"" << sp[4] << "\" xRes=\"" << sp[7] << "\" yRes=\"" << sp[8] << "\" pole=\"" << sp[6] << "\" stdParallel1=\"" << sp[9] << "\" stdParallel2=\"" << sp[10] << "\"";
	  }
	  else if (grid_def == "latLon") {
	    ofs << " startLon=\"" << sp[3] << "\" startLat=\"" << sp[2] << "\" endLon=\"" << sp[5] << "\" endLat=\"" << sp[4] << "\" xRes=\"" << sp[6] << "\" yRes=\"" << sp[7] << "\"";
	  }
	  else if (grid_def == "mercator") {
	    ofs << " startLon=\"" << sp[3] << "\" startLat=\"" << sp[2] << "\" endLon=\"" << sp[5] << "\" endLat=\"" << sp[4] << "\" xRes=\"" << sp[6] << "\" yRes=\"" << sp[7] << "\" resLat=\"" << sp[8] << "\"";
	  }
	  else if (grid_def == "polarStereographic") {
	    ofs << " startLon=\"" << sp[3] << "\" startLat=\"" << sp[2] << "\" projLon=\"" << sp[5] << "\" xRes=\"" << sp[7] << "\" yRes=\"" << sp[8] << "\" pole=\"" << sp[6] << "\"";
	  }
	  ofs << " />" << std::endl;
	  ofs << indent << "    </geospatialCoverage>" << std::endl;
	}
    }
    ofs << indent << "  </contentMetadata>" << std::endl;
  }
  else {
    e=xdoc.element("dsOverview/contentMetadata");
    auto content_metadata=e.to_string();
    strutils::replace_all(content_metadata,"\n","\n"+indent+"  ");
    ofs << indent << "  " << content_metadata << std::endl;
  }
  ofs << indent << "</dsOverview>" << std::endl;
  server.disconnect();
  return true;
}

} // end namespace metadataExport
