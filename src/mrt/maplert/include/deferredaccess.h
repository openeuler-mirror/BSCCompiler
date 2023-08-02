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
#ifndef MRT_MAPLERT_INCLUDE_DEFERREDACCESS_H_
#define MRT_MAPLERT_INCLUDE_DEFERREDACCESS_H_
#include "marray.h"
#include "fieldmeta.h"
#include "methodmeta.h"

namespace maplert {
namespace deferredaccess {
enum DeferredInvokeType {
  kStaticCall,                 // invoke-static
  kVirtualCall,                // invoke-virtual
  kSuperCall,                  // invoke-super
  kDirectCall,                 // invoke-direct
  kInterfaceCall               // invoke-interface
}; // enum DeferredInvokeType

// this offset keep order with MCC_DeferredInvoke stack frame
enum DeferredInvokeParameterOffset {
  kDaiMethod = 0,                   // cache dai Method
  kInvokeType = 1,                  // invoke type
  kClassName = 2,                   // class name
  kMethodName = 3,                  // method name
#if defined(__aarch64__)
  kSignature = 4,                   // signature
  kThisObj = 5,                     // this obj
#elif defined(__arm__)
  kStackOffset = 20,                // StackOffset see argvalue.h, DecodeStackArgs
  kSignature = kStackOffset,        // signature
  kThisObj = kStackOffset + 1,      // this obj
#endif
}; // enum DeferredInvokeParameterOrder

// DAI struct
using DaiClass = struct DAIClass {
  MClass *daiClass;
};
using DaiField = struct DAIField {
  FieldMeta *fieldMeta;
};
using DaiMethod = struct DAIMethod {
  MethodMeta *methodMeta;
};

MClass *GetConstClass(const MObject &caller, const char *descriptor);
void ClinitCheck(const MClass &classObject);
bool IsInstanceOf(const MClass &classObject, const MObject &obj);
MObject *NewInstance(const MClass &classObject);
MArray *NewArray(const MClass &classObject, uint32_t length);
MArray *NewArray(const MClass &classObject, uint32_t length, va_list initialElement);
jvalue LoadField(const FieldMeta &fieldMeta, const MObject *obj);
void StoreField(const FieldMeta &fieldMeta, MObject *obj, jvalue value);
FieldMeta *InitDaiField(DaiField *daiField, const MClass &classObject,
                        const MString &fieldName, const MString &fieldTypeName);
jvalue Invoke(DeferredInvokeType invokeType, const MethodMeta *methodMeta, MObject *obj, jvalue args[]);
MethodMeta *InitDaiMethod(DeferredInvokeType invokeType, DaiMethod *daiMethod, const MClass &classObject,
                          const char *methodName, const char *signatureName);
bool CheckFieldAccess(MObject &caller, const FieldMeta &fieldMeta, const MObject *obj);
}; // namespace deferredaccess
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_DEFERREDACCESS_H_
