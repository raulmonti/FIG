#ifndef MODEL_BUILDER_H
#define MODEL_BUILDER_H

#include "ModelAST.h"
#include "ModelParser.hpp"

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
  ModelBuilder() : file{nullptr} {};
  ~ModelBuilder();
  Model *build(const std::string &filename);
  void error(ModelParserGen::location loc, std::string msg);
};

#endif
