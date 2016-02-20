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

class ProgramError: public std::exception
{

    const char* except;

public:

    ProgramError(const char* msg)
    {
        except = msg;
    }

    virtual ~ProgramError() throw() {};

    virtual const char* what() const throw()
    {
        return except;
    }

};


//==============================================================================

class IOSAComplianceExc: public std::exception
{

    const char* except;

public:

    IOSAComplianceExc(const char* msg)
    {
        except = msg;
    }

    virtual ~IOSAComplianceExc() throw(){};

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

    virtual ~BadSymbol()throw(){};

    virtual const char* what() const throw()
    {
        string buff("BadSymbol Exception!\n");
        buff += string(except);
        return buff.c_str();
    }

};


//==============================================================================

class FigBaseException: public std::exception
{
protected:

    std::string e_;

public:

    FigBaseException(){}

    FigBaseException(std::string s, int l = -1, int c = -1): e_(s)
    {
        e_ += string("At line ") + std::to_string(l) 
            + string(", column ") + std::to_string(c) + string(".");
    }
    virtual ~FigBaseException() throw() {}

    virtual const char* what() const throw()
    {
        return e_.c_str();
    }    
};


//==============================================================================


class FigSyntaxError: public FigBaseException
{

private:

    std::string e_;    // The error

public:

    FigSyntaxError(){}

    FigSyntaxError(std::string s = "", int l = -1, int c = -1): e_(s)
    {
        e_ += string("At line ") + std::to_string(l) 
            + string(", column ") + std::to_string(c) + string(".");
    }

    virtual ~FigSyntaxError() throw() {}


    virtual const char* what() const throw()
    {
        return e_.c_str();
    }
};

//==============================================================================

class FigNotConstant: public FigBaseException
{

private:

    std::string e_;    // The error

public:

    FigNotConstant(std::string s = "", int l = -1, int c = -1): e_(s)
    {
        e_ += string(" At line ") + std::to_string(l) 
            + string(", column ") + std::to_string(c) + string(".");
    }

    virtual ~FigNotConstant() throw() {}


    virtual const char* what() const throw()
    {
        return e_.c_str();
    }
};

//==============================================================================

class FigError: public FigBaseException
{
protected:
    std::string e_;
public:

    FigError(){}
    FigError(std::string s): e_(s){}
    virtual ~FigError(){}
    virtual const char* what() const throw()
    {
        return e_.c_str();
    }
};

//==============================================================================

class FigWarning: public FigBaseException
{
protected:
    std::string e_;
public:

    FigWarning(){}
    FigWarning(std::string s): e_(s){}
    virtual ~FigWarning(){}
    virtual const char* what() const throw()
    {
        return e_.c_str();
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

