#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <tempfile.hpp>
#include <strutils.hpp>

int errno;

TempFile::~TempFile()
{
  close();
  if (rm_file) {
    system(("rm -f "+tpath).c_str());
  }
  tpath="";
  fp=nullptr;
}

TempFile& TempFile::operator=(const TempFile& source)
{
  if (this == &source) {
    return *this;
  }
  tpath=source.tpath;
  *fp=*source.fp;
  rm_file=source.rm_file;
  return *this;
}

void TempFile::close()
{
  if (fp != nullptr) {
    fclose(fp);
    fp=nullptr;
  }
}

std::string TempFile::extension() const {
  size_t index;
  if ( (index=tpath.rfind(".")) == std::string::npos) {
    return "";
  }
  else {
    return tpath.substr(index+1);
  }
}

std::string TempFile::name(const std::string& directory,const std::string& extension)
{
  if (tpath.empty()) {
    fill_temp_path(directory,extension);
  }
  return tpath;
}

bool TempFile::open(const std::string& directory,const std::string& extension)
{
  fill_temp_path(directory,extension);
  if ( (fp=fopen(tpath.c_str(),"w")) == NULL) {
    return false;
  }
  rm_file=true;
  return true;
}

void TempFile::write(const std::string& s)
{
  if (fp != NULL) {
    if (s.empty()) {
	fprintf(fp,"\n");
    }
    else {
	fwrite(s.c_str(),1,s.length(),fp);
    }
  }
  else {
    std::cerr << "Error writing to tempfile - file not open for writing" << std::endl;
    exit(1);
  }
}

void TempFile::writeln(const std::string& s)
{
  if (fp != NULL) {
    if (!s.empty()) {
	fwrite(s.c_str(),1,s.length(),fp);
    }
    fprintf(fp,"\n");
  }
  else {
    std::cerr << "Error writing to tempfile - file not open for writing" << std::endl;
    exit(1);
  }
}

void TempFile::fill_temp_path(const std::string& directory,const std::string& extension)
{
  char tnam[32768];
  if (tmpnam(tnam) == nullptr) {
    std::cerr << "Error: tmpnam failed" << std::endl;
    exit(1);
  }
  auto tnam_s=std::string(tnam);
  auto idx=tnam_s.rfind("/");
  if (idx != std::string::npos) {
    tnam_s=tnam_s.substr(idx+1);
  }
  if (!directory.empty()) {
    tpath=directory;
    if (!strutils::has_ending(tpath,"/")) {
	tpath+="/";
    }
  }
  else {
    tpath="/tmp/";
  }
  tpath+=tnam_s;
  if (!extension.empty()) {
    tpath+=extension;
  }
}

TempDir::~TempDir()
{
  if (rm_file && !tpath.empty()) {
    system(("rm -rf "+tpath).c_str());
  }
  tpath="";
}

std::string TempDir::name(const std::string& directory,const std::string& extension)
{
  if (tpath.empty()) {
    fill_temp_path(directory,extension);
  }
  return tpath;
}

bool TempDir::create(const std::string& directory,const std::string& extension)
{
  fill_temp_path(directory,extension);
  if (mkdir(tpath.c_str(),0755) < 0) {
    return false;
  }
  rm_file=true;
  return true;
}

void TempDir::fill_temp_path(const std::string& directory,const std::string& extension)
{
  char tnam[32768];
  tmpnam(tnam);
  auto tnam_s=std::string(tnam);
  auto idx=tnam_s.rfind("/");
  if (idx != std::string::npos) {
    tnam_s=tnam_s.substr(idx+1);
  }
  if (!directory.empty()) {
    tpath=directory;
    if (!strutils::has_ending(tpath,"/")) {
	tpath+="/";
    }
  }
  else {
    tpath="/tmp/";
  }
  tpath+=tnam_s;
  if (!extension.empty()) {
    tpath+=extension;
  }
}
