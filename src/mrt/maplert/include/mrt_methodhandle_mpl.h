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
#ifndef MAPLE_RUNTIME_METHODHANDLEMPL
#define MAPLE_RUNTIME_METHODHANDLEMPL

#include <jni.h>
#include <iostream>
#include <algorithm>
#include <mutex>
#include "cpphelper.h"
#include "chelper.h"
#include "mrt_libs_api.h"
#include "exception/mrt_exception.h"
#include "mrt_well_known.h"
#include "mrt_primitive_util.h"
#include "mrt_handlecommon.h"
namespace maplert {
enum class OptionFlag : int32_t {
  kFinal,
  kDropArguments,
  kFilterReturnValue,
  kBindto,
  kPermuteArguments
};

jvalue MethodHandleCallEnter(MString *calleeName, MString*, uint32_t paramNum,
                             MObject *methodHandle, VArg &args, const MClass *declareClass);

class MethodHandleMpl {
 public:
  MethodHandleMpl(MObject *obj, bool isExact) :methodHandle(obj), isInvokeExact(isExact) {
    // maybe need check the array length
    dataArray =
        reinterpret_cast<MArray*>(methodHandle->LoadObjectNoRc(WellKnown::GetMFieldMethodHandleDataArrayOffset()));
    metaArray =
        reinterpret_cast<MArray*>(methodHandle->LoadObjectNoRc(WellKnown::GetMFieldMethodHandleMetaArrayOffset()));
    opArray =
        reinterpret_cast<MArray*>(methodHandle->LoadObjectNoRc(WellKnown::GetMFieldMethodHandleOpArrayOffset()));
    typeArray =
        reinterpret_cast<MArray*>(methodHandle->LoadObjectNoRc(WellKnown::GetMFieldMethodHandleTypeArrayOffset()));
    transformNum =
        static_cast<uint32_t>(maplert::MRT_LOAD_JINT(methodHandle, WellKnown::GetMFieldMethodHandleIndexOffset()));
  }
  ~MethodHandleMpl() = default;
  static bool IsExactInvoke(MString *calleeName) {
    constexpr char kInvokeExact[] =
        "Ljava_2Flang_2Finvoke_2FMethodHandle_3B_7CinvokeExact_7C_28ALjava_2Flang_2FObject_3B_29" \
        "Ljava_2Flang_2FObject_3B";
    if (!strcmp(MRT_GetStringContentsPtrRaw(reinterpret_cast<jstring>(calleeName)), kInvokeExact)) {
      return true;
    }
    return false;
  }

  void CheckReturnType();
  string GeneNoSuchMethodExceptionString(const MethodType &methodType, const MethodMeta &method);
  jvalue invoke(vector<jvalue>&paramPtr, uint32_t paramNum, vector<char>&typesMarkPtr, vector<MClass*>&paramTypes,
                bool convertRetVal = true);
 private:
  MethodMeta *GetMeta(uint32_t index) const {
    MethodMeta *type = reinterpret_cast<MethodMeta*>(metaArray->GetObjectElementNoRc(index));
    return type;
  }
  bool CheckParamsType(vector<MClass*> &cSTypes, uint32_t csTypesNum, vector<jvalue>&, uint32_t, bool checkRet = true);
  void PrepareVarg(vector<jvalue> &paramPtr, vector<char> &typesMark, vector<MClass*> &cSTypes, uint32_t &csTypesNum);
  jvalue FilterReturnValue(vector<jvalue>&, const uint32_t, const MObject*, vector<char>&, vector<MClass*>&);
  MObject *methodHandle;
  MArray *dataArray;
  MArray *metaArray;
  MArray *opArray;
  MArray *typeArray;
  uint32_t transformNum;
  bool isInvokeExact;
};
}
#endif
