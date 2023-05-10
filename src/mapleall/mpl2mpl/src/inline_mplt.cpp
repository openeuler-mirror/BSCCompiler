/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "inline_mplt.h"
#include <fstream>
#include "stmt_cost_analyzer.h"

namespace maple {
bool InlineMplt::CollectInlineInfo(uint32 inlineSize, uint32 level) {
  auto tempSet = mirModule.GetFunctionList();
  if (tempSet.empty()) {
    return false;
  }
  for (auto func : std::as_const(tempSet)) {
    if (func == nullptr || func->GetBody() == nullptr || func->IsStatic() ||
        func->GetAttr(FUNCATTR_noinline) || GetFunctionSize(*func) > inlineSize) {
      continue;
    }
    std::vector<MIRFunction*> tmpStaticFuncs;
    std::set<uint32_t> tmpInliningGlobals;
    bool forbidden = Forbidden(func->GetBody(), std::make_pair(inlineSize, level), tmpStaticFuncs, tmpInliningGlobals);
    if (forbidden) {
      continue;
    }
    (void)optimizedFuncs.emplace(func);
    optimizedFuncs.insert(tmpStaticFuncs.cbegin(), tmpStaticFuncs.cend());
    inliningGlobals.insert(tmpInliningGlobals.cbegin(), tmpInliningGlobals.cend());
  }
  if (optimizedFuncs.empty()) {
    return false;
  }
  CollectTypesForOptimizedFunctions();
  CollectTypesForInliningGlobals();
  return true;
}

bool InlineMplt::Forbidden(BaseNode *node, const std::pair<uint32, uint32> &inlineConditions,
                           std::vector<MIRFunction*> &staticFuncs, std::set<uint32_t> &globalSymbols) {
  if (node == nullptr) {
    return false;
  }
  uint32 inlineSize = inlineConditions.first;
  uint32 level = inlineConditions.second;
  bool ret = false;
  if (node->GetOpCode() == OP_block) {
    auto *block = static_cast<BlockNode*>(node);
    for (auto &stmt : block->GetStmtNodes()) {
      ret |= Forbidden(&stmt, inlineConditions, staticFuncs, globalSymbols);
    }
  } else if (node->GetOpCode() == OP_callassigned || node->GetOpCode() == OP_call ||
             node->GetOpCode() == OP_addroffunc) {
    PUIdx puiIdx = (node->GetOpCode() == OP_callassigned || node->GetOpCode() == OP_call) ?
        static_cast<CallNode*>(node)->GetPUIdx() : static_cast<AddroffuncNode*>(node)->GetPUIdx();
    MIRFunction *newCallee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puiIdx);
    CHECK_NULL_FATAL(newCallee->GetFuncSymbol());
    (void)globalSymbols.insert(newCallee->GetFuncSymbol()->GetStIndex());
    if (newCallee->IsStatic()) {
      if (level == 0 || GetFunctionSize(*newCallee) > inlineSize || newCallee->GetAttr(FUNCATTR_noinline)) {
        return true;
      }
      staticFuncs.push_back(newCallee);
      ret |= Forbidden(newCallee->GetBody(), std::make_pair(inlineSize, level - 1), staticFuncs, globalSymbols);
    }
  } else if (node->GetOpCode() == OP_iread) {
    IreadNode *opNode = static_cast<IreadNode*>(node);
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(opNode->GetTyIdx());
    CollectStructOrUnionTypes(type);
  } else if (node->GetOpCode() == OP_dassign || node->GetOpCode() == OP_dread || node->GetOpCode() == OP_addrof) {
    uint32 stIdx = 0;
    if (node->GetOpCode() == OP_dread) {
      stIdx = static_cast<DreadNode*>(node)->GetStIdx().Idx();
    } else if (node->GetOpCode() == OP_addrof) {
      stIdx = static_cast<AddrofNode*>(node)->GetStIdx().Idx();
    } else {
      stIdx = static_cast<DassignNode*>(node)->GetStIdx().Idx();
    }
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx, true);
    if (symbol != nullptr && symbol->IsVar() && symbol->IsGlobal()) {
      (void)globalSymbols.insert(stIdx);
    }
  }
  for (size_t i = 0; i < node->NumOpnds(); ++i) {
    ret |= Forbidden(node->Opnd(i), inlineConditions, staticFuncs, globalSymbols);
  }
  return ret;
}

// should consider nested conditions, such as struct nests struct, struct nests func pointer and the func's
// paramters type or return type is also struct type.
void InlineMplt::CollectStructOrUnionTypes(const MIRType *baseType) {
  if (baseType == nullptr) {
    return;
  }
  std::set<const MIRType*> tmpTypes;
  GetElememtTypesFromDerivedType(baseType, tmpTypes);
  for (const auto type : tmpTypes) {
    if ((type->IsMIRStructType() || type->IsMIRUnionType()) && (optimizedFuncsType.count(type->GetTypeIndex()) == 0)) {
      (void)optimizedFuncsType.insert(type->GetTypeIndex());
      const MIRStructType *structType = static_cast<const MIRStructType*>(type);
      for (size_t i = 0; i < structType->GetFieldsSize(); ++i) {
        MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(structType->GetFieldsElemt(i).second.first);
        CollectStructOrUnionTypes(fieldType);
      }
    }
  }
}

void InlineMplt::GetElememtTypesFromDerivedType(const MIRType *baseType, std::set<const MIRType*> &elementTypes) {
  if (baseType->GetKind() == kTypePointer) {
    GetElememtTypesFromDerivedType(static_cast<const MIRPtrType*>(baseType)->GetPointedType(), elementTypes);
  } else if (baseType->GetKind() == kTypeArray) {
    GetElememtTypesFromDerivedType(static_cast<const MIRArrayType*>(baseType)->GetElemType(), elementTypes);
  } else if (baseType->GetKind() == kTypeFunction) {
    const MIRFuncType *funcType = static_cast<const MIRFuncType*>(baseType);
    MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcType->GetRetTyIdx());
    GetElememtTypesFromDerivedType(retType, elementTypes);
    for (auto paramTyIdx : funcType->GetParamTypeList()) {
      MIRType *paramType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(paramTyIdx);
      GetElememtTypesFromDerivedType(paramType, elementTypes);
    }
  } else if (baseType != nullptr) {
    (void)elementTypes.insert(baseType);
  }
}

void InlineMplt::CollectTypesForOptimizedFunctions() {
  for (auto *func : optimizedFuncs) {
    CollectTypesForSingleFunction(*func);
  }
}

void InlineMplt::CollectTypesForInliningGlobals() {
  for (auto stIdx : inliningGlobals) {
    MIRSymbol *globalSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx);
    CHECK_NULL_FATAL(globalSymbol);
    if (globalSymbol->IsVar()) {
      CollectTypesForGlobalVar(*globalSymbol);
    } else if (globalSymbol->IsFunction()) {
      CollectTypesForSingleFunction(*(globalSymbol->GetFunction()));
    }
  }
}

void InlineMplt::CollectTypesForSingleFunction(const MIRFunction &func) {
  for (uint32 k = 1; k < func.GetSymTab()->GetSymbolTableSize(); ++k) {
    MIRSymbol *localSymbol = func.GetSymTab()->GetSymbolFromStIdx(k);
    CHECK_NULL_FATAL(localSymbol);
    MIRType *type = localSymbol->GetType();
    CollectStructOrUnionTypes(type);
  }
}

void InlineMplt::CollectTypesForGlobalVar(const MIRSymbol &globalSymbol) {
  MIRType *type = globalSymbol.GetType();
  CollectStructOrUnionTypes(type);
}

void InlineMplt::DumpInlineCandidateToFile(const std::string &fileNameStr) {
  std::ofstream file;
  // Change cout's buffer to file.
  std::streambuf *backup = LogInfo::MapleLogger().rdbuf();
  (void)LogInfo::MapleLogger().rdbuf(file.rdbuf());
  file.open(fileNameStr, std::ios::trunc);
  DumpOptimizedFunctionTypes();
  // dump global variables needed for inlining file
  for (auto symbolIdx : inliningGlobals) {
    MIRSymbol *s = GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolIdx);
    ASSERT(s != nullptr, "null ptr check");
    if (s->GetStorageClass() == kScFstatic) {
      if (s->IsNeedForwDecl()) {
        // const string, including initialization
        s->Dump(false, 0, false);
      }
    }
  }
  for (auto symbolIdx : inliningGlobals) {
    MIRSymbol *s = GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolIdx);
    ASSERT(s != nullptr, "null ptr check");
    MIRStorageClass sc = s->GetStorageClass();
    if (s->GetStorageClass() == kScFstatic) {
      if (!s->IsNeedForwDecl()) {
        // const string, including initialization
        s->Dump(false, 0, false);
      }
    } else if (s->GetSKind() == kStFunc) {
      s->GetFunction()->Dump(true);
    } else {
      // static fields as extern
      s->SetStorageClass(kScExtern);
      s->Dump(false, 0, true);
    }
    s->SetStorageClass(sc);
  }
  for (auto *func : optimizedFuncs) {
    func->SetWithLocInfo(false);
    func->Dump();
  }
  // Restore cout's buffer.
  (void)LogInfo::MapleLogger().rdbuf(backup);
  file.close();
}

void InlineMplt::DumpOptimizedFunctionTypes() {
  for (auto it = optimizedFuncsType.begin(); it != optimizedFuncsType.end(); ++it) {
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(*it);
    ASSERT(type != nullptr, "type should not be nullptr here");
    std::string name = type->GetName();
    bool isStructType = type->IsStructType();
    if (isStructType) {
      auto *structType = static_cast<MIRStructType*>(type);
      if (structType->IsImported()) {
        continue;
      }
    }
    LogInfo::MapleLogger() << "type $" << name << " ";
    if (type->GetKind() == kTypeByName) {
      LogInfo::MapleLogger() << "void";
    } else {
      type->Dump(1, true);
    }
    LogInfo::MapleLogger() << '\n';
  }
}

uint32 InlineMplt::GetFunctionSize(MIRFunction &mirFunc) const {
  auto *tempMemPool = memPoolCtrler.NewMemPool("temp mempool", false);
  StmtCostAnalyzer sca(tempMemPool, &mirFunc);
  uint32 funcSize = static_cast<uint32>(sca.GetStmtsCost(mirFunc.GetBody())) / static_cast<uint32>(kSizeScale);
  delete tempMemPool;
  tempMemPool = nullptr;
  return funcSize;
}
} // namespace maple
