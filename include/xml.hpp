// FILE: xml.h

#ifndef XML_H
#define   XML_H

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <stack>
#include <buffer.hpp>

class XMLElement;
class XMLSnippet;
class XMLDocument;

class XMLAttribute
{
public:
  XMLAttribute() : name(),value() {}

  std::string name;
  std::string value;
};

struct XMLElementAddress {
  XMLElementAddress() : p(nullptr) {}

  XMLElement *p;
};
class XMLElement
{
public:
  XMLElement() : name_(),attr_list(),element_address_list(),content_s() {}
  XMLElement(const XMLElement& e) : XMLElement() { *this=e; }
  XMLElement& operator=(const XMLElement& e);
  size_t attribute_count() const { return attr_list.size(); }
  const std::list<XMLAttribute>& attribute_list() const { return attr_list; }
  std::string attribute_value(std::string attribute_name) const;
  std::string content() const { return content_s; }
  XMLElement element(const std::string& xpath) const;
  const std::list<XMLElementAddress>& element_addresses() const { return element_address_list; }
  size_t element_count() const { return element_address_list.size(); }
  std::list<XMLElement> element_list(const std::string& xpath) const;
  std::string name() const { return name_; }
  std::string to_string() const;

  friend class XMLSnippet;
  friend class XMLDocument;
  friend void check(const XMLElement& root,const std::vector<std::string>& comps,size_t this_comp,std::list<XMLElement>& element_list);

private:
  std::string name_;
  std::list<XMLAttribute> attr_list;
  std::list<XMLElementAddress> element_address_list;
  std::string content_s;
};

class XMLSnippet
{
public:
  XMLSnippet() : root_element(),parsed(false),element_addresses(),parse_error_(),untag_buffer(),untag_buffer_off(0) {}
  XMLSnippet(std::string string) : XMLSnippet() { fill(string); }
  virtual ~XMLSnippet() {}
  operator bool() { return parsed; }
  void fill(std::string string);
  XMLElement element(const std::string& xpath);
  std::list<XMLElement> element_list(const std::string& xpath);
  std::string parse_error() const { return parse_error_; }
  void print_tree(std::ostream& outs);
  void show_tree();
  std::string untagged();

  friend void check(XMLElement& root,const std::vector<std::string>& comps,size_t this_comp,std::list<XMLElement>& element_list);

protected:
  void process_new_tag_name(const std::string& xml_element,int tagname_start,int off,std::list<std::string>& tagnames,XMLElementAddress& eaddr,std::list<XMLElement *>& parent_elements);
  void parse(std::string& snippet);
  void printElement(std::ostream& outs,XMLElement& element,bool isRoot,size_t indent);

  XMLElement root_element;
  bool parsed;
  std::vector<XMLElementAddress> element_addresses;
  std::string parse_error_;
  Buffer untag_buffer;
  int untag_buffer_off;
};

class XMLDocument : public XMLSnippet
{
public:
  XMLDocument() : name_(),version_() {}
  XMLDocument(std::string filename) : XMLDocument() { open(filename); }
  operator bool() { return parsed; }
  bool open(const std::string& filename);
  void close();
  bool is_open() const { return parsed; }
  std::string name() const { return name_; }
  std::string version() const { return version_; }
  void show_tree();

private:
  std::string name_;
  std::string version_;
};

extern void showXMLSubTree(XMLElement& root,size_t indent);

class xmlstream
{
public:
  struct StackTag {
    StackTag() : name(),is_closed(false),has_content(false) {}

    std::string name;
    bool is_closed,has_content;
  };
  struct Attribute {
    Attribute() : name(),value(),indent(false) {}

    std::string name,value;
    bool indent;
  };
  struct BeginTag {
    BeginTag() : name() {}

    std::string name;
  };
  struct EndTag {
  };
  struct Content {
  };

  xmlstream() : ofs(),tags() {}
  xmlstream(const std::string& filename) : xmlstream() { open(filename); }
  ~xmlstream();
  bool open(const std::string& filename);
  bool close();
  template <typename Output> xmlstream& operator<<(const Output& output);
  xmlstream& operator<<(const Attribute& attribute);
  xmlstream& operator<<(const BeginTag& begin_tag);
  xmlstream& operator<<(const EndTag& end_tag);
  xmlstream& operator<<(const Content& content);

private:
  std::ofstream ofs;
  std::stack<StackTag> tags;
};

template <typename Output>
xmlstream& xmlstream::operator<<(const Output& output)
{
  if (tags.size() > 0) {
    ofs << output;
    tags.top().has_content=true;
  }
  return *this;
}

extern xmlstream::Attribute attribute(const std::string& name,const std::string& value,bool indent = false);
extern xmlstream::BeginTag begin_tag(const std::string& tagname);
extern xmlstream::EndTag end_tag();
extern xmlstream::Content content();

#endif
