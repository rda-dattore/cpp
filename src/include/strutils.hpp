#ifndef STRUTILS_HPP
#define   STRUTILS_HPP

#include <string>
#include <deque>
#include <vector>
#include <complex>

namespace strutils {

extern void chop(std::string& s,size_t num_chars = 1);
extern void convert_unicode(std::string& s);
extern void replace_all(std::string& s,const std::string& old_s,const std::string& new_s);
extern void strput(char *string,int numeric,int num_chars,char fill = ' ',bool is_signed = false);
extern void trim_back(std::string& s);
extern void trim_front(std::string& s);
extern void trim(std::string& s);
extern void unquote(std::string& s);

extern bool contains(const std::string& s,const std::string& sub_s);
extern bool has_beginning(const std::string& s,const std::string& beginning);
extern bool has_ending(const std::string& s,const std::string& ending);
extern bool has_no_letters(const std::string& s);
extern bool is_alpha(const std::string& s);
extern bool is_alphanumeric(const std::string& s);
extern bool is_numeric(const std::string& s);

extern size_t occurs(const std::string& s,const std::string& find_s);

extern std::deque<std::string> split(const std::string& s,const std::string& separator = "");
extern std::vector<std::pair<std::string,size_t>> split_index(const std::string& s,const std::string& separator = "");

extern std::string btos(bool b);
extern std::string capitalize(const std::string& s,size_t start_word = 0,size_t num_words = 0xffffffff);
extern std::string dtos(double val,size_t max_d = 0);
extern std::string dtos(double val,size_t w,size_t d,char fill = ' ');
extern std::string ftos(float val,size_t max_d = 0);
extern std::string ftos(float val,size_t w,size_t d,char fill = ' ');
extern std::string get_env(const std::string& name);
extern std::string itos(int val);
extern std::string lltos(long long val,size_t w = 0,char fill = ' ');
extern std::string number_with_commas(std::string s);
extern std::string number_with_commas(long long l);
extern std::string soundex(const std::string& s);
extern std::string sql_ready(std::string s);
extern std::string strand(size_t length);
extern std::string substitute(const std::string& s,const std::string& old_s,const std::string& new_s);
extern std::string to_capital(const std::string& s);
extern std::string to_lower(const std::string& s);
extern std::string to_title(const std::string& s);
extern std::string to_upper(const std::string& s,size_t start,size_t num_chars);
extern std::string to_upper(const std::string& s);
extern std::string token(const std::string& s,const std::string& separator,size_t token_number);
extern std::string uuid_gen();
extern std::string wrap(const std::string& s,size_t wrap_width = 80,size_t indent_width = 0);

template <class Type>
void strget(const char *str,Type& numeric_val,size_t num_chars)
{
  std::string cval(str,num_chars);
  char *chr=const_cast<char *>(cval.c_str());
  trim(cval);
  size_t decimal;
  num_chars=decimal=cval.length();
  numeric_val=0;
  int exp=0;
  bool neg_val=false,neg_exp=false,reading_exponent=false;
  for (size_t n=0; n < num_chars; ++n) {
    if (chr[n] == '.') {
	numeric_val*=0.1;
	decimal=n;
    }
    else if (chr[n] == 'E') {
	reading_exponent=true;
    }
    else {
	if (chr[n] == ' ' || chr[n] == '+' || chr[n] == '-') {
	  if (chr[n] == '-') {
	    if (!reading_exponent) {
		neg_val=true;
	    }
	    else {
		neg_exp=true;
	    }
	  }
	  chr[n]='0';
	}
	if (!reading_exponent) {
	  numeric_val+=static_cast<Type>((chr[n]-48)*std::pow(10.,num_chars-n-1));
	}
	else {
	  exp+=static_cast<int>((chr[n]-48)*std::pow(10.,num_chars-n-1));
	}
    }
  }
  if (neg_val) {
    numeric_val=-numeric_val;
  }
  if (decimal != num_chars) {
    auto mult=pow(0.1,num_chars-decimal-1);
    numeric_val*=static_cast<Type>(mult);
  }
  if (reading_exponent) {
    if (neg_exp) {
	exp=-exp;
    }
    numeric_val*=static_cast<Type>(pow(10.,exp));
  }
}

} // end namespace strutils

#endif
