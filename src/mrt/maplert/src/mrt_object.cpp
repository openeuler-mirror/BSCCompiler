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
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cctype>
#include <cstdint>
#include "jni.h"
#include "mrt_monitor_api.h"
#include "sizes.h"
#include "mstring_inline.h"

using namespace std;
namespace maplert {
uint32_t GetObjectDWordSize(const MObject &obj) {
  uint32_t objSize = obj.GetSize();
  return (objSize + sizeof(void*) - 1) / sizeof(void*);
}

// Make and return shallow copy of object.
jobject MRT_CloneJavaObject(jobject jObj) {
  ScopedHandles sHandles;
  ObjHandle<MObject, false> mObj(MObject::JniCastNonNull(jObj));
  size_t objSz = static_cast<size_t>(mObj->GetSize());
  ObjHandle<MObject> newObj(MObject::NewObject(*(mObj->GetClass()), objSz));
  if (newObj() != 0) {
    errno_t tmpResult = memcpy_s(reinterpret_cast<uint8_t*>(newObj()), objSz,
                                 reinterpret_cast<uint8_t*>(mObj()), objSz);
    if (UNLIKELY(tmpResult != EOK)) {
      LOG(FATAL) << "memcpy_s() in MRT_CloneJavaObject() return " << tmpResult << " rather than 0." << maple::endl;
    }
    uint32_t *paddr = reinterpret_cast<uint32_t*>(newObj.AsRaw() + sizeof(MetaRef));
    *paddr = 0;
    Collector::Instance().PostObjectClone(mObj.AsRaw(), newObj.AsRaw());
  }
  return newObj.ReturnJObj();
}

size_t MRT_SizeOfObject(jobject obj) {
  return kHeaderSize + MObject::JniCastNonNull(obj)->GetSize();
}
} // namespace maplert
