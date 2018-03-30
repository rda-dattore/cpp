#include <metahelpers.hpp>
#include <xml.hpp>
#include <strutils.hpp>

namespace html {

void wrap(std::string& string,std::string fill,size_t chars)
{
  int n,m,l;
  size_t index;
  bool foundNonBlank;

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
	foundNonBlank=false;
	for (l=m; l < n; l++) {
	  if (string[l] != ' ') {
	    foundNonBlank=true;
	  }
	}
	if (!foundNonBlank) {
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
  std::string beginTag,endTag;
  Node node;
  int x;

  strutils::trim(string);
  if (string.find(" ") > 0 && string.find(" ") < string.find(">")) {
    endTag=string.substr(0,string.find(" "))+">";
  }
  else {
    endTag=string.substr(0,string.find(">")+1);
  }
  strutils::replace_all(endTag,"<","</");
  string=string.substr(1);
  string=string.substr(string.find("<"));
  string=string.substr(0,string.length()-endTag.length());
  sp=xmlutils::split(string);
  for (n=0; n < sp.size(); n++) {
    if (strutils::has_beginning(sp[n],"<") && !strutils::has_beginning(sp[n],"</")) {
	beginTag=sp[n];
	node.copy=beginTag;
	if (beginTag.find(" ") != std::string::npos) {
	  beginTag=beginTag.substr(0,beginTag.find(" "));
	}
	else {
	  strutils::replace_all(beginTag,">","");
	}
	endTag=beginTag+">";
	strutils::replace_all(endTag,"<","</");
	x=0;
	while (sp[++n] != endTag || x > 0) {
	  if (strutils::has_beginning(sp[n],beginTag)) {
	    if ((strutils::contains(sp[n]," ") && sp[n].substr(0,sp[n].find(" ")) == beginTag) || sp[n] == (beginTag+">")) {
		++x;
	    }
	  }
	  if (sp[n] == endTag) {
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

void process_node(Node& node,size_t wrapLength)
{
  static std::string indent;
  std::string sdum;

  if (node.node_list.size() > 0) {
    for (auto& n : node.node_list) {
	n.value=n.copy;
	if (strutils::has_beginning(n.copy,"<ul>")) {
	  indent+="   ";
	}
	process_node(n,wrapLength);
	if (n.value.length() > 0) {
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
    wrap(node.value,(indent+"  "),wrapLength);
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

std::string html_text_to_ASCII(std::string& elementCopy,size_t wrapLength)
{
  if (elementCopy.length() > 0) {
    auto ASCIIElement=elementCopy;
    strutils::replace_all(ASCIIElement,std::string("\n")+"   ","");
    std::vector<Node> node_list;
    nodes(ASCIIElement,node_list);
    ASCIIElement="";
    for (auto& node : node_list) {
	node.value=node.copy;
	process_node(node,wrapLength);
	while (strutils::has_ending(node.value,"\n\n")) {
	  strutils::chop(node.value);
	}
	wrap(node.value,std::string(""),wrapLength);
	ASCIIElement+=node.value;
    }
    while (strutils::contains(ASCIIElement,"  ")) {
	strutils::replace_all(ASCIIElement,"  "," ");
    }
    while (strutils::contains(ASCIIElement,std::string("\n")+" ")) {
	strutils::replace_all(ASCIIElement,std::string("\n")+" ","\n");
    }
    while (strutils::contains(ASCIIElement,"\n\n")) {
	strutils::replace_all(ASCIIElement,"\n\n","\n");
    }
    strutils::trim(ASCIIElement);
    if (strutils::contains(ASCIIElement,"<") || strutils::contains(ASCIIElement,">")) {
	return "";
    }
    else {
	return ASCIIElement;
    }
  }
  else
    return "";
}

}
