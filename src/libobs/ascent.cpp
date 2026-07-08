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
  if (!idstream::open(filename)) {
    myerror = "Unable to open data file.";
    return false;
  }
  char line[32768];
  fs.getline(line, 32768);
  auto parts = split(line, ",");
  if (parts[0] != R"("site_number")" && parts[0] != R"("sample_analysis_id")") {
    fs.close();
    myerror = "Unrecognized CSV header.";
    return false;
  }
  if (parts.size() == 77 && parts[7] == R"("chl_ug_m3")") {
    header = "ACSM:L1b";
  } else if (parts.size() == 30 && parts[4] == R"("organics_STP_ug_m3")") {
    header = "ACSM:L2";
  } else if (parts.size() == 85 && parts[37] == R"("bc_1_STP_ng_m3")") {
    header = "AE33:L1b";
  } else if (parts.size() == 50 && parts[4] == R"("bc_1_STP_ng_m3")") {
    header = "AE33:L2";
  } else if (parts.size() == 52 && parts[42] == R"("stp_factor")") {
    header = "SMPS:L1b";
  } else if (parts.size() == 18 && parts[3] == R"("stp_factor")") {
    header = "SMPS:L2";
  } else if (parts.size() == 34 && parts[5] == R"("element")") {
    header = "Xact:L1b";
  } else if (parts.size() == 17 && parts[4] == R"("element")") {
    header = "Xact:L2";
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
    return bfstream::eof;
  }
  ++num_read;
  return header_len + fs.gcount();
}

void InputASCENTObservationStream::rewind() {
}

void ASCENTObservation::fill(const unsigned char *stream_buffer, bool
    fill_header_only) {
  auto raw_obs = string(reinterpret_cast<const char *>(stream_buffer));
  auto parts = csv_split(raw_obs);
  auto p0 = split(parts[0], ":");
  instrument_ = p0.front();
  processing_level_ = p0.back();
  location_.latitude = stof(parts[1]);
  location_.longitude = stof(parts[2]);
  location_.elevation = stoi(parts[3]);
  auto id_off = 5;
  auto date_off = 6;
  if (instrument_ == "ACSM" && processing_level_ == "L1b") {
    id_off = 10;
    date_off = 7;
  }
  location_.ID = parts[id_off];
  replace_all(location_.ID, R"(")", "");
  replace_all(parts[date_off], "-", "");
  replace_all(parts[date_off], ":", "");
  replace_all(parts[date_off], " ", "");
  replace_all(parts[date_off], R"(")", "");
  date_time_.set(stoll(parts[date_off]));
  if (instrument_ == "ACSM") {
    if (processing_level_ == "L1b") {
      qc_outcome_ = parts[79];
    } else if (processing_level_ == "L2") {
      qc_outcome_ = parts[31];
    }
  } else if (instrument_ == "AE33") {
    if (processing_level_ == "L1b") {
      qc_outcome_ = parts[87];
    } else if (processing_level_ == "L2") {
      qc_outcome_ = parts[51];
    }
  } else if (instrument_ == "SMPS") {
    if (processing_level_ == "L1b") {
      qc_outcome_ = parts[53];
    } else if (processing_level_ == "L2") {
      qc_outcome_ = parts[8];
    }
  } else if (instrument_ == "Xact") {
    if (processing_level_ == "L1b") {
      qc_outcome_ = parts[35];
    } else if (processing_level_ == "L2") {
      qc_outcome_ = parts[18];
    }
  }
}
