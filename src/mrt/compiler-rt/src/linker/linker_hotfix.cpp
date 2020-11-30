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
#include "linker/linker_hotfix.h"

#include "linker/linker_model.h"
#include "linker/linker_inline.h"
#include "linker/linker_cache.h"
#include "linker/linker.h"

namespace maplert {
using namespace linkerutils;
FeatureName Hotfix::featureName = kFHotfix;
bool Hotfix::ResetResolvedFlags(LinkerMFileInfo &mplInfo) {
  LINKER_VLOG(mpllinker) << mplInfo.name << maple::endl;
  mplInfo.SetFlag(kIsMethodDefResolved, false);
  mplInfo.SetFlag(kIsVTabResolved, false);
  mplInfo.SetFlag(kIsITabResolved, false);
  mplInfo.SetFlag(kIsSuperClassResolved, false);
  mplInfo.SetFlag(kIsGCRootListResolved, false);
  mplInfo.SetFlag(kIsMethodRelocated, false);
  mplInfo.SetFlag(kIsDataRelocated, false);
  return true;
}

bool Hotfix::ReleaseCaches(LinkerMFileInfo &mplInfo) {
  LINKER_VLOG(mpllinker) << mplInfo.name << maple::endl;
#ifdef LINKER_RT_CACHE
  pInvoker->Get<LinkerCache>()->RemoveAllTables(mplInfo);
  pInvoker->Get<LinkerCache>()->FreeAllTables(mplInfo);
#endif // LINKER_RT_CACHE
  return true;
}

void Hotfix::SetClassLoaderParent(const MObject *classLoader, const MObject *newParent) {
  LINKER_VLOG(hotfix) << classLoader << ", " << newParent << maple::endl;
  pInvoker->ResetClassLoaderList(classLoader);
  pInvoker->ResetClassLoaderList(newParent);
  (void)(pInvoker->ForEachDoAction(this, &Hotfix::ResetResolvedFlags));
  (void)(pInvoker->ForEachDoAction(this, &Hotfix::ReleaseCaches));
  (void)(pInvoker->Get<Linker>()->HandleMethodSymbol());
  (void)(pInvoker->Get<Linker>()->HandleDataSymbol());
}

void Hotfix::InsertClassesFront(const MObject *classLoader, const LinkerMFileInfo &mplInfo, const std::string &path) {
  LINKER_VLOG(hotfix) << "insert path[" << path << "] in ClassLoader[" << classLoader << "], isLazyBinding=" <<
      mplInfo.IsFlag(kIsLazy) << maple::endl;
  if (!mplInfo.IsFlag(kIsLazy)) {
    (void)(pInvoker->ForEachDoAction(this, &Hotfix::ResetResolvedFlags));
    (void)(pInvoker->ForEachDoAction(this, &Hotfix::ReleaseCaches));
    (void)(pInvoker->Get<Linker>()->HandleMethodSymbol());
    (void)(pInvoker->Get<Linker>()->HandleDataSymbol());
  }
}

void Hotfix::SetPatchPath(const std::string &path, int32_t mode) {
  LINKER_VLOG(hotfix) << "set patch path[" << path << "], mode =" << mode << maple::endl;
  patchPath = path;
  patchMode = mode;
}

bool Hotfix::IsFrontPatchMode(const std::string &path) {
  return (patchMode == 1 && path == patchPath) ? true : false; // 0 - classloader parent, 1 - dexpath list
}

std::string Hotfix::GetPatchPath() {
  return patchPath;
}
} // namespace maplert
