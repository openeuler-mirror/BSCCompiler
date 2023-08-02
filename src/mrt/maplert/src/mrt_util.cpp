/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "mrt_util.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cctype>
#include <map>
#include "namemangler.h"
#include "panic.h"
#include "mrt_reflection_class.h"
#include "itab_util.h"

namespace maplert {
// string _3B, _7C, _29 is the ascii code for some special character
const int kAsciiStrLength = 3;

// typechar
//  V   void
//  Z   boolean u8
//  B   byte    i8
//  S   short   i16
//  C   char    u16
//  I   int     i32
//  J   long    i64
//  F   float   f32
//  D   double  f64
//  L   ref     u64
//  A   array
//  _   not array

// two usages:
// short form: when typenames passed as NULL
//    return a string for its prototype
//    each argument is represented by two charactors, A/_ + typechar
//    followed by 2 charactors for return type
// normal form:
//    typenames contains char* for arg types and return type
//    in this case, return is NULL
extern "C" char *MRT_FuncnameToPrototypeNames(char *funcName, int argNum, char **typeNames) {
  bool shortForm = true;
  if (typeNames != nullptr) {
    shortForm = false;
  }
  DCHECK(funcName != nullptr) << "MRT_FuncnameToPrototypeNames: funcName is nullptr" << maple::endl;
  // arg type names and return type name
  char *name = funcName;
  unsigned index = 0;
  int protoArgNum = 0;
  unsigned endPos = 0;
  while (*name) {
    if (*name == '_') {
      if (strncmp(name, "_28", kAsciiStrLength) == 0) {
        name += kAsciiStrLength;
        break;
      } else if (endPos == 0 && strncmp(name, "_7C", kAsciiStrLength) == 0) {
        endPos = static_cast<unsigned>(name - funcName);
      }
    }
    name++;
  }

  // collect protoArgNum, whether need to insert this
  char *name0 = name;
  while (*name) {
    if (*name == '[') {
      name++;
    }
    if (*name == 'L') {
      while (!(*name == '_' && strncmp(name, "_3B", kAsciiStrLength) == 0)) {
        name++;
      }
      // when the current pointer meet substring of "_3B", pointer skip 2 steps firstly.
      // then skip more 1 step to skip over substring of "_3B" for code name++;
      name += 2;
    } else if (*name == '_' && strncmp(name, "_29", kAsciiStrLength) == 0) {
      break;
    }
    name++;
    protoArgNum++;
  }

  if (protoArgNum != argNum) {
    printf("argnum = %d; protoargnum = %d\n", argNum, protoArgNum);
    __MRT_ASSERT(0, "");
  }

  char *retPtr = nullptr;
  size_t size;
  if (shortForm) {
    size = static_cast<size_t>(argNum) * 2 + 3; // args*2 + ret*2 + '\0'
  } else {
    size = strlen(funcName) + protoArgNum;
  }
  if (size == 0) {
    return nullptr;
  }
  retPtr = reinterpret_cast<char*>(calloc(size, 1));
  if (retPtr == nullptr) {
    return nullptr;
  }
  char *ret = retPtr;

  name = name0;
  while (*name) {
    char *ret0 = ret;
    if (*name == '[') {
      name++;
      *ret++ = '[';
    } else {
      if (shortForm) {
        *ret++ = '_';
      }
    }

    if (*name == 'L') {
      *ret++ = 'L';
      name++;
      while (!(*name == '_' && strncmp(name, "_3B", kAsciiStrLength) == 0)) {
        if (!shortForm) {
          *ret++ = *name;
        }
        name++;
      }
      name += 2; // skip 2 steps
      if (!shortForm) {
        *ret++ = '_';
        *ret++ = '3';
        *ret++ = 'B';
      }
    } else if (*name == '_' && strncmp(name, "_29", kAsciiStrLength) == 0) {
      if (shortForm) {
        ret--;
      }
      name += 3; // skip 3 steps
      continue;
    } else {
      *ret++ = *name;
    }

    name++;

    if (!shortForm) {
      ret++;
      typeNames[index++] = ret0;
    }
  }

  return retPtr;
}

// function GetClassNametoDescriptor calloc space for javaDescriptor
// remember to free when used in another place
// the className after Decode is shorter so that the classNameLength is large Enough
std::string GetClassNametoDescriptor(const std::string &className) {
  std::string name = className;
  std::string descriptor;
  namemangler::DecodeMapleNameToJavaDescriptor(name, descriptor);
  return descriptor;
}
} // namespace maplert
