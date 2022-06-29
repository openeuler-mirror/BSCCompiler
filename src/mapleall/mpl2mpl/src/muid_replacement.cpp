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
#include "muid_replacement.h"
#include <fstream>
#include "reflection_analysis.h"
#include "me_profile_gen.h"
#include "phase_impl.h"

namespace {
// Version for the mpl linker
constexpr char kMplLinkerVersionNumber[] = "MPL-LINKER V1.1";
constexpr char kMuidFuncPtrStr[] = "__muid_funcptr";
constexpr char kMuidSymPtrStr[] = "__muid_symptr";

#ifdef USE_ARM32_MACRO
enum LinkerRefFormat {
  kLinkerRefAddress   = 0, // must be 0
  kLinkerRefDef       = 1, // def
  kLinkerRefUnDef     = 2, // undef
  kLinkerRefOffset    = 3  // offset
};
#else
constexpr maple::uint64 kFromUndefIndexMask = 0x4000000000000000;
constexpr maple::uint64 kFromDefIndexMask = 0x2000000000000000;
#endif
constexpr maple::uint64 KReservedBits = 2u;
constexpr maple::uint32 kFromUndefIndexMask32Mod = 0x80000000;
constexpr maple::uint32 kFromDefIndexMask32Mod = 0x40000000;
} // namespace

// MUIDReplacement
// This phase is mainly to enable the maple linker about the text and data structure.
// It will do the following things:
// A) It collect the methods, classinfo, vtable, itable , and etc.And then it will generate the
// basic data structures like func_def, func_undef, data_def, data_undef using these symbols.
//
// B) It will replace the relevant reference about the methods and static variable with def or undef
// table.And then we can close these symbols to reduce the code size.
namespace maple {
MUID MUIDReplacement::mplMuid;

MUIDReplacement::MUIDReplacement(MIRModule &mod, KlassHierarchy *kh, bool dump)
    : FuncOptimizeImpl(mod, kh, dump) {
  isLibcore = (GetSymbolFromName(namemangler::GetInternalNameLiteral(namemangler::kJavaLangObjectStr)) != nullptr);
  GenerateTables();
}

MIRSymbol *MUIDReplacement::GetSymbolFromName(const std::string &name) {
  GStrIdx gStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(name);
  return GlobalTables::GetGsymTable().GetSymbolFromStrIdx(gStrIdx);
}

ConstvalNode* MUIDReplacement::GetConstvalNode(int64 index) {
#ifdef USE_ARM32_MACRO
  return builder->CreateIntConst(index, PTY_i32);
#else
  return builder->CreateIntConst(index, PTY_i64);
#endif
}

bool MUIDReplacement::CheckFunctionIsUsed(const MIRFunction &mirFunc) const {
  if (Options::decoupleStatic && mirFunc.GetAttr(FUNCATTR_static)) {
    return false;
  } else {
    return true;
  }
}

void MUIDReplacement::DumpMUIDFile(bool isFunc) {
  std::ofstream outFile;
  const std::string &mplName = GetMIRModule().GetFileName();
  size_t index = mplName.rfind(".");
  CHECK_FATAL(index != std::string::npos, "can not find src file");
  std::string prefix = mplName.substr(0, index);
  std::string outFileName;
  if (isFunc) {
    outFileName = prefix + ".func.muid";
  } else {
    outFileName = prefix + ".data.muid";
  }
  outFile.open(outFileName);
  if (outFile.fail()) {
    return;
  }
  size_t begin = mplName.find("libmaple");
  size_t end = mplName.find("_", begin);
  std::string outName;
  if (begin != std::string::npos && end != std::string::npos && end > begin) {
    outName = mplName.substr(begin, end - begin);
  } else {
    outName = mplName;
  }
  if (isFunc) {
    for (auto const &keyVal : funcDefMap) {
      outFile << outName << " ";
      MIRSymbol *mirFunc = keyVal.second.first;
      outFile << mirFunc->GetName() << " ";
      outFile << keyVal.first.ToStr() << "\n";
    }
  } else {
    for (auto const &keyVal : dataDefMap) {
      outFile << outName << " ";
      MIRSymbol *mirSymbol = keyVal.second.first;
      outFile << mirSymbol->GetName() << " ";
      outFile << keyVal.first.ToStr() << "\n";
    }
  }
  outFile.close();
}

void MUIDReplacement::InsertArrayClassSet(const MIRType &type) {
  auto jArrayType = static_cast<const MIRJarrayType&>(type);
  std::string klassJavaDescriptor;
  namemangler::DecodeMapleNameToJavaDescriptor(jArrayType.GetJavaName(), klassJavaDescriptor);
  arrayClassSet.insert(klassJavaDescriptor);
}

MIRType *MUIDReplacement::GetIntrinsicConstArrayClass(StmtNode &stmt) {
  Opcode op = stmt.GetOpCode();
  if (op == OP_dassign || op == OP_regassign) {
    auto &unode = static_cast<UnaryStmtNode&>(stmt);
    BaseNode &rhsOpnd = *(unode.GetRHS());
    Opcode rhsOp = rhsOpnd.GetOpCode();
    if (rhsOp == OP_intrinsicopwithtype) {
      auto &intrinNode = static_cast<IntrinsicopNode&>(rhsOpnd);
      MIRIntrinsicID intrinsicID = intrinNode.GetIntrinsic();
      if (intrinsicID == INTRN_JAVA_CONST_CLASS || intrinsicID == INTRN_JAVA_INSTANCE_OF) {
        TyIdx tyIdx = intrinNode.GetTyIdx();
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        MIRPtrType *ptrType = static_cast<MIRPtrType*>(type);
        MIRType *jArrayTy = ptrType->GetPointedType();
        if (jArrayTy->GetKind() == kTypeArray || jArrayTy->GetKind() == kTypeJArray) {
          return jArrayTy;
        }
      }
    } else if ((rhsOp == OP_gcmallocjarray) || (rhsOp == OP_gcpermallocjarray)) {
      JarrayMallocNode &jarrayMallocNode = static_cast<JarrayMallocNode&>(rhsOpnd);
      TyIdx tyIdx = jarrayMallocNode.GetTyIdx();
      MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
      if (type->GetKind() == kTypeArray || type->GetKind() == kTypeJArray) {
        return type;
      }
    }
  }
  return nullptr;
}

void MUIDReplacement::CollectArrayClass() {
  for (MIRFunction *mirFunc : GetMIRModule().GetFunctionList()) {
    if (mirFunc->GetBody() == nullptr) {
      continue;
    }
    StmtNode *stmt = mirFunc->GetBody()->GetFirst();
    StmtNode *nextStmt = nullptr;
    while (stmt) {
      nextStmt = stmt->GetNext();
      MIRType *jArrayTy = GetIntrinsicConstArrayClass(*stmt);
      if (jArrayTy != nullptr) {
        InsertArrayClassSet(*jArrayTy);
      }
      stmt = nextStmt;
    }
  }
}

void MUIDReplacement::GenArrayClassCache() {
  if (arrayClassSet.size() == 0) {
    return;
  }
#ifdef USE_32BIT_REF
  MIRType *mType = GlobalTables::GetTypeTable().GetUInt32();
#else
  MIRType *mType = GlobalTables::GetTypeTable().GetUInt64();
#endif
  auto arrayType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*mType, static_cast<uint32>(arrayClassSet.size()));
  auto *arrayClassNameConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), *arrayType);
  auto *arrayClassConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), *arrayType);
  ASSERT_NOT_NULL(arrayClassNameConst);
  ASSERT_NOT_NULL(arrayClassConst);
  // magic number, must consistent with kMplArrayClassCacheMagicNumber(0x1a3) in runtime, defined in mrt_common.h.
  constexpr int32 arrayClassCacheMagicNumber = 0x1a3;
  for (auto arrayClassName : arrayClassSet) {
    uint32 typeNameIdx = ReflectionAnalysis::FindOrInsertRepeatString(arrayClassName);
    MIRIntConst *nameConstValue =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(typeNameIdx, *mType);
    arrayClassNameConst->AddItem(nameConstValue, 0);
    MIRIntConst *constValue =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(arrayClassCacheMagicNumber, *mType);
    arrayClassConst->AddItem(constValue, 0);
  }
  MIRSymbol *arrayClassNameSt = GetMIRModule().GetMIRBuilder()->CreateGlobalDecl(
      namemangler::kArrayClassCacheNameTable + GetMIRModule().GetFileNameAsPostfix(), *arrayType);
  arrayClassNameSt->SetStorageClass(kScFstatic);
  arrayClassNameSt->SetKonst(arrayClassNameConst);

  MIRSymbol *arrayClassCacheSt = GetMIRModule().GetMIRBuilder()->CreateGlobalDecl(
      namemangler::kArrayClassCacheTable + GetMIRModule().GetFileNameAsPostfix(), *arrayType);
  arrayClassCacheSt->SetStorageClass(kScFstatic);
  arrayClassCacheSt->SetKonst(arrayClassConst);
}

void MUIDReplacement::CollectFuncAndDataFromKlasses() {
  // Iterate klasses
  for (Klass *klass : klassHierarchy->GetTopoSortedKlasses()) {
    MIRStructType *sType = klass->GetMIRStructType();
    // DefTable and UndefTable are placed where a class is defined
    if (sType == nullptr || !sType->IsLocal()) {
      continue;
    }
    // Collect FuncDefSet
    for (MethodPair &methodPair : sType->GetMethods()) {
      MIRSymbol *funcSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(methodPair.first.Idx());
      MIRFunction *mirFunc = funcSymbol->GetFunction();
      if (mirFunc != nullptr && mirFunc->GetBody()) {
        AddDefFunc(mirFunc);
      }
    }
    // Cases where an external method can be referred:
    // 1. vtable entry (what we are dealing with here)
    // 2. direct call (collected later when iterating function bodies)
    if (!klass->IsInterface()) {
      for (MethodPair *vMethodPair : sType->GetVTableMethods()) {
        if (vMethodPair != nullptr) {
          MIRSymbol *funcSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(vMethodPair->first.Idx());
          MIRFunction *mirFunc = funcSymbol->GetFunction();
          if (mirFunc != nullptr && mirFunc->GetBody() == nullptr && !mirFunc->IsAbstract()) {
            if (!CheckFunctionIsUsed(*mirFunc)) {
              continue;
            }
            AddUndefFunc(mirFunc);
          }
        }
      }
    }
  }
}

void MUIDReplacement::CollectFuncAndDataFromGlobalTab() {
  // Iterate global symbols
  for (size_t i = 1; i < GlobalTables::GetGsymTable().GetSymbolTableSize(); ++i) {
    // entry 0 is reserved as nullptr
    MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(i);
    CHECK_FATAL(mirSymbol != nullptr, "Invalid global data symbol at index %u", i);
    if (mirSymbol->GetStorageClass() == kScGlobal) {
      if (mirSymbol->IsReflectionClassInfo()) {
        if (!mirSymbol->IsForcedGlobalClassinfo() &&
            preloadedClassInfo.find(mirSymbol->GetName()) == preloadedClassInfo.end()) {
          // With maple linker, global data can be declared as local
          mirSymbol->SetStorageClass(kScFstatic);
        }
        if (mirSymbol->GetKonst() != nullptr) {
          // Use this to exclude forward-declared classinfo symbol
          AddDefData(mirSymbol);
        }
      } else if (mirSymbol->IsStatic()) {
        mirSymbol->SetStorageClass(kScFstatic);
        AddDefData(mirSymbol);
      }
    } else if (mirSymbol->GetStorageClass() == kScExtern &&
               (mirSymbol->IsReflectionClassInfo() || mirSymbol->IsStatic())) {
      AddUndefData(mirSymbol);
    }
  }
}

void MUIDReplacement::CollectSuperClassArraySymbolData() {
  // Iterate global symbols
  for (size_t i = 0; i < GlobalTables::GetGsymTable().GetSymbolTableSize(); ++i) {
    MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(i);
    if ((mirSymbol != nullptr) && mirSymbol->IsReflectionSuperclassInfo()) {
      (void)superClassArraySymbolSet.insert(mirSymbol);
    }
  }
}

void MUIDReplacement::CollectFuncAndDataFromFuncList() {
  // Iterate function bodies
  for (MIRFunction *mirFunc : GetMIRModule().GetFunctionList()) {
    if (mirFunc->GetBody() == nullptr) {
      continue;
    }
    StmtNode *stmt = mirFunc->GetBody()->GetFirst();
    while (stmt != nullptr) {
      PUIdx puidx = 0;
      switch (stmt->GetOpCode()) {
        case OP_call:
        case OP_callassigned: {
          puidx = static_cast<CallNode*>(stmt)->GetPUIdx();
          break;
        }
        case OP_dassign: {
          // epre in ME may have splitted a direct call into addroffunc and an indirect call
          auto *rhs = static_cast<DassignNode*>(stmt)->GetRHS();
          if (rhs != nullptr && rhs->GetOpCode() == OP_addroffunc) {
            puidx = static_cast<AddroffuncNode*>(rhs)->GetPUIdx();
          }
          break;
        }
        case OP_regassign: {
          auto *rhs = static_cast<RegassignNode*>(stmt)->Opnd(0);
          if (rhs != nullptr && rhs->GetOpCode() == OP_addroffunc) {
            puidx = static_cast<AddroffuncNode*>(rhs)->GetPUIdx();
          }
          break;
        }
        default:
          break;
      }
      if (puidx != 0) {
        MIRFunction *undefMIRFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puidx);
        if (undefMIRFunc->GetBody() == nullptr &&
            (undefMIRFunc->IsJava() || !undefMIRFunc->GetBaseClassName().empty())) {
          if (CheckFunctionIsUsed(*undefMIRFunc)) {
            AddUndefFunc(undefMIRFunc);
          }
        }
      }
      // Some stmt requires classinfo but is lowered in CG. Handle them here.
      CollectImplicitUndefClassInfo(*stmt);
      stmt = stmt->GetNext();
    }
  }
}

void MUIDReplacement::CollectFuncAndData() {
  // Iterate klasses
  for (Klass *klass : klassHierarchy->GetTopoSortedKlasses()) {
    MIRStructType *sType = klass->GetMIRStructType();
    // DefTable and UndefTable are placed where a class is defined
    if (sType == nullptr || !sType->IsLocal()) {
      continue;
    }
    // Collect FuncDefSet
    for (MethodPair &methodPair : sType->GetMethods()) {
      MIRSymbol *funcSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(methodPair.first.Idx());
      MIRFunction *mirFunc = funcSymbol->GetFunction();
      CHECK_FATAL(mirFunc != nullptr, "Invalid function symbol for Class %s", sType->GetName().c_str());
      if (mirFunc != nullptr && mirFunc->GetBody()) {
        AddDefFunc(mirFunc);
      }
    }
    // Cases where an external method can be referred:
    // 1. vtable entry (what we are dealing with here)
    // 2. direct call (collected later when iterating function bodies)
    if (!klass->IsInterface()) {
      for (MethodPair *vMethodPair : sType->GetVTableMethods()) {
        CHECK_FATAL(vMethodPair != nullptr, "Invalid vtable_methods entry for Class %s", sType->GetName().c_str());

        MIRSymbol *funcSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(vMethodPair->first.Idx());
        MIRFunction *mirFunc = funcSymbol->GetFunction();
        if (mirFunc != nullptr && !mirFunc->GetBody() && !mirFunc->IsAbstract()) {
          if (CheckFunctionIsUsed(*mirFunc)) {
            AddUndefFunc(mirFunc);
          }
        }
      }
    }
  }
  // Iterate global symbols
  for (size_t i = 1; i < GlobalTables::GetGsymTable().GetSymbolTableSize(); ++i) {
    // entry 0 is reserved as nullptr
    MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(i);
    CHECK_FATAL(mirSymbol != nullptr, "Invalid global data symbol at index %u", i);
    if (mirSymbol->GetStorageClass() == kScGlobal) {
      if (mirSymbol->IsReflectionClassInfo()) {
        if (!mirSymbol->IsForcedGlobalClassinfo() &&
            preloadedClassInfo.find(mirSymbol->GetName()) == preloadedClassInfo.end()) {
          // With maple linker, global data can be declared as local
          mirSymbol->SetStorageClass(kScFstatic);
        }
        if (mirSymbol->GetKonst() != nullptr) {
          // Use this to exclude forward-declared classinfo symbol
          AddDefData(mirSymbol);
        }
      } else if (mirSymbol->IsStatic()) {
        mirSymbol->SetStorageClass(kScFstatic);
        AddDefData(mirSymbol);
      }
    } else if (mirSymbol->GetStorageClass() == kScExtern &&
               (mirSymbol->IsReflectionClassInfo() || mirSymbol->IsStatic())) {
      AddUndefData(mirSymbol);
    }
  }
  // Iterate function bodies
  for (MIRFunction *mirFunc : GetMIRModule().GetFunctionList()) {
    if (mirFunc->GetBody() == nullptr) {
      continue;
    }
    StmtNode *stmt = mirFunc->GetBody()->GetFirst();
    while (stmt != nullptr) {
      PUIdx puidx = 0;
      switch (stmt->GetOpCode()) {
        case OP_call:
        case OP_callassigned: {
          puidx = static_cast<CallNode*>(stmt)->GetPUIdx();
          break;
        }
        case OP_dassign: {
          // epre in ME may have splitted a direct call into addroffunc and an indirect call
          BaseNode *rhs = static_cast<DassignNode*>(stmt)->GetRHS();
          if (rhs != nullptr && rhs->GetOpCode() == OP_addroffunc) {
            puidx = static_cast<AddroffuncNode*>(rhs)->GetPUIdx();
          }
          break;
        }
        case OP_regassign: {
          BaseNode *rhs = static_cast<RegassignNode*>(stmt)->Opnd(0);
          if (rhs != nullptr && rhs->GetOpCode() == OP_addroffunc) {
            puidx = static_cast<AddroffuncNode*>(rhs)->GetPUIdx();
          }
          break;
        }
        case OP_catch: {
          auto &catchNode = static_cast<CatchNode&>(*stmt);
          if (catchNode.GetExceptionTyIdxVec().size() != 1) {
            break;
          }
          MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(catchNode.GetExceptionTyIdxVecElement(0));
          ASSERT(type->GetKind() == kTypePointer, "Must be pointer");
          auto *pType = static_cast<MIRPtrType*>(type);
          Klass *catchClass = nullptr;
          if (pType->GetPointedTyIdx() == PTY_void) {
            catchClass = klassHierarchy->GetKlassFromName(namemangler::kJavaLangExceptionStr);
          } else {
            catchClass = klassHierarchy->GetKlassFromTyIdx(pType->GetPointedTyIdx());
          }
          if (catchClass != nullptr) {
            std::string classInfoName = CLASSINFO_PREFIX_STR + catchClass->GetKlassName();
            MIRSymbol *classSym = GetSymbolFromName(classInfoName);
            if (classSym == nullptr) {
              classSym = builder->CreateGlobalDecl(classInfoName, *GlobalTables::GetTypeTable().GetPtr());
              classSym->SetStorageClass(kScExtern);
              AddUndefData(classSym);
            }
          }
          break;
        }
        default:
          break;
      }
      if (puidx != 0) {
        MIRFunction *undefMIRFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puidx);
        if (!undefMIRFunc->GetBody() && (undefMIRFunc->IsJava() || !undefMIRFunc->GetBaseClassName().empty())) {
          if (CheckFunctionIsUsed(*undefMIRFunc)) {
            AddUndefFunc(undefMIRFunc);
          }
        }
      }
      // Some stmt requires classinfo but is lowered in CG. Handle them here.
      CollectImplicitUndefClassInfo(*stmt);
      stmt = stmt->GetNext();
    }
  }
}

void MUIDReplacement::CollectImplicitUndefClassInfo(StmtNode &stmt) {
  BaseNode *rhs = nullptr;
  std::vector<MIRStructType*> classTyVec;
  if (stmt.GetOpCode() == OP_dassign) {
    auto *dNode = static_cast<DassignNode*>(&stmt);
    rhs = dNode->GetRHS();
  } else if (stmt.GetOpCode() == OP_regassign) {
    auto *rNode = static_cast<RegassignNode*>(&stmt);
    rhs = rNode->Opnd(0);
  } else if (stmt.GetOpCode() == OP_catch) {
    auto *jNode = static_cast<CatchNode*>(&stmt);
    for (TyIdx typeIdx : jNode->GetExceptionTyIdxVec()) {
      auto *pointerType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx));
      MIRType *type = pointerType->GetPointedType();
      if (type != nullptr) {
        if (type->GetKind() == kTypeClass || type->GetKind() == kTypeInterface) {
          classTyVec.push_back(static_cast<MIRStructType*>(type));
        } else if (type == GlobalTables::GetTypeTable().GetVoid()) {
          Klass *objectKlass = klassHierarchy->GetKlassFromLiteral(namemangler::kJavaLangObjectStr);
          if (objectKlass != nullptr) {
            classTyVec.push_back(objectKlass->GetMIRStructType());
          }
        }
      }
    }
  }
  if (rhs != nullptr && rhs->GetOpCode() == OP_gcmalloc) {
    // GCMalloc may require more classinfo than what we have seen so far
    auto *gcMalloc = static_cast<GCMallocNode*>(rhs);
    classTyVec.push_back(
        static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(gcMalloc->GetTyIdx())));
  } else if (rhs != nullptr && rhs->GetOpCode() == OP_intrinsicopwithtype) {
    auto *intrinNode = static_cast<IntrinsicopNode*>(rhs);
    if (intrinNode->GetIntrinsic() == INTRN_JAVA_CONST_CLASS || intrinNode->GetIntrinsic() == INTRN_JAVA_INSTANCE_OF) {
      auto *pointerType =
          static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(intrinNode->GetTyIdx()));
      MIRType *type = pointerType->GetPointedType();
      if (type != nullptr && (type->GetKind() == kTypeClass || type->GetKind() == kTypeInterface)) {
        classTyVec.push_back(static_cast<MIRStructType*>(type));
      }
    }
  }
  for (MIRStructType *classType : classTyVec) {
    if (classType == nullptr) {
      continue;
    }
    std::string classInfoName = CLASSINFO_PREFIX_STR + classType->GetName();
    MIRSymbol *classSym = GetSymbolFromName(classInfoName);
    if (classSym == nullptr) {
      classSym = builder->CreateGlobalDecl(classInfoName, *GlobalTables::GetTypeTable().GetPtr());
      classSym->SetStorageClass(kScExtern);
      AddUndefData(classSym);
    }
  }
}

void MUIDReplacement::InsertFunctionProfile(MIRFunction &currentFunc, int64 index) {
  // Insert profile code  __profile_func_tab[idx] = __profile_func_tab[idx] + 1
  SetCurrentFunction(currentFunc);
  AddrofNode *baseExpr = nullptr;
  MIRArrayType *arrayType = nullptr;
  baseExpr = builder->CreateExprAddrof(0, *funcProfileTabSym, GetMIRModule().GetMemPool());
  arrayType = static_cast<MIRArrayType*>(funcProfileTabSym->GetType());
  ConstvalNode *offsetExpr = GetConstvalNode(index);
  ConstvalNode *incExpr = GetConstvalNode(1);
  ArrayNode *arrayExpr = builder->CreateExprArray(*arrayType, baseExpr, offsetExpr);
  arrayExpr->SetBoundsCheck(false);
  MIRType *elemType = arrayType->GetElemType();
  BaseNode *ireadPtrExpr =
      builder->CreateExprIread(*GlobalTables::GetTypeTable().GetVoidPtr(),
                               *GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType), 0, arrayExpr);
  BaseNode *addExpr =
      builder->CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetUInt32(), ireadPtrExpr, incExpr);
  MIRType *destPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(TyIdx(PTY_u32));
  StmtNode *funcProfileIAssign = builder->CreateStmtIassign(*destPtrType, 0, arrayExpr, addExpr);
  currentFunc.GetBody()->InsertFirst(funcProfileIAssign);
}

void MUIDReplacement::GenerateFuncDefTable() {
  // Use funcDefMap to make sure funcDefTab is sorted by an increasing order of MUID
  for (MIRFunction *mirFunc : funcDefSet) {
    MUID muid = GetMUID(mirFunc->GetName());
    CHECK_FATAL(funcDefMap.find(muid) == funcDefMap.end(), "MUID has been used before, possible collision");
    // Use 0 as the index for now. It will be back-filled once we have the whole map.
    funcDefMap[muid] = SymIdxPair(mirFunc->GetFuncSymbol(), 0);
  }
  uint32 idx = 0;
  uint32 arraySize = static_cast<uint32>(funcDefMap.size());
  MIRArrayType &muidIdxArrayType =
      *GlobalTables::GetTypeTable().GetOrCreateArrayType(*GlobalTables::GetTypeTable().GetUInt32(), arraySize);
  MIRAggConst *muidIdxTabConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), muidIdxArrayType);
  for (auto &keyVal : funcDefMap) {
    // Fill in the real index
    keyVal.second.second = idx++;
    // Use the muid index for now. It will be back-filled once we have the whole vector.
    MIRIntConst *indexConst =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(keyVal.second.second,
                                                             *GlobalTables::GetTypeTable().GetUInt32());
    muidIdxTabConst->AddItem(indexConst, 0);
  }
  FieldVector parentFields;
  FieldVector fields;
  GlobalTables::GetTypeTable().PushIntoFieldVector(fields, "funcUnifiedAddr",
                                                   *GlobalTables::GetTypeTable().GetCompactPtr());
  auto *funcDefTabEntryType = static_cast<MIRStructType*>(
      GlobalTables::GetTypeTable().GetOrCreateStructType("MUIDFuncDefTabEntry", fields, parentFields, GetMIRModule()));
  FieldVector funcinffields;
  GlobalTables::GetTypeTable().PushIntoFieldVector(funcinffields, "funcSize",
                                                   *GlobalTables::GetTypeTable().GetUInt32());
  GlobalTables::GetTypeTable().PushIntoFieldVector(funcinffields, "funcName",
                                                   *GlobalTables::GetTypeTable().GetUInt32());
  auto *funcInfTabEntryType = static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetOrCreateStructType(
      "MUIDFuncInfTabEntry", funcinffields, parentFields, GetMIRModule()));

  FieldVector funcProfinffields;
  GlobalTables::GetTypeTable().PushIntoFieldVector(funcProfinffields, "hash",
                                                   *GlobalTables::GetTypeTable().GetUInt64());
  GlobalTables::GetTypeTable().PushIntoFieldVector(funcProfinffields, "start",
                                                   *GlobalTables::GetTypeTable().GetUInt32());
  GlobalTables::GetTypeTable().PushIntoFieldVector(funcProfinffields, "end",
                                                   *GlobalTables::GetTypeTable().GetUInt32());
  auto *funcProfInfTabEntryType = static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetOrCreateStructType(
      "FuncProfInfTabEntry", funcProfinffields, parentFields, GetMIRModule()));
  FieldVector muidFields;
#ifdef USE_64BIT_MUID
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidLow", *GlobalTables::GetTypeTable().GetUInt32());
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidHigh", *GlobalTables::GetTypeTable().GetUInt32());
#else
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidLow", *GlobalTables::GetTypeTable().GetUInt64());
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidHigh", *GlobalTables::GetTypeTable().GetUInt64());
#endif  // USE_64BIT_MUID
  auto *funcDefMuidTabEntryType =
      static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetOrCreateStructType(
          "MUIDFuncDefMuidTabEntry", muidFields, parentFields, GetMIRModule()));
  MIRArrayType &arrayType = *GlobalTables::GetTypeTable().GetOrCreateArrayType(*funcDefTabEntryType, arraySize);
  MIRAggConst *funcDefTabConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), arrayType);
  MIRArrayType &funcInfArrayType = *GlobalTables::GetTypeTable().GetOrCreateArrayType(*funcInfTabEntryType, arraySize);
  MIRAggConst *funcInfTabConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), funcInfArrayType);
  MIRArrayType &muidArrayType = *GlobalTables::GetTypeTable().GetOrCreateArrayType(*funcDefMuidTabEntryType, arraySize);
  MIRAggConst *funcDefMuidTabConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), muidArrayType);
  MIRArrayType &funcProfInfArrayType =
      *GlobalTables::GetTypeTable().GetOrCreateArrayType(*funcProfInfTabEntryType, arraySize);
  MIRAggConst *funcProfInfTabConst =
      GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), funcProfInfArrayType);
  if (Options::profileFunc) {
    // create a table each field is 4 byte record the call times
    MIRAggConst *funcCallTimesConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), muidIdxArrayType);
    for (uint32 start = 0; start < arraySize; ++start) {
      MIRIntConst *indexConst =
          GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *GlobalTables::GetTypeTable().GetUInt32());
      funcCallTimesConst->AddItem(indexConst, 0);
    }
    if (arraySize) {
      std::string funcProfileName = namemangler::kFunctionProfileTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
      funcProfileTabSym = builder->CreateGlobalDecl(funcProfileName, muidIdxArrayType);
      funcProfileTabSym->SetKonst(funcCallTimesConst);
      funcProfileTabSym->SetStorageClass(kScFstatic);
    }
  }
  // Create funcDefSet to store functions sorted by address
  std::vector<std::pair<MIRSymbol*, MUID>> funcDefArray;
  idx = 0;
  for (MIRFunction *mirFunc : GetMIRModule().GetFunctionList()) {
    ASSERT(mirFunc != nullptr, "null ptr check!");
    MUID muid = GetMUID(mirFunc->GetName());
    MapleMap<MUID, SymIdxPair>::iterator iter = funcDefMap.find(muid);
    if (mirFunc->GetBody() == nullptr || iter == funcDefMap.end()) {
      continue;
    }
    funcDefArray.push_back(std::make_pair(mirFunc->GetFuncSymbol(), muid));
    // Create muidIdxTab to store the index in funcDefTab and funcDefMuidTab
    // With muidIdxTab, we can use index sorted by muid to find the index in funcDefTab and funcDefMuidTab
    // Use the left 1 bit of muidIdx to mark whether the function is weak or not. 1 is for weak
    uint32 muidIdx = iter->second.second;
    constexpr uint32 weakFuncFlag = 0x80000000; // 0b10000000 00000000 00000000 00000000
    auto *indexConst = safe_cast<MIRIntConst>(muidIdxTabConst->GetConstVecItem(muidIdx));
    uint32 tempIdx = (indexConst->GetExtValue() & weakFuncFlag) | idx;
    indexConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(tempIdx,
                                                                      *GlobalTables::GetTypeTable().GetUInt32());
    muidIdxTabConst->SetItem(muidIdx, indexConst, 0);
    if (reflectionList.find(mirFunc->GetName()) != reflectionList.end()) {
      auto *tempConst = safe_cast<MIRIntConst>(muidIdxTabConst->GetConstVecItem(idx));
      tempIdx = weakFuncFlag | tempConst->GetExtValue();
      tempConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(tempIdx,
                                                                       *GlobalTables::GetTypeTable().GetUInt32());
      muidIdxTabConst->SetItem(idx, tempConst, 0);
    }
    if (Options::profileFunc) {
      InsertFunctionProfile(*mirFunc, idx);
    }
    if (Options::lazyBinding && !isLibcore) {
      ReplaceMethodMetaFuncAddr(*mirFunc->GetFuncSymbol(), idx);
    }
    if (Options::genIRProfile || Options::profileFunc) {
      auto funcProfInf = mirFunc->GetProfInf();
      MIRAggConst *funcProfInfEntryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(),
          *funcProfInfTabEntryType);
      uint32 funcProfInfFieldID = 1;

      builder->AddIntFieldConst(*funcProfInfTabEntryType,
          *funcProfInfEntryConst, funcProfInfFieldID++, static_cast<int64>(funcProfInf->funcHash));
      builder->AddIntFieldConst(*funcProfInfTabEntryType,
          *funcProfInfEntryConst, funcProfInfFieldID++, funcProfInf->counterStart);
      builder->AddIntFieldConst(*funcProfInfTabEntryType,
          *funcProfInfEntryConst, funcProfInfFieldID++, funcProfInf->counterEnd);
      funcProfInfTabConst->AddItem(funcProfInfEntryConst, 0);
    }
        // Store the real idx of funcdefTab, for ReplaceAddroffuncConst->FindIndexFromDefTable
    defMuidIdxMap[muid] = idx;
    idx++;
    if (trace) {
      LogInfo::MapleLogger() << "funcDefMap, MUID: " << muid.ToStr()
                             << ", Function Name: " << iter->second.first->GetName()
                             << ", Offset in addr order: " << (idx - 1)
                             << ", Offset in muid order: " << iter->second.second << "\n";
    }
  }
  if (Options::genIRProfile || Options::profileFunc) {
    MeProfGen::DumpSummary();
  }
  // Create funcDefTab, funcInfoTab and funcMuidTab sorted by address, funcMuidIdxTab sorted by muid
  for (auto keyVal : funcDefArray) {
    MIRSymbol *funcSymbol = keyVal.first;
    MUID muid = keyVal.second;
    MIRAggConst *entryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), *funcDefTabEntryType);
    uint32 fieldID = 1;
    MIRAggConst *funcInfEntryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(),
                                                                                   *funcInfTabEntryType);
    uint32 funcInfFieldID = 1;
    MIRAggConst *muidEntryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(),
                                                                                *funcDefMuidTabEntryType);
    uint32 muidFieldID = 1;
    // To be processed by runtime
    builder->AddAddroffuncFieldConst(*funcDefTabEntryType, *entryConst, fieldID++, *funcSymbol);
    funcDefTabConst->AddItem(entryConst, 0);
    // To be emitted as method size by CG
    builder->AddAddroffuncFieldConst(*funcInfTabEntryType, *funcInfEntryConst, funcInfFieldID++, *funcSymbol);
    // To be emitted as method name by CG
    builder->AddAddroffuncFieldConst(*funcInfTabEntryType, *funcInfEntryConst, funcInfFieldID++, *funcSymbol);
    funcInfTabConst->AddItem(funcInfEntryConst, 0);
    builder->AddIntFieldConst(*funcDefMuidTabEntryType, *muidEntryConst, muidFieldID++, muid.data.words[0]);
    builder->AddIntFieldConst(*funcDefMuidTabEntryType, *muidEntryConst, muidFieldID++, muid.data.words[1]);
    funcDefMuidTabConst->AddItem(muidEntryConst, 0);
    mplMuidStr += muid.ToStr();
  }
  if (!funcDefTabConst->GetConstVec().empty()) {
    std::string funcDefTabName = namemangler::kMuidFuncDefTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    funcDefTabSym = builder->CreateGlobalDecl(funcDefTabName, arrayType);
    funcDefTabSym->SetKonst(funcDefTabConst);
    funcDefTabSym->SetStorageClass(kScFstatic);
    // We add the original def tab for lazy binding, and emit them by CG.
    if (Options::lazyBinding) {
      std::string funcDefOrigTabName = namemangler::kMuidFuncDefOrigTabPrefixStr +
                                       GetMIRModule().GetFileNameAsPostfix();
      funcDefOrigTabSym = builder->CreateGlobalDecl(funcDefOrigTabName,
                                                    arrayType);
      funcDefOrigTabSym->SetKonst(funcDefTabConst);
      funcDefOrigTabSym->SetStorageClass(kScFstatic);
    }
  }
  if (!funcInfTabConst->GetConstVec().empty()) {
    std::string funcInfTabName = namemangler::kMuidFuncInfTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    funcInfTabSym = builder->CreateGlobalDecl(funcInfTabName, funcInfArrayType);
    funcInfTabSym->SetKonst(funcInfTabConst);
    funcInfTabSym->SetStorageClass(kScFstatic);
  }
  if (!funcDefMuidTabConst->GetConstVec().empty()) {
    std::string funcDefMuidTabName = namemangler::kMuidFuncDefMuidTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    funcDefMuidTabSym = builder->CreateGlobalDecl(funcDefMuidTabName, muidArrayType);
    funcDefMuidTabSym->SetKonst(funcDefMuidTabConst);
    funcDefMuidTabSym->SetStorageClass(kScFstatic);
  }
  if (!muidIdxTabConst->GetConstVec().empty()) {
    std::string muidIdxTabName = namemangler::kMuidFuncMuidIdxTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    funcMuidIdxTabSym = builder->CreateGlobalDecl(muidIdxTabName, muidIdxArrayType);
    funcMuidIdxTabSym->SetKonst(muidIdxTabConst);
    funcMuidIdxTabSym->SetStorageClass(kScFstatic);
  }

  if (!funcProfInfTabConst->GetConstVec().empty()) {
    std::string profInfTabName = namemangler::kFuncIRProfInfTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    funcProfInfTabSym = builder->CreateGlobalDecl(profInfTabName, funcProfInfArrayType);
    funcProfInfTabSym->SetKonst(funcProfInfTabConst);
    funcProfInfTabSym->SetStorageClass(kScFstatic);
  }
  if (Options::dumpMuidFile) {
    DumpMUIDFile(true);
  }
}

void MUIDReplacement::ReplaceMethodMetaFuncAddr(const MIRSymbol &funcSymbol, int64 index) {
  std::string symbolName = funcSymbol.GetName();
  MIRSymbol *methodAddrDataSt = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
      GlobalTables::GetStrTable().GetStrIdxFromName(namemangler::kMethodAddrDataPrefixStr + symbolName));
  CHECK_FATAL(methodAddrDataSt != nullptr, "methodAddrDataSt symbol is null.");
  MIRConst *mirConst = methodAddrDataSt->GetKonst();
  MIRAggConst *aggConst = safe_cast<MIRAggConst>(mirConst);
  MIRAggConst *agg = safe_cast<MIRAggConst>(aggConst->GetConstVecItem(0));
  MIRConst *elem = agg->GetConstVecItem(0);
  if (elem->GetKind() == kConstAddrofFunc) {
    MIRType &type = elem->GetType();
    MIRConst *constNode = GlobalTables::GetIntConstTable().GetOrCreateIntConst(index, type);
    agg->SetItem(0, constNode, 1);
  }
}

void MUIDReplacement::GenerateDataDefTable() {
  // Use dataDefMap to make sure dataDefTab is sorted by an increasing order of MUID
  for (MIRSymbol *mirSymbol : dataDefSet) {
    MUID muid = GetMUID(mirSymbol->GetName());
    CHECK_FATAL(dataDefMap.find(muid) == dataDefMap.end(), "MUID has been used before, possible collision");
    // Use 0 as the index for now. It will be back-filled once we have the whole map.
    dataDefMap[muid] = SymIdxPair(mirSymbol, 0);
  }
  uint32 idx = 0;
  for (auto &keyVal : dataDefMap) {
    // Fill in the real index
    keyVal.second.second = idx++;
    // Add the def index of cinf for lazy binding.
    if (Options::lazyBinding && !isLibcore) {
      MIRSymbol *mirSymbol = keyVal.second.first;
      if (mirSymbol->IsReflectionClassInfo()) {
        MIRConst *mirConst = mirSymbol->GetKonst();
        auto *aggConst = safe_cast<MIRAggConst>(mirConst);
        // CLASS::kShadow holds a field of uint32 size, for def index.
        // Should we extend it later?
        CHECK_NULL_FATAL(aggConst);
        MIRConst *elemConst = aggConst->GetConstVecItem(static_cast<size_t>(ClassProperty::kShadow));
        auto *intConst = safe_cast<MIRIntConst>(elemConst);
        // We use 0 as flag of not lazy.
        intConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(idx, intConst->GetType());
        aggConst->SetItem(static_cast<size_t>(ClassProperty::kShadow), intConst,
                          aggConst->GetFieldIdItem(static_cast<size_t>(ClassProperty::kShadow)));
      } else {
        ReplaceFieldMetaStaticAddr(*mirSymbol, idx - 1);
      }
    }
  }
  FieldVector parentFields;
  FieldVector fields;
  GlobalTables::GetTypeTable().PushIntoFieldVector(fields, "dataUnifiedAddr",
                                                   *GlobalTables::GetTypeTable().GetCompactPtr());
  auto *dataDefTabEntryType = static_cast<MIRStructType*>(
      GlobalTables::GetTypeTable().GetOrCreateStructType("MUIDDataDefTabEntry", fields, parentFields, GetMIRModule()));
  FieldVector muidFields;
#ifdef USE_64BIT_MUID  // USE_64BIT_MUID
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidLow", *GlobalTables::GetTypeTable().GetUInt32());
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidHigh", *GlobalTables::GetTypeTable().GetUInt32());
#else  // USE_128BIT_MUID
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidLow", *GlobalTables::GetTypeTable().GetUInt64());
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidHigh", *GlobalTables::GetTypeTable().GetUInt64());
#endif
  auto *dataDefMuidTabEntryType =
      static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetOrCreateStructType(
          std::string("MUIDDataDefMuidTabEntry"), muidFields, parentFields, GetMIRModule()));
  size_t arraySize = dataDefMap.size();
  MIRArrayType &arrayType = *GlobalTables::GetTypeTable().GetOrCreateArrayType(*dataDefTabEntryType, arraySize);
  MIRAggConst *dataDefTabConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), arrayType);
  MIRArrayType &muidArrayType = *GlobalTables::GetTypeTable().GetOrCreateArrayType(*dataDefMuidTabEntryType, arraySize);
  MIRAggConst *dataDefMuidTabConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), muidArrayType);
  for (auto keyVal : dataDefMap) {
    MIRSymbol *mirSymbol = keyVal.second.first;
    MIRAggConst *entryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), *dataDefTabEntryType);
    uint32 fieldID = 1;
    MUID muid = keyVal.first;
    MIRAggConst *muidEntryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(),
                                                                                *dataDefMuidTabEntryType);
    uint32 muidFieldID = 1;
    // Will be emitted as 0 and processed by runtime
    builder->AddAddrofFieldConst(*dataDefTabEntryType, *entryConst, fieldID++, *mirSymbol);
    dataDefTabConst->AddItem(entryConst, 0);
    builder->AddIntFieldConst(*dataDefMuidTabEntryType, *muidEntryConst, muidFieldID++, muid.data.words[0]);
    builder->AddIntFieldConst(*dataDefMuidTabEntryType, *muidEntryConst, muidFieldID++, muid.data.words[1]);
    dataDefMuidTabConst->AddItem(muidEntryConst, 0);
    mplMuidStr += muid.ToStr();
    if (trace) {
      LogInfo::MapleLogger() << "dataDefMap, MUID: " << muid.ToStr() << ", Variable Name: " << mirSymbol->GetName()
                             << ", Offset: " << keyVal.second.second << "\n";
    }
  }
  if (Options::dumpMuidFile) {
    DumpMUIDFile(false);
  }
  if (!dataDefTabConst->GetConstVec().empty()) {
    std::string dataDefTabName = namemangler::kMuidDataDefTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    dataDefTabSym = builder->CreateGlobalDecl(dataDefTabName, arrayType);
    dataDefTabSym->SetKonst(dataDefTabConst);
    dataDefTabSym->SetStorageClass(kScFstatic);
    // We add the original def tab for lazy binding, and emit them by CG.
    if (Options::lazyBinding) {
      std::string dataDefOrigTabName = namemangler::kMuidDataDefOrigTabPrefixStr +
                                       GetMIRModule().GetFileNameAsPostfix();
      dataDefOrigTabSym = builder->CreateGlobalDecl(dataDefOrigTabName, arrayType);
      dataDefOrigTabSym->SetKonst(dataDefTabConst);
      dataDefOrigTabSym->SetStorageClass(kScFstatic);
    }
  }
  if (!dataDefMuidTabConst->GetConstVec().empty()) {
    std::string dataDefMuidTabName = namemangler::kMuidDataDefMuidTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    dataDefMuidTabSym = builder->CreateGlobalDecl(dataDefMuidTabName, muidArrayType);
    dataDefMuidTabSym->SetKonst(dataDefMuidTabConst);
    dataDefMuidTabSym->SetStorageClass(kScFstatic);
  }
}

void MUIDReplacement::ReplaceFieldMetaStaticAddr(const MIRSymbol &mirSymbol, uint32 index) {
  std::string symbolName = mirSymbol.GetName();
  MIRSymbol *fieldOffsetDataSt = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
      GlobalTables::GetStrTable().GetStrIdxFromName(namemangler::kFieldOffsetDataPrefixStr + symbolName));
  if (fieldOffsetDataSt == nullptr) {
    if (trace) {
      LogInfo::MapleLogger() << "fieldOffsetDataSt is nullptr, symbolName=" << symbolName << "\n";
    }
    return;
  }
  MIRAggConst *aggConst = safe_cast<MIRAggConst>(fieldOffsetDataSt->GetKonst());
  CHECK_NULL_FATAL(aggConst);
  MIRAggConst *agg = safe_cast<MIRAggConst>(aggConst->GetConstVecItem(0));
  CHECK_NULL_FATAL(agg);
  MIRConst *elem = agg->GetConstVecItem(0);
  CHECK_NULL_FATAL(elem);
  CHECK_FATAL(elem->GetKind() == kConstAddrof, "static field must kConstAddrof.");

  MIRType &type = elem->GetType();
  int64 idx = index * 2 + 1; // add flag to indicate that it's def tab index for emit.
  MIRConst *constNode = GlobalTables::GetIntConstTable().GetOrCreateIntConst(idx, type);
  agg->SetItem(0, constNode, 1);
  (void)idx;
}

void MUIDReplacement::GenerateUnifiedUndefTable() {
  for (MIRFunction *mirFunc : funcUndefSet) {
    MUID muid = GetMUID(mirFunc->GetName());
    CHECK_FATAL(funcUndefMap.find(muid) == funcUndefMap.end(), "MUID has been used before, possible collision");
    // Use 0 as the index for now. It will be back-filled once we have the whole map.
    funcUndefMap[muid] = SymIdxPair(mirFunc->GetFuncSymbol(), 0);
  }
  for (MIRSymbol *mirSymbol : dataUndefSet) {
    MUID muid = GetMUID(mirSymbol->GetName());
    CHECK_FATAL(dataUndefMap.find(muid) == dataUndefMap.end(), "MUID has been used before, possible collision");
    // Use 0 as the index for now. It will be back-filled once we have the whole map.
    dataUndefMap[muid] = SymIdxPair(mirSymbol, 0);
  }
  // Fill in the real index.
  uint32 idx = 0;
  for (auto &keyVal : funcUndefMap) {
    keyVal.second.second = idx++;
  }
  idx = 0;
  for (auto &keyVal : dataUndefMap) {
    keyVal.second.second = idx++;
  }
  FieldVector parentFields;
  FieldVector fields;
  GlobalTables::GetTypeTable().PushIntoFieldVector(fields, "globalAddress",
                                                   *GlobalTables::GetTypeTable().GetCompactPtr());
  auto *unifiedUndefTabEntryType =
      static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetOrCreateStructType(
          std::string("MUIDUnifiedUndefTabEntry"), fields, parentFields, GetMIRModule()));
  FieldVector muidFields;
#ifdef USE_64BIT_MUID
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidLow", *GlobalTables::GetTypeTable().GetUInt32());
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidHigh", *GlobalTables::GetTypeTable().GetUInt32());
#else
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidLow", *GlobalTables::GetTypeTable().GetUInt64());
  GlobalTables::GetTypeTable().PushIntoFieldVector(muidFields, "muidHigh", *GlobalTables::GetTypeTable().GetUInt64());
#endif
  auto *unifiedUndefMuidTabEntryType =
      static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetOrCreateStructType(
          "MUIDUnifiedUndefMuidTabEntry", muidFields, parentFields, GetMIRModule()));
  size_t arraySize = funcUndefMap.size();
  MIRArrayType &funcArrayType = *GlobalTables::GetTypeTable().GetOrCreateArrayType(*unifiedUndefTabEntryType,
                                                                                   arraySize);
  MIRAggConst *funcUndefTabConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), funcArrayType);
  MIRArrayType &funcMuidArrayType =
      *GlobalTables::GetTypeTable().GetOrCreateArrayType(*unifiedUndefMuidTabEntryType, arraySize);
  MIRAggConst *funcUndefMuidTabConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), funcMuidArrayType);
  for (auto keyVal : funcUndefMap) {
    MUID muid = keyVal.first;
    mplMuidStr += muid.ToStr();
    if (trace) {
      LogInfo::MapleLogger() << "funcUndefMap, MUID: " << muid.ToStr()
                             << ", Function Name: " << keyVal.second.first->GetName()
                             << ", Offset: " << keyVal.second.second << "\n";
    }
    MIRAggConst *entryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), *unifiedUndefTabEntryType);
    uint32 fieldID = 1;
    MIRAggConst *muidEntryConst =
      GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), *unifiedUndefMuidTabEntryType);
    uint32 muidFieldID = 1;
    // to be filled by runtime
    builder->AddIntFieldConst(*unifiedUndefTabEntryType, *entryConst, fieldID++, 0);
    funcUndefTabConst->AddItem(entryConst, 0);
    if (sourceFileMethodMap.find(muid) != sourceFileMethodMap.end()) {
      uint32 fileIndex = sourceFileMethodMap[muid].sourceFileIndex;
      uint32 classIndex = sourceFileMethodMap[muid].sourceClassIndex;
      uint32 methodIndex = sourceFileMethodMap[muid].sourceMethodIndex << 1;
      if (sourceFileMethodMap[muid].isVirtual) {
        methodIndex |= 0x1;
      }
      builder->AddIntFieldConst(*unifiedUndefMuidTabEntryType, *muidEntryConst, muidFieldID++,
                                ((fileIndex << kShiftBit16) | classIndex));
      builder->AddIntFieldConst(*unifiedUndefMuidTabEntryType, *muidEntryConst, muidFieldID++,
                                (methodIndex << kShiftBit15) | 0x7FFF);
    } else {
      builder->AddIntFieldConst(*unifiedUndefMuidTabEntryType, *muidEntryConst, muidFieldID++, muid.data.words[0]);
      builder->AddIntFieldConst(*unifiedUndefMuidTabEntryType, *muidEntryConst, muidFieldID++, muid.data.words[1]);
    }
    funcUndefMuidTabConst->AddItem(muidEntryConst, 0);
  }
  if (!funcUndefTabConst->GetConstVec().empty()) {
    std::string funcUndefTabName = namemangler::kMuidFuncUndefTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    funcUndefTabSym = builder->CreateGlobalDecl(funcUndefTabName, funcArrayType);
    funcUndefTabSym->SetKonst(funcUndefTabConst);
    funcUndefTabSym->SetStorageClass(kScFstatic);
  }
  if (!funcUndefMuidTabConst->GetConstVec().empty()) {
    std::string funcUndefMuidTabName =
      namemangler::kMuidFuncUndefMuidTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    funcUndefMuidTabSym = builder->CreateGlobalDecl(funcUndefMuidTabName, funcMuidArrayType);
    funcUndefMuidTabSym->SetKonst(funcUndefMuidTabConst);
    funcUndefMuidTabSym->SetStorageClass(kScFstatic);
  }
  // Continue to generate dataUndefTab
  arraySize = dataUndefMap.size();
  MIRArrayType &dataArrayType = *GlobalTables::GetTypeTable().GetOrCreateArrayType(*unifiedUndefTabEntryType,
                                                                                   arraySize);
  MIRAggConst *dataUndefTabConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), dataArrayType);
  MIRArrayType &dataMuidArrayType =
      *GlobalTables::GetTypeTable().GetOrCreateArrayType(*unifiedUndefMuidTabEntryType, arraySize);
  MIRAggConst *dataUndefMuidTabConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), dataMuidArrayType);
  for (auto keyVal : dataUndefMap) {
    MIRAggConst *entryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), *unifiedUndefTabEntryType);
    uint32 fieldID = 1;
    MIRSymbol *mirSymbol = keyVal.second.first;
    MUID muid = keyVal.first;
    MIRAggConst *muidEntryConst =
        GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), *unifiedUndefMuidTabEntryType);
    uint32 muidFieldID = 1;
    // Will be emitted as 0 and filled by runtime
    builder->AddAddrofFieldConst(*unifiedUndefTabEntryType, *entryConst, fieldID++, *mirSymbol);
    dataUndefTabConst->AddItem(entryConst, 0);
    if (sourceIndexMap.find(muid) != sourceIndexMap.end()) {
      SourceIndexPair pairIndex = sourceIndexMap[muid];
      uint32 value =  (pairIndex.first << kShiftBit16) + pairIndex.second;
      builder->AddIntFieldConst(*unifiedUndefMuidTabEntryType, *muidEntryConst, muidFieldID++, value);
      builder->AddIntFieldConst(*unifiedUndefMuidTabEntryType, *muidEntryConst, muidFieldID++, 0xFFFFFFFF);
    } else if (sourceFileFieldMap.find(muid) != sourceFileFieldMap.end()) {
      uint32 sourceFileIndex = sourceFileFieldMap[muid].sourceFileIndex;
      uint32 sourceClassIndex = sourceFileFieldMap[muid].sourceClassIndex;
      uint32 sourceFieldIndex = sourceFileFieldMap[muid].sourceFieldIndex;
      builder->AddIntFieldConst(*unifiedUndefMuidTabEntryType, *muidEntryConst, muidFieldID++,
                                (sourceFileIndex << kShiftBit16) | sourceClassIndex);
      builder->AddIntFieldConst(*unifiedUndefMuidTabEntryType, *muidEntryConst, muidFieldID++,
                                (sourceFieldIndex << kShiftBit16) | 0xFFFF);
    } else {
      builder->AddIntFieldConst(*unifiedUndefMuidTabEntryType, *muidEntryConst, muidFieldID++, muid.data.words[0]);
      builder->AddIntFieldConst(*unifiedUndefMuidTabEntryType, *muidEntryConst, muidFieldID++, muid.data.words[1]);
    }
    dataUndefMuidTabConst->AddItem(muidEntryConst, 0);
    mplMuidStr += muid.ToStr();
    if (trace) {
      LogInfo::MapleLogger() << "dataUndefMap, MUID: " << muid.ToStr() << ", Variable Name: " << mirSymbol->GetName()
                             << ", Offset: " << keyVal.second.second << "\n";
    }
  }
  if (!dataUndefTabConst->GetConstVec().empty()) {
    std::string dataUndefTabName = namemangler::kMuidDataUndefTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    dataUndefTabSym = builder->CreateGlobalDecl(dataUndefTabName, dataArrayType);
    dataUndefTabSym->SetKonst(dataUndefTabConst);
    dataUndefTabSym->SetStorageClass(kScFstatic);
  }
  if (!dataUndefMuidTabConst->GetConstVec().empty()) {
    std::string dataUndefMuidTabName =
      namemangler::kMuidDataUndefMuidTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    dataUndefMuidTabSym = builder->CreateGlobalDecl(dataUndefMuidTabName, dataMuidArrayType);
    dataUndefMuidTabSym->SetKonst(dataUndefMuidTabConst);
    dataUndefMuidTabSym->SetStorageClass(kScFstatic);
  }
}

void MUIDReplacement::InitRangeTabUseSym(std::vector<MIRSymbol*> &workList, MIRStructType &rangeTabEntryType,
                                         MIRAggConst &rangeTabConst) {
  for (MIRSymbol *mirSymbol : workList) {
    MIRAggConst *entryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), rangeTabEntryType);
    uint32 fieldID = 1;
    if (mirSymbol != nullptr) {
      builder->AddAddrofFieldConst(rangeTabEntryType, *entryConst, fieldID++, *mirSymbol);
      builder->AddAddrofFieldConst(rangeTabEntryType, *entryConst, fieldID++, *mirSymbol);
    } else {
      builder->AddIntFieldConst(rangeTabEntryType, *entryConst, fieldID++, 0);
      builder->AddIntFieldConst(rangeTabEntryType, *entryConst, fieldID++, 0);
    }
    rangeTabConst.AddItem(entryConst, 0);
  }
}

// RangeTable stores begin and end of all MUID tables
void MUIDReplacement::GenerateRangeTable() {
  FieldVector parentFields;
  FieldVector fields;
  GlobalTables::GetTypeTable().PushIntoFieldVector(fields, "tabBegin", *GlobalTables::GetTypeTable().GetVoidPtr());
  GlobalTables::GetTypeTable().PushIntoFieldVector(fields, "tabEnd", *GlobalTables::GetTypeTable().GetVoidPtr());
  auto &rangeTabEntryType = static_cast<MIRStructType&>(
      *GlobalTables::GetTypeTable().GetOrCreateStructType("MUIDRangeTabEntry", fields, parentFields, GetMIRModule()));
  MIRArrayType &rangeArrayType = *GlobalTables::GetTypeTable().GetOrCreateArrayType(rangeTabEntryType, 0);
  MIRAggConst *rangeTabConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), rangeArrayType);
  // First entry is reserved for a compile-time-stamp
  // Second entry is reserved for a decouple-stamp
  mplMuidStr += kMplLinkerVersionNumber;
  const std::string muidStr[2] = { mplMuidStr, mplMuidStr + GetMplMd5().ToStr() };
  for (auto &item : muidStr) {
    uint32 fieldID = 1;
    MIRAggConst *entryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), rangeTabEntryType);
    MUID mplMd5 = GetMUID(item);
    builder->AddIntFieldConst(rangeTabEntryType, *entryConst, fieldID++, mplMd5.data.words[0]);
    builder->AddIntFieldConst(rangeTabEntryType, *entryConst, fieldID++, mplMd5.data.words[1]);
    rangeTabConst->AddItem(entryConst, 0);
  }
  for (uint32 i = RangeIdx::kVtabAndItab; i < RangeIdx::kOldMaxNum; ++i) {
    // Use an integer to mark which entry is for which table
    MIRAggConst *entryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), rangeTabEntryType);
    uint32 fieldID = 1;
    if (i == RangeIdx::kGlobalRootlist) {
      MIRSymbol *st = GetSymbolFromName(namemangler::kGcRootList);
      if (st == nullptr) {
        builder->AddIntFieldConst(rangeTabEntryType, *entryConst, fieldID++, 0);
        builder->AddIntFieldConst(rangeTabEntryType, *entryConst, fieldID++, 0);
        rangeTabConst->AddItem(entryConst, 0);
        continue;
      }
    }
    builder->AddIntFieldConst(rangeTabEntryType, *entryConst, fieldID++, i);
    builder->AddIntFieldConst(rangeTabEntryType, *entryConst, fieldID++, i);
    rangeTabConst->AddItem(entryConst, 0);
  }
  // Please refer to mrt/compiler-rt/include/mpl_linker.h for the layout
  std::vector<MIRSymbol*> workList = {
      funcDefTabSym,
      funcDefOrigTabSym,
      funcInfTabSym,
      funcUndefTabSym,
      dataDefTabSym,
      dataDefOrigTabSym,
      dataUndefTabSym,
      funcDefMuidTabSym,
      funcUndefMuidTabSym,
      dataDefMuidTabSym,
      dataUndefMuidTabSym,
      funcMuidIdxTabSym,
      funcProfileTabSym
  };
  InitRangeTabUseSym(workList, rangeTabEntryType, *rangeTabConst);
  for (int i = RangeIdx::kOldMaxNum + 1; i < RangeIdx::kNewMaxNum; ++i) {
    uint32 fieldID = 1;
    MIRAggConst *entryConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), rangeTabEntryType);
    builder->AddIntFieldConst(rangeTabEntryType, *entryConst, fieldID++, i);
    builder->AddIntFieldConst(rangeTabEntryType, *entryConst, fieldID++, i);
    rangeTabConst->AddItem(entryConst, 0);
  }
  std::string bbProfileName = namemangler::kBBProfileTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
  MIRSymbol *funcProfCounterTabSym = GetSymbolFromName(bbProfileName);
  std::vector<MIRSymbol*> irProfWorkList = {
    funcProfInfTabSym,
    funcProfCounterTabSym
  };
  InitRangeTabUseSym(irProfWorkList, rangeTabEntryType, *rangeTabConst);
  if (!rangeTabConst->GetConstVec().empty()) {
    rangeArrayType.SetSizeArrayItem(0, rangeTabConst->GetConstVec().size());
    std::string rangeTabName = namemangler::kMuidRangeTabPrefixStr + GetMIRModule().GetFileNameAsPostfix();
    rangeTabSym = builder->CreateGlobalDecl(rangeTabName, rangeArrayType);
    rangeTabSym->SetKonst(rangeTabConst);
    rangeTabSym->SetStorageClass(kScFstatic);
  }
}

uint32 MUIDReplacement::FindIndexFromDefTable(const MIRSymbol &mirSymbol, bool isFunc) {
  MUID muid = GetMUID(mirSymbol.GetName());
  if (isFunc) {
    CHECK_FATAL(defMuidIdxMap.find(muid) != defMuidIdxMap.end(), "Local function %s not found in funcDefMap",
                mirSymbol.GetName().c_str());
    return defMuidIdxMap[muid];
  } else {
    CHECK_FATAL(dataDefMap.find(muid) != dataDefMap.end(), "Local variable %s not found in dataDefMap",
                mirSymbol.GetName().c_str());
    return dataDefMap[muid].second;
  }
}

uint32 MUIDReplacement::FindIndexFromUndefTable(const MIRSymbol &mirSymbol, bool isFunc) {
  MUID muid = GetMUID(mirSymbol.GetName());
  if (isFunc) {
    CHECK_FATAL(funcUndefMap.find(muid) != funcUndefMap.end(), "Extern function %s not found in funcUndefMap",
                mirSymbol.GetName().c_str());
    return funcUndefMap[muid].second;
  } else {
    CHECK_FATAL(dataUndefMap.find(muid) != dataUndefMap.end(), "Extern variable %s not found in dataUndefMap",
                mirSymbol.GetName().c_str());
    return dataUndefMap[muid].second;
  }
}

void MUIDReplacement::ClearVtabItab(const std::string &name) {
  MIRSymbol *tabSym = GetSymbolFromName(name);
  if (tabSym == nullptr) {
    return;
  }
  auto *oldConst = tabSym->GetKonst();
  if (oldConst == nullptr || oldConst->GetKind() != kConstAggConst) {
    return;
  }
  safe_cast<MIRAggConst>(oldConst)->GetConstVec().clear();
}

void MUIDReplacement::ReplaceFuncTable(const std::string &name) {
  MIRSymbol *tabSym = GetSymbolFromName(name);
  if (tabSym == nullptr) {
    return;
  }
  auto *oldConst = tabSym->GetKonst();
  if (oldConst == nullptr || oldConst->GetKind() != kConstAggConst) {
    return;
  }
  bool isVtab = false;
  if (tabSym->GetName().find(VTAB_PREFIX_STR) == 0) {
    isVtab = true;
  }
  for (auto *&oldTabEntry : safe_cast<MIRAggConst>(oldConst)->GetConstVec()) {
    CHECK_NULL_FATAL(oldTabEntry);
    if (oldTabEntry->GetKind() == kConstAggConst) {
      auto *aggrC = static_cast<MIRAggConst*>(oldTabEntry);
      for (size_t i = 0; i < aggrC->GetConstVec().size(); ++i) {
        ReplaceAddroffuncConst(aggrC->GetConstVecItem(i), i + 1, isVtab);
      }
    } else if (oldTabEntry->GetKind() == kConstAddrofFunc) {
      ReplaceAddroffuncConst(oldTabEntry, 0xffffffff, isVtab);
    }
  }
}

void MUIDReplacement::ReplaceAddroffuncConst(MIRConst *&entry, uint32 fieldID, bool isVtab = false) {
  CHECK_NULL_FATAL(entry);
  if (entry->GetKind() != kConstAddrofFunc) {
    return;
  }
  MIRType &voidType = *GlobalTables::GetTypeTable().GetVoidPtr();
  auto *funcAddr = static_cast<MIRAddroffuncConst*>(entry);
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(funcAddr->GetValue());
  uint64 offset = 0;
  MIRIntConst *constNode = nullptr;
  if (func->GetBody() != nullptr) {
    ASSERT(func->GetFuncSymbol() != nullptr, "null ptr check!");
    offset = FindIndexFromDefTable(*(func->GetFuncSymbol()), true);
    // Left shifting is needed because in itable 0 and 1 are reserved.
    // 0 marks no entry and 1 marks a conflict.
    // The second least significant bit is set to 1, indicating
    // this is an index into the funcDefTab
    constexpr uint64 idxIntoFuncDefTabFlag = 2u;
    constNode = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        static_cast<int64>(((offset + 1) << KReservedBits) + idxIntoFuncDefTabFlag), voidType);
  } else if (isVtab && func->IsAbstract()) {
    MIRType &type = *GlobalTables::GetTypeTable().GetVoidPtr();
    constNode = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, type);
  } else {
    ASSERT(func->GetFuncSymbol() != nullptr, "null ptr check!");
    offset = FindIndexFromUndefTable(*(func->GetFuncSymbol()), true);
    // The second least significant bit is set to 0, indicating
    // this is an index into the funcUndefTab
    constNode = GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>((offset + 1) << KReservedBits),
                                                                     voidType);
  }
  if (fieldID != 0xffffffff) {
    constNode = GlobalTables::GetIntConstTable().GetOrCreateIntConst(constNode->GetExtValue(),
                                                                     constNode->GetType());
  }
  entry = constNode;
}

void MUIDReplacement::ReplaceFieldTypeTable(const std::string &name) {
  MIRSymbol *tabSym = GetSymbolFromName(name);
  if (tabSym == nullptr) {
    return;
  }
  auto *oldConst = safe_cast<MIRAggConst>(tabSym->GetKonst());
  if (oldConst == nullptr) {
    return;
  }
  for (MIRConst *&oldTabEntry : oldConst->GetConstVec()) {
    CHECK_NULL_FATAL(oldTabEntry);
    if (oldTabEntry->GetKind() == kConstAggConst) {
      uint32 index = static_cast<uint32>(FieldProperty::kPClassType);
      auto *aggrC = static_cast<MIRAggConst*>(oldTabEntry);
      CHECK_NULL_FATAL(aggrC->GetConstVecItem(index));
      if (aggrC->GetConstVecItem(index)->GetKind() == kConstInt) {
        continue;
      } else {
        ReplaceAddrofConst(aggrC->GetConstVecItem(index), true);
        MIRConstPtr mirConst = aggrC->GetConstVecItem(index);
        CHECK_NULL_FATAL(mirConst);
        if (mirConst->GetKind() == kConstInt) {
          MIRIntConst *newIntConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
              static_cast<MIRIntConst*>(mirConst)->GetExtValue(), mirConst->GetType());
          aggrC->SetItem(index, newIntConst, index + 1);
        } else {
          aggrC->SetFieldIdOfElement(index, index + 1);
        }
      }
    } else if (oldTabEntry->GetKind() == kConstAddrof) {
      ReplaceAddrofConst(oldTabEntry, true);
    }
  }
}

void MUIDReplacement::ReplaceDataTable(const std::string &name) {
  MIRSymbol *tabSym = GetSymbolFromName(name);
  if (tabSym == nullptr) {
    return;
  }
  auto *oldConst = safe_cast<MIRAggConst>(tabSym->GetKonst());
  if (oldConst == nullptr) {
    return;
  }
  for (MIRConst *&oldTabEntry : oldConst->GetConstVec()) {
    CHECK_NULL_FATAL(oldTabEntry);
    if (oldTabEntry->GetKind() == kConstAggConst) {
      auto *aggrC = static_cast<MIRAggConst*>(oldTabEntry);
      for (uint32 i = 0; i < static_cast<uint32>(aggrC->GetConstVec().size()); ++i) {
        CHECK_NULL_FATAL(aggrC->GetConstVecItem(i));
        ReplaceAddrofConst(aggrC->GetConstVecItem(i));
        MIRConstPtr mirConst = aggrC->GetConstVecItem(i);
        if (mirConst->GetKind() == kConstInt) {
          MIRIntConst *newIntConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
              static_cast<MIRIntConst*>(mirConst)->GetExtValue(), mirConst->GetType());
          aggrC->SetItem(i, newIntConst, i + 1);
        } else {
          aggrC->SetFieldIdOfElement(i, i + 1);
        }
      }
    } else if (oldTabEntry->GetKind() == kConstAddrof) {
      ReplaceAddrofConst(oldTabEntry);
    }
  }
}

void MUIDReplacement::ReplaceDecoupleKeyTable(MIRAggConst* oldConst) {
  if (oldConst == nullptr) {
    return;
  }
  for (MIRConst *&oldTabEntry : oldConst->GetConstVec()) {
    CHECK_NULL_FATAL(oldTabEntry);
    if (oldTabEntry->GetKind() == kConstAggConst) {
      auto *aggrC = static_cast<MIRAggConst*>(oldTabEntry);
      for (uint32 i = 0; i < static_cast<uint32>(aggrC->GetConstVec().size()); ++i) {
        CHECK_NULL_FATAL(aggrC->GetConstVecItem(i));
        if (aggrC->GetConstVecItem(i)->GetKind() == kConstAggConst) {
          ReplaceDecoupleKeyTable(safe_cast<MIRAggConst>(aggrC->GetConstVecItem(i)));
        } else {
          ReplaceAddrofConst(aggrC->GetConstVecItem(i));
          MIRConstPtr mirConst = aggrC->GetConstVecItem(i);
          if (mirConst->GetKind() == kConstInt) {
            MIRIntConst *newIntConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
                static_cast<MIRIntConst*>(mirConst)->GetExtValue(), mirConst->GetType());
            aggrC->SetItem(i, newIntConst, i + 1);
          } else {
            aggrC->SetFieldIdOfElement(i, i + 1);
          }
        }
      }
    } else if (oldTabEntry->GetKind() == kConstAddrof) {
      ReplaceAddrofConst(oldTabEntry);
    }
  }
}

void MUIDReplacement::ReplaceAddrofConst(MIRConst *&entry, bool muidIndex32Mod) {
  if (entry->GetKind() != kConstAddrof) {
    return;
  }
  MIRType &voidType = *GlobalTables::GetTypeTable().GetVoidPtr();
  auto *addr = static_cast<MIRAddrofConst*>(entry);
  MIRSymbol *addrSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(addr->GetSymbolIndex().Idx());
  CHECK_NULL_FATAL(addrSym);
  if (!addrSym->IsReflectionClassInfo() && !addrSym->IsStatic()) {
    return;
  }
  uint64 offset = 0;
  MIRIntConst *constNode = nullptr;
  if (addrSym->GetStorageClass() != kScExtern) {
    offset = FindIndexFromDefTable(*addrSym, false);
    constNode = GlobalTables::GetIntConstTable().GetOrCreateIntConst(GetDefOrUndefOffsetWithMask(offset, true,
                                                                     muidIndex32Mod), voidType);
  } else {
    offset = FindIndexFromUndefTable(*addrSym, false);
    constNode = GlobalTables::GetIntConstTable().GetOrCreateIntConst(GetDefOrUndefOffsetWithMask(offset, false,
                                                                     muidIndex32Mod), voidType);
  }
  entry = constNode;
}

int64 MUIDReplacement::GetDefOrUndefOffsetWithMask(uint64 offset, bool isDef, bool muidIndex32Mod) const {
  if (isDef) {
    if (muidIndex32Mod) {
      return static_cast<int64>(static_cast<uint32>(offset) | kFromDefIndexMask32Mod);
    } else {
#ifdef USE_ARM32_MACRO
      return static_cast<int64>((offset << KReservedBits) + kLinkerRefDef);
#else
      return static_cast<int64>(offset | kFromDefIndexMask);
#endif
    }
  } else {
    if (muidIndex32Mod) {
      return static_cast<int64>(static_cast<uint32>(offset) | kFromUndefIndexMask32Mod);
    } else {
#ifdef USE_ARM32_MACRO
      return static_cast<int64>((offset << KReservedBits) + kLinkerRefUnDef);
#else
      return static_cast<int64>(offset | kFromUndefIndexMask);
#endif
    }
  }
}

void MUIDReplacement::ReplaceDirectInvokeOrAddroffunc(MIRFunction &currentFunc, StmtNode &stmt) {
  PUIdx puidx;
  CallNode *callNode = nullptr;
  DassignNode *dassignNode = nullptr;
  RegassignNode *regassignNode = nullptr;
  if (stmt.GetOpCode() == OP_callassigned || stmt.GetOpCode() == OP_call) {
    callNode = static_cast<CallNode*>(&stmt);
    puidx = callNode->GetPUIdx();
  } else if (stmt.GetOpCode() == OP_dassign) {
    dassignNode = static_cast<DassignNode*>(&stmt);
    if (dassignNode->GetRHS()->GetOpCode() != OP_addroffunc) {
      return;
    }
    puidx = static_cast<AddroffuncNode*>(dassignNode->GetRHS())->GetPUIdx();
  } else if (stmt.GetOpCode() == OP_regassign) {
    regassignNode = static_cast<RegassignNode*>(&stmt);
    if (regassignNode->Opnd(0)->GetOpCode() != OP_addroffunc) {
      return;
    }
    puidx = static_cast<AddroffuncNode*>(regassignNode->Opnd(0))->GetPUIdx();
  } else {
    CHECK_FATAL(false, "unexpected stmt type in ReplaceDirectInvokeOrAddroffunc");
  }
  MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puidx);
  if (!calleeFunc->IsJava() && calleeFunc->GetBaseClassName().empty()) {
    return;
  }
  // Load the function pointer
  AddrofNode *baseExpr = nullptr;
  uint32 index = 0;
  MIRArrayType *arrayType = nullptr;
  if (calleeFunc->GetBody() != nullptr) {
    // Local function is accessed through funcDefTab
    // Add a comment to store the original function name
    std::string commentLabel = namemangler::kMarkMuidFuncDefStr + calleeFunc->GetName();
    currentFunc.GetBody()->InsertBefore(&stmt, builder->CreateStmtComment(commentLabel));

    std::string moduleName = GetMIRModule().GetFileNameAsPostfix();
    std::string baseName = calleeFunc->GetBaseClassName();
    ASSERT(funcDefTabSym != nullptr, "null ptr check!");
    baseExpr = builder->CreateExprAddrof(0, *funcDefTabSym, GetMIRModule().GetMemPool());
    ASSERT(calleeFunc->GetFuncSymbol() != nullptr, "null ptr check!");
    index = FindIndexFromDefTable(*(calleeFunc->GetFuncSymbol()), true);
    arrayType = static_cast<MIRArrayType*>(funcDefTabSym->GetType());
  } else {
    // External function is accessed through funcUndefTab
    // Add a comment to store the original function name
    std::string commentLabel = namemangler::kMarkMuidFuncUndefStr + calleeFunc->GetName();
    currentFunc.GetBody()->InsertBefore(&stmt, builder->CreateStmtComment(commentLabel));

    ASSERT(funcUndefTabSym != nullptr, "null ptr check!");
    baseExpr = builder->CreateExprAddrof(0, *funcUndefTabSym, GetMIRModule().GetMemPool());
    ASSERT(calleeFunc->GetFuncSymbol() != nullptr, "null ptr check!");
    index = FindIndexFromUndefTable(*(calleeFunc->GetFuncSymbol()), true);
    arrayType = static_cast<MIRArrayType*>(funcUndefTabSym->GetType());
  }
  ConstvalNode *offsetExpr = GetConstvalNode(index);
  ArrayNode *arrayExpr = builder->CreateExprArray(*arrayType, baseExpr, offsetExpr);
  arrayExpr->SetBoundsCheck(false);
  MIRType *elemType = arrayType->GetElemType();
  BaseNode *ireadPtrExpr =
      builder->CreateExprIread(*GlobalTables::GetTypeTable().GetVoidPtr(),
                               *GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType), 1, arrayExpr);
  PregIdx funcPtrPreg = 0;
  MIRSymbol *funcPtrSym = nullptr;
  BaseNode *readFuncPtr = nullptr;
  if (Options::usePreg) {
    funcPtrPreg = currentFunc.GetPregTab()->CreatePreg(PTY_ptr);
    RegassignNode *funcPtrPregAssign = builder->CreateStmtRegassign(PTY_ptr, funcPtrPreg, ireadPtrExpr);
    currentFunc.GetBody()->InsertBefore(&stmt, funcPtrPregAssign);
    readFuncPtr = builder->CreateExprRegread(PTY_ptr, funcPtrPreg);
  } else {
    funcPtrSym = builder->GetOrCreateLocalDecl(kMuidSymPtrStr, *GlobalTables::GetTypeTable().GetVoidPtr());
    DassignNode *addrNode = builder->CreateStmtDassign(*funcPtrSym, 0, ireadPtrExpr);
    currentFunc.GetBody()->InsertBefore(&stmt, addrNode);
    readFuncPtr = builder->CreateExprDread(*funcPtrSym);
  }
  if (callNode != nullptr) {
    // Create icallNode to replace callNode
    IcallNode *icallNode = GetMIRModule().CurFuncCodeMemPool()->New<IcallNode>(
      GetMIRModule(), callNode->GetOpCode() == OP_call ? OP_icall : OP_icallassigned);
    icallNode->SetNumOpnds(callNode->GetNumOpnds() + 1);
    icallNode->GetNopnd().resize(icallNode->GetNumOpnds());
    icallNode->SetNOpndAt(0, readFuncPtr);
    for (size_t i = 1; i < icallNode->GetNopndSize(); ++i) {
      icallNode->SetNOpndAt(i, callNode->GetNopnd()[i - 1]->CloneTree(GetMIRModule().GetCurFuncCodeMPAllocator()));
    }
    icallNode->SetRetTyIdx(calleeFunc->GetReturnTyIdx());
    if (callNode->GetOpCode() == OP_callassigned) {
      icallNode->SetReturnVec(callNode->GetReturnVec());
    }
    currentFunc.GetBody()->ReplaceStmt1WithStmt2(callNode, icallNode);
  } else if (dassignNode != nullptr) {
    dassignNode->SetRHS(readFuncPtr);
  } else if (regassignNode != nullptr) {
    regassignNode->SetOpnd(readFuncPtr, 0);
  }
}

void MUIDReplacement::ReplaceDassign(MIRFunction &currentFunc, const DassignNode &dassignNode) {
  MIRSymbol *mirSymbol = currentFunc.GetLocalOrGlobalSymbol(dassignNode.GetStIdx());
  ASSERT(mirSymbol != nullptr, "null ptr check!");
  if (!mirSymbol->IsStatic()) {
    return;
  }
  // Add a comment to store the original symbol name
  currentFunc.GetBody()->InsertBefore(&dassignNode, builder->CreateStmtComment("Assign to: " + mirSymbol->GetName()));
  // Load the symbol pointer
  AddrofNode *baseExpr = nullptr;
  uint32 index = 0;
  MIRArrayType *arrayType = nullptr;
  if (mirSymbol->GetStorageClass() != kScExtern) {
    // Local static member is accessed through dataDefTab
    baseExpr = builder->CreateExprAddrof(0, *dataDefTabSym);
    index = FindIndexFromDefTable(*mirSymbol, false);
    arrayType = static_cast<MIRArrayType*>(dataDefTabSym->GetType());
  } else {
    // External static member is accessed through dataUndefTab
    baseExpr = builder->CreateExprAddrof(0, *dataUndefTabSym);
    index = FindIndexFromUndefTable(*mirSymbol, false);
    arrayType = static_cast<MIRArrayType*>(dataUndefTabSym->GetType());
  }
  ConstvalNode *offsetExpr = GetConstvalNode(index);
  ArrayNode *arrayExpr = builder->CreateExprArray(*arrayType, baseExpr, offsetExpr);
  arrayExpr->SetBoundsCheck(false);
  MIRType *elemType = arrayType->GetElemType();
  MIRType *mVoidPtr = GlobalTables::GetTypeTable().GetVoidPtr();
  CHECK_FATAL(mVoidPtr != nullptr, "null ptr check");
  BaseNode *ireadPtrExpr =
      builder->CreateExprIread(*mVoidPtr, *GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType),
                               1, arrayExpr);
  PregIdx symPtrPreg = 0;
  MIRSymbol *symPtrSym = nullptr;
  BaseNode *destExpr = nullptr;
  if (Options::usePreg) {
    symPtrPreg = currentFunc.GetPregTab()->CreatePreg(PTY_ptr);
    RegassignNode *symPtrPregAssign = builder->CreateStmtRegassign(PTY_ptr, symPtrPreg, ireadPtrExpr);
    currentFunc.GetBody()->InsertBefore(&dassignNode, symPtrPregAssign);
    destExpr = builder->CreateExprRegread(PTY_ptr, symPtrPreg);
  } else {
    symPtrSym = builder->GetOrCreateLocalDecl(kMuidFuncPtrStr, *mVoidPtr);
    DassignNode *addrNode = builder->CreateStmtDassign(*symPtrSym, 0, ireadPtrExpr);
    currentFunc.GetBody()->InsertBefore(&dassignNode, addrNode);
    destExpr = builder->CreateExprDread(*symPtrSym);
  }
  // Replace dassignNode with iassignNode
  TypeAttrs attrs;
  if (mirSymbol->IsVolatile()) {
    attrs.SetAttr(ATTR_volatile);
  }
  MIRType *destPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirSymbol->GetType(), PTY_ptr, attrs);
  StmtNode *iassignNode = builder->CreateStmtIassign(*destPtrType, 0, destExpr, dassignNode.Opnd(0));
  currentFunc.GetBody()->ReplaceStmt1WithStmt2(&dassignNode, iassignNode);
}

void MUIDReplacement::CollectFieldCallSite() {
  for (MIRFunction *mirFunc : GetMIRModule().GetFunctionList()) {
    if (mirFunc->GetBody() == nullptr) {
      continue;
    }
    SetCurrentFunction(*mirFunc);
    StmtNode *stmt = mirFunc->GetBody()->GetFirst();
    StmtNode *next = nullptr;
    while (stmt != nullptr) {
      next = stmt->GetNext();
      CollectDreadStmt(mirFunc, mirFunc->GetBody(), stmt);
      stmt = next;
    }
  }
}

void MUIDReplacement::CollectDreadStmt(MIRFunction *currentFunc, BlockNode *block, StmtNode *stmt) {
  if (currentFunc == nullptr || stmt == nullptr) {
    return;
  }
  switch (stmt->GetOpCode()) {
    case OP_if: {
      auto *iNode = static_cast<IfStmtNode*>(stmt);
      ASSERT(block != nullptr, "null ptr check!");
      iNode->SetOpnd(CollectDreadExpr(currentFunc, *block, stmt, iNode->Opnd(0)), 0);
      CollectDreadStmt(currentFunc, nullptr, iNode->GetThenPart());
      CollectDreadStmt(currentFunc, nullptr, iNode->GetElsePart());
      break;
    }
    case OP_while: {
      auto *wNode = static_cast<WhileStmtNode*>(stmt);
      ASSERT(block != nullptr, "null ptr check!");
      wNode->SetOpnd(CollectDreadExpr(currentFunc, *block, stmt, wNode->Opnd(0)), 0);
      CollectDreadStmt(currentFunc, nullptr, wNode->GetBody());
      break;
    }
    case OP_block: {
      auto *bNode = static_cast<BlockNode*>(stmt);
      for (auto &stmtNode : bNode->GetStmtNodes()) {
        CollectDreadStmt(currentFunc, bNode, &stmtNode);
      }
      break;
    }
    default:
      ASSERT(block != nullptr, "null ptr check!");
      for (size_t i = 0; i < stmt->NumOpnds(); ++i) {
        stmt->SetOpnd(CollectDreadExpr(currentFunc, *block, stmt, stmt->Opnd(i)), i);
      }
  }
}

void MUIDReplacement::ReplaceDreadStmt(MIRFunction *currentFunc, StmtNode *stmt) {
  if (currentFunc == nullptr || stmt == nullptr) {
    return;
  }
  switch (stmt->GetOpCode()) {
    case OP_if: {
      auto *iNode = static_cast<IfStmtNode*>(stmt);
      iNode->SetOpnd(ReplaceDreadExpr(currentFunc, stmt, iNode->Opnd(0)), 0);
      ReplaceDreadStmt(currentFunc, iNode->GetThenPart());
      ReplaceDreadStmt(currentFunc, iNode->GetElsePart());
      break;
    }
    case OP_while: {
      auto *wNode = static_cast<WhileStmtNode*>(stmt);
      wNode->SetOpnd(ReplaceDreadExpr(currentFunc, stmt, wNode->Opnd(0)), 0);
      ReplaceDreadStmt(currentFunc, wNode->GetBody());
      break;
    }
    case OP_block: {
      auto *bNode = static_cast<BlockNode*>(stmt);
      for (auto &s : bNode->GetStmtNodes()) {
        ReplaceDreadStmt(currentFunc, &s);
      }
      break;
    }
    default: {
      for (size_t i = 0; i < stmt->NumOpnds(); ++i) {
        stmt->SetOpnd(ReplaceDreadExpr(currentFunc, stmt, stmt->Opnd(i)), i);
      }
      break;
    }
  }
}

BaseNode *MUIDReplacement::CollectDreadExpr(MIRFunction *currentFunc, BlockNode &block, StmtNode *stmt,
                                            BaseNode *expr) {
  if (currentFunc == nullptr || stmt == nullptr || expr == nullptr) {
    return nullptr;
  }
  if (expr->GetOpCode() == OP_intrinsicop) {
    auto *intrinsicsNode = static_cast<IntrinsicopNode*>(expr);
    if (intrinsicsNode->GetIntrinsic() == INTRN_MPL_READ_OVTABLE_ENTRY ||
        intrinsicsNode->GetIntrinsic() == INTRN_MPL_READ_OVTABLE_ENTRY2) {
      std::string valueSymName = namemangler::kOffsetTabStr + GetMIRModule().GetFileNameAsPostfix();
      MIRSymbol *offsetTableSym = GetSymbolFromName(valueSymName);
      ASSERT(offsetTableSym != nullptr, "null ptr check");
      auto *arrayType = static_cast<MIRArrayType*>(offsetTableSym->GetType());
      MIRType *elemType = arrayType->GetElemType();
      IreadNode *iread = builder->CreateExprIread(*GlobalTables::GetTypeTable().GetUInt32(),
                                                  *GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType), 1,
                                                  intrinsicsNode->GetNopndAt(0));
      if (intrinsicsNode->GetIntrinsic() == INTRN_MPL_READ_OVTABLE_ENTRY ||
          intrinsicsNode->GetIntrinsic() == INTRN_MPL_READ_OVTABLE_ENTRY2) {
        return iread;
      } else {
        ASSERT(false, "Wrong lazy binding option");
      }
    }
  }
  for (size_t i = 0; i < expr->NumOpnds(); ++i) {
    expr->SetOpnd(CollectDreadExpr(currentFunc, block, stmt, expr->Opnd(i)), i);
  }
  return expr;
}

// Turn dread into iread
BaseNode *MUIDReplacement::ReplaceDreadExpr(MIRFunction *currentFunc, StmtNode *stmt, BaseNode *expr) {
  if (currentFunc == nullptr || stmt == nullptr || expr == nullptr) {
    return nullptr;
  }
  size_t i = 0;
  UnaryNode *uOpnd = nullptr;
  BinaryNode *bopnds = nullptr;
  TernaryNode *topnds = nullptr;
  switch (expr->GetOpCode()) {
    case OP_dread:
    case OP_addrof: {
      return ReplaceDread(*currentFunc, stmt, expr);
    }
    case OP_select: {
      topnds = static_cast<TernaryNode*>(expr);
      for (i = 0; i < topnds->NumOpnds(); ++i) {
        topnds->SetOpnd(ReplaceDreadExpr(currentFunc, stmt, topnds->Opnd(i)), i);
      }
      break;
    }
    default: {
      if (expr->IsUnaryNode()) {
        uOpnd = static_cast<UnaryNode*>(expr);
        uOpnd->SetOpnd(ReplaceDreadExpr(currentFunc, stmt, uOpnd->Opnd(0)), i);
      } else if (expr->IsBinaryNode()) {
        bopnds = static_cast<BinaryNode*>(expr);
        for (i = 0; i < bopnds->NumOpnds(); ++i) {
          bopnds->SetOpnd(ReplaceDreadExpr(currentFunc, stmt, bopnds->GetBOpnd(i)), i);
        }
      } else {
        for (i = 0; i < expr->NumOpnds(); ++i) {
          expr->SetOpnd(ReplaceDreadExpr(currentFunc, stmt, expr->Opnd(i)), i);
        }
      }
      break;
    }
  }
  return expr;
}

BaseNode *MUIDReplacement::ReplaceDread(MIRFunction &currentFunc, const StmtNode *stmt, BaseNode *opnd) {
  if (opnd == nullptr || (opnd->GetOpCode() != OP_dread && opnd->GetOpCode() != OP_addrof)) {
    return opnd;
  }
  auto *dreadNode = static_cast<DreadNode*>(opnd);
  MIRSymbol *mirSymbol = currentFunc.GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
  ASSERT(mirSymbol != nullptr, "null ptr check!");
  if (!mirSymbol->IsStatic()) {
    return opnd;
  }
  // Add a comment to store the original symbol name
  currentFunc.GetBody()->InsertBefore(stmt, builder->CreateStmtComment("Read from: " + mirSymbol->GetName()));
  // Load the symbol pointer
  AddrofNode *baseExpr = nullptr;
  uint32 index = 0;
  MIRArrayType *arrayType = nullptr;
  if (mirSymbol->GetStorageClass() != kScExtern) {
    // Local static member is accessed through dataDefTab
    baseExpr = builder->CreateExprAddrof(0, *dataDefTabSym);
    index = FindIndexFromDefTable(*mirSymbol, false);
    arrayType = static_cast<MIRArrayType*>(dataDefTabSym->GetType());
  } else {
    // External static member is accessed through dataUndefTab
    baseExpr = builder->CreateExprAddrof(0, *dataUndefTabSym);
    index = FindIndexFromUndefTable(*mirSymbol, false);
    arrayType = static_cast<MIRArrayType*>(dataUndefTabSym->GetType());
  }
  ConstvalNode *offsetExpr = GetConstvalNode(index);
  ArrayNode *arrayExpr = builder->CreateExprArray(*arrayType, baseExpr, offsetExpr);
  arrayExpr->SetBoundsCheck(false);
  MIRType *elemType = arrayType->GetElemType();
  BaseNode *ireadPtrExpr =
      builder->CreateExprIread(*GlobalTables::GetTypeTable().GetVoidPtr(),
                               *GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType), 1, arrayExpr);
  if (opnd->GetOpCode() == OP_addrof) {
    return ireadPtrExpr;
  }
  MIRType *destType = mirSymbol->GetType();
  MIRType *destPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*destType);
  return builder->CreateExprIread(*destType, *destPtrType, 0, ireadPtrExpr);
}

void MUIDReplacement::ProcessFunc(MIRFunction *func) {
  // Libcore-all module is self-contained, so no need to do all these replacement
  ASSERT(func != nullptr, "null ptr check!");
  if (isLibcore || func->IsEmpty()) {
    return;
  }
  SetCurrentFunction(*func);
  StmtNode *stmt = func->GetBody()->GetFirst();
  StmtNode *next = nullptr;
  while (stmt != nullptr) {
    next = stmt->GetNext();
    ReplaceDreadStmt(func, stmt);
    // Replace direct func invoke
    if (stmt->GetOpCode() == OP_callassigned || stmt->GetOpCode() == OP_call) {
      ReplaceDirectInvokeOrAddroffunc(*func, *stmt);
    } else if (stmt->GetOpCode() == OP_dassign) {
      ReplaceDirectInvokeOrAddroffunc(*func, *stmt);
      auto *dassignNode = static_cast<DassignNode*>(stmt);
      ReplaceDassign(*func, *dassignNode);
    } else if (stmt->GetOpCode() == OP_regassign) {
      ReplaceDirectInvokeOrAddroffunc(*func, *stmt);
    }
    stmt = next;
  }
}

// Create GC Root
void MUIDReplacement::GenerateGlobalRootList() {
  MIRType *voidType = GlobalTables::GetTypeTable().GetVoidPtr();
  MIRArrayType *arrayType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*voidType, 0);
  MIRAggConst *newConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), *arrayType);
  for (StIdx stidx : GetMIRModule().GetSymbolSet()) {
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(stidx.Idx());
    MIRSymKind st = symbol->GetSKind();
    MIRStorageClass sc = symbol->GetStorageClass();
    if (!(st == kStVar && sc == kScGlobal)) {
      continue;
    }
    TyIdx typeIdx = symbol->GetTyIdx();
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx);
    PrimType pty = type->GetPrimType();
    if (!(pty == PTY_ptr || pty == PTY_ref)) {
      continue;
    }
    // It is a pointer/ref type.  Check its pointed type.
    if (pty == PTY_ptr) {
      if (type->GetKind() != kTypePointer) {
        continue;
      }
      auto *pointType = static_cast<MIRPtrType*>(type);
      MIRType *pointedType = pointType->GetPointedType();
      if (!(pointedType->GetKind() == kTypeClass)) {
        continue;
      }
    }
    // Now it is a pointer/ref to a class.  Record it for GC scanning.
    ASSERT(PTY_ptr < GlobalTables::GetTypeTable().GetTypeTable().size(), "index out of bound");
    MIRType &ptrType = *GlobalTables::GetTypeTable().GetTypeTable()[PTY_ptr];
    MIRConst *constNode = GetMIRModule().GetMemPool()->New<MIRAddrofConst>(symbol->GetStIdx(), 0, ptrType);
    newConst->AddItem(constNode, 0);
  }
  std::string gcRootsName = namemangler::kGcRootList;
  if (!newConst->GetConstVec().empty()) {
    MIRSymbol *gcRootsSt = builder->CreateSymbol(newConst->GetType().GetTypeIndex(), gcRootsName, kStVar,
                                                 kScAuto, nullptr, kScopeGlobal);
    arrayType->SetSizeArrayItem(0, newConst->GetConstVec().size());
    gcRootsSt->SetKonst(newConst);
  }
}

// should use a new pass to generate these information
void MUIDReplacement::GenerateCompilerVersionNum() {
  MIRType *ptrType = GlobalTables::GetTypeTable().GetVoidPtr();
  MIRArrayType &arrayType = *GlobalTables::GetTypeTable().GetOrCreateArrayType(*ptrType, 0);
  MIRAggConst *newConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), arrayType);
  MIRType &type = *GlobalTables::GetTypeTable().GetInt32();
  MIRConst *firstConst =
      GlobalTables::GetIntConstTable().GetOrCreateIntConst(Version::GetMajorVersion(), type);
  MIRConst *secondConst =
      GlobalTables::GetIntConstTable().GetOrCreateIntConst(Version::GetMinorVersion(), type);
  newConst->AddItem(firstConst, 0);
  newConst->AddItem(secondConst, 0);
  std::string symName = namemangler::kCompilerVersionNum + GetMIRModule().GetFileNameAsPostfix();
  MIRSymbol *versionNum = builder->CreateGlobalDecl(symName, arrayType);
  versionNum->SetKonst(newConst);
}

void MUIDReplacement::GenericSourceMuid() {
  if (Options::sourceMuid != "") {
    MIRArrayType &arrayType =
      *GlobalTables::GetTypeTable().GetOrCreateArrayType(*GlobalTables::GetTypeTable().GetUInt8(), 0);
    MIRAggConst *newConst = GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), arrayType);
    for (const char &c : Options::sourceMuid) {
      MIRConst *charConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
          c, *GlobalTables::GetTypeTable().GetUInt8());
      newConst->AddItem(charConst, 0);
    }

    MIRType &type = *GlobalTables::GetTypeTable().GetInt64();
    MIRConst *slotConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, type);
    newConst->AddItem(slotConst, 0);
    newConst->AddItem(slotConst, 0);
    std::string symName = namemangler::kSourceMuid + GetMIRModule().GetFileNameAsPostfix();
    MIRSymbol *sourceMuid = builder->CreateGlobalDecl(symName, arrayType);
    sourceMuid->SetKonst(newConst);
  }
}

void MUIDReplacement::GenCompilerMfileStatus() {
  MIRType &type = *GlobalTables::GetTypeTable().GetInt32();
  MIRConst *intConst =
      GlobalTables::GetIntConstTable().GetOrCreateIntConst(Options::gcOnly ? 1 : 0, type);
  std::string symName = namemangler::kCompilerMfileStatus + GetMIRModule().GetFileNameAsPostfix();
  MIRSymbol *mfileStatus = builder->CreateGlobalDecl(symName.c_str(), type);
  mfileStatus->SetKonst(intConst);
}

void MUIDReplacement::GenerateSourceInfo() {
  for (Klass *klass : klassHierarchy->GetTopoSortedKlasses()) {
    if (klass->IsClassIncomplete() || klass->IsInterfaceIncomplete()) {
      continue;
    }
    MIRStructType *structType = klass->GetMIRStructType();
    ASSERT(structType != nullptr, "null ptr check!");
    if (klass->GetMIRStructType()->IsLocal()) {
      continue;
    }
    for (const MIRPragma *prag : structType->GetPragmaVec()) {
      const MapleVector<MIRPragmaElement*> &elemVector = prag->GetElementVector();
      GStrIdx typeStrIdx = GlobalTables::GetTypeTable().GetTypeFromTyIdx(prag->GetTyIdx())->GetNameStrIdx();
      std::string typeName = GlobalTables::GetStrTable().GetStringFromStrIdx(typeStrIdx);
      if (typeName == "Lharmonyos_2Fannotation_2FInterpreter_3B") {
        if (prag->GetKind() == kPragmaClass) {
          int64 firstVal = elemVector[0]->GetI64Val();
          int64 secondVal = elemVector[1]->GetI64Val();
          std::string symbolName = CLASSINFO_PREFIX_STR + klass->GetKlassName();
          MUID muid = GetMUID(symbolName);
          sourceIndexMap[muid] = SourceIndexPair(firstVal, secondVal);
        } else if (prag->GetKind() == kPragmaFunc) {
          std::string funcName = GlobalTables::GetStrTable().GetStringFromStrIdx(prag->GetStrIdx());
          MUID muid = GetMUID(funcName);
          uint32 sourceFileIndex = static_cast<uint32>(elemVector[0]->GetI64Val());
          uint32 sourceClassIndex = static_cast<uint32>(elemVector[1]->GetI64Val());
          uint32 sourceMethodIndex = static_cast<uint32>(elemVector[2]->GetI64Val());
          bool isVirtual = elemVector[3]->GetI64Val() == 1 ? true : false;
          SourceFileMethod methodInf = {sourceFileIndex, sourceClassIndex, sourceMethodIndex, isVirtual};
          (void)sourceFileMethodMap.emplace(std::pair<MUID, SourceFileMethod>(muid, methodInf));
        } else if (prag->GetKind() == kPragmaField) {
          std::string fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(prag->GetStrIdx());
          MUID muid = GetMUID(fieldName);
          uint32 sourceFileIndex = static_cast<uint32>(elemVector[0]->GetI64Val());
          uint32 sourceClassIndex = static_cast<uint32>(elemVector[1]->GetI64Val());
          uint32 sourceFieldIndex = static_cast<uint32>(elemVector[2]->GetI64Val());
          SourceFileField fieldInf = {sourceFileIndex, sourceClassIndex, sourceFieldIndex};
          (void)sourceFileFieldMap.emplace(std::pair<MUID, SourceFileField>(muid, fieldInf));
        }
      }
    }
  }
}

void MUIDReplacement::ReleasePragmaMemPool() {
  for (Klass *klass : klassHierarchy->GetTopoSortedKlasses()) {
    MIRStructType *mirStruct = klass->GetMIRStructType();
    mirStruct->GetPragmaVec().clear();
    mirStruct->GetPragmaVec().shrink_to_fit();
  }
}

void MUIDReplacement::GenerateTables() {
  GenerateGlobalRootList();
  if (Options::partialAot) {
    GenerateSourceInfo();
  }
  if (Options::buildApp) {
    CollectFieldCallSite();
  }
  CollectFuncAndData();
  ReleasePragmaMemPool();
  CollectSuperClassArraySymbolData();
  CollectArrayClass();
  GenArrayClassCache();

  GenerateFuncDefTable();
  GenerateDataDefTable();
  GenerateUnifiedUndefTable();
  GenerateRangeTable();
  // When MapleLinker is enabled, MUIDReplacement becomes the last
  // phase that updates the reflection string table, thus the table
  // is emitted here.
  ReflectionAnalysis::GenStrTab(GetMIRModule());

  // Replace super class array
  for (auto superArraySymbol : superClassArraySymbolSet) {
    ReplaceDataTable(superArraySymbol->GetName());
  }
  // Replace undef entries in vtab/itab/reflectionMetaData
  for (Klass *klass : klassHierarchy->GetTopoSortedKlasses()) {
    ReplaceFieldTypeTable(namemangler::kFieldsInfoPrefixStr + klass->GetKlassName());
    if (Options::buildApp && !Options::genVtabAndItabForDecouple && klass->GetNeedDecoupling()) {
      ClearVtabItab(VTAB_PREFIX_STR + klass->GetKlassName());
      ClearVtabItab(ITAB_PREFIX_STR + klass->GetKlassName());
      ClearVtabItab(ITAB_CONFLICT_PREFIX_STR + klass->GetKlassName());
    } else {
      ReplaceFuncTable(VTAB_PREFIX_STR + klass->GetKlassName());
      ReplaceFuncTable(ITAB_PREFIX_STR + klass->GetKlassName());
      ReplaceFuncTable(ITAB_CONFLICT_PREFIX_STR + klass->GetKlassName());
    }
  }
  ReplaceDataTable(namemangler::kGcRootList);
  if (Options::buildApp) {
    // ReplaceVtableOffsetTable
    ReplaceDataTable(namemangler::kVtabOffsetTabStr + GetMIRModule().GetFileNameAsPostfix());
    // ReplaceFieldOffsetTable
    ReplaceDataTable(namemangler::kFieldOffsetTabStr + GetMIRModule().GetFileNameAsPostfix());
  }
  if (Options::decoupleStatic) {
    MIRSymbol *tabSym = GetSymbolFromName(namemangler::kDecoupleStaticKeyStr + GetMIRModule().GetFileNameAsPostfix());
    if (tabSym == nullptr) {
      return;
    }
    ReplaceDecoupleKeyTable(static_cast<MIRAggConst*>(tabSym->GetKonst()));
  }
  // Set the flag for decouple
  {
    MIRType &type = *GlobalTables::GetTypeTable().GetVoidPtr();
    uint64 value = 0;
    if (Options::buildApp) {
      value = Options::buildApp;
      if (Options::lazyBinding) {
        value = kDecoupleAndLazy;
      }
    }
    if (Options::decoupleStatic) {
      value |= 0x4;
    }
    MIRConst *newConst =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(value), type);
    std::string decoupleOption = namemangler::kDecoupleOption + GetMIRModule().GetFileNameAsPostfix();
    MIRSymbol *decoupleSt = builder->CreateGlobalDecl(decoupleOption, type);
    decoupleSt->SetKonst(newConst);
  }
  GenericSourceMuid();
  GenCompilerMfileStatus();
  GenerateCompilerVersionNum();
}

bool M2MMuidReplacement::PhaseRun(maple::MIRModule &m) {
  OPT_TEMPLATE_NEWPM(MUIDReplacement, m)
  return true;
}

void M2MMuidReplacement::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.SetPreservedAll();
}
}  // namespace maple
