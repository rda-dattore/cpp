#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <tempfile.hpp>
#include <strutils.hpp>

using std::cerr;
using std::endl;
using std::string;

int errno;

TempFile::~TempFile() {
  close();
  if (rm_file) {
    if (system(("rm -f " + tpath).c_str()) != 0) {
    } // suppress compiler warning
  }
  tpath = "";
  fp = nullptr;
}

TempFile& TempFile::operator=(const TempFile& source) {
  if (this == &source) {
    return *this;
  }
  tpath = source.tpath;
  *fp = *source.fp;
  rm_file = source.rm_file;
  return *this;
}

void TempFile::close() {
  if (fp != nullptr) {
    fclose(fp);
    fp = nullptr;
  }
}

string TempFile::extension() const {
  auto idx = tpath.rfind(".");
  if (idx == string::npos) {
    return "";
  } else {
    return tpath.substr(idx + 1);
  }
}

string TempFile::name(const string& directory, const string& extension) {
  if (tpath.empty()) {
    fill_temp_path(directory, extension);
  }
  return tpath;
}

bool TempFile::open(const string& directory, const string& extension) {
  fill_temp_path(directory, extension);
  fp = fopen(tpath.c_str(), "w");
  if (fp == nullptr) {
    return false;
  }

  // run chmod to override any umask that was used by mkdir
  chmod(tpath.c_str(), 0644);
  rm_file = true;
  return true;
}

void TempFile::write(const string& s) {
  if (fp != nullptr) {
    if (s.empty()) {
      fprintf(fp, "\n");
    } else {
      fwrite(s.c_str(), 1, s.length(), fp);
    }
  } else {
    cerr << "Error writing to tempfile - file not open for writing" << endl;
    exit(1);
  }
}

void TempFile::writeln(const string& s) {
  if (fp != nullptr) {
    if (!s.empty()) {
      fwrite(s.c_str(), 1, s.length(), fp);
    }
    fprintf(fp, "\n");
  } else {
    cerr << "Error writing to tempfile - file not open for writing" << endl;
    exit(1);
  }
}

void TempFile::fill_temp_path(const string& directory, const string&
     extension) {
  char tnam[32768];
  if (tmpnam(tnam) == nullptr) {
    cerr << "Error: tmpnam failed" << endl;
    exit(1);
  }
  auto tnam_s = string(tnam);
  auto idx = tnam_s.rfind("/");
  if (idx != string::npos) {
    tnam_s = tnam_s.substr(idx + 1);
  }
  if (!directory.empty()) {
    tpath = directory;
    if (tpath.back() != '/') {
      tpath += "/";
    }
  } else {
    tpath = "/tmp/";
  }
  tpath += tnam_s;
  if (!extension.empty()) {
    tpath += extension;
  }
}

TempDir::~TempDir() {
  if (rm_file && !tpath.empty()) {
    if (system(("rm -rf " + tpath).c_str()) != 0) {
    } // suppress compiler warning
  }
  tpath = "";
}

string TempDir::name(const string& directory, const string& extension) {
  if (tpath.empty()) {
    fill_temp_path(directory, extension);
  }
  return tpath;
}

bool TempDir::create(const string& directory, const string& extension) {
  fill_temp_path(directory, extension);
  if (mkdir(tpath.c_str(), 0755) < 0) {
    return false;
  }

  // run chmod to override any umask that was used by mkdir
  chmod(tpath.c_str(), 0755);
  rm_file = true;
  return true;
}

void TempDir::fill_temp_path(const string& directory, const string& extension) {
  if (!directory.empty()) {
    tpath = directory;
    if (tpath.back() != '/') {
      tpath += "/";
    }
  } else {
    tpath = "/tmp/";
  }
  char tnam[32768];
  if (tmpnam(tnam) != nullptr) {
    auto tnam_s = string(tnam);
    auto idx = tnam_s.rfind("/");
    if (idx != string::npos) {
      tnam_s = tnam_s.substr(idx + 1);
    }
    tpath += tnam_s;
  }
  if (!extension.empty()) {
    tpath += extension;
  }
}
