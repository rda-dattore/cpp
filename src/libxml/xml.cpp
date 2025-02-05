#include <iostream>
#include <fstream>
#include <memory>
#include <list>
#include <regex>
#include <zlib.h>
#include <xml.hpp>
#include <strutils.hpp>

using std::cout;
using std::endl;
using std::string;
using strutils::replace_all;
using strutils::split;

XMLElement& XMLElement::operator=(const XMLElement& e) {
  if (this == &e) {
    return *this;
  }
  name_=e.name_;
  element_address_list.clear();
  element_address_list=e.element_address_list;
  attr_list.clear();
  attr_list=e.attr_list;
  content_s=e.content_s;
  return *this;
}

string XMLElement::attribute_value(string attribute_name) const {
  for (const auto& attr : attr_list) {
    if (attr.name == attribute_name) {
      return attr.value;
    }
  }
  return "";
}

XMLElement XMLElement::element(const string& xpath) const {
  auto elist=element_list(xpath);
  if (elist.size() > 0) {
    return elist.front();
  } else {
    return XMLElement();
  }
}

std::list<XMLElement> XMLElement::element_list(const string& xpath) const {
  string s=this->name_+"/"+xpath;
  replace_all(s,"\\/","$SLASH$");
  auto sp=split(s,"/");
  std::list<XMLElement> element_list;
  check(*this,sp,0,element_list);
  return element_list;
}

string XMLElement::to_string() const {
  string s;
  if (!name_.empty()) {
    s+="<"+name_;
    for (const auto& attr : attr_list) {
      s+=" "+attr.name+"=\""+attr.value+"\"";
    }
    if (element_address_list.size() > 0 || !content_s.empty()) {
      s+=">";
      if (!content_s.empty()) {
        s+=content_s;
      } else {
        for (const auto& addr : element_address_list) {
          s+=addr.p->to_string();
        }
      }
      s+="</"+name_+">";
    } else {
      s+=" />";
    }
  }
  return s;
}

void XMLSnippet::fill(string string) {
  root_element_.name_="";
  root_element_.element_address_list.clear();
  root_element_.attr_list.clear();
  root_element_.content_s="";
  for (size_t n=0; n < element_addresses.size(); ++n) {
    delete element_addresses[n].p;
  }
  element_addresses.clear();
  parse(string);
  if (parse_error_.empty()) {
    parsed=true;
  }
}

XMLElement XMLSnippet::element(const string& xpath) {
  if (parsed) {
    auto elist=element_list(xpath);
    if (elist.size() > 0) {
      return elist.front();
    }
  }
  return XMLElement();
}

std::list<XMLElement> XMLSnippet::element_list(const string& xpath) {
  std::list<XMLElement> element_list;
  if (parsed) {
    auto s=xpath;
    replace_all(s,"\\/","$SLASH$");
    auto sp=split(s,"/");
    check(root_element_,sp,0,element_list);
  }
  return element_list;
}

void XMLSnippet::print_tree(std::ostream& outs) {
  outs << "<?xml version=\"1.0\" ?>" << endl;
  printElement(outs,root_element_,true,0);
}

void XMLSnippet::show_tree() {
  cout << "Root: " << root_element_.name_ << endl;
  showXMLSubTree(root_element_,0);
}

string XMLSnippet::untagged() {
  return string(reinterpret_cast<char *>(&untag_buffer[0]),untag_buffer_off);
}

void XMLSnippet::process_new_tag_name(const string& xml_element,int tagname_start,int off,std::list<string>& tagnames,XMLElementAddress& eaddr,std::list<XMLElement *>& parent_elements) {
  auto s=xml_element.substr(tagname_start,off-tagname_start);
  strutils::trim(s);
  tagnames.emplace_back(s);
  if (root_element_.name_.empty()) {
    eaddr.p=&root_element_;
  } else {
    if (parent_elements.size() == 0 || eaddr.p != parent_elements.back()) {
      parent_elements.emplace_back(eaddr.p);
    }
    eaddr.p=new XMLElement;
    element_addresses.emplace_back(eaddr);
    parent_elements.back()->element_address_list.emplace_back(eaddr);
  }
  eaddr.p->name_=s;
}

void XMLSnippet::parse(string& xml_element) {
  int off=0,len,n;
  int tagname_start=0,last_space=0,attribute_value_start=0,content_end=0;
  std::list<string> tagnames;
  std::list<int> content_starts;
  std::list<XMLElement> elements;
  string sdum;
  XMLAttribute attr;
  XMLElementAddress eaddr;
  std::list<XMLElement *> parent_elements;
  int last_untag_off=-1,untag_off=0;
  bool in_tag=false;
  bool in_tagname_open=false,in_tagname_close=false;
  bool in_attribute=false;
  bool in_comment=false,in_cdata=false;
  bool in_single_quotes=false;
  bool in_double_quotes=false;

  eaddr.p=NULL;
  untag_buffer.allocate(xml_element.length()+1);
  untag_buffer_off=0;
// strip any leading extraneous characters - xml_element should begin with '<'
  while (off < static_cast<int>(xml_element.length()) && xml_element[off] != '<') {
    ++off;
  }
  if (off == static_cast<int>(xml_element.length())) {
    parse_error_="Not an XML element";
    return;
  }
  while (off < static_cast<int>(xml_element.length())) {
    if (xml_element[off] == '<') {
      if (!in_attribute) {
        if (xml_element.substr(off+1,3) == "!--") {
          if (in_comment) {
            parse_error_="Previous comment was not closed";
            return;
          }
          in_comment=true;
          off+=2;
        } else if (xml_element.substr(off+1,8) == "![CDATA[") {
          if (in_cdata) {
            parse_error_="Previous CDATA was not closed";
            return;
          }
          in_cdata=true;
          off+=7;
        } else if (xml_element[off+1] == '/') {
          if (!in_tagname_close) {
            in_tagname_close=true;
            tagname_start=off+2;
            content_end=off;
            off++;
          } else {
            parse_error_="Bad tag close specification at offset: "+strutils::itos(off);
            return;
          }
        } else {
          in_tag=true;
          in_tagname_open=true;
          tagname_start=off+1;
        }
      }
    } else if (xml_element[off] == ' ') {
      if (!in_attribute) {
        if (in_tagname_open) {
          process_new_tag_name(xml_element,tagname_start,off,tagnames,eaddr,parent_elements);
          in_tagname_open=false;
        }
      }
      last_space=off;
    } else if (xml_element[off] == '=') {
      if (!in_attribute) {
        if (xml_element[off+1] == '"' || xml_element[off+1] == '\'') {
          attr.name=xml_element.substr(last_space+1,off-1-last_space);
          strutils::trim(attr.name);
          in_attribute=true;
          if (xml_element[off+1] == '"') {
            in_double_quotes=true;
          } else {
            in_single_quotes=true;
          }
          attribute_value_start=off+2;
          off++;
        } else if (in_tagname_open) {
          parse_error_="Bad attribute specification at offset: "+strutils::itos(off);
          return;
        }
      }
    } else if (xml_element[off] == '"') {
      if (in_attribute && !in_single_quotes) {
        attr.value=xml_element.substr(attribute_value_start,off-attribute_value_start);
        eaddr.p->attr_list.emplace_back(attr);
        in_attribute=false;
        in_double_quotes=false;
      }
    } else if (xml_element[off] == '\'') {
      if (in_attribute && !in_double_quotes) {
        attr.value=xml_element.substr(attribute_value_start,off-attribute_value_start);
        eaddr.p->attr_list.emplace_back(attr);
        in_attribute=false;
        in_single_quotes=false;
      }
    } else if (xml_element[off] == '-') {
      if (!in_attribute) {
        if (xml_element.substr(off,3) == "-->") {
          if (!in_comment) {
            parse_error_="Comment close found with no open";
            return;
          }
          in_comment=false;
          off+=2;
        }
      }
    } else if (xml_element[off] == ']') {
      if (!in_attribute) {
        if (xml_element.substr(off,3) == "]]>") {
          if (!in_cdata) {
            parse_error_="CDATA close found with no open";
            return;
          }
          in_cdata=false;
          off+=2;
        }
      }
    } else if (xml_element[off] == '/') {
      if (!in_attribute) {
        if (in_tag && xml_element[off+1] == '>') {
          tagnames.pop_back();
          in_tag=false;
          if (parent_elements.size() > 0) {
            if (parent_elements.back() == eaddr.p) {
              parent_elements.pop_back();
            }
            if (parent_elements.size() > 0) {
              eaddr.p=parent_elements.back();
            } else {
              eaddr.p=&root_element_;
            }
          } else {
            eaddr.p=&root_element_;
          }
          ++off;
        }
      }
    } else if (xml_element[off] == '>') {
      if (!in_attribute) {
        if (in_tagname_open) {
          process_new_tag_name(xml_element,tagname_start,off,tagnames,eaddr,parent_elements);
          in_tagname_open=false;
        } else if (in_tagname_close) {
          sdum=xml_element.substr(tagname_start,off-tagname_start);
          strutils::trim(sdum);
          if (tagnames.size() == 0) {
            parse_error_="Found element end for '"+sdum+"' but did not find the element beginning";
            return;
          } else {
            if (sdum == tagnames.back()) {
              in_tagname_close=false;
              if (content_end > content_starts.back()) {
                eaddr.p->content_s=xml_element.substr(content_starts.back(),content_end-content_starts.back());
              }
              tagnames.pop_back();
              content_starts.pop_back();
              if (parent_elements.size() > 0) {
                if (parent_elements.back() == eaddr.p) {
                  parent_elements.pop_back();
                }
                if (parent_elements.size() > 0) {
                  eaddr.p=parent_elements.back();
                } else {
                  eaddr.p=&root_element_;
                }
              } else {
                eaddr.p=&root_element_;
              }
            } else {
              parse_error_="End of element '"+sdum+"' does not match beginning of element '"+tagnames.back()+"'";
              return;
            }
          }
        }
        if (in_tag) {
          in_tag=false;
          content_starts.emplace_back(off+1);
        }
      }
    }
    if (xml_element[off] != '>' && !in_tag && !in_tagname_close) {
      untag_off=off;
      if (last_untag_off < 0) {
        last_untag_off=off;
      }
    } else if (last_untag_off >= 0) {
      len=untag_off-last_untag_off+1;
      std::copy(&xml_element[last_untag_off],&xml_element[last_untag_off+len],&untag_buffer[untag_buffer_off]);
      untag_buffer_off+=len;
      last_untag_off=-1;
    }
    ++off;
  }
  if (tagnames.size() > 0) {
    parse_error_="End tag missing somewhere - "+strutils::itos(tagnames.size())+" tags still remaining: ";
    n=0;
    for (auto& tagname : tagnames) {
      if (n > 0) {
        parse_error_+=", ";
      }
      parse_error_+=tagname;
      ++n;
    }
  } else if (content_starts.size() > 0) {
    parse_error_="Content not closed somewhere";
  } else if (parent_elements.size() > 0) {
    parse_error_="Element not closed somewhere; size = "+strutils::itos(parent_elements.size())+"; "+parent_elements.front()->name();
  }
}

void XMLSnippet::printElement(std::ostream& outs,XMLElement& element,bool isRoot,size_t indent) {
  for (size_t n=0; n < indent; ++n) {
    outs << " ";
  }
  outs << "<" << element.name_;
  auto is_first=true;
  for (auto attr : element.attr_list) {
    if (isRoot && !is_first) {
      outs << endl << " ";
      for (size_t n=0; n < element.name_.length(); ++n) {
        outs << " ";
      }
    }
    outs << " " << attr.name << "=\"" << attr.value << "\"";
    is_first=false;
  }
  outs << ">";
  if (element.element_address_list.size() > 0) {
    outs << endl;
    for (auto address : element.element_address_list)
      printElement(outs,*address.p,false,indent+2);
  } else
    outs << element.content_s;
  if (element.element_address_list.size() > 0) {
    for (size_t n=0; n < indent; ++n) {
      outs << " ";
    }
  }
  outs << "</" << element.name_ << ">" << endl;
}

z_stream *zs=nullptr;
Buffer zbuf;
size_t gunzip(const unsigned char *compressed,size_t compressed_length,std::unique_ptr<char[]>& uncompressed) {
  if (zs == nullptr) {
    zs=new z_stream;
    zs->zalloc=Z_NULL;
    zs->zfree=Z_NULL;
    zs->opaque=Z_NULL;
    zs->next_in=Z_NULL;
    zs->avail_in=0;
    if (inflateInit2(zs,MAX_WBITS+32) != Z_OK) {
      delete zs;
      zs=nullptr;
      return 0;
    }
  } else {
    inflateReset2(zs,MAX_WBITS+32);
  }
  zbuf.allocate(compressed_length);
  std::copy(compressed,compressed+compressed_length,&zbuf[0]);
  zs->next_in=&zbuf[0];
  zs->avail_in=compressed_length;
  zs->avail_out=(compressed[compressed_length-1] << 24)+(compressed[compressed_length-2] << 16)+(compressed[compressed_length-3] << 8)+compressed[compressed_length-4];
  uncompressed.reset(new char[zs->avail_out]);
  zs->next_out=reinterpret_cast<unsigned char *>(uncompressed.get());
  if (inflate(zs,Z_FINISH) != Z_STREAM_END) {
    delete zs;
    zs=nullptr;
    return 0;
  }
  return zs->total_out;
}

bool XMLDocument::open(const string& filename) {
  if (!name_.empty() || !version_.empty()) {
    return false;
  }
  std::ifstream ifs(filename);
  if (!ifs.is_open()) {
    return false;
  }
  name_=filename;
  ifs.seekg(0,std::ios::end);
  size_t buf_len=ifs.tellg();
  std::unique_ptr<char[]> buffer;
  buffer.reset(new char[buf_len]);
  ifs.seekg(0,std::ios::beg);
  ifs.read(buffer.get(),buf_len);
  ifs.close();
  unsigned char *b=reinterpret_cast<unsigned char *>(buffer.get());
  if (b[0] == 0x1f && b[1] == 0x8b) {
// gzipped
    if ( (buf_len=gunzip(b,buf_len,buffer)) == 0) {
      return false;
    }
  }
  string sline;
  sline.assign(buffer.get(),256);
  if (std::regex_search(sline,std::regex("^<\\?xml"))) {
    replace_all(sline,"<?xml","");
    while (strutils::has_beginning(sline," ")) {
      sline=sline.substr(1);
    }
    if (!std::regex_search(sline,std::regex("^version="))) {
      parse_error_="Unable to determine XML version";
      return false;
    }
    replace_all(sline,"version=","");
    auto c=sline.front();
    if (c != '"' && c != '\'') {
      parse_error_="Bad version attribute";
      return false;
    }
    sline=sline.substr(1);
    version_=sline.substr(0,sline.find(string(1,c)));
  }
  sline.assign(buffer.get(),buf_len);
  size_t idx;
  auto n=0;
  while ( (idx=sline.substr(n).find("?>",n)) != string::npos) {
    n+=(idx+2);
  }
  sline=sline.substr(n);
  idx = sline.find("<!DOCTYPE");
  if (idx != string::npos && sline.substr(idx+9).find("[") > 0) {
    auto idx2 = sline.find("]>");
    if (idx2 != string::npos && idx2 > idx) {
      sline = sline.substr(0, idx) + sline.substr(idx2+2);
    }
  }
  strutils::trim(sline);
  parse(sline);
  if (parse_error_.empty()) {
    parsed=true;
    return true;
  } else {
    return false;
  }
}

void XMLDocument::close() {
  if (parsed) {
    name_="";
    version_="";
    root_element_.name_="";
    root_element_.element_address_list.clear();
    root_element_.attr_list.clear();
    root_element_.content_s="";
    parsed=false;
    parse_error_="";
    for (size_t n=0; n < element_addresses.size(); ++n) {
      delete element_addresses[n].p;
    }
    element_addresses.clear();
  }
}

void XMLDocument::show_tree() {
  cout << "XML Document: " << name_ << endl;
  cout << "  version=" << version_ << endl << endl;
  cout << "Root: " << root_element_.name_ << endl;
  showXMLSubTree(root_element_,0);
}

void check(const XMLElement& root,const std::deque<string>& comps,size_t this_comp,std::list<XMLElement>& element_list) {
  auto tc=comps[this_comp];
  replace_all(tc,"$SLASH$","/");
  if (tc.find("@") != string::npos) {
    auto sp=split(tc,"@");
    if (root.name_ == sp[0]) {
      auto is_match=true;
      for (size_t n=1; n < sp.size(); ++n) {
        auto spx=split(sp[n],"=");
        if (root.attribute_value(spx[0]) != spx[1]) {
          is_match=false;
          break;
        }
      }
      if (is_match) {
        if (this_comp < comps.size()-1) {
          for (auto& address : root.element_address_list)
            check(*address.p,comps,this_comp+1,element_list);
        } else
          element_list.emplace_back(root);
      }
    }
  } else {
    if (root.name_ == tc) {
      if (this_comp < comps.size()-1) {
        for (auto& address : root.element_address_list) {
          check(*address.p,comps,this_comp+1,element_list);
        }
      } else {
        element_list.emplace_back(root);
      }
    }
  }
}

void showXMLSubTree(XMLElement& root,size_t indent) {
  for (size_t n=0; n < indent; ++n) {
    cout << " ";
  }
  cout << "Element: " << root.name();
  if (root.attribute_count() > 0) {
    for (auto& attribute : root.attribute_list() ) {
      cout << "@" << attribute.name << "=" << attribute.value;
    }
  }
  if (!root.content().empty()) {
    cout << " - " << root.content();
  }
  cout << endl;
  for (auto& address : root.element_addresses() ) {
    showXMLSubTree(*address.p,indent+2);
  }
}
