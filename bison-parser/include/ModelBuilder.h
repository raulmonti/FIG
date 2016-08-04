#ifndef MODEL_BUILDER_H
#define MODEL_BUILDER_H

#include "ModelAST.h"
#include "ModelParser.hpp"

/** This class uses the lexer and the parser to build a Model AST **/

//Declare lexer's type. 
# define YY_DECL ModelParserGen::ModelParser::symbol_type yylex (ModelBuilder& builder)
YY_DECL;

class ModelBuilder {
private:
    //starts lexer on the given file
    void scan(FILE *file);
    //finish lexer
    void scan_end();
    //file to be parsed
    FILE *file;
    Model *model;
    //let ModelParser to set the final Model:
    void set_result(Model *model);
    friend class ModelParserGen::ModelParser;
public:
    Log *log;
    ModelBuilder(Log *log) : file{nullptr}, log{log} {};
    ~ModelBuilder();
    Model *build(const std::string &filename);
    void error(ModelParserGen::location loc, std::string msg);
};

#endif
