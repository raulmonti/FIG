/* -*- C++ -*- */
/* IOSA PARSER GENERATOR
 *
 * Leonardo Rodríguez
 */

%skeleton "lalr1.cc" 
%require "3.0.4"
%defines
%define api.namespace {ModelParserGen}
%define parser_class_name {ModelParser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define parse.error verbose
%define parse.trace false

%code requires
{
#include <cassert>
#include <string>
#include <sstream>
#include <memory>
    
#include "ModelAST.h"
#include "Util.h"

    using std::shared_ptr;
    using std::make_shared;
    using std::static_pointer_cast;
    
//definition YY_DECL should be available also for ModelScannerGen.ll
//ModelScannerGen includes ModuleParser.hpp
# define YY_DECL							\
    ModelParserGen::ModelParser::symbol_type yylex (shared_ptr<ModelAST> *result)
}

%param {shared_ptr<ModelAST> *result}
%locations

%code
{
    //Declared only when ModelParser class is already defined
    YY_DECL;
}

%define api.token.prefix {TOK_}
%token
END  0  "end of file"
COMMA ";"
MODULE "module"
ENDMODULE "endmodule"
PROP "properties"
ENDPROP "endproperties"
TINT "int"
TBOOL "bool"
TFLOAT "float"
CONST "const"
LPAREN "("
RPAREN ")"
LBRACKET "["
RBRACKET "]"
LBRACE "{"
RBRACE "}"
CLOCK "clock"
EQ "="
SQUOTE "'"
SEMICOLON ":"
COLON ","
DOTDOT ".."
AT "@"
ERLANG "erlang"
NORMAL "normal"
UNIFORM "uniform"
EXP "exponential"
INIT "init"
EQQ "=="
NEQ "!="
LT "<"
GT ">"
LE "<="
GE ">="
AND "&"
MID "|"
PLUS "+"
MINUS "-"
TIMES "*"
MOD "%"
DIV "/"
QMARK "?"
ADM "!"
ARROW "->"
TRUE "true"
FALSE "false"
PROPT "P"
PROPS "S"
UNTIL "U"
;

%left "|";
%left "&";
%nonassoc "==" "!=";
%nonassoc "<" "<=" ">" ">=";
%left "+" "-";
%left "*" "/" "%";
%nonassoc "!";
%nonassoc UMINUS;
%nonassoc NEG;

%token <std::string> ID "id"
%token <int>   INTL "int_literal"
%token <float> FLOATL "float_literal"
%type  <shared_ptr<Model>> model
%type  <shared_ptr<Decl>> global_decl
%type  <shared_ptr<Exp>>  exp
%type  <shared_ptr<Dist>> dist
%type  <shared_vector<Effect>> effects
%type  <shared_ptr<Location>> location
%type  <shared_ptr<Exp>> guard
%type  <shared_ptr<Action>> action
%type  <shared_vector<Exp>> exp_seq
%type  <shared_vector<Exp>> array_init
%type  <shared_ptr<Decl>> decl
%type  <shared_ptr<ModuleBody>> module_body
%type  <Type> type
%type  <shared_ptr<Prop>> prop
%type  <shared_vector<Prop>> proplist

%%
%start begin;

begin: model[m] {*result = $m;}
|  model[m] "properties" proplist[v] "endproperties"
{auto &res = $m; res->add_props($v); *result = res;}
|  proplist[v]
{//assert model is already created
    assert(*result != nullptr);
    shared_ptr<Model> model = static_pointer_cast<Model>(*result);
    model->add_props($v);
}

model: global_decl[d] ";"
{$$ = make_shared<Model>($d);}
| model[m] global_decl[d] ";"
{$m->add_decl($d); $$ = $m;}
| "module" "id"[id] module_body[b] "endmodule"
{$$ = make_shared<Model>($id, $b);}
| model[m] "module" "id"[id] module_body[b] "endmodule"
{
    string &id = $id;
    shared_ptr<Model> model = $m;
    if (model->has_module(id)) {
	std::cerr << "Two modules with the same name " + id << std::endl;
	exit(1);
    } else {
	$m->add_module(id, $b);
	$$ = std::move($m);
    }
}

prop: "P" "(" exp[l] "U" exp[r] ")" 
{ $$ = make_shared<Prop>($l, $r);}
| "S" "(" exp[r] ")"
{ $$ = make_shared<Prop>($r);}    

proplist: prop[p]
{ $$ = vector<shared_ptr<Prop>>{$p};}
| proplist[v] prop[p]
{ $$ = $v ; $$.push_back($p);}

type: "int"
{$$ = Type::tint;}
| "bool"
{$$ = Type::tbool;}
| "float"
{$$ = Type::tfloat;}

global_decl: "const" type[t] "id"[id] "=" exp[e]
{$$ = make_shared<Decl>($t, $id, $e);}
| "const" "id"[id] "[" exp[size] "]" ":" "[" exp[lower] ".." exp[upper] "]" "init" array_init[seq]
{$$ = make_shared<Decl>(Type::tint, $id, $size, $lower, $upper, $seq);}
| "const" "id"[id] "[" exp[size] "]" ":" type[t] "init" array_init[seq]
{$$ = make_shared<Decl>($t, $id, $size, $seq);}
| "const" type[t] "id"[id] "[" exp[size] "]" "=" array_init[seq]
{$$ = make_shared<Decl>($t, $id, $size, $seq);}

module_body: decl[d] ";"
{$$ = make_shared<ModuleBody>($d);}
| action[a] ";"
{$$ = make_shared<ModuleBody>($a);}
| module_body[mb] decl[d] ";"
{$mb->add_decl($d); $$ = $mb;}
| module_body[mb] action[a] ";"
{$mb->add_action($a); $$ = $mb;}

decl: "id"[id] ":" "[" exp[low] ".." exp[upper] "]"
{$$ = make_shared<Decl>(Type::tint, $id, $low, $upper);}
| "id"[id] ":" "[" exp[low] ".." exp[upper] "]" "init" exp[e]
{$$ = make_shared<Decl>(Type::tint, $id, $low, $upper, $e);}
| "id"[id] ":" "bool" "init" exp[e]
{$$ = make_shared<Decl>(Type::tbool, $id, $e);}
| "id"[id] ":" "clock"
{$$ = make_shared<Decl>(Type::tclock, $id);}
| "id"[id] "[" exp[size] "]" ":"
"[" exp[lower] ".." exp[upper] "]" "init" array_init[seq]
{$$ = make_shared<Decl>(Type::tint, $id, $size, $lower, $upper, $seq);}
| "id"[id] "[" exp[size] "]" ":" "bool" "init" array_init[seq]
{$$ = make_shared<Decl>(Type::tbool, $id, $size, $seq);}

array_init: "{" exp_seq[es] "}"
{$$ = $es;}
| exp[e]
{$$ = shared_vector<Exp>{$e};}

exp_seq: exp_seq[es] "," exp[e]
{$$ = concat($es, shared_vector<Exp>{$e});}
| exp[e]
{$$ = shared_vector<Exp>{$e};}

action: "[" "id"[id] "?" "]" guard[e] "->" effects[eff]
{$$ = make_shared<Action>($id, LabelType::in, $e, $eff);}
| "[" "id"[id] "!" "]" guard[e] "@" location[loc] "->" effects[eff]
{$$ = make_shared<Action>($id, LabelType::out, $e, $loc, $eff);}
| "[" "]" guard[e] "@" location[loc] "->" effects[eff] //see [] in atm_queue.sa
{$$ = make_shared<Action>(LabelType::empty, $e, $loc, $eff);}
| "[" "id"[id] "!" "!" "]" guard[e] "->" effects[eff]
{$$ = make_shared<Action>($id, LabelType::commited, $e, $eff);}

guard: %empty
{$$ = make_shared<BConst>(true);}
| exp[e]
{$$ = $e;}

effects: %empty
{$$ = shared_vector<Effect>(); }
|"(" location[loc] "'" "=" exp[e] ")"
{$$ = shared_vector<Effect>{make_shared<Effect>($loc, $e)};}
| "(" location[loc] "'" "=" dist[d] ")"
{$$ = shared_vector<Effect>{make_shared<Effect>($loc, $d)};}
| effects[e1] "&" effects[e2]
{$$ = concat($e1, $e2);}

dist: "erlang" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<Dist>(DistType::erlang,  Arity::two, $e1, $e2);}
| "normal" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<Dist>(DistType::normal,  Arity::two, $e1, $e2);}
| "uniform" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<Dist>(DistType::uniform, Arity::two, $e1, $e2);}
| "exponential" "(" exp[e] ")"
{$$ = make_shared<Dist>(DistType::exponential, Arity::one, $e);}

location: "id"[id] {$$ = make_shared<Location>($id);}
| "id"[id] "[" exp[e] "]" {$$ = make_shared<Location>($id, $e);}
		 
// Note on expressions types: a separation between int-exp and bool-exp
// introduces reduce/reduce conflicts
// with the "identifier" rules (how to reduce an
// identifier? as a bexp or as a iexp)?
// Seems to be convenient to join them since 
// we have to do typechecking anyway.
// Expressions of type float are only allowed in distribution's parameters,
// That should be checked during type-checking.

exp : location[loc]
{$$ = make_shared<LocExp>($loc);}
| INTL[i]
{$$ = make_shared<IConst>($i);}
| FLOATL[i]
{$$ = make_shared<FConst>($i);}
| "true"
{$$ = make_shared<BConst>(true);}
| "false"
{$$ = make_shared<BConst>(false);}
| exp[e1] "+" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::plus, $e1, $e2);}
| exp[e1] "-" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::minus, $e1, $e2);}
| exp[e1] "/" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::div, $e1, $e2);}
| exp[e1] "*" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::times, $e1, $e2);}
| exp[e1] "%" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::mod, $e1, $e2);}
| exp[e1] "==" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::eq, $e1, $e2);}
| exp[e1] "!=" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::neq, $e1, $e2);}
| exp[e1] "<" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::lt, $e1, $e2);}
| exp[e1] ">" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::gt, $e1, $e2);}
| exp[e1] "<=" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::le, $e1, $e2);}
| exp[e1] ">=" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::ge, $e1, $e2);}
| exp[e1] "&" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::andd, $e1, $e2);}
| exp[e1] "|" exp[e2]
{$$ = make_shared<OpExp>(Arity::two, ExpOp::orr, $e1, $e2);}
| "-" exp[e] %prec UMINUS
{$$ = make_shared<OpExp>(Arity::one, ExpOp::minus, $e);}
| "!" exp[e] %prec NEG
{$$ = make_shared<OpExp>(Arity::one, ExpOp::nott, $e);}
| "(" exp[e] ")" {$$ = $e;}

%%
      
void
ModelParserGen::ModelParser::error(const ModelParserGen::location& l, const std::string& m) {
    std::stringstream ss;
    ss << "at line " << l << " " << m;
    ModelAST::on_scanner_error(ss.str());
}
