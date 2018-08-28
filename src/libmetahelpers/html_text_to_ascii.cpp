#include <metahelpers.hpp>
#include <xml.hpp>
#include <strutils.hpp>

namespace html {

void wrap(std::string& string,std::string fill,size_t chars)
{
  int n,m,l;
  size_t index;
  bool found_non_blank;

  n=0;
  while (string.substr(n).length() > chars) {
    index=string.substr(n).find("\n");
    if (index != std::string::npos && index <= chars) {
	n+=(index+1);
    }
    else {
	m=n;
	n+=chars;
	while (string[n] != ' ') {
	  --n;
	}
	found_non_blank=false;
	for (l=m; l < n; l++) {
	  if (string[l] != ' ') {
	    found_non_blank=true;
	  }
	}
	if (!found_non_blank) {
	  n=m+chars+1;
	  while (string[n] != ' ' && string.substr(n,1) != "\n") {
	    ++n;
	  }
	}
	string=string.substr(0,n)+"\n"+fill+string.substr(n+1);
	++n;
    }
  }
}

void nodes(std::string string,std::vector<Node>& node_list)
{
  std::deque<std::string> sp,spn;
  size_t n;
  std::string begin_tag,end_tag;
  Node node;
  int x;

  strutils::trim(string);
  if (string.find(" ") > 0 && string.find(" ") < string.find(">")) {
    end_tag=string.substr(0,string.find(" "))+">";
  }
  else {
    end_tag=string.substr(0,string.find(">")+1);
  }
  strutils::replace_all(end_tag,"<","</");
  string=string.substr(1);
  string=string.substr(string.find("<"));
  string=string.substr(0,string.length()-end_tag.length());
  sp=xmlutils::split(string);
  for (n=0; n < sp.size(); n++) {
    if (strutils::has_beginning(sp[n],"<") && !strutils::has_beginning(sp[n],"</")) {
	begin_tag=sp[n];
	node.copy=begin_tag;
	if (begin_tag.find(" ") != std::string::npos) {
	  begin_tag=begin_tag.substr(0,begin_tag.find(" "));
	}
	else {
	  strutils::replace_all(begin_tag,">","");
	}
	end_tag=begin_tag+">";
	strutils::replace_all(end_tag,"<","</");
	x=0;
	while (sp[++n] != end_tag || x > 0) {
	  if (strutils::has_beginning(sp[n],begin_tag)) {
	    if ((strutils::contains(sp[n]," ") && sp[n].substr(0,sp[n].find(" ")) == begin_tag) || sp[n] == (begin_tag+">")) {
		++x;
	    }
	  }
	  if (sp[n] == end_tag) {
	    --x;
	  }
	  node.copy+=sp[n];
	}
	node.copy+=sp[n];
	spn=xmlutils::split(node.copy);
	node.node_list.clear();
	if (spn.size() > 3) {
	  nodes(node.copy,node.node_list);
	}
	node_list.emplace_back(node);
    }
  }
}

void process_node(Node& node,size_t wrap_length)
{
  static std::string indent;
  std::string sdum;

  if (node.node_list.size() > 0) {
    for (auto& n : node.node_list) {
	n.value=n.copy;
	if (strutils::has_beginning(n.copy,"<ul>")) {
	  indent+="   ";
	}
	process_node(n,wrap_length);
	if (!n.value.empty()) {
	  strutils::replace_all(node.value,n.copy,n.value);
	}
	if (strutils::has_beginning(n.copy,"<ul>")) {
	  strutils::chop(indent,3);
	}
    }
  }
  if (strutils::has_beginning(node.copy,"<a href=")) {
    node.value=node.value.substr(node.value.find(">")+1);
    strutils::replace_all(node.value,"</a>","");
    sdum=node.copy.substr(node.copy.find("href=")+6);
    sdum=sdum.substr(0,sdum.find("\""));
    if (!strutils::has_ending(node.value,".")) {
	node.value+=" at ["+sdum+"]";
    }
    else {
	node.value=node.value.substr(0,node.value.length()-1)+" at ["+sdum+"].";
    }
  }
  else if (strutils::has_beginning(node.copy,"<li>")) {
    strutils::replace_all(node.value,"<li>","");
    strutils::replace_all(node.value,"</li>","");
    node.value="\n"+indent+"* "+node.value;
    wrap(node.value,(indent+"  "),wrap_length);
  }
  else if (strutils::has_beginning(node.copy,"<ul>")) {
    strutils::replace_all(node.value,"<ul>","");
    strutils::replace_all(node.value,"</ul>","\n");
  }
  else {
    sdum=node.value.substr(0,node.value.find(">")+1);
    strutils::replace_all(node.value,sdum,"");
    if (strutils::contains(sdum," ")) {
      sdum="</"+node.value.substr(1,node.value.find(" ")-1)+">";
    }
    else {
	sdum.insert(1,"/");
    }
    strutils::replace_all(node.value,sdum,"");
    if (sdum == "</p>" || sdum == "</P>") {
	node.value+="\n";
    }
  }
}

std::string html_text_to_ascii(std::string& element_copy,size_t wrap_length)
{
  if (!element_copy.empty()) {
    auto ascii_element=element_copy;
    strutils::replace_all(ascii_element,std::string("\n")+"   ","");
    std::vector<Node> node_list;
    nodes(ascii_element,node_list);
    ascii_element="";
    for (auto& node : node_list) {
	node.value=node.copy;
	process_node(node,wrap_length);
	while (strutils::has_ending(node.value,"\n\n")) {
	  strutils::chop(node.value);
	}
	wrap(node.value,std::string(""),wrap_length);
	ascii_element+=node.value;
    }
    while (strutils::contains(ascii_element,"  ")) {
	strutils::replace_all(ascii_element,"  "," ");
    }
    while (strutils::contains(ascii_element,std::string("\n")+" ")) {
	strutils::replace_all(ascii_element,std::string("\n")+" ","\n");
    }
    while (strutils::contains(ascii_element,"\n\n")) {
	strutils::replace_all(ascii_element,"\n\n","\n");
    }
    strutils::trim(ascii_element);
    if (strutils::contains(ascii_element,"<") || strutils::contains(ascii_element,">")) {
	return "";
    }
    else {
	return ascii_element;
    }
  }
  else {
    return "";
  }
}

}
