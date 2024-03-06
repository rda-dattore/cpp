#ifndef JSON_HPP
#define  JSON_HPP

#include <fstream>
#include <unordered_map>
#include <vector>
#include <strutils.hpp>

class JSON {
public:
  enum class ValueType{ Nonexistent, String, Number, Object, Array, Boolean,
      Null };

  class Value {
  public:
    Value(const ValueType& type, void *content) : m_type(type), json_value(
        content) { }
    operator bool() const;
    void clear();
    std::vector<std::string> keys() const;
    void pretty_print(std::ostream& o, size_t indent) const;
    size_t size() const;
    std::string to_string() const;
    ValueType type() const { return m_type; }
    const Value& operator[](const char* key) const;
    const Value& operator[](std::string key) const;
    const Value& operator[](int index) const;
    const Value& operator[](size_t index) const;
    bool operator>(long long l) const;
    bool operator<(long long l) const;
    bool operator==(long long l) const;
    bool operator!=(long long l) const { return !(*this == l); }
    bool operator>=(long long l) const;
    bool operator<=(long long l) const;
    bool operator==(std::string s) const;
    bool operator!=(std::string s) const { return !(*this == s); }
    bool operator==(const char *s) const;
    bool operator!=(const char *s) const { return !(*this == s); }
    friend std::ostream& operator<<(std::ostream& o, const Value& v);
    friend bool operator==(const Value& v1, const Value& v2);
    friend bool operator!=(const Value& v1, const Value& v2);
    friend bool operator!=(const Value& v, const long long& l);

  private:
    ValueType m_type;
    void *json_value;
  };

  class String {
  public:
    String(std::string s) : s(s) { }
    std::string to_string() const { return s; }
    friend std::ostream& operator<<(std::ostream& o, const String& str);

  private:
    std::string s;
  };

  class Number {
  public:
    Number(double d) : d(d), m_is_float(true) { }
    Number(long long l) : l(l), m_is_float(false) { }
    double dvalue() const { return d; }
    bool is_float() const { return m_is_float; }
    long long lvalue() const { return l; }
    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& o, const Number& num);

  private:
    union {
      double d;
      long long l;
    };
    bool m_is_float;
  };

  class Object {
  public:
    Object() : pairs() { }
    Object(std::string json_object) : Object() { fill(json_object); }
    Object(std::ifstream& ifs) : Object() { fill(ifs); }
    ~Object();
    operator bool() const { return pairs.size() > 0; }
    void fill(std::string json_object);
    void fill(std::ifstream& ifs);
    std::vector<std::string> keys() const;
    void pretty_print(std::ostream& o, size_t indent = 0) const;
    size_t size() const { return pairs.size(); }
    std::string to_string() const { return "[Object]"; }
    const Value& operator[](const char* key) const;
    const Value& operator[](std::string key) const;
    friend std::ostream& operator<<(std::ostream& o, const Object& obj);

  private:
    void clear();

    std::unordered_map<std::string, Value *> pairs;
  };

  class Array {
  public:
    Array(std::string json_array) : elements() { fill(json_array); }
    ~Array();
    operator bool() const { return elements.size() > 0; }
    void fill(std::string json_array);
    void pretty_print(std::ostream& o, size_t indent = 0) const;
    size_t size() const { return elements.size(); }
    std::string to_string() const { return "[Array]"; }
    const Value& operator[](size_t index) const;
    friend std::ostream& operator<<(std::ostream& o, const Array& arr);

  private:
    void clear();

    std::vector<Value *> elements;
  };

  class Boolean {
  public:
    friend class Value;
    Boolean(bool b) : b(b) { }
    std::string to_string() const { return (b) ? "true" : "false"; }
    friend std::ostream& operator<<(std::ostream& o, const Boolean& b);

  private:
    bool b;
  };

  class Null {
  public:
    Null() { }
    friend std::ostream& operator<<(std::ostream& o, const Null& n);

  private:
  };

};

namespace JSONUtils {

extern void find_csv_ends(const std::string& json, std::vector<size_t>&
    csv_ends);

} // end namespace JSONUtils

#endif
