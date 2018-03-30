#include <iostream>
#include <fstream>
#include <string>
#include <utils.hpp>

namespace filelock {

std::string build_lock_name(const std::string& filename)
{
  auto lock_name=filename;
  auto idx=lock_name.rfind("/");
  if (idx >= 0) {
    lock_name=lock_name.substr(idx+1);
  }
/*
  n=lock_name.length();
  while (n > 0 && lock_name.charAt(n-1) != '/')
    n--;
  lock_name.insertAt(n,".lock.");
*/
  return "/tmp/.lock."+lock_name;
}

bool is_locked(const std::string& filename)
{
  std::ifstream ifs;
  ifs.open(build_lock_name(filename).c_str());
  if (ifs.is_open()) {
    ifs.close();
    return true;
  }
  else {
    return false;
  }
}

int lock(const std::string& filename,std::string contents)
{
  std::ifstream ifs;
  std::ofstream ofs;

  ifs.open(filename);
  if (!ifs.is_open()) {
    return -1;
  }
  ofs.open(build_lock_name(filename).c_str());
  if (contents.length() > 0) {
    ofs << contents << std::endl;
  }
  ofs.close();
  return 0;
}

int unlock(const std::string& filename)
{
  if (!is_locked(filename)) {
    return -1;
  }
  if (remove(build_lock_name(filename).c_str()) < 0) {
    return -1;
  }
  return 0;
}

bool open_lock_file(const std::string& filename,std::ifstream& ifs)
{
  ifs.open(build_lock_name(filename).c_str());
  if (!ifs.is_open()) {
    return false;
  }
  return true;
}

} // end namespace filelock
