/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_BC_INPUT_INCLUDE_BC_INPUT_INL_H_
#define HIR2MPL_BC_INPUT_INCLUDE_BC_INPUT_INL_H_
#include "bc_input.h"
#include <typeinfo>
#include "dex_parser.h"
#include "hir2mpl_env.h"

namespace maple {
namespace bc {
template <class T>
bool BCInput<T>::ReadBCFile(uint32 index, const std::string &fileName, const std::list<std::string> &classNamesIn) {
  std::unique_ptr<BCParserBase> bcParser;
  if (typeid(T) == typeid(DexReader)) {
    bcParser = std::make_unique<DexParser>(index, fileName, classNamesIn);
  } else {
    CHECK_FATAL(false, "Unsupported BC reader: %s in BCInput", typeid(T).name());
  }
  if (!bcParser->OpenFile()) {
    return false;
  }
  if (!bcParser->ParseHeader()) {
    ERR(kLncErr, "Parse Header failed in : %s.", fileName.c_str());
    return false;
  }
  if (!bcParser->Verify()) {
    ERR(kLncErr, "Verify file failed in : %s.", fileName.c_str());
    return false;
  }
  if (bcParser->RetrieveClasses(bcClasses) == false) {
    ERR(kLncErr, "Retrieve classes failed in : %s.", fileName.c_str());
    return false;
  }
  (void)bcParserMap.insert(std::make_pair(fileName, std::move(bcParser)));
  return true;
}

template <class T>
bool BCInput<T>::ReadBCFiles(const std::vector<std::string> &fileNames, const std::list<std::string> &classNamesIn) {
  for (uint32 i = 0; i < fileNames.size(); ++i) {
    if (BCInput<T>::ReadBCFile(i, fileNames[i], classNamesIn) == false) {
      return false;
    }
    RegisterFileInfo(fileNames[i]);
  }
  return true;
}

template <class T>
BCClass *BCInput<T>::GetFirstClass() {
  if (bcClasses.size() == 0) {
    return nullptr;
  }
  itKlass = bcClasses.begin();
  return itKlass->get();
}

template <class T>
BCClass *BCInput<T>::GetNextClass() {
  if (itKlass == bcClasses.end()) {
    return nullptr;
  }
  ++itKlass;
  if (itKlass == bcClasses.end()) {
    return nullptr;
  }
  return itKlass->get();
}

template <class T>
void BCInput<T>::RegisterFileInfo(const std::string &fileName) {
  GStrIdx fileNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fileName);
  GStrIdx fileInfoIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename");
  module.PushFileInfoPair(MIRInfoPair(fileInfoIdx, fileNameIdx));
  module.PushFileInfoIsString(true);
}

template <class T>
bool BCInput<T>::CollectDependentTypeNamesOnAllBCFiles(std::unordered_set<std::string> &allDepSet) {
  FEOptions::ModeCollectDepTypes mode = FEOptions::GetInstance().GetModeCollectDepTypes();
  bool isSuccess = true;
  switch (mode) {
    case FEOptions::ModeCollectDepTypes::kAll:
      isSuccess = CollectAllDepTypeNamesOnAllBCFiles(allDepSet);
      break;
    case FEOptions::ModeCollectDepTypes::kFunc:
      isSuccess = CollectMethodDepTypeNamesOnAllBCFiles(allDepSet);
      break;
  }
  return isSuccess;
}

template <class T>
bool BCInput<T>::CollectAllDepTypeNamesOnAllBCFiles(std::unordered_set<std::string> &allDepSet) {
  for (auto &item : bcParserMap) {
    std::unordered_set<std::string> depSet;
    if (item.second->CollectAllDepTypeNames(depSet) == false) {
      ERR(kLncErr, "Collect all dependent typenames failed in : %s.", item.first.c_str());
      return false;
    }
    allDepSet.insert(depSet.cbegin(), depSet.cend());
  }
  BCUtil::AddDefaultDepSet(allDepSet);  // allDepSet is equal to "DefaultTypeSet + TypeSet - ClassSet"
  std::unordered_set<std::string> classSet;
  (void)CollectClassNamesOnAllBCFiles(classSet);
  for (const auto &elem : classSet) {
    (void)allDepSet.erase(elem);
  }
  return true;
}

template <class T>
bool BCInput<T>::CollectMethodDepTypeNamesOnAllBCFiles(std::unordered_set<std::string> &depSet) {
  bool isSuccess = true;
  for (const std::unique_ptr<BCClass> &klass : bcClasses) {
    if (klass == nullptr) {
      continue;
    }
    std::list<std::string> superClassNames = klass->GetSuperClassNames();
    depSet.insert(superClassNames.cbegin(), superClassNames.cend());
    std::vector<std::string> superInterfaceNames = klass->GetSuperInterfaceNames();
    depSet.insert(superInterfaceNames.cbegin(), superInterfaceNames.cend());
    for (const std::unique_ptr<BCClassMethod> &method : klass->GetMethods()) {
      if (method == nullptr) {
        continue;
      }
      (void)klass->GetBCParser().CollectMethodDepTypeNames(depSet, *method);
    }
  }
  std::unordered_set<std::string> classSet;
  isSuccess = isSuccess && CollectClassNamesOnAllBCFiles(classSet);
  BCUtil::AddDefaultDepSet(depSet);  // DepSet is equal to "DefaultTypeSet + TypeSet - ClassSet"
  for (const auto &elem : classSet) {
    (void)depSet.erase(elem);
  }
  return isSuccess;
}

template <class T>
bool BCInput<T>::CollectClassNamesOnAllBCFiles(std::unordered_set<std::string> &allClassSet) {
  for (auto &item : bcParserMap) {
    std::unordered_set<std::string> classSet;
    if (item.second->CollectAllClassNames(classSet) == false) {
      ERR(kLncErr, "Collect all class names failed in : %s.", item.first.c_str());
      return false;
    }
    allClassSet.insert(classSet.cbegin(), classSet.cend());
  }
  return true;
}
}  // namespace bc
}  // namespace maple
#endif  // HIR2MPL_BC_INPUT_INCLUDE_BC_INPUT_INL_H_
