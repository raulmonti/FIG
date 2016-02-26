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

USE "sh build.sh"

#############################################
# USE EXAMPLE
#############################################

Read main.cpp file for an example.

#############################################
# TODO
#############################################

TODO    When and how to delete static class members as GLOBAL_MODEL_AST for
        example.
        Change * for shared_ptr in static members to avoid memory leaks.
TODO    Precompiler might modify line sizes when replacing the name of a
        constant with its value.
TODO    Unify first parsing and precompiling.
TODO    Specialized exception for reporting modeling mistakes to the user.
TODO    Full syntactic support for Carlos clock distributions.

DONE    Capitalize modules names.
DONE    Change "end module" for "endmodule" in our syntax.
DONE    Check that clocks have unique distribution in between transitions.
DONE    work with constants.
DONE    Decide if the module named smtsolver is really needed or if it can be
        taken into iosacompliance module.
DONE    Parser clear() method.
TODO    Check all the constants resolution process.
DONE    Build input-enable transition to comply with IOSA 6th property.
TODO    Complete the counterexample tests for iosacompliance.
TODO    Test modules for parallel compatibility (disjoint output labels).
DONE    AUTOMATIC tests instead of counterexample texts that we have for the
        parser.

TODO    Improve efficiency and correctness by using const when needed and avoid
        copying when not needed.
DONE    Initialize variables with constant expressions and not only numbers.
DONE    Bad typing should quit the parser, as well as bad naming. 
TODO    Verify initializations out of range for variables.


#-------------------------------------------------------------------------------
FIXME   Propiedad 7, chequear sat de las guardas. Y ! sat (p1 != p2) de las pos
        tomando en cuenta la g1 y g2 ... un bolonqui importante.
            El problema esta en casos como:
                    [a?] a == 9      -> t=1     ;
                    [a?] t == 1 -> a = 9;





#-------------------------------------------------------------------------------
TODO 	parametric skip whites for Parser class.
TODO 	test suit.
DONE 	reset parser method.
TODO    leak check
TODO    Idea: parser with templates
TODO    Idea: Build the AST in a single bunch of method like Accept or Expect,
        instead of distributing this work in between every grammar rule. A 
        stack may be needed for this.
DONE    Allow expressions as range limits.
TODO    Opcion para no usar sat solver.
DONE    Clean the exceptions file.
TODO    When discover a WARNING wrt IOSA compliance, give away some valuation
        explaining the mistake.
DONE    Some Parser Errors report are not very nice, they look ambiguous fix
        them.


#-------------------------------------------------------------------------------
DONE    Iosa compliance with property 4. Check 'iosacompliance.cpp'.
DONE    add limit conditions for integer values when testing for exahusted
        clocks and other properties for IOSA.
DONE    In parser TEST and TESTB are strange, check them out and remove them
        if they are not really needed.
DONE    Should not be able to ask for clock values nor do any comparison against
        them.
DONE    Presendencia entre - y > anda mal :S
DONE    Dar rangos a las variables.
DONE    Cambiar >> por  ->, y : es parte de la seccion del reloj habilitador
DONE    Propiedad 3, agregar unsat de las guardas.
DONE    [a?] -> (t'=1); is working wrong when t is integer.
*/
