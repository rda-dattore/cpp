#include <fstream>
#include <regex>
#include <metadata_export_pg.hpp>
#include <metadata.hpp>
#include <xml.hpp>
#include <PostgreSQL.hpp>
#include <strutils.hpp>
#include <utils.hpp>

using namespace PostgreSQL;

namespace metadataExport {

bool export_to_thredds(std::ostream& ofs,std::string ident,XMLDocument& xdoc,size_t indent_length)
{
  std::string indent(indent_length,' ');
  Server server(metautils::directives.database_server,metautils::directives.metadb_username,metautils::directives.metadb_password,"rdadb");
  ofs << indent << "<catalog xmlns=\"http://www.unidata.ucar.edu/namespaces/thredds/InvCatalog/v1.0\"" << std::endl;
  ofs << indent << "         xmlns:xlink=\"http://www.w3.org/1999/xlink\"" << std::endl;
  ofs << indent << "         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
  ofs << indent << "         xsi:schemaLocation=\"http://www.unidata.ucar.edu/namespaces/thredds/InvCatalog/v1.0" << std::endl;
  ofs << indent << "                             http://www.unidata.ucar.edu/schemas/thredds/InvCatalog.1.0.xsd\"" << std::endl;
  ofs << indent << "         name=";
  if (ident.length() == 5 && ident[3] == '.' && strutils::is_numeric(strutils::substitute(ident,".",""))) {
    LocalQuery query("select title,summary from search.datasets where dsid = '"+ident+"'");
    if (query.submit(server) < 0) {
	std::cout << "Content-type: text/plain" << std::endl << std::endl;
	std::cout << "Database error: " << query.error() << std::endl;
	exit(1);
    }
    Row row;
    query.fetch_row(row);
    auto title=strutils::substitute(row[0],"\"","&quot;");
    ofs << "\"" << title << " (ds" << ident << ")\">" << std::endl;
    ofs << indent << "  <service base=\"https://rda.ucar.edu/\" name=\"RDADB Server\" serviceType=\"HTTPServer\" desc=\"UCAR/CISL Research Data Archive\" />" << std::endl;
    ofs << indent << "  <dataset name=\"" << title << " (ds" << ident << ")\" ID=\"edu.ucar.dss.ds" << ident << "\" harvest=\"true\">" << std::endl;
    ofs << indent << "    <metadata>" << std::endl;
    ofs << indent << "      <publisher>" << std::endl;
    ofs << indent << "        <name vocabulary=\"DIF\">UCAR/NCAR/CISL/DSS > Data Support Section, Computational and Information Systems Laboratory, National Center for Atmospheric Research, University Corporation for Atmospheric Research</name>" << std::endl;
    ofs << indent << "        <contact url=\"https://rda.ucar.edu/\" email=\"dssweb@ucar.edu\" />" << std::endl;
    ofs << indent << "      </publisher>" << std::endl;
    ofs << indent << "      <documentation type=\"summary\">" << std::endl;
    ofs << indent << "        " << htmlutils::convert_html_summary_to_ascii("<summary>"+row[1]+"</summary>",72-indent_length,8+indent_length) << std::endl;
    ofs << indent << "      </documentation>" << std::endl;
    ofs << indent << "    </metadata>" << std::endl;
    ofs << indent << "  </dataset>" << std::endl;
  }
  else if (strutils::has_beginning(ident,"range")) {
    LocalQuery query("select * from dssdb.dsranges where min = '"+ident.substr(5)+"'");
    if (query.submit(server) < 0) {
	std::cout << "Content-type: text/plain" << std::endl << std::endl;
	std::cout << "Database error: " << query.error() << std::endl;
	exit(1);
    }
    Row row;
    query.fetch_row(row);
    ofs << "\"Datasets "+row[1]+".0-"+row[2]+".9: "+row[0]+"\">" << std::endl;
    ofs << indent << "  <dataset name=\"Datasets "+row[1]+".0-"+row[2]+".9: "+row[0]+"\" ID=\"ucar.scd.dss.range"+row[1]+"\">" << std::endl;
    ofs << indent << "    <metadata>" << std::endl;
    ofs << indent << "      <publisher>" << std::endl;
    ofs << indent << "        <name vocabulary=\"DIF\">UCAR/NCAR/CISL/DSS > Data Support Section, Computational and Information Systems Laboratory, National Center for Atmospheric Research, University Corporation for Atmospheric Research</name>" << std::endl;
    ofs << indent << "      </publisher>" << std::endl;
    ofs << indent << "    </metadata>" << std::endl;
    query.set("select dsid,title from search.datasets where (type = 'P' or type = 'H') and dsid != '999.9' and dsid >= '"+row[1]+".0' and dsid <= '"+row[2]+".9'");
    if (query.submit(server) < 0) {
	std::cout << "Content-type: text/plain" << std::endl << std::endl;
	std::cout << "Database error: " << query.error() << std::endl;
	exit(1);
    }
    while (query.fetch_row(row)) {
	ofs << indent << "    <catalogRef xlink:href=\"https://rda.ucar.edu/metadata/thredds/ds_catalogs/ds"+row[0]+".thredds\" xlink:title=\"ds"+row[0]+":  "+row[1]+"\" />" << std::endl;
    }
    ofs << indent << "  </dataset>" << std::endl;
  }
  else if (ident == "main") {
    ofs << "\"DSS Dataset Holdings\">" << std::endl;
    ofs << indent << "  <dataset name=\"DSS Dataset Holdings\" ID=\"ucar.scd.dss\">" << std::endl;
    ofs << indent << "    <metadata>" << std::endl;
    ofs << indent << "      <publisher>" << std::endl;
    ofs << indent << "        <name vocabulary=\"DIF\">UCAR/NCAR/SCD/DSS > Data Support Section, Scientific Computing Division, National Center for Atmospheric Research, University Corporation for Atmospheric Research</name>" << std::endl;
    ofs << indent << "        <contact url=\"https://rda.ucar.edu/\" email=\"dssweb@ucar.edu\" />" << std::endl;
    ofs << indent << "      </publisher>" << std::endl;
    ofs << indent << "    </metadata>" << std::endl;
    ofs << indent << "    <documentation type=\"summary\">" << std::endl;
    ofs << indent << "      The Data Support Section (DSS) manages a large archive of meteorological, oceanographic, and related research data that have been assembled from various and numerous sources from around the world." << std::endl;
    ofs << indent << "    </documentation>" << std::endl;
    ofs << indent << "    <documentation xlink:href=\"https://rda.ucar.edu/\" xlink:title=\"RDA Home Page\" xlink:show=\"new\" />" << std::endl;
    LocalQuery query("select * from dssdb.dsranges");
    if (query.submit(server) < 0) {
	std::cout << "Content-type: text/plain" << std::endl << std::endl;
	std::cout << "Database error: " << query.error() << std::endl;
	exit(1);
    }
    for (const auto& row : query) {
	if (!strutils::has_beginning(row[0],"Reserved"))
	  ofs << indent << "    <catalogRef xlink:href=\"https://rda.ucar.edu/datasets/ranges/range"+row[1]+".thredds\" xlink:title=\"Datasets "+row[1]+".0-"+row[2]+".9: "+row[0]+"\" />" << std::endl;
    }
    ofs << indent << "  </dataset>" << std::endl;
  }
  ofs << indent << "</catalog>" << std::endl;
  server.disconnect();
  return true;
}

} // end namespace metadataExport
