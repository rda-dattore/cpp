#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <json.hpp>
#include <strutils.hpp>

using Array = JSON::Array;
using Boolean = JSON::Boolean;
using Object = JSON::Object;
using Null = JSON::Null;
using Number = JSON::Number;
using String = JSON::String;
using Value = JSON::Value;
using ValueType = JSON::ValueType;
using std::ostream;
using std::runtime_error;
using std::string;
using std::vector;

namespace JSONUtils {

void emplace_value(void *c, string v, string k = "") {
  Value *p = nullptr;
  if (v == "null") {
    auto n = new Null();
    p = new Value(ValueType::Null, n);
  } else if (v == "true") {
    auto b = new Boolean(true);
    p = new Value(ValueType::Boolean, b);
  } else if (v == "false") {
    auto b = new Boolean(false);
    p = new Value(ValueType::Boolean, b);
  } else if (v.front() == '"' && v.back() == '"') {
    auto s = new String(v.substr(1, v.length() - 2));
    p = new Value(ValueType::String, s);
  } else if (v.front() == '{' && v.back() == '}') {
    auto o = new Object(v);
    p = new Value(ValueType::Object, o);
  } else if (v.front() == '[' && v.back() == ']') {
    auto a = new Array(v);
    p = new Value(ValueType::Array, a);
  } else {
    auto n = new Number(std::stoll(v));
    p = new Value(ValueType::Number, n);
  }
  if (k.empty()) {
    reinterpret_cast<vector<Value *> *>(c)->emplace_back(p);
  } else {
    reinterpret_cast<std::unordered_map<string, Value *> *>(c)->emplace(k, p);
  }
}

} // end namespace JSONUtils

Value::operator bool() const {
  if (m_type == ValueType::Boolean) {
    return reinterpret_cast<Boolean *>(json_value)->b;
  } else {
    throw runtime_error("json value is not a Boolean");
  }
}

void Value::clear() {
  switch (m_type) {
    case ValueType::String: {
      delete reinterpret_cast<String *>(json_value);
      break;
    }
    case ValueType::Number: {
      delete reinterpret_cast<Number *>(json_value);
      break;
    }
    case ValueType::Object: {
      delete reinterpret_cast<Object *>(json_value);
      break;
    }
    case ValueType::Array: {
      delete reinterpret_cast<Array *>(json_value);
      break;
    }
    case ValueType::Boolean: {
      delete reinterpret_cast<Boolean *>(json_value);
      break;
    }
    case ValueType::Null:
    case ValueType::Nonexistent: {
      delete reinterpret_cast<Null *>(json_value);
      break;
    }
  }
}

vector<string> Value::keys() const {
  switch (m_type) {
    case ValueType::Object: {
      return reinterpret_cast<Object *>(json_value)->keys();
    }
    default: {
      return vector<string>{};
    }
  }
}

size_t Value::size() const {
  switch (m_type) {
    case ValueType::String:
    case ValueType::Number:
    case ValueType::Boolean: {
      return 1;
    }
    case ValueType::Object: {
      return reinterpret_cast<Object *>(json_value)->size();
    }
    case ValueType::Array: {
      return reinterpret_cast<Array *>(json_value)->size();
    }
    default: {
      return 0;
    }
  }
}

string Value::to_string() const {
  switch (m_type) {
    case ValueType::String: {
      return reinterpret_cast<String *>(json_value)->to_string();
    }
    case ValueType::Number: {
      return reinterpret_cast<Number *>(json_value)->to_string();
    }
    case ValueType::Object: {
      return reinterpret_cast<Object *>(json_value)->to_string();
    }
    case ValueType::Array: {
      return reinterpret_cast<Array *>(json_value)->to_string();
    }
    case ValueType::Boolean: {
      return reinterpret_cast<Boolean *>(json_value)->to_string();
    }
    default: {
      return "";
    }
  }
}

const Value& Value::operator[](const char *key) const {
  return (*this)[string(key)];
}

const Value& Value::operator[](string key) const {
  static const void *n = new Null();
  static const Value nonexistent(ValueType::Nonexistent, const_cast<void *>(n));
  switch (m_type) {
    case ValueType::Object: {
      return (*(reinterpret_cast<Object *>(json_value)))[key];
    }
    default: {
      return nonexistent;
    }
  }
}

const Value& Value::operator[](int index) const {
  return (*this)[static_cast<size_t>(index)];
}

const Value& Value::operator[](size_t index) const {
  static const void *n = new Null();
  static const Value nonexistent(ValueType::Nonexistent, const_cast<void *>(n));
  switch (m_type) {
    case ValueType::Array: {
      return (*(reinterpret_cast<Array *>(json_value)))[index];
    }
    default: {
      return nonexistent;
    }
  }
}

bool Value::operator>(long long l) const {
  if (m_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(json_value)->number() > l);
  }
  throw runtime_error("json value is not a Number");
}

bool Value::operator<(long long l) const {
  if (m_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(json_value)->number() < l);
  }
  throw runtime_error("json value is not a Number");
}

bool Value::operator==(long long l) const {
  if (m_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(json_value)->number() == l);
  }
  throw runtime_error("json value is not a Number");
}

bool Value::operator>=(long long l) const {
  if (m_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(json_value)->number() >= l);
  }
  throw runtime_error("json value is not a Number");
}

bool Value::operator<=(long long l) const {
  if (m_type == ValueType::Number) {
    return (reinterpret_cast<Number *>(json_value)->number() <= l);
  }
  throw runtime_error("json value is not a Number");
}

bool Value::operator==(string s) const
{
  if (m_type == ValueType::String) {
    return (reinterpret_cast<String *>(json_value)->to_string() == s);
  }
  throw runtime_error("json value is not a String");
}

bool Value::operator==(const char *s) const {
  return (*this == string(s));
}

void Object::clear() {
  for (auto& e : pairs) {
    e.second->clear();
    delete e.second;
  }
  pairs.clear();
}

Object::~Object() {
  this->clear();
}

void Object::fill(string json_object) {
  this->clear();
  strutils::trim(json_object);
  if (json_object.front() != '{' || json_object.back() != '}') {
    throw runtime_error("not a json object");
  }
  json_object.erase(0, 1);
  json_object.pop_back();
  if (!json_object.empty()) {
    vector<size_t> vec;
    JSONUtils::find_csv_ends(json_object, vec);
    size_t first = 0;
    for (auto last : vec) {
      auto pair = json_object.substr(first, last - first);
      auto in_quotes = false;
      size_t i = 0;
      for (size_t n = 0; n < pair.length(); ++n) {
        switch (pair[n]) {
          case '"': {
            in_quotes = !in_quotes;
            break;
          }
          case ':': {
            if (!in_quotes) {
              i = n;
              n = pair.length();
            }
            break;
          }
          default: { }
        }
      }
      auto k = pair.substr(0, i);
      strutils::trim(k);
      if (k.front() != '"' || k.back() != '"') {
        pairs.clear();
        throw runtime_error("invalid key '" + k + "'");
      }
      k.erase(0, 1);
      k.pop_back();
      auto v = pair.substr(i + 1);
      strutils::trim(v);
      if (pairs.find(k) == pairs.end()) {
        JSONUtils::emplace_value(&pairs, v, k);
/*
        if (value == "null") {
          auto n = new Null();
          auto p = new Value(ValueType::Null, n);
          pairs.emplace(k, p);
        } else if (value == "true") {
          auto b = new Boolean(true);
          auto p = new Value(ValueType::Boolean, b);
          pairs.emplace(k, p);
        } else if (value == "false") {
          auto b = new Boolean(false);
          auto p = new Value(ValueType::Boolean, b);
          pairs.emplace(k, p);
        } else if (value.front() == '"' && value.back() == '"') {
          auto s = new String(value.substr(1, value.length() - 2));
          auto p = new Value(ValueType::String, s);
          pairs.emplace(k, p);
        } else if (value.front() == '{' && value.back() == '}') {
          auto o = new Object(value);
          auto p = new Value(ValueType::Object, o);
          pairs.emplace(k, p);
        } else if (value.front() == '[' && value.back() == ']') {
          auto a = new Array(value);
          auto p = new Value(ValueType::Array, a);
          pairs.emplace(k, p);
        } else {
          auto n = new Number(std::stoll(value));
          auto p = new Value(ValueType::Number, n);
          pairs.emplace(k, p);
        }
*/
      } else {
      }
      first = ++last;
    }
  }
}

void Object::fill(std::ifstream& ifs) {
  if (!ifs.is_open()) {
    throw runtime_error("unable to open file to fill json object");
  }
  ifs.seekg(0, std::ios::end);
  auto length = ifs.tellg();
  std::unique_ptr<char[]> buffer(new char[length]);
  ifs.seekg(0, std::ios::beg);
  ifs.read(buffer.get(), length);
  string json(buffer.get(), length);
  fill(json);
}

vector<string> Object::keys() const {
  vector<string> v;
  for (const auto& e : pairs) {
    v.emplace_back(e.first);
  }
  return std::move(v);
}

ostream& operator<<(ostream& o, const Value& v) {
  switch (v.m_type) {
    case ValueType::String: {
      o << *(reinterpret_cast<String *>(v.json_value));
      break;
    }
    case ValueType::Number: {
      o << *(reinterpret_cast<Number *>(v.json_value));
      break;
    }
    case ValueType::Object: {
      o << *(reinterpret_cast<Object *>(v.json_value));
      break;
    }
    case ValueType::Array: {
      o << *(reinterpret_cast<Array *>(v.json_value));
      break;
    }
    case ValueType::Boolean: {
      o << *(reinterpret_cast<Boolean *>(v.json_value));
      break;
    }
    default: { }
  }
  return o;
}

bool operator==(const Value& v1, const Value& v2) {
  if (v1.m_type == v2.m_type) {
    switch (v1.m_type) {
      case ValueType::String: {
      }
      case ValueType::Number: {
        return (reinterpret_cast<Number *>(v1.json_value)->number() ==
            reinterpret_cast<Number *>(v2.json_value)->number());
      }
      case ValueType::Object: {
      }
      case ValueType::Array: {
      }
      case ValueType::Boolean: {
      }
      default: {
        return true;
      }
    }
  }
  return false;
}

bool operator!=(const Value& v1, const Value& v2) {
  return !(v1 == v2);
}

bool operator!=(const Value& v, const long long& l) {
  return !(v == l);
}

ostream& operator<<(ostream& o, const String& str) {
  o << str.s;
  return o;
}

ostream& operator<<(ostream& o, const Number& num) {
  o << num.l;
  return o;
}

ostream& operator<<(ostream& o, const Object& obj) {
  o << "[Object]";
  return o;
}

ostream& operator<<(ostream& o, const Array& arr) {
  o << "[Array]";
  return o;
}

ostream& operator<<(ostream& o, const Boolean& b) {
  if (b.b) {
    o << "true";
  } else {
    o << "false";
  }
  return o;
}

ostream& operator<<(ostream& o, const Null& n) {
  return o;
}

const Value& Object::operator[](const char* key) const {
  return (*this)[string(key)];
}

const Value& Object::operator[](string key) const {
  static const void *n = new Null();
  static const Value nonexistent(ValueType::Nonexistent, const_cast<void *>(n));
  auto i = pairs.find(key);
  if (i != pairs.end()) {
    return *i->second;
  }
  return nonexistent;
}

void Array::clear() {
  for (auto& e : elements) {
    e->clear();
    delete e;
  }
  elements.clear();
}

Array::~Array() {
  this->clear();
}

void Array::fill(string json_array) {
  this->clear();
  strutils::trim(json_array);
  if (json_array.front() != '[' || json_array.back() != ']') {
    throw runtime_error("not a json Array");
  }
  json_array.erase(0, 1);
  json_array.pop_back();
  if (!json_array.empty()) {
    vector<size_t> vec;
    JSONUtils::find_csv_ends(json_array, vec);
    size_t first = 0;
    for (auto last : vec) {
      auto v = json_array.substr(first, last - first);
      strutils::trim(v);
      JSONUtils::emplace_value(&elements, v);
/*
      if (array_value == "null") {
        auto n = new Null();
        auto p = new Value(ValueType::Null, n);
        elements.emplace_back(p);
      }
      else if (array_value == "true") {
        auto b = new Boolean(true);
        auto p = new Value(ValueType::Boolean, b);
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
*/
      first = ++last;
    }
  }
}

const Value& Array::operator[](size_t index) const {
  static const void *n = new Null();
  static const Value nonexistent(ValueType::Nonexistent, const_cast<void *>(n));
  if (index >= elements.size()) {
    return nonexistent;
  }
  return *elements[index];
}
