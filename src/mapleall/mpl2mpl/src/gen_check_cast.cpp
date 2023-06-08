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
#include "gen_check_cast.h"
#include <iostream>
#include <algorithm>
#include "reflection_analysis.h"
#include "mir_lower.h"

namespace {
constexpr char kMCCReflectThrowCastException[] = "MCC_Reflect_ThrowCastException";
constexpr char kMCCReflectCheckCastingNoArray[] = "MCC_Reflect_Check_Casting_NoArray";
constexpr char kMCCReflectCheckCastingArray[] = "MCC_Reflect_Check_Casting_Array";
constexpr char kCastTargetClass[] = "castTargetClass";
constexpr char kObjectClassSym[] = "_objectClassSym";
constexpr char kMCCIsAssignableFrom[] = "MCC_IsAssignableFrom";
constexpr char kMCCThrowCastException[] = "MCC_ThrowCastException";
constexpr char kClinitBridge[] = "clinitbridge";
constexpr char kInstanceOfCacheFalse[] = "instanceOfCacheFalse";
constexpr char kCacheTrueClass[] = "cacheTrueClass";
constexpr char kCacheFalseClass[] = "cacheFalseClass";
constexpr char kTargetClass[] = "targetClass";
constexpr char kIsAssignableFromResult[] = "isAssignableFromResult";
} // namespace

// This phase does two things:
// #1 implement the opcode check-cast vx, type_id
//    according to the check-cast definition:
//    Checks whether the object reference in vx can be cast
//    to an instance of a class referenced by type_id.
//    Throws ClassCastException if the cast is not possible, continues execution otherwise.
//    in our case check if object can be cast or insert MCC_Reflect_ThrowCastException
//    before the stmt.
// #2 optimise instance-of && cast
namespace maple {
CheckCastGenerator::CheckCastGenerator(MIRModule &mod, KlassHierarchy *kh, bool dump)
    : FuncOptimizeImpl(mod, kh, dump) {
  InitTypes();
  InitFuncs();
}

PreCheckCast::PreCheckCast(MIRModule &mod, KlassHierarchy *kh, bool dump) : FuncOptimizeImpl(mod, kh, dump) {}

void CheckCastGenerator::InitTypes() {
  const MIRType *javaLangObjectType = WKTypes::Util::GetJavaLangObjectType();
  CHECK_FATAL(javaLangObjectType != nullptr, "The pointerObjType in InitTypes is null!");
  pointerObjType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*javaLangObjectType);
  classinfoType = GlobalTables::GetTypeTable().GetOrCreateClassType(namemangler::kClassMetadataTypeName,
                                                                    GetMIRModule());
  pointerClassMetaType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*classinfoType);
}

void CheckCastGenerator::InitFuncs() {
  throwCastException = builder->GetOrCreateFunction(kMCCReflectThrowCastException, TyIdx(PTY_void));
  throwCastException->SetAttr(FUNCATTR_nosideeffect);
  checkCastingNoArray = builder->GetOrCreateFunction(kMCCReflectCheckCastingNoArray, TyIdx(PTY_void));
  checkCastingNoArray->SetAttr(FUNCATTR_nosideeffect);
  checkCastingArray = builder->GetOrCreateFunction(kMCCReflectCheckCastingArray, TyIdx(PTY_void));
  checkCastingArray->SetAttr(FUNCATTR_nosideeffect);

  mccIsAssignableFrom =
      builder->GetOrCreateFunction(kMCCIsAssignableFrom, GlobalTables::GetTypeTable().GetUInt1()->GetTypeIndex());
  castExceptionFunc = builder->GetOrCreateFunction(kMCCThrowCastException, TyIdx(PTY_void));
  castExceptionFunc->SetAttr(FUNCATTR_nosideeffect);
}

MIRSymbol *CheckCastGenerator::GetOrCreateClassInfoSymbol(const std::string &className) const {
  std::string classInfoName = CLASSINFO_PREFIX_STR + className;
  builder->GlobalLock();
  MIRSymbol *classInfoSymbol = builder->GetGlobalDecl(classInfoName);
  if (classInfoSymbol == nullptr) {
    GStrIdx gStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(className);
    MIRType *classType =
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(GlobalTables::GetTypeNameTable().GetTyIdxFromGStrIdx(gStrIdx));
    MIRStorageClass sclass = (classType != nullptr && static_cast<MIRClassType*>(classType)->IsLocal()) ? kScGlobal
                                                                                                        : kScExtern;
    // Creating global symbol needs synchronization.
    classInfoSymbol = builder->CreateGlobalDecl(classInfoName, *GlobalTables::GetTypeTable().GetPtr(), sclass);
  }
  builder->GlobalUnlock();
  return classInfoSymbol;
}

void CheckCastGenerator::GenCheckCast(StmtNode &stmt) {
  // Handle the special case like (Type)null, we don't need a checkcast.
  if (stmt.GetOpCode() == OP_intrinsiccallwithtypeassigned) {
    auto *callNode = static_cast<IntrinsiccallNode*>(&stmt);
    ASSERT(callNode->GetNopndSize() == 1, "array size error");
    BaseNode *opnd = callNode->Opnd(0);
    if (opnd->GetOpCode() == OP_constval) {
      const size_t callNodeNretsSize = callNode->GetReturnVec().size();
      CHECK_FATAL(callNodeNretsSize > 0, "container check");
      CallReturnPair callReturnPair = callNode->GetReturnVec()[0];
      StmtNode *assignReturnTypeNode = nullptr;
      if (!callReturnPair.second.IsReg()) {
        assignReturnTypeNode =
            builder->CreateStmtDassign(callReturnPair.first, callReturnPair.second.GetFieldID(), opnd);
      } else {
        PregIdx pregIdx = callReturnPair.second.GetPregIdx();
        MIRPreg *mirPreg = currFunc->GetPregTab()->PregFromPregIdx(pregIdx);
        assignReturnTypeNode = builder->CreateStmtRegassign(mirPreg->GetPrimType(), pregIdx, opnd);
      }
      currFunc->GetBody()->ReplaceStmt1WithStmt2(&stmt, assignReturnTypeNode);
      return;
    }
  }
  // Do type check first.
  auto *callNode = static_cast<IntrinsiccallNode*>(&stmt);
  ASSERT(callNode->GetNopndSize() == 1, "array size error");
  TyIdx checkTyidx = callNode->GetTyIdx();
  MIRType *checkType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(checkTyidx);
  Klass *checkKlass = klassHierarchy->GetKlassFromTyIdx(static_cast<MIRPtrType*>(checkType)->GetPointedTyIdx());

  if (funcWithoutCastCheck.find(currFunc->GetName()) == funcWithoutCastCheck.end()) {
    if ((checkKlass != nullptr) && (checkKlass->GetKlassName() != "")) {
      if (checkKlass->GetKlassName() == namemangler::kJavaLangObjectStr) {
        const size_t callNodeNopndSize1 = callNode->GetNopndSize();
        CHECK_FATAL(callNodeNopndSize1 > 0, "container check");
        if (callNode->GetNopndAt(0)->GetPrimType() != PTY_ref && callNode->GetNopndAt(0)->GetPrimType() != PTY_ptr) {
          // If source = Ljava_2Flang_2FObject_3B, sub = primitive type then throw CastException.
          MIRSymbol *classSt = GetOrCreateClassInfoSymbol(checkKlass->GetKlassName());
          BaseNode *valueExpr = builder->CreateExprAddrof(0, *classSt);
          MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
          args.push_back(valueExpr);
          args.push_back(callNode->GetNopndAt(0));
          args.push_back(builder->CreateIntConst(0, PTY_ptr));
          StmtNode *dassignStmt = builder->CreateStmtCall(throwCastException->GetPuidx(), args);
          currFunc->GetBody()->InsertBefore(&stmt, dassignStmt);
        }
      } else {
        MIRSymbol *classSt = GetOrCreateClassInfoSymbol(checkKlass->GetKlassName());
        BaseNode *valueExpr = builder->CreateExprAddrof(0, *classSt);

        BaseNode *castClassReadNode = nullptr;
        StmtNode *castClassAssign = nullptr;
        BaseNode *opnd0 = callNode->GetNopndAt(0);
        if ((opnd0 != nullptr) && (opnd0->GetOpCode() == OP_regread)) {
          PregIdx castClassSymPregIdx = currFunc->GetPregTab()->CreatePreg(PTY_ref);
          castClassAssign = builder->CreateStmtRegassign(PTY_ref, castClassSymPregIdx, valueExpr);
          castClassReadNode = builder->CreateExprRegread(PTY_ref, castClassSymPregIdx);
        } else {
          MIRSymbol *castClassSym =
              builder->GetOrCreateLocalDecl(kCastTargetClass, *GlobalTables::GetTypeTable().GetRef());
          castClassAssign = builder->CreateStmtDassign(*castClassSym, 0, valueExpr);
          castClassReadNode = builder->CreateExprDread(*castClassSym);
        }
        BaseNode *nullPtrConst = builder->CreateIntConst(0, PTY_ptr);
        const size_t callNodeNopndSize2 = callNode->GetNopndSize();
        CHECK_FATAL(callNodeNopndSize2 > 0, "container check");
        BaseNode *cond =
            builder->CreateExprCompare(OP_ne, *GlobalTables::GetTypeTable().GetUInt1(),
                                       *GlobalTables::GetTypeTable().GetPtrType(), callNode->GetNopndAt(0),
                                       nullPtrConst);
        auto *ifStmt = static_cast<IfStmtNode*>(builder->CreateStmtIf(cond));
        MIRType *mVoidPtr = GlobalTables::GetTypeTable().GetVoidPtr();
        CHECK_FATAL(mVoidPtr != nullptr, "builder->GetVoidPtr() is null in CheckCastGenerator::GenCheckCast");
        BaseNode *opnd = callNode->GetNopndAt(0);
        BaseNode *ireadExpr = GetObjectShadow(opnd);
        BaseNode *innerCond = builder->CreateExprCompare(OP_ne, *GlobalTables::GetTypeTable().GetUInt1(),
                                                         *GlobalTables::GetTypeTable().GetPtrType(), castClassReadNode,
                                                         ireadExpr);
        auto *innerIfStmt = static_cast<IfStmtNode*>(builder->CreateStmtIf(innerCond));
        MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
        args.push_back(castClassReadNode);
        args.push_back(opnd);
        StmtNode *dassignStmt = builder->CreateStmtCall(checkCastingNoArray->GetPuidx(), args);
        innerIfStmt->GetThenPart()->AddStatement(dassignStmt);
        ifStmt->GetThenPart()->AddStatement(castClassAssign);
        ifStmt->GetThenPart()->AddStatement(innerIfStmt);
        currFunc->GetBody()->InsertBefore(&stmt, ifStmt);
      }
    } else {
      MIRType *pointedType =
          GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<MIRPtrType*>(checkType)->GetPointedTyIdx());
      if (pointedType->GetKind() == kTypeJArray) {
        // Java array.
        auto *jarrayType = static_cast<MIRJarrayType*>(pointedType);
        std::string arrayName = jarrayType->GetJavaName();
        uint64 dim = 0;
        while (arrayName[dim] == 'A') {
          ++dim;
        }
        MIRSymbol *elemClassSt = nullptr;
        std::string elementName = arrayName.substr(dim, arrayName.size() - dim);
        MIRType *mVoidPtr2 = GlobalTables::GetTypeTable().GetVoidPtr();
        CHECK_FATAL(mVoidPtr2 != nullptr, "null ptr check");
        if (elementName == "I" || elementName == "F" || elementName == "B" || elementName == "C" ||
            elementName == "S" || elementName == "J" || elementName == "D" || elementName == "Z" ||
            elementName == "V") {
          std::string primClassinfoName = PRIMITIVECLASSINFO_PREFIX_STR + elementName;
          builder->GlobalUnlock();
          elemClassSt = builder->GetGlobalDecl(primClassinfoName);
          if (elemClassSt == nullptr) {
            elemClassSt = builder->CreateGlobalDecl(primClassinfoName, *GlobalTables::GetTypeTable().GetPtr());
          }
          builder->GlobalUnlock();
        } else {
          elemClassSt = GetOrCreateClassInfoSymbol(elementName);
        }
        BaseNode *valueExpr = builder->CreateExprAddrof(0, *elemClassSt);
        builder->GlobalLock();
        builder->GlobalUnlock();
        MapleVector<BaseNode*> opnds(currFunc->GetCodeMempoolAllocator().Adapter());
        opnds.push_back(valueExpr);
        const size_t callNodeNopndSize3 = callNode->GetNopndSize();
        CHECK_FATAL(callNodeNopndSize3 > 0, "container check");
        opnds.push_back(callNode->GetNopndAt(0));
        opnds.push_back(builder->CreateIntConst(dim, PTY_ptr));
        StmtNode *dassignStmt = builder->CreateStmtCall(checkCastingArray->GetPuidx(), opnds);
        currFunc->GetBody()->InsertBefore(&stmt, dassignStmt);
      } else {
        MIRTypeKind kd = pointedType->GetKind();
        if (kd == kTypeStructIncomplete || kd == kTypeClassIncomplete || kd == kTypeInterfaceIncomplete) {
          LogInfo::MapleLogger() << "Warining: CheckCastGenerator::GenCheckCast "
                                 << GlobalTables::GetStrTable().GetStringFromStrIdx(pointedType->GetNameStrIdx())
                                 << " INCOMPLETE \n";
        } else {
          CHECK_FATAL(false, "unsupport kind");
        }
      }
    }
  }
  if (callNode->GetOpCode() == OP_intrinsiccallwithtype) {
    return;
  }
  BaseNode *opnd = callNode->Opnd(0);
  ASSERT(opnd->GetOpCode() == OP_dread || opnd->GetOpCode() == OP_regread || opnd->GetOpCode() == OP_iread ||
         opnd->GetOpCode() == OP_retype, "unknown calltype! check it!");
  MIRType *fromType = nullptr;
  if (opnd->GetOpCode() == OP_dread) {
    fromType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
        currFunc->GetLocalOrGlobalSymbol(static_cast<AddrofNode*>(opnd)->GetStIdx())->GetTyIdx());
  } else if (opnd->GetOpCode() == OP_retype) {
    fromType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<RetypeNode*>(opnd)->GetTyIdx());
  } else if (opnd->GetOpCode() == OP_iread) {
    auto *irnode = static_cast<IreadNode*>(opnd);
    auto *ptrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(irnode->GetTyIdx()));
    if (irnode->GetFieldID() != 0) {
      MIRType *pointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrType->GetPointedTyIdx());
      MIRStructType *structType = nullptr;
      if (pointedType->GetKind() != kTypeJArray) {
        structType = static_cast<MIRStructType*>(pointedType);
      } else {
        // It's a Jarray type. Using it's parent's field info: java.lang.Object.
        structType = static_cast<MIRJarrayType*>(pointedType)->GetParentType();
      }
      CHECK_FATAL(structType != nullptr, "null ptr check");
      fromType = structType->GetFieldType(irnode->GetFieldID());
    } else {
      fromType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrType->GetPointedTyIdx());
    }
  } else if (opnd->GetOpCode() == OP_regread) {
    auto *regReadNode = static_cast<RegreadNode*>(opnd);
    MIRPreg *mirPreg = currFunc->GetPregTab()->PregFromPregIdx(regReadNode->GetRegIdx());
    CHECK_FATAL(mirPreg->GetPrimType() == PTY_ref || mirPreg->GetPrimType() == PTY_ptr,
                "must be reference or ptr type for preg");
    if (opnd->GetPrimType() == PTY_ref) {
      fromType = mirPreg->GetMIRType();
    } else {
      const size_t gTypeTableSize = GlobalTables::GetTypeTable().GetTypeTable().size();
      CHECK_FATAL(gTypeTableSize > PTY_ptr, "container check");
      fromType = GlobalTables::GetTypeTable().GetTypeTable()[PTY_ptr];
    }
  }
  CHECK_FATAL((fromType->GetPrimType() == maple::PTY_ptr || fromType->GetPrimType() == maple::PTY_ref),
              "unknown fromType! check it!");
  CHECK_FATAL(GlobalTables::GetTypeTable().GetTypeFromTyIdx(callNode->GetTyIdx())->GetPrimType() == maple::PTY_ptr ||
              GlobalTables::GetTypeTable().GetTypeFromTyIdx(callNode->GetTyIdx())->GetPrimType() == maple::PTY_ref,
              "unknown fromType! check it!");
  CHECK_FATAL(!callNode->GetReturnVec().empty(), "container check");
  CallReturnPair callReturnPair = callNode->GetReturnVec()[0];
  StmtNode *assignReturnTypeNode = nullptr;
  if (!callReturnPair.second.IsReg()) {
    assignReturnTypeNode = builder->CreateStmtDassign(callReturnPair.first, callReturnPair.second.GetFieldID(), opnd);
  } else {
    PregIdx pregIdx = callReturnPair.second.GetPregIdx();
    MIRPreg *mirPreg = currFunc->GetPregTab()->PregFromPregIdx(pregIdx);
    assignReturnTypeNode = builder->CreateStmtRegassign(mirPreg->GetPrimType(), pregIdx, opnd);
  }
  currFunc->GetBody()->ReplaceStmt1WithStmt2(&stmt, assignReturnTypeNode);
}

void CheckCastGenerator::GenAllCheckCast() {
  auto &stmtNodes = currFunc->GetBody()->GetStmtNodes();
  for (auto &stmt : stmtNodes) {
    if (stmt.GetOpCode() == OP_intrinsiccallwithtypeassigned || stmt.GetOpCode() == OP_intrinsiccallwithtype) {
      auto &callNode = static_cast<IntrinsiccallNode&>(stmt);
      if (callNode.GetIntrinsic() == INTRN_JAVA_CHECK_CAST) {
        GenCheckCast(stmt);
        if (stmt.GetOpCode() == OP_intrinsiccallwithtype) {
          currFunc->GetBody()->RemoveStmt(&stmt);
        }
      }
    }
  }
}

BaseNode *CheckCastGenerator::GetObjectShadow(BaseNode *opnd) const {
  FieldID fieldID = builder->GetStructFieldIDFromFieldNameParentFirst(WKTypes::Util::GetJavaLangObjectType(),
                                                                      namemangler::kShadowClassName);
  BaseNode *ireadExpr =
      builder->CreateExprIread(*GlobalTables::GetTypeTable().GetPtr(), *pointerObjType, fieldID, opnd);
  return ireadExpr;
}

// optimise instance-of && cast
// #1 convert instance-of, cast to check IsAssignableFrom
//   1) check nullptr
//   2) check type, call IsAssignableFrom(targetClass, objClass)
// eg:
//   (1). cast
//   intrinsiccallwithtypeassigned <* <$targetClass>> JAVA_CHECK_CAST (dread ref %Reg2_R45718) { dassign %Reg2_R45709 0}
//   ==>
//   ## check nullptr
//   Example: if (ne u1 ptr (dread ref %Reg2_R45718, constval ptr 0)) {
//     dassign %objClass 0 (iread ptr <* <$Ljava_2Flang_2FObject_3B>> 1 (dread ref %Reg2_R45718))
//     dassign %targetClass 0 (intrinsicopwithtype ref <* <$targetClass>> JAVA_CONST_CLASS ())
//   ## check type
//     dassign %isAssignableFromResult 0
//        (intrinsicopwithtype ref <* <$targetClass>> JAVA_ISASSIGNABLEFROM (dread ptr %objClass))
//     Example: if (eq u1 ptr (dread u1 %isAssignableFromResult, constval u1 0)) {
//       call &MCC_ThrowCastException (dread ptr %targetClass, dread ref %Reg2_R45718)
//     }
//   }
//   dassign %Reg2_R45709 0 (dread ref %Reg2_R45718)
//   (2) instance-of
//   dassign %Reg0_Z 0 (intrinsicopwithtype u1 <* <$targetClass>> JAVA_INSTANCE_OF (dread ref %Reg0_R45718))
//   ==>
//   ## check nullptr
//   Example: if (ne u1 ptr (dread ref %Reg0_R45718, constval ptr 0)) {
//     dassign %objClass 0 (iread ptr <* <$Ljava_2Flang_2FObject_3B>> 1 (dread ref %Reg0_R45718))
//     dassign %Reg0_Z 0 (intrinsicopwithtype ref <* <$targetClass>> JAVA_ISASSIGNABLEFROM (dread ptr %objClass))
//   Example: } else {
//     dassign %Reg0_Z 0 (constval u1 0)
//   }
// #2 optimise type check(IsAssignableFrom(targetClass, objClass))
//     1) slow path, invoke runtime api to check
//     2) fast path, use cache to check type first, if fail, then go to slow path
//     fast path implement:
//       (1) target class is is final class or is private and inner class and has no subclass
//           it means there can't be subclass of target class,so if  a obj is instance of target
//           class , the obj must be referred the target class,replace the instance-of with
//           maple IR,the IR do the things check if obj.getClass() == target_class,below is the detail
//           suppose the obj is %obj, target-class is T,result is saved in reg %1
//           regassign u1 %1 (constval u1 0)
//           brfalse (ne u1 ptr (regread ref %obj, constval ptr 0))  #check if obj is null
//           #check if obj's class is equal the target class ,if equal set the result 1
//           brflase (eq u1 ptr(
//             iread ptr <* <$Ljava_2Flang_2FObject_3B>> 1 (regread ref %obj),
//             addrof ptr T))
//           regassign u1 %1 (constval u1 1)
//       (2) check cache
//         a. check objClass equal to targetClass, if fail, then
//         b. check objClass.cacheTrueClass equal to targetClass, if fail, then
//         c. check objClass.cacheTFalseClass equal to targetClass, if fail, then
//         b. go to slow path
// eg
//   dassign %Reg0_Z 0 (intrinsicopwithtype ref <* <$targetClass>> JAVA_ISASSIGNABLEFROM (dread ptr %objClass))
//   ==>
//   ##set result true first
//   dassign %Reg0_Z 0 (constval u1 1)
//   Example: if (ne u1 ptr (dread ptr %objClass, dread ptr %targetClass)) {
//     // load cacheTrueClass, it stored in runtime
//     dassign %cacheTrueClass 0 (iread u64 <* <$__class_meta__>> 11 (dread ptr %objClass))
//     Example: if (ne u1 ptr (dread ptr %targetClass, dread u64 %cacheTrueClass)) {
//       ##load cacheFalseClass, it stored in runtime
//       dassign %cacheFalseClass 0 (iread u64 <* <$__class_meta__>> 10 (dread ptr %objClass))
//       Example: if (eq u1 ptr (
//         dread ptr %targetClass,
//         lshr u64 (dread u64 %cacheFalseClass, constval u8 32))) {
//         dassign %Reg0_Z 0 (constval u1 0)
//       Example: } else {
//         ##check cache fail, got to slow path, check in runtime
//         callassigned &MCC_IsAssignableFrom (dread ptr %objClass, dread ptr %targetClass) { dassign %Reg0_Z 0 }
//       }
//     }
//   }
void CheckCastGenerator::AssignedCastValue(StmtNode &stmt) const {
  if (stmt.GetOpCode() == OP_intrinsiccallwithtype) {
    return;
  }
  // Assigned
  auto *assignedCallNode = static_cast<IntrinsiccallNode*>(&stmt);
  MapleVector<BaseNode*> nopnd = assignedCallNode->GetNopnd();
  BaseNode *opnd = nopnd[0];
  ASSERT(!assignedCallNode->GetReturnVec().empty(), "container check");
  CallReturnPair callReturnPair = assignedCallNode->GetCallReturnPair(0);
  StmtNode *assignReturnTypeNode = nullptr;
  if (!callReturnPair.second.IsReg()) {
    assignReturnTypeNode = builder->CreateStmtDassign(callReturnPair.first, callReturnPair.second.GetFieldID(), opnd);
  } else {
    PregIdx pregIdx = callReturnPair.second.GetPregIdx();
    MIRPreg *mirpreg = currFunc->GetPregTab()->PregFromPregIdx(pregIdx);
    assignReturnTypeNode = builder->CreateStmtRegassign(mirpreg->GetPrimType(), pregIdx, opnd);
  }
  currFunc->GetBody()->InsertAfter(&stmt, assignReturnTypeNode);
}

void CheckCastGenerator::ConvertCheckCastToIsAssignableFrom(StmtNode &stmt) {
  AssignedCastValue(stmt);

  auto *callNode = static_cast<IntrinsiccallNode*>(&stmt);
  MIRType *checkType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(callNode->GetTyIdx());
  auto *ptrCheckType = static_cast<MIRPtrType*>(checkType);
  MIRType *ptype = ptrCheckType->GetPointedType();
  MIRTypeKind kd = ptype->GetKind();
  if (kd == kTypeClass || kd == kTypeInterface || kd == kTypeJArray) {
    if (funcWithoutCastCheck.find(currFunc->GetName()) != funcWithoutCastCheck.end()) {
      currFunc->GetBody()->RemoveStmt(&stmt);
      return;
    }

    StmtNode *objectClassAssign = nullptr;
    StmtNode *isAssignableFromResultAssign = nullptr;
    BaseNode *objectClassReadNode = nullptr;
    BaseNode *checkTypeReadNode = nullptr;
    StmtNode *checkTypeAssign = nullptr;
    BaseNode *isAssignableFromResultReadNode = nullptr;
    BaseNode *isAssignableFromNode = nullptr;
    MapleVector<BaseNode*> nopnd = callNode->GetNopnd();
    BaseNode *opnd = nopnd[0];
    BaseNode *ireadShadowExpr = GetObjectShadow(opnd);

    MapleVector<BaseNode*> arguments(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
    BaseNode *checkTypeNode = builder->CreateExprIntrinsicop(INTRN_JAVA_CONST_CLASS, OP_intrinsicopwithtype,
                                                             *checkType, arguments);
    ConstvalNode *falseVal = builder->GetConstUInt1(false);
    BaseNode *nullVal = builder->CreateIntConst(0, PTY_ptr);
    if (opnd->op == OP_regread) {
      PrimType classPrimType =  ireadShadowExpr->GetPrimType();
      PregIdx objectClassSymPregIdx = currFunc->GetPregTab()->CreatePreg(classPrimType);
      objectClassAssign = builder->CreateStmtRegassign(classPrimType, objectClassSymPregIdx, ireadShadowExpr);
      objectClassReadNode = builder->CreateExprRegread(PTY_ptr, objectClassSymPregIdx);

      PregIdx checkTypePregIdx = currFunc->GetPregTab()->CreatePreg(classPrimType);
      checkTypeAssign = builder->CreateStmtRegassign(classPrimType, checkTypePregIdx, checkTypeNode);
      checkTypeReadNode = builder->CreateExprRegread(PTY_ptr, checkTypePregIdx);

      MapleVector<BaseNode*> opndArgs(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
      opndArgs.push_back(objectClassReadNode);
      isAssignableFromNode =
          builder->CreateExprIntrinsicop(INTRN_JAVA_ISASSIGNABLEFROM, OP_intrinsicopwithtype, *checkType, opndArgs);
      PregIdx isAssignableFromResultIdx = currFunc->GetPregTab()->CreatePreg(PTY_u1);
      isAssignableFromResultAssign =
          builder->CreateStmtRegassign(PTY_u1, isAssignableFromResultIdx, isAssignableFromNode);
      isAssignableFromResultReadNode = builder->CreateExprRegread(PTY_u1, isAssignableFromResultIdx);
    } else {
      MIRSymbol *objectClassSym =
          builder->GetOrCreateLocalDecl(kObjectClassSym, *GlobalTables::GetTypeTable().GetPtr());
      objectClassAssign = builder->CreateStmtDassign(*objectClassSym, 0, ireadShadowExpr);
      objectClassReadNode = builder->CreateExprDread(*objectClassSym);

      MIRSymbol *checkTypeSym = builder->GetOrCreateLocalDecl(kTargetClass, *GlobalTables::GetTypeTable().GetPtr());
      checkTypeAssign = builder->CreateStmtDassign(*checkTypeSym, 0, checkTypeNode);
      checkTypeReadNode = builder->CreateExprDread(*checkTypeSym);
      MapleVector<BaseNode*> opndArgs(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
      opndArgs.push_back(objectClassReadNode);
      isAssignableFromNode =
          builder->CreateExprIntrinsicop(INTRN_JAVA_ISASSIGNABLEFROM, OP_intrinsicopwithtype, *checkType, opndArgs);
      MIRSymbol *isAssignableFromResultSym =
          builder->GetOrCreateLocalDecl(kIsAssignableFromResult, *GlobalTables::GetTypeTable().GetUInt1());
      isAssignableFromResultAssign = builder->CreateStmtDassign(*isAssignableFromResultSym, 0, isAssignableFromNode);
      isAssignableFromResultReadNode = builder->CreateExprDread(*isAssignableFromResultSym);
    }

    BaseNode *condZero = builder->CreateExprCompare(
        OP_ne, *GlobalTables::GetTypeTable().GetUInt1(), *GlobalTables::GetTypeTable().GetPtrType(), opnd, nullVal);
    IfStmtNode *ifObjZeroNode = builder->CreateStmtIf(condZero);

    BaseNode *condIsAssignableFrom = builder->CreateExprCompare(OP_eq, *GlobalTables::GetTypeTable().GetUInt1(),
                                                                *GlobalTables::GetTypeTable().GetPtrType(),
                                                                isAssignableFromResultReadNode, falseVal);
    IfStmtNode *ifIsAssignableFromNode = builder->CreateStmtIf(condIsAssignableFrom);

    MapleVector<BaseNode*> throwArguments(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
    throwArguments.push_back(checkTypeReadNode);
    throwArguments.push_back(opnd);
    StmtNode *callThrowCastException = builder->CreateStmtCall(castExceptionFunc->GetPuidx(), throwArguments);
    ifIsAssignableFromNode->GetThenPart()->AddStatement(callThrowCastException);

    ifObjZeroNode->GetThenPart()->AddStatement(objectClassAssign);
    ifObjZeroNode->GetThenPart()->AddStatement(checkTypeAssign);
    ifObjZeroNode->GetThenPart()->AddStatement(isAssignableFromResultAssign);
    ifObjZeroNode->GetThenPart()->AddStatement(ifIsAssignableFromNode);

    currFunc->GetBody()->ReplaceStmt1WithStmt2(&stmt, ifObjZeroNode);
  } else if (kd == kTypeStructIncomplete || kd == kTypeClassIncomplete || kd == kTypeInterfaceIncomplete) {
    std::cout << "Warining: CheckCastGenerator::GenCheckCast " <<
        GlobalTables::GetStrTable().GetStringFromStrIdx(ptype->GetNameStrIdx()) <<
        " INCOMPLETE \n";
    currFunc->GetBody()->RemoveStmt(&stmt);
  } else {
    ASSERT(false, "unsupport kind");
  }
}

void CheckCastGenerator::GenAllCheckCast(bool isHotFunc) {
  auto &stmtNodes = currFunc->GetBody()->GetStmtNodes();
  for (auto &stmt : stmtNodes) {
    if (stmt.GetOpCode() == OP_intrinsiccallwithtypeassigned || stmt.GetOpCode() == OP_intrinsiccallwithtype) {
      auto &callNode = static_cast<IntrinsiccallNode&>(stmt);
      if (callNode.GetIntrinsic() == INTRN_JAVA_CHECK_CAST) {
        if (isHotFunc) {
          ConvertCheckCastToIsAssignableFrom(stmt);
        } else {
          GenCheckCast(stmt);
          if (stmt.GetOpCode() == OP_intrinsiccallwithtype) {
            currFunc->GetBody()->RemoveStmt(&stmt);
          }
        }
      }
    }
  }
}

// Use "srcclass == targetclass" replace instanceof if target class is final.
void CheckCastGenerator::ReplaceNoSubClassIsAssignableFrom(BlockNode &blockNode, StmtNode &stmt,
                                                           const MIRPtrType &ptrType,
                                                           const IntrinsicopNode &intrinsicNode) const {
  MapleVector<BaseNode*> nopnd = intrinsicNode.GetNopnd();
  BaseNode *subClassNode = nopnd[0];
  MIRClassType &targetClassType = static_cast<MIRClassType&>(*ptrType.GetPointedType());
  const std::string &className = GlobalTables::GetStrTable().GetStringFromStrIdx(targetClassType.GetNameStrIdx());
  PregIdx targetClassSymPregIdx = 0;
  MIRSymbol *targetClassSym = nullptr;
  BaseNode *targetClassNode = nullptr;
  // this pattern:
  //   regassign ptr %6 (intrinsicopwithtype ref <* <$Ljava_lang_String_3B>> JAVA_CONST_CLASS ())
  //   regassign u1 %2 (intrinsicopwithtype ref <* <$Ljava_2Flang_2FString_3B>> JAVA_ISASSIGNABLEFROM (regread ptr %1))
  // for intrinsic JAVA_ISASSIGNABLEFROM, we can reuse JAVA_CONST_CLASS result(%6),
  // rathe than define symbol again(CreateExprAddrof).
  bool isPreDefinedConstClass = IsDefinedConstClass(stmt, ptrType, targetClassSymPregIdx, targetClassSym);
  if (!isPreDefinedConstClass) {
    MIRSymbol *targetClassSy = GetOrCreateClassInfoSymbol(className);
    targetClassNode = builder->CreateExprAddrof(0, *targetClassSy);
  } else {
    if (targetClassSym == nullptr) {
      targetClassNode = builder->CreateExprRegread(PTY_ref, targetClassSymPregIdx);
    } else {
      targetClassNode = builder->CreateExprDread(*targetClassSym);
    }
  }
  BaseNode *innerCond = builder->CreateExprCompare(OP_eq, *GlobalTables::GetTypeTable().GetUInt1(),
                                                   *GlobalTables::GetTypeTable().GetPtrType(),
                                                   subClassNode, targetClassNode);
  auto *innerIfStmt = static_cast<IfStmtNode*>(builder->CreateStmtIfThenElse(innerCond));

  StmtNode *retFalseNode = nullptr;
  StmtNode *retTrueNode = nullptr;
  ConstvalNode *falseVal = builder->GetConstUInt1(false);
  ConstvalNode *trueVal = builder->GetConstUInt1(true);
  if (stmt.op == OP_regassign) {
    auto *regAssignNode = static_cast<RegassignNode*>(&stmt);
    MIRPreg *mirPreg = currFunc->GetPregTab()->PregFromPregIdx(regAssignNode->GetRegIdx());
    retFalseNode = builder->CreateStmtRegassign(mirPreg->GetPrimType(), regAssignNode->GetRegIdx(), falseVal);
    retTrueNode = builder->CreateStmtRegassign(mirPreg->GetPrimType(), regAssignNode->GetRegIdx(), trueVal);
  } else {
    auto *dAssignNode = static_cast<DassignNode*>(&stmt);
    MIRSymbol *instanceOfRet = currFunc->GetLocalOrGlobalSymbol(dAssignNode->GetStIdx());
    retFalseNode = builder->CreateStmtDassign(*instanceOfRet, 0, falseVal);
    retTrueNode = builder->CreateStmtDassign(*instanceOfRet, 0, trueVal);
  }
  innerIfStmt->GetThenPart()->AddStatement(retTrueNode);
  innerIfStmt->GetElsePart()->AddStatement(retFalseNode);
  blockNode.ReplaceStmt1WithStmt2(&stmt, innerIfStmt);
}

bool CheckCastGenerator::IsDefinedConstClass(const StmtNode &stmt, const MIRPtrType &targetClassType,
                                             PregIdx &classSymPregIdx, MIRSymbol *&classSym) const {
  StmtNode *stmtPre = stmt.GetPrev();
  Opcode opPre = stmtPre->GetOpCode();
  if ((opPre != OP_dassign) && (opPre != OP_regassign)) {
    return false;
  }

  auto *unode = static_cast<UnaryStmtNode*>(stmtPre);
  ASSERT(unode->GetRHS() != nullptr, "null ptr check!");
  if (unode->GetRHS()->GetOpCode() != OP_intrinsicopwithtype) {
    return false;
  }
  auto *preIntrinsicNode = static_cast<IntrinsicopNode*>(unode->GetRHS());
  if (preIntrinsicNode->GetIntrinsic() != INTRN_JAVA_CONST_CLASS) {
    return false;
  }

  MIRType *classType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(preIntrinsicNode->GetTyIdx());
  if (!classType->EqualTo(targetClassType)) {
    return false;
  }

  if (opPre == OP_regassign) {
    auto *regAssignNodePre = static_cast<RegassignNode*>(stmtPre);
    classSymPregIdx = regAssignNodePre->GetRegIdx();
  } else {
    auto *dAssignNode = static_cast<DassignNode*>(stmtPre);
    classSym = currFunc->GetLocalOrGlobalSymbol(dAssignNode->GetStIdx());
  }
  return true;
}

// inline check cache, it implements __MRT_IsAssignableFromCheckCache
void CheckCastGenerator::ReplaceIsAssignableFromUsingCache(BlockNode &blockNode, StmtNode &stmt,
                                                           const MIRPtrType &targetClassType,
                                                           const IntrinsicopNode &intrinsicNode) const {
  StmtNode *resultFalse = nullptr;
  StmtNode *resultTrue = nullptr;
  StmtNode *cacheFalseClassesAssign = nullptr;
  StmtNode *cacheTrueClassesAssign = nullptr;
  StmtNode *targetClassAssign = nullptr;
  BaseNode *targetClassReadNode = nullptr;
  BaseNode *cacheFalseClassesReadNode = nullptr;
  BaseNode *cacheTrueClassesReadNode = nullptr;
  BaseNode *cacheFalseClasses = nullptr;
  BaseNode *cacheTrueClasses = nullptr;
  StmtNode *callMCCIsAssignableFromStmt = nullptr;
  PregIdx targetClassSymPregIdx = 0;
  MIRSymbol *targetClassSym = nullptr;
  bool isDefinedConstClass = IsDefinedConstClass(stmt, targetClassType, targetClassSymPregIdx, targetClassSym);

  MapleVector<BaseNode*> nopnd = intrinsicNode.GetNopnd();
  BaseNode *objectClassNode = nopnd[0];
  MapleVector<BaseNode*> arguments(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
  BaseNode *targetClassNode =
      builder->CreateExprIntrinsicop(INTRN_JAVA_CONST_CLASS, OP_intrinsicopwithtype, targetClassType, arguments);
  FieldID superCacheTrueFldid = builder->GetStructFieldIDFromFieldName(*classinfoType, kClinitBridge);
  FieldID superCacheFalseFldid = builder->GetStructFieldIDFromFieldName(*classinfoType, kInstanceOfCacheFalse);

  ConstvalNode *falseVal = builder->GetConstUInt1(false);
  ConstvalNode *trueVal = builder->GetConstUInt1(true);
  if (stmt.op == OP_regassign) {
    auto *regAssignNode = static_cast<RegassignNode*>(&stmt);
    PrimType classPrimType = objectClassNode->GetPrimType();
    MIRPreg *mirPreg = currFunc->GetPregTab()->PregFromPregIdx(regAssignNode->GetRegIdx());
    resultFalse = builder->CreateStmtRegassign(mirPreg->GetPrimType(), regAssignNode->GetRegIdx(), falseVal);
    resultTrue = builder->CreateStmtRegassign(mirPreg->GetPrimType(), regAssignNode->GetRegIdx(), trueVal);

    if (!isDefinedConstClass) {
      targetClassSymPregIdx = currFunc->GetPregTab()->CreatePreg(classPrimType);
      targetClassAssign = builder->CreateStmtRegassign(classPrimType, targetClassSymPregIdx, targetClassNode);
    }
    targetClassReadNode = builder->CreateExprRegread(PTY_ptr, targetClassSymPregIdx);
    cacheTrueClasses = builder->CreateExprIread(*GlobalTables::GetTypeTable().GetPtr(), *pointerClassMetaType,
                                                superCacheTrueFldid, objectClassNode);
    cacheFalseClasses = builder->CreateExprIread(*GlobalTables::GetTypeTable().GetPtr(), *pointerClassMetaType,
                                                 superCacheFalseFldid, objectClassNode);

    // cache true class
    PregIdx cacheTrueClassSymPregIdx = currFunc->GetPregTab()->CreatePreg(classPrimType);
    cacheTrueClassesAssign = builder->CreateStmtRegassign(classPrimType, cacheTrueClassSymPregIdx, cacheTrueClasses);
    cacheTrueClassesReadNode = builder->CreateExprRegread(PTY_ptr, cacheTrueClassSymPregIdx);
    // cache false class
    PregIdx cacheFalseClassSymPregIdx = currFunc->GetPregTab()->CreatePreg(classPrimType);
    cacheFalseClassesAssign =
        builder->CreateStmtRegassign(classPrimType, cacheFalseClassSymPregIdx, cacheFalseClasses);
    cacheFalseClassesReadNode = builder->CreateExprRegread(PTY_ptr, cacheFalseClassSymPregIdx);

    MapleVector<BaseNode*> opnds(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
    opnds.push_back(objectClassNode);
    opnds.push_back(targetClassReadNode);
    callMCCIsAssignableFromStmt = builder->CreateStmtCallRegassigned(mccIsAssignableFrom->GetPuidx(), opnds,
                                                                     regAssignNode->GetRegIdx(), OP_callassigned);
  } else {
    MIRSymbol *cacheTrueClassSym =
        builder->GetOrCreateLocalDecl(kCacheTrueClass, *GlobalTables::GetTypeTable().GetPtr());
    MIRSymbol *cacheFalseClassSym =
        builder->GetOrCreateLocalDecl(kCacheFalseClass, *GlobalTables::GetTypeTable().GetPtr());
    auto *dAssignNode = static_cast<DassignNode*>(&stmt);
    MIRSymbol *instanceOfRet = currFunc->GetLocalOrGlobalSymbol(dAssignNode->GetStIdx());
    resultFalse = builder->CreateStmtDassign(*instanceOfRet, 0, falseVal);
    resultTrue = builder->CreateStmtDassign(*instanceOfRet, 0, trueVal);

    if (!isDefinedConstClass) {
      targetClassSym = builder->GetOrCreateLocalDecl(kTargetClass, *GlobalTables::GetTypeTable().GetUIntType());
      targetClassAssign = builder->CreateStmtDassign(*targetClassSym, 0, targetClassNode);
    }
    ASSERT(targetClassSym != nullptr, "null ptr check!");
    targetClassReadNode = builder->CreateExprDread(*targetClassSym);
    cacheTrueClasses = builder->CreateExprIread(*GlobalTables::GetTypeTable().GetPtr(), *pointerClassMetaType,
                                                superCacheTrueFldid, objectClassNode);
    cacheFalseClasses = builder->CreateExprIread(*GlobalTables::GetTypeTable().GetPtr(), *pointerClassMetaType,
                                                 superCacheFalseFldid, objectClassNode);
    // cache true class
    cacheTrueClassesAssign = builder->CreateStmtDassign(*cacheTrueClassSym, 0, cacheTrueClasses);
    cacheTrueClassesReadNode = builder->CreateExprDread(*cacheTrueClassSym);

    // cache false class
    cacheFalseClassesAssign = builder->CreateStmtDassign(*cacheFalseClassSym, 0, cacheFalseClasses);
    cacheFalseClassesReadNode = builder->CreateExprDread(*cacheFalseClassSym);

    MapleVector<BaseNode*> opnds(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
    opnds.push_back(objectClassNode);
    opnds.push_back(targetClassReadNode);
    callMCCIsAssignableFromStmt =
        builder->CreateStmtCallAssigned(mccIsAssignableFrom->GetPuidx(), opnds, instanceOfRet, OP_callassigned);
  }

  BaseNode *classEqualCond = builder->CreateExprCompare(OP_ne, *GlobalTables::GetTypeTable().GetUInt1(),
                                                        *GlobalTables::GetTypeTable().GetPtrType(),
                                                        objectClassNode, targetClassReadNode);
  auto *classEqualCondIfStmt = static_cast<IfStmtNode*>(builder->CreateStmtIf(classEqualCond));
  classEqualCondIfStmt->GetThenPart()->AddStatement(cacheTrueClassesAssign);

  BaseNode *classCacheTrueEqualCond = builder->CreateExprCompare(OP_ne, *GlobalTables::GetTypeTable().GetUInt1(),
                                                                 *GlobalTables::GetTypeTable().GetPtrType(),
                                                                 targetClassReadNode, cacheTrueClassesReadNode);
  auto *classCacheTrueEqualCondIfStmt = static_cast<IfStmtNode*>(builder->CreateStmtIf(classCacheTrueEqualCond));
  classEqualCondIfStmt->GetThenPart()->AddStatement(classCacheTrueEqualCondIfStmt);

  BaseNode *classCacheFalseEqualCond = builder->CreateExprCompare(OP_eq, *GlobalTables::GetTypeTable().GetUInt1(),
                                                                  *GlobalTables::GetTypeTable().GetPtrType(),
                                                                  targetClassReadNode, cacheFalseClassesReadNode);
  auto *classCacheFalseEqualCondIfStmt =
      static_cast<IfStmtNode*>(builder->CreateStmtIfThenElse(classCacheFalseEqualCond));
  classCacheFalseEqualCondIfStmt->GetThenPart()->AddStatement(resultFalse);
  classCacheFalseEqualCondIfStmt->GetElsePart()->AddStatement(callMCCIsAssignableFromStmt);

  classCacheTrueEqualCondIfStmt->GetThenPart()->AddStatement(cacheFalseClassesAssign);
  classCacheTrueEqualCondIfStmt->GetThenPart()->AddStatement(classCacheFalseEqualCondIfStmt);

  blockNode.InsertBefore(&stmt, resultTrue);
  if (!isDefinedConstClass) {
    blockNode.InsertBefore(&stmt, targetClassAssign);
  }
  blockNode.ReplaceStmt1WithStmt2(&stmt, classEqualCondIfStmt);
}

void CheckCastGenerator::CheckIsAssignableFrom(BlockNode &blockNode, StmtNode &stmt,
                                               const IntrinsicopNode &intrinsicNode) const {
  MIRType *targetClassType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(intrinsicNode.GetTyIdx());
  auto *ptrType = static_cast<MIRPtrType*>(targetClassType);
  MIRType *ptype = ptrType->GetPointedType();
  MIRTypeKind kd = ptype->GetKind();
  if (Options::buildApp == 0 && kd == kTypeClass) {
    auto *classType = static_cast<MIRClassType*>(ptype);
    Klass *clazz = klassHierarchy->GetKlassFromTyIdx(classType->GetTypeIndex());
    if (classType->IsFinal() || (clazz && clazz->IsPrivateInnerAndNoSubClass())) {
      ReplaceNoSubClassIsAssignableFrom(blockNode, stmt, *ptrType, intrinsicNode);
      return;
    }
  }
  ReplaceIsAssignableFromUsingCache(blockNode, stmt, *ptrType, intrinsicNode);
}

void CheckCastGenerator::ConvertInstanceofToIsAssignableFrom(StmtNode &stmt,
                                                             const IntrinsicopNode &intrinsicNode) const {
  MIRType *targetClassType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(intrinsicNode.GetTyIdx());
  StmtNode *resultFalse = nullptr;
  StmtNode *result = nullptr;
  StmtNode *objectClassAssign = nullptr;
  BaseNode *objectClassReadNode = nullptr;

  MapleVector<BaseNode*> nopnd = intrinsicNode.GetNopnd();
  BaseNode *opnd = nopnd[0];
  BaseNode *ireadShadowExpr = GetObjectShadow(opnd);

  ConstvalNode *falseVal = builder->GetConstUInt1(false);
  BaseNode *nullVal = builder->CreateIntConst(0, PTY_ptr);
  if (stmt.op == OP_regassign) {
    auto *regAssignNode = static_cast<RegassignNode*>(&stmt);
    PrimType classPrimType = ireadShadowExpr->GetPrimType();
    MIRPreg *mirPreg = currFunc->GetPregTab()->PregFromPregIdx(regAssignNode->GetRegIdx());
    resultFalse = builder->CreateStmtRegassign(mirPreg->GetPrimType(), regAssignNode->GetRegIdx(), falseVal);

    PregIdx objectClassSymPregIdx = currFunc->GetPregTab()->CreatePreg(classPrimType);
    objectClassAssign = builder->CreateStmtRegassign(classPrimType, objectClassSymPregIdx, ireadShadowExpr);
    objectClassReadNode = builder->CreateExprRegread(PTY_ptr, objectClassSymPregIdx);

    MapleVector<BaseNode*> arguments(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
    arguments.push_back(objectClassReadNode);
    BaseNode *assignableFromNode = builder->CreateExprIntrinsicop(INTRN_JAVA_ISASSIGNABLEFROM, OP_intrinsicopwithtype,
                                                                  *targetClassType, arguments);
    result = builder->CreateStmtRegassign(mirPreg->GetPrimType(), regAssignNode->GetRegIdx(), assignableFromNode);
  } else {
    auto *regAssignNode = static_cast<DassignNode*>(&stmt);
    MIRSymbol *instanceOfRet = currFunc->GetLocalOrGlobalSymbol(regAssignNode->GetStIdx());
    resultFalse = builder->CreateStmtDassign(*instanceOfRet, 0, falseVal);

    MIRSymbol *objectClassSym = builder->GetOrCreateLocalDecl(kObjectClassSym, *GlobalTables::GetTypeTable().GetPtr());
    objectClassAssign = builder->CreateStmtDassign(*objectClassSym, 0, ireadShadowExpr);
    objectClassReadNode = builder->CreateExprDread(*objectClassSym);
    MapleVector<BaseNode*> arguments(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
    arguments.push_back(objectClassReadNode);
    BaseNode *assignableFromNode = builder->CreateExprIntrinsicop(INTRN_JAVA_ISASSIGNABLEFROM, OP_intrinsicopwithtype,
                                                                  *targetClassType, arguments);
    result = builder->CreateStmtDassign(*instanceOfRet, 0, assignableFromNode);
  }

  BaseNode *condZero = builder->CreateExprCompare(
      OP_ne, *GlobalTables::GetTypeTable().GetUInt1(), *GlobalTables::GetTypeTable().GetPtrType(), opnd, nullVal);
  IfStmtNode *ifObjZeroNode = builder->CreateStmtIfThenElse(condZero);

  ifObjZeroNode->GetThenPart()->AddStatement(objectClassAssign);
  ifObjZeroNode->GetThenPart()->InsertLast(result);
  ifObjZeroNode->GetElsePart()->AddStatement(resultFalse);

  currFunc->GetBody()->InsertBefore(&stmt, ifObjZeroNode);
  currFunc->GetBody()->RemoveStmt(&stmt);
}

void CheckCastGenerator::OptimizeInstanceof() {
  StmtNode *stmt = currFunc->GetBody()->GetFirst();
  StmtNode *next = nullptr;
  while (stmt != nullptr) {
    next = stmt->GetNext();
    Opcode op = stmt->GetOpCode();
    if (op == OP_dassign || op == OP_regassign) {
      auto *unode = static_cast<UnaryStmtNode*>(stmt);
      if (unode->GetRHS() != nullptr && unode->GetRHS()->GetOpCode() == OP_intrinsicopwithtype) {
        auto *intrinsicNode = static_cast<IntrinsicopNode*>(unode->GetRHS());
        ASSERT(intrinsicNode != nullptr, "null ptr check!");
        if (intrinsicNode->GetIntrinsic() == INTRN_JAVA_INSTANCE_OF) {
          ConvertInstanceofToIsAssignableFrom(*stmt, *intrinsicNode);
        }
      }
    }
    stmt = next;
  }
}

void CheckCastGenerator::OptimizeIsAssignableFrom() const {
  StmtNode *stmt = currFunc->GetBody()->GetFirst();
  StmtNode *next = nullptr;
  while (stmt != nullptr) {
    next = stmt->GetNext();
    Opcode op = stmt->GetOpCode();
    if (op == OP_if) {
      auto *ifStmtNode = static_cast<IfStmtNode*>(stmt);
      BlockNode *thenpart = ifStmtNode->GetThenPart();
      ASSERT(thenpart != nullptr, "null ptr check!");
      StmtNode *thenStmt = thenpart->GetFirst();
      while (thenStmt != nullptr) {
        if (thenStmt->GetOpCode() == OP_dassign || thenStmt->GetOpCode() == OP_regassign) {
          auto *unode = static_cast<UnaryStmtNode*>(thenStmt);
          ASSERT(unode->GetRHS() != nullptr, "null ptr check!");
          if (unode->GetRHS()->GetOpCode() == OP_intrinsicopwithtype) {
            auto *intrinsicNode = static_cast<IntrinsicopNode*>(unode->GetRHS());
            if (intrinsicNode->GetIntrinsic() == INTRN_JAVA_ISASSIGNABLEFROM) {
              CheckIsAssignableFrom(*thenpart, *thenStmt, *intrinsicNode);
              break;
            }
          }
        }
        thenStmt = thenStmt->GetNext();
      }
    }
    stmt = next;
  }
}

StmtNode *PreCheckCast::GetAssignRet(IntrinsiccallNode &callnode) {
  BaseNode *opnd = callnode.Opnd(0);
  ASSERT(!callnode.GetReturnVec().empty(), "container check");
  CallReturnPair callretpair = callnode.GetCallReturnPair(0);
  StmtNode *assignRet = nullptr;
  if (!callretpair.second.IsReg()) {
    assignRet = builder->CreateStmtDassign(callretpair.first, callretpair.second.GetFieldID(), opnd);
  } else {
    PregIdx pregidx = callretpair.second.GetPregIdx();
    MIRPreg *mirpreg = currFunc->GetPregTab()->PregFromPregIdx(pregidx);
    assignRet = builder->CreateStmtRegassign(mirpreg->GetPrimType(), pregidx, opnd);
  }
  return assignRet;
}

void PreCheckCast::ProcessFunc(MIRFunction *func) {
  if (func->IsEmpty()) {
    return;
  }
  SetCurrentFunction(*func);
  StmtNode *next = nullptr;
  for (StmtNode *stmt = currFunc->GetBody()->GetFirst(); stmt != nullptr; stmt = next) {
    next = stmt->GetNext();
    if (stmt->GetOpCode() != OP_intrinsiccallwithtypeassigned) {
      continue;
    }

    auto *callnode = static_cast<IntrinsiccallNode*>(stmt);
    if (callnode == nullptr || callnode->GetIntrinsic() != INTRN_JAVA_CHECK_CAST) {
      continue;
    }

    // handle the special case like (Type)null, we dont need a checkcast.
    ASSERT(callnode->GetNopndSize() == 1, "");
    BaseNode *opnd = callnode->Opnd(0);
    if (opnd->op == OP_constval) {
      StmtNode *assignRet = GetAssignRet(*callnode);
      func->GetBody()->ReplaceStmt1WithStmt2(stmt, assignRet);
      continue;
    }
    // split OP_intrinsiccallwithtypeassigned to OP_intrinsiccall + dassign.
    if (opnd->GetPrimType() != PTY_ref && opnd->GetPrimType() != PTY_ptr) {
      continue;
    }
    StmtNode *assignRet = GetAssignRet(*callnode);
    func->GetBody()->InsertAfter(stmt, assignRet);
    StmtNode *newCall = builder->CreateStmtIntrinsicCall(callnode->GetIntrinsic(), callnode->GetNopnd(),
                                                         callnode->GetTyIdx());
    func->GetBody()->ReplaceStmt1WithStmt2(stmt, newCall);
  }
}

bool M2MPreCheckCast::PhaseRun(maple::MIRModule &m) {
  OPT_TEMPLATE_NEWPM(PreCheckCast, m);
  return true;
}

void M2MPreCheckCast::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.SetPreservedAll();
}


void CheckCastGenerator::ProcessFunc(MIRFunction *func) {
  if (func->IsEmpty()) {
    return;
  }
  SetCurrentFunction(*func);
  bool isHotFunc = false;

#ifdef USE_32BIT_REF
  const auto &proFileData = GetMIRModule().GetProfile().GetFunctionProf();
  const auto &funcItem = proFileData.find(func->GetName());
  if (funcItem != proFileData.end()) {
    int callTimes = (funcItem->second).callTimes;
    isHotFunc = callTimes > 0;
  }
#endif

  GenAllCheckCast(isHotFunc);
  if (isHotFunc) {
    OptimizeInstanceof();
    OptimizeIsAssignableFrom();
  }
  MIRLower mirlowerer(GetMIRModule(), func);
  mirlowerer.LowerFunc(*func);
}

bool M2MCheckCastGeneration::PhaseRun(maple::MIRModule &m) {
  OPT_TEMPLATE_NEWPM(CheckCastGenerator, m);
  return true;
}

void M2MCheckCastGeneration::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.SetPreservedAll();
}
}  // namespace maple
