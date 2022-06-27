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
#include "rc_setter.h"
#include <fstream>
#include "ark_annotation_map.h"
#include "ark_annotation_processor.h"
#include "fe_manager.h"
#include "bc_util.h"

namespace maple {
namespace bc {
RCSetter *RCSetter::rcSetter = nullptr;

void RCSetter::ProcessClassRCAnnotation(const GStrIdx &classIdx, const std::vector<MIRPragma*> &pragmas) {
  if (pragmas.empty()) {
    return;
  }
  MIRStructType *type = FEManager::GetTypeManager().GetStructTypeFromName(classIdx);
  for (auto pragma : pragmas) {
    if (pragma->GetKind() != kPragmaClass || pragma->GetStrIdx() != type->GetNameStrIdx()) {
      continue;
    }
    if (!ArkAnnotation::GetInstance().IsRCUnownedOuter(pragma->GetTyIdx()) &&
        !ArkAnnotation::GetInstance().IsRCUnownedThis(pragma->GetTyIdx())) {
      continue;
    }
    const std::string &className = type->GetName();
    MIRSymbol *jcSymbol = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(className, *type);
    if (jcSymbol != nullptr) {
      jcSymbol->SetAttr(ATTR_rcunownedthis);
    }
    for (auto &field : type->GetFields()) {
      std::string fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(field.first);
      const std::string prefix = "this_24";
      if (fieldName.compare(0, prefix.size(), prefix) == 0) {
        field.second.second.SetAttr(FLDATTR_rcunowned);
      }
    }
  }
}

void RCSetter::ProcessMethodRCAnnotation(MIRFunction &mirFunc, const std::string &className,
                                         MIRStructType &structType, const MIRPragma &pragma) {
  if (ArkAnnotation::GetInstance().IsRCUnownedOuter(pragma.GetTyIdx()) ||
      ArkAnnotation::GetInstance().IsRCUnownedThis(pragma.GetTyIdx())) {
    // set ATTR_rcunowned to the field this$n of local class
    // the current function belongs to
    MIRSymbol *jcSymbol = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(className, structType);
    if (jcSymbol != nullptr) {
      jcSymbol->SetAttr(ATTR_rcunownedthis);
    }
    for (auto &field : structType.GetFields()) {
      const std::string &fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(field.first);
      const std::string prefix = "this_24";
      if (fieldName.compare(0, prefix.size(), prefix) == 0) {
        field.second.second.SetAttr(FLDATTR_rcunowned);
      }
    }
  } else if (ArkAnnotation::GetInstance().IsRCUnownedCap(pragma.GetTyIdx()) ||
             ArkAnnotation::GetInstance().IsRCUnownedCapList(pragma.GetTyIdx())) {
    // handle old RCUnownedCapRef annotation.
    MIRPragmaElement *elem = pragma.GetElementVector()[0];
    if (ArkAnnotation::GetInstance().IsRCUnownedCap(pragma.GetTyIdx())) {
      GStrIdx strIdx(elem->GetI32Val());
      CollectUnownedLocalVars(&mirFunc, strIdx);
    } else if (ArkAnnotation::GetInstance().IsRCUnownedCapList(pragma.GetTyIdx())) {
      for (auto eit : elem->GetSubElemVec()) {
        if (eit->GetSubElemVec().empty()) {
          continue;
        }
        MIRPragmaElement *e = eit->GetSubElemVec()[0];
        GStrIdx strIdx(e->GetI32Val());
        CollectUnownedLocalVars(&mirFunc, strIdx);
      }
    }
  }
}

void RCSetter::ProcessFieldRCAnnotation(const StructElemNameIdx &fieldElemNameIdx,
    const MIRType &fieldType, const std::vector<MIRPragma*> &pragmas) {
  if (pragmas.empty()) {
    return;
  }
  for (auto pragma : pragmas) {
    if (pragma->GetKind() != kPragmaVar ||
        pragma->GetStrIdx() != fieldElemNameIdx.elem ||
        pragma->GetTyIdxEx() != fieldType.GetTypeIndex()) {
      continue;
    }
    if (!ArkAnnotation::GetInstance().IsRCWeak(pragma->GetTyIdx()) &&
        !ArkAnnotation::GetInstance().IsRCUnowned(pragma->GetTyIdx())) {
      continue;
    }
    FieldAttrKind attr;
    if (ArkAnnotation::GetInstance().IsRCWeak(pragma->GetTyIdx())) {
      attr = FLDATTR_rcweak;
    } else if (ArkAnnotation::GetInstance().IsRCUnowned(pragma->GetTyIdx())) {
      attr = FLDATTR_rcunowned;
    }
    MIRStructType *structType = FEManager::GetTypeManager().GetStructTypeFromName(fieldElemNameIdx.klass);
    for (auto &field : structType->GetFields()) {
      if (field.first == fieldElemNameIdx.elem) {
        field.second.second.SetAttr(attr);
        break;
      }
    }
  }
}

void RCSetter::GetUnownedVarInLocalVars(const BCClassMethod &method, MIRFunction &mirFunction) {
  if (method.GetSrcLocalInfoPtr() == nullptr) {
    return;
  }
  std::set<uint16> unownedRegNums;
  // map<regNum, set<tuple<name, typeName, signature>>>
  for (auto &local : *method.GetSrcLocalInfoPtr()) {
    for (auto &item : local.second) {
      bool isUnowned = BCUtil::HasContainSuffix(std::get<0>(item), "$unowned");
      if (isUnowned) {
        GStrIdx strIdx = FEManager::GetMIRBuilder().GetOrCreateStringIndex(namemangler::EncodeName(std::get<0>(item)));
        CollectUnownedLocalVars(&mirFunction, strIdx);
        (void)unownedRegNums.insert(local.first);
      }
    }
  }
  // set ATTR_rcunowned for symbols according their reg num.
  SetAttrRCunowned(mirFunction, unownedRegNums);
}

void RCSetter::SetAttrRCunowned(MIRFunction &mirFunction, const std::set<uint16> &unownedRegNums) const {
  if (unownedRegNums.empty()) {
    return;
  }
  MIRSymbolTable *symTab = mirFunction.GetSymTab();
  if (symTab == nullptr) {
    return;
  }
  for (uint32 i = 0; i < mirFunction.GetSymTab()->GetSymbolTableSize(); ++i) {
    MIRSymbol *symbol = mirFunction.GetSymTab()->GetSymbolFromStIdx(i);
    if (symbol == nullptr) {
      continue;
    }
    MIRType *ty = symbol->GetType();
    if (ty->GetPrimType() == PTY_ref || ty->GetPrimType() == PTY_ptr) {
      uint32 regNum = BCUtil::Name2RegNum(symbol->GetName());
      if (unownedRegNums.find(regNum) != unownedRegNums.end()) {
        symbol->SetAttr(ATTR_rcunowned);
      }
    }
  }
}

void RCSetter::MarkRCUnownedForUnownedLocalFunctions() const {
  // mark all local variables unowned for @UnownedLocal functions.
  for (auto func : unownedLocalFuncs) {
    for (uint32 idx = 0; idx < func->GetSymTab()->GetSymbolTableSize(); idx++) {
      MIRSymbol *sym = func->GetSymTab()->GetSymbolFromStIdx(idx);
      if (sym == nullptr) {
        continue;
      }
      MIRType *ty = sym->GetType();
      if (ty->GetPrimType() == PTY_ref || ty->GetPrimType() == PTY_ptr) {
        sym->SetAttr(ATTR_rcunowned);
      }
    }
  }
}

bool RCSetter::IsAnonymousInner(const MIRStructType &structType) const {
  if (!(structType.GetKind() == kTypeClass || structType.GetKind() == kTypeInterface)) {
    return false;
  }
  // inner class has annotation Ldalvik/annotation/InnerClass;
  bool isCreat = false;
  const std::string &className =
      ArkAnnotationMap::GetArkAnnotationMap().GetAnnotationTypeName("Ldalvik_2Fannotation_2FInnerClass_3B");
  MIRStructType *inner = FEManager::GetTypeManager().GetOrCreateClassOrInterfaceType(
      className, true, FETypeFlag::kSrcExtern, isCreat);
  GStrIdx strIdx = FEManager::GetMIRBuilder().GetOrCreateStringIndex("name");
  unsigned tyIdx = inner->GetTypeIndex();
  if (structType.GetKind() == kTypeClass) {
    const MIRClassType &classType = static_cast<const MIRClassType&>(structType);
    for (auto it : classType.GetPragmaVec()) {
      if (it->GetTyIdx() == tyIdx) {
        for (auto eit : it->GetElementVector()) {
          // inner class and has no name
          if (eit->GetNameStrIdx() == strIdx && eit->GetI32Val() == 0) {
            return true;
          }
        }
      }
    }
  } else if (structType.GetKind() == kTypeInterface) {
    const MIRInterfaceType &interfaceType = static_cast<const MIRInterfaceType&>(structType);
    for (auto it : interfaceType.GetPragmaVec()) {
      if (it->GetTyIdx() == tyIdx) {
        for (auto eit : it->GetElementVector()) {
          // inner class and has no name
          if (eit->GetNameStrIdx() == strIdx && eit->GetI32Val() == 0) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool RCSetter::IsMethodEnclosing(const MIRFunction &func, const MIRStructType &structType) const {
  if (!(structType.GetKind() == kTypeClass || structType.GetKind() == kTypeInterface)) {
    return false;
  }
  bool isCreat = false;
  const std::string &className = namemangler::GetInternalNameLiteral(
      ArkAnnotationMap::GetArkAnnotationMap().GetAnnotationTypeName("Ldalvik_2Fannotation_2FEnclosingMethod_3B"));
  MIRStructType *inner = FEManager::GetTypeManager().GetOrCreateClassOrInterfaceType(
      className, true, FETypeFlag::kSrcExtern, isCreat);
  unsigned tyIdx = inner->GetTypeIndex();
  if (structType.GetKind() == kTypeClass) {
    const MIRClassType &classType = static_cast<const MIRClassType&>(structType);
    for (auto it : classType.GetPragmaVec()) {
      if (it->GetTyIdx() == tyIdx && !(it->GetElementVector().empty()) &&
          func.GetNameStrIdx() == it->GetElementVector()[0]->GetI32Val()) {
        return true;
      }
    }
  } else if (structType.GetKind() == kTypeInterface) {
    const MIRInterfaceType &interfaceType = static_cast<const MIRInterfaceType&>(structType);
    for (auto it : interfaceType.GetPragmaVec()) {
      if (it->GetTyIdx() == tyIdx && (!it->GetElementVector().empty()) &&
          func.GetNameStrIdx() == it->GetElementVector()[0]->GetI32Val()) {
        return true;
      }
    }
  }
  return false;
}

void RCSetter::MarkRCUnownedForAnonymousClasses() const {
  // mark captured unowned fields for anonymous class.
  for (auto mit : unownedLocalVars) {
    MIRFunction *func = mit.first;
    // check for anonymous class
    // mark captured var name aaa -> val$aaa (or val_24aaa) field in anonymous class
    for (auto sit : FEManager::GetTypeManager().GetStructNameTypeMap()) {
      ASSERT_NOT_NULL(sit.second.first);
      if (IsAnonymousInner(*(sit.second.first)) && IsMethodEnclosing(*func, *(sit.second.first))) {
        for (auto &fit : sit.second.first->GetFields()) {
          const std::string &fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(fit.first);
          constexpr size_t prefixLength = sizeof("val_24") - 1;
          if (fieldName.compare(0, prefixLength, "val_24") == 0) {
            std::string nameWithSuffix = fieldName + "_24unowned";
            for (auto nit : mit.second) {
              GStrIdx strIdx(nit);
              const std::string &varName = GlobalTables::GetStrTable().GetStringFromStrIdx(strIdx);
              if (nameWithSuffix.compare(prefixLength, varName.length(), varName) == 0 ||
                  fieldName.compare(prefixLength, varName.length(), varName) == 0) {
                fit.second.second.SetAttr(FLDATTR_rcunowned);
              }
            }
          }
        }
      }
    }
  }
}

void RCSetter::SetRCUnownedAttributeInternalSetFieldAttrRCUnowned(size_t opnd, const MIRFunction &calledFunc) const {
  const MIRSymbol *arg = calledFunc.GetFormal(opnd);
  for (const StmtNode *s = calledFunc.GetBody()->GetFirst(); s != nullptr; s = s->GetNext()) {
    // 1. checking iassign statements that assign parameter arg
    if (s->GetOpCode() != OP_iassign) {
      continue;
    }
    const IassignNode *ian = static_cast<const IassignNode*>(s);
    const BaseNode *val = ian->GetRHS();
    if (val->GetOpCode() != OP_dread) {
      continue;
    }
    const DreadNode *valDrn = static_cast<const DreadNode*>(val);
    const MIRSymbol *valSymbol = calledFunc.GetLocalOrGlobalSymbol(valDrn->GetStIdx());
    const GStrIdx &valStrIdx = valSymbol->GetNameStrIdx();
    if (valStrIdx != arg->GetNameStrIdx()) {
      continue;
    }
    // 2. get the class fields of the iassign statement
    auto it = iputStmtFieldMap.find(s);
    if (it != iputStmtFieldMap.end()) {
      const GStrIdx &fieldStrIdx = it->second;
      // 3. set rcunwoned attribute for that field
      const MIRType *type = calledFunc.GetClassType();
      CHECK_NULL_FATAL(type);
      MIRType *typePtr = GlobalTables::GetTypeTable().GetTypeFromTyIdx(type->GetTypeIndex());
      MIRStructType *structType = static_cast<MIRStructType*>(typePtr);
      for (auto &fit : structType->GetFields()) {
        if (fit.first == fieldStrIdx) {
          fit.second.second.SetAttr(FLDATTR_rcunowned);
          break;
        }
      }
    }
  }
}

void RCSetter::SetRCUnownedAttribute(const CallNode &callNode, MIRFunction &func,
                                     const MIRFunction &calledFunc, const std::set<GStrIdx> &gStrIdx) const {
  for (size_t i = 0; i < callNode.NumOpnds(); ++i) {
    BaseNode *bn = callNode.GetNopndAt(i);
    if (bn->GetOpCode() != OP_dread) {
      continue;
    }
    DreadNode *drn = static_cast<DreadNode*>(bn);
    const MIRSymbol *symbol = func.GetLocalOrGlobalSymbol(drn->GetStIdx());
    const GStrIdx &strIdx = symbol->GetNameStrIdx();
    // checking maple name in ALIAS
    for (auto als : func.GetAliasVarMap()) {
      if (als.second.mplStrIdx != strIdx) {
        continue;
      }
      for (auto sit : gStrIdx) {
        if (sit != als.first) {
          continue;
        }
        // now we have a link and a field need to be set rcunowned
        //
        // the focus is the i_th parameter arg.
        // we will go through body of <init> of the lambda class
        // 1. checking iassign statements that assign parameter arg
        // 2. get the class fields of the iassign statement
        // 3. set rcunwoned attribute for that field
        SetRCUnownedAttributeInternalSetFieldAttrRCUnowned(i, calledFunc);
      }
    }
  }
}

void RCSetter::MarkRCUnownedForLambdaClasses() const {
  // mark captured unowned fields for lambda classes.
  for (auto mit : unownedLocalVars) {
    MIRFunction *func = mit.first;
    // scan function body to find lambda functions used
    // and set rcunowned attribute on corresponding captured fields
    // of local classes for lambda functions
    for (const StmtNode *stmt = func->GetBody()->GetFirst(); stmt != nullptr; stmt = stmt->GetNext()) {
      if (stmt->GetOpCode() != OP_callassigned) {
        continue;
      }
      const CallNode *call = static_cast<const CallNode*>(stmt);
      MIRFunction *calledFunc = GlobalTables::GetFunctionTable().GetFuncTable().at(call->GetPUIdx());
      // only care calling <init> of local classes for lambda functions
      if (!(calledFunc->GetAttr(FUNCATTR_constructor) && calledFunc->GetAttr(FUNCATTR_public) &&
            calledFunc->GetAttr(FUNCATTR_synthetic))) {
        continue;
      }
      // go through call arguments to find maple names with ALIAS info
      // and their src names matching captured var names
      SetRCUnownedAttribute(*call, *func, *calledFunc, mit.second);
    }
  }
}

void RCSetter::MarkRCAttributes() const {
  MarkRCUnownedForUnownedLocalFunctions();
  MarkRCUnownedForAnonymousClasses();
  MarkRCUnownedForLambdaClasses();
}

void RCSetter::CollectUnownedLocalFuncs(MIRFunction *func) {
  (void)unownedLocalFuncs.insert(func);
}

void RCSetter::CollectUnownedLocalVars(MIRFunction *func, const GStrIdx &strIdx) {
  (void)unownedLocalVars[func].insert(strIdx);
}

void RCSetter::CollectInputStmtField(StmtNode *stmt, const GStrIdx &fieldName) {
  (void)iputStmtFieldMap.emplace(stmt, fieldName);
}

void RCSetter::LoadRCFieldAttrWhiteList(const std::string &file) {
  std::string line;
  std::ifstream infile(file);
  uint32 lineNum = 0;
  std::vector<std::string> vecItem;
  std::vector<FieldAttrKind> vecAttr;
  std::string item;
  std::string className;
  std::string fieldName;
  CHECK_FATAL(infile.is_open(), "(ToIDEUser)RCFieldAttrWhiteList file %s open failed", file.c_str());
  while (std::getline(infile, line)) {
    lineNum++;
    if (line.at(0) == '#') {
      continue;
    }
    std::stringstream ss;
    ss.str(line);
    vecItem.clear();
    while (std::getline(ss, item, ':')) {
      vecItem.push_back(item);
    }
    CHECK_FATAL(vecItem.size() > 2, "(ToIDEUser)invalid line %d in RCFieldAttrWhiteList file %s", lineNum,
                file.c_str());
    className = vecItem[0];
    fieldName = vecItem[1];
    vecAttr.clear();
    for (size_t i = 2; i < vecItem.size(); i++) {
      std::string &strAttr = vecItem[i];
      if (strAttr.compare("RCWeak") == 0) {
        vecAttr.push_back(FLDATTR_rcweak);
      } else if (strAttr.compare("RCUnowned") == 0) {
        vecAttr.push_back(FLDATTR_rcunowned);
      }
    }
    rcFieldAttrWhiteListMap[className][fieldName] = vecAttr;
  }
  infile.close();
}

void RCSetter::SetRCFieldAttrByWhiteList(FieldAttrs &attr, const std::string &className,
                                         const std::string &fieldName) {
  auto itClass = rcFieldAttrWhiteListMap.find(className);
  if (itClass != rcFieldAttrWhiteListMap.end()) {
    auto itField = itClass->second.find(fieldName);
    if (itField != itClass->second.end()) {
      for (auto &attrIt : itField->second) {
        attr.SetAttr(attrIt);
      }
    }
  }
}
}  // namespace bc
}  // namespace maple