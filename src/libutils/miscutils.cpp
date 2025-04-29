#include <utils.hpp>

using std::move;
using std::string;

namespace miscutils {

size_t min_byte_width(size_t value) {
  if (value == 0) {
    return 1;
  }
  size_t num_bytes = 0;
  while (value > 0) {
    ++num_bytes;
    value /= 256;
  }
  return num_bytes;
}

string this_function_label(string function_name) {
  return move(string(function_name + "()"));
}

} // end namespace miscutils
