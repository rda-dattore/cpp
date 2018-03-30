#include <string>
#include <xml.hpp>

xmlstream::~xmlstream()
{
  if (ofs.is_open())
    close();
}

bool xmlstream::open(const std::string& filename)
{
  ofs.open(filename.c_str());
  if (ofs.is_open()) {
    ofs << "<?xml version=\"1.0\" ?>" << std::endl;
    return true;
  }
  else
    return false;
}

xmlstream::EndTag end_tag();
bool xmlstream::close()
{
  if (ofs.is_open()) {
    while (!tags.empty())
	*this << end_tag();
    ofs.close();
    return true;
  }
  else
    return false;
}

xmlstream& xmlstream::operator<<(const Attribute& attribute)
{
  if (tags.size() > 0 && !tags.top().is_closed) {
    if (attribute.indent)
	ofs << "\n" << std::string((tags.size()-1)*2+tags.top().name.length()+2,' ');
    else
	ofs << " ";
    ofs << attribute.name << "=\"" << attribute.value << "\"";
  }
  return *this;
}

xmlstream& xmlstream::operator<<(const BeginTag& begin_tag)
{
  StackTag st;

  if (tags.size() > 0) {
    if (tags.top().has_content)
	*this << end_tag();
    else if (!tags.top().is_closed) {
	ofs << ">" << std::endl;
	tags.top().is_closed=true;
    }
  }
  st.name=begin_tag.name;
  st.is_closed=false;
  st.has_content=false;
  tags.push(st);
  ofs << std::string((tags.size()-1)*2,' ') << "<" << begin_tag.name;
  return *this;
}

xmlstream& xmlstream::operator<<(const EndTag& end_tag)
{
  if (tags.size() > 0) {
    if (!tags.top().is_closed)
	ofs << " />" << std::endl;
    else {
	if (!tags.top().has_content)
	  ofs << std::string((tags.size()-1)*2,' ');
	ofs << "</" << tags.top().name << ">" << std::endl;
    }
    tags.pop();
  }
  return *this;
}

xmlstream& xmlstream::operator<<(const Content& content)
{
  if (tags.size() > 0 && !tags.top().is_closed) {
    ofs << ">";
    tags.top().is_closed=true;
    tags.top().has_content=true;
  }
  return *this;
}

xmlstream::Attribute attribute(const std::string& name,const std::string& value,bool indent)
{
  xmlstream::Attribute attr;

  attr.name=name;
  attr.value=value;
  attr.indent=indent;
  return attr;
}

xmlstream::BeginTag begin_tag(const std::string& tagname)
{
  xmlstream::BeginTag begin_tag;

  begin_tag.name=tagname;
  return begin_tag;
}

xmlstream::EndTag end_tag()
{
  xmlstream::EndTag end_tag;

  return end_tag;
}

xmlstream::Content content()
{
  xmlstream::Content content;

  return content;
}
