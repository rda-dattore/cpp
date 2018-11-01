#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <unordered_set>
#include <deque>
#include <regex>
#include <cstring>
#include <cmath>
#include <strutils.hpp>

namespace htmlutils {

std::string unicode_escape_to_html(std::string u)
{
  auto ue=std::regex("(\\\\u([0-9a-fA-F]){4})");
  std::smatch parts;
  while (std::regex_search(u,parts,ue) && parts.ready()) {
    auto x=parts[1].str().substr(2);
    while (x[0] == '0') {
        x.erase(0,1);
    }
    strutils::replace_all(u,parts[1],"&#x"+x+";");
  }
  return u;
}

} // end namespace htmlutils

namespace strutils {

void chop(std::string& s,size_t num_chars)
{
  int new_length;
  if (num_chars == 0) {
    return;
  }
  if ( (new_length=s.length()-num_chars) < 0) {
    new_length=0;
  }
  s.resize(new_length);
}

void convert_unicode(std::string& s)
{
  for (size_t n=0; n < s.length(); ++n) {
    if (static_cast<int>(s[n]) == 0xc2 || static_cast<int>(s[n]) == 0xc3) {
	switch (static_cast<int>(s[n+1])) {
	  case 0x80:
	  case 0x81:
	  case 0x82:
	  case 0x83:
	  case 0x84:
	  case 0x85:
	    s[n+1]='A';
	    break;
	  case 0x87:
	    s[n+1]='C';
	    break;
	  case 0x88:
	  case 0x89:
	  case 0x8a:
	  case 0x8b:
	    s[n+1]='E';
	    break;
	  case 0x8c:
	  case 0x8d:
	  case 0x8e:
	  case 0x8f:
	    s[n+1]='I';
	    break;
	  case 0x91:
	    s[n+1]='N';
	    break;
	  case 0x92:
	  case 0x93:
	  case 0x94:
	  case 0x95:
	  case 0x96:
	  case 0x98:
	    s[n+1]='O';
	    break;
	  case 0x99:
	  case 0x9a:
	  case 0x9b:
	  case 0x9c:
	    s[n+1]='U';
	    break;
	  case 0x9d:
	    s[n+1]='Y';
	    break;
	  case 0xa0:
	  case 0xa1:
	  case 0xa2:
	  case 0xa3:
	  case 0xa4:
	  case 0xa5:
	    s[n+1]='a';
	    break;
	  case 0xa7:
	    s[n+1]='c';
	    break;
	  case 0xa8:
	  case 0xa9:
	  case 0xaa:
	  case 0xab:
	    s[n+1]='e';
	    break;
	  case 0xac:
	  case 0xad:
	  case 0xae:
	  case 0xaf:
	    s[n+1]='i';
	    break;
	  case 0xb1:
	    s[n+1]='n';
	    break;
	  case 0xb2:
	  case 0xb3:
	  case 0xb4:
	  case 0xb5:
	  case 0xb6:
	  case 0xb8:
	    s[n+1]='o';
	    break;
	  case 0xb9:
	  case 0xba:
	  case 0xbb:
	  case 0xbc:
	    s[n+1]='u';
	    break;
	  case 0xbd:
	  case 0xbf:
	    s[n+1]='y';
	    break;
	}
	s.erase(n++,1);
    }
  }
}

void replace_all(std::string& s,const std::string& old_s,const std::string& new_s)
{
  size_t pos=0,index;

  if (s.empty() || old_s == new_s)
    return;
  while (pos < s.length() && (index=s.find(old_s,pos)) != std::string::npos) {
    s.replace(index,old_s.length(),new_s);
    pos=index+new_s.length();
  }
}

void strput(char *string,int numeric,int num_chars,char fill,bool is_signed)
{
  int off=0;
  auto sign=' ';
  if (numeric < 0) {
    is_signed=true;
    sign='-';
    numeric=-numeric;
  }
  else if (is_signed) {
    sign='+';
  }
  if (is_signed) {
    --num_chars;
    if (fill != ' ') {
	string[off++]=sign;
    }
    else {
	string[off++]=' ';
    }
  }
  if (numeric >= pow(10.,num_chars)) {
    std::cerr << "Error in strput: integer " << numeric << " is larger than " << "character length " << num_chars << std::endl;
    exit(1);
  }
  auto lead=true;
  for (int n=0; n < num_chars; ++n) {
    auto div=pow(10.,num_chars-1-n);
    auto chr=numeric/div;
    if (chr != 0 || (n+1) == num_chars) {
	if (is_signed && fill == ' ') {
	  string[n+off-1]=sign;
	  is_signed=false;
	}
	string[n+off]=chr+48;
	lead=false;
    }
    else {
	if (lead) {
	  string[n+off]=fill;
	}
	else {
	  string[n+off]='0';
	}
    }
    if (numeric >= div) {
	numeric-=chr*div;
    }
  }
}

void trim_back(std::string& s)
{
  int beg,end;

  if (s.empty()) {
    return;
  }
  beg=end=s.length()-1;
  while (beg >= 0 && (s[beg] == ' ' || s[beg] == 0x9 || s[beg] == 0xa || s[beg] == 0xd)) {
    --beg;
  }
  if (beg != end) {
    s.erase(beg+1);
  }
}

void trim_front(std::string& s)
{
  size_t n;

  if (s.empty()) {
    return;
  }
  n=0;
  while (n < s.length() && (s[n] == ' ' || s[n] == 0x9 || s[n] == 0xa || s[n] == 0xd)) {
    ++n;
  }
  if (n > 0) {
    s.erase(0,n);
  }
}

void trim(std::string& s)
{
  trim_front(s);
  trim_back(s);
}

bool contains(const std::string& s,const std::string& sub_s)
{
  if (sub_s.empty()) {
    return false;
  }
  if (s.find(sub_s,0) != std::string::npos) {
    return true;
  }
  else {
    return false;
  }
}

bool has_beginning(const std::string& s,const std::string& beginning)
{
  if (s.empty() || beginning.empty() || beginning.length() > s.length())
    return false;
  else {
    if (s.compare(0,beginning.size(),beginning) == 0) {
	return true;
    }
    else {
	return false;
    }
  }
}

bool has_ending(const std::string& s,const std::string& ending)
{
  if (s.empty() || ending.empty() || ending.length() > s.length()) {
    return false;
  }
  else {
    if (s.compare(s.size()-ending.size(),ending.size(),ending) == 0) {
	return true;
    }
    else {
	return false;
    }
  }
}

bool has_no_letters(const std::string& s)
{
  for (size_t n=0; n < s.length(); ++n) {
    if ((s[n] >= 'A' && s[n] <= 'Z') || (s[n] >= 'a' && s[n] <= 'z')) {
        return false;
    }
  }
  return true;
}

bool is_alpha(const std::string& s)
{
  for (size_t n=0; n < s.length(); ++n) {
    if ((s[n] < 'A' || s[n] > 'Z') && (s[n] < 'a' || s[n] > 'z'))
        return false;
  }
  return true;
}

bool is_alphanumeric(const std::string& s)
{
  for (size_t n=0; n < s.length(); ++n) {
    if ((s[n] < '0' || s[n] > '9') && (s[n] < 'A' || s[n] > 'Z') && (s[n] < 'a' || s[n] > 'z'))
        return false;
  }
  return true;
}

bool is_numeric(const std::string& s)
{
  size_t num_decimals=0;

  for (size_t n=0; n < s.length(); ++n) {
    if ((s[n] < '0' || s[n] > '9') && (n > 0 || s[n] != '-')) {
        if (s[n] == '.' && n > 0) {
          num_decimals++;
          if (num_decimals > 1) {
            return false;
	  }
        }
        else {
          return false;
	}
    }
  }
  return true;
}

size_t occurs(const std::string& s,const std::string& find_s)
{
  size_t num_occurs=0,pos=0,index;

  while (pos < s.length() && (index=s.find(find_s,pos)) != std::string::npos) {
    ++num_occurs;
    pos=index+find_s.length();
  }
  return num_occurs;
}

std::deque<std::string> split(const std::string& s,const std::string& separator)
{
  std::deque<std::string> parts;
  if (s.empty()) {
    return parts;
  }
  int check_len=s.length()-separator.length()+1;
  if (check_len < 1) {
    parts.emplace_back(s);
    return parts;
  }
  size_t start=0,end=0;
  auto in_white_space=false;
  while (static_cast<int>(end) < check_len) {
    if (separator.empty()) {
	if (s[end] == ' ') {
	  if (!in_white_space) {
	    parts.emplace_back(s.substr(start,end-start));
	    in_white_space=true;
	  }
	}
	else if (in_white_space) {
	  start=end;
	  in_white_space=false;
	}
	++end;
    }
    else {
	if (strncmp(&s[end],separator.c_str(),separator.length()) == 0) {
	  parts.emplace_back(s.substr(start,end-start));
	  end+=separator.length();
	  start=end;
	}
	else {
	  ++end;
	}
    }
  }
  if (start < s.length()) {
    parts.emplace_back(s.substr(start));
  }
  else {
    parts.emplace_back("");
  }
  return parts;
}

std::vector<std::pair<std::string,size_t>> split_index(const std::string& s,const std::string& separator)
{
  std::vector<std::pair<std::string,size_t>> parts;
  if (s.empty()) {
    return parts;
  }
  int check_len=s.length()-separator.length()+1;
  if (check_len < 1) {
    parts.emplace_back(std::make_pair(s,0));
    return parts;
  }
  size_t start=0,end=0;
  auto in_white_space=false;
  while (static_cast<int>(end) < check_len) {
    if (separator.empty()) {
	if (s[end] == ' ') {
	  if (!in_white_space) {
	    parts.emplace_back(std::make_pair(s.substr(start,end-start),start));
	    in_white_space=true;
	  }
	}
	else if (in_white_space) {
	  start=end;
	  in_white_space=false;
	}
	++end;
    }
    else {
	if (strncmp(&s[end],separator.c_str(),separator.length()) == 0) {
	  parts.emplace_back(std::make_pair(s.substr(start,end-start),start));
	  end+=separator.length();
	  start=end;
	}
	else {
	  ++end;
	}
    }
  }
  if (start < s.length()) {
    parts.emplace_back(std::make_pair(s.substr(start),start));
  }
  else {
    parts.emplace_back(std::make_pair("",s.length()));
  }
  return parts;
}

std::string btos(bool b)
{
  if (b) {
    return "true";
  }
  else {
    return "false";
  }
}

std::string capitalize(const std::string& s,size_t start_word,size_t num_words)
{
  std::string cap_s;
  if (!s.empty()) {
    cap_s=s;
    if (start_word == 0 && cap_s[0] >= 'a' && cap_s[0] <= 'z') {
        cap_s[0]-=32;
    }
    size_t pos=1;
    size_t nwords=0;
    size_t end_word=start_word+num_words;
    size_t index_s=std::string::npos;
    size_t index_p=std::string::npos;
    while (pos < cap_s.length()) {
	if (index_s == std::string::npos) {
	  index_s=cap_s.find("/",pos);
	}
	if (index_p == std::string::npos) {
	  index_p=cap_s.find(" ",pos);
	}
	if (index_s == std::string::npos && index_p == std::string::npos) {
	  if (nwords >= start_word && nwords < end_word) {
	    for (size_t n=pos; n < cap_s.length(); ++n) {
		if (cap_s[n] >= 'A' && cap_s[n] <= 'Z') {
		  cap_s[n]+=32;
		}
	    }
	  }
	  break;
	}
	size_t index;
	if (index_s <= index_p) {
	  index=index_s;
	  index_s=std::string::npos;
	}
	else {
	  index=index_p;
	  index_p=std::string::npos;
	}
	++index;
	if (nwords >= start_word && nwords < end_word) {
	  for (size_t n=pos; n < index; ++n) {
	    if (cap_s[n] >= 'A' && cap_s[n] <= 'Z') {
		cap_s[n]+=32;
	    }
	  }
	  if (cap_s[index] >= 'a' && cap_s[index] <= 'z') {
	    cap_s[index]-=32;
	  }
	}
	pos=index+1;
	++nwords;
    }
  }
  return cap_s;
}

std::string dtos(double val,size_t max_d)
{
  std::stringstream ss;
  std::string s;

  ss.setf(std::ios::fixed);
  ss.precision(max_d);
  ss << val;
  s=ss.str();
  if (contains(s,".")) {
    while (std::regex_search(s,std::regex("0$"))) {
	chop(s,1);
    }
  }
  if (s[s.size()-1] == '.') {
    chop(s,1);
  }
  return s;
}

std::string dtos(double val,size_t w,size_t d,char fill)
{
  std::stringstream ss;
  ss.setf(std::ios::fixed);
  ss.width(w--);
  if (val < 0.) {
    --w;
  }
  auto v=std::fabs(val);
  while (v > 1.) {
    --w;
    v/=10.;
  }
  if (w < d) {
    ss.precision(w);
  }
  else {
    ss.precision(d);
  }
  ss.fill(fill);
  ss << val;
  return ss.str();
}

std::string ftos(float val,size_t max_d)
{
  return dtos(val,max_d);
}

std::string ftos(float val,size_t w,size_t d,char fill)
{
  return dtos(val,w,d,fill);
}

std::string get_env(const std::string& name)
{
  char *n=getenv(name.c_str());
  return (n) ? n : "";
}

std::string itos(int val)
{
  std::stringstream ss;
  ss << val;
  return ss.str();
}

std::string lltos(long long val,size_t w,char fill)
{
  std::stringstream ss;
  if (w > 0) {
    ss.width(w);
  }
  ss.fill(fill);
  ss << val;
  return ss.str();
}

std::string number_with_commas(std::string s)
{
  std::string nwc;
  if (is_numeric(s)) {
    nwc=s;
    auto idx=nwc.find(".");
    if (idx == std::string::npos) {
	idx=nwc.length();
    }
    int n=idx;
    while (n > 3) {
	n-=3;
	nwc.insert(n,",");
    }
  }
  return nwc;
}

std::string number_with_commas(long long l)
{
  return number_with_commas(lltos(l,0,' '));
}

std::string soundex(const std::string& s)
{
  std::string head,tail;
  int n;

  if (s.empty() || !is_alpha(s))
    return "";
  head=s.substr(0,1);
  head=to_upper(head);
  tail=s.substr(1);
  tail=to_upper(tail);
  replace_all(tail,"A","");
  replace_all(tail,"E","");
  replace_all(tail,"I","");
  replace_all(tail,"O","");
  replace_all(tail,"U","");
  replace_all(tail,"H","");
  replace_all(tail,"W","");
  replace_all(tail,"Y","");
  replace_all(tail,"B","1");
  replace_all(tail,"F","1");
  replace_all(tail,"P","1");
  replace_all(tail,"V","1");
  replace_all(tail,"C","2");
  replace_all(tail,"G","2");
  replace_all(tail,"J","2");
  replace_all(tail,"K","2");
  replace_all(tail,"Q","2");
  replace_all(tail,"S","2");
  replace_all(tail,"X","2");
  replace_all(tail,"Z","2");
  replace_all(tail,"D","3");
  replace_all(tail,"T","3");
  replace_all(tail,"L","4");
  replace_all(tail,"M","5");
  replace_all(tail,"N","5");
  replace_all(tail,"R","6");
  for (n=s.length()-1; n > 0; n--) {
    if (tail[n] == tail[n-1])
        tail[n]='*';
  }
  replace_all(tail,"*","");
  while (tail.length() < 3)
    tail+="0";
  return head+tail;
}

std::string strand(size_t length)
{
  time_t tm=time(NULL);
  static size_t cnt=1;
  std::string s;
  s.reserve(length);
  auto a=reinterpret_cast<long long>(&s[0]);
  auto ia=(a % 0x7fffffff);
  srand(tm*getpid()*ia*cnt);
  for (size_t n=0; n < length; ++n) {
    auto mnum=(rand() % 100);
    while (1) {
        while (mnum > 0 && (mnum < 13 || (mnum > 22 && mnum < 30) || (mnum > 55 && mnum < 62) || mnum > 87)) {
          mnum/=10;
        }
        if (mnum != 0)
          break;
        else
          mnum=(rand() % 100);
    }
    s.push_back(mnum+35);
  }
  ++cnt;
  return s;
}

std::string substitute(const std::string& s,const std::string& old_s,const std::string& new_s)
{
  std::string ss=s;
  replace_all(ss,old_s,new_s);
  return ss;
}

std::string to_capital(const std::string& s)
{
  if (s == to_upper(s)) {
    return s;
  }
  std::string cap_s;
  if (!s.empty()) {
    cap_s=s;
    if (cap_s[0] >= 'a' and cap_s[0] <= 'z') {
	cap_s[0]-=32;
    }
    size_t pos=1;
    size_t idx;
    while ( (idx=cap_s.find("_",pos)) != std::string::npos) {
	cap_s[idx]=' ';
	pos=idx+1;
	if (cap_s[pos] >= 'a' and cap_s[pos] <= 'z') {
	  cap_s[pos]-=32;
	}
    }
  }
  return cap_s;
}

std::string to_lower(const std::string& s)
{
  std::string ls;
  ls.reserve(s.length());
  for (size_t n=0; n < s.length(); ++n) {
    if (s[n] >= 'A' && s[n] <= 'Z') {
	ls.push_back(static_cast<char>(static_cast<int>(s[n])+32));
    }
    else {
	ls.push_back(s[n]);
    }
  }
  return ls;
}

std::string to_title(const std::string& s)
{
  static std::unordered_set<std::string> *uncapitalized_words=nullptr;
  if (uncapitalized_words == nullptr) {
    uncapitalized_words=new std::unordered_set<std::string>;
    uncapitalized_words->emplace("a");
    uncapitalized_words->emplace("an");
    uncapitalized_words->emplace("and");
    uncapitalized_words->emplace("as");
    uncapitalized_words->emplace("at");
    uncapitalized_words->emplace("but");
    uncapitalized_words->emplace("by");
    uncapitalized_words->emplace("for");
    uncapitalized_words->emplace("from");
    uncapitalized_words->emplace("in");
    uncapitalized_words->emplace("nor");
    uncapitalized_words->emplace("of");
    uncapitalized_words->emplace("on");
    uncapitalized_words->emplace("or");
    uncapitalized_words->emplace("the");
    uncapitalized_words->emplace("to");
    uncapitalized_words->emplace("up");
  }
  auto title_s=s;
  if (!s.empty()) {
    auto words=split_index(s,"");
    auto last=words.size()-1;
    for (size_t n=0; n < words.size(); ++n) {
	if (words[n].first[0] >= 'A' && words[n].first[0] <= 'Z') {
	  auto num_lower=0;
	  for (size_t m=1; m < words[n].first.length(); ++m) {
	    if (words[n].first[m] >= 'a' && words[n].first[m] <= 'z') {
		++num_lower;
	    }
	  }
	  if (num_lower == 0) {
	    auto end=words[n].second+words[n].first.length();
	    for (size_t m=words[n].second+1; m < end; ++m) {
		if (title_s[m] >= 'A' && title_s[m] <= 'Z') {
		  title_s[m]+=32;
		}
	    }
	    if (n > 0 && n < last && uncapitalized_words->find(strutils::to_lower(words[n].first)) != uncapitalized_words->end()) {
		title_s[words[n].second]+=32;
	    }
	  }
	}
	else if (words[n].first[0] >= 'a' && words[n].first[0] <= 'z') {
	  auto num_upper=0;
	  for (size_t m=1; m < words[n].first.length(); ++m) {
	    if (words[n].first[m] >= 'A' && words[n].first[m] <= 'Z') {
		++num_upper;
	    }
	  }
	  if (num_upper == 0 && (uncapitalized_words->find(words[n].first) == uncapitalized_words->end() || n == 0 || n == last)) {
	    title_s[words[n].second]-=32;
	  }
	}
    }
  }
  return title_s;
}

std::string to_upper(const std::string& s,size_t start,size_t num_chars)
{
  if (start >= s.length()) {
    return s;
  }
  std::string us;
  us.reserve(s.length());
  size_t n=0;
  for (n=0; n < start; ++n) {
    us.push_back(s[n]);
  }
  size_t end=start+num_chars;
  if (end > s.length()) {
    end=s.length();
  }
  for (; n < end; ++n) {
    if (s[n] >= 'a' && s[n] <= 'z') {
	us.push_back(static_cast<char>(static_cast<int>(s[n])-32));
    }
    else {
	us.push_back(s[n]);
    }
  }
  for (; n < s.length(); ++n) {
    us.push_back(s[n]);
  }
  return us;
}

std::string to_upper(const std::string& s)
{
  return to_upper(s,0,s.length());
}

std::string token(const std::string& s,const std::string& separator,size_t token_number)
{
  auto parts=split(s,separator);
  if (token_number < parts.size()) {
    return parts[token_number];
  }
  else {
    return "";
  }
}

std::string uuid_gen()
{
  time_t tm=time(NULL);
  static size_t cnt=1;
  auto a=reinterpret_cast<long long>(&cnt);
  auto ia=(a % 0xffffffff);
  srand(tm*getpid()*ia*cnt);
  std::stringstream uuid;
  uuid << std::hex;
  for (size_t n=0; n < 16; ++n) {
    if (n == 4 || n == 6 || n == 8 || n == 10) {
	uuid << "-";
    }
    uuid << std::setw(2) << std::setfill('0') << (rand() % 255);
  }
  ++cnt;
  return uuid.str();
}

std::string wrap(const std::string& s,size_t wrap_width,size_t indent_width)
{
  std::string s_indent(indent_width,' ');
  std::string s_wrap;
  auto lines=split(s,"\n");
  for (const auto& line : lines) {
    if (!s_wrap.empty() || line.empty()) {
	s_wrap+="\n";
    }
    size_t pos=0;
    while (pos < line.length()) {
	auto end_pos=pos+wrap_width-indent_width;
	s_wrap+=s_indent;
	if (end_pos < line.length()) {
	  auto index=line.rfind(' ',end_pos);
	  if (index < pos) {
// the next contiguous block of text overflows the wrap_width
	    index=line.find(' ',end_pos);
	    if (index == std::string::npos) {
		index=line.length();
	    }
	  }
	  s_wrap+=line.substr(pos,index-pos)+"\n";
	  pos=index+1;
	}
	else {
	  s_wrap+=line.substr(pos);
	  trim_back(s_wrap);
	  pos=line.length();
	}
    }
  }
  return s_wrap;
}

} // end namespace strutils
