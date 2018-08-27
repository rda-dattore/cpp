#include <fstream>
#include <string>
#include <regex>
#include <unordered_map>
#include <sys/stat.h>
#include <metadata.hpp>
#include <datetime.hpp>
#include <MySQL.hpp>
#include <bsort.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bitmap.hpp>
#include <search.hpp>

extern std::string myerror;

namespace summarizeMetadata {

namespace obsData {

void check_point(double latp,double lonp,MySQL::Server& server,my::map<Entry>& location_table)
{
  size_t n,m,cnt,num;
  double lat,lon;
  std::string point[9],point2[9];
  MySQL::Query query;
  MySQL::Row row;
  std::string whereConditions;
  Entry e;
  obsData::PointEntry pe;
  static my::map<obsData::PointEntry> pointCache;

  pe.key=strutils::ftos(latp)+","+strutils::ftos(lonp);
  if (!pointCache.found(pe.key,pe)) {
    pe.locations=NULL;
    lat=latp+0.25;
    cnt=0;
    for (n=0; n < 3; n++) {
	if (n != 2) {
	  num=2;
	  lon=lonp+0.25;
	}
	else {
	  num=1;
	  lat=latp;
	  lon=lonp;
	}
	for (m=0; m < num; m++) {
	  point[cnt]="POINT("+strutils::ftos(lat+90.)+" ";
	  point2[cnt]="";
	  if (lon < 0.)
	    point[cnt]+=strutils::ftos(lon+360.);
	  else if (lon <= 30.) {
	    point2[cnt]=point[cnt];
	    point[cnt]+=strutils::ftos(lon);
	    point2[cnt]+=strutils::ftos(lon+360.);
	  }
	  else
	    point[cnt]+=strutils::ftos(lon);
	  point[cnt]+=")";
	  if (point2[cnt].length() > 0)
	    point2[cnt]+=")";
	  lon-=0.5;
	  cnt++;
	}
	lat-=0.5;
    }
    for (n=0; n < cnt; n++) {
	if (whereConditions.length() > 0)
	  whereConditions+=" or ";
	whereConditions+="Within(GeomFromText('"+point[n]+"'),bounds) = 1";
	if (point2[n].length() > 0)
	  whereConditions+=" or Within(GeomFromText('"+point2[n]+"'),bounds) = 1";
    }
    query.set("select name,AsText(bounds) from search.political_boundaries where "+whereConditions);
    if (query.submit(server) < 0) {
	std::cerr << "Error: " << query.error() << std::endl;
	exit(1);
    }
//std::cerr << query.show() << std::endl;
    while (query.fetch_row(row)) {
	if (!location_table.found(row[0],e)) {
	  for (n=0; n < cnt; n++) {
	    if (within_polygon(row[1],point[n]) || (point2[n].length() > 0 && within_polygon(row[1],point2[n]))) {
		e.key=row[0];
		location_table.insert(e);
		if (pe.locations == nullptr) {
		  pe.locations.reset(new std::list<std::string>);
		}
		(pe.locations)->emplace_back(e.key);
		n=cnt;
	    }
	  }
	}
    }
    pointCache.insert(pe);
  }
  else {
    if (pe.locations != NULL) {
	for (auto location : *pe.locations) {
	  e.key=location;
	  if (!location_table.found(e.key,e)) {
	    location_table.insert(e);
	  }
	}
    }
  }
}

void compress_locations(std::list<std::string>& location_list,my::map<ParentLocation>& parent_location_table,std::vector<std::string>& sorted_array,std::string caller,std::string user)
{
  ParentLocation pl,pl2;
  Entry e;
  bool matched_to_parent=true;

  TempDir temp_dir;
  if (!temp_dir.create("/tmp")) {
    std::cerr << "Error creating temporary directory" << std::endl;
    exit(1);
  }
  std::string gcmd_locations;
  struct stat buf;
  if (stat((meta_directives.server_root+"/web/metadata/gcmd_locations").c_str(),&buf) == 0) {
    gcmd_locations=meta_directives.server_root+"/web/metadata/gcmd_locations";
  }
  else {
    gcmd_locations=unixutils::remote_web_file("http://rda.ucar.edu/metadata/gcmd_locations",temp_dir.name());
  }
  std::ifstream ifs(gcmd_locations.c_str());
  if (!ifs.is_open()) {
    std::cerr << "Error opening gcmd_locations" << std::endl;
    exit(1);
  }
  char line[256];
  ifs.getline(line,256);
  while (!ifs.eof()) {
    std::string sline=line;
    pl.key=sline;
    while (strutils::occurs(pl.key," > ") > 1) {
	e.key=pl.key;
	pl.key=pl.key.substr(0,pl.key.rfind(" > "));
	if (!parent_location_table.found(pl.key,pl)) {
	  pl.children_table.reset(new my::map<Entry>);
	  pl.children_table->insert(e);
	  pl.matched_table=nullptr;
	  pl.consolidated_parent_table.reset(new my::map<Entry>);
	  parent_location_table.insert(pl);
	}
	else {
	  if (!pl.children_table->found(e.key,e)) {
	    pl.children_table->insert(e);
	  }
	}
    }
    ifs.getline(line,256);
  }
  ifs.close();
//special handling for Antarctica since it has no children
  pl.key="Continent > Antarctica";
  pl.children_table.reset(new my::map<Entry>);
  pl.matched_table=nullptr;
  pl.consolidated_parent_table.reset(new my::map<Entry>);
  parent_location_table.insert(pl);
// match location keywords to their parents
  for (auto item : location_list) {
    e.key=item;
    if (strutils::occurs(e.key," > ") > 1) {
	pl.key=e.key.substr(0,e.key.rfind(" > "));
    }
    else {
	pl.key=e.key;
    }
    parent_location_table.found(pl.key,pl);
    if (pl.matched_table == nullptr) {
	pl.matched_table.reset(new my::map<Entry>);
	parent_location_table.replace(pl);
    }
    pl.matched_table->insert(e);
  }
  sorted_array.clear();
  sorted_array.reserve(parent_location_table.size());
  for (auto& key : parent_location_table.keys()) {
    sorted_array.emplace_back(key);
  }
  binary_sort(sorted_array,
  [](const std::string& left,const std::string& right) -> bool
  {
    if (strutils::occurs(left," > ") > strutils::occurs(right," > ")) {
	return true;
    }
    else if (strutils::occurs(left," > ") < strutils::occurs(right," > ")) {
	return false;
    }
    else {
	if (left <= right) {
	  return true;
	}
	else {
	  return false;
	}
    }
  });
// continue matching parents to their parents until done
  while (matched_to_parent) {
    matched_to_parent=false;
    for (size_t n=0; n < parent_location_table.size(); ++n) {
	pl.key=sorted_array[n];
	if (strutils::occurs(pl.key," > ") > 1) {
	  parent_location_table.found(pl.key,pl);
	  if (pl.matched_table != nullptr) {
	    pl2.key=pl.key.substr(0,pl.key.rfind(" > "));
	    parent_location_table.found(pl2.key,pl2);
	    if (pl2.matched_table == nullptr) {
		pl2.matched_table.reset(new my::map<Entry>);
		parent_location_table.replace(pl2);
	    }
	    for (const auto& key : pl.children_table->keys()) {
		e.key=key;
		pl2.children_table->insert(e);
	    }
	    for (const auto& key : pl.matched_table->keys()) {
		e.key=key;
		pl2.matched_table->insert(e);
	    }
	    for (const auto& key : pl.consolidated_parent_table->keys()) {
		e.key=key;
		pl2.consolidated_parent_table->insert(e);
	    }
	    e.key=pl.key;
	    pl2.consolidated_parent_table->insert(e);
	    pl.matched_table->clear();
	    pl.matched_table.reset();
	    pl.matched_table=nullptr;
	    parent_location_table.replace(pl);
	    matched_to_parent=true;
	  }
	}
    }
  }
}

bool summarize_obs_data(std::string caller,std::string user)
{
  std::string dsnum2=strutils::substitute(meta_args.dsnum,".","");
  MySQL::Query query;
  my::map<Entry> mss_table;
  my::map<obsData::SummaryEntry> current_obsdata_table,summary_table;
  std::list<std::string> summary_table_keys;
  Entry e;
  obsData::SummaryEntry se,se2;
  MySQL::Row row;
  std::deque<std::string> sp;
  std::string error,sline;
  my::map<ParentLocation> parent_location_table;
  std::list<std::string> parent_location_table_keys;
  ParentLocation pl;
  bool updatedBitmap=false;

  MySQL::Server server(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  query.set("code,format_code","ObML.ds"+dsnum2+"_primaries");
  if (query.submit(server) < 0) {
    std::cerr << query.error() << std::endl;
    exit(1);
  }
  while (query.fetch_row(row)) {
    e.key=row[0];
    e.code=row[1];
    mss_table.insert(e);
  }
  query.set("format_code,observationType_code,platformType_code,box1d_row,box1d_bitmap","search.obs_data","dsid = '"+meta_args.dsnum+"'");
  if (query.submit(server) < 0) {
    metautils::log_error("summarize_obs_data() returned '"+query.error()+"' while querying search.obs_data",caller,user);
  }
  while (query.fetch_row(row)) {
    se.key=row[0]+"<!>"+row[1]+"<!>"+row[2]+"<!>"+row[3];
    se.box1d_bitmap=row[4];
    current_obsdata_table.insert(se);
  }
  if (server.command("lock tables ObML.ds"+dsnum2+"_locations write",error) < 0) {
    std::cerr << server.error() << std::endl;
    exit(1);
  }
  query.set("mssID_code,observationType_code,platformType_code,start_date,end_date,box1d_row,box1d_bitmap","ObML.ds"+dsnum2+"_locations");
  if (query.submit(server) < 0) {
    metautils::log_error("summarize_obs_data() returned '"+query.error()+"' while querying ObML.ds"+dsnum2+"_locations",caller,user);
  }
  while (query.fetch_row(row)) {
    mss_table.found(row[0],e);
    se.key=e.code+"<!>"+row[1]+"<!>"+row[2]+"<!>"+row[5];
    if (!summary_table.found(se.key,se)) {
	se.start_date=row[3];
	se.end_date=row[4];
	se.box1d_bitmap=row[6];
	summary_table.insert(se);
	summary_table_keys.push_back(se.key);
    }
    else {
	if (row[3] < se.start_date) {
	  se.start_date=row[3];
	}
	if (row[4] > se.end_date) {
	  se.end_date=row[4];
	}
	if (row[6] != se.box1d_bitmap) {
	  se.box1d_bitmap=bitmap::add_box1d_bitmaps(se.box1d_bitmap,row[6]);
	}
	summary_table.replace(se);
    }
  }
  if (server.command("unlock tables",error) < 0) {
    std::cerr << server.error() << std::endl;
    exit(1);
  }
  for (const auto& key : summary_table_keys) {
    if (summary_table.found(key,se) && current_obsdata_table.found(key,se2)) {
	if (se.box1d_bitmap != se2.box1d_bitmap) {
	  updatedBitmap=true;
	  break;
	}
    }
    else {
	updatedBitmap=true;
	break;
    }
  }
  if (server.command("lock tables search.obs_data write",error) < 0) {
    std::cerr << server.error() << std::endl;
    exit(1);
  }
  server._delete("search.obs_data","dsid = '"+meta_args.dsnum+"'");
  for (const auto& key : summary_table_keys) {
    sp=strutils::split(key,"<!>");
    summary_table.found(key,se);
    if (server.insert("search.obs_data","'"+meta_args.dsnum+"',"+sp[0]+","+sp[1]+","+sp[2]+",'"+se.start_date+"','"+se.end_date+"',"+sp[3]+",'"+se.box1d_bitmap+"'") < 0) {
	error=server.error();
	if (!strutils::has_beginning(error,"Duplicate entry")) {
	  std::cerr << error << std::endl;
	  exit(1);
	}
    }
  }
  if (server.command("unlock tables",error) < 0) {
    std::cerr << server.error() << std::endl;
    exit(1);
  }
  error=summarize_locations("ObML");
  if (error.length() > 0) {
    metautils::log_error("summarize_obs_data() returned '"+error+"'",caller,user);
  }
  server.disconnect();
  return updatedBitmap;
}

} // end namespace obsData

} // end namespace summarizeMetadata

namespace metadata {

namespace ObML {

ObservationData::ObservationData() : num_types(0),observation_types(),observation_indexes(),id_tables(),platform_tables(),unique_observation_table(),unknown_id_re("unknown"),unknown_ids(nullptr),track_unique_observations(true),is_empty(true)
{
  MySQL::Server server(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  MySQL::LocalQuery query("obsType","ObML.obsTypes");
  if (query.submit(server) == 0) {
    MySQL::Row row;
    while (query.fetch_row(row)) {
	observation_types.emplace(num_types,row[0]);
	observation_indexes.emplace(row[0],num_types);
	++num_types;
    }
    for (size_t n=0; n < num_types; ++n) {
	id_tables.emplace_back(new my::map<IDEntry>(9999));
	platform_tables.emplace_back(new my::map<PlatformEntry>);
    }
  }
}

bool ObservationData::added_to_ids(std::string observation_type,IDEntry& ientry,std::string data_type,std::string data_type_map,float lat,float lon,double unique_timestamp,DateTime *start_datetime,DateTime *end_datetime)
{
  if (lat < -90. || lat > 90. || lon < -180. || lon > 360.) {
    myerror="latitude or longitude out of range";
    return false;
  }
  auto o=observation_indexes.find(observation_type);
  if (o == observation_indexes.end()) {
    myerror="no index for observation type '"+observation_type+"'";
    return false;
  }
  const static DateTime base(1000,1,1,0,0);
  if (unique_timestamp < 0.) {
    unique_timestamp=start_datetime->seconds_since(base);
  }
  auto true_lon= (lon <= 180.) ? lon : (360.-lon);
  if (!id_tables[o->second]->found(ientry.key,ientry)) {
    ientry.data.reset(new metadata::ObML::IDEntry::Data);
    ientry.data->S_lat=ientry.data->N_lat=lat;
    ientry.data->W_lon=ientry.data->E_lon=true_lon;
    ientry.data->min_lon_bitmap.reset(new float[360]);
    ientry.data->max_lon_bitmap.reset(new float[360]);
    for (size_t n=0; n < 360; ++n) {
	ientry.data->min_lon_bitmap[n]=ientry.data->max_lon_bitmap[n]=999.;
    }
    size_t n,m;
    try {
	geoutils::convert_lat_lon_to_box(1,0.,true_lon,n,m);
    }
    catch (std::exception& e) {
	myerror=e.what();
	return false;
    }
    ientry.data->min_lon_bitmap[m]=ientry.data->max_lon_bitmap[m]=true_lon;
    ientry.data->start=*start_datetime;
    if (end_datetime == nullptr) {
	ientry.data->end=ientry.data->start;
    }
    else {
	ientry.data->end=*end_datetime;
    }
    id_tables[o->second]->insert(ientry);
    DataTypeEntry dte;
    dte.key=data_type;
    dte.data.reset(new DataTypeEntry::Data);
    dte.data->map=data_type_map;
    ientry.data->data_types_table.insert(dte);
    if (track_unique_observations) {
	ientry.data->nsteps=1;
	dte.data->nsteps=1;
	std::stringstream key;
	key.setf(std::ios::fixed);
	key << o->second << ientry.key << unique_timestamp << std::endl;
	unique_observation_table.emplace(key.str(),std::vector<std::string>{data_type});
    }
  }
  else {
    if (lat != ientry.data->S_lat || true_lon != ientry.data->W_lon) {
	if (lat < ientry.data->S_lat) {
	  ientry.data->S_lat=lat;
	}
	if (lat > ientry.data->N_lat) {
	  ientry.data->N_lat=lat;
	}
	if (true_lon < ientry.data->W_lon) {
	  ientry.data->W_lon=true_lon;
	}
	if (true_lon > ientry.data->E_lon) {
	  ientry.data->E_lon=true_lon;
	}
	size_t n,m;
	try {
	  geoutils::convert_lat_lon_to_box(1,0.,true_lon,n,m);
	}
	catch (std::exception& e) {
	  myerror=e.what();
	  return false;
	}
	if (ientry.data->min_lon_bitmap[m] > 900.) {
	  ientry.data->min_lon_bitmap[m]=ientry.data->max_lon_bitmap[m]=true_lon;
	}
	else {
	  if (true_lon < ientry.data->min_lon_bitmap[m]) {
	    ientry.data->min_lon_bitmap[m]=true_lon;
	  }
	  if (true_lon > ientry.data->max_lon_bitmap[m]) {
	    ientry.data->max_lon_bitmap[m]=true_lon;
	  }
	}
    }
    if (*start_datetime < ientry.data->start) {
	ientry.data->start=*start_datetime;
    }
    if (end_datetime == nullptr) {
	if (*start_datetime > ientry.data->end) {
	  ientry.data->end=*start_datetime;
	}
    }
    else {
	if (*end_datetime > ientry.data->end) {
	  ientry.data->end=*end_datetime;
	}
    }
    DataTypeEntry dte;
    if (!ientry.data->data_types_table.found(data_type,dte)) {
	dte.key=data_type;
	dte.data.reset(new DataTypeEntry::Data);
	dte.data->map=data_type_map;
	ientry.data->data_types_table.insert(dte);
    }
    if (track_unique_observations) {
	std::stringstream key;
	key.setf(std::ios::fixed);
	key << o->second << ientry.key << unique_timestamp << std::endl;
	if (unique_observation_table.find(key.str()) == unique_observation_table.end()) {
	  ++(ientry.data->nsteps);
	  ++(dte.data->nsteps);
	  unique_observation_table.emplace(key.str(),std::vector<std::string>{data_type});
	}
	else {
	  auto &data_types=unique_observation_table[key.str()];
	  if (std::find(data_types.begin(),data_types.end(),data_type) == data_types.end()) {
	    ++(dte.data->nsteps);
	    data_types.emplace_back(data_type);
	  }
	}
    }
  }
  if (std::regex_search(ientry.key,unknown_id_re)) {
    if (unknown_ids == nullptr) {
	unknown_ids.reset(new std::unordered_set<std::string>);
    }
    unknown_ids->emplace(ientry.key);
  }
  is_empty=false;
  return true;
}

bool ObservationData::added_to_platforms(std::string observation_type,std::string platform_type,float lat,float lon)
{
  if (lat < -90. || lat > 90. || lon < -180. || lon > 360.) {
    myerror="latitude or longitude out of range";
    return false;
  }
  auto o=observation_indexes.find(observation_type);
  if (o == observation_indexes.end()) {
    myerror="no index for observation type '"+observation_type+"'";
    return false;
  }
  PlatformEntry pentry;
  if (!platform_tables[o->second]->found(platform_type,pentry)) {
    pentry.key=platform_type;
    pentry.boxflags.reset(new summarizeMetadata::BoxFlags);
    pentry.boxflags->initialize(361,180,0,0);
    platform_tables[o->second]->insert(pentry);
  }
  if (lat == -90.) {
    pentry.boxflags->spole=1;
  }
  else if (lat == 90.) {
    pentry.boxflags->npole=1;
  }
  else {
    size_t n,m;
    auto true_lon= (lon <= 180.) ? lon : (lon-360.);
    try {
	geoutils::convert_lat_lon_to_box(1,lat,true_lon,n,m);
    }
    catch (std::exception& e) {
	myerror=e.what();
	return false;
    }
    pentry.boxflags->flags[n-1][m]=1;
    pentry.boxflags->flags[n-1][360]=1;
  }
  return true;
}

std::string string_coordinate_to_db(std::string coordinate_value)
{
  std::string s=strutils::substitute(coordinate_value,".","");
  size_t idx;
  if ( (idx=coordinate_value.find(".")) == std::string::npos) {
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

} // end namespace ObML

} // end namespace metadata
