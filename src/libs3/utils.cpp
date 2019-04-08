#include <iostream>
#include <iomanip>
#include <sstream>

namespace s3 {

void show_hex(std::ostream& outs,const unsigned char *s,size_t len)
{
  outs << std::setfill('0') << std::hex;
  for (size_t n=0; n < len; ++n) {
    outs << std::setw(2) << static_cast<int>(s[n]);
  }
  outs << std::dec;
}

std::string uhash_to_string(const unsigned char *uhash,size_t len)
{
  std::stringstream hash_ss;
  show_hex(hash_ss,uhash,len);
  return hash_ss.str();
}

} // end namespace s3
