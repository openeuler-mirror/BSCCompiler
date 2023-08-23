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
#include "java_lang_Thread.h"
#include "chelper.h"
#include "thread_api.h"
using namespace maplert;

#ifdef __cplusplus
extern "C" {
#endif

jobject Native_Thread_currentThread() {
  jobject thread = maple::IThread::Current()->GetRawPeer();
  RC_LOCAL_INC_REF(thread);
  return thread;
}

#ifdef __cplusplus
}
#endif