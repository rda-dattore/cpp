#include <fstream>
#include <memory>
#include <vector>
#include <json.hpp>
#include <strutils.hpp>
#include <myerror.hpp>

JSON::ValueBase::operator bool() const
{
  if (_type == ValueType::boolean) {
    return reinterpret_cast<Value<bool> *>(const_cast<ValueBase *>(this))->_data;
  }
  else {
    return false;
  }
}

std::ostream& operator<<(std::ostream& o,const JSON::ValueBase& v)
{
  v.print(o);
  return o;
}

bool operator==(const std::string& s,const JSON::ValueBase& v)
{
  return (v == s);
}

bool operator==(const JSON::ValueBase& v,const std::string& s)
{
  if (v._type == JSON::ValueType::string) {
    if (reinterpret_cast<JSON::Value<std::string> *>(const_cast<JSON::ValueBase *>(&v))->_data == s) {
	return true;
    }
    else {
	return false;
    }
  }
  else {
    return false;
  }
}

bool operator==(const int& i,const JSON::ValueBase& v)
{
  return (v == i);
}

bool operator==(const JSON::ValueBase& v,const int& i)
{
  if (v._type == JSON::ValueType::number) {
    if (reinterpret_cast<JSON::Value<int> *>(const_cast<JSON::ValueBase *>(&v))->_data == i) {
	return true;
    }
    else {
	return false;
    }
  }
  else {
    return false;
  }
}

template <class T>
std::vector<std::string> JSON::Value<T>::keys() const
{
  return _data.keys();
}

template <class T>
size_t JSON::Value<T>::size() const
{
  return _data.size();
}

template <class T>
std::string JSON::Value<T>::to_string() const
{
  return _data.to_string();
}

template <class T>
void JSON::Value<T>::print(std::ostream& o) const
{
  o << _data;
}

void JSON::Object::clear()
{
  for (auto& e : pairs) {
    delete e.second;
  }
  pairs.clear();
}

JSON::Object::~Object()
{
  this->clear();
}

void JSON::Object::fill(std::string json_object)
{
  this->clear();
  strutils::trim(json_object);
  if (json_object.front() != '{' || json_object.back() != '}') {
    myerror="not a JSON object";
    return;
  }
  json_object.erase(0,1);
  json_object.pop_back();
  if (!json_object.empty()) {
    std::vector<size_t> csv_ends;
    JSONUtils::find_csv_ends(json_object,csv_ends);
    size_t next_start=0;
    for (auto end : csv_ends) {
	auto pair=json_object.substr(next_start,end-next_start);
	auto in_quotes=0;
	size_t idx=0;
	for (size_t n=0; n < pair.length(); ++n) {
	  switch (pair[n]) {
	    case '"':
	    {
		in_quotes=1-in_quotes;
		break;
	    }
	    case ':':
	    {
		idx=n;
		n=pair.length();
		break;
	    }
	    default:
	    {}
	  }
	}
	auto key=pair.substr(0,idx);
	strutils::trim(key);
	if (key.front() != '"' || key.back() != '"') {
	  myerror="invalid key '"+key+"'";
	  pairs.clear();
	  return;
	}
	key.erase(0,1);
	key.pop_back();
	auto value=pair.substr(idx+1);
	strutils::trim(value);
	if (pairs.find(key) == pairs.end()) {
	  if (value == "null") {
	    auto p=new Value<Null>(Null(),ValueType::null);
	    pairs.emplace(key,p);
	  }
	  else if (value == "true") {
	    auto p=new Value<Boolean>(Boolean(true),ValueType::boolean);
	    pairs.emplace(key,p);
	  }
	  else if (value == "false") {
	    auto p=new Value<Boolean>(Boolean(false),ValueType::boolean);
	    pairs.emplace(key,p);
	  }
	  else if (value.front() == '"' && value.back() == '"') {
	    auto p=new Value<String>(String(value.substr(1,value.length()-2)),ValueType::string);
	    pairs.emplace(key,p);
	  }
	  else if (value.front() == '{' && value.back() == '}') {
	    auto o=new Object(value);
	    auto p=new Value<Object>(*o,ValueType::object);
	    pairs.emplace(key,p);
	  }
	  else if (value.front() == '[' && value.back() == ']') {
	    auto a=new Array(value);
	    auto p=new Value<Array>(*a,ValueType::array);
	    pairs.emplace(key,p);
	  }
	  else {
	    auto p=new Value<Number>(Number(std::stoi(value)),ValueType::number);
	    pairs.emplace(key,p);
	  }
	}
	else {
	}
	next_start=++end;
    }
  }
}

void JSON::Object::fill(std::ifstream& ifs)
{
  if (!ifs.is_open()) {
    myerror="unable to open file";
    return;
  }
  ifs.seekg(0,std::ios::end);
  auto length=ifs.tellg();
  std::unique_ptr<char[]> buffer(new char[length]);
  ifs.seekg(0,std::ios::beg);
  ifs.read(buffer.get(),length);
  std::string json(buffer.get(),length);
  fill(json);
}

std::vector<std::string> JSON::Object::keys() const
{
  std::vector<std::string> keys;
  for (const auto& e : pairs) {
    keys.emplace_back(e.first);
  }
  return keys;
}

std::ostream& operator<<(std::ostream& o,const JSON::String& str)
{
  o << str.s;
  return o;
}

std::ostream& operator<<(std::ostream& o,const JSON::Number& num)
{
  o << num.i;
  return o;
}

std::ostream& operator<<(std::ostream& o,const JSON::Object& obj)
{
  o << "[JSON::Object]";
  return o;
}

std::ostream& operator<<(std::ostream& o,const JSON::Array& arr)
{
  o << "[JSON::Array]";
  return o;
}

std::ostream& operator<<(std::ostream& o,const JSON::Boolean& b)
{
  if (b.b) {
    o << "true";
  }
  else {
    o << "false";
  }
  return o;
}

std::ostream& operator<<(std::ostream& o,const JSON::Null& n)
{
  return o;
}

const JSON::ValueBase* JSON::Object::operator[](const char* key) const
{
  return (*this)[std::string(key)];
}

const JSON::ValueBase* JSON::Object::operator[](std::string key) const
{
  auto iter=pairs.find(key);
  if (iter != pairs.end()) {
    return iter->second;
  }
  else {
    return nullptr;
  }
}

void JSON::Array::clear()
{
  for (auto& e : elements) {
    delete e;
  }
  elements.clear();
}

JSON::Array::~Array()
{
  this->clear();
}

void JSON::Array::fill(std::string json_array)
{
  this->clear();
  strutils::trim(json_array);
  if (json_array.front() != '[' || json_array.back() != ']') {
    myerror="not a JSON array";
    return;
  }
  json_array.erase(0,1);
  json_array.pop_back();
  if (!json_array.empty()) {
    std::vector<size_t> csv_ends;
    JSONUtils::find_csv_ends(json_array,csv_ends);
    size_t next_start=0;
    for (auto end : csv_ends) {
	auto array_value=json_array.substr(next_start,end-next_start);
	strutils::trim(array_value);
	if (array_value == "null") {
	  auto p=new Value<Null>(Null(),ValueType::null);
	  elements.emplace_back(p);
	}
	else if (array_value == "true") {
	  auto p=new Value<Boolean>(Boolean(true),ValueType::boolean);
	  elements.emplace_back(p);
	}
	else if (array_value == "false") {
	  auto p=new Value<Boolean>(Boolean(false),ValueType::boolean);
	  elements.emplace_back(p);
	}
	else if (array_value.front() == '"' && array_value.back() == '"') {
	  auto p=new Value<String>(String(array_value.substr(1,array_value.length()-2)),ValueType::string);
	  elements.emplace_back(p);
	}
	else if (array_value.front() == '{' && array_value.back() == '}') {
	  auto o=new Object(array_value);
	  auto p=new Value<Object>(*o,ValueType::object);
	  elements.emplace_back(p);
	}
	else if (array_value.front() == '[' && array_value.back() == ']') {
	  auto a=new Array(array_value);
	  auto p=new Value<Array>(*a,ValueType::array);
	  elements.emplace_back(p);
	}
	else {
	  auto p=new Value<Number>(Number(std::stoi(array_value)),ValueType::number);
	  elements.emplace_back(p);
	}
	next_start=++end;
    }
  }
}
