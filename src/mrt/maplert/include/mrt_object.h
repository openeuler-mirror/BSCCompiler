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
#ifndef JAVA_MRT_OBJECT_H_
#define JAVA_MRT_OBJECT_H_
#include "mobject.h"
#include "base/logging.h"

namespace maplert {
uint32_t GetObjectDWordSize(const MObject &obj);

inline void *MRT_GetAddress32ByAddress(uint64_t *addr) {
  DCHECK(addr != nullptr) << "MRT_GetAddress32ByAddress: addr is nullptr!" << maple::endl;
  return reinterpret_cast<void*>(uint64_t{ *reinterpret_cast<uint32_t*>(addr) });
}

inline void *MRT_GetAddressByAddress(uint64_t *addr) {
  return reinterpret_cast<void*>(*addr);
}
} // namespace maplert
#endif // JAVA_MRT_OBJECT_H_
