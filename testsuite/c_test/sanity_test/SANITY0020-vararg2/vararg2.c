/*
 * Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
 *
 * Licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include <stdarg.h>

static double f (float a, ...);
static double (*fp) (float a, ...);

main () {
  fp = f;
  if (fp ((float) 1, (double)2) != 3.0) {
    abort ();
  }
  exit (0);
}

static double
f (float a, ...) {
  va_list ap;

  va_start(ap, a);
  float i = va_arg(ap, double);
  return (double)a + i;
}
