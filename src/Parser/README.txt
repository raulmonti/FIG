#############################################
# PARSER MODULE FOR FIG
#############################################

#############################################
# REQUIRES
#############################################

1) flex (tested working with version 2.5.35)
2) g++  (tested working with version 4.8.4)

#############################################
# COMPILING (and running test):
#############################################
// WARNING !!!!!!
// Use build.sh !!!!!!!!!
// !!!!!!

// The following compilation models are broken:

1) flex++ lexer.l 
2) Remove line 22 of lex.yy.cc (#define yyFLexLexer yyFLexLexer)
3) g++ -std=c++11 -c lex.yy.cc ast.cpp && g++ -c parser.cpp -std=c++11 && g++ -std=c++11 main.cpp lex.yy.o parser.o ast.o -o Parser
4) ./Parser test.fig

* Or the fast way:
flex++ lexer.l && sed -i '/#define yyFlexLexer yyFlexLexer/d' ./lex.yy.cc && g++ -std=c++11 -c lex.yy.cc ast.cpp && g++ -c parser.cpp -std=c++11 && g++ -std=c++11 main.cpp lex.yy.o parser.o ast.o -o Parser


#############################################
# USE EXAMPLE
#############################################

Read main.cpp file for an example.

#############################################
# TODO
#############################################

/*
TODO 	parametric skip whites for Parser class.
TODO 	test suit.
TODO 	reset parser method.
TODO    leak check
TODO    Idea: parser with templates
TODO    Idea: Build the AST in a single bunch of method like Accept or Expect,
        instead of distributing this work in between every grammar rule. A 
        stack may be needed for this.
TODO    Idea: Exception handling using numbers and table with 
TODO    Check label section in ioscompilance or better remove label section as
        it seams useless.
TODO    Allow expressions as range limits.
FIXME   Syntax shouldn't include the explicit reset of clocks. We should ensure
        condition 4 for IOSA reseting clocks that are about to be used.
FIXME   Presendencia entre - y > anda mal :S
FIXME   Bool a bool, Int a int, variables tipo clock y sacar seccion clock y
        dar rangos a las variables.
DONE    Cambiar >> por  ->, y : es parte de la seccion del reloj habilitador
TODO    Opcion para no usar sat solver.
DONE    Propiedad 3, agregar unsat de las guardas.
FIXME   Propiedad 4, arreglar todo , ver foto.
TODO    Propiedad 6, construir imput enableness transitions.
FIXME   Propiedad 7, chequear sat de las guardas.
FIXME   Should not be able to ask for clock values nor do any comparison against
        them.
FIXME   Remove LABELS section.
*/
