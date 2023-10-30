#include <stdlib.h>
#include <complex>
#include <utils.hpp>

namespace xtox {

long long htoi(std::string hex_string)
{
  long long ival=0;
  for (size_t n=0; n < hex_string.length(); ++n) {
    long long hval;
    switch (hex_string[n]) {
	case 'a':
	case 'A': {
	  hval=10;
	  break;
	}
	case 'b':
	case 'B': {
	  hval=11;
	  break;
	}
	case 'c':
	case 'C': {
	  hval=12;
	  break;
	}
	case 'd':
	case 'D': {
	  hval=13;
	  break;
	}
	case 'e':
	case 'E': {
	  hval=14;
	  break;
	}
	case 'f':
	case 'F': {
	  hval=15;
	  break;
	}
	default: {
	  hval=(hex_string[n]-48);
	}
    }
    ival+=hval*pow(16.,hex_string.length()-n-1);
  }
  return ival;
}

char *lltoh(long long lval)
{
  int n,num_chars=0;
  long long power,ch;
  size_t cnt;
  char *string;

  while (lval >= pow(16.,num_chars)) {
    ++num_chars;
  }
  string=new char[num_chars+3];
  string[0]='0';
  string[1]='x';
  string[num_chars+2]='\0';
  cnt=2;
  for (n=num_chars-1; n >= 0; n--) {
    power=pow(16.,n);
    ch=lval/power;
    switch (ch) {
	case 10:
	{
	  string[cnt]='a';
	  break;
	}
	case 11:
	{
	  string[cnt]='b';
	  break;
	}
	case 12:
	{
	  string[cnt]='c';
	  break;
	}
	case 13:
	{
	  string[cnt]='d';
	  break;
	}
	case 14:
	{
	  string[cnt]='e';
	  break;
	}
	case 15:
	{
	  string[cnt]='f';
	  break;
	}
	default:
	{
//	  string[cnt]=(int)ch+48;
string[cnt]=ch+'0';
	}
    }
    lval-=ch*power;
    ++cnt;
  }
  return string;
}

char *itoh(int ival)
{
  return lltoh(ival);
}

void convert_octal(long long octal,long long& lval,int power = 0)
{
  if (octal > 7) {
    convert_octal(octal/10,lval,power+1);
  }
  lval+=(octal % 10)*pow(8.,power);
}

int otoi(int octal)
{
  long long lval=0;

  convert_octal(octal,lval);
  return static_cast<int>(lval);
}

long long otoll(long long octal)
{
  long long lval=0;

  convert_octal(octal,lval);
  return lval;
}

long long itoo(int ival)
{
  int n;
  long long oval=0;
  int max_power=0;
  size_t power,i;

  while (ival >= pow(8.,max_power)) {
    ++max_power;
  }
  for (n=max_power-1; n >= 0; --n) {
    power=pow(8.,n);
    i=ival/power;
    oval+=(i*pow(10.,n));
    ival-=i*power;
  }
  return oval;
}

long long ltoo(long long lval)
{
  long long oval=0,l;
  int n;
  int max_power=0;
  size_t power;

  while (lval >= pow(8.,max_power)) {
    max_power++;
  }
  for (n=max_power-1; n >= 0; --n) {
    power=pow(8.,n);
    l=lval/power;
    oval+=(l*pow(10.,n));
    lval-=l*power;
  }
  return oval;
}

} // end namespace xtox
