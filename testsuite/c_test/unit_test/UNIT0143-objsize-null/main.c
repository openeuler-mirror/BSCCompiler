/*
 * (c) Copyright 2015-2019 by Solid Sands B.V.,
 *     Amsterdam, the Netherlands. All rights reserved.
 *     Subject to conditions in the RESTRICTIONS file.
 * (c) Copyright 1989,2005-2006,2009 ACE Associated Computer Experts bv
 * (c) Copyright 2005-2006,2009 ACE Associated Computer Experts bv
 * All rights reserved.  Subject to conditions in RESTRICTIONS file.
 */

#include "def.h"

/* Compatibility: Preprocessor condition intended */
#if defined( CVAL_STDC )
#include <stdlib.h>
#include <limits.h>
#else
#include <cstdlib>
#include <climits>
#endif
#include <string.h>
void main()
{
	int    	res, res2;
	wchar_t	wc;
	wc = (wchar_t) 0;
	printf("%d\n", __builtin_object_size (NULL, 1));
	res = wctomb(NULL, wc);
	wc = (wchar_t) 0177;
	res2 = wctomb(NULL, wc);
}
