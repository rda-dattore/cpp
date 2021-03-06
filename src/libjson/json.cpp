#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <json.hpp>
#include <strutils.hpp>
#include <myerror.hpp>

void JSON::Value::clear()
{
  switch (_type) {
    case ValueType::String: {
	delete reinterpret_cast<String *>(_json_value);
	break;
    }
    case ValueType::Number: {
	delete reinterpret_cast<Number *>(_json_value);
	break;
    }
    case ValueType::Object: {
	delete reinterpret_cast<Object *>(_json_value);
	break;
    }
    case ValueType::Array: {
	delete reinterpret_cast<Array *>(_json_value);
	break;
    }
    case ValueType::Boolean: {
	delete reinterpret_cast<Boolean *>(_json_value);
	break;
    }
    case ValueType::Null:
    case ValueType::Nonexistent: {
	delete reinterpret_cast<Null *>(_json_value);
	break;
    }
  }
}

std::vector<std::string> JSON::Value::keys() const
{
  switch (_type) {
    case ValueType::Object: {
	return reinterpret_cast<Object *>(_json_value)->keys();
    }
    default: {
	return std::vector<std::string>{};
    }
  }
}

size_t JSON::Value::size() const
{
  switch (_type) {
    case ValueType::String:
    case ValueType::Number:
    case ValueType::Boolean: {
	return 1;
    }
    case ValueType::Object: {
	return reinterpret_cast<Object *>(_json_value)->size();
    }
    case ValueType::Array: {
	return reinterpret_cast<Array *>(_json_value)->size();
    }
    default: {
	return 0;
    }
  }
}

std::string JSON::Value::to_string() const
{
  switch (_type) {
    case ValueType::String: {
	return reinterpret_cast<String *>(_json_value)->to_string();
    }
    case ValueType::Number: {
	return reinterpret_cast<Number *>(_json_value)->to_string();
    }
    case ValueType::Object: {
	return reinterpret_cast<Object *>(_json_value)->to_string();
    }
    case ValueType::Array: {
	return reinterpret_cast<Array *>(_json_value)->to_string();
    }
    case ValueType::Boolean: {
	return reinterpret_cast<Boolean *>(_json_value)->to_string();
    }
    default: {
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
    case ValueType::Object: {
	return (*(reinterpret_cast<Object *>(_json_value)))[key];
    }
    default: {
	return nonexistent;
    }
  }
}

const JSON::Value& JSON::Value::operator[](int index) const
{
  return (*this)[static_cast<size_t>(index)];
}

const JSON::Value& JSON::Value::operator[](size_t index) const
{
  static const void *n=new Null();
  static const Value nonexistent(ValueType::Nonexistent,const_cast<void *>(n));
  switch (_type) {
    case ValueType::Array: {
	return (*(reinterpret_cast<Array *>(_json_value)))[index];
    }
    default: {
	return nonexistent;
    }
  }
}

bool JSON::Value::operator>(long long l) const
{
  if (_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(_json_value)->number() > l);
  }
  else {
    return false;
  }
}

bool JSON::Value::operator<(long long l) const
{
  if (_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(_json_value)->number() < l);
  }
  else {
    return false;
  }
}

bool JSON::Value::operator==(long long l) const
{
  if (_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(_json_value)->number() == l);
  }
  else {
    return false;
  }
}

bool JSON::Value::operator>=(long long l) const
{
  if (_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(_json_value)->number() >= l);
  }
  else {
    return false;
  }
}

bool JSON::Value::operator<=(long long l) const
{
  if (_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(_json_value)->number() <= l);
  }
  else {
    return false;
  }
}

bool JSON::Value::operator==(std::string s) const
{
  if (_type == ValueType::String) {
    return (reinterpret_cast<String *>(_json_value)->to_string() == s);
  }
  else {
    return false;
  }
}

bool JSON::Value::operator==(const char *s) const
{
  return (*this == std::string(s));
}

void JSON::Object::clear()
{
  for (auto& e : pairs) {
    e.second->clear();
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
	bool in_quotes=false;
	size_t idx=0;
	for (size_t n=0; n < pair.length(); ++n) {
	  switch (pair[n]) {
	    case '"': {
		in_quotes=!in_quotes;
		break;
	    }
	    case ':': {
		if (!in_quotes) {
		  idx=n;
		  n=pair.length();
		}
		break;
	    }
	    default: {}
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
	    auto n=new Number(std::stoll(value));
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
    case JSON::ValueType::String: {
	o << *(reinterpret_cast<JSON::String *>(v._json_value));
	break;
    }
    case JSON::ValueType::Number: {
	o << *(reinterpret_cast<JSON::Number *>(v._json_value));
	break;
    }
    case JSON::ValueType::Object: {
	o << *(reinterpret_cast<JSON::Object *>(v._json_value));
	break;
    }
    case JSON::ValueType::Array: {
	o << *(reinterpret_cast<JSON::Array *>(v._json_value));
	break;
    }
    case JSON::ValueType::Boolean: {
	o << *(reinterpret_cast<JSON::Boolean *>(v._json_value));
	break;
    }
    default: {}
  }
  return o;
}

bool operator==(const JSON::Value& v1,const JSON::Value& v2)
{
  if (v1._type == v2._type) {
    switch (v1._type) {
	case JSON::ValueType::String: {
	}
	case JSON::ValueType::Number: {
	  return (reinterpret_cast<JSON::Number *>(v1._json_value)->number() == reinterpret_cast<JSON::Number *>(v2._json_value)->number());
	}
	case JSON::ValueType::Object: {
	}
	case JSON::ValueType::Array: {
	}
	case JSON::ValueType::Boolean: {
	}
	default: {
	  return true;
	}
    }
  }
  else {
    return false;
  }
}

bool operator!=(const JSON::Value& v1,const JSON::Value& v2)
{
  return !(v1 == v2);
}

bool operator!=(const JSON::Value& v,const int& i)
{
  return !(v == i);
}

std::ostream& operator<<(std::ostream& o,const JSON::String& str)
{
  o << str.s;
  return o;
}

std::ostream& operator<<(std::ostream& o,const JSON::Number& num)
{
  o << num.l;
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
    e->clear();
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
	  auto n=new Number(std::stoll(array_value));
	  auto p=new Value(ValueType::Number,n);
	  elements.emplace_back(p);
	}
	next_start=++end;
    }
  }
}

const JSON::Value& JSON::Array::operator[](size_t index) const
{
  static const void *n=new Null();
  static const Value nonexistent(ValueType::Nonexistent,const_cast<void *>(n));
  if (index >= elements.size()) {
    return nonexistent;
  }
  else {
    return *elements[index];
  }
}
