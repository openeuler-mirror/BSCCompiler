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
  unsigned int hash = 5381; // 5381: initial value for DJB hash algorithm
  while (*str != 0) {
    hash += (hash << 5) + static_cast<unsigned char>(*str++); // 5: calculate the hash code of data
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

// optimization for hot method retrival.
// check risk incurred by multi-threads.
std::map<const char*, unsigned int, CmpStr> hotMethodCache;
std::map<const char*, unsigned int>::const_iterator it;
std::mutex mapLock;

unsigned int GetSecondHashIndex(const char *name) {
  std::lock_guard<std::mutex> guard(mapLock);
  if (hotMethodCache.size() == 0) {
    for (unsigned int i = 0; i < ITAB_HOTMETHOD_SIZE; ++i) {
      hotMethodCache.emplace(itabHotMethod[i], i);
    }
  }

  it = hotMethodCache.find(name);
  if (it != hotMethodCache.end()) {
    return it->second;
  }
  unsigned int hashcode = DJBHash(name);
  return ITAB_HOTMETHOD_SIZE + (hashcode % (kItabSecondHashSize - ITAB_HOTMETHOD_SIZE));
}
}  // namespace maple
