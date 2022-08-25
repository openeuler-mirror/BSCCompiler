/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cl_option.h"
#include <string>

namespace opts {

maplecl::Option<bool> MD({"-MD"},
                    "  -MD                         \tWrite a depfile containing user and system headers\n",
                    {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<std::string> MT({"-MT"},
                           "  -MT<args>                   \tSpecify name of main file output in depfile\n",
                           {driverCategory, clangCategory}, maplecl::hide, maplecl::joinedValue);

maplecl::Option<std::string> MF({"-MF"},
                           "  -MF<file>                   \tWrite depfile output from -MD, -M to <file>\n",
                           {driverCategory, clangCategory}, maplecl::hide, maplecl::joinedValue);

/* Should we use std option in hir2mpl ??? */
maplecl::Option<std::string> std({"-std"},
                            "  -std Ignonored\n",
                            {driverCategory, clangCategory});

/* ##################### Warnings Options ############################################################### */

maplecl::Option<bool> wUnusedMacro({"-Wunused-macros"},
                              "  -Wunused-macros             \twarning: macro is not used\n",
                              {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wBadFunctionCast({"-Wbad-function-cast"},
                                  "  -Wbad-function-cast         \twarning: "
                                  "cast from function call of type A to non-matching type B\n",
                                  {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wStrictPrototypes({"-Wstrict-prototypes"},
                                   "  -Wstrict-prototypes         \twarning: "
                                   "Warn if a function is declared or defined without specifying the argument types\n",
                                   {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wUndef({"-Wundef"},
                        "  -Wundef                     \twarning: "
                        "Warn if an undefined identifier is evaluated in an #if directive. "
                        "Such identifiers are replaced with zero\n",
                        {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wCastQual({"-Wcast-qual"},
                           "  -Wcast-qual                 \twarning: "
                           "Warn whenever a pointer is cast so as to remove a type qualifier from the target type. "
                           "For example, warn if a const char * is cast to an ordinary char *\n",
                           {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wMissingFieldInitializers({"-Wmissing-field-initializers"},
                                           "  -Wmissing-field-initializers\twarning: "
                                           "Warn if a structure’s initializer has some fields missing\n",
                                           {driverCategory, clangCategory}, maplecl::hide,
                                           maplecl::DisableWith("-Wno-missing-field-initializers"));

maplecl::Option<bool> wUnusedParameter({"-Wunused-parameter"},
                                  "  -Wunused-parameter       \twarning: "
                                  "Warn whenever a function parameter is unused aside from its declaration\n",
                                  {driverCategory, clangCategory}, maplecl::hide,
                                  maplecl::DisableWith("-Wno-unused-parameter"));

maplecl::Option<bool> wAll({"-Wall"},
                      "  -Wall                    \tThis enables all the warnings about constructions "
                      "that some users consider questionable\n",
                      {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wExtra({"-Wextra"},
                        "  -Wextra                  \tEnable some extra warning flags that are not enabled by -Wall\n",
                        {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wWriteStrings({"-Wwrite-strings"},
                               "  -Wwrite-strings          \tWhen compiling C, give string constants the type "
                               "const char[length] so that copying the address of one into "
                               "a non-const char * pointer produces a warning\n",
                               {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wVla({"-Wvla"},
                      "  -Wvla                    \tWarn if a variable-length array is used in the code\n",
                      {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wFormatSecurity({"-Wformat-security"},
                                 "  -Wformat-security                    \tWwarn about uses of format "
                                 "functions that represent possible security problems\n",
                                 {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wShadow({"-Wshadow"},
                         "  -Wshadow                             \tWarn whenever a local variable "
                         "or type declaration shadows another variable\n",
                         {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wTypeLimits({"-Wtype-limits"},
                             "  -Wtype-limits                         \tWarn if a comparison is always true or always "
                             "false due to the limited range of the data type\n",
                             {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wSignCompare({"-Wsign-compare"},
                              "  -Wsign-compare                         \tWarn when a comparison between signed and "
                              " unsigned values could produce an incorrect result when the signed value is converted "
                              "to unsigned\n",
                              {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wShiftNegativeValue({"-Wshift-negative-value"},
                                     "  -Wshift-negative-value                 \tWarn if left "
                                     "shifting a negative value\n",
                                     {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wPointerArith({"-Wpointer-arith"},
                                "  -Wpointer-arith                        \tWarn about anything that depends on the "
                                "“size of” a function type or of void\n",
                               {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wIgnoredQualifiers({"-Wignored-qualifiers"},
                                    "  -Wignored-qualifiers                   \tWarn if the return type of a "
                                    "function has a type qualifier such as const\n",
                                    {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wFormat({"-Wformat"},
                         "  -Wformat                     \tCheck calls to printf and scanf, etc., "
                         "to make sure that the arguments supplied have types appropriate "
                         "to the format string specified\n",
                         {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wFloatEqual({"-Wfloat-equal"},
                             "  -Wfloat-equal                \tWarn if floating-point values are used "
                             "in equality comparisons\n",
                             {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wDateTime({"-Wdate-time"},
                           "  -Wdate-time                  \tWarn when macros __TIME__, __DATE__ or __TIMESTAMP__ "
                           "are encountered as they might prevent bit-wise-identical reproducible compilations\n",
                           {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wImplicitFallthrough({"-Wimplicit-fallthrough"},
                                      "  -Wimplicit-fallthrough       \tWarn when a switch case falls through\n",
                                      {driverCategory, clangCategory}, maplecl::hide);

maplecl::Option<bool> wShiftOverflow({"-Wshift-overflow"},
                                "  -Wshift-overflow             \tWarn about left shift overflows\n",
                                {driverCategory, clangCategory}, maplecl::hide,
                                maplecl::DisableWith("-Wno-shift-overflow"));

} /* namespace opts */
