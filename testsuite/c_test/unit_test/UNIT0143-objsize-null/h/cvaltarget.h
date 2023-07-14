/*
 * Always included file that describes the implementation's data model.
 */

#ifndef CVAL_TARGET_H
#define CVAL_TARGET_H

/*
 * SuperTest Usage configuration
 */

        /* Uncomment this #define to compile tests without the diagnostic
         * library. It makes tests smaller and tests do not generate any
         * output, which is useful on bare-metal targets. See also CVAL_EXIT()
         * below.
         */
/* #define CVAL_SILENT_LIBRARY  */

        /* Remove this #define when C complex types are not supported. For C90
         * or C++ compilers, this macro will automatically be undefined, since
         * C complex types are only supported by C99 and later versions of C.
         */
/* #define CVAL_HAVE_COMPLEX */

        /* If the compiler does not support floating point,
         * this #define skips floating point support in the
         * SuperTest diagnostic library. It does NOT automatically
         * skip tests with floating point. To achieve that,
         * subtract any or all of the predefined test sets
         * float/double/longdouble in SETS/ in the test set
         * definition.
         */
/* #define CVAL_NOFLOAT		*/

        /* The SuperTest diagnostic library reads the arguments to the
         * main() function called argc and argv. If they are inconsistent
         * (happens rarely and only on embedded systems), this can be
         * turned off by enabling the #define CVAL_NO_ARGC_ARGV.
         */
/* #define CVAL_NO_ARGC_ARGV	*/

        /*
         * Uncomment this define to use static arrays instead
         * of dynamically allocated memory in the test suite
         * "optimize/". This is useful if your target does
         * not support dynamically allocated memory.
         */
/* #define CVAL_STATIC_MEMORY */

/*
 * SuperTest has a very small footprint. When CVAL_SILENT_LIBRARY
 * is set, it only needs to return its exit value. It uses the
 * macro CVAL_EXIT(x) to do this. This macro can be redefined here
 * its default definition of 'exit(x)' is not correct.
 *
 * If CVAL_SILENT_LIBRARY is not defined (which is the default),
 * then additionally the tests use CVAL_PUTCHAR(x) to print
 * diagnostic output. It can be redefined here is 'putchar(x)'
 * is not the right way to do this.
 */
#ifdef COMPILING_CVAL_LIB

    #include <stdlib.h>
    #define CVAL_EXIT(x)	exit(x)

    #ifdef CVAL_SILENT_LIBRARY
            /* No output needed for CVAL_SILENT_LIBRARY, no
             * stdio.h included.
             */
        #define CVAL_PUTCHAR(x)	/* No output */
    #else
        #include <stdio.h>
        #define CVAL_PUTCHAR(x)	putchar(x)
    #endif

#endif

/*
 * Target dependent values.
 *
 * These values must be adapted to the data-model of the target architecture.
 * This default file target/cvaltarget.h in SuperTest is set to work for
 * the common 32 and 64 bit data models. For many not-so mainstream
 * processors, they may need to be modified.
 * If these value are not set up to match the target, SuperTest will fail
 * with an error "Null test failed", when testing "suite/FIRST/test0.c"
 */

#include <limits.h>     /* Only needed for INT_MAX==LONG_MAX compare */
#if INT_MAX == LONG_MAX
    #define CVAL_LONGSIZE       32      /* The size of a long in bits. */
    #define CVAL_POINTERSIZE    32      /* The size of a void* in bits. */
#else
    #define CVAL_LONGSIZE       64      /* The size of a long in bits. */
    #define CVAL_POINTERSIZE    64      /* The size of a void* in bits. */
#endif

#define CVAL_LONGLONGSIZE       64      /* The size of a long long in bits. */
#define CVAL_INTSIZE            32      /* The size of an int in bits. */
#define CVAL_SHORTSIZE          16      /* The size of a short in bits.     */
#define CVAL_CHARSIZE           8       /* The size of a char in bits.      */
#define CVAL_WCHARSIZE          32      /* The size of wchar_t in bits.     */

/* #define ONECOMPL	*/		/* machine is one's complement */
#define TWOCOMPL			/* machine is two's complement */

/*
 * IEEE-754 example values
 */
#define FLOAT_MANTISSA		24
#define DOUBLE_MANTISSA		53

/*
 * Floating point absolute and relative precision for float and double.
 */

#ifdef CVAL_FP_IEEE
/*
 * These values should be attainable for IEEE-754.
 * Accumulating errors in eg math functions might exceed the bounds.
 * From considerations of quality, an attempt should be made to fix such
 * math functions.
 * Otherwise these values might be adapted.
 */
#define CVAL_A_FLOAT_PRECISION		5E-6
#define CVAL_R_FLOAT_PRECISION		5E-7

#define CVAL_A_DOUBLE_PRECISION		1E-14
#define CVAL_R_DOUBLE_PRECISION		1E-15

#define CVAL_A_LDOUBLE_PRECISION	1E-14
#define CVAL_R_LDOUBLE_PRECISION	1E-15

#else	/* ! CVAL_FP_IEEE */

/*
 * These are well within ISO-C minimum precision requirements for
 * the accuracy of floating point representation.
 */
#define CVAL_A_FLOAT_PRECISION		5E-4
#define CVAL_R_FLOAT_PRECISION		5E-5

#define CVAL_A_DOUBLE_PRECISION		5E-8
#define CVAL_R_DOUBLE_PRECISION		5E-9

#define CVAL_A_LDOUBLE_PRECISION	5E-8
#define CVAL_R_LDOUBLE_PRECISION	5E-9

#endif	/* CVAL_FP_IEEE */

/*
 * The locale category LC_CTYPE affects the behavior of the
 * character handling functions and the multibyte and wide
 * character functions. To test UTF-16 and UTF-32 Unicode
 * utilities (Section C11:7.28) a specific UTF-8 encoding
 * locale should be set. The default locale "C" commonly
 * does not provide UTF-8 and the existence of any other
 * locale is user defined. In this case, the well-known
 * "en_US.UTF-8" locale is selected, however, any other
 * UTF-8 compliant locale would be also suitable.
 */
#define CVAL_LOCALE     "en_US.UTF-8"

#endif	/* CVAL_TARGET_H */

