#include <bits.hpp>

extern "C" {

void gbyte_(long long *buffer,long long& loc,long long& offset,long long& bits)
{
  bits::get(buffer,loc,offset,bits);
}

void gbytes_(long long *buffer,long long *loc,long long& offset,long long& bits,long long& skip,long long& num)
{
  bits::get(buffer,loc,offset,bits,skip,num);
}

}
