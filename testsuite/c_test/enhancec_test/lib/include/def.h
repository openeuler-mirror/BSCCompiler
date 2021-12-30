/*
 *
 * COPYRIGHT NOTICE (NOT TO BE REMOVED):
 *
 * This file, or parts of it, or modified versions of it, may not be copied,
 * reproduced or transmitted in any form, including reprinting, translation,
 * photocopying or microfilming, or by any means, electronic, mechanical or
 * otherwise, or stored in a retrieval system, or used for any purpose, without
 * the prior written permission of all Owners unless it is explicitly marked as
 * having Classification `Public'.
 *
 * Owners of this file give notice:
 *   (c) Copyright 2015-2019 by Solid Sands B.V.,
 *       Amsterdam, the Netherlands. All rights reserved.
 *   (c) Copyright 1996-2012 ACE Associated Compiler Experts bv
 *   (c) Copyright 1995-2012 ACE Associated Computer Experts bv
 * All rights, including copyrights, reserved.
 *
 * This file contains or may contain restricted information and is UNPUBLISHED
 * PROPRIETARY SOURCE CODE OF THE Owners.  The Copyright Notice(s) above do not
 * evidence any actual or intended publication of such source code.  This file
 * is additionally subject to the conditions listed in the RESTRICTIONS file
 * and is with NO WARRANTY.
 *
 * END OF COPYRIGHT NOTICE
 */

#ifndef CVAL_DEF_H
#define CVAL_DEF_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <limits.h>

#include "cvaltarget.h"

/********************************
 * The following section define SuperTest specific macros used by the
 * tests or by the diagnostic library. They are protected by #ifdefs, so
 * that they can be overruled easily in the cvaltarget.h file.
 */

#ifndef CVAL_A_LDOUBLE_PRECISION
#define CVAL_A_LDOUBLE_PRECISION CVAL_A_DOUBLE_PRECISION
#endif

#ifndef CVAL_R_LDOUBLE_PRECISION
#define CVAL_R_LDOUBLE_PRECISION CVAL_R_DOUBLE_PRECISION
#endif

/*
 *	This file does not (and should not) #include standard headers.
 *	If testfiles need them, they should include them.
 */

#ifdef CVAL_SILENT_LIBRARY
        /* Just shorthands */
    #define CVAL_NODIAG
#else
    #define CVAL_YESDIAG
#endif /* CVAL_SILENT_LIBRARY */

#ifndef CVAL_HEADER
    #ifdef CVAL_YESDIAG
        #define CVAL_HEADER(s)		CVAL_HEADER_FUNC(s)
    #else
        #define CVAL_HEADER(s)          /* Nothing here */
    #endif
#endif

#ifndef CVAL_SECTION
    #ifdef CVAL_YESDIAG
        #define CVAL_SECTION(s)		CVAL_SECTION_FUNC(s)
    #else
        #define CVAL_SECTION(s)		/* Nothing here */
    #endif
#endif

#ifndef CVAL_ENDSECTION
    #ifdef CVAL_YESDIAG
        #define CVAL_ENDSECTION()	CVAL_ENDSECTION_FUNC()
    #else
        #define CVAL_ENDSECTION()	/* Nothing here */
    #endif
#endif

#ifndef CVAL_ACTION
    #ifdef CVAL_YESDIAG
        #define CVAL_ACTION(s)		CVAL_ACTION_FUNC(s)
    #else
        #define CVAL_ACTION(s)
    #endif
#endif

#ifndef CVAL_DOTEST
    #ifdef CVAL_YESDIAG
        #define CVAL_DOTEST(do_str)	CVAL_ACTION (#do_str); do_str;
    #else
        #define CVAL_DOTEST(do_str)     do_str;
    #endif
#endif

#ifndef CVAL_NOTEST
    #ifdef CVAL_YESDIAG
        #define CVAL_NOTEST()		CVAL_NOTEST_FUNC()
    #else
        #define CVAL_NOTEST()           /* Nothing here */
    #endif
#endif

#ifndef CVAL_CASE
    #ifdef CVAL_YESDIAG
        #define CVAL_CASE(c)		CVAL_CASE_FUNC(c)
    #else
        #define CVAL_CASE(c)            (void)(c);
    #endif
#endif

#ifndef CVAL_NOCASE
    #ifdef CVAL_YESDIAG
        #define CVAL_NOCASE()		CVAL_NOCASE_FUNC()
    #else
        #define CVAL_NOCASE()           /* Nothing here */
    #endif
#endif

#ifndef CVAL_RFLAG
#define CVAL_RFLAG	1
#endif

#ifndef CVAL_VFLAG
#define CVAL_VFLAG	1
#endif

#ifndef CVAL_VERIFY
    #ifdef CVAL_NODIAG
            /* Use a library call because CVAL_EXIT is not
             * available here, and is available in library.
             */
        #define CVAL_VERIFY(cond)   CVAL_VERIFY_MINIFUNC(cond)
    #endif /* CVAL_NODIAG */
#endif /* CVAL_VERIFY */

#ifndef CVAL_VERIFY
    #define CVAL_VERIFY(be)	CVAL_VERIFY_FUNC(be, #be, __LINE__)
#endif /* CVAL_VERIFY */


#define CVAL_ARRAY_COUNT(x)	(sizeof(x) / sizeof(x[0]))

/*
 *	For backwards compatibility.
 */
#define do_exit(eval)   (cval_exit_func(eval))
#define CVAL_CONST	const

#ifndef NULLCHAR
    #define NULLCHAR	'\0'
#endif /* NULLCHAR */

/*
 * The following is used to check floating point against some other value
 * with a predefined precision.
 */
#ifndef CVAL_FC
#define CVAL_FC(f, val) cval_float_approximates((f), (val))
#endif
#ifndef CVAL_DC
#define CVAL_DC(f, val) cval_double_approximates((f), (val))
#endif
#ifndef CVAL_QC
#define CVAL_QC(f, val) cval_ldouble_approximates((f), (val))
#endif

/******************************************
 * The following section defines SuperTest specific macros to identify
 * the C or C++ version of the compiler. This is done to make it easier
 * to write version specific code in tests. (The macros defined by the
 * language standard are harder to memorize and not so orthogonal.)
 *
 * Two sets of macros exists, and from each set EXACTLY ONE macro must
 * be defined. The first set defines the current language that is compiled,
 * C or C++. It respectively contains the macros:
 *
 *      CVAL_STDC, CVAL_CXX
 *
 * The other set defines the specific version of C or C++ that is compiled -
 * C90, C99, C11, C++03, C++11, C++14 or C++17, respectively defined by:
 *
 *      CVAL_C90, CVAL_C99, CVAL_C11, CVAL_C18
 *      CVAL_CXX03, CVAL_CXX11, CVAL_CXX14, CVAL_CXX17
 *
 * Notes:
 *      - C89 and C90 are the same language.
 *      - C++03 is an amendment ("fix update") of C++98. It is treated
 *        the same in SuperTest and named CXX03.
 *      - The C language committee has published corrigenda of language
 *        revisions. They are named after the 'base' revision.
 *      - The C++ language committee does not publish corrigenda, yet
 *        C++03 is commonly considered, by the market and by compiler
 *        developers, as a corrigendum of C++98.
 *
 * The code below computes the correct setting of the macros from the
 * macros defined by the language standards. This code can be overruled
 * by setting the macros (one from each set) explicitly in the target
 * file "cvaltarget.h"
 */

            /* If not one of CVAL_STDC and CVAL_CXX is set explictly, figure
             * it out in the enclosed block.
             */
#ifndef CVAL_STDC
#ifndef CVAL_CXX

    #ifdef __cplusplus
        #define CVAL_CXX    1
    #else
        #ifdef __STDC__
            #define CVAL_STDC   1
        #else
            #error "Expected definition of one of __STDC__ or __cplusplus__ \
    not found - Explictly define CVAL_STDC or CVAL_CXX in cvaltarget.h"
        #endif
    #endif

#endif /* CVAL_CXX */
#endif /* CVAL_STDC */


    /* If not one of CVAL_C* specific version macros is set, figure it out
     * in the enclosed block.
     */
#ifndef CVAL_C90
#ifndef CVAL_C99
#ifndef CVAL_C11
#ifndef CVAL_C18
#ifndef CVAL_CXX03
#ifndef CVAL_CXX11
#ifndef CVAL_CXX14
#ifndef CVAL_CXX17

    #ifdef CVAL_CXX
        #if __cplusplus == 199711L
            #define CVAL_CXX03  1
        #elif __cplusplus == 201103L
            #define CVAL_CXX11  1
        #elif __cplusplus == 201402L
            #define CVAL_CXX14  1
        #elif __cplusplus == 201703L
            #define CVAL_CXX17  1
        #else
            #error "__cplusplus version is not recognized, set one of: \
CVAL_CXX03, CVAL_CXX11, CVAL_CXX14 or CVAL_CXX17 explicitly in cvaltarget.h"
        #endif
    #endif

    #ifdef CVAL_STDC
        #ifndef __STDC_VERSION__
            #define CVAL_C90    1
        #elif __STDC_VERSION__ == 199409L   /* C90:Corrigendum 1 */
            #define CVAL_C90    1
        #elif __STDC_VERSION__ == 199603L   /* C90:Corrigendum 2 */
            #define CVAL_C90    1
        #elif __STDC_VERSION__ == 199901L
            #define CVAL_C99    1
        #elif __STDC_VERSION__ == 201112L
            #define CVAL_C11    1
        #elif __STDC_VERSION__ == 201710L
            #define CVAL_C18    1
        #else
            #error "__STDC_VERSION__ is not recognized, set one of: \
CVAL_C90, CVAL_C99 or CVAL_C11 explicitly in cvaltarget.h"
        #endif
    #endif

#endif /* CVAL_CXX17 */
#endif /* CVAL_CXX14 */
#endif /* CVAL_CXX11 */
#endif /* CVAL_CXX03 */
#endif /* CVAL_C18 */
#endif /* CVAL_C11 */
#endif /* CVAL_C99 */
#endif /* CVAL_C90 */

#ifndef CVAL_C99_ONWARD
#ifndef CVAL_C11_ONWARD
#ifndef CVAL_C18_ONWARD
#ifndef CVAL_CXX11_ONWARD
#ifndef CVAL_CXX14_ONWARD
#ifndef CVAL_CXX17_ONWARD

    #ifdef CVAL_STDC
        #if   defined( CVAL_C18 )
            #define CVAL_C99_ONWARD
            #define CVAL_C11_ONWARD
            #define CVAL_C18_ONWARD
        #elif   defined( CVAL_C11 )
            #define CVAL_C99_ONWARD
            #define CVAL_C11_ONWARD
        #elif defined( CVAL_C99 )
            #define CVAL_C99_ONWARD
        #endif
    #endif

    #ifdef CVAL_CXX
        #if   defined( CVAL_CXX17 )
            #define CVAL_CXX11_ONWARD
            #define CVAL_CXX14_ONWARD
            #define CVAL_CXX17_ONWARD
        #elif defined( CVAL_CXX14 )
            #define CVAL_CXX11_ONWARD
            #define CVAL_CXX14_ONWARD
        #elif defined( CVAL_CXX11 )
            #define CVAL_CXX11_ONWARD
        #endif
    #endif

#endif /* CVAL_C99_ONWARD */
#endif /* CVAL_C11_ONWARD */
#endif /* CVAL_C18_ONWARD */
#endif /* CVAL_CXX11_ONWARD */
#endif /* CVAL_CXX14_ONWARD */
#endif /* CVAL_CXX17_ONWARD */

/* Undefine CVAL_HAVE_COMPLEX on C90 and C++ compilers, since C complex types
 * are only supported on C99 and later.
 */
#ifdef CVAL_HAVE_COMPLEX
    #ifdef CVAL_C90
        #undef CVAL_HAVE_COMPLEX
    #endif /* CVAL_C90 */

    #ifdef CVAL_CXX
        #undef CVAL_HAVE_COMPLEX
    #endif /* CVAL_CXX */
#endif /* CVAL_HAVE_COMPLEX */

/*************************************
 *	Declaration of library functions
 */

void	CVAL_VERBOSE (int);
void	CVAL_VERIFY_FUNC (int, const char *, int);
void    CVAL_VERIFY_MINIFUNC (int cond);
void	CVAL_ACTION_FUNC (const char *);
void	CVAL_HEADER_FUNC (const char *);
void	CVAL_SECTION_FUNC (const char *);
void	CVAL_ENDSECTION_FUNC (void);
void	CVAL_NOTEST_FUNC (void);
void	CVAL_CASE_FUNC (int);
void	CVAL_NOCASE_FUNC (void);

void            cval_exit_func (int);
void            cval_exit_nolib (int);  /* For tests with own main() */
void		cval_anyeffect (void);

int		cval_identity_int (int);
unsigned int	cval_identity_uint (unsigned int);
long		cval_identity_long (long);
unsigned long	cval_identity_ulong (unsigned long);
void		*cval_identity_pointer (void *);

/* Compatibility: Preprocessor condition intended */
#if defined( CVAL_C90 ) || \
    defined( CVAL_CXX03 )
        /* Empty Case */
#else
        /* Have long long */
    long long	   cval_identity_longlong (long long);
    unsigned long long cval_identity_ulonglong (unsigned long long);
#endif

#ifndef CVAL_NOFLOAT
float		cval_identity_float (float x);
double		cval_identity_double (double x);
long double	cval_identity_ldouble (long double x);
int		cval_float_approximates (double, double);
int		cval_double_approximates (double, double);
int		cval_ldouble_approximates (long double, long double);

#ifdef CVAL_HAVE_COMPLEX
float _Complex		cval_identity_fcmplx (float _Complex);
double _Complex		cval_identity_cmplx (double _Complex);
long double _Complex	cval_identity_lcmplx (long double _Complex);

int		cval_fcmplx_approximates (float _Complex, float _Complex);
int 		cval_cmplx_approximates (double _Complex, double _Complex);
int 		cval_lcmplx_approximates (
				long double _Complex, long double _Complex);
#endif
#endif

#ifdef __DSPC__
__fixed		cval_identity_fixed(__fixed x);
long __fixed	cval_identity_lfixed(long __fixed x);
__accum		cval_identity_accum(__accum x);
long __accum	cval_identity_laccum(long __accum x);
#endif

#ifdef __EMBEDDEDC__
signed short _Fract cval_identity_shr( signed short _Fract );
signed short _Accum cval_identity_shk( signed short _Accum );
signed _Fract cval_identity_sr( signed _Fract );
signed _Accum cval_identity_sk( signed _Accum );
signed long _Fract cval_identity_slr( signed long _Fract );
signed long _Accum cval_identity_slk( signed long _Accum );
unsigned short _Fract cval_identity_uhr( unsigned short _Fract );
unsigned short _Accum cval_identity_uhk( unsigned short _Accum );
unsigned _Fract cval_identity_ur( unsigned _Fract );
unsigned _Accum cval_identity_uk( unsigned _Accum );
unsigned long _Fract cval_identity_ulr( unsigned long _Fract );
unsigned long _Accum cval_identity_ulk( unsigned long _Accum );
#endif

    /* Temporary solution for cval_put*: these should not be used in
     * tests at all. This stuff is instrumentation for debugging, which
     * tests should not have. Instead, fail of comment
     */
#ifdef COMPILING_CVAL_LIB
    extern int	cval_put_string(const char *);
    extern void	cval_put_decimal(long);
    extern void	cval_put_hex(long);
#else
    #ifdef CVAL_YESDIAG
        extern int	cval_put_string(const char *);
        extern void	cval_put_decimal(long);
        extern void	cval_put_hex(long);
    #else
        #define	cval_put_string(s)  0
        #define	cval_put_decimal(l) /* Nothing here */
        #define	cval_put_hex(l)     /* Nothing here */
    #endif /* CVAL_YESDIAG */
#endif /* COMPILING_CVAL_LIB */


extern char*	cval_strcat (char *, const char *);
extern void     cval_reverse (char *s);
extern int	cval_strlen (const char *);
extern void	cval_itoa (int, char *);
extern char*	cval_strcpy (char *, const char *);
extern char*    cval_strncpy (char *s1, const char *s2, int n);
extern int	cval_strcmp (const char *, const char *);
extern int      cval_strncmp (const char *s1, const char *s2, int n);
extern int	cval_memcmp (const void *, const void *, int);
extern void*	cval_memcpy (void *, const void *, int);
extern void*	cval_memset (void *, int, int);
extern void	cval_exit_nolib (int);

#define cval_never()	cval_identity_int(0)
#define cval_always()	cval_identity_int(1)

#ifndef MAIN
    #ifdef CVAL_YESDIAG
            /* Create the entry TEST__MAIN that is called by the real
             * main in report.o the supertest library stlib.a
             */
        void TEST__MAIN (void);
        # define MAIN	void TEST__MAIN(void)
    #else
            /* Place the main() function inline in the test. So main
             * from library is not used, linker should not include
             * report.o
             */
        #define MAIN                    \
            void TEST__MAIN (void);     \
            int main (void) {           \
                TEST__MAIN ();          \
                cval_exit_func (0);     \
                return 0;               \
            }                           \
            void TEST__MAIN (void)
    #endif /* CVAL_YESDIAG */
#endif

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /* CVAL_DEF_H */
