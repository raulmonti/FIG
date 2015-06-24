#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <string>

class Badcharfound: public std::exception
{

public:

  std::string c;

  Badcharfound(){}

  Badcharfound(std::string s){
    c = s;
  }

  virtual ~Badcharfound() throw() {}


  virtual const char* what() const throw()
  {
    return std::string("ERROR! Found bad character: ").append(c).c_str();
  }
};


class SyntaxError: public std::exception
{

public:

  std::string c;

  SyntaxError(){}

  SyntaxError(std::string s){
    c = s;
  }

  virtual ~SyntaxError() throw() {}


  virtual const char* what() const throw()
  {
    return c.c_str();
  }
};


#endif

