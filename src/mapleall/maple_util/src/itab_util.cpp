/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "itab_util.h"
#include <map>
#include <mutex>
#include <cstring>

namespace maple {
unsigned int DJBHash(const char *str) {
  unsigned int hash = 5381; // initial value for DJB hash algorithm
  while (*str) {
    hash += (hash << 5) + (unsigned char)(*str++); // calculate the hash code of data
  }
  return (hash & 0x7FFFFFFF);
}

unsigned int GetHashIndex(const char *name) {
  unsigned int hashcode = DJBHash(name);
  return (hashcode % kHashSize);
}

struct CmpStr {
  bool operator()(char const *a, char const *b) const {
    return std::strcmp(a, b) < 0;
  }
};

std::mutex mapLock;

unsigned int GetSecondHashIndex(const char *name) {
  std::lock_guard<std::mutex> guard(mapLock);
  unsigned int hashcode = DJBHash(name);
  return hashcode % kItabSecondHashSize;
}
}  // namespace maple
