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

#ifndef HIR2MPL_BC_INPUT_INCLUDE_BC_INPUT_H
#define HIR2MPL_BC_INPUT_INCLUDE_BC_INPUT_H
#include <string>
#include <list>
#include "mir_module.h"
#include "bc_class.h"
#include "bc_parser.h"

namespace maple {
namespace bc {
template <class T>
class BCInput {
 public:
  explicit BCInput(MIRModule &moduleIn) : module(moduleIn) {}
  ~BCInput() = default;
  bool ReadBCFile(uint32 index, const std::string &fileName, const std::list<std::string> &classNamesIn);
  bool ReadBCFiles(const std::vector<std::string> &fileNames, const std::list<std::string> &classNamesIn);
  const MIRModule &GetModule() const {
    return module;
  }
  BCClass *GetFirstClass();
  BCClass *GetNextClass();
  void RegisterFileInfo(const std::string &fileName);
  bool CollectDependentTypeNamesOnAllBCFiles(std::unordered_set<std::string> &allDepSet);
  bool CollectClassNamesOnAllBCFiles(std::unordered_set<std::string> &allClassSet);

 private:
  bool CollectAllDepTypeNamesOnAllBCFiles(std::unordered_set<std::string> &allDepSet);
  bool CollectMethodDepTypeNamesOnAllBCFiles(std::unordered_set<std::string> &allDepSet);

  std::map<std::string, std::unique_ptr<BCParserBase>> bcParserMap;  // map<filename, BCParser>
  MIRModule &module;

  std::list<std::unique_ptr<BCClass>> bcClasses;
  std::list<std::unique_ptr<BCClass>>::const_iterator itKlass;
};
}
}
#include "bc_input-inl.h"
#endif  // HIR2MPL_BC_INPUT_INCLUDE_BC_INPUT_H