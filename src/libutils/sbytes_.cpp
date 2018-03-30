#include <bits.hpp>

extern "C" {

void sbyte_(long long *buffer,long long& loc,long long& offset,long long& bits)
{
  bits::set(buffer,loc,offset,bits);
}

void sbytes_(long long *buffer,long long *loc,long long& offset,long long& bits,long long& skip,long long& num)
{
  bits::set(buffer,loc,offset,bits,skip,num);
}

}
