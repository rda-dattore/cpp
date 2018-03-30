#include <stdlib.h>
#include <complex>
#include <utils.hpp>

namespace xtox {

long long hex2long(std::string hex_string,int num_chars)
{
  static long long ival=0;
  static int length=num_chars;
  long long add=0;

  if (num_chars > 1) {
    hex2long(hex_string,num_chars-1);
  }
  switch (hex_string[num_chars-1]) {
    case 'a':
    case 'A':
    {
	add=10*pow(16.,length-num_chars);
	break;
    }
    case 'b':
    case 'B':
    {
	add=11*pow(16.,length-num_chars);
	break;
    }
    case 'c':
    case 'C':
    {
	add=12*pow(16.,length-num_chars);
	break;
    }
    case 'd':
    case 'D':
    {
	add=13*pow(16.,length-num_chars);
	break;
    }
    case 'e':
    case 'E':
    {
	add=14*pow(16.,length-num_chars);
	break;
    }
    case 'f':
    case 'F':
    {
	add=15*pow(16.,length-num_chars);
	break;
    }
    default:
    {
	add=(hex_string[num_chars-1]-48)*pow(16.,length-num_chars);
    }
  }
  ival+=add;
  return ival;
}

long long htoi(std::string hex_string)
{
  return hex2long(hex_string,hex_string.length());
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
