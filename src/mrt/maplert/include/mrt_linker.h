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
#ifndef MRT_CLASS_MPLLINKER_IMPL_H_
#define MRT_CLASS_MPLLINKER_IMPL_H_

#include "jni.h"
#include "linker_api.h"
#include "linker/linker_model.h"
#include "linker/linker.h"
#include "linker/linker_inline.h"
#include "linker/linker_hotfix.h"
#include "linker/linker_debug.h"
#include "linker/linker_cache.h"
#include "linker/linker_method_builder.h"
#ifdef LINKER_DECOUPLE
#include "linker/decouple/linker_decouple.h"
#endif
#include "linker/linker_lazy_binding.h"

namespace maplert {
class MplLinkerImpl : public LinkerInvoker {
 public:
  MplLinkerImpl();
  ~MplLinkerImpl() = default;
 private:
  Linker linkerFeature;
  Hotfix hotfixFeature;
  MethodBuilder methodBuilderFeature;
#ifdef LINKER_DECOUPLE
  Decouple decoupleFeature;
#endif
  LazyBinding lazyBindingFeature;
#ifdef LINKER_RT_CACHE
  LinkerCache linkerCacheFeature;
#endif // LINKER_RT_CACHE
  Debug debugFeature;
};
} // namespace maplert
#endif // MRT_CLASS_MPLLINKER_IMPL_H_
