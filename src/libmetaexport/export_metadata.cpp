#include <iostream>
#include <regex>
#include <sys/stat.h>
#include <pthread.h>
#include <metadata_export.hpp>
#include <xml.hpp>
#include <MySQL.hpp>
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

namespace metadataExport {

bool export_metadata(std::string format,std::unique_ptr<TokenDocument>& token_doc,std::ostream& ofs,std::string ident,size_t initial_indent_length)
{
  auto exported=false;
  TempDir temp_dir;
  temp_dir.create("/tmp");
  if (std::regex_search(ident,std::regex("^[0-9][0-9][0-9]\\.[0-9]$"))) {
    std::string ds_overview;
    struct stat buf;
    if (stat((metautils::directives.server_root+"/web/datasets/ds"+ident+"/metadata/dsOverview.xml").c_str(),&buf) == 0) {
	ds_overview=metautils::directives.server_root+"/web/datasets/ds"+ident+"/metadata/dsOverview.xml";
    }
    else {
	ds_overview=unixutils::remote_web_file("https://rda.ucar.edu/datasets/ds"+ident+"/metadata/dsOverview.xml",temp_dir.name());
    }
    XMLDocument xdoc(ds_overview);
    if (xdoc.is_open()) {
	if (format == "oai_dc") {
	  exported=export_to_oai_dc(ofs,ident,xdoc,initial_indent_length);
	}
	else if (format == "datacite") {
	  exported=export_to_datacite(ofs,ident,xdoc,initial_indent_length);
	}
	else if (format == "dc-meta-tags") {
	  exported=export_to_dc_meta_tags(ofs,ident,xdoc,initial_indent_length);
	}
	else if (format == "dif") {
	  exported=export_to_dif(ofs,ident,xdoc,initial_indent_length);
	}
	else if (format == "fgdc") {
	  exported=export_to_fgdc(ofs,ident,xdoc,initial_indent_length);
	}
	else if (format == "iso19139") {
	  exported=export_to_iso19139(token_doc,ofs,ident,xdoc,initial_indent_length);
	}
	else if (format == "iso19115-3") {
	  exported=export_to_iso19115_3(token_doc,ofs,ident,xdoc,initial_indent_length);
	}
	else if (format == "json-ld") {
	  exported=export_to_json_ld(ofs,ident,xdoc,initial_indent_length);
	}
	else if (format == "native") {
	  exported=export_to_native(ofs,ident,xdoc,initial_indent_length);
	}
	else if (format == "thredds") {
	  exported=export_to_thredds(ofs,ident,xdoc,initial_indent_length);
	}
	xdoc.close();
    }
  }
  return exported;
}

} // end namespace metadataExport
