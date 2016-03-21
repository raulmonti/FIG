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
     * @brief
     */
    ParsingModel();

    /**
     * @brief
     */
    ~ParsingModel();

    /**
     * @brief
     */
    void
    parse_model(stringstream *model);

    /**
     * @brief
     */
    void
    parse_properties(stringstream *model);

    /**
     * @brief
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
