#include <iostream>
#include <string>

void web_error(std::string message,bool print_content_type)
{
  if (print_content_type) {
    std::cout << "Access-Control-Allow-Origin: https://rda.ucar.edu" << std::endl;
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
  }
  std::cout << "Error: " << message << std::endl;
  exit(1);
}

