static char SccsId[] = "$Id: for_13.c,v 1.8 2002/06/02 03:03:09 cdg Exp $";

static char copyright[] =
    "COPYRIGHT (C) 1990-2002 NULLSTONE CORPORATION, ALL RIGHTS RESERVED.";

/*
 *  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NULLSTONE CORPORATION
 *  AND IS CONFIDENTIAL INFORMATION OF NULLSTONE CORPORATION.
 *  The copyright notice above does not evidence any
 *  actual or intended publication of such source code.
 *  Use, duplication, and disclosure are subject to license restrictions.
 */

/*
 *  F: $Id: for_13.c,v 1.8 2002/06/02 03:03:09 cdg Exp $
 *
 *  T: Template file for creating Nullstone tests.
 *
 *  A: This file can be used as a template for creating Nullstone
 *  A: tests.
 *
 *  D: unsigned char, unsigned short, unsigned int, unsigned long
 *
 *  W: 8+?,16+?,16+?,32+?,0,0,0,0
 *
 *  V: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
 */

#include <limits.h>
#ifndef T
#define T UINT
#endif /* T */

#ifndef V
#define V 0
#endif /* V */

#define TYPE unsigned char

__attribute__((noinline)) unsigned char gen_uchar (unsigned char X) {
  return X;
}

int kernel ()
{
  TYPE i;
  TYPE count;
  TYPE start;
  TYPE stop;

  start = gen_uchar (255 - V);
  stop = gen_uchar (0);

  count = 0;
  for (i = start; i != stop; i++)
    count++;
  if (count == (V + 1))
  {
    return 0;
  }
  return 1;
}

int main() {
  return kernel ();
}

