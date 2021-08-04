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

/**
  * This file is @deprecated
  */

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
		std::string buff("BadSymbol Exception!\n");
		buff += std::string(except);
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
		e_ += std::string("At line ") + std::to_string(l)
			+ std::string(", column ") + std::to_string(c) + std::string(".");
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
		e_ += std::string("At line ") + std::to_string(l)
			+ std::string(", column ") + std::to_string(c) + std::string(".");
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
		e_ += std::string(" At line ") + std::to_string(l)
			+ std::string(", column ") + std::to_string(c) + std::string(".");
    }

    virtual ~FigNotConstant() throw() {}


    virtual const char* what() const throw()
    {
        return e_.c_str();
    }
};


#endif // EXCEPTIONS_H

