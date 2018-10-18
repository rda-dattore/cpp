#include <fstream>
#include <unordered_map>
#include <vector>

class JSON
{
public:
  enum class ValueType {string,number,object,array,boolean,null};

  class ValueBase
  {
  public:
    ValueBase() : _type() {}
    virtual ~ValueBase() {}
    virtual size_t size() const=0;
    ValueType type() const { return _type; }
    operator bool() const;
    friend std::ostream& operator<<(std::ostream& o,const ValueBase& v);
    friend bool operator==(const std::string& s,const ValueBase& v);
    friend bool operator==(const ValueBase& v,const std::string& s);
    friend bool operator==(const int& i,const ValueBase& v);
    friend bool operator==(const ValueBase& v,const int& i);

  protected:
    ValueType _type;

  private:
    virtual void print(std::ostream& o) const=0;
  };

  template <class T>
  class Value : public ValueBase
  {
  public:
    Value(const T& data,const ValueType& type) : _data(data) { _type=type; }
    size_t size() const;

    T _data;

  private:
    virtual void print(std::ostream& o) const;
  };

  class String
  {
  public:
    String(std::string s) : s(s) {}
    size_t size() const { return 1; }
    friend std::ostream& operator<<(std::ostream& o,const String& str);

  private:
    std::string s;
  };

  class Number
  {
  public:
    Number(int i) : i(i) {}
    size_t size() const { return 1; }
    friend std::ostream& operator<<(std::ostream& o,const Number& num);

  private:
    int i;
  };

  class Object
  {
  public:
    Object() : pairs() {}
    ~Object() {}
    bool filled(std::string json_object);
    bool filled(std::ifstream& ifs);
    std::vector<std::string> keys() const;
    size_t size() const { return pairs.size(); }
    const ValueBase* operator[](std::string key) const;
    friend std::ostream& operator<<(std::ostream& o,const Object& obj);

  private:
    std::unordered_map<std::string,ValueBase *> pairs;
  };

  class Array
  {
  public:
    Array() : elements() {}
    bool filled(std::string json_array);
    size_t size() const { return elements.size(); }
    const ValueBase* operator[](size_t index) const;
    friend std::ostream& operator<<(std::ostream& o,const Array& arr);

  private:
    std::vector<ValueBase *> elements;
  };

  class Boolean
  {
  public:
    Boolean(bool b) : b(b) {}
    size_t size() const { return 1; }
    friend std::ostream& operator<<(std::ostream& o,const Boolean& b);

  private:
    bool b;
  };

  class Null
  {
  public:
    Null() {}
    size_t size() const { return 0; }
    friend std::ostream& operator<<(std::ostream& o,const Null& n);

  private:
  };

};

namespace JSONUtils {

extern void find_csv_ends(const std::string& json,std::vector<size_t>& csv_ends);

} // end namespace JSONUtils
