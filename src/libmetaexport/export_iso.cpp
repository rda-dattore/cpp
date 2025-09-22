#include <fstream>
#include <metadata_export.hpp>
#include <xml.hpp>
#include <myerror.hpp>

using std::string;
using std::unique_ptr;

namespace metadataExport {

extern bool export_to_iso19115_3(unique_ptr<TokenDocument>& token_doc, std::
    ostream& ofs, string dsnum, XMLDocument& xdoc, size_t indent_length);
extern bool export_to_iso19139(unique_ptr<TokenDocument>& token_doc, std::
    ostream& ofs, string dsnum, XMLDocument& xdoc, size_t indent_length);

bool export_to_iso(string version, unique_ptr<TokenDocument>& token_doc, std::
    ostream& ofs, string dsnum, XMLDocument& xdoc, size_t indent_length) {
  if (version == "19139") {
    return export_to_iso19139(token_doc, ofs, dsnum, xdoc, indent_length);
  } else if (version == "19115_3") {
    return export_to_iso19115_3(token_doc, ofs, dsnum, xdoc, indent_length);
  } else {
    myerror = "unrecognized ISO version number '" + version + "'";
    return false;
  }
}

} // end namespace metadataExport
