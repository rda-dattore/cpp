#include <fstream>
#include <ascent.hpp>
#include <strutils.hpp>
#include <myerror.hpp>

using std::copy;
using std::string;
using strutils::split;
using strutils::substitute;
using strutils::trim;

void InputASCENTStream::close() {
}

int InputASCENTStream::ignore() {
return -1;
}

bool InputASCENTStream::open(string filename) {
  if (!ibfstream::open(filename)) {
    myerror = "Unable to open data file.";
    return false;
  }
  char line[32768];
  fs.getline(line, 32768);
  auto parts = split(line, ",");
  if (parts[0] != R"("site_number")") {
    fs.close();
    myerror = "Unrecognized CSV header.";
    return false;
  }
  if (parts[4] == R"("bc_1_STP_ng_m3")") {
    header = "AE33";
  }
  if (parts[4] == R"("organics_ug_STP_m3")") {
    header = "ACSM";
  }
  if (parts[4] == R"("element")") {
    header = "Xact";
  }
  if (parts[3] == R"("stp_factor")") {
    header = "SMPS";
  }
  if (header.empty()) {
    fs.close();
    myerror = "Unrecognized instrument.";
    return false;
  }
  std::ifstream ifs(substitute(filename, "csv", "txt"));
  if (!ifs.is_open()) {
    fs.close();
    myerror = "Unable to open metadata file.";
    return false;
  }
  ifs.getline(line, 32768);
  while (!ifs.eof()) {
    if (line[0] == 'C' && string(line).find("Coordinates: ") == 0) {
      auto cparts = split(substitute(substitute(string(line).substr(13),
          "(", ""), ")", ""), ",");
      trim(cparts[0]);
      trim(cparts[1]);
      header += "," + cparts[0] + "," + cparts[1] + ",";
      header_len = header.length();
      break;
    }
    ifs.getline(line, 32768);
  }
  ifs.close();
  return true;
}

int InputASCENTStream::peek() {
return -1;
}

int InputASCENTStream::read(unsigned char *buffer, size_t buffer_length) {
  auto *cbuf = reinterpret_cast<char *>(buffer);
  copy(header.begin(), header.end(), &cbuf[0]);
  fs.getline(&cbuf[header_len], buffer_length-header_len);
  if (fs.eof()) {
    return -1;
  }
  return header_len + fs.gcount();
}

void InputASCENTStream::rewind() {
}

void ASCENTObservation::fill(const unsigned char *stream_buffer, bool
    fill_header_only) {
}
