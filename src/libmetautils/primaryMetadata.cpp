#include <metadata.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>
#include <PostgreSQL.hpp>

using namespace PostgreSQL;
using std::string;
using strutils::strget;
using unixutils::exists_on_server;

namespace metautils {

namespace primaryMetadata {

bool expand_file(string dirname, string filename, string *file_format) {
  if (args.data_format.find("grib") != 0) {

    // check to see if the file is a tar file
    auto fp = fopen64(filename.c_str(), "r");
    unsigned char buffer[512];
    if (fread(buffer, 1, 512, fp) != 512) {
      return false;
    }
    fclose(fp);
    auto cksum_len = 0;
    for (size_t n = 148; n < 156; ++n) {
      if (((reinterpret_cast<
          char *>(buffer))[n] >= '0' && (reinterpret_cast<char *>(buffer))[n] <=
          '9') || (reinterpret_cast<char *>(buffer))[n] == ' ') {
        ++cksum_len;
      } else {
        n = 156;
      }
    }
    int cksum;
    strget(&(reinterpret_cast<char *>(buffer))[148], cksum, cksum_len);
    auto sum = 256;
    for (size_t n = 0; n < 148; ++n) {
      sum += static_cast<int>(buffer[n]);
    }
    for (size_t n = 156; n < 512; ++n) {
      sum += static_cast<int>(buffer[n]);
    }
    if (sum == xtox::otoi(cksum)) {
      unixutils::untar(dirname, filename);
      if (file_format != nullptr) {
        if (!file_format->empty()) {
          (*file_format) += ".";
        }
        (*file_format) += "TAR";
      }
      return true;
    }
  }

  // check to see if the file is gzipped
  auto fp = fopen64(filename.c_str(), "r");
  unsigned char buffer[4];
  if (fread(buffer, 1, 4, fp) != 4) {
    return false;
  }
  fclose(fp);
  int magic;
  bits::get(buffer, magic, 0, 32);
  if (magic == 0x1f8b0808) {
    if (system(("mv " + filename + " " + filename + ".gz; gunzip -f " +
        filename).c_str()) != 0) {
      return false;
    }
    if (file_format != nullptr) {
      if (!file_format->empty()) {
        (*file_format) += ".";
      }
      (*file_format) += "GZ";
    }
    return true;
  }
  return false;
}

} // end namespace primaryMetadata

} // end namespace metautils
