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

  std::string e;    // The error
  int l;            // Line number
  int c;            // Column number

  SyntaxError(){}

  SyntaxError(std::string s = "", int lnum = -1, int col = -1){
    e = s;
    l = lnum;
    c = col;
  }

  virtual ~SyntaxError() throw() {}


  virtual const char* what() const throw()
  {
    std::string str = string("At line ") + std::to_string(l) 
                    + string(", column ") + std::to_string(c) + string(".");
    return (e + str).c_str();
  }
};


#endif

