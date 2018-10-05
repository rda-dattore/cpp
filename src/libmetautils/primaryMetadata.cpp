#include <metadata.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

namespace metautils {

namespace primaryMetadata {

bool expand_file(std::string dirname,std::string filename,std::string *file_format)
{
  if (!std::regex_search(args.data_format,std::regex("^grib"))) {
// check to see if the file is a tar file
    auto fp=fopen64(filename.c_str(),"r");
    unsigned char buffer[512];
    fread(buffer,1,512,fp);
    fclose(fp);
    auto cksum_len=0;
    for (size_t n=148; n < 156; ++n) {
	if (((reinterpret_cast<char *>(buffer))[n] >= '0' && (reinterpret_cast<char *>(buffer))[n] <= '9') || (reinterpret_cast<char *>(buffer))[n] == ' ') {
	  ++cksum_len;
	}
	else {
	  n=156;
	}
    }
    int cksum;
    strutils::strget(&(reinterpret_cast<char *>(buffer))[148],cksum,cksum_len);
    auto sum=256;
    for (size_t n=0; n < 148; ++n) {
	sum+=static_cast<int>(buffer[n]);
    }
    for (size_t n=156; n < 512; ++n) {
	sum+=static_cast<int>(buffer[n]);
    }
    if (sum == xtox::otoi(cksum)) {
	unixutils::untar(dirname,filename);
	if (file_format != nullptr) {
	  if (!file_format->empty()) {
	    (*file_format)+=".";
	  }
	  (*file_format)+="TAR";
	}
	return true;
    }
  }
// check to see if the file is gzipped
  auto fp=fopen64(filename.c_str(),"r");
  unsigned char buffer[4];
  fread(buffer,1,4,fp);
  fclose(fp);
  int magic;
  bits::get(buffer,magic,0,32);
  if (magic == 0x1f8b0808) {
    system(("mv "+filename+" "+filename+".gz; gunzip -f "+filename).c_str());
    if (file_format != nullptr) {
	if (!file_format->empty()) {
	  (*file_format)+=".";
	}
	(*file_format)+="GZ";
    }
    return true;
  }
  return false;
}

void match_web_file_to_hpss_primary(std::string url,std::string& metadata_file)
{
  MySQL::Server server(directives.database_server,directives.rdadb_username,directives.rdadb_password,"dssdb");
  MySQL::Query query("select m.mssfile from wfile as w left join mssfile as m on m.mssid = w.mssid where w.dsid = 'ds"+args.dsnum+"' and w.wfile = '"+metautils::relative_web_filename(url)+"' and w.property = 'A' and w.type = 'D' and m.data_size = w.data_size");
  if (query.submit(server) < 0) {
    std::cerr << query.error() << std::endl;
    exit(1);
  }
  MySQL::Row row;
  if (query.fetch_row(row) && !row[0].empty()) {
    metadata_file=row[0];
    strutils::replace_all(metadata_file,"/FS/DSS/","");
    strutils::replace_all(metadata_file,"/DSS/","");
    strutils::replace_all(metadata_file,"/","%");
    if (unixutils::exists_on_server(directives.web_server,"/data/web/datasets/ds"+args.dsnum+"/metadata/fmd/"+metadata_file+".GrML",directives.rdadata_home)) {
	metadata_file+=".GrML";
    }
    else if (unixutils::exists_on_server(directives.web_server,"/data/web/datasets/ds"+args.dsnum+"/metadata/fmd/"+metadata_file+".ObML",directives.rdadata_home)) {
	metadata_file+=".ObML";
    }
    else if (unixutils::exists_on_server(directives.web_server,"/data/web/datasets/ds"+args.dsnum+"/metadata/fmd/"+metadata_file+".SatML",directives.rdadata_home)) {
	metadata_file+=".SatML";
    }
    else {
	metadata_file=row[0];
    }
  }
  else {
    metadata_file="";
  }
  server.disconnect();
}

} // end namespace primaryMetadata

} // end namespace metautils
