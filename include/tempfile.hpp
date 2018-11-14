// FILE: tempfile.h

#include <stdio.h>
#include <stdlib.h>
#include <string>

#ifndef TEMPFILE_H
#define  TEMPFILE_H

class TempFile {
public:
  TempFile() : tpath(),fp(nullptr),rm_file(true) {}
  TempFile(const TempFile& source) : TempFile() { *this=source; }
  TempFile(const std::string& directory,const std::string& extension = "") : TempFile() { open(directory,extension); }
  ~TempFile();
  TempFile& operator=(const TempFile& source);
  void close();
  std::string base_name() const { return tpath.substr(0,tpath.rfind(".")); }
  std::string extension() const;
  std::string name(const std::string& directory = "",const std::string& extension = "");
  bool open(const std::string& directory = "",const std::string& extension = "");
  void set_keep() { rm_file=false; }
  void set_remove() { rm_file=true; }
  void write(const std::string& s);
  void writeln(const std::string& s);

private:
  void fill_temp_path(const std::string& directory,const std::string& extension);

  std::string tpath;
  FILE *fp;
  bool rm_file;
};

class TempDir {
public:
  TempDir() : tpath(),rm_file(true) {}
//  TempDir(const std::string& directory,const std::string& extension = "");
  ~TempDir();
  std::string name(const std::string& directory = "",const std::string& extension = "");
  bool create(const std::string& directory = "",const std::string& extension = "");
  void set_keep() { rm_file=false; }

private:
  void fill_temp_path(const std::string& directory,const std::string& extension);

  std::string tpath;
  bool rm_file;
};

#endif
