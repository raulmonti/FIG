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
    void save_location(shared_ptr<ModelAST> m, location location);
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
LOGNORMAL "lognormal"
UNIFORM "uniform"
EXP "exponential"
WEIBULL "weibull"
GAMMA "gamma"
RAYLEIGH "rayleigh"
INIT "init"
EQQ "=="
NEQ "!="
LT "<"
GT ">"
LE "<="
GE ">="
IMPLY "=>"
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
FLOOR "floor"
CEIL  "ceil"
SGN "sgn"
ABS "abs"
MIN "min"
MAX "max"
LOG "log"
POW "pow"
UNDERSCORE "_"
;

%right "=>"
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
%type  <shared_ptr<Exp>>  exp
%type  <shared_ptr<Dist>> dist
%type  <shared_vector<Effect>> effects
%type  <shared_ptr<Location>> location
%type  <shared_ptr<Exp>> guard
%type  <shared_ptr<TransitionAST>> transition
%type  <shared_vector<Exp>> exp_seq
%type  <shared_ptr<Decl>> decl
%type  <shared_ptr<Decl>> decl_body
%type  <shared_ptr<ModuleAST>> module
%type  <Type> type
%type  <vector<DeclQualifier>> qualifierlist
%type  <DeclQualifier> qualifier
%type  <shared_ptr<Prop>> prop
%type  <shared_vector<Prop>> proplist

%%
%start begin;

begin: model[m] {*result = $m; save_location(*result, @m);}
|  model[m] "properties" proplist[v] "endproperties"
{auto &res = $m; res->add_props($v); *result = res; save_location(*result, @m);}
|  proplist[v]
{//assert model is already created
    assert(*result != nullptr);
    shared_ptr<Model> model = static_pointer_cast<Model>(*result);
    model->add_props($v);
    save_location(model, @v);
}

model: decl[d] ";"
{$$ = make_shared<Model>($d); save_location($$, @$);}
| model[m] decl[d] ";"
{$m->add_decl($d); $$ = $m; save_location($$, @$);}
| "module" "id"[id] module[b] "endmodule"
{$b->set_name($id);
    $$ = make_shared<Model>($b); save_location($$, @$);}
| model[m] "module" "id"[id] module[b] "endmodule"
{
    string &id = $id;
    shared_ptr<Model> model = $m;
    if (model->has_module(id)) {
	std::cerr << "Two modules with the same name " + id << std::endl;
	exit(1);
    } else {
        $b->set_name(id);
        $m->add_module($b);
	$$ = std::move($m);
    }
    save_location($$, @$);
}

prop: "P" "(" exp[l] "U" exp[r] ")" 
{ $$ = make_shared<TransientProp>($l, $r); save_location($$, @$);}
| "S" "(" exp[r] ")"
{ $$ = make_shared<RateProp>($r); save_location($$, @$);}

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


module: decl[d] ";"
{$$ = make_shared<ModuleAST>($d); save_location($$, @$);}
| transition[a] ";"
{$$ = make_shared<ModuleAST>($a);}
| module[mb] decl[d] ";"
{$mb->add_decl($d); $$ = $mb; save_location($$, @$);}
| module[mb] transition[a] ";"
{$mb->add_transition($a); $$ = $mb; save_location($$, @$);}

qualifier : "const" {$$ = DeclQualifier::constant;}

qualifierlist :  qualifierlist[ql] qualifier[q]
{$$ = $ql; $$.push_back($q);}
| %empty
{$$ = std::vector<DeclQualifier>();}

decl: qualifierlist[ql] decl_body[d] {
    $$ = $d;
    for (DeclQualifier q : $ql) {
        $$->add_qualifier(q);
    }
}

decl_body:
"id"[id] ":" "[" exp[low] ".." exp[upper] "]"
{$$ = make_shared<RangedDecl>($id, $low, $upper); save_location($$, @$);}
| "id"[id] ":" "[" exp[low] ".." exp[upper] "]" "init" exp[e]
{$$ = make_shared<RangedDecl>($id, $low, $upper, $e);
    save_location($$, @$);}
| "id"[id] ":" type[t] "init" exp[e]
{$$ = make_shared<InitializedDecl>($t, $id, $e); save_location($$, @$);}
| type[t] "id"[id] "=" exp[e]
{$$ = make_shared<InitializedDecl>($t, $id, $e); save_location($$, @$);}
| "id"[id] ":" "clock"
{$$ = make_shared<ClockDecl>($id); save_location($$, @$);}
| "id"[id] "[" exp[size] "]" ":"
"[" exp[lower] ".." exp[upper] "]" "init" exp[e]
{$$ = make_shared<RangedInitializedArray>($id, $size, $lower, $upper, $e);}
| "id"[id] "[" exp[size] "]" ":"
"[" exp[lower] ".." exp[upper] "]" "init" "{" exp_seq[seq] "}"
{$$ = make_shared<RangedMultipleInitializedArray>
            ($id, $size, $lower, $upper, $seq);}
| "id"[id] "[" exp[size] "]" ":" type[t] "init" exp[e]
{$$ = make_shared<InitializedArray>($t, $id, $size, $e);}
| "id"[id] "[" exp[size] "]" ":" type[t] "init" "{" exp_seq[seq] "}"
{$$ = make_shared<MultipleInitializedArray>($t, $id, $size, $seq);}

exp_seq: exp_seq[es] "," exp[e]
{$$ = concat($es, shared_vector<Exp>{$e});}
| exp[e]
{$$ = shared_vector<Exp>{$e};}

transition: "[" "id"[id] "?" "]" guard[e] "->" effects[eff]
{$$ = make_shared<InputTransition>($id, $e, $eff);
save_location($$, @$);}
| "[" "id"[id] "!" "]" guard[e] "@" location[loc] "->" effects[eff]
{$$ = make_shared<OutputTransition>($id, $e, $eff, $loc);
    save_location($$, @$);}
| "[" "]" guard[e] "@" location[loc] "->" effects[eff]
{$$ = make_shared<TauTransition>($e, $eff, $loc);
    save_location($$, @$);}
| "[" "_" "?" "]" guard[e]  "->" effects[eff]
{$$ = make_shared<WildcardInputTransition>($e, $eff);
    save_location($$, @$);}
| "[" "id"[id] "!" "!" "]" guard[e] "->" effects[eff]
{$$ = make_shared<OutputCommittedTransition>($id, $e, $eff);
    save_location($$, @$);}
| "[" "id" [id] "?" "?" "]" guard[e] "->" effects[eff]
{$$ = make_shared<InputCommittedTransition>($id, $e, $eff); };


guard: %empty
{$$ = make_shared<BConst>(true); save_location($$, @$);}
| exp[e]
{$$ = $e;}

effects: %empty
{$$ = shared_vector<Effect>(); }
|"(" location[loc] "'" "=" exp[e] ")"
{$$ = shared_vector<Effect>{make_shared<Assignment>($loc, $e)};}
| "(" location[loc] "'" "=" dist[d] ")"
{$$ = shared_vector<Effect>{make_shared<ClockReset>($loc, $d)};}
| effects[e1] "&" effects[e2]
{$$ = concat($e1, $e2);}

dist: "erlang" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<MultipleParameterDist>(DistType::erlang, $e1, $e2);
save_location($$, @$);}
| "normal" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<MultipleParameterDist>(DistType::normal, $e1, $e2);
    save_location($$, @$);}
| "uniform" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<MultipleParameterDist>(DistType::uniform, $e1, $e2);
    save_location($$, @$);}
| "exponential" "(" exp[e] ")"
{$$ = make_shared<SingleParameterDist>(DistType::exponential, $e);
    save_location($$, @$);}
| "lognormal" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<MultipleParameterDist>(DistType::lognormal, $e1, $e2);
    save_location($$, @$);}
| "weibull" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<MultipleParameterDist>(DistType::weibull, $e1, $e2);
    save_location($$, @$);}
| "gamma" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<MultipleParameterDist>(DistType::gamma, $e1, $e2);
    save_location($$, @$);}
| "rayleigh" "(" exp[e] ")"
{$$ = make_shared<SingleParameterDist>(DistType::rayleigh, $e);
    save_location($$, @$);}

location: "id"[id]
{$$ = make_shared<Location>($id);
    save_location($$, @$);}
| "id"[id] "[" exp[e] "]"
{$$ = make_shared<ArrayPosition>($id, $e);
    save_location($$, @$);}
		 
// Note on expressions types: a separation between int-exp and bool-exp
// introduces reduce/reduce conflicts
// with the "identifier" rules (how to reduce an
// identifier? as a bexp or as a iexp)?
// Seems to be convenient to join them since 
// we have to do typechecking anyway.
// Expressions of type float are only allowed in distribution's parameters,
// That should be checked during type-checking.

exp : location[loc]
{$$ = make_shared<LocExp>($loc); save_location($$, @$);}
| INTL[i]
{$$ = make_shared<IConst>($i); save_location($$, @$);}
| FLOATL[i]
{$$ = make_shared<FConst>($i); save_location($$, @$);}
| "true"
{$$ = make_shared<BConst>(true); save_location($$, @$);}
| "false"
{$$ = make_shared<BConst>(false); save_location($$, @$);}
| exp[e1] "+" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::plus, $e1, $e2);
    save_location($$, @$);}
| exp[e1] "-" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::minus, $e1, $e2);
    save_location($$, @$);}
| exp[e1] "/" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::div, $e1, $e2);
    save_location($$, @$);}
| exp[e1] "*" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::times, $e1, $e2);
    save_location($$, @$);}
| exp[e1] "%" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::mod, $e1, $e2);
    save_location($$, @$);}
| exp[e1] "==" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::eq, $e1, $e2);
    save_location($$, @$);}
| exp[e1] "!=" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::neq, $e1, $e2);
    save_location($$, @$);}
| exp[e1] "<" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::lt, $e1, $e2);
    save_location($$, @$);}
| exp[e1] ">" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::gt, $e1, $e2);
    save_location($$, @$);}
| exp[e1] "<=" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::le, $e1, $e2);
    save_location($$, @$);}
| exp[e1] ">=" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::ge, $e1, $e2);
    save_location($$, @$);}
| exp[e1] "&" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::andd, $e1, $e2);
    save_location($$, @$);}
| exp[e1] "=>" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::implies, $e1, $e2);
    save_location($$, @$);}
| exp[e1] "|" exp[e2]
{$$ = make_shared<BinOpExp>(ExpOp::orr, $e1, $e2);
    save_location($$, @$);}
| "log" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<BinOpExp>(ExpOp::log, $e1, $e2);
    save_location($$, @$);}
| "pow" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<BinOpExp>(ExpOp::pow, $e1, $e2);
    save_location($$, @$);}
| "min" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<BinOpExp>(ExpOp::min, $e1, $e2);
    save_location($$, @$);}
| "max" "(" exp[e1] "," exp[e2] ")"
{$$ = make_shared<BinOpExp>(ExpOp::max, $e1, $e2);
    save_location($$, @$);}
| "-" exp[e] %prec UMINUS
{$$ = make_shared<UnOpExp>(ExpOp::minus, $e);
    save_location($$, @$);}
| "!" exp[e] %prec NEG
{$$ = make_shared<UnOpExp>(ExpOp::nott, $e);
    save_location($$, @$);}
| "floor" exp[e] %prec UMINUS
{$$ = make_shared<UnOpExp>(ExpOp::floor, $e);
        save_location($$, @$);}
| "ceil" exp[e] %prec UMINUS
{$$ = make_shared<UnOpExp>(ExpOp::ceil, $e);
        save_location($$, @$);}
| "abs" exp[e] %prec UMINUS
{$$ = make_shared<UnOpExp>(ExpOp::abs, $e);
    save_location($$, @$);}
| "sgn" exp[e] %prec UMINUS
{$$ = make_shared<UnOpExp>(ExpOp::sgn, $e);
    save_location($$, @$);}
| "(" exp[e] ")"
{$$ = $e; save_location($$, @$);}

%%
      
void
ModelParserGen::ModelParser::error(const ModelParserGen::location& l,
                                   const std::string& m) {
    std::stringstream ss;
    ss << "at " << l << " " << m;
    ModelAST::on_scanner_error(ss.str());
}

inline void save_location(shared_ptr<ModelAST> m, location loc) {
    shared_ptr<location> ploc = make_shared<location>(loc);
    m->set_location(ploc);
}
