/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef AST2MPL_INCLUDE_ASTMACROS_H
#define AST2MPL_INCLUDE_ASTMACROS_H
#include <iostream>

namespace maple {
const uint32_t kSrcFileNum = 2;

// ast2mpl options
// these values can be use such as -o=1 at command lines
const uint32_t kCheckAssertion = 1;
const uint32_t kNoComment = 2;
const uint32_t kNoLoc = 4;

// these values can be use such as -d=3 at command lines
const int kDebugLevelZero = 0;
const int kDebugLevelOne = 1;
const int kDebugLevelTwo = 2;
const int kDebugLevelThree = 3;

const int kComplexRealID = 1;
const int kComplexImagID = 2;

const int kBitToByteShift = 3;

const uint32_t kFloat128Size = 2;
const uint32_t kInt32Width = 32;
const uint32_t kInt32Mask = 0xFFFFFFFFULL;
const int kDefaultIndent = 1;

#define ASTDEBUG
#ifdef ASTDEBUG
#define LOCATION __func__ << "() at " << __FILE__ << ":" << __LINE__

#define DUMPINFO(stmtClass, s)                      \
  do {                                              \
    if (maple::ast2mplDebug > kDebugLevelOne) {     \
      std::cout << LOCATION << '\n';                \
      s->dump();                                    \
      std::cout << " " << '\n';                     \
    }                                               \
  } while (0)

#define NOTYETHANDLED(s)                                                                                            \
  do {                                                                                                              \
    std::cout << "\n" << LOCATION << " <<<<<<<<<<<<<<<<<< Not Yet Handled: " << s << "<<<<<<<<<<<<<<<<<<" << '\n';  \
    if ((maple::ast2mplOption & kCheckAssertion) != 0) {                                                            \
      ASSERT(false, "Not yet handled");                                                                             \
    }                                                                                                               \
  } while (0)

// print empty line
#define DEBUGPRINT_N(n)                           \
  do {                                            \
    if (maple::ast2mplDebug >= n) {               \
      std::cout << " " << '\n';                   \
    }                                             \
  } while (0)

// print indent
#define DEBUGPRINTIND(n)                          \
  do {                                            \
    if (maple::ast2mplDebug > kDebugLevelZero) {  \
      PrintIndentation(n);                        \
    }                                             \
  } while (0)

// print str
#define DEBUGPRINT_S_LEVEL(str, level)            \
  do {                                            \
    if (maple::ast2mplDebug >= level) {           \
      PrintIndentation(ast2mplDebugIndent);       \
      std::cout << " " << str << '\n';            \
    }                                             \
  } while (0)

#define DEBUGPRINT_FUNC(name)                     \
  do {                                            \
    int ind = maple::ast2mplDebugIndent;          \
    Util::SetIndent(kDefaultIndent);              \
    if (maple::ast2mplDebug > kDebugLevelZero) {  \
      PrintIndentation(ast2mplDebugIndent);       \
      std::cout << name << " {" << '\n';          \
    }                                             \
    Util::SetIndent(ind);                         \
  } while (0)

#define DEBUGPRINT_FUNC_END(name)                 \
  do {                                            \
    int ind = maple::ast2mplDebugIndent;          \
    Util::SetIndent(kDefaultIndent);              \
    if (maple::ast2mplDebug > kDebugLevelZero) {  \
      PrintIndentation(ast2mplDebugIndent);       \
      std::cout << "}\n" << '\n';                 \
    }                                             \
    Util::SetIndent(ind);                         \
  } while (0)

#define DEBUGPRINT_NODE(node, type)                                                       \
  do {                                                                                    \
    if (maple::ast2mplDebug > kDebugLevelOne) {                                           \
      PrintIndentation(maple::ast2mplDebugIndent);                                        \
      std::cout << "  >> node: ";                                                         \
      static_cast<const type*>(node)->Print(static_cast<const MIRModule*>(module), 0);    \
      std::cout << "\n";                                                                  \
    }                                                                                     \
  } while (0)

// print var = val
#define DEBUGPRINT_V_LEVEL(var, level)                                    \
  do {                                                                    \
    if (maple::ast2mplDebug >= level) {                                   \
      PrintIndentation(maple::ast2mplDebugIndent);                        \
      std::cout << LOCATION << " " << #var << " = " << var << '\n';       \
    }                                                                     \
  } while (0)

#define DEBUGPRINT_X_LEVEL(var, level)                                                                    \
  do {                                                                                                    \
    if (maple::ast2mplDebug >= level) {                                                                   \
      PrintIndentation(maple::ast2mplDebugIndent);                                                        \
      std::cout << LOCATION << " " << #var << " = " << std::hex << "0x" << var << std::dec << '\n';       \
    }                                                                                                     \
  } while (0)

// print var = val
#define DEBUGPRINT_V_LEVEL_PURE(var, level)           \
  do {                                                \
    if (maple::ast2mplDebug >= level) {               \
      PrintIndentation(maple::ast2mplDebugIndent);    \
      std::cout << #var << " = " << var << '\n';      \
    }                                                 \
  } while (0)

// print val0 val1
#define DEBUGPRINT_NN_LEVEL(var0, var1, level)        \
  do {                                                \
    if (maple::ast2mplDebug >= level) {               \
      PrintIndentation(maple::ast2mplDebugIndent);    \
      std::cout << var0 << " " << var1;               \
    }                                                 \
  } while (0)

// print var0 = val0, var1 = val1
#define DEBUGPRINT_VV_LEVEL(var0, var1, level)                                                                \
  do {                                                                                                        \
    if (maple::ast2mplDebug >= level) {                                                                       \
      PrintIndentation(maple::ast2mplDebugIndent);                                                            \
      std::cout << LOCATION << " " << #var0 << " = " << var0 << ", " << #var1 << " = " << var1 << '\n';       \
    }                                                                                                         \
  } while (0)

// print val0, var = val
#define DEBUGPRINT_SV_LEVEL(val0, var, level)                                                    \
  do {                                                                                           \
    if (maple::ast2mplDebug >= level) {                                                          \
      PrintIndentation(maple::ast2mplDebugIndent);                                               \
      std::cout << LOCATION << " " << val0 << ", " << #var << " = " << var << '\n';              \
    }                                                                                            \
  } while (0)

#define DEBUGPRINT_SX_LEVEL(val0, var, level)                                                                      \
  do {                                                                                                             \
    if (maple::ast2mplDebug >= level) {                                                                            \
      PrintIndentation(maple::ast2mplDebugIndent);                                                                 \
      std::cout << LOCATION << " " << val0 << ", " << #var << " = " << std::hex << "0x" << var << std::dec         \
                << '\n';                                                                                           \
    }                                                                                                              \
  } while (0)
#else

#define DUMPINFO(stmtClass, s)
#define NOTHANDLED
#define DEBUGPRINT_N(n)
#define DEBUGPRINTIND(n)
#define DEBUGPRINT_S_LEVEL(str, level)
#define DEBUGPRINT_FUNC(name)
#define DEBUGPRINT_FUNC_END(name)
#define DEBUGPRINT_NODE(node, type)
#define DEBUGPRINT_V_LEVEL(var, level)
#define DEBUGPRINT_X_LEVEL(var, level)
#define DEBUGPRINT_V_LEVEL_PURE(var, level)
#define DEBUGPRINT_NN_LEVEL(var0, var1, level)
#define DEBUGPRINT_VV_LEVEL(var0, var1, level)
#define DEBUGPRINT_SV_LEVEL(val0, var, level)
#endif

#define DEBUGPRINT00 DEBUGPRINT_N(0)
#define DEBUGPRINT01 DEBUGPRINT_N(1)
#define DEBUGPRINT02 DEBUGPRINT_N(2)
#define DEBUGPRINT03 DEBUGPRINT_N(3)

#define DEBUGPRINT_S(var) DEBUGPRINT_S_LEVEL(var, 1)
#define DEBUGPRINT_S2(var) DEBUGPRINT_S_LEVEL(var, 2)
#define DEBUGPRINT_S3(var) DEBUGPRINT_S_LEVEL(var, 3)
#define DEBUGPRINT_S4(var) DEBUGPRINT_S_LEVEL(var, 4)
#define DEBUGPRINT_S5(var) DEBUGPRINT_S_LEVEL(var, 5)
#define DEBUGPRINT_S6(var) DEBUGPRINT_S_LEVEL(var, 6)

#define DEBUGPRINT0(var) DEBUGPRINT_V_LEVEL(var, 0)
#define DEBUGPRINT(var) DEBUGPRINT_V_LEVEL(var, 1)
#define DEBUGPRINT2(var) DEBUGPRINT_V_LEVEL(var, 2)
#define DEBUGPRINT3(var) DEBUGPRINT_V_LEVEL(var, 3)
#define DEBUGPRINT4(var) DEBUGPRINT_V_LEVEL(var, 4)
#define DEBUGPRINT5(var) DEBUGPRINT_V_LEVEL(var, 5)
#define DEBUGPRINT6(var) DEBUGPRINT_V_LEVEL(var, 6)

#define DEBUGPRINT_X(var) DEBUGPRINT_X_LEVEL(var, 1)
#define DEBUGPRINT_X2(var) DEBUGPRINT_X_LEVEL(var, 2)
#define DEBUGPRINT_X3(var) DEBUGPRINT_X_LEVEL(var, 3)
#define DEBUGPRINT_X4(var) DEBUGPRINT_X_LEVEL(var, 4)
#define DEBUGPRINT_X5(var) DEBUGPRINT_X_LEVEL(var, 5)
#define DEBUGPRINT_X6(var) DEBUGPRINT_X_LEVEL(var, 6)

#define DEBUGPRINT_PURE(var) DEBUGPRINT_V_LEVEL_PURE(var, 1)
#define DEBUGPRINT2_PURE(var) DEBUGPRINT_V_LEVEL_PURE(var, 2)
#define DEBUGPRINT3_PURE(var) DEBUGPRINT_V_LEVEL_PURE(var, 3)
#define DEBUGPRINT4_PURE(var) DEBUGPRINT_V_LEVEL_PURE(var, 4)
#define DEBUGPRINT5_PURE(var) DEBUGPRINT_V_LEVEL_PURE(var, 5)
#define DEBUGPRINT6_PURE(var) DEBUGPRINT_V_LEVEL_PURE(var, 6)

#define DEBUGPRINT_NN(var0, var1) DEBUGPRINT_NN_LEVEL(var0, var1, 1)
#define DEBUGPRINT_NN_2(var0, var1) DEBUGPRINT_NN_LEVEL(var0, var1, 2)
#define DEBUGPRINT_NN_3(var0, var1) DEBUGPRINT_NN_LEVEL(var0, var1, 3)

#define DEBUGPRINT_VV(var0, var1) DEBUGPRINT_VV_LEVEL(var0, var1, 1)
#define DEBUGPRINT_VV_2(var0, var1) DEBUGPRINT_VV_LEVEL(var0, var1, 2)
#define DEBUGPRINT_VV_3(var0, var1) DEBUGPRINT_VV_LEVEL(var0, var1, 3)

#define DEBUGPRINT_SV(var0, var) DEBUGPRINT_SV_LEVEL(var0, var, 1)
#define DEBUGPRINT_SV_2(var0, var) DEBUGPRINT_SV_LEVEL(var0, var, 2)
#define DEBUGPRINT_SV_3(var0, var) DEBUGPRINT_SV_LEVEL(var0, var, 3)

#define DEBUGPRINT_SX(var0, var) DEBUGPRINT_SX_LEVEL(var0, var, 1)
#define DEBUGPRINT_SX_2(var0, var) DEBUGPRINT_SX_LEVEL(var0, var, 2)
#define DEBUGPRINT_SX_3(var0, var) DEBUGPRINT_SX_LEVEL(var0, var, 3)

// A: module->fileinfo,   B:"filename",
// C: stridx,              D:module->fileinfo_isstring,  E:true;
#define SET_INFO_PAIR(a, b, c, d, e)                               \
  do {                                                             \
    a.emplace_back(builder->GetOrCreateStringIndex(b), c);         \
    d.emplace_back(e)                                              \
  } while (0)
} // namespace maple
#endif  // AST2MPL_INCLUDE_ASTMACROS_H
