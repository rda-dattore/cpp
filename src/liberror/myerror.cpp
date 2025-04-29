#include <iostream>
#include <cstdlib>
#include <myerror.hpp>

using std::cerr;
using std::cout;
using std::endl;

void print_myerror() {
  if (!myerror.empty()) {
    cerr << "Error: " << myerror << endl;
  }
}

void print_mywarning() {
  if (!mywarning.empty()) {
    cout << "Warning: " << mywarning << endl;
  }
}

void print_myoutput() {
  if (!myoutput.empty()) {
    cout << "Output: " << myoutput << endl;
  }
}
