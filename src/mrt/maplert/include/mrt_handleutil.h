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
#ifndef MAPLE_RUNTIME_HANDLEUTIL
#define MAPLE_RUNTIME_HANDLEUTIL

#include "mrt_methodhandle.h"
#include "mrt_handlecommon.h"
namespace maplert {
struct ConvertJStruct {
  MClass **parameterTypes;
  uint32_t arraySize;
  jvalue *value;
};
struct TypeInfo {
  MClass **types;
  char *typesMark;
};

bool ConvertJvalue(ConvertJStruct&, const MethodHandle&, const MClass *from, const MClass *to, bool needDec = false);

bool ConvertParams(CallParam&, const MethodHandle&, const ArgsWrapper&, BaseArgValue&, ScopedHandles&);

string GeneExceptionString(const MethodHandle &mh, MClass **parameterTypes, uint32_t arraySize);

bool ConvertReturnValue(const MethodHandle &mh, MClass **paramTypes, uint32_t arraySize, jvalue &value);

void IsConvertibleOrThrow(uint32_t arraySize, const MethodHandle &mh, MClass **paramTypes, uint32_t paramNum);

bool ClinitCheck(const MClass *mplFieldOrMethod);

static inline bool IsConvertible(const MethodHandle &mh, uint32_t arraySize) {
  const MethodType *methodTypeMplObj = mh.GetMethodTypeMplObj();
  vector<MClass*> ptypesVal = methodTypeMplObj->GetParamsType();
  if (arraySize - 1 != ptypesVal.size()) {
    return false;
  }
  return true;
}

void GetParameterAndRtType(MClass **types, char *typesMark, MString *protoStr, SizeInfo &sz,
                           const MClass *declareClass);
jvalue PolymorphicCallEnter(const MString *calleeStr, const MString*,
                            uint32_t, MObject *mhObj, Arg&, const MClass *declareClass = nullptr);
MClass *GetContextCls(Arg &arg, MString *&calleeName);

#if defined(__arm__)
extern "C" int64_t PolymorphicCallEnter32(int32_t *args);
#endif
}
#endif
