#include <fstream>
#include <sstream>
#include <regex>
#include <sstream>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <strutils.hpp>
#include <utils.hpp>
#include <bitmap.hpp>
#include <bsort.hpp>
#include <MySQL.hpp>
#include <tempfile.hpp>
#include <metadata.hpp>

namespace metadata {

void open_inventory(std::string& filename,TempDir **tdir,std::ofstream& ofs,std::string cmd_type,std::string caller,std::string user)
{
  if (ofs.is_open()) {
    return;
  }
  if (!std::regex_search(meta_args.path,std::regex("^https://rda.ucar.edu"))) {
    return;
  }
  filename=meta_args.path+"/"+meta_args.filename;
  if (std::regex_search(meta_args.path,std::regex("^/FS/DSS/"))) {
    strutils::replace_all(filename,"/FS/DSS/","");
  }
  else if (std::regex_search(meta_args.path,std::regex("^/DSS/"))) {
    strutils::replace_all(filename,"/DSS/","");
  }
  else {
    filename=metautils::relative_web_filename(filename);
  }
  strutils::replace_all(filename,"/","%");
  if (*tdir == nullptr) {
    *tdir=new TempDir();
    if (!(*tdir)->create(meta_directives.temp_path)) {
	metautils::log_error("open_inventory(): unable to create temporary directory",caller,user);
    }
  }
// create the directory tree in the temp directory
  std::stringstream output,error;
  if (unixutils::mysystem2("/bin/mkdir -p "+(*tdir)->name()+"/metadata/inv",output,error) < 0) {
    metautils::log_error("open_inventory(): unable to create a temporary directory tree - '"+error.str()+"'",caller,user);
  }
  ofs.open((*tdir)->name()+"/metadata/inv/"+filename+"."+cmd_type+"_inv");
  if (!ofs.is_open()) {
    metautils::log_error("open_inventory(): couldn't open the inventory output file",caller,user);
  }
}

void close_inventory(std::string filename,TempDir *tdir,std::ofstream& ofs,std::string cmd_type,bool insert_into_db,bool create_cache,std::string caller,std::string user)
{
  if (!ofs.is_open()) {
    return;
  }
  ofs.close();
  ofs.clear();
  MySQL::Server server(meta_directives.database_server,meta_directives.metadb_username,meta_directives.metadb_password,"");
  if (std::regex_search(meta_args.path,std::regex("^/FS/DSS/")) || std::regex_search(meta_args.path,std::regex("^/DSS/"))) {
    server.update(cmd_type+".ds"+strutils::substitute(meta_args.dsnum,".","")+"_primaries","inv = 'Y'","mssID = '"+strutils::substitute(filename,"%","/")+"'");
  }
  else {
    server.update("W"+cmd_type+".ds"+strutils::substitute(meta_args.dsnum,".","")+"_webfiles","inv = 'Y'","webID = '"+strutils::substitute(filename,"%","/")+"'");
  }
  std::string relative_path="metadata/inv/";
  system(("gzip "+tdir->name()+"/"+relative_path+filename+"."+cmd_type+"_inv").c_str());
  std::string herror;
  if (unixutils::rdadata_sync(tdir->name(),relative_path,"/data/web/datasets/ds"+meta_args.dsnum,meta_directives.rdadata_home,herror) < 0) {
    metautils::log_warning("close_inventory(): couldn't sync '"+filename+"."+cmd_type+"_inv' - rdadata_sync error(s): '"+herror+"'",caller,user);
  }
  if (meta_args.inventory_only) {
    meta_args.filename=filename;
  }
  server.disconnect();
  std::stringstream output,error;
  if (insert_into_db) {
    if (create_cache) {
	unixutils::mysystem2(meta_directives.local_root+"/bin/iinv -d "+meta_args.dsnum+" -f "+filename+"."+cmd_type+"_inv",output,error);
    }
    else {
	unixutils::mysystem2(meta_directives.local_root+"/bin/iinv -d "+meta_args.dsnum+" -C -f "+filename+"."+cmd_type+"_inv",output,error);
    }
  }
  if (!error.str().empty()) {
    metautils::log_warning("close_inventory(): '"+error.str()+"' while running iinv",caller,user);
  }
}

void copy_ancillary_files(std::string file_type,std::string metadata_file,std::string file_type_name,std::string caller,std::string user)
{
  std::stringstream output,error;
  if (unixutils::mysystem2("/bin/tcsh -c \"curl -s --data 'authKey=qGNlKijgo9DJ7MN&cmd=listfiles&value=/SERVER_ROOT/web/datasets/ds"+meta_args.dsnum+"/metadata/fmd&pattern="+metadata_file+"' http://rda.ucar.edu/cgi-bin/dss/remoteRDAServerUtils\"",output,error) < 0) {
    metautils::log_warning("copy_ancillary_files(): unable to copy "+file_type+" files - error: '"+error.str()+"'",caller,user);
  }
  else {
    auto oparts=strutils::split(output.str(),"\n");
    auto path_parts=strutils::split(oparts[0],"/");
    std::string __host__="/"+path_parts[1];
    TempFile itemp(meta_directives.temp_path),otemp(meta_directives.temp_path);
    TempDir tdir;
    if (!tdir.create(meta_directives.temp_path)) {
	metautils::log_error("copy_ancillary_files(): unable to create temporary directory",caller,user);
    }
// create the directory tree in the temp directory
    std::stringstream output,error;
    if (unixutils::mysystem2("/bin/mkdir -p "+tdir.name()+"/metadata/wfmd",output,error) < 0) {
	metautils::log_error("copy_ancillary_files(): unable to create a temporary directory - '"+error.str()+"'",caller,user);
    }
    for (size_t n=0; n < oparts.size(); ++n) {
	if (strutils::has_ending(oparts[n],".xml")) {
	  auto hs=oparts[n];
	  strutils::replace_all(hs,__host__,"/__HOST__");
	  unixutils::rdadata_sync_from(hs,itemp.name(),meta_directives.rdadata_home,error);
	  std::ifstream ifs(itemp.name().c_str());
	  if (ifs.is_open()) {
	    auto fparts=strutils::split(oparts[n],file_type);
	    std::ofstream ofs((tdir.name()+"/metadata/wfmd/"+file_type_name+"."+file_type+fparts[1]).c_str());
	    char line[32768];
	    ifs.getline(line,32768);
	    while (!ifs.eof()) {
		std::string line_s=line;
		if (strutils::contains(line_s,"parent=")) {
		  strutils::replace_all(line_s,metadata_file,file_type_name+"."+file_type);
		}
		ofs << line_s << std::endl;
		ifs.getline(line,32768);
	    }
	    ifs.close();
	    ofs.close();
	    ofs.clear();
	  }
	  else {
	    metautils::log_warning("copy_ancillary_files(): unable to copy "+file_type+" file '"+oparts[n]+"'",caller,user);
	  }
	}
    }
    std::string herror;
    if (unixutils::rdadata_sync(tdir.name(),"metadata/wfmd/","/data/web/datasets/ds"+meta_args.dsnum,meta_directives.rdadata_home,herror) < 0) {
	metautils::log_warning("copy_ancillary_files(): unable to sync - rdadata_sync error(s): '"+herror+"'",caller,user);
    }
  }
}

void write_initialize(bool& is_mss_file,std::string& filename,std::string ext,std::string tdir_name,std::ofstream& ofs,std::string caller,std::string user)
{
  if (std::regex_search(meta_args.path,std::regex("^https://rda.ucar.edu"))) {
    is_mss_file=false;
  }
// check for dataset directory on web server
  auto num_tries=0;
  while (num_tries < 3 && !unixutils::exists_on_server(meta_directives.web_server,"/data/web/datasets/ds"+meta_args.dsnum,meta_directives.rdadata_home)) {
    ++num_tries;
    sleep(15);
  }
  if (num_tries == 3 && !unixutils::exists_on_server(meta_directives.web_server,"/data/web/datasets/ds"+meta_args.dsnum,meta_directives.rdadata_home)) {
    metautils::log_error("write_initialize(): dataset ds"+meta_args.dsnum+" does not exist or is unavailable",caller,user);
  }
  filename=meta_args.path+"/"+meta_args.filename;
  if (is_mss_file) {
    strutils::replace_all(filename,"/FS/DSS/","");
    strutils::replace_all(filename,"/DSS/","");
    strutils::replace_all(filename,"/","%");
    if (meta_args.member_name.length() > 0) {
	filename+="..m.."+strutils::substitute(meta_args.member_name,"/","%");
    }
    std::stringstream output,error;
    if (!meta_args.overwrite_only && unixutils::exists_on_server(meta_directives.web_server,"/data/web/datasets/ds"+meta_args.dsnum+"/metadata/fmd/"+filename+"."+ext,meta_directives.rdadata_home)) {
	unixutils::mysystem2("/local/dss/bin/dcm -G -d "+meta_args.dsnum+" "+meta_args.path+"/"+meta_args.filename,output,error);
    }
// create the directory tree in the temp directory
    if (unixutils::mysystem2("/bin/mkdir -p "+tdir_name+"/metadata/fmd",output,error) < 0) {
	metautils::log_error("write_initialize(): unable to create a temporary directory (1) - '"+error.str()+"'",caller,user);
    }
    ofs.open(tdir_name+"/metadata/fmd/"+filename+"."+ext);
    if (!ofs.is_open()) {
	metautils::log_error("write_initialize(): unable to open MSS output file",caller,user);
    }
  }
  else {
    filename=metautils::relative_web_filename(filename);
    strutils::replace_all(filename,"/","%");
    std::stringstream output,error;
    if (!meta_args.overwrite_only && unixutils::exists_on_server(meta_directives.web_server,"/data/web/datasets/ds"+meta_args.dsnum+"/metadata/wfmd/"+filename+"."+ext,meta_directives.rdadata_home)) {
	unixutils::mysystem2("/local/dss/bin/dcm -G -d "+meta_args.dsnum+" "+meta_args.path+"/"+meta_args.filename,output,error);
    }
// create the directory tree in the temp directory
    if (unixutils::mysystem2("/bin/mkdir -p "+tdir_name+"/metadata/wfmd",output,error) < 0) {
	metautils::log_error("write_initialize(): unable to create a temporary directory (2) - '"+error.str()+"'",caller,user);
    }
    ofs.open(tdir_name+"/metadata/wfmd/"+filename+"."+ext);
    if (!ofs.is_open()) {
	metautils::log_error("write_initialize(): unable to open Web output file",caller,user);
    }
  }
  ofs << "<?xml version=\"1.0\" ?>" << std::endl;
}

void write_finalize(bool is_mss_file,std::string filename,std::string ext,std::string tdir_name,std::ofstream& ofs,std::string caller,std::string user)
{
  ofs.close();
  std::string relative_path="metadata/";
  if (!is_mss_file) {
    relative_path+="w";
  }
  relative_path+="fmd/";
  system(("gzip "+tdir_name+"/"+relative_path+filename+"."+ext).c_str());
  std::string error;
  if (unixutils::rdadata_sync(tdir_name,relative_path,"/data/web/datasets/ds"+meta_args.dsnum,meta_directives.rdadata_home,error) < 0) {
    metautils::log_warning("write_finalize(): unable to sync '"+filename+"."+ext+"' - rdadata_sync error(s): '"+error+"'",caller,user);
  }
  meta_args.filename=filename;
}

namespace GrML {

void write_grml(my::map<GridEntry>& grid_table,std::string caller,std::string user)
{
  bool is_mss_file=true;
  std::string filename;
  TempDir tdir;
  if (!tdir.create(meta_directives.temp_path)) {
    metautils::log_error("write_grml(): unable to create a temporary directory",caller,user);
  }
  std::ofstream ofs;
  write_initialize(is_mss_file,filename,"GrML",tdir.name(),ofs,caller,user);
  ofs << "<GrML xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << "      xsi:schemaLocation=\"https://rda.ucar.edu/schemas" << std::endl;
  ofs << "                          https://rda.ucar.edu/schemas/GrML.xsd\"" << std::endl;
  ofs << "      uri=\"";
  if (is_mss_file) {
    ofs << "file://MSS:";
  }
  ofs << meta_args.path << "/" << meta_args.filename;
  if (meta_args.member_name.length() > 0) {
    ofs << "..m.." << meta_args.member_name;
  }
  ofs << "\" format=\"";
  if (meta_args.data_format == "grib") {
    ofs << "WMO_GRIB1";
  }
  else if (meta_args.data_format == "grib0") {
    ofs << "WMO_GRIB0";
  }
  else if (meta_args.data_format == "grib2") {
    ofs << "WMO_GRIB2";
  }
  else if (meta_args.data_format == "jraieeemm") {
    ofs << "proprietary_Binary";
  }
  else if (meta_args.data_format == "oct") {
    ofs << "DSS_Octagonal_Grid";
  }
  else if (meta_args.data_format == "tropical") {
    ofs << "DSS_Tropical_Grid";
  }
  else if (meta_args.data_format == "ll") {
    ofs << "DSS_5-Degree_LatLon_Grid";
  }
  else if (meta_args.data_format == "slp") {
    ofs << "DSS_SLP_Grid";
  }
  else if (meta_args.data_format == "navy") {
    ofs << "DSS_Navy_Grid";
  }
  else if (meta_args.data_format == "ussrslp") {
    ofs << "USSR_SLP_Grid";
  }
  else if (meta_args.data_format == "on84") {
    ofs << "NCEP_ON84";
  }
  else if (meta_args.data_format == "netcdf") {
    ofs << "netCDF";
  }
  else if (meta_args.data_format == "hdf5nc4") {
    ofs << "netCDF4";
  }
  else if (meta_args.data_format == "cgcm1") {
    ofs << "proprietary_ASCII";
  }
  else if (meta_args.data_format == "cmorph025") {
    ofs << "NCEP_CPC_CMORPH025";
  }
  else if (meta_args.data_format == "cmorph8km") {
    ofs << "NCEP_CPC_CMORPH8km";
  }
  else if (meta_args.data_format == "gpcp") {
    ofs << "NASA_GSFC_GPCP";
  }
  else {
    ofs << "??";
  }
  ofs << "\">" << std::endl;
  ofs << "  <timeStamp value=\"" << dateutils::current_date_time().to_string("%Y-%m-%d %T %Z") << "\" />" << std::endl;
  std::vector<std::string> array;
  array.reserve(grid_table.size());
  for (const auto& key : grid_table.keys()) {
    array.emplace_back(key);
  }
  binary_sort(array,
  [](std::string& left,std::string& right) -> int
  {
    if (left[0] <= right[0]) {
	return -1;
    }
    else {
	return 1;
    }
  });
  for (size_t m=0; m < grid_table.size(); ++m) {
    GridEntry gentry;
    grid_table.found(array[m],gentry);
    auto g_parts=strutils::split(gentry.key,"<!>");
// lat-lon grid
    if (std::stoi(g_parts[0]) == Grid::latitudeLongitudeType) {
	auto slat=g_parts[3];
	if (std::regex_search(slat,std::regex("^-"))) {
	  slat=slat.substr(1,32768)+"S";
	}
	else {
	  slat+="N";
	}
	auto slon=g_parts[4];
	if (std::regex_search(slon,std::regex("^-"))) {
	  slon=slon.substr(1,32768)+"W";
	}
	else {
	  slon+="E";
	}
	auto elat=g_parts[5];
	if (std::regex_search(elat,std::regex("^-"))) {
	  elat=elat.substr(1,32768)+"S";
	}
	else {
	  elat+="N";
	}
	auto elon=g_parts[6];
	if (std::regex_search(elon,std::regex("^-"))) {
	  elon=elon.substr(1,32768)+"W";
	}
	else {
	  elon+="E";
	}
	ofs << "  <grid timeRange=\"" << g_parts[9] << "\" definition=\"latLon\" numX=\"" << g_parts[1] << "\" numY=\"" << g_parts[2] << "\" startLat=\"" << slat << "\" startLon=\"" << slon << "\" endLat=\"" << elat << "\" endLon=\"" << elon << "\" xRes=\"" << g_parts[7] << "\" yRes=\"" << g_parts[8] << "\">" << std::endl;
    }
// gaussian lat-lon grid
    else if (std::stoi(g_parts[0]) == Grid::gaussianLatitudeLongitudeType) {
	auto slat=g_parts[3];
	if (strutils::has_beginning(slat,"-")) {
	  slat=slat.substr(1,32768)+"S";
	}
	else {
	  slat+="N";
	}
	auto slon=g_parts[4];
	if (strutils::has_beginning(slon,"-")) {
	  slon=slon.substr(1,32768)+"W";
	}
	else {
	  slon+="E";
	}
	auto elat=g_parts[5];
	if (strutils::has_beginning(elat,"-")) {
	  elat=elat.substr(1,32768)+"S";
	}
	else {
	  elat+="N";
	}
	auto elon=g_parts[6];
	if (strutils::has_beginning(elon,"-")) {
	  elon=elon.substr(1,32768)+"W";
	}
	else {
	  elon+="E";
	}
	ofs << "  <grid timeRange=\"" << g_parts[9] << "\" definition=\"gaussLatLon\" numX=\"" << g_parts[1] << "\" numY=\"" << g_parts[2] << "\" startLat=\"" << slat << "\" startLon=\"" << slon << "\" endLat=\"" << elat << "\" endLon=\"" << elon << "\" xRes=\"" << g_parts[7] << "\" circles=\"" << g_parts[8] << "\">" << std::endl;
    }
// polar-stereographic grid
    else if (std::stoi(g_parts[0]) == Grid::polarStereographicType) {
	auto slat=g_parts[3];
	if (strutils::has_beginning(slat,"-")) {
	  slat=slat.substr(1,32768)+"S";
	}
	else {
	  slat+="N";
	}
	auto slon=g_parts[4];
	if (strutils::has_beginning(slon,"-")) {
	  slon=slon.substr(1,32768)+"W";
	}
	else {
	  slon+="E";
	}
	auto elat=g_parts[5];
	if (strutils::has_beginning(elat,"-")) {
	  elat=elat.substr(1,32768)+"S";
	}
	else {
	  elat+="N";
	}
	auto elon=g_parts[6];
	if (strutils::has_beginning(elon,"-")) {
	  elon=elon.substr(1,32768)+"W";
	}
	else {
	  elon+="E";
	}
	ofs << "  <grid timeRange=\"" << g_parts[10] << "\" definition=\"polarStereographic\" numX=\"" << g_parts[1] << "\" numY=\"" << g_parts[2] << "\"  startLat=\"" << slat << "\" startLon=\"" << slon << "\" resLat=\"" << elat << "\" projLon=\"" << elon << "\" pole=\"" << g_parts[9] << "\" xRes=\"" << g_parts[7] << "\" yRes=\"" << g_parts[8] << "\">" << std::endl;
    }
// lambert-conformal grid
    else if (std::stoi(g_parts[0]) == Grid::lambertConformalType) {
	auto slat=g_parts[3];
	if (strutils::has_beginning(slat,"-")) {
	  slat=slat.substr(1,32768)+"S";
	}
	else {
	  slat+="N";
	}
	auto slon=g_parts[4];
	if (strutils::has_beginning(slon,"-")) {
	  slon=slon.substr(1,32768)+"W";
	}
	else {
	  slon+="E";
	}
	auto elat=g_parts[5];
	if (strutils::has_beginning(elat,"-")) {
	  elat=elat.substr(1,32768)+"S";
	}
	else {
	  elat+="N";
	}
	auto elon=g_parts[6];
	if (strutils::has_beginning(elon,"-")) {
	  elon=elon.substr(1,32768)+"W";
	}
	else {
	  elon+="E";
	}
	std::string stdpar1=g_parts[10];
	if (strutils::has_beginning(stdpar1,"-")) {
	  stdpar1=stdpar1.substr(1,32768)+"S";
	}
	else {
	  stdpar1+="N";
	}
	std::string stdpar2=g_parts[11];
	if (strutils::has_beginning(stdpar2,"-")) {
	  stdpar2=stdpar2.substr(1,32768)+"S";
	}
	else {
	  stdpar2+="N";
	}
	ofs << "  <grid timeRange=\"" << g_parts[12] << "\" definition=\"lambertConformal\" numX=\"" << g_parts[1] << "\" numY=\"" << g_parts[2] << "\"  startLat=\"" << slat << "\" startLon=\"" << slon << "\" resLat=\"" << elat << "\" projLon=\"" << elon << "\" pole=\"" << g_parts[9] << "\" xRes=\"" << g_parts[7] << "\" yRes=\"" << g_parts[8] << "\" stdParallel1=\"" << stdpar1 << "\" stdParallel2=\"" << stdpar2 << "\">" << std::endl;
    }
    else if (std::stoi(g_parts[0]) == Grid::mercatorType) {
	auto slat=g_parts[3];
	if (strutils::has_beginning(slat,"-")) {
	  slat=slat.substr(1,32768)+"S";
	}
	else {
	  slat+="N";
	}
	auto slon=g_parts[4];
	if (strutils::has_beginning(slon,"-")) {
	  slon=slon.substr(1,32768)+"W";
	}
	else {
	  slon+="E";
	}
	auto elat=g_parts[5];
	if (strutils::has_beginning(elat,"-")) {
	  elat=elat.substr(1,32768)+"S";
	}
	else {
	  elat+="N";
	}
	auto elon=g_parts[6];
	if (strutils::has_beginning(elon,"-")) {
	  elon=elon.substr(1,32768)+"W";
	}
	else {
	  elon+="E";
	}
	ofs << "  <grid timeRange=\"" << g_parts[9] << "\" definition=\"mercator\" numX=\"" << g_parts[1] << "\" numY=\"" << g_parts[2] << "\" startLat=\"" << slat << "\" startLon=\"" << slon << "\" endLat=\"" << elat << "\" endLon=\"" << elon << "\" xRes=\"" << g_parts[7] << "\" yRes=\"" << g_parts[8] << "\">" << std::endl;
    }
// spherical harmonics
    else if (std::stoi(g_parts[0]) == Grid::sphericalHarmonicsType) {
	ofs << "  <grid timeRange=\"" << g_parts[4] << "\" definition=\"sphericalHarmonics\" t1=\"" << g_parts[1] << "\" t2=\"" << g_parts[2] << "\" t3=\"" << g_parts[3] << "\">" << std::endl;
    }
    else {
	metautils::log_error("write_grml(): no grid definition map for "+g_parts[0],caller,user);
    }
// print out process elements
    for (const auto& key : gentry.process_table.keys()) {
	ofs << "    <process value=\"" << key << "\" />" << std::endl;
    }
// print out ensemble elements
    for (const auto& e_key : gentry.ensemble_table.keys()) {
	auto e_parts=strutils::split(e_key,"<!>");
	ofs << "    <ensemble type=\"" << e_parts[0] << "\"";
	if (e_parts[1].length() > 0) {
	  ofs << " ID=\"" << e_parts[1] << "\"";
	}
	if (e_parts.size() > 2) {
	  ofs << " size=\"" << e_parts[2] << "\"";
	}
	ofs << " />" << std::endl;
    }
// print out level elements
    std::list<std::string> keys_to_remove;
    for (const auto& l_key : gentry.level_table.keys()) {
	LevelEntry lentry;
	gentry.level_table.found(l_key,lentry);
	auto l_parts=strutils::split(l_key,":");
	if (meta_args.data_format == "grib" || meta_args.data_format == "grib0" || meta_args.data_format == "grib2" || meta_args.data_format == "netcdf" || meta_args.data_format == "hdf5nc4") {
	  if (strutils::occurs(l_key,":") == 1) {
	    ofs << "    <level map=\"";
	    if (strutils::contains(l_parts[0],",")) {
		ofs << l_parts[0].substr(0,l_parts[0].find(",")) << "\" type=\"" << l_parts[0].substr(l_parts[0].find(",")+1);
	    }
	    else {
		ofs << l_parts[0];
	    }
	    ofs << "\" value=\"" << l_parts[1] << "\">" << std::endl;
	    for (const auto& param : lentry.parameter_code_table.keys()) {
		ParameterEntry pentry;
		if (lentry.parameter_code_table.found(param,pentry)) {
		  ofs << "      <parameter map=\"" << param.substr(0,param.find(":")) << "\" value=\"" << param.substr(param.find(":")+1) << "\" start=\"" << pentry.start_date_time.to_string("%Y-%m-%d %R %Z") << "\" end=\"" << pentry.end_date_time.to_string("%Y-%m-%d %R %Z") << "\" nsteps=\"" << pentry.num_time_steps << "\" />" << std::endl;
		}
	    }
	    ofs << "    </level>" << std::endl;
	    keys_to_remove.emplace_back(l_key);
	  }
	}
	else if (meta_args.data_format == "oct" || meta_args.data_format == "tropical" || meta_args.data_format == "ll" || meta_args.data_format == "navy" || meta_args.data_format == "slp") {
	  if (l_parts[0] == "0") {
	    if (l_parts[1] == "1013" || l_parts[1] == "1001" || l_parts[1] == "980" || l_parts[1] == "0") {
		ofs << "    <level type=\"" << l_parts[1] << "\" value=\"0\">" << std::endl;
	    }
	    else {
		ofs << "    <level value=\"" << l_parts[1] << "\">" << std::endl;
	    }
	    for (const auto& param : lentry.parameter_code_table.keys()) {
		ParameterEntry pentry;
		if (lentry.parameter_code_table.found(param,pentry)) {
		  ofs << "      <parameter value=\"" << param << "\" start=\"" << pentry.start_date_time.to_string("%Y-%m-%d %R %Z") << "\" end=\"" << pentry.end_date_time.to_string("%Y-%m-%d %R %Z") << "\" nsteps=\"" << pentry.num_time_steps << "\" />" << std::endl;
		}
	    }
	    ofs << "    </level>" << std::endl;
	    keys_to_remove.emplace_back(l_key);
	  }
	}
	else if (meta_args.data_format == "jraieeemm") {
	  ofs << "    <level map=\"ds" << meta_args.dsnum << "\" type=\"" << l_parts[0] << "\" value=\"" << l_parts[1] << "\">" << std::endl;
	  for (const auto& param : lentry.parameter_code_table.keys()) {
	    ParameterEntry pentry;
	    if (lentry.parameter_code_table.found(param,pentry)) {
		ofs << "      <parameter map=\"ds" << meta_args.dsnum << "\" value=\"" << param << "\" start=\"" << pentry.start_date_time.to_string("%Y-%m-%d %R %Z") << "\" end=\"" << pentry.end_date_time.to_string("%Y-%m-%d %R %Z") << "\" nsteps=\"" << pentry.num_time_steps << "\" />" << std::endl;
	    }
	  }
	  ofs << "    </level>" << std::endl;
	  keys_to_remove.emplace_back(l_key);
	}
	else if (meta_args.data_format == "on84") {
	  auto n=std::stoi(l_parts[0]);
	  if (n < 3 || n == 6 || n <= 7 || (n == 8 && l_parts.size() < 3) || (n >= 128 && n <= 135)) {
	    ofs << "    <level type=\"" << n << "\" value=\"" << l_parts[1] << "\">" << std::endl;
	    for (const auto& param : lentry.parameter_code_table.keys()) {
		ParameterEntry pentry;
		if (lentry.parameter_code_table.found(param,pentry)) {
		  ofs << "      <parameter value=\"" << param << "\" start=\"" << pentry.start_date_time.to_string("%Y-%m-%d %R %Z") << "\" end=\"" << pentry.end_date_time.to_string("%Y-%m-%d %R %Z") << "\" nsteps=\"" << pentry.num_time_steps << "\" />" << std::endl;
		}
	    }
	    ofs << "    </level>" << std::endl;
	    keys_to_remove.emplace_back(l_key);
	  }
	}
	else if (meta_args.data_format == "cgcm1") {
	  ofs << "    <level map=\"ds" << meta_args.dsnum << "\" type=\"" << l_parts[0] << "\" value=\"";
	  auto sdum=l_parts[0];
	  if (sdum == "1") {
	    sdum="0";
	  }
	  ofs << sdum << "\">" << std::endl;
	  for (const auto& param : lentry.parameter_code_table.keys()) {
	    ParameterEntry pentry;
	    if (lentry.parameter_code_table.found(param,pentry))
		ofs << "      <parameter map=\"ds" << meta_args.dsnum << "\" value=\"" << param << "\" start=\"" << pentry.start_date_time.to_string("%Y-%m-%d %R %Z") << "\" end=\"" << pentry.end_date_time.to_string("%Y-%m-%d %R %Z") << "\" nsteps=\"" << pentry.num_time_steps << "\" />" << std::endl;
	  }
	  ofs << "    </level>" << std::endl;
	  keys_to_remove.emplace_back(l_key);
	}
	else if (meta_args.data_format == "cmorph025" || meta_args.data_format == "cmorph8km" || meta_args.data_format == "gpcp") {
	  ofs << "    <level type=\"" << l_key << "\" value=\"0\">" << std::endl;;
	  for (const auto& param : lentry.parameter_code_table.keys()) {
	    ParameterEntry pentry;
	    if (lentry.parameter_code_table.found(param,pentry))
		ofs << "      <parameter value=\"" << param << "\" start=\"" << pentry.start_date_time.to_string("%Y-%m-%d %R %Z") << "\" end=\"" << pentry.end_date_time.to_string("%Y-%m-%d %R %Z") << "\" nsteps=\"" << pentry.num_time_steps << "\" />" << std::endl;
	  }
	  ofs << "    </level>" << std::endl;
	  keys_to_remove.emplace_back(l_key);
	}
	else {
	  ofs << l_key << std::endl;
	}
    }
    for (const auto& r_key : keys_to_remove) {
	gentry.level_table.remove(r_key);
    }
// print out layer elements
    keys_to_remove.clear();
    for (const auto& key : gentry.level_table.keys()) {
	LevelEntry lentry;
	gentry.level_table.found(key,lentry);
	auto y_parts=strutils::split(key,":");
	if (meta_args.data_format == "grib" || meta_args.data_format == "grib0" || meta_args.data_format == "grib2" || meta_args.data_format == "netcdf" || meta_args.data_format == "hdf5nc4") {
	  if (strutils::occurs(key,":") == 2) {
	    ofs << "    <layer map=\"";
	    if (strutils::contains(y_parts[0],",")) {
		ofs << y_parts[0].substr(0,y_parts[0].find(",")) << "\" type=\"" << y_parts[0].substr(y_parts[0].find(",")+1);
	    }
	    else {
		ofs << y_parts[0];
	    }
	    ofs << "\" bottom=\"" << y_parts[2] << "\" top=\"" << y_parts[1] << "\">" << std::endl;
	    for (const auto& param : lentry.parameter_code_table.keys()) {
		ParameterEntry pentry;
		if (lentry.parameter_code_table.found(param,pentry)) {
		  ofs << "      <parameter map=\"" << param.substr(0,param.find(":")) << "\" value=\"" << param.substr(param.find(":")+1) << "\" start=\"" << pentry.start_date_time.to_string("%Y-%m-%d %R %Z") << "\" end=\"" << pentry.end_date_time.to_string("%Y-%m-%d %R %Z") << "\" nsteps=\"" << pentry.num_time_steps << "\" />" << std::endl;
		}
	    }
	    ofs << "    </layer>" << std::endl;
	    keys_to_remove.emplace_back(key);
	  }
	}
	else if (meta_args.data_format == "oct" || meta_args.data_format == "tropical" || meta_args.data_format == "ll" || meta_args.data_format == "navy" || meta_args.data_format == "slp") {
	  if (y_parts[0] == "1") {
	    if (y_parts[1] == "1002" && y_parts[2] == "950") {
		ofs << "    <layer type=\"" << y_parts[1] << "\" bottom=\"0\" top=\"0\">" << std::endl;
	    }
	    else {
		ofs << "    <layer bottom=\"" << y_parts[2] << "\" top=\"" << y_parts[1] << "\">" << std::endl;
	    }
	    for (const auto& param : lentry.parameter_code_table.keys()) {
		ParameterEntry pentry;
		if (lentry.parameter_code_table.found(param,pentry)) {
		  ofs << "      <parameter value=\"" << param << "\" start=\"" << pentry.start_date_time.to_string("%Y-%m-%d %R %Z") << "\" end=\"" << pentry.end_date_time.to_string("%Y-%m-%d %R %Z") << "\" nsteps=\"" << pentry.num_time_steps << "\" />" << std::endl;
		}
	    }
	    ofs << "    </layer>" << std::endl;
	    keys_to_remove.emplace_back(key);
	  }
	}
	else if (meta_args.data_format == "on84") {
	  auto n=std::stoi(y_parts[0]);
	  if ((n == 8 && y_parts.size() == 3) || (n >= 144 && n <= 148)) {
	    ofs << "    <layer type=\"" << n << "\" bottom=\"" << y_parts[2] << "\" top=\"" << y_parts[1] << "\">" << std::endl;
	    for (const auto& param : lentry.parameter_code_table.keys()) {
		ParameterEntry pentry;
		if (lentry.parameter_code_table.found(param,pentry)) {
		  ofs << "      <parameter value=\"" << param << "\" start=\"" << pentry.start_date_time.to_string("%Y-%m-%d %R %Z") << "\" end=\"" << pentry.end_date_time.to_string("%Y-%m-%d %R %Z") << "\" nsteps=\"" << pentry.num_time_steps << "\" />" << std::endl;
		}
	    }
	    ofs << "    </layer>" << std::endl;
	    keys_to_remove.emplace_back(key);
	  }
	}
	else {
	  ofs << key << std::endl;
	}
    }
    for (const auto& r_key : keys_to_remove) {
	gentry.level_table.remove(r_key);
    }
    if (gentry.level_table.size() > 0) {
	metautils::log_error("write_grml(): level/layer table should be empty but table length is "+strutils::itos(gentry.level_table.size()),caller,user);
    }
    ofs << "  </grid>" << std::endl;
  }
  ofs << "</GrML>" << std::endl;
  write_finalize(is_mss_file,filename,"GrML",tdir.name(),ofs,caller,user);
}

} // end namespace GrML

namespace ObML {

void copy_obml(std::string metadata_file,std::string URL,std::string caller,std::string user)
{
  TempFile itemp(meta_directives.temp_path);
  auto ObML_name=metautils::relative_web_filename(URL);
  strutils::replace_all(ObML_name,"/","%");
  TempDir tdir;
  if (!tdir.create(meta_directives.temp_path)) {
    metautils::log_error("copy_obml(): unable to create temporary directory",caller,user);
  }
// create the directory tree in the temp directory
  std::stringstream output,error;
  if (unixutils::mysystem2("/bin/mkdir -p "+tdir.name()+"/metadata/wfmd",output,error) < 0) {
    metautils::log_error("copy_obml(): unable to create a temporary directory - '"+error.str()+"'",caller,user);
  }
  unixutils::rdadata_sync_from("/__HOST__/web/datasets/ds"+meta_args.dsnum+"/metadata/fmd/"+metadata_file,itemp.name(),meta_directives.rdadata_home,error);
  std::ifstream ifs(itemp.name().c_str());
  if (!ifs.is_open()) {
    metautils::log_error("copy_obml(): unable to open input file",caller,user);
  }
  std::ofstream ofs((tdir.name()+"/metadata/wfmd/"+ObML_name+".ObML").c_str());
  if (!ofs.is_open()) {
    metautils::log_error("copy_obml(): unable to open output file",caller,user);
  }
  char line[32768];
  ifs.getline(line,32768);
  while (!ifs.eof()) {
    std::string sline=line;
    if (strutils::contains(sline,"uri=")) {
	sline=sline.substr(sline.find(" format"));
	sline="      uri=\""+URL+"\""+sline;
    }
    else if (strutils::contains(sline,"ref=")) {
	strutils::replace_all(sline,metadata_file,ObML_name+".ObML");
    }
    ofs << sline << std::endl;
    ifs.getline(line,32768);
  }
  ifs.close();
  ofs.close();
  std::string herror;
  if (unixutils::rdadata_sync(tdir.name(),"metadata/wfmd/","/data/web/datasets/ds"+meta_args.dsnum,meta_directives.rdadata_home,herror) < 0) {
    metautils::log_warning("copy_obml(): unable to sync '"+ObML_name+".ObML' - rdadata_sync error(s): '"+herror+"'",caller,user);
  }
  copy_ancillary_files("ObML",metadata_file,ObML_name,caller,user);
  meta_args.filename=ObML_name;
}

void write_obml(my::map<IDEntry> **ID_table,my::map<PlatformEntry> *platform_table,std::string caller,std::string user)
{
  std::ofstream ofs,ofs2;
  TempFile tfile2(meta_directives.temp_path);
  std::string path,filename,cmd_type;
  std::deque<std::string> sp;
  int n;
  size_t m,xx;
  size_t numIDs,time,hr,min,sec;
  bitmap::longitudeBitmap::bitmap_gap biggest,current;
  struct PlatformData {
    PlatformData() : nsteps(0),data_types_table(),start(),end() {}

    size_t nsteps;
    my::map<DataTypeEntry> data_types_table;
    DateTime start,end;
  } platform;
  DataTypeEntry de,de2;
  bool is_mss_file=true;
  MySQL::Server server;
  std::string sdum,output,error;
  IDEntry ientry;
  PlatformEntry pentry;
  metadata::ObML::ObservationTypes obsTypes;

  TempDir tdir;
  if (!tdir.create(meta_directives.temp_path)) {
    metautils::log_error("write_obml(): unable to create temporary directory",caller,user);
  }
  write_initialize(is_mss_file,filename,"ObML",tdir.name(),ofs,caller,user);
  ofs << "<ObML xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << "       xsi:schemaLocation=\"https://rda.ucar.edu/schemas" << std::endl;
  ofs << "                           https://rda.ucar.edu/schemas/ObML.xsd\"" << std::endl;
  ofs << "       uri=\"";
  if (is_mss_file) {
    cmd_type="fmd";
    ofs << "file://MSS:" << meta_args.path << "/" << meta_args.filename;
    if (!meta_args.member_name.empty()) {
	ofs << "..m.." << meta_args.member_name;
    }
  }
  else {
    cmd_type="wfmd";
    ofs << "file://web:" << metautils::relative_web_filename(meta_args.path+"/"+meta_args.filename);
  }
  ofs << "\" format=\"";
  if (meta_args.data_format == "on29" || meta_args.data_format == "on124") {
    ofs << "NCEP_" << strutils::to_upper(meta_args.data_format);
  }
  else if (strutils::contains(meta_args.data_format,"bufr")) {
    ofs << "WMO_BUFR";
  }
  else if (meta_args.data_format == "cpcsumm" || meta_args.data_format == "ghcnmv3" || meta_args.data_format == "uadb") {
    ofs << "proprietary_ASCII";
  }
  else if (meta_args.data_format == "imma") {
    ofs << "NOAA_IMMA";
  }
  else if (meta_args.data_format == "isd") {
    ofs << "NCDC_ISD";
  }
  else if (meta_args.data_format == "netcdf") {
    ofs << "netCDF";
  }
  else if (meta_args.data_format == "nodcbt") {
    ofs << "NODC_BT";
  }
  else if (std::regex_search(meta_args.data_format,std::regex("^td32"))) {
    ofs << "NCDC_" << strutils::to_upper(meta_args.data_format);
  }
  else if (meta_args.data_format == "tsr") {
    ofs << "DSS_TSR";
  }
  else if (meta_args.data_format == "wmssc") {
    ofs << "DSS_WMSSC";
  }
  else if (meta_args.data_format == "hdf5") {
    ofs << "HDF5";
  }
  else if (meta_args.data_format == "little_r") {
    ofs << "LITTLE_R";
  }
  ofs << "\">" << std::endl;
  ofs << "  <timeStamp value=\"" << dateutils::current_date_time().to_string("%Y-%m-%d %T %Z") << "\" />" << std::endl;
  for (xx=0; xx < NUM_OBS_TYPES; xx++) {
    ID_table[xx]->keysort(
    [](const std::string& left,const std::string& right) -> bool
    {
	if (left <= right)
	  return true;
	else
	  return false;
    });
    if (platform_table[xx].size() > 0) {
	ofs << "  <observationType value=\"" << obsTypes.types[xx] << "\">" << std::endl;
	for (const auto& key : platform_table[xx].keys()) {
	  platform_table[xx].found(key,pentry);
	  ofs2.open((tdir.name()+"/metadata/"+cmd_type+"/"+filename+".ObML."+obsTypes.types[xx]+"."+key+".IDs.xml").c_str());
	  ofs2 << "<?xml version=\"1.0\" ?>" << std::endl;
	  ofs2 << "<IDs parent=\"" << filename << ".ObML\" group=\"" << obsTypes.types[xx] << "." << key << "\">" << std::endl;
	  numIDs=0;
	  platform.nsteps=0;
	  platform.start.set(9999,12,31,235959);
	  platform.end.set(1,1,1,0);
	  platform.data_types_table.clear();
	  for (const auto& ikey : ID_table[xx]->keys()) {
	    if (strutils::has_beginning(ikey,key)) {
		sp=strutils::split(ikey,"[!]");
		if (sp.size() < 3) {
		  std::cerr << "Error parsing ID: " << ikey << std::endl;
		  exit(1);
		}
		ofs2 << "  <ID type=\"" << sp[1] << "\" value=\"" << sp[2] << "\"";
		ID_table[xx]->found(ikey,ientry);
		if (floatutils::myequalf(ientry.data->S_lat,ientry.data->N_lat) && floatutils::myequalf(ientry.data->W_lon,ientry.data->E_lon)) {
		  ofs2 << " lat=\"" << strutils::ftos(ientry.data->S_lat,4) << "\" lon=\"" << strutils::ftos(ientry.data->W_lon,4) << "\"";
		}
		else {
		  size_t w_index,e_index;
		  bitmap::longitudeBitmap::west_east_bounds(ientry.data->min_lon_bitmap.get(),w_index,e_index);
		  ofs2 << " cornerSW=\"" << strutils::ftos(ientry.data->S_lat,4) << "," << strutils::ftos(ientry.data->min_lon_bitmap[w_index],4) << "\" cornerNE=\"" << strutils::ftos(ientry.data->N_lat,4) << "," << strutils::ftos(ientry.data->max_lon_bitmap[e_index],4) << "\"";
		}
		ofs2 << " start=\"";
		time=ientry.data->start.time();
		hr=time/10000;
		min=(time-hr*10000)/100;
		sec=time % 100;
		if (sec != 99) {
		  ofs2 << ientry.data->start.to_string("%Y-%m-%d %H:%MM:%SS %Z");
		}
		else if (min != 99) {
		  ofs2 << ientry.data->start.to_string("%Y-%m-%d %H:%MM %Z");
		}
		else if (hr != 99) {
		  ofs2 << ientry.data->start.to_string("%Y-%m-%d %H00 %Z");
		}
		else {
		  ofs2 << ientry.data->start.to_string("%Y-%m-%d");
		}
		ofs2 << "\" end=\"";
		time=ientry.data->end.time();
		hr=time/10000;
		min=(time-hr*10000)/100;
		sec=time % 100;
		if (sec != 99) {
		  ofs2 << ientry.data->end.to_string("%Y-%m-%d %H:%MM:%SS %Z");
		}
		else if (min != 99) {
		  ofs2 << ientry.data->end.to_string("%Y-%m-%d %H:%MM %Z");
		}
		else if (hr != 99) {
		  ofs2 << ientry.data->end.to_string("%Y-%m-%d %H00 %Z");
		}
		else {
		  ofs2 << ientry.data->end.to_string("%Y-%m-%d");
		}
		ofs2 << "\" numObs=\"" << ientry.data->nsteps << "\">" << std::endl;
		if (ientry.data->start < platform.start) {
		  platform.start=ientry.data->start;
		}
		if (ientry.data->end > platform.end) {
		  platform.end=ientry.data->end;
		}
		platform.nsteps+=ientry.data->nsteps;
		for (const auto& data_type : ientry.data->data_types_table.keys()) {
		  ientry.data->data_types_table.found(data_type,de);
		  if (de.data->nsteps > 0) {
		    ofs2 << "    <dataType";
		    if (meta_args.data_format == "cpcsumm" || meta_args.data_format == "netcdf" || meta_args.data_format == "hdf5" || meta_args.data_format == "ghcnmv3") {
			ofs2 << " map=\"ds" << meta_args.dsnum << "\"";
		    }
		    else if (strutils::contains(meta_args.data_format,"bufr")) {
			ofs2 << " map=\"" << de.data->map << "\"";
		    }
		    ofs2 << " value=\"" << de.key << "\" numObs=\"" << de.data->nsteps << "\"";
		    if (de.data->vdata == nullptr || de.data->vdata->res_cnt == 0) {
			ofs2 << " />" << std::endl;
		    }
		    else {
			ofs2 << ">" << std::endl;
			ofs2 << "      <vertical min_altitude=\"" << de.data->vdata->min_altitude << "\" max_altitude=\"" << de.data->vdata->max_altitude << "\" vunits=\"" << de.data->vdata->units << "\" avg_nlev=\"" << de.data->vdata->avg_nlev/de.data->nsteps << "\" avg_vres=\"" << de.data->vdata->avg_res/de.data->vdata->res_cnt << "\" />" << std::endl;
			ofs2 << "    </dataType>" << std::endl;
		    }
		    if (!platform.data_types_table.found(de.key,de2)) {
			de2.key=de.key;
			de2.data.reset(new DataTypeEntry::Data);
			platform.data_types_table.insert(de2);
		    }
		    if (strutils::contains(meta_args.data_format,"bufr")) {
			de2.data->map=de.data->map;
		    }
		    de2.data->nsteps+=de.data->nsteps;
		    if (de2.data->vdata != nullptr && de2.data->vdata->res_cnt > 0) {
			de2.data->vdata->avg_res+=de.data->vdata->avg_res;
			de2.data->vdata->res_cnt+=de.data->vdata->res_cnt;
			if (de.data->vdata->min_altitude < de2.data->vdata->min_altitude) {
			  de2.data->vdata->min_altitude=de.data->vdata->min_altitude;
			}
			if (de.data->vdata->max_altitude > de2.data->vdata->max_altitude) {
			  de2.data->vdata->max_altitude=de.data->vdata->max_altitude;
			}
			de2.data->vdata->avg_nlev+=de.data->vdata->avg_nlev;
			de2.data->vdata->units=de.data->vdata->units;
		    }
		  }
		}
		ofs2 << "  </ID>" << std::endl;
		numIDs++;
	    }
	  }
	  ofs2 << "</IDs>" << std::endl;
	  ofs2.close();
	  ofs << "    <platform type=\"" << key << "\" numObs=\"" << platform.nsteps << "\">" << std::endl;
	  ofs << "      <IDs ref=\"" << filename << ".ObML." << obsTypes.types[xx] << "." << key << ".IDs.xml\" numIDs=\"" << numIDs << "\" />" << std::endl;
	  ofs << "      <temporal start=\"" << platform.start.to_string("%Y-%m-%d") << "\" end=\"" << platform.end.to_string("%Y-%m-%d") << "\" />" << std::endl;
	  ofs << "      <locations ref=\"" << filename << ".ObML." << obsTypes.types[xx] << "." << key << ".locations.xml\" />" << std::endl;
	  ofs2.open((tdir.name()+"/metadata/"+cmd_type+"/"+filename+".ObML."+obsTypes.types[xx]+"."+key+".locations.xml").c_str());
	  ofs2 << "<?xml version=\"1.0\" ?>" << std::endl;
	  ofs2 << "<locations parent=\"" << filename << ".ObML\" group=\"" << obsTypes.types[xx] << "." << key << "\">" << std::endl;
	  if (pentry.boxflags->spole == 1) {
	    ofs2 << "  <box1d row=\"0\" bitmap=\"0\" />" << std::endl;
	  }
	  for (n=0; n < 180; ++n) {
	    if (pentry.boxflags->flags[n][360] == 1) {
		ofs2 << "  <box1d row=\"" << n+1 << "\" bitmap=\"";
		for (m=0; m < 360; ++m) {
		  ofs2 << static_cast<int>(pentry.boxflags->flags[n][m]);
		}
		ofs2 << "\" />" << std::endl;
	    }
	  }
	  if (pentry.boxflags->npole == 1) {
	    ofs2 << "  <box1d row=\"181\" bitmap=\"0\" />" << std::endl;
	  }
	  ofs2 << "</locations>" << std::endl;
	  ofs2.close();
	  for (const auto& key2 : platform.data_types_table.keys()) {
	    platform.data_types_table.found(key2,de);
	    ofs << "      <dataType";
	    if (meta_args.data_format == "cpcsumm" || meta_args.data_format == "netcdf" || meta_args.data_format == "hdf5" || meta_args.data_format == "ghcnmv3") {
		ofs << " map=\"ds" << meta_args.dsnum << "\"";
	    }
	    else if (strutils::contains(meta_args.data_format,"bufr")) {
		ofs << " map=\"" << de.data->map << "\"";
	    }
	    ofs << " value=\"" << de.key << "\" numObs=\"" << de.data->nsteps << "\"";
	    if (de.data->vdata == nullptr || de.data->vdata->res_cnt == 0) {
		ofs << " />" << std::endl;
	    }
	    else {
		ofs << ">" << std::endl;
		ofs << "        <vertical min_altitude=\"" << de.data->vdata->min_altitude << "\" max_altitude=\"" << de.data->vdata->max_altitude << "\" vunits=\"" << de.data->vdata->units << "\" avg_nlev=\"" << de.data->vdata->avg_nlev/de.data->nsteps << "\" avg_vres=\"" << de.data->vdata->avg_res/de.data->vdata->res_cnt << "\" />" << std::endl;
		ofs << "      </dataType>" << std::endl;
	    }
	  }
	  ofs << "    </platform>" << std::endl;
	}
	ofs << "  </observationType>" << std::endl;
    }
  }
  ofs << "</ObML>" << std::endl;
  write_finalize(is_mss_file,filename,"ObML",tdir.name(),ofs,caller,user);
}

} // end namespace ObML

namespace FixML {

void copy_fixml(std::string metadata_file,std::string URL,std::string caller,std::string user)
{
  auto FixML_name=metautils::relative_web_filename(URL);
  strutils::replace_all(FixML_name,"/","%");
  TempDir tdir;
  if (!tdir.create(meta_directives.temp_path)) {
    metautils::log_error("copy_fixml(): unable to create temporary directory",caller,user);
  }
// create the directory tree in the temp directory
  std::stringstream output,error;
  if (unixutils::mysystem2("/bin/mkdir -p "+tdir.name()+"/metadata/wfmd",output,error) < 0) {
    metautils::log_error("copy_fixml(): unable to create a temporary directory - '"+error.str()+"'",caller,user);
  }
  TempFile itemp(meta_directives.temp_path);
  unixutils::rdadata_sync_from("/__HOST_/web/datasets/ds"+meta_args.dsnum+"/metadata/fmd/"+metadata_file,itemp.name(),meta_directives.rdadata_home,error);
  std::ifstream ifs(itemp.name().c_str());
  if (!ifs.is_open()) {
    metautils::log_error("copy_fixml(): unable to open input file",caller,user);
  }
  std::ofstream ofs((tdir.name()+"/metadata/wfmd/"+FixML_name+".FixML").c_str());
  if (!ofs.is_open()) {
    metautils::log_error("copy_fixml(): unable to open output file",caller,user);
  }
  char line[32768];
  ifs.getline(line,32768);
  while (!ifs.eof()) {
    std::string line_s=line;
    if (strutils::contains(line_s,"uri=")) {
	line_s=line_s.substr(line_s.find(" format"));
	line_s="      uri=\""+URL+"\""+line_s;
    }
    else if (strutils::contains(line_s,"ref=")) {
	strutils::replace_all(line_s,metadata_file,FixML_name+".FixML");
    }
    ofs << line_s << std::endl;
    ifs.getline(line,32768);
  }
  ifs.close();
  ofs.close();
  std::string herror;
  if (unixutils::rdadata_sync(tdir.name(),"metadata/wfmd/","/data/web/datasets/ds"+meta_args.dsnum,meta_directives.rdadata_home,herror) < 0) {
    metautils::log_warning("copy_fixml(): unable to sync '"+FixML_name+".FixML' - rdadata_sync error(s): '"+herror+"'",caller,user);
  }
  copy_ancillary_files("FixML",metadata_file,FixML_name,caller,user);
  meta_args.filename=FixML_name;
}

void write_fixml(my::map<FeatureEntry>& feature_table,my::map<StageEntry>& stage_table,std::string caller,std::string user)
{
  std::ofstream ofs,ofs2;
  TempFile tfile2(meta_directives.temp_path);
  std::string filename,cmd_type,sdum,output,error;
  FeatureEntry fe;
  StageEntry se;
  ClassificationEntry ce;
  size_t n,m;
  MySQL::Server server;
  bool is_mss_file=true;

  TempDir tdir;
  if (!tdir.create(meta_directives.temp_path)) {
    metautils::log_error("write_fixml(): unable to create temporary directory",caller,user);
  }
  write_initialize(is_mss_file,filename,"FixML",tdir.name(),ofs,caller,user);
  ofs << "<FixML xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << "       xsi:schemaLocation=\"https://rda.ucar.edu/schemas" << std::endl;
  ofs << "                           https://rda.ucar.edu/schemas/FixML.xsd\"" << std::endl;
  ofs << "       uri=\"";
  if (is_mss_file) {
    cmd_type="fmd";
    ofs << "file://MSS:";
  }
  else {
    cmd_type="wfmd";
  }
  ofs << meta_args.path << "/" << meta_args.filename << "\" format=\"";
  if (meta_args.data_format == "hurdat") {
    ofs << "HURDAT";
  }
  else if (meta_args.data_format == "cxml") {
    ofs << "CXML";
  }
  else if (meta_args.data_format == "tcvitals") {
    ofs << "proprietary_ASCII";
  }
  ofs << "\">" << std::endl;
  ofs << "  <timeStamp value=\"" << dateutils::current_date_time().to_string("%Y-%m-%d %T %Z") << "\" />" << std::endl;
  ofs.setf(std::ios::fixed);
  ofs.precision(1);
  for (const auto& key : feature_table.keys()) {
    feature_table.found(key,fe);
    ofs << "  <feature ID=\"" << fe.key;
    if (fe.data->alt_ID.length() > 0) {
	ofs << "-" << fe.data->alt_ID;
    }
    ofs << "\">" << std::endl;
    for (const auto& class_ : fe.data->classification_list) {
	ce=class_;
	ofs << "    <classification stage=\"" << ce.key << "\" nfixes=\"" << ce.nfixes << "\"";
	if (ce.src.length() > 0) {
	  ofs << " source=\"" << ce.src << "\"";
	}
	ofs << ">" << std::endl;
	ofs << "      <start dateTime=\"" << ce.start_datetime.to_string("%Y-%m-%d %H:%MM %Z") << "\" latitude=\"" << fabs(ce.start_lat);
	if (ce.start_lat >= 0.) {
	  ofs << "N";
	}
	else {
	  ofs << "S";
	}
	ofs << "\" longitude=\"" << fabs(ce.start_lon);
	if (ce.start_lon < 0.) {
	  ofs << "W";
	}
	else {
	  ofs << "E";
	}
	ofs << "\" />" << std::endl;
	ofs << "      <end dateTime=\"" << ce.end_datetime.to_string("%Y-%m-%d %H:%MM %Z") << "\" latitude=\"" << fabs(ce.end_lat);
	if (ce.end_lat >= 0.) {
	  ofs << "N";
	}
	else {
	  ofs << "S";
	}
	ofs << "\" longitude=\"" << fabs(ce.end_lon);
	if (ce.end_lon < 0.) {
	  ofs << "W";
	}
	else {
	  ofs << "E";
	}
	ofs << "\" />" << std::endl;
	ofs << "      <centralPressure units=\"" << ce.pres_units << "\" min=\"" << static_cast<int>(ce.min_pres) << "\" max=\"" << static_cast<int>(ce.max_pres) << "\" />" << std::endl;
	ofs << "      <windSpeed units=\"" << ce.wind_units << "\" min=\"" << static_cast<int>(ce.min_speed) << "\" max=\"" << static_cast<int>(ce.max_speed) << "\" />" << std::endl;
	ofs << "      <boundingBox southWest=\"" << ce.min_lat << "," << ce.min_lon << "\" northEast=\"" << ce.max_lat << "," << ce.max_lon << "\" />" << std::endl;
	ofs << "    </classification>" << std::endl;
    }
    ofs << "  </feature>" << std::endl;
  }
  ofs << "</FixML>" << std::endl;
  for (const auto& key : stage_table.keys()) {
    ofs2.open((tdir.name()+"/metadata/"+cmd_type+"/"+filename+".FixML."+key+".locations.xml").c_str());
    ofs2 << "<?xml version=\"1.0\" ?>" << std::endl;
    ofs2 << "<locations parent=\"" << filename << ".FixML\" stage=\"" << key << "\">" << std::endl;
    stage_table.found(key,se);
    if (se.data->boxflags.spole == 1) {
	ofs2 << "  <box1d row=\"0\" bitmap=\"0\" />" << std::endl;
    }
    for (n=0; n < 180; ++n) {
	if (se.data->boxflags.flags[n][360] == 1) {
	  ofs2 << "  <box1d row=\"" << n+1 << "\" bitmap=\"";
	  for (m=0; m < 360; ++m) {
	    ofs2 << static_cast<int>(se.data->boxflags.flags[n][m]);
	  }
	  ofs2 << "\" />" << std::endl;
	}
    }
    if (se.data->boxflags.npole == 1) {
	ofs2 << "  <box1d row=\"181\" bitmap=\"0\" />" << std::endl;
    }
    ofs2 << "</locations>" << std::endl;
    ofs2.close();
  }
  write_finalize(is_mss_file,filename,"FixML",tdir.name(),ofs,caller,user);
}

} // end namespace FixML

namespace SatML {

void write_satml(my::map<ScanLineEntry>& scan_line_table,std::list<std::string>& scan_line_table_keys,my::map<ImageEntry>& image_table,std::list<std::string>& image_table_keys,std::string caller,std::string user)
{
  std::ofstream ofs,ofs2;
  TempFile tfile2(meta_directives.temp_path);
  std::string filename,cmd_type;
  std::stringstream output,error;
  std::string herror;
  ScanLineEntry se;
  ImageEntry ie;
  bool is_mss_file=true;

  TempDir tdir;
  if (!tdir.create(meta_directives.temp_path)) {
    metautils::log_error("write_satml(): unable to create temporary directory",caller,user);
  }
  write_initialize(is_mss_file,filename,"SatML",tdir.name(),ofs,caller,user);
  ofs << "<SatML xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << "       xsi:schemaLocation=\"https://rda.ucar.edu/schemas" << std::endl;
  ofs << "                           https://rda.ucar.edu/schemas/SatML.xsd\"" << std::endl;
  ofs << "       uri=\"";
  if (is_mss_file) {
    cmd_type="fmd";
    ofs << "file://MSS:";
  }
  else {
    cmd_type="wfmd";
  }
  ofs << meta_args.path << "/" << meta_args.filename << "\" format=\"";
  if (meta_args.data_format == "noaapod") {
    ofs << "NOAA_Polar_Orbiter";
  }
  else if (meta_args.data_format == "mcidas") {
    ofs << "McIDAS";
  }
  ofs << "\">" << std::endl;
  ofs << "  <timeStamp value=\"" << dateutils::current_date_time().to_string("%Y-%m-%d %T %Z") << "\" />" << std::endl;
  if (meta_args.data_format == "noaapod") {
    for (const auto& key : scan_line_table_keys) {
	scan_line_table.found(key,se);
	ofs << "  <satellite ID=\"" << se.sat_ID << "\">" << std::endl;
	double avg_width=0.;
	double res=0.;
	for (const auto& scanline : se.scan_line_list) {
	  avg_width+=scanline.width;
	  res+=scanline.res;
	}
	ofs << "    <swath units=\"km\" avgWidth=\"" << round(avg_width/se.scan_line_list.size()) << "\" res=\"" << round(res/se.scan_line_list.size()) << "\">" << std::endl;
	ofs << "      <data type=\"" << key << "\">" << std::endl;
	ofs << "        <temporal start=\"" << se.start_date_time.to_string("%Y-%m-%d %T %Z") << "\" end=\"" << se.end_date_time.to_string("%Y-%m-%d %T %Z") << "\" />" << std::endl;
	if (se.sat_ID == "NOAA-11") {
	  ofs << "        <spectralBand units=\"um\" value=\"0.58-0.68\" />" << std::endl;
	  ofs << "        <spectralBand units=\"um\" value=\"0.725-1.1\" />" << std::endl;
	  ofs << "        <spectralBand units=\"um\" value=\"3.55-3.93\" />" << std::endl;
	  ofs << "        <spectralBand units=\"um\" value=\"10.3-11.3\" />" << std::endl;
	  ofs << "        <spectralBand units=\"um\" value=\"11.5-12.5\" />" << std::endl;
	}
	ofs << "        <scanLines ref=\"" << meta_args.filename << "." << key << ".scanLines\" num=\"" << se.scan_line_list.size() << "\" />" << std::endl;
	ofs2.open((tdir.name()+"/metadata/"+cmd_type+"/"+meta_args.filename+"."+key+".scanLines").c_str());
	ofs2 << "<?xml version=\"1.0\" ?>" << std::endl;
	ofs2 << "<scanLines parent=\"" << meta_args.filename << ".SatML\">" << std::endl;
	for (const auto& scanline : se.scan_line_list) {
	  ofs2 << "  <scanLine time=\"" << scanline.date_time.to_string("%Y-%m-%d %T %Z") << "\" numPoints=\"409\" width=\"" << round(scanline.width) << "\" res=\"" << round(scanline.res) << "\">" << std::endl;
	  ofs2 << "    <start lat=\"" << round(scanline.first_coordinate.lat*100.)/100. << "\" lon=\"" << round(scanline.first_coordinate.lon*100.)/100. << "\" />" << std::endl;
	  ofs2 << "    <subpoint lat=\"" << round(scanline.subpoint.lat*100.)/100. << "\" lon=\"" << round(scanline.subpoint.lon*100.)/100. << "\" />" << std::endl;
	  ofs2 << "    <end lat=\"" << round(scanline.last_coordinate.lat*100.)/100. << "\" lon=\"" << round(scanline.last_coordinate.lon*100.)/100. << "\" />" << std::endl;
	  ofs2 << "  </scanLine>" << std::endl;
	}
	ofs2 << "</scanLines>" << std::endl;
	ofs2.close();
	ofs << "      </data>" << std::endl;
	ofs << "    </swath>" << std::endl;
	ofs << "  </satellite>" << std::endl;
    }
  }
  else if (meta_args.data_format == "mcidas") {
    for (const auto& key : image_table_keys) {
	image_table.found(key,ie);
	ofs << "  <satellite ID=\"" << ie.key << "\">" << std::endl;
	ofs << "    <temporal start=\"" << ie.start_date_time.to_string("%Y-%m-%d %T %Z") << "\" end=\"" << ie.end_date_time.to_string("%Y-%m-%d %T %Z") << "\" />" << std::endl;
	ofs << "    <images ref=\"" << meta_args.filename << ".images\" num=\"" << ie.image_list.size() << "\" />" << std::endl;
	ofs2.open((tdir.name()+"/metadata/"+cmd_type+"/"+meta_args.filename+".images").c_str());
	ofs2 << "<?xml version=\"1.0\" ?>" << std::endl;
	ofs2 << "<images parent=\"" << meta_args.filename << ".SatML\">" << std::endl;
	for (const auto& image : ie.image_list) {
	  ofs2 << "  <image>" << std::endl;
	  ofs2 << "    <time value=\"" << image.date_time.to_string("%Y-%m-%d %T %Z") << "\" />" << std::endl;
	  ofs2 << "    <resolution units=\"km\" x=\"" << round(image.xres) << "\" y=\"" << round(image.yres) << "\" />" << std::endl;
	  if (ie.key == "GMS Visible") {
	    ofs2 << "    <spectralBand units=\"um\" value=\"0.55-1.05\" />" << std::endl;
	  }
	  else if (ie.key == "GMS Infrared") {
	    ofs2 << "    <spectralBand units=\"um\" value=\"10.5-12.5\" />" << std::endl;
	  }
	  ofs2 << "    <centerPoint lat=\"" << round(image.center.lat*100.)/100. << "\" lon=\"" << round(image.center.lon*100.)/100. << "\" />" << std::endl;
	  if (image.corners.nw_coord.lat < -99. && image.corners.nw_coord.lon < -999. && image.corners.ne_coord.lat < -99. && image.corners.ne_coord.lon < -999. && image.corners.sw_coord.lat < -99. && image.corners.sw_coord.lon < -999. && image.corners.se_coord.lat < -99. && image.corners.se_coord.lon < -999.) {
	    ofs2 << "    <fullDiskImage />" << std::endl;
	  }
	  else {
	    ofs2 << "    <northwestCorner lat=\"" << round(image.corners.nw_coord.lat*100.)/100. << "\" lon=\"" << round(image.corners.nw_coord.lon*100.)/100. << "\" />" << std::endl;
	    ofs2 << "    <southwestCorner lat=\"" << round(image.corners.sw_coord.lat*100.)/100. << "\" lon=\"" << round(image.corners.sw_coord.lon*100.)/100. << "\" />" << std::endl;
	    ofs2 << "    <southeastCorner lat=\"" << round(image.corners.se_coord.lat*100.)/100. << "\" lon=\"" << round(image.corners.se_coord.lon*100.)/100. << "\" />" << std::endl;
	    ofs2 << "    <northeastCorner lat=\"" << round(image.corners.ne_coord.lat*100.)/100. << "\" lon=\"" << round(image.corners.ne_coord.lon*100.)/100. << "\" />" << std::endl;
	  }
	  ofs2 << "  </image>" << std::endl;
	}
	ofs2 << "</images>" << std::endl;
	ofs2.close();
	ofs << "  </satellite>" << std::endl;
    }
  }
  ofs << "</SatML>" << std::endl;
  write_finalize(is_mss_file,filename,"SatML",tdir.name(),ofs,caller,user);
}

} // end namespace SatML

} // end namespace metadata
