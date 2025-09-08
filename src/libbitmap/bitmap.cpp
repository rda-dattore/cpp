#include <iostream>
#include <unordered_map>
#include <bitmap.hpp>
#include <strutils.hpp>

using std::stoi;
using std::string;
using std::vector;
using strutils::has_ending;
using strutils::split;

namespace bitmap {

static const string NON_MONOTONIC_DELIMITER("<N>");

void compress(const string& uncompressed_bitmap,string& compressed_bitmap)
{
  if (&uncompressed_bitmap == &compressed_bitmap) {
    return;
  }
  auto start=uncompressed_bitmap.find(":");
  if (start == string::npos || strutils::contains(uncompressed_bitmap,"-")) {
    compressed_bitmap=uncompressed_bitmap;
    return;
  }
  else {
    ++start;
  }
  compressed_bitmap=uncompressed_bitmap.substr(0,start);
  auto off=start;
  auto cnt=0;
  auto last_c=' ';
  for (size_t n=start; n < uncompressed_bitmap.length(); ++n) {
    auto c=uncompressed_bitmap[n];
    if (last_c == ' ') {
      last_c=c;
    }
    if (c != last_c) {
      if (cnt > 4) {
        compressed_bitmap+="-"+strutils::itos(cnt)+"/"+string(&last_c,1);
      }
      else {
        compressed_bitmap+=uncompressed_bitmap.substr(off,cnt);
      }
      off+=cnt;
      cnt=0;
    }
    last_c=c;
    ++cnt;
  }
  if (cnt > 4) {
    compressed_bitmap+="-"+strutils::itos(cnt)+"/"+string(&last_c,1);
  }
  else {
    compressed_bitmap+=uncompressed_bitmap.substr(off,cnt);
  }
  if (compressed_bitmap.length() > 255) {
    further_compress(compressed_bitmap,compressed_bitmap);
  }
}

void compress_non_monotonic_values(const vector<size_t>& values,size_t multiplier,string& compressed_bitmap)
{
  size_t n=1;
  vector<size_t> vvec;
  vvec.reserve(values.size());
  for (auto& value : values) {
    vvec.emplace_back((multiplier*n)+value);
    ++n;
  }
  compress_values(vvec,compressed_bitmap);
  compressed_bitmap.insert(compressed_bitmap.find(":"),NON_MONOTONIC_DELIMITER+strutils::itos(multiplier));
}

void compress_values(const vector<size_t>& values,string& compressed_bitmap)
{
  static const string BIT[2]={"1","0"};
  static const string ALIAS[2][28]={
    {"","","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",},
    {"","0","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"}
  };
  static const std::unordered_map<char,char> ZERO_ALIAS{
    {'0','2'},{'a','3'},{'b','4'},{'c','5'},{'d','6'},{'e','7'},{'f','8'},
    {'g','9'},{'h','!'},{'i','"'},{'j','#'},{'k','$'},{'l','%'},{'m','&'},
   {'n','\''},{'o','('},{'p',')'},{'q','*'},{'r','+'},{'s',','},{'t',';'},
    {'u','<'},{'v','='},{'w','>'},{'x','?'},{'y','@'},{'z','.'}
  };

  if (values.empty()) {
    return;
  }
  compressed_bitmap=strutils::itos(values.front())+":";
  auto alternate_compressed_bitmap=strutils::itos(values.front())+":";
  auto last_value=values.front();
  auto it=values.begin();
  auto end=values.end();
  ++it;
  size_t idx=0;
  int min_diff=0x7fffffff;
  for (; it != end; ++it) {
    int cnt=0,diff;
    while (it != end && (diff=*it-last_value) <= 1) {
      if (diff < 0 && diff < min_diff) {
        min_diff=diff;
      }
      if (*it != last_value) {
        ++cnt;
        last_value=*it;
      }
      ++it;
    }
    if (cnt > 0) {
      ++cnt;
      if (cnt > 4) {
        compressed_bitmap+="-"+strutils::itos(cnt)+"/"+BIT[idx];
      }
      else {
        for (int n=0; n < cnt; ++n) {
          compressed_bitmap+=BIT[idx];
        }
      }
      auto repeat=cnt/27;
      if (repeat > 4) {
        alternate_compressed_bitmap+="-"+strutils::itos(repeat)+"/"+ALIAS[idx][27];
      }
      else {
        for (int n=0; n < repeat; ++n) {
          alternate_compressed_bitmap+=ALIAS[idx][27];
        }
      }
      alternate_compressed_bitmap+=ALIAS[idx][(cnt % 27)];
      idx=1-idx;
    }
    else {
      compressed_bitmap+=BIT[idx];
      if (idx == 0 && ((alternate_compressed_bitmap.back() >= 'a' && alternate_compressed_bitmap.back() <= 'z') || alternate_compressed_bitmap.back() == '0')) {
        alternate_compressed_bitmap.back()=ZERO_ALIAS.at(alternate_compressed_bitmap.back());
      }
      else {
        alternate_compressed_bitmap+=BIT[idx];
      }
      idx=1-idx;
    }
    if (it != end) {
      cnt=*it-last_value-1;
      if (cnt >= 0) {
        if (cnt > 4) {
          compressed_bitmap+="-"+strutils::itos(cnt)+"/"+BIT[idx];
        }
        else {
          for (int n=0; n < cnt; ++n) {
            compressed_bitmap+=BIT[idx];
          }
        }
        auto repeat=cnt/27;
        if (repeat > 4) {
          alternate_compressed_bitmap+="-"+strutils::itos(repeat)+"/"+ALIAS[idx][27];
        }
        else {
          for (int n=0; n < repeat; ++n) {
            alternate_compressed_bitmap+=ALIAS[idx][27];
          }
        }
        alternate_compressed_bitmap+=ALIAS[idx][(cnt % 27)];
        idx=1-idx;
      }
      else {
        ++cnt;
        if (cnt < min_diff) {
          min_diff=cnt;
        }
      }
      last_value=*it;
    }
    else {
      break;
    }
  }
  if (min_diff < 0) {
    compress_non_monotonic_values(values,fabs(min_diff)+1,compressed_bitmap);
  }
  else {
    if (idx == 0) {
      compressed_bitmap+=BIT[idx];
      if ((alternate_compressed_bitmap.back() >= 'a' && alternate_compressed_bitmap.back() <= 'z') || alternate_compressed_bitmap.back() == '0') {
        alternate_compressed_bitmap.back()=ZERO_ALIAS.at(alternate_compressed_bitmap.back());
      }
      else {
        alternate_compressed_bitmap+=BIT[idx];
      }
    }
//std::cerr << compressed_bitmap << " (" << compressed_bitmap.length() << ")" << std::endl;
//std::cerr << alternate_compressed_bitmap << " (" << alternate_compressed_bitmap.length() << ")" << std::endl;
    string further_compressed_bitmap;
    if (compressed_bitmap.length() > 255) {
      further_compress(compressed_bitmap,further_compressed_bitmap);
//std::cerr << "*" << further_compressed_bitmap << " (" << further_compressed_bitmap.length() << ")" << std::endl;
      if (further_compressed_bitmap.length() > 255) {
        further_compress(alternate_compressed_bitmap,further_compressed_bitmap);
//std::cerr << "**" << further_compressed_bitmap << " (" << further_compressed_bitmap.length() << ")" << std::endl;
        compressed_bitmap= (further_compressed_bitmap.length() < alternate_compressed_bitmap.length()) ? std::move(further_compressed_bitmap) : std::move(alternate_compressed_bitmap);
      }
      else {
        compressed_bitmap=std::move(further_compressed_bitmap);
      }
    }
  }
}

void expand_box1d_bitmap(const string& box1d_bitmap, char
    *expanded_box1d_bitmap) {
  if (box1d_bitmap.length() != 120) {
    return;
  }
  for (size_t n = 0; n < 360; ++n) {
    expanded_box1d_bitmap[n] = '0';
  }
  for (size_t n = 0; n < box1d_bitmap.length(); ++n) {
    if (box1d_bitmap[n] >= '4') {
      expanded_box1d_bitmap[n*3] = '1';
    }
    if (string("2367").find(box1d_bitmap[n]) != string::npos) {
      expanded_box1d_bitmap[n*3+1] = '1';
    }
    if (string("1357").find(box1d_bitmap[n]) != string::npos) {
      expanded_box1d_bitmap[n*3+2] = '1';
    }
  }
}

void further_compress(const string& compressed_bitmap,string& further_compressed_bitmap)
{
  if (&compressed_bitmap == &further_compressed_bitmap) {
    return;
  }
  auto start=compressed_bitmap.find(":");
  if (start == string::npos) {
    further_compressed_bitmap=compressed_bitmap;
    return;
  }
  else {
    ++start;
  }
  further_compressed_bitmap=compressed_bitmap;
  auto idx=further_compressed_bitmap.find(":");
  if (idx != string::npos) {
    if (idx > 0) {
      ++idx;
    }
    else {
      idx=string::npos;
    }
  }
  if (idx != string::npos) {
    auto do_check=true;
    while (do_check) {
      do_check=false;
      auto groups=split(further_compressed_bitmap.substr(idx),"-");
      if (groups.size() > 0) {
        further_compressed_bitmap=further_compressed_bitmap.substr(0,idx);
        if (groups.front().empty()) {
          groups.pop_front();
        }
        string s1,s2;
        for (const auto& g : groups) {
          auto idx2=g.find("/");
          if (idx2 != string::npos) {
            ++idx2;
            if (g[idx2++] == '{') {
              idx2=g.find("}")+1;
            }
          }
          else {
            idx2=0;
          }
          if (g.length() > (idx2+4)) {
            auto g_length=g.length()-idx2;
            int max_check_len=g_length/2+1;
            s1.reserve(max_check_len);
            s2.reserve(max_check_len);
            int check_len=2;
            string best_new_g;
            while (check_len < max_check_len) {
              int curr_idx=0,last_idx=0;
              auto new_g=g.substr(idx2);
              int end_idx=new_g.length()-check_len*2;
              while (curr_idx <= end_idx) {
                s1=new_g.substr(curr_idx,check_len);
                s2=new_g.substr(curr_idx+check_len,check_len);
//std::cerr << "checking " << s1 << " against " << s2 << std::endl;
                while (s1 == s2) {
                  curr_idx+=check_len;
                  s2=new_g.substr(curr_idx+check_len,check_len);
                }
                if (curr_idx == last_idx) {
                  ++curr_idx;
                }
                else {
                  curr_idx+=check_len;
                  if ( (curr_idx-last_idx) > (check_len+5) ) {
//std::cerr << "replace from " << last_idx << " to " << (curr_idx-1) << " with -" << (curr_idx-last_idx)/check_len << "/{" << new_g.substr(last_idx,check_len) << "}" << std::endl;
                    string substitute="-"+std::to_string((curr_idx-last_idx)/check_len)+"/{"+new_g.substr(last_idx,check_len)+"}";
                    new_g=new_g.substr(0,last_idx)+substitute+new_g.substr(curr_idx);
                    end_idx=new_g.length()-check_len*2;
                    if (end_idx < 0) {
                      break;
                    }
                    curr_idx-=(curr_idx-last_idx-substitute.length());
                  }
                }
                last_idx=curr_idx;
              }
              if (new_g.length() < g_length && (new_g.length() < best_new_g.length() || best_new_g.empty())) {
                best_new_g=new_g;
              }
              ++check_len;
            }
            if (!best_new_g.empty()) {
              if (idx2 > 0) {
                further_compressed_bitmap+="-"+g.substr(0,idx2);
              }
              else if (best_new_g[0] != '-') {
                further_compressed_bitmap+="-";
              }
              further_compressed_bitmap+=best_new_g;
              do_check=true;
            }
            else {
              if (idx2 > 0) {
                further_compressed_bitmap+="-";
              }
              further_compressed_bitmap+=g;
            }
          }
          else if (!g.empty()) {
            if (&g != &groups.front()) {
              further_compressed_bitmap+="-";
            }
            further_compressed_bitmap+=g;
          }
        }
      }
    }
    auto groups=split(further_compressed_bitmap.substr(idx),"-");
    if (groups.size() > 0) {
      further_compressed_bitmap=further_compressed_bitmap.substr(0,idx);
      if (groups.front().empty()) {
        groups.pop_front();
      }
      auto last_group=groups.front();
      auto num_occurs=1;
      groups.pop_front();
      for (const auto& g : groups) {
        if (g == last_group) {
          ++num_occurs;
        }
        else if (!last_group.empty() && g.substr(0,last_group.length()) == last_group && strutils::occurs(last_group,"/") == strutils::occurs(g,"/")) {
          auto end=g.substr(last_group.length());
          further_compressed_bitmap+="-"+std::to_string(++num_occurs)+"/{"+last_group+"}"+g.substr(last_group.length());
          num_occurs=1;
          last_group="";
        }
        else {
          if (num_occurs > 1) {
            further_compressed_bitmap+="-"+std::to_string(num_occurs)+"/{"+last_group+"}";
            num_occurs=1;
          }
          else if (!last_group.empty()) {
            if (last_group.find("/") != string::npos) {
              further_compressed_bitmap+="-";
            }
            further_compressed_bitmap+=last_group;
          }
          last_group=g;
        }
      }
      if (num_occurs > 1) {
        further_compressed_bitmap+="-"+std::to_string(num_occurs)+"/{"+last_group+"}";
      }
      else if (!last_group.empty()) {
        further_compressed_bitmap+="-"+last_group;
      }
    }
  }
}

void uncompress(const string& compressed_bitmap,size_t& first_value,string& uncompressed_bitmap)
{
  auto start=compressed_bitmap.find(":");
  if (start == string::npos) {
    if (compressed_bitmap[0] == '!') {
      first_value = stoi(compressed_bitmap.substr(1));
      uncompressed_bitmap = "1";
    } else {
      first_value = 0;
      uncompressed_bitmap = "";
    }
    return;
  }
  uncompressed_bitmap.reserve(32768);
  first_value=stoi(compressed_bitmap.substr(0,start));
  auto groups=split(compressed_bitmap,"-");
  uncompressed_bitmap=groups.front().substr(start+1);
  groups.pop_front();
  for (const auto& g : groups) {
    auto idx=g.find("/");
    size_t num=stoi(g.substr(0,idx));
    string back=g.substr(idx+1);
    if (back[0] == '{') {
      if ( (idx=back.find("}")) != string::npos) {
        string repeat=back.substr(1,idx-1);
        auto s_idx=repeat.find("/");
        if (s_idx != string::npos) {
          repeat.insert(0,"0:-");
          size_t first_value;
          uncompress(repeat,first_value,repeat);
        }
        for (size_t n=0; n < num; ++n) {
          uncompressed_bitmap+=repeat;
        }
        ++idx;
        if (back.length() > idx) {
          uncompressed_bitmap+=back.substr(idx);
        }
      }
    }
    else {
      for (size_t n=0; n < num; ++n) {
        uncompressed_bitmap+=back.substr(0,1);
      }
      if (back.length() > 1) {
        uncompressed_bitmap+=back.substr(1);
      }
    }
  }
}

void decode_bit(char bit,vector<size_t>& values,size_t& next_value,size_t multiplier)
{
  switch (bit) {
    case '0': {
      ++next_value;
      break;
    }
    case '1': {
      values.emplace_back(next_value++);
      values.back()-=(multiplier*values.size());
      break;
    }
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z': {
      size_t num_vals=bit-63;
      for (size_t m=0; m < num_vals; ++m) {
        values.emplace_back(next_value++);
        values.back()-=(multiplier*values.size());
      }
      break;
    }
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z': {
      size_t skip=bit-95;
      next_value+=skip;
      break;
    }
// skip is 1-8 then '1'
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      size_t skip=bit-49;
      next_value+=skip;
      values.emplace_back(next_value++);
      values.back()-=(multiplier*values.size());
      break;
    }
// skip is 9-20 then '1'
    case '!':
    case '"':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',': {
      size_t skip=bit-24;
      next_value+=skip;
      values.emplace_back(next_value++);
      values.back()-=(multiplier*values.size());
      break;
    }
// skip is 21-26 then '1'
    case ';':
    case '<':
    case '=':
    case '>':
    case '?':
    case '@': {
      size_t skip=bit-38;
      next_value+=skip;
      values.emplace_back(next_value++);
      values.back()-=(multiplier*values.size());
      break;
    }
// skip is 0 then '1'
    case '.': {
      size_t skip=bit-46;
      next_value+=skip;
      values.emplace_back(next_value++);
      values.back()-=(multiplier*values.size());
      break;
    }
    default: {
      throw std::runtime_error("decode_bit(): invalid value bit '"+string(bit,1)+"'");
    }
  }
}

void decode_group(string group,vector<size_t>& values,size_t& next_value,size_t multiplier)
{
  auto parts=split(group,"/");
  size_t repeat=1;
  switch (parts.size()) {
    case 1: {
      break;
    }
    case 2: {
      repeat=stoi(parts[0]);
      parts.pop_front();
      break;
    }
    default: {
      throw std::runtime_error("decode_group(): malformed bitmap group '"+group+"'");
    }
  }
  for (size_t n=0; n < repeat; ++n) {
    decode_bit(parts[0][0],values,next_value,multiplier);
  }
  for (size_t n=1; n < parts[0].length(); ++n) {
    decode_bit(parts[0][n],values,next_value,multiplier);
  }
}

void uncompress_values(const string& compressed_bitmap, vector<size_t>&
    values) {
  values.clear();
  size_t start = compressed_bitmap.find(":");
  if (start == string::npos) {
    if (compressed_bitmap[0] == '!') {
      auto vlist = split(compressed_bitmap.substr(1), ",");
      for (size_t n = 0; n < vlist.size(); ++n) {
        values.emplace_back(stoi(vlist[n]));
      }
    }
    return;
  }
  auto sparts = split(compressed_bitmap.substr(0, start),
      NON_MONOTONIC_DELIMITER);
  size_t next_value = stoi(sparts[0]);
  size_t multiplier;
  if (sparts.size() > 1) {
    multiplier = stoi(sparts[1]);
  } else {
    multiplier = 0;
  }
  auto sections = split(compressed_bitmap, "-");
  auto s0 = sections[0];
  if (!has_ending(s0, ":")) {
    s0 = s0.substr(start+1);
    for (size_t n = 0; n < s0.length(); ++n) {
      decode_bit(s0[n], values, next_value, multiplier);
    }
  }
  for (size_t n = 1; n < sections.size(); ++n) {
    auto parts = split(sections[n], "/");
    if (parts.size() == 3) {
      parts[1] += "/" + parts[2];
    } else if (parts.size() != 2) {
      throw std::runtime_error("uncompress_values(): malformed bitmap group "
          "in '" + compressed_bitmap + "'");
    }
    auto repeat = stoi(parts[0]);
    string bitmap_section;
    if (parts[1][0] == '{') {
      auto idx = parts[1].find("}");
      if (idx == string::npos) {
        throw std::runtime_error("uncompress_values(): missing end-of-group "
            "delimiter in '" + compressed_bitmap + "'");
      }
      bitmap_section = parts[1].substr(1, idx-1);
      for (int m = 0; m < repeat; ++m) {
        decode_group(bitmap_section, values, next_value, multiplier);
      }
      bitmap_section = parts[1].substr(idx);
    } else {
      for (int m = 0; m < repeat; ++m) {
        decode_bit(parts[1][0], values, next_value, multiplier);
      }
      bitmap_section = parts[1];
    }
    if (bitmap_section.length() > 1) {
      for (size_t m = 1; m < bitmap_section.length(); ++m) {
        decode_bit(bitmap_section[m], values, next_value, multiplier);
      }
    }
  }
}

string add_box1d_bitmaps(const string& compressed_box1d_bitmap1,const string& compressed_box1d_bitmap2)
{
  if (compressed_box1d_bitmap1.length() != 120 || compressed_box1d_bitmap2.length() != 120) {
    return "";
  }
  char combined[120];
  for (size_t n=0; n < compressed_box1d_bitmap1.length(); ++n) {
    auto c1=compressed_box1d_bitmap1[n];
    auto c2=compressed_box1d_bitmap2[n];
    if (c1 == c2) {
      combined[n]=c1;
    }
    else if (c1 == '0' || c2 == '0') {
      combined[n]= (c1 == '0') ? c2 : c1;
    }
    else if ((c1 >= '5' && c2 >= '5') || (c1 == '3' && c2 > '3') || (c2 == '3' && c1 > '3')) {
      combined[n]='7';
    }
    else if ((c1 == '4' && c2 > '4') || (c2 == '4' && c1 > '4')) {
      combined[n]= (c1 > '4') ? c1 : c2; 
    }
    else if (c1 == '1') {
      combined[n]= ((c2 % 2) == 0) ? static_cast<char>(c2+1) : c2;
    }
    else if (c2 == '1') {
      combined[n]= ((c1 % 2) == 0) ? static_cast<char>(c1+1) : c1;
    }
// one character is a '2' and the other is > '2'
    else {
      combined[n]= (c1 > c2) ? c1 : c2;
      if (combined[n] == '4' || combined[n] == '5') {
        combined[n]+=2;
      }
    }
  }
  return string(combined,120);
}

bool contains_value(const string& compressed_bitmap,size_t value_to_find)
{
  auto start=compressed_bitmap.find(":");
  if (start == string::npos) {
    if (compressed_bitmap[0] == '!') {
      auto groups=split(compressed_bitmap.substr(1),",");
      for (size_t n=0; n < groups.size(); ++n) {
        if (std::stoul(groups[n]) == value_to_find) {
          return true;
        }
      }
      return false;
    }
    else {
      return false;
    }
  }
  else {
    size_t value=stoi(compressed_bitmap.substr(0,start));
    if (value == value_to_find) {
      return true;
    }
    else if (value > value_to_find) {
      return false;
    }
    else {
      auto groups=split(compressed_bitmap,"-");
      auto first_group=groups[0];
      if (!has_ending(first_group,":")) {
        first_group=first_group.substr(start+1);
        for (size_t n=0; n < first_group.length(); ++n) {
          if (first_group[n] == '1') {
            if (value++ == value_to_find) {
              return true;
            }
          }
          else if (first_group[n] == '0') {
            value++;
          }
          else if (first_group[n] >= 'A' && first_group[n] <= 'Z') {
            auto cnt=first_group[n]-63;
            for (int m=0; m < cnt; m++) {
              if (value++ == value_to_find) {
                return true;
              }
            }
          }
          else if (first_group[n] >= 'a' && first_group[n] <= 'z') {
            auto cnt=first_group[n]-95;
            for (int m=0; m < cnt; m++) {
              value++;
            }
          }
        }
      }
      for (size_t n=1; n < groups.size(); ++n) {
        auto g_parts=split(groups[n],"/");
        auto cnt=stoi(g_parts[0]);
        if (g_parts[1][0] == '{') {
          auto idx=g_parts[1].find("}");
          auto repeated_group=g_parts[1].substr(1,idx-1);
          for (int m=0; m < cnt; ++m) {
            for (size_t l=0; l < repeated_group.length(); ++l) {
              if (repeated_group[l] == '1') {
                if (value == value_to_find) {
                  return true;
                }
              }
              value++;
            }
          }
          g_parts[1]=g_parts[1].substr(idx);
        }
        else {
          if (g_parts[1][0] == '1') {
            for (int m=0; m < cnt; ++m) {
              if ( (value+m) == value_to_find) {
                return true;
              }
            }
          }
          value+=cnt;
        }
        if (g_parts[1].length() > 1) {
          for (size_t m=1; m < g_parts[1].length(); ++m) {
            if (g_parts[1][m] == '1') {
              if (value == value_to_find) {
                return true;
              }
            }
            value++;
          }
        }
      }
    }
    return false;
  }
}

namespace longitudeBitmap {

void west_east_bounds(const float *longitude_bitmap,size_t& west_index,size_t& east_index)
{
  bitmap_gap current,biggest;
  size_t n=0,m=0;
  for (; n < 720; ++n,++m) {
    if (m == 360) {
      m=0;
    }
    if (longitude_bitmap[m] > 900.) {
      if (current.start < 0) {
        current.start=n;
      }
      current.end=n;
      ++current.length;
    }
    else {
      if (current.length > biggest.length) {
        biggest=current;
      }
      current.start=-1;
      current.length=0;
    }
  }
  if (biggest.length == 0) {
    biggest=current;
  }
  if (biggest.start < 0) {
    biggest.start+=360;
  }
  else {
    --biggest.start;
    if (biggest.start > 359) {
      biggest.start-=360;
    }
    else if (biggest.start < 0) {
      biggest.start+=360;
    }
  }
  ++biggest.end;
  if (biggest.end < 0) {
    biggest.end+=360;
  }
  else if (biggest.end > 359) {
    biggest.end-=360;
  }
  west_index=biggest.end;
  east_index=biggest.start;
}

} // end namespace longitudeBitmap

} // end namespace bitmap
