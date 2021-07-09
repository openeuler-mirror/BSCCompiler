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
#include "chelper.h"
#include "mrt_class_init.h"

#define CLASS_PREFIX(className) extern void *MRT_CLASSINFO(className);
#define MRT_CLASSINFO(className) __MRT_MAGIC_PASTE(CLASSINFO_PREFIX, className)
#include "white_list.def"
#undef MRT_CLASSINFO
#undef CLASS_PREFIX

namespace maplert {
extern "C" {

void MRT_BootstrapClinit(void) {
#define CLASS_PREFIX(className) (void)MRT_TryInitClass(*reinterpret_cast<MClass*>(&MRT_CLASSINFO(className)));
#define MRT_CLASSINFO(className) __MRT_MAGIC_PASTE(CLASSINFO_PREFIX, className)
#ifdef __ANDROID__

#include "white_list.def"

#else
  (void)MRT_TryInitClass(*reinterpret_cast<MClass*>(&MRT_CLASSINFO(Ljava_2Flang_2FString_3B)));
  (void)MRT_TryInitClass(*reinterpret_cast<MClass*>(&MRT_CLASSINFO(Ljava_2Flang_2FByte_24ByteCache_3B)));
  (void)MRT_TryInitClass(*reinterpret_cast<MClass*>(&MRT_CLASSINFO(Ljava_2Flang_2FByte_3B)));
#endif
#undef MRT_CLASSINFO
#undef CLASS_PREFIX
}

} // extern "C"
} // namespace maplert
