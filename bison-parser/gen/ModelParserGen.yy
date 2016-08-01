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
#include <string>
#include "ModelAST.h"
  
  class ModelBuilder;
  using namespace ASTNode;
}

%param {ModelBuilder &builder}
%locations

%code
{
  #include "ModelBuilder.h"
}

%define api.token.prefix {TOK_}
%token
END  0  "end of file"
COMMA ";"
MODULE "module"
ENDMODULE "endmodule"
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
%type <Model*> model
%type <Decl*> global_decl
%type <Exp*>  exp
%type <Dist*> dist
%type <vector<Effect*>> effects
%type <Location*> location
%type <Exp*> guard
%type <Action*> action
%type <vector<Exp*>> exp_seq
%type <vector<Exp*>> array_init
%type <Decl*> decl
%type <ModuleBody*> module_body
%type <Type> type

%%
%start begin;

begin: model[m] {builder.set_result($m);}

model: global_decl[d] ";"
{$$ = new Model($d);}
| model[m] global_decl[d] ";"
{$m->add_decl($d); $$ = $m;}
| "module" "id"[id] module_body[b] "endmodule"
{$$ = new Model($id, $b);}
| model[m] "module" "id"[id] module_body[b] "endmodule"
{$m->add_module($id, $b); $$ = $m;}

type: "int"
{$$ = Type::tint;}
| "bool"
{$$ = Type::tbool;}
| "float"
{$$ = Type::tfloat;}

global_decl: "const" type[t] "id"[id] "=" exp[e]
{$$ = new Decl($t, $id, $e); }
| "const" "id"[id] "[" exp[size] "]" ":" "[" exp[lower] ".." exp[upper] "]" "init" array_init[seq]
{$$ = new Decl(Type::tint, $id, $size, $lower, $upper, $seq);}
| "const" "id"[id] "[" exp[size] "]" ":" type[t] "init" array_init[seq]
{$$ = new Decl($t, $id, $size, $seq);}
| "const" type[t] "id"[id] "[" exp[size] "]" "=" array_init[seq]
{$$ = new Decl($t, $id, $size, $seq);}

module_body: decl[d] ";"
{$$ = new ModuleBody($d);}
| action[a] ";"
{$$ = new ModuleBody($a);}
| module_body[mb] decl[d] ";"
{$mb->add_decl($d); $$ = $mb;}
| module_body[mb] action[a] ";"
{$mb->add_action($a); $$ = $mb;}

decl: "id"[id] ":" "[" exp[low] ".." exp[upper] "]"
{$$ = new Decl(Type::tint, $id, $low, $upper);}
| "id"[id] ":" "[" exp[low] ".." exp[upper] "]" "init" exp[e]
{$$ = new Decl(Type::tint, $id, $low, $upper, $e);}
| "id"[id] ":" "bool" "init" exp[e]
{$$ = new Decl(Type::tbool, $id, $e);}
| "id"[id] ":" "clock"
{$$ = new Decl(Type::tclock, $id);}
| "id"[id] "[" exp[size] "]" ":"
"[" exp[lower] ".." exp[upper] "]" "init" array_init[seq]
{$$ = new Decl(Type::tint, $id, $size, $lower, $upper, $seq);}
| "id"[id] "[" exp[size] "]" ":" "bool" "init" array_init[seq]
{$$ = new Decl(Type::tbool, $id, $size, $seq);}

array_init: "{" exp_seq[es] "}"
{$$ = $es;}
| exp[e]
{$$ = vector<Exp*>{$e};}

exp_seq: exp_seq[es] "," exp[e]
{$$ = concat($es, vector<Exp*>{$e});}
| exp[e]
{$$ = vector<Exp*> {$e};}

action: "[" "id"[id] "?" "]" guard[e] "->" effects[eff]
{$$ = new Action($id, LabelType::in, $e, $eff);}
| "[" "id"[id] "!" "]" guard[e] "@" location[loc] "->" effects[eff]
{$$ = new Action($id, LabelType::out, $e, $loc, $eff);}
| "[" "]" guard[e] "@" location[loc] "->" effects[eff] //see [] in atm_queue.sa
{$$ = new Action(LabelType::empty, $e, $loc, $eff);}
| "[" "id"[id] "!" "!" "]" guard[e] "->" effects[eff]
{$$ = new Action($id, LabelType::commited, $e, $eff);}

guard: %empty
{$$ = new BConst(true);}
| exp[e]
{$$ = $e;}

effects: %empty
{$$ = vector<Effect*>(); }
|"(" location[loc] "'" "=" exp[e] ")"
{$$ = vector<Effect*>{new Effect($loc, $e)};}
| "(" location[loc] "'" "=" dist[d] ")"
{$$ = vector<Effect*>{new Effect($loc, $d)};}
| effects[e1] "&" effects[e2]
{$$ = concat($e1, $e2);}

dist: "erlang" "(" exp[e1] "," exp[e2] ")"
{$$ = new Dist(DistType::erlang,  Arity::two, $e1, $e2);}
| "normal" "(" exp[e1] "," exp[e2] ")"
{$$ = new Dist(DistType::normal,  Arity::two, $e1, $e2);}
| "uniform" "(" exp[e1] "," exp[e2] ")"
{$$ = new Dist(DistType::uniform, Arity::two, $e1, $e2);}
| "exponential" "(" exp[e] ")"
{$$ = new Dist(DistType::exponential, Arity::one, $e);}

location: "id"[id] {$$ = new Location($id);}
| "id"[id] "[" exp[e] "]" {$$ = new Location($id, $e);}
		 
// Note on expressions types: a separation between int-exp and bool-exp
// introduces reduce/reduce conflicts
// with the "identifier" rules (how to reduce an
// identifier? as a bexp or as a iexp)?
// Seems to be convenient to join them since 
// we have to do typechecking anyway.
// Expressions of type float are only allowed in distribution's parameters,
// That should be checked during type-checking.

exp : location[loc]
{$$ = new LocExp($loc);}
| INTL[i]
{$$ = new IConst($i);}
| FLOATL[i]
{$$ = new FConst($i);}
| "true"
{$$ = new BConst(true);}
| "false"
{$$ = new BConst(false);}
| exp[e1] "+" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::plus, $e1, $e2);}
| exp[e1] "-" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::minus, $e1, $e2);}
| exp[e1] "/" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::div, $e1, $e2);}
| exp[e1] "*" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::times, $e1, $e2);}
| exp[e1] "%" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::mod, $e1, $e2);}
| exp[e1] "==" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::eq, $e1, $e2);}
| exp[e1] "!=" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::neq, $e1, $e2);}
| exp[e1] "<" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::lt, $e1, $e2);}
| exp[e1] ">" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::gt, $e1, $e2);}
| exp[e1] "<=" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::le, $e1, $e2);}
| exp[e1] ">=" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::ge, $e1, $e2);}
| exp[e1] "&" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::andd, $e1, $e2);}
| exp[e1] "|" exp[e2]
{$$ = new OpExp(Arity::two, ExpOp::orr, $e1, $e2);}
| "-" exp[e] %prec UMINUS
{$$ = new OpExp(Arity::one, ExpOp::minus, $e);}
| "!" exp[e] %prec NEG
{$$ = new OpExp(Arity::one, ExpOp::nott, $e);}
| "(" exp[e] ")" {$$ = $e;}

%%

void
ModelParserGen::ModelParser::error(const ModelParserGen::location& l, const std::string& m) {
  std::cerr << "Error in location " << l << " : " << m << std::endl;
}

void
ModelBuilder::set_result(Model *model) {
    this->model = model;
}
