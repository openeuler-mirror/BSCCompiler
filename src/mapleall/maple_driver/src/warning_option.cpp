/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include "driver_options.h"

namespace opts {

/* ##################### BOOL Options ############################################################### */

maplecl::Option<bool> version({"--version", "-v"},
    "  --version [command]         \tPrint version and exit.\n",
    {driverCategory}, kOptDriver | kOptCommon);

maplecl::Option<bool> wUnusedMacro({"-Wunused-macros"},
    "  -Wunused-macros             \twarning: macro is not used\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> wBadFunctionCast({"-Wbad-function-cast"},
    "  -Wbad-function-cast         \twarning: cast from function call of type A to non-matching type B\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-bad-function-cast"));

maplecl::Option<bool> wStrictPrototypes({"-Wstrict-prototypes"},
    "  -Wstrict-prototypes         \twarning: Warn if a function is declared or defined without specifying the "
    "argument types\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-strict-prototypes"));

maplecl::Option<bool> wUndef({"-Wundef"},
    "  -Wundef                     \twarning: Warn if an undefined identifier is evaluated in an #if directive. "
    "Such identifiers are replaced with zero\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-undef"));

maplecl::Option<bool> wCastQual({"-Wcast-qual"},
    "  -Wcast-qual                 \twarning: Warn whenever a pointer is cast so as to remove a type qualifier "
    "from the target type. For example, warn if a const char * is cast to an ordinary char *\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-cast-qual"));

maplecl::Option<bool> wMissingFieldInitializers({"-Wmissing-field-initializers"},
    "  -Wmissing-field-initializers\twarning: Warn if a structure's initializer has some fields missing\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-field-initializers"));

maplecl::Option<bool> wUnusedParameter({"-Wunused-parameter"},
    "  -Wunused-parameter          \twarning: Warn whenever a function parameter is unused aside from its "
    "declaration\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-parameter"));

maplecl::Option<bool> wAll({"-Wall"},
    "  -Wall                       \tThis enables all the warnings about constructions that some users consider "
    "questionable\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-all"));

maplecl::Option<bool> wExtra({"-Wextra"},
    "  -Wextra                     \tEnable some extra warning flags that are not enabled by -Wall\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-extra"));

maplecl::Option<bool> wWriteStrings({"-Wwrite-strings"},
    "  -Wwrite-strings             \tWhen compiling C, give string constants the type const char[length] so that "
    "copying the address of one into a non-const char * pointer produces a warning\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-write-strings"));

maplecl::Option<bool> wVla({"-Wvla"},
    "  -Wvla                       \tWarn if a variable-length array is used in the code\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-vla"));

maplecl::Option<bool> wFormatSecurity({"-Wformat-security"},
    "  -Wformat-security           \tWwarn about uses of format functions that represent possible security problems.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-security"));

maplecl::Option<bool> wShadow({"-Wshadow"},
    "  -Wshadow                    \tWarn whenever a local variable or type declaration shadows another variable.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shadow"));

maplecl::Option<bool> wTypeLimits({"-Wtype-limits"},
    "  -Wtype-limits               \tWarn if a comparison is always true or always false due to the limited range "
    "of the data type.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-type-limits"));

maplecl::Option<bool> wSignCompare({"-Wsign-compare"},
    "  -Wsign-compare              \tWarn when a comparison between signed and  unsigned values could produce an "
    "incorrect result when the signed value is converted to unsigned.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sign-compare"));

maplecl::Option<bool> wShiftNegativeValue({"-Wshift-negative-value"},
    "  -Wshift-negative-value      \tWarn if left shifting a negative value.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shift-negative-value"));

maplecl::Option<bool> wPointerArith({"-Wpointer-arith"},
    "  -Wpointer-arith             \tWarn about anything that depends on the “size of” a function type or of void.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pointer-arith"));

maplecl::Option<bool> wIgnoredQualifiers({"-Wignored-qualifiers"},
    "  -Wignored-qualifiers        \tWarn if the return type of a function has a type qualifier such as const.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-ignored-qualifiers"));

maplecl::Option<bool> wFormat({"-Wformat"},
    "  -Wformat                    \tCheck calls to printf and scanf, etc., to make sure that the arguments "
    "supplied have types appropriate to the format string specified.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format"));

maplecl::Option<bool> wFloatEqual({"-Wfloat-equal"},
    "  -Wfloat-equal               \tWarn if floating-point values are used in equality comparisons.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-float-equal"));

maplecl::Option<bool> wDateTime({"-Wdate-time"},
    "  -Wdate-time                 \tWarn when macros __TIME__, __DATE__ or __TIMESTAMP__ are encountered as "
    "they might prevent bit-wise-identical reproducible compilations\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-date-time"));

maplecl::Option<bool> wImplicitFallthrough({"-Wimplicit-fallthrough"},
    "  -Wimplicit-fallthrough      \tWarn when a switch case falls through\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-implicit-fallthrough"));

maplecl::Option<bool> wShiftOverflow({"-Wshift-overflow"},
    "  -Wshift-overflow            \tWarn about left shift overflows\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shift-overflow"));

maplecl::Option<bool> oWnounusedcommandlineargument({"-Wno-unused-command-line-argument"},
    "  -Wno-unused-command-line-argument\n"
    "                              \tno unused command line argument\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnoconstantconversion({"-Wno-constant-conversion"},
    "  -Wno-constant-conversion    \tno constant conversion\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnounknownwarningoption({"-Wno-unknown-warning-option"},
    "  -Wno-unknown-warning-option \tno unknown warning option\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oW({"-W"},
    "  -W                          \tThis switch is deprecated; use -Wextra instead.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWabi({"-Wabi"},
    "  -Wabi                       \tWarn about things that will change when compiling with an ABI-compliant "
    "compiler.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-abi"));

maplecl::Option<bool> oWabiTag({"-Wabi-tag"},
    "  -Wabi-tag                   \tWarn if a subobject has an abi_tag attribute that the complete object type "
    "does not have.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWaddrSpaceConvert({"-Waddr-space-convert"},
    "  -Waddr-space-convert        \tWarn about conversions between address spaces in the case where the resulting "
    "address space is not contained in the incoming address space.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWaddress({"-Waddress"},
    "  -Waddress                   \tWarn about suspicious uses of memory addresses.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-address"));

maplecl::Option<bool> oWaggregateReturn({"-Waggregate-return"},
    "  -Waggregate-return          \tWarn about returning structures\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-aggregate-return"));

maplecl::Option<bool> oWaggressiveLoopOptimizations({"-Waggressive-loop-optimizations"},
    "  -Waggressive-loop-optimizations\n"
    "                              \tWarn if a loop with constant number of iterations triggers undefined "
    "behavior.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-aggressive-loop-optimizations"));

maplecl::Option<bool> oWalignedNew({"-Waligned-new"},
    "  -Waligned-new               \tWarn about 'new' of type with extended alignment without -faligned-new.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-aligned-new"));

maplecl::Option<bool> oWallocZero({"-Walloc-zero"},
    "  -Walloc-zero                \t-Walloc-zero Warn for calls to allocation functions that specify zero bytes.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-alloc-zero"));

maplecl::Option<bool> oWalloca({"-Walloca"},
    "  -Walloca                    \tWarn on any use of alloca.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-alloca"));

maplecl::Option<bool> oWarrayBounds({"-Warray-bounds"},
    "  -Warray-bounds              \tWarn if an array is accessed out of bounds.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-array-bounds"));

maplecl::Option<bool> oWassignIntercept({"-Wassign-intercept"},
    "  -Wassign-intercept          \tWarn whenever an Objective-C assignment is being intercepted by the garbage "
    "collector.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-assign-intercept"));

maplecl::Option<bool> oWattributes({"-Wattributes"},
    "  -Wattributes                \tWarn about inappropriate attribute usage.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-attributes"));

maplecl::Option<bool> oWboolCompare({"-Wbool-compare"},
    "  -Wbool-compare              \tWarn about boolean expression compared with an integer value different from "
    "true/false.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-bool-compare"));

maplecl::Option<bool> oWboolOperation({"-Wbool-operation"},
    "  -Wbool-operation            \tWarn about certain operations on boolean expressions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-bool-operation"));

maplecl::Option<bool> oWbuiltinDeclarationMismatch({"-Wbuiltin-declaration-mismatch"},
    "  -Wbuiltin-declaration-mismatch\n"
    "                              \tWarn when a built-in function is declared with the wrong signature.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-builtin-declaration-mismatch"));

maplecl::Option<bool> oWbuiltinMacroRedefined({"-Wbuiltin-macro-redefined"},
    "  -Wbuiltin-macro-redefined   \tWarn when a built-in preprocessor macro is undefined or redefined.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-builtin-macro-redefined"));

maplecl::Option<bool> oW11Compat({"-Wc++11-compat"},
    "  -Wc++11-compat              \tWarn about C++ constructs whose meaning differs between ISO C++ 1998 and "
    "ISO C++ 2011.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oW14Compat({"-Wc++14-compat"},
    "  -Wc++14-compat              \tWarn about C++ constructs whose meaning differs between ISO C++ 2011 and ISO "
    "C++ 2014.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oW1zCompat({"-Wc++1z-compat"},
    "  -Wc++1z-compat              \tWarn about C++ constructs whose meaning differs between ISO C++ 2014 and "
    "(forthcoming) ISO C++ 201z(7?).\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWc90C99Compat({"-Wc90-c99-compat"},
    "  -Wc90-c99-compat            \tWarn about features not present in ISO C90\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-c90-c99-compat"));

maplecl::Option<bool> oWc99C11Compat({"-Wc99-c11-compat"},
    "  -Wc99-c11-compat            \tWarn about features not present in ISO C99\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-c99-c11-compat"));

maplecl::Option<bool> oWcastAlign({"-Wcast-align"},
    "  -Wcast-align                \tWarn about pointer casts which increase alignment.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-cast-align"));

maplecl::Option<bool> oWcharSubscripts({"-Wchar-subscripts"},
    "  -Wchar-subscripts           \tWarn about subscripts whose type is \"char\".\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-char-subscripts"));

maplecl::Option<bool> oWchkp({"-Wchkp"},
    "  -Wchkp                      \tWarn about memory access errors found by Pointer Bounds Checker.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWclobbered({"-Wclobbered"},
    "  -Wclobbered                 \tWarn about variables that might be changed by \"longjmp\" or \"vfork\".\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-clobbered"));

maplecl::Option<bool> oWcomment({"-Wcomment"},
    "  -Wcomment                   \tWarn about possibly nested block comments\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWcomments({"-Wcomments"},
    "  -Wcomments                  \tSynonym for -Wcomment.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWconditionallySupported({"-Wconditionally-supported"},
    "  -Wconditionally-supported   \tWarn for conditionally-supported constructs.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-conditionally-supported"));

maplecl::Option<bool> oWconversion({"-Wconversion"},
    "  -Wconversion                \tWarn for implicit type conversions that may change a value.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-conversion"));

maplecl::Option<bool> oWconversionNull({"-Wconversion-null"},
    "  -Wconversion-null           \tWarn for converting NULL from/to a non-pointer type.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-conversion-null"));

maplecl::Option<bool> oWctorDtorPrivacy({"-Wctor-dtor-privacy"},
    "  -Wctor-dtor-privacy         \tWarn when all constructors and destructors are private.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-ctor-dtor-privacy"));

maplecl::Option<bool> oWdanglingElse({"-Wdangling-else"},
    "  -Wdangling-else             \tWarn about dangling else.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-dangling-else"));

maplecl::Option<bool> oWdeclarationAfterStatement({"-Wdeclaration-after-statement"},
    "  -Wdeclaration-after-statement\n"
    "                              \tWarn when a declaration is found after a statement.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-declaration-after-statement"));

maplecl::Option<bool> oWdeleteIncomplete({"-Wdelete-incomplete"},
    "  -Wdelete-incomplete         \tWarn when deleting a pointer to incomplete type.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-delete-incomplete"));

maplecl::Option<bool> oWdeleteNonVirtualDtor({"-Wdelete-non-virtual-dtor"},
    "  -Wdelete-non-virtual-dtor   \tWarn about deleting polymorphic objects with non-virtual destructors.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-delete-non-virtual-dtor"));

maplecl::Option<bool> oWdeprecated({"-Wdeprecated"},
    "  -Wdeprecated                \tWarn if a deprecated compiler feature\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-deprecated"));

maplecl::Option<bool> oWdeprecatedDeclarations({"-Wdeprecated-declarations"},
    "  -Wdeprecated-declarations   \tWarn about uses of __attribute__((deprecated)) declarations.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-deprecated-declarations"));

maplecl::Option<bool> oWdisabledOptimization({"-Wdisabled-optimization"},
    "  -Wdisabled-optimization     \tWarn when an optimization pass is disabled.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-disabled-optimization"));

maplecl::Option<bool> oWdiscardedArrayQualifiers({"-Wdiscarded-array-qualifiers"},
    "  -Wdiscarded-array-qualifiers\tWarn if qualifiers on arrays which are pointer targets are discarded.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-discarded-array-qualifiers"));

maplecl::Option<bool> oWdiscardedQualifiers({"-Wdiscarded-qualifiers"},
    "  -Wdiscarded-qualifiers      \tWarn if type qualifiers on pointers are discarded.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-discarded-qualifiers"));

maplecl::Option<bool> oWdivByZero({"-Wdiv-by-zero"},
    "  -Wdiv-by-zero               \tWarn about compile-time integer division by zero.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-div-by-zero"));

maplecl::Option<bool> oWdoublePromotion({"-Wdouble-promotion"},
    "  -Wdouble-promotion          \tWarn about implicit conversions from \"float\" to \"double\".\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-double-promotion"));

maplecl::Option<bool> oWduplicateDeclSpecifier({"-Wduplicate-decl-specifier"},
    "  -Wduplicate-decl-specifier  \tWarn when a declaration has duplicate const\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-duplicate-decl-specifier"));

maplecl::Option<bool> oWduplicatedBranches({"-Wduplicated-branches"},
    "  -Wduplicated-branches       \tWarn about duplicated branches in if-else statements.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-duplicated-branches"));

maplecl::Option<bool> oWduplicatedCond({"-Wduplicated-cond"},
    "  -Wduplicated-cond           \tWarn about duplicated conditions in an if-else-if chain.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-duplicated-cond"));

maplecl::Option<bool> oWeffc({"-Weffc++"},
    "  -Weffc++                    \tWarn about violations of Effective C++ style rules.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-effc++"));

maplecl::Option<bool> oWemptyBody({"-Wempty-body"},
    "  -Wempty-body                \tWarn about an empty body in an if or else statement.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-empty-body"));

maplecl::Option<bool> oWendifLabels({"-Wendif-labels"},
    "  -Wendif-labels              \tWarn about stray tokens after #else and #endif.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-endif-labels"));

maplecl::Option<bool> oWenumCompare({"-Wenum-compare"},
    "  -Wenum-compare              \tWarn about comparison of different enum types.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-enum-compare"));

maplecl::Option<bool> oWerror({"-Werror"},
    "  -Werror                     \tTreat all warnings as errors.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-error"));

maplecl::Option<bool> oWexpansionToDefined({"-Wexpansion-to-defined"},
    "  -Wexpansion-to-defined      \tWarn if 'defined' is used outside #if.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWfatalErrors({"-Wfatal-errors"},
    "  -Wfatal-errors              \tExit on the first error occurred.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-fatal-errors"));

maplecl::Option<bool> oWfloatConversion({"-Wfloat-conversion"},
    "  -Wfloat-conversion          \tWarn for implicit type conversions that cause loss of floating point precision.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-float-conversion"));

maplecl::Option<bool> oWformatContainsNul({"-Wformat-contains-nul"},
    "  -Wformat-contains-nul       \tWarn about format strings that contain NUL bytes.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-contains-nul"));

maplecl::Option<bool> oWformatExtraArgs({"-Wformat-extra-args"},
    "  -Wformat-extra-args         \tWarn if passing too many arguments to a function for its format string.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-extra-args"));

maplecl::Option<bool> oWformatNonliteral({"-Wformat-nonliteral"},
    "  -Wformat-nonliteral         \tWarn about format strings that are not literals.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-nonliteral"));

maplecl::Option<bool> oWformatOverflow({"-Wformat-overflow"},
    "  -Wformat-overflow           \tWarn about function calls with format strings that write past the end of the "
    "destination region.  Same as -Wformat-overflow=1.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-overflow"));

maplecl::Option<bool> oWformatSignedness({"-Wformat-signedness"},
    "  -Wformat-signedness         \tWarn about sign differences with format functions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-signedness"));

maplecl::Option<bool> oWformatTruncation({"-Wformat-truncation"},
    "  -Wformat-truncation         \tWarn about calls to snprintf and similar functions that truncate output. "
    "Same as -Wformat-truncation=1.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-truncation"));

maplecl::Option<bool> oWformatY2k({"-Wformat-y2k"},
    "  -Wformat-y2k                \tWarn about strftime formats yielding 2-digit years.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-y2k"));

maplecl::Option<bool> oWformatZeroLength({"-Wformat-zero-length"},
    "  -Wformat-zero-length        \tWarn about zero-length formats.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-format-zero-length"));

maplecl::Option<bool> oWframeAddress({"-Wframe-address"},
    "  -Wframe-address             \tWarn when __builtin_frame_address or __builtin_return_address is used unsafely.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-frame-address"));

maplecl::Option<bool> oWfreeNonheapObject({"-Wfree-nonheap-object"},
    "  -Wfree-nonheap-object       \tWarn when attempting to free a non-heap object.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-free-nonheap-object"));

maplecl::Option<bool> oWignoredAttributes({"-Wignored-attributes"},
    "  -Wignored-attributes        \tWarn whenever attributes are ignored.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-ignored-attributes"));

maplecl::Option<bool> oWimplicit({"-Wimplicit"},
    "  -Wimplicit                  \tWarn about implicit declarations.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-implicit"));

maplecl::Option<bool> oWimplicitFunctionDeclaration({"-Wimplicit-function-declaration"},
    "  -Wimplicit-function-declaration\n"
    "                              \tWarn about implicit function declarations.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-implicit-function-declaration"));

maplecl::Option<bool> oWimplicitInt({"-Wimplicit-int"},
    "  -Wimplicit-int              \tWarn when a declaration does not specify a type.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-implicit-int"));

maplecl::Option<bool> oWincompatiblePointerTypes({"-Wincompatible-pointer-types"},
    "  -Wincompatible-pointer-types\tWarn when there is a conversion between pointers that have incompatible types.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-incompatible-pointer-types"));

maplecl::Option<bool> oWinheritedVariadicCtor({"-Winherited-variadic-ctor"},
    "  -Winherited-variadic-ctor   \tWarn about C++11 inheriting constructors when the base has a variadic "
    "constructor.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-inherited-variadic-ctor"));

maplecl::Option<bool> oWinitSelf({"-Winit-self"},
    "  -Winit-self                 \tWarn about variables which are initialized to themselves.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-init-self"));

maplecl::Option<bool> oWinline({"-Winline"},
    "  -Winline                    \tWarn when an inlined function cannot be inlined.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-inline"));

maplecl::Option<bool> oWintConversion({"-Wint-conversion"},
    "  -Wint-conversion            \tWarn about incompatible integer to pointer and pointer to integer conversions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-int-conversion"));

maplecl::Option<bool> oWintInBoolContext({"-Wint-in-bool-context"},
    "  -Wint-in-bool-context       \tWarn for suspicious integer expressions in boolean context.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-int-in-bool-context"));

maplecl::Option<bool> oWintToPointerCast({"-Wint-to-pointer-cast"},
    "  -Wint-to-pointer-cast       \tWarn when there is a cast to a pointer from an integer of a different size.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-int-to-pointer-cast"));

maplecl::Option<bool> oWinvalidMemoryModel({"-Winvalid-memory-model"},
    "  -Winvalid-memory-model      \tWarn when an atomic memory model parameter is known to be outside the valid "
    "range.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-invalid-memory-model"));

maplecl::Option<bool> oWinvalidOffsetof({"-Winvalid-offsetof"},
    "  -Winvalid-offsetof          \tWarn about invalid uses of the \"offsetof\" macro.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-invalid-offsetof"));

maplecl::Option<bool> oWLiteralSuffix({"-Wliteral-suffix"},
    "  -Wliteral-suffix            \tWarn when a string or character literal is followed by a ud-suffix which does "
    "not begin with an underscore.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-literal-suffix"));

maplecl::Option<bool> oWLogicalNotParentheses({"-Wlogical-not-parentheses"},
    "  -Wlogical-not-parentheses   \tWarn about logical not used on the left hand side operand of a comparison.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-logical-not-parentheses"));

maplecl::Option<bool> oWinvalidPch({"-Winvalid-pch"},
    "  -Winvalid-pch               \tWarn about PCH files that are found but not used.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-invalid-pch"));

maplecl::Option<bool> oWjumpMissesInit({"-Wjump-misses-init"},
    "  -Wjump-misses-init          \tWarn when a jump misses a variable initialization.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-jump-misses-init"));

maplecl::Option<bool> oWLogicalOp({"-Wlogical-op"},
    "  -Wlogical-op                \tWarn about suspicious uses of logical operators in expressions. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-logical-op"));

maplecl::Option<bool> oWLongLong({"-Wlong-long"},
    "  -Wlong-long                 \tWarn if long long type is used.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-long-long"));

maplecl::Option<bool> oWmain({"-Wmain"},
    "  -Wmain                      \tWarn about suspicious declarations of \"main\".\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-main"));

maplecl::Option<bool> oWmaybeUninitialized({"-Wmaybe-uninitialized"},
    "  -Wmaybe-uninitialized       \tWarn about maybe uninitialized automatic variables.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-maybe-uninitialized"));

maplecl::Option<bool> oWmemsetEltSize({"-Wmemset-elt-size"},
    "  -Wmemset-elt-size           \tWarn about suspicious calls to memset where the third argument contains the "
    "number of elements not multiplied by the element size.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-memset-elt-size"));

maplecl::Option<bool> oWmemsetTransposedArgs({"-Wmemset-transposed-args"},
    "  -Wmemset-transposed-args    \tWarn about suspicious calls to memset where the third argument is constant "
    "literal zero and the second is not.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-memset-transposed-args"));

maplecl::Option<bool> oWmisleadingIndentation({"-Wmisleading-indentation"},
    "  -Wmisleading-indentation    \tWarn when the indentation of the code does not reflect the block structure.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-misleading-indentatio"));

maplecl::Option<bool> oWmissingBraces({"-Wmissing-braces"},
    "  -Wmissing-braces            \tWarn about possibly missing braces around initializers.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-braces"));

maplecl::Option<bool> oWmissingDeclarations({"-Wmissing-declarations"},
    "  -Wmissing-declarations      \tWarn about global functions without previous declarations.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-declarations"));

maplecl::Option<bool> oWmissingFormatAttribute({"-Wmissing-format-attribute"},
    "  -Wmissing-format-attribute  \t\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-format-attribute"));

maplecl::Option<bool> oWmissingIncludeDirs({"-Wmissing-include-dirs"},
    "  -Wmissing-include-dirs      \tWarn about user-specified include directories that do not exist.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-include-dirs"));

maplecl::Option<bool> oWmissingParameterType({"-Wmissing-parameter-type"},
    "  -Wmissing-parameter-type    \tWarn about function parameters declared without a type specifier in K&R-style "
    "functions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-parameter-type"));

maplecl::Option<bool> oWmissingPrototypes({"-Wmissing-prototypes"},
    "  -Wmissing-prototypes        \tWarn about global functions without prototypes.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-missing-prototypes"));

maplecl::Option<bool> oWmultichar({"-Wmultichar"},
    "  -Wmultichar                 \tWarn about use of multi-character character constants.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-multichar"));

maplecl::Option<bool> oWmultipleInheritance({"-Wmultiple-inheritance"},
    "  -Wmultiple-inheritance      \tWarn on direct multiple inheritance.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnamespaces({"-Wnamespaces"},
    "  -Wnamespaces                \tWarn on namespace definition.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnarrowing({"-Wnarrowing"},
    "  -Wnarrowing                 \tWarn about narrowing conversions within { } that are ill-formed in C++11.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-narrowing"));

maplecl::Option<bool> oWnestedExterns({"-Wnested-externs"},
    "  -Wnested-externs            \tWarn about \"extern\" declarations not at file scope.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-nested-externs"));

maplecl::Option<bool> oWnoexcept({"-Wnoexcept"},
    "  -Wnoexcept                  \tWarn when a noexcept expression evaluates to false even though the expression "
    "can't actually throw.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-noexcept"));

maplecl::Option<bool> oWnoexceptType({"-Wnoexcept-type"},
    "  -Wnoexcept-type             \tWarn if C++1z noexcept function type will change the mangled name of a symbol.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-noexcept-type"));

maplecl::Option<bool> oWnonTemplateFriend({"-Wnon-template-friend"},
    "  -Wnon-template-friend       \tWarn when non-templatized friend functions are declared within a template.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-non-template-friend"));

maplecl::Option<bool> oWnonVirtualDtor({"-Wnon-virtual-dtor"},
    "  -Wnon-virtual-dtor          \tWarn about non-virtual destructors.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-non-virtual-dtor"));

maplecl::Option<bool> oWnonnull({"-Wnonnull"},
    "  -Wnonnull                   \tWarn about NULL being passed to argument slots marked as requiring non-NULL.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-nonnull"));

maplecl::Option<bool> oWnonnullCompare({"-Wnonnull-compare"},
    "  -Wnonnull-compare           \tWarn if comparing pointer parameter with nonnull attribute with NULL.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-nonnull-compare"));

maplecl::Option<bool> oWnormalized({"-Wnormalized"},
    "  -Wnormalized                \tWarn about non-normalized Unicode strings.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-normalized"));

maplecl::Option<bool> oWnullDereference({"-Wnull-dereference"},
    "  -Wnull-dereference          \tWarn if dereferencing a NULL pointer may lead to erroneous or undefined "
    "behavior.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-null-dereference"));

maplecl::Option<bool> oWodr({"-Wodr"},
    "  -Wodr                       \tWarn about some C++ One Definition Rule violations during link time "
    "optimization.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-odr"));

maplecl::Option<bool> oWoldStyleCast({"-Wold-style-cast"},
    "  -Wold-style-cast            \tWarn if a C-style cast is used in a program.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-old-style-cast"));

maplecl::Option<bool> oWoldStyleDeclaration({"-Wold-style-declaration"},
    "  -Wold-style-declaration     \tWarn for obsolescent usage in a declaration.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-old-style-declaration"));

maplecl::Option<bool> oWoldStyleDefinition({"-Wold-style-definition"},
    "  -Wold-style-definition      \tWarn if an old-style parameter definition is used.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-old-style-definition"));

maplecl::Option<bool> oWopenmSimd({"-Wopenm-simd"},
    "  -Wopenm-simd                \tWarn if the vectorizer cost model overrides the OpenMP or the Cilk Plus "
    "simd directive set by user. \n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWoverflow({"-Woverflow"},
    "  -Woverflow                  \tWarn about overflow in arithmetic expressions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-overflow"));

maplecl::Option<bool> oWoverlengthStrings({"-Woverlength-strings"},
    "  -Woverlength-strings        \tWarn if a string is longer than the maximum portable length specified by "
    "the standard.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-overlength-strings"));

maplecl::Option<bool> oWoverloadedVirtual({"-Woverloaded-virtual"},
    "  -Woverloaded-virtual        \tWarn about overloaded virtual function names.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-overloaded-virtual"));

maplecl::Option<bool> oWoverrideInit({"-Woverride-init"},
    "  -Woverride-init             \tWarn about overriding initializers without side effects.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-override-init"));

maplecl::Option<bool> oWoverrideInitSideEffects({"-Woverride-init-side-effects"},
    "  -Woverride-init-side-effects\tWarn about overriding initializers with side effects.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-override-init-side-effects"));

maplecl::Option<bool> oWpacked({"-Wpacked"},
    "  -Wpacked                    \tWarn when the packed attribute has no effect on struct layout.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-packed"));

maplecl::Option<bool> oWpackedBitfieldCompat({"-Wpacked-bitfield-compat"},
    "  -Wpacked-bitfield-compat    \tWarn about packed bit-fields whose offset changed in GCC 4.4.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-packed-bitfield-compat"));

maplecl::Option<bool> oWpadded({"-Wpadded"},
    "  -Wpadded                    \tWarn when padding is required to align structure members.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-padded"));

maplecl::Option<bool> oWparentheses({"-Wparentheses"},
    "  -Wparentheses               \tWarn about possibly missing parentheses.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-parentheses"));

maplecl::Option<bool> oWpedantic({"-Wpedantic"},
    "  -Wpedantic                  \tIssue warnings needed for strict compliance to the standard.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWpedanticMsFormat({"-Wpedantic-ms-format"},
    "  -Wpedantic-ms-format        \t\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pedantic-ms-format"));

maplecl::Option<bool> oWplacementNew({"-Wplacement-new"},
    "  -Wplacement-new             \tWarn for placement new expressions with undefined behavior.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-placement-new"));

maplecl::Option<bool> oWpmfConversions({"-Wpmf-conversions"},
    "  -Wpmf-conversions           \tWarn when converting the type of pointers to member functions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pmf-conversions"));

maplecl::Option<bool> oWpointerCompare({"-Wpointer-compare"},
    "  -Wpointer-compare           \tWarn when a pointer is compared with a zero character constant.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pointer-compare"));

maplecl::Option<bool> oWpointerSign({"-Wpointer-sign"},
    "  -Wpointer-sign              \tWarn when a pointer differs in signedness in an assignment.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pointer-sign"));

maplecl::Option<bool> oWpointerToIntCast({"-Wpointer-to-int-cast"},
    "  -Wpointer-to-int-cast       \tWarn when a pointer is cast to an integer of a different size.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pointer-to-int-cast"));

maplecl::Option<bool> oWpragmas({"-Wpragmas"},
    "  -Wpragmas                   \tWarn about misuses of pragmas.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-pragmas"));

maplecl::Option<bool> oWprotocol({"-Wprotocol"},
    "  -Wprotocol                  \tWarn if inherited methods are unimplemented.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-protocol"));

maplecl::Option<bool> oWredundantDecls({"-Wredundant-decls"},
    "  -Wredundant-decls           \tWarn about multiple declarations of the same object.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-redundant-decls"));

maplecl::Option<bool> oWregister({"-Wregister"},
    "  -Wregister                  \tWarn about uses of register storage specifier.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-register"));

maplecl::Option<bool> oWreorder({"-Wreorder"},
    "  -Wreorder                   \tWarn when the compiler reorders code.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-reorder"));

maplecl::Option<bool> oWrestrict({"-Wrestrict"},
    "  -Wrestrict                  \tWarn when an argument passed to a restrict-qualified parameter aliases with "
    "another argument.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-restrict"));

maplecl::Option<bool> oWreturnLocalAddr({"-Wreturn-local-addr"},
    "  -Wreturn-local-addr         \tWarn about returning a pointer/reference to a local or temporary variable.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-return-local-addr"));

maplecl::Option<bool> oWreturnType({"-Wreturn-type"},
    "  -Wreturn-type               \tWarn whenever a function's return type defaults to \"int\" (C)\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-return-type"));

maplecl::Option<bool> oWselector({"-Wselector"},
    "  -Wselector                  \tWarn if a selector has multiple methods.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-selector"));

maplecl::Option<bool> oWsequencePoint({"-Wsequence-point"},
    "  -Wsequence-point            \tWarn about possible violations of sequence point rules.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sequence-point"));

maplecl::Option<bool> oWshadowIvar({"-Wshadow-ivar"},
    "  -Wshadow-ivar               \tWarn if a local declaration hides an instance variable.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shadow-ivar"));

maplecl::Option<bool> oWshiftCountNegative({"-Wshift-count-negative"},
    "  -Wshift-count-negative      \tWarn if shift count is negative.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shift-count-negative"));

maplecl::Option<bool> oWshiftCountOverflow({"-Wshift-count-overflow"},
    "  -Wshift-count-overflow      \tWarn if shift count >= width of type.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-shift-count-overflow"));

maplecl::Option<bool> oWsignConversion({"-Wsign-conversion"},
    "  -Wsign-conversion           \tWarn for implicit type conversions between signed and unsigned integers.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sign-conversion"));

maplecl::Option<bool> oWsignPromo({"-Wsign-promo"},
    "  -Wsign-promo                \tWarn when overload promotes from unsigned to signed.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sign-promo"));

maplecl::Option<bool> oWsizedDeallocation({"-Wsized-deallocation"},
    "  -Wsized-deallocation        \tWarn about missing sized deallocation functions.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sized-deallocation"));

maplecl::Option<bool> oWsizeofArrayArgument({"-Wsizeof-array-argument"},
    "  -Wsizeof-array-argument     \tWarn when sizeof is applied on a parameter declared as an array.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sizeof-array-argument"));

maplecl::Option<bool> oWsizeofPointerMemaccess({"-Wsizeof-pointer-memaccess"},
    "  -Wsizeof-pointer-memaccess  \tWarn about suspicious length parameters to certain string functions if the "
    "argument uses sizeof.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sizeof-pointer-memaccess"));

maplecl::Option<bool> oWstackProtector({"-Wstack-protector"},
    "  -Wstack-protector           \tWarn when not issuing stack smashing protection for some reason.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-stack-protector"));

maplecl::Option<bool> oWstrictAliasing({"-Wstrict-aliasing"},
    "  -Wstrict-aliasing           \tWarn about code which might break strict aliasing rules.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-strict-aliasing"));

maplecl::Option<bool> oWstrictNullSentinel({"-Wstrict-null-sentinel"},
    "  -Wstrict-null-sentinel      \tWarn about uncasted NULL used as sentinel.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-strict-null-sentinel"));

maplecl::Option<bool> oWstrictOverflow({"-Wstrict-overflow"},
    "  -Wstrict-overflow           \tWarn about optimizations that assume that signed overflow is undefined.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-strict-overflow"));

maplecl::Option<bool> oWstrictSelectorMatch({"-Wstrict-selector-match"},
    "  -Wstrict-selector-match     \tWarn if type signatures of candidate methods do not match exactly.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-strict-selector-match"));

maplecl::Option<bool> oWstringopOverflow({"-Wstringop-overflow"},
    "  -Wstringop-overflow         \tWarn about buffer overflow in string manipulation functions like memcpy "
    "and strcpy.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-stringop-overflow"));

maplecl::Option<bool> oWsubobjectLinkage({"-Wsubobject-linkage"},
    "  -Wsubobject-linkage         \tWarn if a class type has a base or a field whose type uses the anonymous "
    "namespace or depends on a type with no linkage.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-subobject-linkage"));

maplecl::Option<bool> oWsuggestAttributeConst({"-Wsuggest-attribute=const"},
    "  -Wsuggest-attribute=const   \tWarn about functions which might be candidates for __attribute__((const)).\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-attribute=const"));

maplecl::Option<bool> oWsuggestAttributeFormat({"-Wsuggest-attribute=format"},
    "  -Wsuggest-attribute=format  \tWarn about functions which might be candidates for format attributes.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-attribute=format"));

maplecl::Option<bool> oWsuggestAttributeNoreturn({"-Wsuggest-attribute=noreturn"},
    "  -Wsuggest-attribute=noreturn\tWarn about functions which might be candidates for __attribute__((noreturn)).\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-attribute=noreturn"));

maplecl::Option<bool> oWsuggestAttributePure({"-Wsuggest-attribute=pure"},
    "  -Wsuggest-attribute=pure    \tWarn about functions which might be candidates for __attribute__((pure)).\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-attribute=pure"));

maplecl::Option<bool> oWsuggestFinalMethods({"-Wsuggest-final-methods"},
    "  -Wsuggest-final-methods     \tWarn about C++ virtual methods where adding final keyword would improve "
    "code quality.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-final-methods"));

maplecl::Option<bool> oWsuggestFinalTypes({"-Wsuggest-final-types"},
    "  -Wsuggest-final-types       \tWarn about C++ polymorphic types where adding final keyword would improve "
    "code quality.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-suggest-final-types"));

maplecl::Option<bool> oWswitch({"-Wswitch"},
    "  -Wswitch                    \tWarn about enumerated switches.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-switch"));

maplecl::Option<bool> oWswitchBool({"-Wswitch-bool"},
    "  -Wswitch-bool               \tWarn about switches with boolean controlling expression.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-switch-bool"));

maplecl::Option<bool> oWswitchDefault({"-Wswitch-default"},
    "  -Wswitch-default            \tWarn about enumerated switches missing a \"default:\" statement.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-switch-default"));

maplecl::Option<bool> oWswitchEnum({"-Wswitch-enum"},
    "  -Wswitch-enum               \tWarn about all enumerated switches missing a specific case.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-switch-enum"));

maplecl::Option<bool> oWswitchUnreachable({"-Wswitch-unreachable"},
    "  -Wswitch-unreachable        \tWarn about statements between switch's controlling expression and the "
    "first case.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-switch-unreachable"));

maplecl::Option<bool> oWsyncNand({"-Wsync-nand"},
    "  -Wsync-nand                 \tWarn when __sync_fetch_and_nand and __sync_nand_and_fetch built-in "
    "functions are used.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-sync-nand"));

maplecl::Option<bool> oWsystemHeaders({"-Wsystem-headers"},
    "  -Wsystem-headers            \tDo not suppress warnings from system headers.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-system-headers"));

maplecl::Option<bool> oWtautologicalCompare({"-Wtautological-compare"},
    "  -Wtautological-compare      \tWarn if a comparison always evaluates to true or false.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-tautological-compare"));

maplecl::Option<bool> oWtemplates({"-Wtemplates"},
    "  -Wtemplates                 \tWarn on primary template declaration.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWterminate({"-Wterminate"},
    "  -Wterminate                 \tWarn if a throw expression will always result in a call to terminate().\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-terminate"));

maplecl::Option<bool> oWtraditional({"-Wtraditional"},
    "  -Wtraditional               \tWarn about features not present in traditional C. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-traditional"));

maplecl::Option<bool> oWtraditionalConversion({"-Wtraditional-conversion"},
    "  -Wtraditional-conversion    \tWarn of prototypes causing type conversions different from what would "
    "happen in the absence of prototype. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-traditional-conversion"));

maplecl::Option<bool> oWtrampolines({"-Wtrampolines"},
    "  -Wtrampolines               \tWarn whenever a trampoline is generated. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-trampolines"));

maplecl::Option<bool> oWtrigraphs({"-Wtrigraphs"},
    "  -Wtrigraphs                 \tWarn if trigraphs are encountered that might affect the meaning of the program.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWundeclaredSelector({"-Wundeclared-selector"},
    "  -Wundeclared-selector       \tWarn about @selector()s without previously declared methods. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-undeclared-selector"));

maplecl::Option<bool> oWuninitialized({"-Wuninitialized"},
    "  -Wuninitialized             \tWarn about uninitialized automatic variables. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-uninitialized"));

maplecl::Option<bool> oWunknownPragmas({"-Wunknown-pragmas"},
    "  -Wunknown-pragmas           \tWarn about unrecognized pragmas. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unknown-pragmas"));

maplecl::Option<bool> oWunsafeLoopOptimizations({"-Wunsafe-loop-optimizations"},
    "  -Wunsafe-loop-optimizations \tWarn if the loop cannot be optimized due to nontrivial assumptions. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unsafe-loop-optimizations"));

maplecl::Option<bool> oWunsuffixedFloatConstants({"-Wunsuffixed-float-constants"},
    "  -Wunsuffixed-float-constants\tWarn about unsuffixed float constants. \n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWunused({"-Wunused"},
    "  -Wunused                    \tEnable all -Wunused- warnings. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused"));

maplecl::Option<bool> oWunusedButSetParameter({"-Wunused-but-set-parameter"},
    "  -Wunused-but-set-parameter  \tWarn when a function parameter is only set, otherwise unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-but-set-parameter"));

maplecl::Option<bool> oWunusedButSetVariable({"-Wunused-but-set-variable"},
    "  -Wunused-but-set-variable   \tWarn when a variable is only set, otherwise unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-but-set-variable"));

maplecl::Option<bool> oWunusedConstVariable({"-Wunused-const-variable"},
    "  -Wunused-const-variable     \tWarn when a const variable is unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-const-variable"));

maplecl::Option<bool> oWunusedFunction({"-Wunused-function"},
    "  -Wunused-function           \tWarn when a function is unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-function"));

maplecl::Option<bool> oWunusedLabel({"-Wunused-label"},
    "  -Wunused-label              \tWarn when a label is unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-label"));

maplecl::Option<bool> oWunusedLocalTypedefs({"-Wunused-local-typedefs"},
    "  -Wunused-local-typedefs     \tWarn when typedefs locally defined in a function are not used. \n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWunusedResult({"-Wunused-result"},
    "  -Wunused-result             \tWarn if a caller of a function, marked with attribute warn_unused_result,"
    " does not use its return value. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-result"));

maplecl::Option<bool> oWunusedValue({"-Wunused-value"},
    "  -Wunused-value              \tWarn when an expression value is unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-value"));

maplecl::Option<bool> oWunusedVariable({"-Wunused-variable"},
    "  -Wunused-variable           \tWarn when a variable is unused. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-unused-variable"));

maplecl::Option<bool> oWuselessCast({"-Wuseless-cast"},
    "  -Wuseless-cast              \tWarn about useless casts. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-useless-cast"));

maplecl::Option<bool> oWvarargs({"-Wvarargs"},
    "  -Wvarargs                   \tWarn about questionable usage of the macros used to retrieve variable "
    "arguments.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-varargs"));

maplecl::Option<bool> oWvariadicMacros({"-Wvariadic-macros"},
    "  -Wvariadic-macros           \tWarn about using variadic macros. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-variadic-macros"));

maplecl::Option<bool> oWvectorOperationPerformance({"-Wvector-operation-performance"},
    "  -Wvector-operation-performance\n"
    "                              \tWarn when a vector operation is compiled outside the SIMD. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-vector-operation-performance"));

maplecl::Option<bool> oWvirtualInheritance({"-Wvirtual-inheritance"},
    "  -Wvirtual-inheritance       \tWarn on direct virtual inheritance.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWvirtualMoveAssign({"-Wvirtual-move-assign"},
    "  -Wvirtual-move-assign       \tWarn if a virtual base has a non-trivial move assignment operator. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-virtual-move-assign"));

maplecl::Option<bool> oWvolatileRegisterVar({"-Wvolatile-register-var"},
    "  -Wvolatile-register-var     \tWarn when a register variable is declared volatile. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-volatile-register-var"));

maplecl::Option<bool> oWzeroAsNullPointerConstant({"-Wzero-as-null-pointer-constant"},
    "  -Wzero-as-null-pointer-constant\n"
    "                              \tWarn when a literal '0' is used as null pointer. \n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-zero-as-null-pointer-constant"));

maplecl::Option<bool> oWnoScalarStorageOrder({"-Wno-scalar-storage-order"},
    "  -Wno-scalar-storage-order   \tDo not warn on suspicious constructs involving reverse scalar storage order.\n",
    {driverCategory, unSupCategory}, maplecl::DisableWith("-Wscalar-storage-order"), maplecl::kHide);

maplecl::Option<bool> oWnoReservedIdMacro({"-Wno-reserved-id-macro"},
    "  -Wno-reserved-id-macro      \tDo not warn when macro name is a reserved identifier.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnoGnuZeroVariadicMacroArguments({"-Wno-gnu-zero-variadic-macro-arguments"},
    "  -Wno-gnu-zero-variadic-macro-arguments\n"
    "                              \tDo not warn when token pasting of ',' and __VA_ARGS__ is a "
    "GNU extension.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> oWnoGnuStatementExpression({"-Wno-gnu-statement-expression"},
    "  -Wno-gnu-statement-expression\n"
    "                              \tDo not warn when use of GNU statement expression extension.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<bool> suppressWarnings({"-w"},
    "  -w                          \tSuppress all warnings.\n",
    {driverCategory, clangCategory, asCategory, ldCategory}, kOptFront);

/* ##################### String Options ############################################################### */

maplecl::Option<std::string> oWeakReferenceMismatches({"-weak_reference_mismatches"},
    "  -weak_reference_mismatches  \tSpecifies what to do if a symbol import conflicts between file "
    "(weak in one and not in another) the default is to treat the symbol as non-weak.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<std::string> oWerrorE({"-Werror="},
    "  -Werror=                    \tTreat specified warning as error.\n",
    {driverCategory, clangCategory}, kOptFront, maplecl::DisableWith("-Wno-error="));

maplecl::Option<std::string> oWnormalizedE({"-Wnormalized="},
    "  -Wnormalized=               \t-Wnormalized=[none|id|nfc|nfkc]   Warn about non-normalized Unicode strings.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<std::string> oWplacementNewE({"-Wplacement-new="},
    "  -Wplacement-new=             \tWarn for placement new expressions with undefined behavior.\n",
    {driverCategory, clangCategory}, kOptFront);

maplecl::Option<std::string> oWstackUsage({"-Wstack-usage="},
    "  -Wstack-usage=              \t-Wstack-usage=<byte-size> Warn if stack usage might exceed <byte-size>.\n",
    {driverCategory, unSupCategory}, maplecl::kHide);

maplecl::Option<std::string> oWstrictAliasingE({"-Wstrict-aliasing="},
    "  -Wstrict-aliasing=          \tWarn about code which might break strict aliasing rules.\n",
    {driverCategory, clangCategory}, kOptFront);

/* ##################### DIGITAL Options ############################################################### */

maplecl::Option<uint32_t> oWframeLargerThan({"-Wframe-larger-than="},
    "  -Wframe-larger-than=        \tWarn if a function's stack frame requires in excess of <byte-size>.\n",
    {driverCategory, clangCategory}, kOptFront);

}