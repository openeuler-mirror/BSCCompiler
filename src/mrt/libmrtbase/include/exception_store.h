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

#ifndef EXCEPTION_STORE_H
#define EXCEPTION_STORE_H

#include "jni.h"
#include "tls_store.h"

namespace maple {

class ExceptionVisitor {
 public:
  static inline jobject GetPendingException() {
    return (jobject)maple::tls::GetTLS(tls::kSlotException);
  }

  static inline void SetPendingException(const jobject ex) {
    maple::tls::StoreTLS(reinterpret_cast<void*>(ex), tls::kSlotException);
  }

  static inline jobject GetThrowningException() {
    return (jobject)maple::tls::GetTLS(tls::kSlotThrowException);
  }

  static inline void SetThrowningException(const jobject ex) {
    maple::tls::StoreTLS(reinterpret_cast<void*>(ex), tls::kSlotThrowException);
  }

  static inline void *GetSignalHandler() {
    return reinterpret_cast<void*>(maple::tls::GetTLS(tls::kSlotSignalHandler));
  }

  static inline void  SetSignalHandler(const void *args) {
    maple::tls::StoreTLS(args, tls::kSlotSignalHandler);
  }

  static inline void *GetExceptionAddress() {
    return reinterpret_cast<void*>(maple::tls::GetTLS(tls::kSlotExceptionAddress));
  }

  static inline void SetExceptionAddress(const void *addr) {
    maple::tls::StoreTLS(addr, tls::kSlotExceptionAddress);
  }

};

} // namespace maple

#endif //EXCEPTION_STORE_H
