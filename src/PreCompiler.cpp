#include "PreCompiler.h"
#include "parser.h"
#include "iosacompliance.h" // ast2expr
#include "FigException.h"
#include <z3++.h>
#include <vector>


using namespace std;
using namespace parser;
using namespace fig;


// Definition of static Precompiler class members
map<string,string> Precompiler::mConstTable;


//==============================================================================


void
Precompiler::solve_constant_defs(vector<AST*> defs, const parsingContext &pc)
{
    z3::context c;
    z3::solver s(c);
    vector<string> names;
    vector<expr> exprs;

    for(const auto &it: defs){
        string n = it->get_lexeme(_NAME);
        names.push_back(n);
        z3::expr v(c);
        if(pc.at(n).first==T_ARIT){
            v = c.real_const(n.c_str());
        }else{
            v = c.bool_const(n.c_str());
        }
        exprs.push_back(v);
        z3::expr e = ast2expr(it->get_first(_EXPRESSION),c,pc);
        s.add(v == e);
    }
    assert(sat==s.check());
    auto m = s.get_model();
    for(int i = 0; i < exprs.size(); ++i){
        if(pc.at(names[i]).first==T_ARIT){
            Precompiler::mConstTable[names[i]] = 
                Z3_get_numeral_string(c, m.eval(exprs[i],false));
        }else{
            if(Z3_get_bool_value(c,m.eval(exprs[i],false))){
                Precompiler::mConstTable[names[i]] = "true";
            }else{
                Precompiler::mConstTable[names[i]] = "false";
            }
        }
    }
}


//==============================================================================


void
rec_circ_depend(string start, const map<string, vector<string>> &depend){

    static set<string> stack;
    auto ret = stack.insert(start);
    if(!ret.second){
        string err = "";
        for(const auto &it: stack){
            err += it;
            err += " ";
        }
        stack.clear();
        throw FigException(err);
    }
    for(const auto &it: depend.at(start)){
        rec_circ_depend(it, depend);
    }
    stack.erase(start);
}


//==============================================================================


void
check_no_const_circular_depend(const vector<AST*> &consts)
{
    map<string, vector<string>> depend;

    for(const auto &it: consts){
        string name = it->get_lexeme(_NAME);
        AST* expr = it->get_first(_EXPRESSION);
        vector<string> depends = expr->get_all_lexemes(_NAME);
        depend[name] = depends;
    }

    for(const auto &it: depend){
        try{
            rec_circ_depend(it.first, depend);
        }catch(FigException &e){
            throw FigException("[ERROR] Circular dependency on definition"
                               " of constant " + it.first + ". Stack: " + 
                               e.msg());
        }
    }
}


//==============================================================================


string
Precompiler::pre_compile(AST* ast, const parsingContext &pc)
{
    string result = "";

    // Check constants and solve their values.
    vector<AST*> consts = ast->get_all_ast(_CONST);
    check_no_const_circular_depend(consts);
    solve_constant_defs(consts, pc);
    
    // Replace constants by values and constants definitions by blanks.
    auto lexemes = Parser::get_lexemes();
    for(int i = 0; i < lexemes.size(); ++i){
        if(lexemes[i] == "const"){
            while(lexemes[i] != ";"){
                if(lexemes[i] != "\n" && lexemes[i] != "\t"){
                    for(int j = 0; j < lexemes[i].size(); ++j){
                        result += " ";
                    }
                }else{
                    result += lexemes[i];
                }
                i++;
            }
            result += " "; // replaces ';'
        }else{
            auto it = mConstTable.find(lexemes[i]);
            if(it != mConstTable.end()){
                result += it->second;
            }else if(lexemes[i] != "EOF"){
                result += lexemes[i];
            }
        }
    }
    return result;
}

