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
#include "java_lang_reflect_Array.h"
#include "mrt_array.h"
#include "exception/mrt_exception.h"
#include "utils/string_utils.h"
using namespace maplert;

#ifdef __cplusplus
extern "C" {
#endif

jobject Java_java_lang_reflect_Array_createObjectArray__Ljava_lang_Class_2I(
    JNIEnv *env, jclass, jobject componentClass, jint length) {
  maplert::ScopedObjectAccess soa;
  if (UNLIKELY(length < 0)) {
    std::string msg = std::to_string(length);
    MRT_ThrowNewException("java/lang/NegativeArraySizeException", msg.c_str());
    return nullptr;
  }
  return MRT_JNI_AddLocalReference(env, MRT_NewObjArray(length, static_cast<jclass>(componentClass), nullptr));
}

jobject Java_java_lang_reflect_Array_createMultiArray__Ljava_lang_Class_2_3I(
    JNIEnv *env, jclass, jclass javaElementClass, jobject javaDimArray) {
  maplert::ScopedObjectAccess soa;
  DCHECK(javaDimArray != nullptr) << "reflect_Array_createMultiArry:: javaDimArray is nullptr" << maple::endl;
  MArray *mDimArray = reinterpret_cast<MArray*>(javaDimArray);
  uint32_t dimCount =  mDimArray->GetLength();
  jint *dimArray = reinterpret_cast<jint*>(mDimArray->ConvertToCArray());
  jclass arrayClass = javaElementClass;
  for (uint32_t i = 0; i < dimCount; i++) {
    jint dimension = dimArray[i];
    arrayClass = MRT_ReflectGetOrCreateArrayClassObj(arrayClass);
    if (UNLIKELY(arrayClass == nullptr)) {
      LOG(FATAL) << "the result of MRT_ReflectGetOrCreateArrayClassObj() from arrayClass is nullptr" << maple::endl;
    }
    if (UNLIKELY(dimension < 0)) {
      MRT_ThrowNewException("java/lang/NegativeArraySizeException",
                            stringutils::Format("Dimension %u: %d", i, dimension).c_str());
      return nullptr;
    }
  }
  return MRT_JNI_AddLocalReference(env, MRT_RecursiveCreateMultiArray(arrayClass, 0, dimCount, dimArray));
}

#ifdef __cplusplus
}
#endif
