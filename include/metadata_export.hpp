#ifndef METADATA_EXPORT_H
#define   METADATA_EXPORT_H

#include <vector>
#include <mymap.hpp>
#include <xml.hpp>
#include <PostgreSQL.hpp>
#include <tokendoc.hpp>

namespace metadataExport {

const std::string DATACITE_HOSTING_INSTITUTION = "University Corporation For "
    "Atmospheric Research (UCAR):National Center for Atmospheric Research "
    "(NCAR):Computational and Information Systems Laboratory (CISL):"
    "Information Services Division (ISD):Data Engineering and Curation Section "
    "(DECS)";
const std::string PUBLISHER = "UCAR/NCAR - Research Data Archive";

struct Entry {
  Entry() : key(), min_lon(0.), min_lat(0.) { }

  std::string key;
  double min_lon, min_lat;
};

struct HorizontalResolutionEntry {
  HorizontalResolutionEntry() : key(), resolution(0.), uom() { }

  std::string key;
  double resolution;
  std::string uom;
};

struct PThreadStruct {
  PThreadStruct() : query(), tid(0) { }

  PostgreSQL::LocalQuery query;
  pthread_t tid;
};

extern "C" void *runQuery(void *ts);

extern std::string primary_size(std::string dsnum, PostgreSQL::Server& server);

extern void add_replace_from_element_content(XMLDocument& xdoc, std::string
    xpath, std::string replace_token, TokenDocument& token_doc, std::string
    if_key = "");
extern void add_to_resolution_table(double lon_res, double lat_res, std::string
    units, my::map<Entry>& resolution_table);
extern void fill_geographic_extent_data(PostgreSQL::Server& server, std::string
    dsnum, XMLDocument& dataset_overview, double& min_west_lon, double&
    min_south_lat, double& max_east_lon, double& max_north_lat, bool& is_grid,
    std::vector<HorizontalResolutionEntry> *hres_list = nullptr, my::map<Entry>
    *unique_places_table = nullptr);

extern bool compare_references(XMLElement& left, XMLElement& right);
extern bool export_metadata(std::string format, std::unique_ptr<TokenDocument>&
    token_doc, std::ostream& ofs, std::string dsnum, size_t
    initial_indent_length = 0);
extern bool export_to_datacite(std::string version, std::ostream& ofs, std::
    string dsnum, XMLDocument& xdoc, size_t indent_length);
extern bool export_to_dc_meta_tags(std::ostream& ofs, std::string dsnum, size_t
    indent_length);
extern bool export_to_dif(std::ostream& ofs, std::string dsnum, XMLDocument&
     xdoc, size_t indent_length);
extern bool export_to_fgdc(std::ostream& ofs, std::string dsnum, XMLDocument&
     xdoc, size_t indent_length);
extern bool export_to_iso(std::string version, std::unique_ptr<TokenDocument>&
     token_doc, std::ostream& ofs, std::string dsnum, XMLDocument& xdoc, size_t
     indent_length);
extern bool export_to_json_ld(std::ostream& ofs, std::string dsnum, XMLDocument&
     xdoc, size_t indent_length);
extern bool export_to_native(std::ostream& ofs, std::string dsnum, XMLDocument&
     xdoc, size_t indent_length);
extern bool export_to_oai_dc(std::ostream& ofs, std::string dsnum, XMLDocument&
     xdoc, size_t indent_length);
extern bool export_to_thredds(std::ostream& ofs, std::string ident, XMLDocument&
     xdoc, size_t indent_length);

} // end namespace metadataExport

#endif
