#include <fstream>
#include <metadata_export.hpp>
#include <xml.hpp>
#include <myerror.hpp>

using std::string;

namespace metadataExport {

extern bool export_to_datacite_3(std::ostream& ofs, string dsnum, XMLDocument&
    xdoc, size_t indent_length);
extern bool export_to_datacite_4(std::ostream& ofs, string dsnum, XMLDocument&
    xdoc, size_t indent_length);

bool export_to_datacite(string version, std::ostream& ofs, string dsnum,
    XMLDocument& xdoc, size_t indent_length) {
  if (version == "3") {
    return export_to_datacite_3(ofs, dsnum, xdoc, indent_length);
  } else if (version == "4") {
    return export_to_datacite_4(ofs, dsnum, xdoc, indent_length);
  } else {
    myerror = "unrecognized DataCite version number '" + version + "'";
    return false;
  }
}

} // end namespace metadataExport
