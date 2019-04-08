#include <string>
#include <s3.hpp>

namespace s3 {

Object::Object(std::string key,std::string last_modified,std::string etag,long long size,std::string owner_name,std::string storage_class) : key_(),last_modified_(),etag_(),size_(),owner_name_(),storage_class_()
{
  reset(key,last_modified,etag,size,owner_name,storage_class);
}

void Object::reset(std::string key,std::string last_modified,std::string etag,long long size,std::string owner_name,std::string storage_class)
{
  key_=key;
  last_modified_=last_modified;
  etag_=etag;
  size_=size;
  owner_name_=owner_name;
  storage_class_=storage_class;
}

void Object::show() const
{
  std::cout << "Object: " << key_ << std::endl;
  std::cout << "  Last Modified: " << last_modified_ << std::endl;
  std::cout << "  ETag: " << etag_ << std::endl;
  std::cout << "  Size: " << size_ << std::endl;
  std::cout << "  Owner: " << owner_name_ << std::endl;
  std::cout << "  Storage Class: " << storage_class_ << std::endl;
}

} // end namespace s3
