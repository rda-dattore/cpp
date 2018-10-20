#include <fstream>
#include <unordered_map>
#include <vector>
#include <strutils.hpp>

class JSON
{
public:
  enum class ValueType {Nonexistent,String,Number,Object,Array,Boolean,Null};

  class Value
  {
  public:

    Value(const ValueType& type,void *content) : _type(type),_json_value(content) {}
    void clear();
    std::vector<std::string> keys() const;
    size_t size() const;
    std::string to_string() const;
    ValueType type() const { return _type; }
    const Value& operator[](const char* key) const;
    const Value& operator[](std::string key) const;
    const Value& operator[](size_t index) const;
    bool operator>(int i) const;
    bool operator<(int i) const;
    bool operator==(int i) const;
    bool operator>=(int i) const;
    bool operator<=(int i) const;
    friend std::ostream& operator<<(std::ostream& o,const Value& v);
    friend bool operator==(const Value& v1,const Value& v2);
    friend bool operator!=(const Value& v1,const Value& v2);
    friend bool operator!=(const Value& v,const int& i);

  private:
    ValueType _type;
    void *_json_value;
  };

  class String
  {
  public:
    String(std::string s) : s(s) {}
    std::string to_string() const { return s; }
    friend std::ostream& operator<<(std::ostream& o,const String& str);

  private:
    std::string s;
  };

  class Number
  {
  public:
    Number(int i) : i(i) {}
    int number() const { return i; }
    std::string to_string() const { return strutils::itos(i); }
    friend std::ostream& operator<<(std::ostream& o,const Number& num);

  private:
    int i;
  };

  class Object
  {
  public:
    Object(std::string json_object) : pairs() { fill(json_object); }
    Object(std::ifstream& ifs) : pairs() { fill(ifs); }
    ~Object();
    operator bool() const { return pairs.size() > 0; }
    void fill(std::string json_object);
    void fill(std::ifstream& ifs);
    std::vector<std::string> keys() const;
    size_t size() const { return pairs.size(); }
    std::string to_string() const { return "[Object]"; }
    const Value& operator[](const char* key) const;
    const Value& operator[](std::string key) const;
    friend std::ostream& operator<<(std::ostream& o,const Object& obj);

  private:
    void clear();

    std::unordered_map<std::string,Value *> pairs;
  };

  class Array
  {
  public:
    Array(std::string json_array) : elements() { fill(json_array); }
    ~Array();
    operator bool() const { return elements.size() > 0; }
    void fill(std::string json_array);
    size_t size() const { return elements.size(); }
    std::string to_string() const { return "[Array]"; }
    const Value& operator[](size_t index) const;
    friend std::ostream& operator<<(std::ostream& o,const Array& arr);

  private:
    void clear();

    std::vector<Value *> elements;
  };

  class Boolean
  {
  public:
    Boolean(bool b) : b(b) {}
    std::string to_string() const { return (b) ? "true" : "false"; }
    friend std::ostream& operator<<(std::ostream& o,const Boolean& b);

  private:
    bool b;
  };

  class Null
  {
  public:
    Null() {}
    friend std::ostream& operator<<(std::ostream& o,const Null& n);

  private:
  };

};

namespace JSONUtils {

extern void find_csv_ends(const std::string& json,std::vector<size_t>& csv_ends);

} // end namespace JSONUtils
