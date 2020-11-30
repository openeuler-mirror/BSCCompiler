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
#ifndef MAPLE_RUNTIME_ADDRESS_H
#define MAPLE_RUNTIME_ADDRESS_H

#include <atomic>
#include <functional>
#include "gc_callback.h"

// Operations for the address type and memory operations
namespace maplert {
static constexpr address_t kDummyAddress = static_cast<address_t>(-1);

template<class T>
inline T &AddrToLVal(address_t addr) {
  return *reinterpret_cast<T*>(addr);
}

template<class T>
inline std::atomic<T> &AddrToLValAtomic(address_t addr) {
  return *reinterpret_cast<std::atomic<T>*>(addr);
}

static inline address_t RefFieldToAddress(reffield_t refField) {
  return static_cast<address_t>(refField);
}

static inline reffield_t AddressToRefField(address_t addr) {
  return static_cast<reffield_t>(addr);
}

// Note: only used to load reference field of Java object
//       raw load, no RC-bookkeeping
static inline address_t LoadRefField(address_t *fieldAddr) {
  return static_cast<address_t>(*reinterpret_cast<reffield_t*>(fieldAddr));
}

// dialect of the above version. Doesn't do NULL-check against obj
static inline address_t LoadRefField(address_t obj, std::size_t offset) {
  return LoadRefField(reinterpret_cast<address_t*>(obj + offset));
}

// Note: only used to store reference field of Java object
//       raw store, no RC-bookkeeping
static inline void StoreRefField(address_t *fieldAddr, address_t newVal) {
  *reinterpret_cast<reffield_t*>(fieldAddr) = static_cast<reffield_t>(newVal);
}

// dialect of the above version. Doesn't do NULL-check against obj
static inline void StoreRefField(address_t obj, std::size_t offset, address_t newVal) {
  StoreRefField(reinterpret_cast<address_t*>(obj + offset), newVal);
}
}
#endif
