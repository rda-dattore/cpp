#include <fstream>
#include <ascent.hpp>
#include <strutils.hpp>
#include <myerror.hpp>

using std::copy;
using std::stoi;
using std::stof;
using std::stoll;
using std::string;
using strutils::csv_split;
using strutils::replace_all;
using strutils::split;
using strutils::substitute;
using strutils::trim;

void InputASCENTObservationStream::close() {
}

int InputASCENTObservationStream::ignore() {
return -1;
}

bool InputASCENTObservationStream::open(string filename) {
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
  string lat, lon, elev;
  ifs.getline(line, 32768);
  while (!ifs.eof()) {
    if (line[0] == 'C' && string(line).find("Coordinates: ") == 0) {
      auto cparts = split(substitute(substitute(string(line).substr(13),
          "(", ""), ")", ""), ",");
      lon = cparts[0];
      trim(lon);
      lat = cparts[1];
      trim(lat);
    } else if (line[0] == 'E' && string(line).find("Elevation: ") == 0) {
      auto eparts = split(string(line).substr(11));
      elev = eparts[0];
      trim(elev);
    }
    ifs.getline(line, 32768);
  }
  ifs.close();
  header += "," + lat + "," + lon + "," + elev + ",";
  header_len = header.length();
  return true;
}

int InputASCENTObservationStream::peek() {
return -1;
}

int InputASCENTObservationStream::read(unsigned char *buffer, size_t
    buffer_length) {
  auto *cbuf = reinterpret_cast<char *>(buffer);
  copy(header.begin(), header.end(), &cbuf[0]);
  fs.getline(&cbuf[header_len], buffer_length-header_len);
  if (fs.eof()) {
    return -1;
  }
  return header_len + fs.gcount();
}

void InputASCENTObservationStream::rewind() {
}

void ASCENTObservation::fill(const unsigned char *stream_buffer, bool
    fill_header_only) {
  auto raw_obs = string(reinterpret_cast<const char *>(stream_buffer));
  auto parts = csv_split(raw_obs);
  instrument_ = parts[0];
  location_.latitude = stof(parts[1]);
  location_.longitude = stof(parts[2]);
  location_.elevation = stoi(parts[3]);
  location_.ID = parts[5];
  replace_all(parts[6], "-", "");
  replace_all(parts[6], ":", "");
  replace_all(parts[6], " ", "");
  replace_all(parts[6], R"(")", "");
  date_time_.set(stoll(parts[6]));
  if (instrument_ == "ACSM") {
    qc_outcome_ = parts[31];
  } else if (instrument_ == "AE33") {
    qc_outcome_ = parts[51];
  } else if (instrument_ == "SMPS") {
    qc_outcome_ = parts[8];
  } else if (instrument_ == "Xact") {
    qc_outcome_ = parts[18];
  }
}
