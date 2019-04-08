#include <string>
#include <s3.hpp>

namespace s3 {

Bucket::Bucket(std::string name) : name_()
{
  reset(name);
}

void Bucket::reset(std::string name)
{
  name_=name;
}

} // end namespace s3
