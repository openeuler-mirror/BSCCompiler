

#define bool _Bool
#define true 1
#define false 0
#define NULL (void *) 0

#include "structs.h"
#include "prototypes.h"
#include "global_vars.h"

/*
 * TOTAL functions:   17
 * VOID functions:    0
 * NO-VOID functions: 17
 */

/*
 * return type: signed char
 * param_0 type: double
 */
signed char func17(double param_0) {
	double local_double_1 = /* LITERAL */ (double) 9.105397335719785e+307;
	signed short int local_signed_short_int_1 = /* LITERAL */ (signed short int) -30262;
	unsigned long int local_unsigned_long_int_1 = 2985533704UL;
	bool local_bool_1 = false;
	struct struct1  *local_pointer_struct_struct1_1 = NULL;
	struct struct3  local_struct_struct3_1;
	struct struct2  *local_pointer_struct_struct2_1 = NULL;
	signed char local_signed_char_1 = /* LITERAL */ (signed char) -86;

	do { 
		(global_array_9_float_1)[(local_double_1) <= (global_unsigned_char_1)] *= ((local_signed_short_int_1) & (global_unsigned_long_long_int_1)) >> ((local_unsigned_long_int_1) ^ (/* LITERAL */ (signed char) -26));
		global_unsigned_char_1 |= local_bool_1;
		/* stmt invocation */ func7(*(local_pointer_struct_struct1_1), (115U) / (local_bool_1), (local_struct_struct3_1).signed_int_0, 435471674L, /* expr invocation */ func8(40072U), global_signed_char_1);
	} while ((local_bool_1) && ((global_array_3_signed_long_long_int_1) <= ((local_pointer_struct_struct2_1)->pointer_signed_long_long_int_3)));
	--(global_bool_1);
	(/* LITERAL */ (signed short int) -1145) | (local_signed_char_1);
	return global_signed_char_1;
}

