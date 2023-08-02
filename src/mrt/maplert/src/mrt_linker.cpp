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
#include "mrt_linker.h"
#include "mrt_linker_api.h"

namespace maplert {
LinkerAPI *LinkerAPI::pInstance = nullptr;
LinkerAPI &LinkerAPI::Instance() {
  if (pInstance == nullptr) {
    pInstance = new (std::nothrow) MplLinkerImpl();
    if (pInstance == nullptr) {
      LINKER_LOG(FATAL) << "new MplLinkerImpl failed" << maple::endl;
    }
  }
  return *pInstance;
}
MplLinkerImpl::MplLinkerImpl()
    : linkerFeature(*this),
      hotfixFeature(*this),
      methodBuilderFeature(*this),
#ifdef LINKER_DECOUPLE
      decoupleFeature(*this, methodBuilderFeature),
#endif
      lazyBindingFeature(*this, methodBuilderFeature),
#ifdef LINKER_RT_CACHE
      linkerCacheFeature(*this),
#endif // LINKER_RT_CACHE
      debugFeature(*this) {
  features[kFLinker] = &linkerFeature;
  features[kFHotfix] = &hotfixFeature;
  features[kFDebug] = &debugFeature;
  features[kFMethodBuilder] = &methodBuilderFeature;
#ifdef LINKER_DECOUPLE
  features[kFDecouple] = &decoupleFeature;
#endif
  features[kFLazyBinding] = &lazyBindingFeature;
#ifdef LINKER_RT_CACHE
  features[kFLinkerCache] = &linkerCacheFeature;
#endif // LINKER_RT_CACHE
}
#ifdef __cplusplus
extern "C" {
#endif
bool MRT_LinkerIsJavaText(const void *addr) {
  return LinkerAPI::Instance().IsJavaText(addr);
}
void *MRT_LinkerGetSymbolAddr(void *handle, const char *symbol, bool isFunction) {
  return LinkerAPI::Instance().GetSymbolAddr(handle, symbol, isFunction);
}
void MRT_LinkerSetCachePath(const char *path) {
#ifdef LINKER_RT_CACHE
  LinkerAPI::Instance().SetCachePath(path);
#else
  (void)path;
#endif // LINKER_RT_CACHE
}
#ifdef __cplusplus
}
#endif
} // namespace maplert
