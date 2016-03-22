#ifndef PARSING_MODEL_H
#define PARSING_MODEL_H

#include<sstream>
#include<vector>
#include<map>
#include<string>

#include"Parser.h"
#include"Iosacompliance.h"
#include"PreCompiler.h"
#include"Ast.h"


using namespace std;
using namespace parser;

class ParsingModel
{

private:

    Parser              *mParser;
    Verifier            *mVerifier;
    Precompiler         *mPrecompiler;

    AST                 *mModel;
    AST                 *mProperties;
    vector<string>      mModelLxms;
    vector<string>      mPropLxms;
    parsingContext      mPc;
    map<string,string>  mCt;

public:

    /**
     * @brief Ctor
     */
    ParsingModel();

    /**
     * @brief Dtor
     */
    ~ParsingModel();

    /**
     * @brief Parse and save the model in @model. Also checks for its
     * compliance with IOSA.
     */
    void
    parse_model(stringstream *model);

    /**
     * @brief Parse and save the properties in @props.
     */
    void
    parse_properties(stringstream *props);

    /**
     * @brief Compile the model previously parsed with parse_model method.
     */
    void
    compile_model(void);

    /**
     * @brief
     */
    const AST*
    get_model_ast(void);

    /**
     * @brief
     */
    const AST*
    get_props_ast(void);
};



#endif // PARSING_MODEL_H
