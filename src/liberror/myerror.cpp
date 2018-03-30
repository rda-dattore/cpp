#include <iostream>
#include <cstdlib>
#include <myerror.hpp>

void print_myerror()
{
  if (!mywarning.empty()) {
    std::cerr << "Warning: " << mywarning << std::endl;
  }
  if (!myerror.empty()) {
    std::cerr << "Error: " << myerror << std::endl;
  }
}
