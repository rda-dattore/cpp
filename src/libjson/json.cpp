#include <fstream>
#include <memory>
#include <vector>
#include <json.hpp>
#include <strutils.hpp>
#include <myerror.hpp>

std::vector<std::string> JSON::Value::keys() const
{
  switch (_type) {
    case ValueType::Object:
    {
	return reinterpret_cast<Object *>(_content)->keys();
    }
    default:
    {
	return std::vector<std::string>{};
    }
  }
}

size_t JSON::Value::size() const
{
  switch (_type) {
    case ValueType::String:
    case ValueType::Number:
    case ValueType::Boolean:
    {
	return 1;
    }
    case ValueType::Object:
    {
	return reinterpret_cast<Object *>(_content)->size();
    }
    case ValueType::Array:
    {
	return reinterpret_cast<Array *>(_content)->size();
    }
    default:
    {
	return 0;
    }
  }
}

std::string JSON::Value::to_string() const
{
  switch (_type) {
    case ValueType::String:
    {
	return reinterpret_cast<String *>(_content)->to_string();
    }
    case ValueType::Number:
    {
	return reinterpret_cast<Number *>(_content)->to_string();
    }
    case ValueType::Object:
    {
	return reinterpret_cast<Object *>(_content)->to_string();
    }
    case ValueType::Array:
    {
	return reinterpret_cast<Array *>(_content)->to_string();
    }
    case ValueType::Boolean:
    {
	return reinterpret_cast<Boolean *>(_content)->to_string();
    }
    default:
    {
	return "";
    }
  }
}

const JSON::Value& JSON::Value::operator[](const char *key) const
{
  return (*this)[std::string(key)];
}

const JSON::Value& JSON::Value::operator[](std::string key) const
{
  static const void *n=new Null();
  static const Value nonexistent(ValueType::Nonexistent,const_cast<void *>(n));
  switch (_type) {
    case ValueType::Object:
    {
	return (*(reinterpret_cast<Object *>(_content)))[key];
    }
    default:
    {
	return nonexistent;
    }
  }
}

bool JSON::Value::operator>(int i) const
{
  if (_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(_content)->number() > i);
  }
  else {
    return false;
  }
}

bool JSON::Value::operator<(int i) const
{
  if (_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(_content)->number() < i);
  }
  else {
    return false;
  }
}

bool JSON::Value::operator==(int i) const
{
  if (_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(_content)->number() == i);
  }
  else {
    return false;
  }
}

bool JSON::Value::operator>=(int i) const
{
  if (_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(_content)->number() >= i);
  }
  else {
    return false;
  }
}

bool JSON::Value::operator<=(int i) const
{
  if (_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(_content)->number() <= i);
  }
  else {
    return false;
  }
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
	    auto n=new Null();
	    auto p=new Value(ValueType::Null,n);
	    pairs.emplace(key,p);
	  }
	  else if (value == "true") {
	    auto b=new Boolean(true);
	    auto p=new Value(ValueType::Boolean,b);
	    pairs.emplace(key,p);
	  }
	  else if (value == "false") {
	    auto b=new Boolean(false);
	    auto p=new Value(ValueType::Boolean,b);
	    pairs.emplace(key,p);
	  }
	  else if (value.front() == '"' && value.back() == '"') {
	    auto s=new String(value.substr(1,value.length()-2));
	    auto p=new Value(ValueType::String,s);
	    pairs.emplace(key,p);
	  }
	  else if (value.front() == '{' && value.back() == '}') {
	    auto o=new Object(value);
	    auto p=new Value(ValueType::Object,o);
	    pairs.emplace(key,p);
	  }
	  else if (value.front() == '[' && value.back() == ']') {
	    auto a=new Array(value);
	    auto p=new Value(ValueType::Array,a);
	    pairs.emplace(key,p);
	  }
	  else {
	    auto n=new Number(std::stoi(value));
	    auto p=new Value(ValueType::Number,n);
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

std::ostream& operator<<(std::ostream& o,const JSON::Value& v)
{
  switch (v._type) {
    case JSON::ValueType::String:
    {
	o << *(reinterpret_cast<JSON::String *>(v._content));
	break;
    }
    case JSON::ValueType::Number:
    {
	o << *(reinterpret_cast<JSON::Number *>(v._content));
	break;
    }
    case JSON::ValueType::Object:
    {
	o << *(reinterpret_cast<JSON::Object *>(v._content));
	break;
    }
    case JSON::ValueType::Array:
    {
	o << *(reinterpret_cast<JSON::Array *>(v._content));
	break;
    }
    case JSON::ValueType::Boolean:
    {
	o << *(reinterpret_cast<JSON::Boolean *>(v._content));
	break;
    }
    default:
    {}
  }
  return o;
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

const JSON::Value& JSON::Object::operator[](const char* key) const
{
  return (*this)[std::string(key)];
}

const JSON::Value& JSON::Object::operator[](std::string key) const
{
  static const void *n=new Null();
  static const Value nonexistent(ValueType::Nonexistent,const_cast<void *>(n));
  auto iter=pairs.find(key);
  if (iter != pairs.end()) {
    return *iter->second;
  }
  else {
    return nonexistent;
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
	  auto n=new Null();
	  auto p=new Value(ValueType::Null,n);
	  elements.emplace_back(p);
	}
	else if (array_value == "true") {
	  auto b=new Boolean(true);
	  auto p=new Value(ValueType::Boolean,b);
	  elements.emplace_back(p);
	}
	else if (array_value == "false") {
	  auto b=new Boolean(false);
	  auto p=new Value(ValueType::Boolean,b);
	  elements.emplace_back(p);
	}
	else if (array_value.front() == '"' && array_value.back() == '"') {
	  auto s=new String(array_value.substr(1,array_value.length()-2));
	  auto p=new Value(ValueType::String,s);
	  elements.emplace_back(p);
	}
	else if (array_value.front() == '{' && array_value.back() == '}') {
	  auto o=new Object(array_value);
	  auto p=new Value(ValueType::Object,o);
	  elements.emplace_back(p);
	}
	else if (array_value.front() == '[' && array_value.back() == ']') {
	  auto a=new Array(array_value);
	  auto p=new Value(ValueType::Array,a);
	  elements.emplace_back(p);
	}
	else {
	  auto n=new Number(std::stoi(array_value));
	  auto p=new Value(ValueType::Number,n);
	  elements.emplace_back(p);
	}
	next_start=++end;
    }
  }
}
