/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_CODERELAYOUT_H
#define MPL2MPL_INCLUDE_CODERELAYOUT_H
#include <fstream>
#include "phase_impl.h"
#include "file_layout.h"
#include "maple_phase_manager.h"

namespace maple {
class CodeReLayout : public FuncOptimizeImpl {
 public:
  CodeReLayout(MIRModule &mod, KlassHierarchy *kh, bool dump);
  ~CodeReLayout() override = default;

  FuncOptimizeImpl *Clone() override {
    return new CodeReLayout(*this);
  }

  void ProcessFunc(MIRFunction *func) override;
  void Finish() override;

 private:
  std::string StaticFieldFilename(const std::string &mplFile) const;
  void GenLayoutSym();
  void AddStaticFieldRecord();
  CallNode *CreateRecordFieldStaticCall(BaseNode *node, const std::string &name);
  void FindDreadRecur(const StmtNode *stmt, BaseNode *node);
  void InsertProfileBeforeDread(const StmtNode *stmt, BaseNode *opnd);
  MIRSymbol *GetorCreateStaticFieldSym(const std::string &fieldName);
  MIRSymbol *GenStrSym(const std::string &str);
  std::unordered_map<std::string, MIRSymbol*> str2SymMap;
  uint32 layoutCount[static_cast<uint32>(LayoutType::kLayoutTypeCount)] = {};
};

MAPLE_MODULE_PHASE_DECLARE(M2MCodeReLayout)
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_CODERELAYOUT_H
