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


#-------------------------------------------------------------------------------
/*
TODO    Check that clocks have unique distribution in between transitions.
TODO    work with constants.
TODO    Decide if the module named smtsolver is really needed or if it can be
        taken into iosacompliance module.
TODO    Build input-enable transition to comply with IOSA 6th property.
TODO    Complete the counterexample tests for iosacompliance.
TODO    Test modules for parallel compatibility (disjoint output labels).
TODO    AUTOMATIC tests instead of counterexample texts that we have for the
        parser.




#-------------------------------------------------------------------------------
FIXME   Propiedad 7, chequear sat de las guardas. Y ! sat (p1 != p2) de las pos
        tomando en cuenta la g1 y g2 ... un bolonqui importante.
            El problema esta en casos como:
                    [a?] a == 9      -> t=1     ;
                    [a?] t == 1 -> a = 9;





#-------------------------------------------------------------------------------
TODO 	parametric skip whites for Parser class.
TODO 	test suit.
TODO 	reset parser method.
TODO    leak check
TODO    Idea: parser with templates
TODO    Idea: Build the AST in a single bunch of method like Accept or Expect,
        instead of distributing this work in between every grammar rule. A 
        stack may be needed for this.
TODO    Allow expressions as range limits.
TODO    Opcion para no usar sat solver.
TODO    Clean the exceptions file.
TODO    When discover a WARNING wrt IOSA compliance, give away some valuation
        explaining the mistake.
TODO    Some Parser Errors report are not very nice, they look ambiguous fix
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
