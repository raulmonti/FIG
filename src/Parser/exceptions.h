//==============================================================================
//
// Exceptions file for FIG project 'Parser module'.
// Raul Monti
// 2015
//
//==============================================================================


#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <string>


//==============================================================================

class IOSAComplianceExc: public std::exception
{

    const char* except;

public:

    IOSAComplianceExc(const char* msg)
    {
        except = msg;
    }

    virtual ~IOSAComplianceExc(){};

    virtual const char* what() const throw()
    {
        return except;
    }

};


//==============================================================================

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


//==============================================================================

class BadSymbol: public std::exception
{

public:

    const char* except;

public:

    BadSymbol(const char* msg)
    {
        except = msg;
    }

    virtual ~BadSymbol(){};

    virtual const char* what() const throw()
    {
        string buff("BadSymbol Exception!\n");
        buff += string(except);
        return buff.c_str();
    }

};


//==============================================================================


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


//==============================================================================


class BadAST: public std::exception
{

public:

  BadAST(){}

  virtual ~BadAST() throw() {}

  virtual const char* what() const throw()
  {
    return std::string("WRONG AST!").c_str();
  }
};


#endif // EXCEPTIONS_H

