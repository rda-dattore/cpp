#include <fstream>
#include <sys/stat.h>
#include <metadata_export_pg.hpp>
#include <metadata.hpp>
#include <xml.hpp>
#include <PostgreSQL.hpp>
#include <mymap.hpp>
#include <tempfile.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <gridutils.hpp>
#include <bsort.hpp>

using namespace PostgreSQL;
using std::endl;
using strutils::replace_all;
using strutils::substitute;

namespace metadataExport {

bool export_to_dif(std::ostream& ofs,std::string dsid,XMLDocument& xdoc,size_t indent_length)
{
  const char *res_keywords[]={
  "1 km - &lt; 10 km or approximately .01 degree - &lt; .09 degree",
  "10 km - &lt; 50 km or approximately .09 degree - &lt; .5 degree",
  "50 km - &lt; 100 km or approximately .5 degree - &lt; 1 degree",
  "100 km - &lt; 250 km or approximately 1 degree - &lt; 2.5 degrees",
  "250 km - &lt; 500 km or approximately 2.5 degrees - &lt; 5.0 degrees",
  "500 km - &lt; 1000 km or approximately 5 degrees - &lt; 10 degrees",
  "&gt;= 1000 km or &gt;= 10 degrees",
  "&lt; 1 meter",
  "1 meter - &lt; 30 meters",
  "30 meters - &lt; 100 meters",
  "100 meters - &lt; 250 meters",
  "250 meters - &lt; 500 meters",
  "500 meters - &lt; 1 km"
  };
  TempDir temp_dir;
  if (!temp_dir.create("/tmp")) {
    std::cout << "Content-type: text/plain" << endl << endl;
    std::cout << "Error creating temporary directory" << endl;
    exit(1);
  }
  Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"rdadb");
  std::string indent(indent_length,' ');
  auto e=xdoc.element("dsOverview/creator@vocabulary=GCMD");
  auto dss_centername=e.attribute_value("name");
  if (strutils::contains(dss_centername,"/SCD/")) {
    replace_all(dss_centername,"SCD","CISL");
  }
  ofs << indent << "<DIF xmlns=\"http://gcmd.gsfc.nasa.gov/Aboutus/xml/dif/\"" << endl;
  ofs << indent << "     xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << endl;
  ofs << indent << "     xsi:schemaLocation=\"http://gcmd.gsfc.nasa.gov/Aboutus/xml/dif/" << endl;
  ofs << indent << "                         http://gcmd.gsfc.nasa.gov/Aboutus/xml/dif/dif_v9.7.1.xsd\">" << endl;
  ofs << indent << "  <Entry_ID>NCAR_GDEX_" << dsid << "</Entry_ID>" << std::
      endl;
  e=xdoc.element("dsOverview/title");
  auto dstitle=e.content();
  ofs << indent << "  <Entry_Title>" << dstitle << "</Entry_Title>" << endl;
  ofs << indent << "  <Data_Set_Citation>" << endl;
  ofs << indent << "    <Dataset_Creator>" << dss_centername << "</Dataset_Creator>" << endl;
  ofs << indent << "    <Dataset_Title>" << dstitle << "</Dataset_Title>" << endl;
  ofs << indent << "    <Dataset_Publisher>" << dss_centername << "</Dataset_Publisher>" << endl;
  ofs << indent << "    <Online_Resource>https://rda.ucar.edu/datasets/" << dsid << "/</Online_Resource>" << endl;
  ofs << indent << "  </Data_Set_Citation>" << endl;
  auto elist=xdoc.element_list("dsOverview/contact");
  for (const auto& element : elist) {
    ofs << indent << "  <Personnel>" << endl;
    ofs << indent << "    <Role>Technical Contact</Role>" << endl;
    auto contact_name=element.content();
    auto idx=contact_name.find(" ");
    auto fname=contact_name.substr(0,idx);
    auto lname=contact_name.substr(idx+1);
    LocalQuery query("logname,phoneno","dssdb.dssgrp","fstname = '"+fname+"' and lstname = '"+lname+"'");
    if (query.submit(server) < 0) {
        std::cout << "Content-type: text/plain" << endl << endl;
        std::cout << "Database error: " << query.error() << endl;
        exit(1);
    }
    Row row;
    query.fetch_row(row);
    ofs << indent << "    <First_Name>" << fname << "</First_Name>" << endl;
    ofs << indent << "    <Last_Name>" << lname << "</Last_Name>" << endl;
    ofs << indent << "    <Email>" << row[0] << "@ucar.edu</Email>" << endl;
    ofs << indent << "    <Phone>" << row[1] << "</Phone>" << endl;
    ofs << indent << "    <Fax>(303)-497-1291</Fax>" << endl;
    ofs << indent << "    <Contact_Address>" << endl;
    ofs << indent << "      <Address>National Center for Atmospheric Research</Address>" << endl;
    ofs << indent << "      <Address>CISL/DSS</Address>" << endl;
    ofs << indent << "      <Address>P.O. Box 3000</Address>" << endl;
    ofs << indent << "      <City>Boulder</City>" << endl;
    ofs << indent << "      <Province_or_State>CO</Province_or_State>" << endl;
    ofs << indent << "      <Postal_Code>80307</Postal_Code>" << endl;
    ofs << indent << "      <Country>U.S.A.</Country>" << endl;
    ofs << indent << "    </Contact_Address>" << endl;
    ofs << indent << "  </Personnel>" << endl;
  }
  ofs << indent << "  <Personnel>" << endl;
  ofs << indent << "    <Role>DIF Author</Role>" << endl;
  ofs << indent << "    <First_Name>Bob</First_Name>" << endl;
  ofs << indent << "    <Last_Name>Dattore</Last_Name>" << endl;
  ofs << indent << "    <Email>dattore@ucar.edu</Email>" << endl;
  ofs << indent << "    <Phone>(303)-497-1825</Phone>" << endl;
  ofs << indent << "    <Fax>(303)-497-1291</Fax>" << endl;
  ofs << indent << "    <Contact_Address>" << endl;
  ofs << indent << "      <Address>National Center for Atmospheric Research</Address>" << endl;
  ofs << indent << "      <Address>CISL/DSS</Address>" << endl;
  ofs << indent << "      <Address>P.O. Box 3000</Address>" << endl;
  ofs << indent << "      <City>Boulder</City>" << endl;
  ofs << indent << "      <Province_or_State>CO</Province_or_State>" << endl;
  ofs << indent << "      <Postal_Code>80307</Postal_Code>" << endl;
  ofs << indent << "      <Country>U.S.A.</Country>" << endl;
  ofs << indent << "    </Contact_Address>" << endl;
  ofs << indent << "  </Personnel>" << endl;
  LocalQuery query("select g.path from search.variables as v left join search.gcmd_sciencekeywords as g on g.uuid = v.keyword where v.dsid = '"+dsid+"' and v.vocabulary = 'GCMD'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << endl << endl;
    std::cout << "Database error: " << query.error() << endl;
    exit(1);
  }
  for (const auto& row : query) {
    auto parts=strutils::split(row[0]," > ");
    ofs << indent << "  <Parameters>" << endl;
    ofs << indent << "    <Category>" << parts[0] << "</Category>" << endl;
    ofs << indent << "    <Topic>" << parts[1] << "</Topic>" << endl;
    ofs << indent << "    <Term>" << parts[2] << "</Term>" << endl;
    ofs << indent << "    <Variable_Level_1>" << parts[3] << "</Variable_Level_1>" << endl;
    if (parts.size() > 4) {
	ofs << indent << "    <Variable_Level_2>" << parts[4] << "</Variable_Level_2>" << endl;
	if (parts.size() > 5) {
	  ofs << indent << "    <Variable_Level_3>" << parts[5] << "</Variable_Level_3>" << endl;
	  if (parts.size() > 6) {
	    ofs << indent << "    <Detailed_Variable>" << parts[6] << "</Detailed_Variable>" << endl;
	  }
	}
    }
    ofs << indent << "  </Parameters>" << endl;
  }
  e=xdoc.element("dsOverview/topic@vocabulary=ISO");
  ofs << indent << "  <ISO_Topic_Category>" << e.content() << "</ISO_Topic_Category>" << endl;
  query.set("select g.path from search.platforms_new as p left join search.gcmd_platforms as g on g.uuid = p.keyword where p.dsid = '"+dsid+"' and p.vocabulary = 'GCMD'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << endl << endl;
    std::cout << "Database error: " << query.error() << endl;
    exit(1);
  }
  for (const auto& row : query) {
    ofs << indent << "  <Source_Name>" << endl;
    auto index=row[0].find(" > ");
    if (index != std::string::npos) {
	ofs << indent << "   <Short_Name>" << row[0].substr(0,index) <<
            "</Short_Name>" << endl;
	ofs << indent << "   <Long_Name>" << substitute(row[0].substr(index+3),
            "&", "&amp;") << "</Long_Name>" << endl;
    } else {
	ofs << indent << "    <Short_Name>" << row[0] << "</Short_Name>" <<
            endl;
    }
    ofs << indent << "  </Source_Name>" << endl;
  }
  query.set("min(date_start),max(date_end)","dssdb.dsperiod","dsid = '"+dsid+"' and date_start < '9998-01-01' and date_end < '9998-01-01'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << endl << endl;
    std::cout << "Database error: " << query.error() << endl;
    exit(1);
  }
  if (query.num_rows() > 0) {
    Row row;
    query.fetch_row(row);
    ofs << indent << "  <Temporal_Coverage>" << endl;
    ofs << indent << "    <Start_Date>" << row[0] << "</Start_Date>" << endl;
    ofs << indent << "    <Stop_Date>" << row[1] << "</Stop_Date>" << endl;
    ofs << indent << "  </Temporal_Coverage>" << endl;
  }
  e=xdoc.element("dsOverview/continuingUpdate");
  if (e.attribute_value("value") == "no") {
    ofs << indent << "  <Data_Set_Progress>Complete</Data_Set_Progress>" << endl;
  }
  else {
    ofs << indent << "  <Data_Set_Progress>In Work</Data_Set_Progress>" << endl;
  }
  query.set("select definition, def_params from (select distinct grid_definition_code from \"WGrML\".summary where dsid = '"+dsid+"') as s left join \"WGrML\".grid_definitions as d on d.code = s.grid_definition_code");
  query.submit(server);
  if (query.num_rows() > 0) {
    double min_west_lon=9999.,min_south_lat=9999.,max_east_lon=-9999.,max_north_lat=-9999.;
    my::map<Entry> resolution_table;
    for (const auto& row : query) {
	double west_lon,south_lat,east_lon,north_lat;
	if (gridutils::filled_spatial_domain_from_grid_definition(row[0]+"<!>"+row[1],"primeMeridian",west_lon,south_lat,east_lon,north_lat)) {
	  if (west_lon < min_west_lon) {
	    min_west_lon=west_lon;
	  }
	  if (east_lon > max_east_lon) {
	    max_east_lon=east_lon;
	  }
	  if (south_lat < min_south_lat) {
	    min_south_lat=south_lat;
	  }
	  if (north_lat > max_north_lat) {
	    max_north_lat=north_lat;
	  }
	}
	auto sdum=gridutils::convert_grid_definition(row[0]+"<!>"+row[1]);
	if (strutils::contains(sdum,"&deg; x")) {
	  auto sp=strutils::split(sdum," x ");
	  sdum=sp[1];
	  sdum=sdum.substr(0,sdum.find("&deg;"));
	  replace_all(sdum,"~","");
	  add_to_resolution_table(std::stof(sdum),std::stof(substitute(sp[0],"&deg;","")),"degrees",resolution_table);
	}
	else if (strutils::contains(sdum,"km x")) {
	  auto sp=strutils::split(sdum," x ");
	  sdum=sp[1];
	  sdum=sdum.substr(0,sdum.find("km"));
	  add_to_resolution_table(std::stof(sdum),std::stof(substitute(sp[0],"km","")),"km",resolution_table);
	}
    }
    if (max_north_lat > -999.) {
	ofs << indent << "  <Spatial_Coverage>" << endl;
	ofs << indent << "    <Southernmost_Latitude>" << fabs(min_south_lat);
	if (min_south_lat < 0.) {
	  ofs << " South";
	}
	else {
	  ofs << " North";
	}
	ofs << "</Southernmost_Latitude>" << endl;
	ofs << indent << "    <Northernmost_Latitude>" << fabs(max_north_lat);
	if (max_north_lat < 0.) {
	  ofs << " South";
	}
	else {
	  ofs << " North";
	}
	ofs << "</Northernmost_Latitude>" << endl;
	if (min_west_lon < -180.) {
	  min_west_lon+=360.;
	}
	ofs << indent << "    <Westernmost_Longitude>" << fabs(min_west_lon);
	if (min_west_lon < 0.) {
	  ofs << " West";
	}
	else {
	  ofs << " East";
	}
	ofs << "</Westernmost_Longitude>" << endl;
	if (max_east_lon > 180.) {
	  max_east_lon-=360.;
	}
	ofs << indent << "    <Easternmost_Longitude>" << fabs(max_east_lon);
	if (max_east_lon < 0.) {
	  ofs << " West";
	}
	else {
	  ofs << " East";
	}
	ofs << "</Easternmost_Longitude>" << endl;
	ofs << indent << "  </Spatial_Coverage>" << endl;
    }
    for (const auto& key : resolution_table.keys()) {
	ofs << indent << "  <Data_Resolution>" << endl;
	Entry entry;
	resolution_table.found(key,entry);
	auto sp=strutils::split(entry.key,"<!>");
	ofs << indent << "    <Latitude_Resolution>" << entry.min_lat << " " << sp[1] << "</Latitude_Resolution>" << endl;
	ofs << indent << "    <Longitude_Resolution>" << entry.min_lon << " " << sp[1] << "</Longitude_Resolution>" << endl;
	ofs << indent << "    <Horizontal_Resolution_Range>" << res_keywords[std::stoi(sp[0])] << "</Horizontal_Resolution_Range>" << endl;
	ofs << indent << "  </Data_Resolution>" << endl;
    }
  }
/*
    query.set("keyword","search.time_resolutions","dsid = '"+dsid+"' and vocabulary = 'GCMD'");
    if (query.submit(server) < 0) {
	std::cout << "Content-type: text/plain" << endl << endl;
	std::cout << "Database error: " << query.error() << endl;
	exit(1);
    }
    if (query.num_rows() > 0) {
	while (query.fetch_row(row)) {
	ofs << "  <Data_Resolution>" << endl;
	ofs << "  </Data_Resolution>" << endl;
    }
*/
  elist=xdoc.element_list("dsOverview/project@vocabulary=GCMD");
  for (const auto& element : elist) {
    ofs << indent << "  <Project>" << endl;
    auto project=element.content();
    if (strutils::contains(project," > ")) {
      ofs << indent << "   <Short_Name>" << project.substr(0,project.find(" > ")) << "</Short_Name>" << endl;
      ofs << indent << "   <Long_Name>" << project.substr(project.find(" > ")+3) << "</Long_Name>" << endl;
    }
    else {
      ofs << indent << "    <Short_Name>" << project << "</Short_Name>" << endl;
    }
    ofs << indent << "  </Project>" << endl;
  }
  e=xdoc.element("dsOverview/restrictions/access");
  if (e.name() == "access") {
    auto access_restrictions=e.to_string();
    ofs << indent << "  <Access_Constraints>" << endl;
    ofs << htmlutils::convert_html_summary_to_ascii(access_restrictions,80,indent_length+4) << endl;
    ofs << indent << "  </Access_Constraints>" << endl;
  }
  e=xdoc.element("dsOverview/restrictions/usage");
  if (e.name() == "usage") {
    auto usage_restrictions=e.to_string();
    ofs << indent << "  <Use_Constraints>" << endl;
    ofs << htmlutils::convert_html_summary_to_ascii(usage_restrictions,80,indent_length+4) << endl;
    ofs << indent << "  </Use_Constraints>" << endl;
  }
  ofs << indent << "  <Data_Set_Language>English</Data_Set_Language>" << endl;
  ofs << indent << "  <Data_Center>" << endl;
  ofs << indent << "    <Data_Center_Name>" << endl;
  ofs << indent << "      <Short_Name>" << dss_centername.substr(0,dss_centername.find(" > ")) << "</Short_Name>" << endl;
  ofs << indent << "      <Long_Name>" << dss_centername.substr(dss_centername.find(" > ")+3) << "</Long_Name>" << endl;
  ofs << indent << "    </Data_Center_Name>" << endl;
  ofs << indent << "    <Data_Center_URL>https://rda.ucar.edu/</Data_Center_URL>" << endl;
  ofs << indent << "    <Data_Set_ID>" << dsid << "</Data_Set_ID>" << endl;
  ofs << indent << "    <Personnel>" << endl;
  ofs << indent << "      <Role>DATA CENTER CONTACT</Role>" << endl;
  ofs << indent << "      <Last_Name>DSS Help Desk</Last_Name>" << endl;
  ofs << indent << "      <Email>dssweb@ucar.edu</Email>" << endl;
  ofs << indent << "      <Phone>(303)-497-1231</Phone>" << endl;
  ofs << indent << "      <Fax>(303)-497-1291</Fax>" << endl;
  ofs << indent << "      <Contact_Address>" << endl;
  ofs << indent << "        <Address>National Center for Atmospheric Research</Address>" << endl;
  ofs << indent << "        <Address>CISL/DSS</Address>" << endl;
  ofs << indent << "        <Address>P.O. Box 3000</Address>" << endl;
  ofs << indent << "        <City>Boulder</City>" << endl;
  ofs << indent << "        <Province_or_State>CO</Province_or_State>" << endl;
  ofs << indent << "        <Postal_Code>80307</Postal_Code>" << endl;
  ofs << indent << "        <Country>U.S.A.</Country>" << endl;
  ofs << indent << "      </Contact_Address>" << endl;
  ofs << indent << "    </Personnel>" << endl;
  ofs << indent << "  </Data_Center>" << endl;
  auto distribution_size=primary_size("('" + dsid + "')", server);
  if (!distribution_size.empty()) {
    ofs << indent << "  <Distribution>" << endl;
    ofs << indent << "    <Distribution_Size>" << distribution_size << "</Distribution_Size>" << endl;
    ofs << indent << "  </Distribution>" << endl;
  }
  query.set("keyword","search.formats","dsid = '"+dsid+"'");
  if (query.submit(server) < 0) {
    std::cout << "Content-type: text/plain" << endl << endl;
    std::cout << "Database error: " << query.error() << endl;
    exit(1);
  }
  struct stat buf;
  std::string format_ref;
  if (stat((metautils::directives.server_root+"/web/metadata/FormatReferences.xml.new").c_str(),&buf) == 0) {
    format_ref=metautils::directives.server_root+"/web/metadata/FormatReferences.xml.new";
  }
  else {
    format_ref=unixutils::remote_web_file("https://rda.ucar.edu/metadata/FormatReferences.xml.new",temp_dir.name());
  }
  XMLDocument fdoc(format_ref);
  my::map<Entry> format_table;
  for (const auto& row : query) {
    auto format=row[0];
    replace_all(format,"proprietary_","");
    e=fdoc.element(("formatReferences/format@name="+format+"/DIF").c_str());
    if (e.name() != "DIF") {
	e=fdoc.element(("formatReferences/format@name="+format+"/type").c_str());
    }
    Entry entry;
    entry.key=e.content();
    if (!format_table.found(entry.key,entry)) {
	format_table.insert(entry);
    }
  }
  fdoc.close();
  for (const auto& key : format_table.keys()) {
    ofs << indent << "  <Distribution>" << endl;
    ofs << indent << "    <Distribution_Format>" << key << "</Distribution_Format>" << endl;
    ofs << indent << "  </Distribution>" << endl;
  }
  elist=xdoc.element_list("dsOverview/reference");
  if (elist.size() > 0) {
    std::vector<XMLElement> earray;
    earray.reserve(elist.size());
    for (const auto& element : elist) {
	earray.emplace_back(element);
    }
    binary_sort(earray,compare_references);
    ofs << indent << "  <Reference>" << endl;
    for (size_t n=0; n < elist.size(); ++n) {
	if (n > 0) {
	  ofs << endl;
	}
	auto reference="<reference><p>"+earray[n].element("authorList").content()+", "+earray[n].element("year").content()+": "+earray[n].element("title").content()+". ";
	auto ref_type=earray[n].attribute_value("type");
	if (ref_type == "journal") {
	  e=earray[n].element("periodical");
	  reference+=e.content()+", "+e.attribute_value("number")+", "+e.attribute_value("pages");
	}
	else if (ref_type == "preprint") {
	  e=earray[n].element("conference");
	  reference+=" Proceedings of the "+e.content()+", "+e.attribute_value("host")+", "+e.attribute_value("location")+", "+e.attribute_value("pages");
	}
	else if (ref_type == "technical_report") {
	  e=earray[n].element("organization");
	  reference+=e.attribute_value("reportID")+", "+e.content()+", "+e.attribute_value("pages")+" pp.";
	}
	else if (ref_type == "book") {
	  e=earray[n].element("publisher");
	  reference+=e.content()+", "+e.attribute_value("place");
	}
	auto doi=earray[n].element("doi").content();
	if (!doi.empty()) {
	  reference+=" (DOI: "+doi+")";
	}
	auto url=earray[n].element("url").content();
	if (!url.empty()) {
	  reference+=", URL: "+url;
	}
	reference+=".</p></reference>";
	ofs << htmlutils::convert_html_summary_to_ascii(reference,80,indent_length+4) << endl;
    }
    ofs << indent << "  </Reference>" << endl;
  }
  ofs << indent << "  <Summary>" << endl;
  e=xdoc.element("dsOverview/summary");
  ofs << htmlutils::convert_html_summary_to_ascii(e.to_string(),80,indent_length+4) << endl;
  ofs << indent << "  </Summary>" << endl;
  elist=xdoc.element_list("dsOverview/relatedDataset");
  for (const auto& element : elist) {
    ofs << indent << "  <Related_URL>" << endl;
    ofs << indent << "    <URL_Content_Type>" << endl;
    ofs << indent << "      <Type>VIEW RELATED INFORMATION</Type>" << endl;
    ofs << indent << "    </URL_Content_Type>" << endl;
    ofs << indent << "    <URL>https://rda.ucar.edu/datasets/" << element.attribute_value("ID") << "/</URL>" << endl;
    ofs << indent << "  </Related_URL>" << endl;
  }
  elist=xdoc.element_list("dsOverview/relatedResource");
  for (const auto& element : elist) {
    ofs << indent << "  <Related_URL>" << endl;
    ofs << indent << "    <URL_Content_Type>" << endl;
    ofs << indent << "      <Type>VIEW RELATED INFORMATION</Type>" << endl;
    ofs << indent << "    </URL_Content_Type>" << endl;
    ofs << indent << "    <URL>" << element.attribute_value("url") << "</URL>" << endl;
    ofs << indent << "    <Description>" << element.content() << "</Description>" << endl;
    ofs << indent << "  </Related_URL>" << endl;
  }
  ofs << indent << "  <IDN_Node>" << endl;
  ofs << indent << "    <Short_Name>USA/NCAR</Short_Name>" << endl;
  ofs << indent << "  </IDN_Node>" << endl;
  ofs << indent << "  <Metadata_Name>CEOS IDN DIF</Metadata_Name>" << endl;
  ofs << indent << "  <Metadata_Version>9.7</Metadata_Version>" << endl;
  ofs << indent << "</DIF>" << endl;                    
  server.disconnect();
  return true;
}

} // end namespace metadataExport
