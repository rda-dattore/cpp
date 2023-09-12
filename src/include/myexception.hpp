#ifndef MYEXCEPTION_H
#define   MYEXCPTION_H

#define IMPLEMENTED private: \
                      void implemented() { }

#include <iostream>
#include <exception>
#include <string>

namespace my {

class Exception : public std::exception {
public:
  Exception(std::string message) : m(message) { }
  const char *what() const throw() {
      std::cerr << "  what(): " << m << std::endl;
      exit(1);
  }

protected:
  virtual void implemented() = 0;

  std::string m;
};

class CoreDump : public std::exception {
public:
  CoreDump(std::string message) : m(message) { }
  const char *what() const throw() { return m.c_str(); }

protected:
  virtual void implemented() = 0;

  std::string m;
};

class AlreadyOpen_Error : public Exception {
public:
  AlreadyOpen_Error(std::string message) : Exception(message) { }
  IMPLEMENTED
};

class BadOperation_Error : public Exception {
public:
  BadOperation_Error(std::string message) : Exception(message) { }
  IMPLEMENTED
};

class BadSpecification_Error : public Exception {
public:
  BadSpecification_Error(std::string message) : Exception(message) { }
  IMPLEMENTED
};

class BadType_Error : public Exception {
public:
  BadType_Error(std::string message) : Exception(message) { }
  IMPLEMENTED
};

class BufferOverflow_Error : public Exception {
public:
  BufferOverflow_Error(std::string message) : Exception(message) { }
  IMPLEMENTED
};

class InvalidJSON_Error : public Exception {
public:
  InvalidJSON_Error(std::string message) : Exception(message) { }
  IMPLEMENTED
};

class MissingParameter_Error : public Exception {
public:
  MissingParameter_Error(std::string message) : Exception(message) { }
  IMPLEMENTED
};

class NotAuthorized_Error : public Exception {
public:
  NotAuthorized_Error(std::string message) : Exception(message) { }
  IMPLEMENTED
};

class NotFound_Error : public Exception {
public:
  NotFound_Error(std::string message) : Exception(message) { }
  IMPLEMENTED
};

class OpenFailed_Error : public Exception {
public:
  OpenFailed_Error(std::string message) : Exception(message) { }
  IMPLEMENTED
};

class UndefinedVariable_Error : public Exception {
public:
  UndefinedVariable_Error(std::string message) : Exception(message) { }
  IMPLEMENTED
};

} // end namespace my

#endif
