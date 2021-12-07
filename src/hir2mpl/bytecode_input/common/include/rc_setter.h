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
#ifndef BC_INPUT_INCLUDE_RC_SETTER_H
#define BC_INPUT_INCLUDE_RC_SETTER_H
#include <string>
#include <map>
#include <set>
#include "types_def.h"
#include "bc_class.h"
#include "mir_type.h"
#include "mir_function.h"

namespace maple {
namespace bc {
class RCSetter {
 public:
  static RCSetter &GetRCSetter() {
    ASSERT(rcSetter != nullptr, "rcSetter is not initialize");
    return *rcSetter;
  }

  static void InitRCSetter(const std::string &whiteListName) {
    rcSetter = new RCSetter(whiteListName);
  }

  static void ReleaseRCSetter() {
    if (rcSetter != nullptr) {
      delete rcSetter;
      rcSetter = nullptr;
    }
  }

  void ProcessClassRCAnnotation(const GStrIdx &classIdx, const std::vector<MIRPragma*> &pragmas);
  void ProcessMethodRCAnnotation(MIRFunction &mirFunc, const std::string &className,
                                 MIRStructType &structType, const MIRPragma &pragma);
  void ProcessFieldRCAnnotation(const StructElemNameIdx &fieldElemNameIdx,
                                const MIRType &fieldType, const std::vector<MIRPragma*> &pragmas);
  void GetUnownedVarInLocalVars(const BCClassMethod &method, MIRFunction &mirFunction);
  void CollectUnownedLocalFuncs(MIRFunction *func);
  void CollectUnownedLocalVars(MIRFunction *func, const GStrIdx &strIdx);
  void CollectInputStmtField(StmtNode *stmt, const GStrIdx &fieldName);
  void SetRCFieldAttrByWhiteList(FieldAttrs &attr, const std::string &className, const std::string &fieldName);
  void SetAttrRCunowned(MIRFunction &mirFunction, const std::set<uint16> &unownedRegNums) const;
  void MarkRCAttributes() const;

 private:
  explicit RCSetter(const std::string &whiteListName) : rcFieldAttrWhiteListName(whiteListName) {
    if (!rcFieldAttrWhiteListName.empty()) {
      LoadRCFieldAttrWhiteList(rcFieldAttrWhiteListName);
    }
  }
  ~RCSetter() = default;
  void MarkRCUnownedForUnownedLocalFunctions() const;
  void MarkRCUnownedForAnonymousClasses() const;
  void MarkRCUnownedForLambdaClasses() const;
  void SetRCUnownedAttributeInternalSetFieldAttrRCUnowned(size_t opnd, const MIRFunction &calledFunc) const;
  void SetRCUnownedAttribute(const CallNode &callNode, MIRFunction &func,
                             const MIRFunction &calledFunc, const std::set<GStrIdx> &gStrIdx) const;
  bool IsAnonymousInner(const MIRStructType &structType) const;
  bool IsMethodEnclosing(const MIRFunction &func, const MIRStructType &structType) const;
  void LoadRCFieldAttrWhiteList(const std::string &file);

  static RCSetter *rcSetter;
  const std::string rcFieldAttrWhiteListName;
  std::map<std::string, std::map<std::string, std::vector<FieldAttrKind>>> rcFieldAttrWhiteListMap;
  std::set<MIRFunction*> unownedLocalFuncs;
  std::map<MIRFunction*, std::set<GStrIdx>> unownedLocalVars;
  std::map<const StmtNode*, GStrIdx> iputStmtFieldMap;
};
}  // namespace bc
}  // namespace maple
#endif  // BC_INPUT_INCLUDE_RC_SETTER_H