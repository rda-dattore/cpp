#include <iostream>
#include <sys/stat.h>
#include <pthread.h>
#include <metadata_export.hpp>
#include <xml.hpp>
#include <bsort.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <metadata.hpp>
#include <metahelpers.hpp>
#include <gridutils.hpp>
#include <citation.hpp>
#include <hereDoc.hpp>
#include <tokendoc.hpp>
#include <myerror.hpp>

using std::string;
using std::stringstream;
using std::unique_ptr;
using strutils::replace_all;

namespace metadataExport {

extern "C" string bind_export_metadata(string format, string ident, size_t
    initial_indent_length) {
  unique_ptr<TokenDocument> u;
  stringstream ss;
  export_metadata(format, u, ss, ident, initial_indent_length);
  return ss.str();
}

bool export_metadata(string format, unique_ptr<TokenDocument>& token_doc, std::
    ostream& ofs, string ident, size_t initial_indent_length) {
  auto exported = false;
  TempDir temp_dir;
  temp_dir.create("/tmp");
  string ds_overview;
  ds_overview = unixutils::remote_web_file("https://gdex.ucar.edu/oai/?verb="
      "GetRecord&metadataPrefix=native&identifier=oai:edu.ucar.gdex:" + ident,
      temp_dir.name());
  XMLDocument xdoc(ds_overview);
  if (xdoc.is_open()) {
    replace_all(format, "-", "_");
    if (format == "oai_dc") {
      exported = export_to_oai_dc(ofs, ident, xdoc, initial_indent_length);
    } else if (format.find("datacite") == 0) {
      exported = export_to_datacite(format.substr(8), ofs, ident, xdoc,
          initial_indent_length);
    } else if (format == "dc_meta_tags") {
      exported = export_to_dc_meta_tags(ofs, ident, initial_indent_length);
    } else if (format == "dif") {
      exported = export_to_dif(ofs, ident, xdoc, initial_indent_length);
    } else if (format == "fgdc") {
      exported = export_to_fgdc(ofs, ident, xdoc, initial_indent_length);
    } else if (format.find("iso") == 0) {
      exported = export_to_iso(format.substr(3), token_doc, ofs, ident, xdoc,
          initial_indent_length);
    } else if (format == "json_ld") {
      exported = export_to_json_ld(ofs, ident, xdoc, initial_indent_length);
    } else if (format == "native") {
      exported = export_to_native(ofs, ident, xdoc, initial_indent_length);
    } else if (format == "thredds") {
      exported = export_to_thredds(ofs, ident, xdoc, initial_indent_length);
    }
    xdoc.close();
  } else {
    myerror = xdoc.parse_error();
  }
  return exported;
}

} // end namespace metadataExport
