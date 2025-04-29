#ifndef MYLDAP_H
#define   MYLDAP_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <ldap.h>

namespace MyLDAP {

class Result;

class Server
{
public:
  Server() : ldap(nullptr),_connected(false) {}
  Server(std::string uri) : Server() { connect(uri); }
  operator bool() const { return _connected; }
  bool bind(std::string username,std::string password);
  void connect(std::string uri);
  std::string error() const;
  Result full_search(std::string base_dn,std::string filter,std::string attribute_list) const;

private:
  struct LDAP_DELETER {
    void operator()(LDAP *ldap) {
	ldap_destroy(ldap);
    }
  };
  std::unique_ptr<LDAP,LDAP_DELETER> ldap;
  bool _connected;
};

class Entry;

class Result
{
public:
  friend class Server;

  Result() : entries() {}
  Entry entry(size_t index) const;
  void print_entries(std::ostream& outs) const;
  size_t size() const { return entries.size(); }

private:
  std::vector<Entry> entries;
};

class Entry
{
public:
  friend class Server;

  Entry() : attributes() {}
  std::string attribute_value(std::string attribute_name) const;
  void print_attributes(std::ostream& outs) const;

private:
  std::unordered_map<std::string,std::string> attributes;
};

} // end namespace MyLDAP

#endif
