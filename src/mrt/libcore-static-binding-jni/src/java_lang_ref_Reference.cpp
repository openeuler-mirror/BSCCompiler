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
#include "java_lang_ref_Reference.h"
#include "java2c_rule.h"
#include "mrt_reference_api.h"
using namespace maplert;

#ifdef __cplusplus
extern "C" {
#endif

address_t Ljava_2Flang_2Fref_2FReference_3B_7Cget_7C_28_29Ljava_2Flang_2FObject_3B(address_t javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  address_t ret = MRT_ReferenceGetReferent(javaThis);
  j2cRule.Epilogue();
  return ret;
}

void Ljava_2Flang_2Fref_2FReference_3B_7Cclear_7C_28_29V(address_t javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  if (javaThis != 0) {
    MRT_ReferenceClearReferent(javaThis);
  }
  j2cRule.Epilogue();
}

#ifdef __cplusplus
}
#endif
