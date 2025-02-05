#include <string>
#include <vector>
#include <strutils.hpp>
#include <xmlutils.hpp>

using std::string;
using std::vector;
using strutils::chop;
using strutils::replace_all;
using strutils::trim;
using strutils::trim_back;

namespace htmlutils {

struct SummaryNode {
  SummaryNode() : node_copy(), node_value(), nodes() { }

  string node_copy, node_value;
  vector<SummaryNode> nodes;
};

void fill_summary_nodes(string html, vector<SummaryNode>& nodes) {
  trim(html);
  string end_tag;
  if (html.find(" ") != string::npos && html.find(" ") < html.find(">")) {
    end_tag = html.substr(0, html.find(" ")) + ">";
  } else {
    end_tag = html.substr(0, html.find(">")+1);
  }
  replace_all(end_tag,"<","</");
  html = html.substr(1);
  html = html.substr(html.find("<"));
  html = html.substr(0, html.length()-end_tag.length());
  auto sp = xmlutils::split(html);
  for (size_t n = 0; n < sp.size(); ++n) {
    if (sp[n][0] == '<' && sp[n][1] != '/' && !strutils::has_ending(sp[n],
        "/>")) {
      auto begin_tag = sp[n];
      SummaryNode node;
      node.node_copy = begin_tag;
      if (begin_tag.find(" ") != string::npos) {
        begin_tag = begin_tag.substr(0, begin_tag.find(" "));
      } else {
        replace_all(begin_tag, ">", "");
      }
      end_tag = begin_tag + ">";
      replace_all(end_tag, "<", "</");
      int x = 0;
      while (n < sp.size()-1 && (sp[++n] != end_tag || x > 0)) {
        if (sp[n].find(begin_tag) == 0) {
          if ((sp[n].find(" ") != string::npos && sp[n].substr(0, sp[n].find(
              " ")) == begin_tag) || sp[n] == (begin_tag + ">")) {
            ++x;
          }
        }
        if (sp[n] == end_tag) {
          --x;
        }
        node.node_copy += sp[n];
      }
      node.node_copy += sp[n];
      auto spn = xmlutils::split(node.node_copy);
      node.nodes.clear();
      if (spn.size() > 3) {
        fill_summary_nodes(node.node_copy, node.nodes);
      }
      nodes.emplace_back(node);
    }
  }
}

void wrap_summary_node(string& text, string fill, size_t wrap_length) {
  auto line_len = wrap_length;
  size_t n = 0;
  while (text.substr(n).length() > line_len) {
    auto idx = text.substr(n).find("\n");
    if (idx != string::npos && idx <= line_len) {
      n += (idx+1);
    } else {
      if (line_len == wrap_length && fill.length() >= 2 && (text.substr(n, fill.
          length()-2) != fill.substr(0, fill.length()-2) || text.substr(n, fill.
          length()-1)[fill.length()-2] != '*' || text.substr(n, fill.length())[
          fill.length()-1] != ' ')) {
        line_len -= fill.length();
      }
      auto m = n;
      n += (line_len-1);
      if (text.length() > (n+1) && text[n+1] == ' ') {
        ++n;
      } else {
        while (text[n] != ' ') {
          --n;
        }
      }
      auto found_non_blank = false;
      for (size_t l = m; l < n; ++l) {
        if (text[l] != ' ') {
          found_non_blank = true;
        }
      }
      if (!found_non_blank) {
        n = m + line_len + 1;
        while (n < text.length() && text[n] != ' ' && text.substr(n, 1) !=
            "\n") {
          ++n;
        }
      }
      if (n != text.length()) {
        text = text.substr(0, n) + "\n" + fill + text.substr(n+1);
      } else {
        text = text.substr(0, n) + "\n" + fill;
      }
      n += (fill.length()+1);
    }
  }
}

void process_summary_node(SummaryNode& node, size_t wrap_length) {
  static string indent;

  replace_all(node.node_copy, "> <", "><");
  replace_all(node.node_value, "> <", "><");
  replace_all(node.node_copy, ". </", ".</");
  replace_all(node.node_value, ". </", ".</");
  if (!node.nodes.empty()) {
    for (auto& n : node.nodes) {
      if (n.node_copy.find("<ul>") == 0) {
        indent += "   ";
      }
      n.node_value = n.node_copy;
      process_summary_node(n, wrap_length);
      if (!n.node_value.empty()) {
        replace_all(node.node_value, n.node_copy, n.node_value);
      }
      if (n.node_copy.find("<ul>") == 0) {
        chop(indent);
        chop(indent);
        chop(indent);
      }
    }
  }
  if (node.node_copy.find("<a href=") == 0) {
    auto idx = node.node_value.substr(9).find("\"") + 9;
    idx = node.node_value.substr(idx).find(">") + idx;
    node.node_value = node.node_value.substr(idx+1);
    replace_all(node.node_value, "</a>", "");
    auto temp = node.node_copy.substr(node.node_copy.find("href=")+6);
    if ( (idx=temp.find("\"")) == string::npos) {
      idx = temp.find("'");
    }
    temp = temp.substr(0, idx);
    if (temp.find("mailto:") == 0) {
      node.node_value = "[" + temp + "]";
    } else {
      node.node_value += " [" + temp + "]";
    }
  } else if (node.node_copy.find("<font") == 0) {
    node.node_value = node.node_value.substr(node.node_value.find(">")+1);
    replace_all(node.node_value, "</font>", "");
  } else if (node.node_copy.find("<li>") == 0) {
    replace_all(node.node_value, "<li>", "");
    replace_all(node.node_value, "</li>", "");
    node.node_value = "\n" + indent + "    * " + node.node_value;
    wrap_summary_node(node.node_value, (indent + "      "), wrap_length);
  } else if (node.node_copy.find("<p>") == 0) {
    replace_all(node.node_value, "<p>", "");
    if (node.node_value.find("\n") == 0) {
      node.node_value = node.node_value.substr(1);
    }
    replace_all(node.node_value, "</p>", "\n");
  } else if (node.node_copy.find("<P>") == 0) {
    replace_all(node.node_value, "<P>", "");
    replace_all(node.node_value, "</P>", "");
    if (!strutils::has_ending(node.node_value,"\n")) {
      node.node_value += "\n";
    }
  } else if (node.node_copy.find("<ul>") == 0) {
    replace_all(node.node_value, "<ul>", "");
    replace_all(node.node_value, "</ul>", "\n");
  } else if (node.node_copy.find("<") == 0) {
    auto idx = node.node_copy.find(" ");
    auto idx2 = node.node_copy.find(">");
    string temp;
    if (idx == string::npos || idx2 < idx) {
      temp = node.node_copy.substr(1, idx2-1);
    } else {
      temp = node.node_copy.substr(1, idx-1);
    }
    node.node_value = node.node_value.substr(idx2+1);
    replace_all(node.node_value, "</" + temp + ">", "");
  }
}

string convert_html_summary_to_ascii(string summary,size_t wrap_length,size_t indent_length)
{
  vector<SummaryNode> nodes;
  string ascii_summary;
  string indent;

  ascii_summary.reserve(50000);
  for (size_t n=0; n < indent_length; ++n) {
    indent+=" ";
  }
  replace_all(summary,"\n"," ");
  replace_all(summary,"&nbsp;"," ");
  replace_all(summary,"&deg;","degree");
  replace_all(summary,"&lt;","less than");
  replace_all(summary,"&gt;","greater than");
  replace_all(summary,"<br>",";");
  replace_all(summary,"<br />",";");
  while (strutils::contains(summary,"  ")) {
    replace_all(summary,"  "," ");
  }
  fill_summary_nodes(summary,nodes);
  for (auto& node : nodes) {
    node.node_value=node.node_copy;
    process_summary_node(node,wrap_length);
    while (strutils::has_ending(node.node_value,"\n\n")) {
      chop(node.node_value);
    }
    wrap_summary_node(node.node_value,indent,wrap_length);
    ascii_summary+=indent+node.node_value+"\n";
  }
  trim_back(ascii_summary);
  return ascii_summary;
}

string transform_superscripts(string s)
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
