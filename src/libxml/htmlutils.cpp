#include <string>
#include <vector>
#include <strutils.hpp>
#include <xmlutils.hpp>

namespace htmlutils {

struct SummaryNode {
  SummaryNode() : node_copy(),node_value(),nodes() {}

  std::string node_copy,node_value;
  std::vector<SummaryNode> nodes;
};

void fill_summary_nodes(std::string string,std::vector<SummaryNode>& nodes)
{
  std::deque<std::string> sp,spn;
  std::string begin_tag,end_tag;
  SummaryNode node;

  strutils::trim(string);
  if (string.find(" ") != std::string::npos && string.find(" ") < string.find(">")) {
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
  for (size_t n=0; n < sp.size(); ++n) {
    if (strutils::has_beginning(sp[n],"<") && !strutils::has_beginning(sp[n],"</") && !strutils::has_ending(sp[n],"/>")) {
	begin_tag=sp[n];
	node.node_copy=begin_tag;
	if (begin_tag.find(" ") != std::string::npos) {
	  begin_tag=begin_tag.substr(0,begin_tag.find(" "));
	}
	else {
	  strutils::replace_all(begin_tag,">","");
	}
	end_tag=begin_tag+">";
	strutils::replace_all(end_tag,"<","</");
	int x=0;
	while (n < sp.size()-1 && (sp[++n] != end_tag || x > 0)) {
	  if (strutils::has_beginning(sp[n],begin_tag)) {
	    if ((strutils::contains(sp[n]," ") && sp[n].substr(0,sp[n].find(" ")) == begin_tag) || sp[n] == (begin_tag+">")) {
		++x;
	    }
	  }
	  if (sp[n] == end_tag) {
	    --x;
	  }
	  node.node_copy+=sp[n];
	}
	node.node_copy+=sp[n];
	spn=xmlutils::split(node.node_copy);
	node.nodes.clear();
	if (spn.size() > 3) {
	  fill_summary_nodes(node.node_copy,node.nodes);
	}
	nodes.emplace_back(node);
    }
  }
}

void wrap_summary_node(std::string& string,std::string fill,size_t chars)
{
  size_t n,m,l;
  size_t index;
size_t line_len;
  bool found_non_blank;

//  chars-=fill.length();
line_len=chars;
  n=0;
//  while (string.substr(n).length() > chars) {
while (string.substr(n).length() > line_len) {
    index=string.substr(n).find("\n");
//    if (index >= 0 && index <= (int)chars)
if (index != std::string::npos && index <= line_len)
	n+=(index+1);
    else {
if (line_len == chars && fill.length() >= 2 && (string.substr(n,fill.length()-2) != fill.substr(0,fill.length()-2) || string.substr(n,fill.length()-1)[fill.length()-2] != '*' || string.substr(n,fill.length())[fill.length()-1] != ' '))
line_len-=fill.length();
	m=n;
//	n+=chars;
n+=line_len-1;
if (string.length() > (n+1) && string[n+1] == ' ')
n++;
else {
	while (string[n] != ' ')
	  --n;
}
	found_non_blank=false;
	for (l=m; l < n; l++) {
	  if (string[l] != ' ')
	    found_non_blank=true;
	}
	if (!found_non_blank) {
//	  n=m+chars+1;
n=m+line_len+1;
	  while (n < string.length() && string[n] != ' ' && string.substr(n,1) != "\n")
	    ++n;
	}
	if (n != string.length()) {
	  string=string.substr(0,n)+"\n"+fill+string.substr(n+1);
	}
	else {
	  string=string.substr(0,n)+"\n"+fill;
	}
	n+=(fill.length()+1);
    }
  }
}

void process_summary_node(SummaryNode& node,size_t wrap_length)
{
  static std::string indent;
  std::string temp;
  size_t idx,idx2;

  strutils::replace_all(node.node_copy,"> <","><");
  strutils::replace_all(node.node_value,"> <","><");
/*
  strutils::replace_all(node.node_copy,". <",".<");
  strutils::replace_all(node.node_value,". <",".<");
*/
strutils::replace_all(node.node_copy,". </",".</");
strutils::replace_all(node.node_value,". </",".</");
  if (node.nodes.size() > 0) {
    for (auto& n : node.nodes) {
	n.node_value=n.node_copy;
	if (strutils::has_beginning(n.node_copy,"<ul>")) {
	  indent+="   ";
	}
	process_summary_node(n,wrap_length);
	if (!n.node_value.empty()) {
	  strutils::replace_all(node.node_value,n.node_copy,n.node_value);
	}
	if (strutils::has_beginning(n.node_copy,"<ul>")) {
	  strutils::chop(indent);
	  strutils::chop(indent);
	  strutils::chop(indent);
	}
    }
  }
  if (strutils::has_beginning(node.node_copy,"<a href=")) {
idx=node.node_value.substr(9).find("\"")+9;
idx=node.node_value.substr(idx).find(">")+idx;
//    node.node_value=node.node_value.substr(node.node_value.find(">")+1);
node.node_value=node.node_value.substr(idx+1);
    strutils::replace_all(node.node_value,"</a>","");
    temp=node.node_copy.substr(node.node_copy.find("href=")+6);
    if ( (idx=temp.find("\"")) == std::string::npos) {
	idx=temp.find("'");
    }
    temp=temp.substr(0,idx);
    if (strutils::has_beginning(temp,"mailto:")) {
	node.node_value="["+temp+"]";
    }
    else {
	node.node_value+=" ["+temp+"]";
    }
  }
  else if (strutils::has_beginning(node.node_copy,"<font")) {
    node.node_value=node.node_value.substr(node.node_value.find(">")+1);
    strutils::replace_all(node.node_value,"</font>","");
  }
  else if (strutils::has_beginning(node.node_copy,"<li>")) {
    strutils::replace_all(node.node_value,"<li>","");
    strutils::replace_all(node.node_value,"</li>","");
    node.node_value="\n"+indent+"    * "+node.node_value;
    wrap_summary_node(node.node_value,(indent+"      "),wrap_length);
  }
  else if (strutils::has_beginning(node.node_copy,"<p>")) {
    strutils::replace_all(node.node_value,"<p>","");
    if (strutils::has_beginning(node.node_value,"\n")) {
	node.node_value=node.node_value.substr(1);
    }
    strutils::replace_all(node.node_value,"</p>","\n");
  }
  else if (strutils::has_beginning(node.node_copy,"<P>")) {
    strutils::replace_all(node.node_value,"<P>","");
    strutils::replace_all(node.node_value,"</P>","");
    if (!strutils::has_ending(node.node_value,"\n")) {
      node.node_value+="\n";
    }
  }
  else if (strutils::has_beginning(node.node_copy,"<ul>")) {
    strutils::replace_all(node.node_value,"<ul>","");
    strutils::replace_all(node.node_value,"</ul>","\n");
  }
  else if (strutils::has_beginning(node.node_copy,"<")) {
    idx=node.node_copy.find(" ");
    idx2=node.node_copy.find(">");
    if (idx == std::string::npos || idx2 < idx) {
	temp=node.node_copy.substr(1,idx2-1);
    }
    else {
	temp=node.node_copy.substr(1,idx-1);
    }
    node.node_value=node.node_value.substr(idx2+1);
    strutils::replace_all(node.node_value,"</"+temp+">","");
  }
}

std::string convert_html_summary_to_ascii(std::string summary,size_t wrap_length,size_t indent_length)
{
  std::vector<SummaryNode> nodes;
  std::string ascii_summary;
  std::string indent;

  ascii_summary.reserve(50000);
  for (size_t n=0; n < indent_length; ++n) {
    indent+=" ";
  }
  strutils::replace_all(summary,"\n"," ");
  strutils::replace_all(summary,"&nbsp;"," ");
  strutils::replace_all(summary,"&deg;","degree");
  strutils::replace_all(summary,"&lt;","less than");
  strutils::replace_all(summary,"&gt;","greater than");
  while (strutils::contains(summary,"  ")) {
    strutils::replace_all(summary,"  "," ");
  }
  fill_summary_nodes(summary,nodes);
  for (auto& node : nodes) {
    node.node_value=node.node_copy;
    process_summary_node(node,wrap_length);
    while (strutils::has_ending(node.node_value,"\n\n")) {
	strutils::chop(node.node_value);
    }
    wrap_summary_node(node.node_value,indent,wrap_length);
    ascii_summary+=indent+node.node_value+"\n";
  }
  strutils::trim_back(ascii_summary);
  return ascii_summary;
}

std::string transform_superscripts(std::string s)
{
  auto ts=s;
  while (strutils::contains(ts,"^")) {
    auto n=ts.find("^");
    auto m=n+1;
    while ((ts[m] >= '0' && ts[m] <= '9') || ts[m] == '-') {
      ++m;
    }
    ts=ts.substr(0,n)+"<sup>"+ts.substr(n+1,m-n-1)+"</sup>"+ts.substr(m);
  }
  ts="<nobr>"+ts+"</nobr>";
  return ts;
}

} // end namespace htmlutils
