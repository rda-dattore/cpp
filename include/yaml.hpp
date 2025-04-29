// FILE: yaml.h

#ifndef YAML_H
#define   YAML_H

#include <string>
#include <unordered_map>
#include <vector>

namespace YAML {

class Document;

class ComponentIterator;
class Component
{
public:
  enum class Type {_NULL,_SCALAR_INT,_SCALAR_STRING,_SCALAR_FLOAT,_SCALAR_BOOL,_MAPPING,_SEQUENCE};

  Component() : type_(Type::_NULL),storage(nullptr) {}
  Component(const Component& c) : Component() { *this=c; } 
  ~Component();
  Component& operator=(const Component& c);
  void fill(void *value);
  void set(Type type);
  std::string to_string() const;
  Type type() const { return type_; }
  void update(int i,std::string key = "");
  void update(const char *s,std::string key = "");
  void update(std::string s,std::string key = "");
  void update(float f,std::string key = "");
  void update(bool b,std::string key = "");
  void update(const Component& c,std::string key = "");
  const Component& operator[](const char* key) const;
  const Component& operator[](std::string key) const;
  const Component& operator[](int index) const;
  const Component& operator[](size_t index) const;
  bool operator>(long long l) const;
  bool operator<(long long l) const;
  bool operator==(long long l) const;
  bool operator!=(long long l) const { return !(*this == l); }
  bool operator>=(long long l) const;
  bool operator<=(long long l) const;
  bool operator==(std::string s) const;
  bool operator!=(std::string s) const { return !(*this == s); }
  bool operator==(const char *s) const;
  bool operator!=(const char *s) const { return !(*this == s); }
  friend std::ostream& operator<<(std::ostream& o,const Component& c);
  friend class ComponentIterator;
  friend class Document;

// range-based support
  ComponentIterator begin() const;
  ComponentIterator end() const;

private:
  void clear();

  Type type_;
  void *storage;
};

class ComponentIterator
{
public:
  ComponentIterator(const Component& c,bool is_end) : _c(c),_map_it(),_seq_it(),_is_end(is_end) {}
  ComponentIterator(const Component& c,bool is_end,std::unordered_map<std::string,Component>::iterator map_it) : _c(c),_map_it(map_it),_seq_it(),_is_end(is_end) {}
  ComponentIterator(const Component& c,bool is_end,std::vector<Component>::iterator seq_it) : _c(c),_map_it(),_seq_it(seq_it),_is_end(is_end) {}
  bool operator!=(const ComponentIterator& source) { return(_is_end != source._is_end); }
  const ComponentIterator& operator++();
  std::pair<std::string,const Component&> operator*();

private:
  const Component &_c;
  std::unordered_map<std::string,Component>::iterator _map_it;
  std::vector<Component>::iterator _seq_it;
  bool _is_end;

};

class Document
{
public:
  Document() : name_(),parsed(false),parse_error_(),root_component() {}
  Document(std::string filename) : Document() { open(filename); }
  operator bool() { return parsed; }
  bool open(const std::string& filename);
  void close();
  bool is_open() const { return parsed; }
  std::string name() const { return name_; }
  std::string parse_error() const { return parse_error_; }
  void show_tree();
  const Component& operator[](const char* key) const;
  const Component& operator[](std::string key) const;
  const Component& operator[](int index) const;
  const Component& operator[](size_t index) const;

private:
  void parse(std::string& yaml_content);

  std::string name_;
  bool parsed;
  std::string parse_error_;
  Component root_component;
};

} // end namespace YAML

#endif
