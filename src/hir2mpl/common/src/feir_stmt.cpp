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
#include "feir_stmt.h"
#include "opcode_info.h"
#include "literalstrname.h"
#include "mir_type.h"
#include "feir_builder.h"
#include "feir_var_reg.h"
#include "feir_var_name.h"
#include "fe_manager.h"
#include "hir2mpl_env.h"
#include "feir_var_type_scatter.h"
#include "fe_options.h"
#include "feir_type_helper.h"
#include "bc_util.h"
#include "rc_setter.h"
#include "fe_utils.h"
#include "enhance_c_checker.h"
#include "fe_macros.h"

namespace maple {
std::string GetFEIRNodeKindDescription(FEIRNodeKind kindArg) {
  switch (kindArg) {
#define FEIR_NODE_KIND(kind, description)                            \
    case k##kind: {                                                  \
      return description;                                            \
    }
#include "feir_node_kind.def"
#undef FEIR_NODE_KIND
    default: {
      CHECK_FATAL(false, "Undefined FEIRNodeKind %u", static_cast<uint32>(kindArg));
      return "";
    }
  }
}

// ---------- FEIRStmt ----------
std::list<StmtNode*> FEIRStmt::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  return std::list<StmtNode*>();
}

bool FEIRStmt::IsStmtInstImpl() const {
  switch (kind) {
    case kStmtAssign:
    case kStmtNonAssign:
    case kStmtDAssign:
    case kStmtJavaTypeCheck:
    case kStmtJavaConstClass:
    case kStmtJavaConstString:
    case kStmtJavaMultiANewArray:
    case kStmtCallAssign:
    case kStmtJavaDynamicCallAssign:
    case kStmtIAssign:
    case kStmtUseOnly:
    case kStmtReturn:
    case kStmtBranch:
    case kStmtGoto:
    case kStmtCondGoto:
    case kStmtSwitch:
    case kStmtArrayStore:
    case kStmtFieldStore:
    case kStmtFieldLoad:
      return true;
    default:
      return false;
  }
}

bool FEIRStmt::IsStmtInstComment() const {
  return (kind == kStmtPesudoComment);
}

bool FEIRStmt::ShouldHaveLOC() const {
  return (IsStmtInstImpl() || IsStmtInstComment());
}

BaseNode *FEIRStmt::ReplaceAddrOfNode(BaseNode *node) const {
  switch (node->GetOpCode()) {
    case OP_dread:
      node->SetOpCode(OP_addrof);
      node->SetPrimType(PTY_ptr);
      return node;
    case OP_iread:
      node->SetOpCode(OP_iaddrof);
      node->SetPrimType(PTY_ptr);
      return node;
    default:
      return node;
  }
}

std::string FEIRStmt::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtCheckPoint ----------
void FEIRStmtCheckPoint::Reset() {
  predCPs.clear();
  localUD.clear();
  lastDef.clear();
  cacheUD.clear();
  defs.clear();
  uses.clear();
}

void FEIRStmtCheckPoint::RegisterDFGNode(UniqueFEIRVar &var) {
  CHECK_NULL_FATAL(var);
  if (var->IsDef()) {
    defs.push_back(&var);
    lastDef[FEIRDFGNode(var)] = &var;
  } else {
    uses.push_back(&var);
    auto it = lastDef.find(FEIRDFGNode(var));
    if (it != lastDef.end()) {
      CHECK_FATAL(localUD[&var].insert(it->second).second, "localUD insert failed");
    }
  }
}

void FEIRStmtCheckPoint::RegisterDFGNodes(const std::list<UniqueFEIRVar*> &vars) {
  for (UniqueFEIRVar *var : vars) {
    CHECK_NULL_FATAL(var);
    RegisterDFGNode(*var);
  }
}

void FEIRStmtCheckPoint::RegisterDFGNodeFromAllVisibleStmts() {
  if (firstVisibleStmt == nullptr) {
    return;
  }
  FELinkListNode *node = static_cast<FELinkListNode*>(firstVisibleStmt);
  while (node != this) {
    FEIRStmt *stmt = static_cast<FEIRStmt*>(node);
    stmt->RegisterDFGNodes2CheckPoint(*this);
    node = node->GetNext();
  }
}

void FEIRStmtCheckPoint::AddPredCheckPoint(FEIRStmtCheckPoint &stmtCheckPoint) {
  if (predCPs.find(&stmtCheckPoint) == predCPs.end()) {
    CHECK_FATAL(predCPs.insert(&stmtCheckPoint).second, "pred checkpoints insert error");
  }
}

std::set<UniqueFEIRVar*> &FEIRStmtCheckPoint::CalcuDef(UniqueFEIRVar &use) {
  CHECK_NULL_FATAL(use);
  auto itLocal = localUD.find(&use);
  // search localUD
  if (itLocal != localUD.end()) {
    return itLocal->second;
  }
  // search cacheUD
  auto itCache = cacheUD.find(FEIRDFGNode(use));
  if (itCache != cacheUD.end()) {
    return itCache->second;
  }
  // search by DFS
  std::set<const FEIRStmtCheckPoint*> visitSet;
  std::set<UniqueFEIRVar*> &result = cacheUD[FEIRDFGNode(use)];
  CalcuDefDFS(result, use, *this, visitSet);
  if (result.size() == 0) {
    WARN(kLncWarn, "use var %s without def", use->GetNameRaw().c_str());
  }
  return result;
}

void FEIRStmtCheckPoint::CalcuDefDFS(std::set<UniqueFEIRVar*> &result, const UniqueFEIRVar &use,
                                     const FEIRStmtCheckPoint &cp,
                                     std::set<const FEIRStmtCheckPoint*> &visitSet) const {
  CHECK_NULL_FATAL(use);
  if (visitSet.find(&cp) != visitSet.end()) {
    return;
  }
  CHECK_FATAL(visitSet.insert(&cp).second, "visitSet insert failed");
  auto itLast = cp.lastDef.find(FEIRDFGNode(use));
  if (itLast != cp.lastDef.end()) {
    CHECK_FATAL(result.insert(itLast->second).second, "def insert failed");
    return;
  }
  // optimization by cacheUD
  auto itCache = cp.cacheUD.find(FEIRDFGNode(use));
  if (itCache != cp.cacheUD.end()) {
    for (UniqueFEIRVar *def : itCache->second) {
      CHECK_FATAL(result.insert(def).second, "def insert failed");
    }
    if (itCache->second.size() > 0) {
      return;
    }
  }
  // optimization by cacheUD (end)
  for (const FEIRStmtCheckPoint *pred : cp.predCPs) {
    CHECK_NULL_FATAL(pred);
    CalcuDefDFS(result, use, *pred, visitSet);
  }
}

std::string FEIRStmtCheckPoint::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind) << " preds: [ ";
  for (FEIRStmtCheckPoint *pred : predCPs) {
    ss << pred->GetID() << ", ";
  }
  ss << " ]";
  return ss.str();
}

// ---------- FEIRStmtNary ----------
FEIRStmtNary::FEIRStmtNary(Opcode opIn, std::list<std::unique_ptr<FEIRExpr>> argExprsIn)
    : FEIRStmt(kFEIRStmtNary), op(opIn), argExprs(std::move(argExprsIn)) {}

std::list<StmtNode*> FEIRStmtNary::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> stmts;
  StmtNode *stmt = nullptr;
  if (kOpcodeInfo.IsAssertBoundary(op)) {
    auto args = ReplaceBoundaryChecking(mirBuilder);
    if (args.size() > 0) {
      stmt = mirBuilder.CreateStmtNary(op, std::move(args));
    }
  } else if (argExprs.size() > 1) {
    MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    for (const auto &arg : argExprs) {
      BaseNode *node = arg->GenMIRNode(mirBuilder);
      args.push_back(node);
    }
    stmt = mirBuilder.CreateStmtNary(op, std::move(args));
  } else if (argExprs.size() == 1) {
    BaseNode *node = argExprs.front()->GenMIRNode(mirBuilder);
    if (op == OP_eval && argExprs.front()->IsAddrof()) {
      node = ReplaceAddrOfNode(node);  // addrof va_list
    }
    stmt = mirBuilder.CreateStmtNary(op, node);
  } else {
    CHECK_FATAL(false, "Invalid arg size for MIR StmtNary");
  }
  if (stmt != nullptr) {
    stmts.emplace_back(stmt);
  }
  return stmts;
}

// ---------- FEStmtAssign ----------
FEIRStmtAssign::FEIRStmtAssign(FEIRNodeKind argKind, std::unique_ptr<FEIRVar> argVar)
    : FEIRStmt(argKind),
      hasException(false),
      var(std::move(argVar)) {}

void FEIRStmtAssign::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  if (var != nullptr) {
    var->SetDef(true);
    checkPoint.RegisterDFGNode(var);
  }
}

std::string FEIRStmtAssign::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEStmtDAssign ----------
FEIRStmtDAssign::FEIRStmtDAssign(std::unique_ptr<FEIRVar> argVar, std::unique_ptr<FEIRExpr> argExpr, int32 argFieldID)
    : FEIRStmtAssign(FEIRNodeKind::kStmtDAssign, std::move(argVar)),
      fieldID(argFieldID) {
  SetExpr(std::move(argExpr));
}

void FEIRStmtDAssign::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  expr->RegisterDFGNodes2CheckPoint(checkPoint);
  var->SetDef(true);
  checkPoint.RegisterDFGNode(var);
}

bool FEIRStmtDAssign::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  return expr->CalculateDefs4AllUses(checkPoint, udChain);
}

void FEIRStmtDAssign::InitTrans4AllVarsImpl() {
  switch (expr->GetKind()) {
    case FEIRNodeKind::kExprDRead: {
      FEIRExprDRead *dRead = static_cast<FEIRExprDRead*>(expr.get());
      UniqueFEIRVarTrans trans1 = std::make_unique<FEIRVarTrans>(FEIRVarTransKind::kFEIRVarTransDirect, var);
      dRead->SetTrans(std::move(trans1));
      var->SetTrans(dRead->CreateTransDirect());
      break;
    }
    case FEIRNodeKind::kExprArrayLoad: {
      FEIRExprArrayLoad *arrayLoad = static_cast<FEIRExprArrayLoad*>(expr.get());
      UniqueFEIRVarTrans trans1 = std::make_unique<FEIRVarTrans>(FEIRVarTransKind::kFEIRVarTransArrayDimIncr, var);
      arrayLoad->SetTrans(std::move(trans1));
      var->SetTrans(arrayLoad->CreateTransArrayDimDecr());
      break;
    }
    default:
      break;
  }
}

std::list<StmtNode*> FEIRStmtDAssign::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  ASSERT(var != nullptr, "dst var is nullptr");
  ASSERT(expr != nullptr, "src expr is nullptr");
  MIRSymbol *dstSym = var->GenerateMIRSymbol(mirBuilder);
  BaseNode *srcNode = expr->GenMIRNode(mirBuilder);
  if (fieldID != 0) {
    MIRType *mirType = var->GetType()->GenerateMIRTypeAuto();
    CHECK_FATAL((mirType->GetKind() == MIRTypeKind::kTypeStruct || mirType->GetKind() == MIRTypeKind::kTypeUnion),
                "If fieldID is not 0, then the variable must be a structure");
  }
  InsertNonnullChecking(mirBuilder, *dstSym, ans);
  CheckNonnullArgsAndRetForFuncPtr();
  CheckBoundaryArgsAndRetForFuncPtr();
  StmtNode *mirStmt = mirBuilder.CreateStmtDassign(*dstSym, fieldID, srcNode);
  ans.push_back(mirStmt);
  AssignBoundaryVarAndChecking(mirBuilder, ans);
  return ans;
}

void FEIRStmtDAssign::InsertNonnullChecking(MIRBuilder &mirBuilder, const MIRSymbol &dstSym,
                                            std::list<StmtNode*> &ans) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic()) {
    return;
  }
  if (fieldID == 0) {
    if (!dstSym.GetAttr(ATTR_nonnull)) {
      return;
    }
  } else {
    FieldID tmpID = fieldID;
    FieldPair fieldPair = static_cast<MIRStructType*>(dstSym.GetType())->TraverseToFieldRef(tmpID);
    if (!fieldPair.second.second.GetAttr(FLDATTR_nonnull)) {
      return;
    }
  }
  if (ENCChecker::HasNullExpr(expr)) {
    FE_ERR(kLncErr, "%s:%d error: null assignment of nonnull pointer",
           FEManager::GetModule().GetFileNameFromFileNum(srcFileIndex).c_str(), srcFileLineNum);
    return;
  }
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtUseOnly>(OP_assignassertnonnull, expr->Clone());
  std::list<StmtNode*> stmts = stmt->GenMIRStmts(mirBuilder);
  ans.splice(ans.end(), stmts);
}

std::string FEIRStmtDAssign::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  ss << " def : " << var->GetNameRaw() << ", uses : " << expr->DumpDotString() << std::endl;
  return ss.str();
}

// ---------- FEIRStmtJavaTypeCheck ----------
FEIRStmtJavaTypeCheck::FEIRStmtJavaTypeCheck(std::unique_ptr<FEIRVar> argVar, std::unique_ptr<FEIRExpr> argExpr,
                                             std::unique_ptr<FEIRType> argType,
                                             FEIRStmtJavaTypeCheck::CheckKind argCheckKind)
    : FEIRStmtAssign(FEIRNodeKind::kStmtJavaTypeCheck, std::move(argVar)),
      checkKind(argCheckKind),
      expr(std::move(argExpr)),
      type(std::move(argType)) {}

FEIRStmtJavaTypeCheck::FEIRStmtJavaTypeCheck(std::unique_ptr<FEIRVar> argVar, std::unique_ptr<FEIRExpr> argExpr,
                                             std::unique_ptr<FEIRType> argType,
                                             FEIRStmtJavaTypeCheck::CheckKind argCheckKind,
                                             uint32 argTypeID)
    : FEIRStmtAssign(FEIRNodeKind::kStmtJavaTypeCheck, std::move(argVar)),
      checkKind(argCheckKind),
      expr(std::move(argExpr)),
      type(std::move(argType)),
      typeID(argTypeID) {}

void FEIRStmtJavaTypeCheck::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  expr->RegisterDFGNodes2CheckPoint(checkPoint);
  var->SetDef(true);
  checkPoint.RegisterDFGNode(var);
}

bool FEIRStmtJavaTypeCheck::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  return expr->CalculateDefs4AllUses(checkPoint, udChain);
}

std::list<StmtNode*> FEIRStmtJavaTypeCheck::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  CHECK_FATAL(expr->GetKind() == FEIRNodeKind::kExprDRead, "only support expr dread");
  BaseNode *objNode = expr->GenMIRNode(mirBuilder);
  MIRSymbol *ret = var->GenerateLocalMIRSymbol(mirBuilder);
  MIRType *mirType = type->GenerateMIRType();
  MIRType *mirPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirType, PTY_ref);
  MapleVector<BaseNode*> arguments(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  arguments.push_back(objNode);
  if (FEOptions::GetInstance().IsAOT()) {
    arguments.push_back(mirBuilder.CreateIntConst(typeID, PTY_i32));
  }
  if (checkKind == kCheckCast) {
    StmtNode *callStmt = mirBuilder.CreateStmtIntrinsicCallAssigned(INTRN_JAVA_CHECK_CAST, std::move(arguments), ret,
                                                                    mirPtrType->GetTypeIndex());
    ans.push_back(callStmt);
  } else {
    BaseNode *instanceOf = mirBuilder.CreateExprIntrinsicop(INTRN_JAVA_INSTANCE_OF, OP_intrinsicopwithtype, *mirPtrType,
                                                            std::move(arguments));
    instanceOf->SetPrimType(PTY_u1);
    DassignNode *stmt = mirBuilder.CreateStmtDassign(*ret, 0, instanceOf);
    ans.push_back(stmt);
  }
  return ans;
}

std::string FEIRStmtJavaTypeCheck::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  ss << "uses :" << expr->DumpDotString();
  return ss.str();
}

// ---------- FEIRStmtJavaConstClass ----------
FEIRStmtJavaConstClass::FEIRStmtJavaConstClass(std::unique_ptr<FEIRVar> argVar, std::unique_ptr<FEIRType> argType)
    : FEIRStmtAssign(FEIRNodeKind::kStmtJavaConstClass, std::move(argVar)),
      type(std::move(argType)) {}

void FEIRStmtJavaConstClass::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  var->SetDef(true);
  checkPoint.RegisterDFGNode(var);
}

std::list<StmtNode*> FEIRStmtJavaConstClass::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  MIRSymbol *varSym = var->GenerateLocalMIRSymbol(mirBuilder);
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  MIRType *ptrType = type->GenerateMIRTypeAuto(kSrcLangJava);
  BaseNode *expr =
      mirBuilder.CreateExprIntrinsicop(INTRN_JAVA_CONST_CLASS, OP_intrinsicopwithtype, *ptrType, std::move(args));
  StmtNode *stmt = mirBuilder.CreateStmtDassign(*varSym, 0, expr);
  ans.push_back(stmt);
  return ans;
}

// ---------- FEIRStmtJavaConstString ----------
FEIRStmtJavaConstString::FEIRStmtJavaConstString(std::unique_ptr<FEIRVar> argVar, const std::string &argStrVal,
                                                 uint32 argFileIdx, uint32 argStringID)
    : FEIRStmtAssign(FEIRNodeKind::kStmtJavaConstString, std::move(argVar)),
      strVal(argStrVal),  fileIdx(argFileIdx), stringID(argStringID) {}

void FEIRStmtJavaConstString::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  var->SetDef(true);
  checkPoint.RegisterDFGNode(var);
}

std::list<StmtNode*> FEIRStmtJavaConstString::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  MIRSymbol *literalVal = FEManager::GetJavaStringManager().GetLiteralVar(strVal);
  if (literalVal == nullptr) {
    literalVal = FEManager::GetJavaStringManager().CreateLiteralVar(mirBuilder, strVal, false);
  }
  MIRSymbol *literalValPtr = FEManager::GetJavaStringManager().GetLiteralPtrVar(literalVal);
  if (literalValPtr == nullptr) {
    std::string localStrName = kLocalStringPrefix + std::to_string(fileIdx) + "_" + std::to_string(stringID);
    MIRType *typeString = FETypeManager::kFEIRTypeJavaString->GenerateMIRTypeAuto(kSrcLangJava);
    MIRSymbol *symbolLocal = mirBuilder.GetOrCreateLocalDecl(localStrName.c_str(), *typeString);
    if (!FEOptions::GetInstance().IsAOT()) {
      MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
      args.push_back(mirBuilder.CreateExprAddrof(0, *literalVal));
      StmtNode *stmtCreate = mirBuilder.CreateStmtCallAssigned(
          FEManager::GetTypeManager().GetPuIdxForMCCGetOrInsertLiteral(), std::move(args), symbolLocal,
          OP_callassigned);

      ans.push_back(stmtCreate);
    }
    literalValPtr = symbolLocal;
  }
  MIRSymbol *varDst = var->GenerateLocalMIRSymbol(mirBuilder);
  AddrofNode *node = mirBuilder.CreateDread(*literalValPtr, PTY_ptr);
  StmtNode *stmt = mirBuilder.CreateStmtDassign(*varDst, 0, node);
  ans.push_back(stmt);
  return ans;
}

// ---------- FEIRStmtJavaFillArrayData ----------
FEIRStmtJavaFillArrayData::FEIRStmtJavaFillArrayData(std::unique_ptr<FEIRExpr> arrayExprIn, const int8 *arrayDataIn,
                                                     uint32 sizeIn, const std::string &arrayNameIn)
    : FEIRStmtAssign(FEIRNodeKind::kStmtJavaFillArrayData, nullptr),
      arrayExpr(std::move(arrayExprIn)),
      arrayData(arrayDataIn),
      size(sizeIn),
      arrayName(arrayNameIn) {}

void FEIRStmtJavaFillArrayData::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  arrayExpr->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRStmtJavaFillArrayData::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  return arrayExpr->CalculateDefs4AllUses(checkPoint, udChain);
}

std::list<StmtNode*> FEIRStmtJavaFillArrayData::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  PrimType elemPrimType = ProcessArrayElemPrimType();
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  BaseNode *nodeArray = arrayExpr->GenMIRNode(mirBuilder);
  args.push_back(nodeArray);
  MIRSymbol *elemDataVar = ProcessArrayElemData(mirBuilder, elemPrimType);
  BaseNode *nodeAddrof = mirBuilder.CreateExprAddrof(0, *elemDataVar);
  args.push_back(nodeAddrof);
  uint64 elemPrimTypeSize = GetPrimTypeSize(elemPrimType);
  uint64 val = elemPrimTypeSize * size;
  BaseNode *nodebytes = mirBuilder.CreateIntConst(val, PTY_i32);
  args.push_back(nodebytes);
  StmtNode *stmt = mirBuilder.CreateStmtIntrinsicCallAssigned(INTRN_JAVA_ARRAY_FILL, std::move(args), nullptr);
  ans.push_back(stmt);
  return ans;
}

PrimType FEIRStmtJavaFillArrayData::ProcessArrayElemPrimType() const {
  const FEIRType &arrayType = arrayExpr->GetTypeRef();
  CHECK_FATAL(arrayType.IsArray(), "var should be array type");
  UniqueFEIRType elemType = arrayType.Clone();
  (void)elemType->ArrayDecrDim();
  PrimType elemPrimType = elemType->GetPrimType();
  return elemPrimType;
}

MIRSymbol *FEIRStmtJavaFillArrayData::ProcessArrayElemData(MIRBuilder &mirBuilder, PrimType elemPrimType) const {
  // specify size for const array
  uint32 sizeIn = size;
  MIRType *arrayTypeWithSize = GlobalTables::GetTypeTable().GetOrCreateArrayType(
      *GlobalTables::GetTypeTable().GetPrimType(elemPrimType), 1, &sizeIn);
  MIRSymbol *arrayVar = mirBuilder.GetOrCreateGlobalDecl(arrayName, *arrayTypeWithSize);
  arrayVar->SetAttr(ATTR_readonly);
  arrayVar->SetStorageClass(kScFstatic);
  MIRAggConst *val = FillArrayElem(mirBuilder, elemPrimType, *arrayTypeWithSize, size);
  arrayVar->SetKonst(val);
  return arrayVar;
}

MIRAggConst *FEIRStmtJavaFillArrayData::FillArrayElem(MIRBuilder &mirBuilder, PrimType elemPrimType,
                                                      MIRType &arrayTypeWithSize, uint32 elemSize) const {
  MemPool *mp = mirBuilder.GetMirModule().GetMemPool();
  MIRModule &module = mirBuilder.GetMirModule();
  MIRAggConst *val = module.GetMemPool()->New<MIRAggConst>(module, arrayTypeWithSize);
  MIRConst *cst = nullptr;
  for (uint32 i = 0; i < elemSize; ++i) {
    MIRType &elemType = *GlobalTables::GetTypeTable().GetPrimType(elemPrimType);
    switch (elemPrimType) {
      case PTY_u1:
        cst = mp->New<MIRIntConst>((reinterpret_cast<const bool*>(arrayData))[i], elemType);
        break;
      case PTY_i8:
        cst = mp->New<MIRIntConst>((reinterpret_cast<const int8*>(arrayData))[i], elemType);
        break;
      case PTY_u8:
        cst = mp->New<MIRIntConst>((reinterpret_cast<const uint8*>(arrayData))[i], elemType);
        break;
      case PTY_i16:
        cst = mp->New<MIRIntConst>((reinterpret_cast<const int16*>(arrayData))[i], elemType);
        break;
      case PTY_u16:
        cst = mp->New<MIRIntConst>((reinterpret_cast<const uint16*>(arrayData))[i], elemType);
        break;
      case PTY_i32:
        cst = mp->New<MIRIntConst>((reinterpret_cast<const int32*>(arrayData))[i], elemType);
        break;
      case PTY_u32:
        cst = mp->New<MIRIntConst>((reinterpret_cast<const uint32*>(arrayData))[i], elemType);
        break;
      case PTY_i64:
        cst = mp->New<MIRIntConst>((reinterpret_cast<const int64*>(arrayData))[i], elemType);
        break;
      case PTY_u64:
        cst = mp->New<MIRIntConst>((reinterpret_cast<const uint64*>(arrayData))[i], elemType);
        break;
      case PTY_f32:
        cst = mp->New<MIRFloatConst>((reinterpret_cast<const float*>(arrayData))[i], elemType);
        break;
      case PTY_f64:
        cst = mp->New<MIRDoubleConst>((reinterpret_cast<const double*>(arrayData))[i], elemType);
        break;
      default:
        CHECK_FATAL(false, "Unsupported primitive type");
        break;
    }
    val->PushBack(cst);
  }
  return val;
}

// ---------- FEIRStmtJavaMultiANewArray ----------
UniqueFEIRVar FEIRStmtJavaMultiANewArray::varSize = nullptr;
UniqueFEIRVar FEIRStmtJavaMultiANewArray::varClass = nullptr;
UniqueFEIRType FEIRStmtJavaMultiANewArray::typeAnnotation = nullptr;
FEStructMethodInfo *FEIRStmtJavaMultiANewArray::methodInfoNewInstance = nullptr;

FEIRStmtJavaMultiANewArray::FEIRStmtJavaMultiANewArray(std::unique_ptr<FEIRVar> argVar,
                                                       std::unique_ptr<FEIRType> argElemType,
                                                       std::unique_ptr<FEIRType> argArrayType)
    : FEIRStmtAssign(FEIRNodeKind::kStmtJavaMultiANewArray, std::move(argVar)),
      elemType(std::move(argElemType)),
      arrayType(std::move(argArrayType)) {}

void FEIRStmtJavaMultiANewArray::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  for (const UniqueFEIRExpr &expr : exprSizes) {
    expr->RegisterDFGNodes2CheckPoint(checkPoint);
  }
  var->SetDef(true);
  checkPoint.RegisterDFGNode(var);
}

bool FEIRStmtJavaMultiANewArray::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  bool success = true;
  for (const UniqueFEIRExpr &expr : exprSizes) {
    success = success && expr->CalculateDefs4AllUses(checkPoint, udChain);
  }
  return success;
}

void FEIRStmtJavaMultiANewArray::AddVarSize(std::unique_ptr<FEIRVar> argVarSize) {
  argVarSize->SetType(FETypeManager::kPrimFEIRTypeI32->Clone());
  UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(std::move(argVarSize));
  exprSizes.push_back(std::move(expr));
}

void FEIRStmtJavaMultiANewArray::AddVarSizeRev(std::unique_ptr<FEIRVar> argVarSize) {
  argVarSize->SetType(FETypeManager::kPrimFEIRTypeI32->Clone());
  UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(std::move(argVarSize));
  exprSizes.push_front(std::move(expr));
}

std::list<StmtNode*> FEIRStmtJavaMultiANewArray::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  // size array fill
  MapleVector<BaseNode*> argsSizeArrayFill(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  for (const UniqueFEIRExpr &expr : exprSizes) {
    BaseNode *node = expr->GenMIRNode(mirBuilder);
    argsSizeArrayFill.push_back(node);
  }
  MIRSymbol *symSize = GetVarSize()->GenerateLocalMIRSymbol(mirBuilder);
  StmtNode *stmtSizeArrayFill = mirBuilder.CreateStmtIntrinsicCallAssigned(INTRN_JAVA_FILL_NEW_ARRAY,
      std::move(argsSizeArrayFill), symSize, TyIdx(PTY_i32));
  ans.push_back(stmtSizeArrayFill);
  // const class
  FEIRStmtJavaConstClass feStmtConstClass(GetVarClass()->Clone(), elemType->Clone());
  std::list<StmtNode*> stmtsConstClass = feStmtConstClass.GenMIRStmts(mirBuilder);
  (void)ans.insert(ans.end(), stmtsConstClass.begin(), stmtsConstClass.end());
  // invoke newInstance
  UniqueFEIRVar varRetCall = var->Clone();
  varRetCall->SetType(FETypeManager::kFEIRTypeJavaObject->Clone());
  FEIRStmtCallAssign feStmtCall(GetMethodInfoNewInstance(), OP_callassigned, varRetCall->Clone(), true);
  feStmtCall.AddExprArg(FEIRBuilder::CreateExprDRead(GetVarClass()->Clone()));
  feStmtCall.AddExprArg(FEIRBuilder::CreateExprDRead(GetVarSize()->Clone()));
  std::list<StmtNode*> stmtsCall = feStmtCall.GenMIRStmts(mirBuilder);
  (void)ans.insert(ans.end(), stmtsCall.begin(), stmtsCall.end());
  // check cast
  var->SetType(arrayType->Clone());
  UniqueFEIRExpr expr = std::make_unique<FEIRExprDRead>(std::move(varRetCall));
  FEIRStmtJavaTypeCheck feStmtCheck(var->Clone(), std::move(expr), arrayType->Clone(),
                                    FEIRStmtJavaTypeCheck::kCheckCast);
  std::list<StmtNode*> stmtsCheck = feStmtCheck.GenMIRStmts(mirBuilder);
  (void)ans.insert(ans.end(), stmtsCheck.begin(), stmtsCheck.end());
  return ans;
}

const UniqueFEIRVar &FEIRStmtJavaMultiANewArray::GetVarSize() {
  if (varSize != nullptr) {
    return varSize;
  }
  HIR2MPL_PARALLEL_FORBIDDEN();
  GStrIdx varNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("tmpsize");
  UniqueFEIRType varSizeType = FETypeManager::kPrimFEIRTypeI32->Clone();
  (void)varSizeType->ArrayIncrDim();
  varSize = std::make_unique<FEIRVarName>(varNameIdx, std::move(varSizeType), true);
  return varSize;
}

const UniqueFEIRVar &FEIRStmtJavaMultiANewArray::GetVarClass() {
  if (varClass != nullptr) {
    return varClass;
  }
  HIR2MPL_PARALLEL_FORBIDDEN();
  GStrIdx varNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("tmpclass");
  varClass = std::make_unique<FEIRVarName>(varNameIdx, FETypeManager::kFEIRTypeJavaClass->Clone(), true);
  return varClass;
}

const UniqueFEIRType &FEIRStmtJavaMultiANewArray::GetTypeAnnotation() {
  if (typeAnnotation != nullptr) {
    return typeAnnotation;
  }
  HIR2MPL_PARALLEL_FORBIDDEN();
  typeAnnotation = std::make_unique<FEIRTypeDefault>(PTY_ref);
  static_cast<FEIRTypeDefault*>(typeAnnotation.get())->LoadFromJavaTypeName("Ljava/lang/annotation/Annotation;", false);
  return typeAnnotation;
}

FEStructMethodInfo &FEIRStmtJavaMultiANewArray::GetMethodInfoNewInstance() {
  if (methodInfoNewInstance != nullptr) {
    return *methodInfoNewInstance;
  }
  StructElemNameIdx structElemNameIdx(bc::BCUtil::GetMultiANewArrayClassIdx(),
                                      bc::BCUtil::GetMultiANewArrayElemIdx(),
                                      bc::BCUtil::GetMultiANewArrayTypeIdx(),
                                      bc::BCUtil::GetMultiANewArrayFullIdx());
  methodInfoNewInstance = static_cast<FEStructMethodInfo*>(FEManager::GetTypeManager().RegisterStructMethodInfo(
      structElemNameIdx, kSrcLangJava, true));
  return *methodInfoNewInstance;
}

std::string FEIRStmtJavaMultiANewArray::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtUseOnly ----------
FEIRStmtUseOnly::FEIRStmtUseOnly(FEIRNodeKind argKind, Opcode argOp, std::unique_ptr<FEIRExpr> argExpr)
    : FEIRStmt(argKind),
      op(argOp) {
  if (argExpr != nullptr) {
    expr = std::move(argExpr);
  }
}

FEIRStmtUseOnly::FEIRStmtUseOnly(Opcode argOp, std::unique_ptr<FEIRExpr> argExpr)
    : FEIRStmtUseOnly(FEIRNodeKind::kStmtUseOnly, argOp, std::move(argExpr)) {}

void FEIRStmtUseOnly::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  if (expr != nullptr) {
    expr->RegisterDFGNodes2CheckPoint(checkPoint);
  }
}

bool FEIRStmtUseOnly::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  if (expr != nullptr) {
    return expr->CalculateDefs4AllUses(checkPoint, udChain);
  }
  return true;
}

std::list<StmtNode*> FEIRStmtUseOnly::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  ASSERT_NOT_NULL(expr);
  if (SkipNonnullChecking(mirBuilder)) {
    return ans;
  }
  BaseNode *srcNode = expr->GenMIRNode(mirBuilder);
  StmtNode *mirStmt = mirBuilder.CreateStmtNary(op, srcNode);
  ans.push_back(mirStmt);
  return ans;
}

bool FEIRStmtUseOnly::SkipNonnullChecking(MIRBuilder &mirBuilder) const {
  if (!kOpcodeInfo.IsAssertNonnull(op)) {
    return false;
  }
  return ENCChecker::HasNonnullAttrInExpr(mirBuilder, expr);
}

std::string FEIRStmtUseOnly::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  if (expr != nullptr) {
    ss << expr->DumpDotString();
  }
  return ss.str();
}

// ---------- FEIRStmtCallAssertNonnull ----------
std::list<StmtNode*> FEIRStmtCallAssertNonnull::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  ASSERT_NOT_NULL(expr);
  if (SkipNonnullChecking(mirBuilder)) {
    return ans;
  }
  BaseNode *srcNode = expr->GenMIRNode(mirBuilder);
  GStrIdx stridx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(GetFuncName());
  StmtNode *mirStmt = mirBuilder.CreateStmtCallAssertNonnull(op, srcNode, stridx, GetParamIndex());
  ans.push_back(mirStmt);
  return ans;
}

// ---------- FEIRStmtCallAssertBoundary ----------
std::list<StmtNode*> FEIRStmtCallAssertBoundary::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> stmts;
  StmtNode *stmt = nullptr;
  auto args = ReplaceBoundaryChecking(mirBuilder);
  if (args.size() > 0) {
    GStrIdx stridx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(GetFuncName());
    stmt = mirBuilder.CreateStmtCallAssertBoundary(op, std::move(args), stridx, GetParamIndex());
  }
  if (stmt != nullptr) {
    stmts.emplace_back(stmt);
  }
  return stmts;
}

// ---------- FEIRStmtReturn ----------
FEIRStmtReturn::FEIRStmtReturn(std::unique_ptr<FEIRExpr> argExpr)
    : FEIRStmtUseOnly(FEIRNodeKind::kStmtReturn, OP_return, std::move(argExpr)) {}

std::list<StmtNode*> FEIRStmtReturn::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  StmtNode *mirStmt = nullptr;
  if (expr == nullptr) {
    mirStmt = mirBuilder.CreateStmtReturn(nullptr);
  } else {
    BaseNode *srcNode = expr->GenMIRNode(mirBuilder);
    if (mirBuilder.GetCurrentFunction()->IsFirstArgReturn()) {
      MIRSymbol *firstArgRetSym = mirBuilder.GetCurrentFunction()->GetFormal(0);
      BaseNode *addrNode = mirBuilder.CreateDread(*firstArgRetSym, PTY_ptr);
      StmtNode *iNode = mirBuilder.CreateStmtIassign(*firstArgRetSym->GetType(), 0, addrNode, srcNode);
      ans.emplace_back(iNode);
      mirStmt = mirBuilder.CreateStmtReturn(nullptr);
    } else {
      InsertNonnullChecking(mirBuilder, ans);
      ENCChecker::InsertBoundaryAssignChecking(mirBuilder, ans, expr, srcFileIndex, srcFileLineNum);
      mirStmt = mirBuilder.CreateStmtReturn(srcNode);
    }
  }
  ans.emplace_back(mirStmt);
  return ans;
}

void FEIRStmtReturn::InsertNonnullChecking(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || expr == nullptr) {
    return;
  }
  if (!mirBuilder.GetCurrentFunction()->GetAttrs().GetAttr(FUNCATTR_nonnull)) {
    return;
  }
  if (ENCChecker::HasNullExpr(expr)) {
    FE_ERR(kLncErr, "%s:%d error: null returned from function that requires a nonnull return value",
             FEManager::GetModule().GetFileNameFromFileNum(srcFileIndex).c_str(), srcFileLineNum);
    return;
  }
  if ((expr->GetKind() == kExprDRead || expr->GetKind() == kExprIRead) && expr->GetPrimType() == PTY_ptr) {
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtUseOnly>(OP_returnassertnonnull, expr->Clone());
    std::list<StmtNode*> stmts = stmt->GenMIRStmts(mirBuilder);
    ans.splice(ans.end(), stmts);
  }
}

// ---------- FEIRStmtGoto ----------
FEIRStmtGoto::FEIRStmtGoto(uint32 argLabelIdx)
    : FEIRStmt(FEIRNodeKind::kStmtGoto),
      labelIdx(argLabelIdx),
      stmtTarget(nullptr) {}

FEIRStmtGoto::~FEIRStmtGoto() {
  stmtTarget = nullptr;
}

std::list<StmtNode*> FEIRStmtGoto::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  CHECK_NULL_FATAL(stmtTarget);
  GotoNode *gotoNode = mirBuilder.CreateStmtGoto(OP_goto, stmtTarget->GetMIRLabelIdx());
  ans.push_back(gotoNode);
  return ans;
}

std::string FEIRStmtGoto::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtGoto2 ----------
FEIRStmtGoto2::FEIRStmtGoto2(uint32 qIdx0, uint32 qIdx1)
    : FEIRStmt(FEIRNodeKind::kStmtGoto), labelIdxOuter(qIdx0), labelIdxInner(qIdx1) {}

std::list<StmtNode*> FEIRStmtGoto2::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> stmts;
  GotoNode *gotoNode = mirBuilder.CreateStmtGoto(
      OP_goto, FEIRStmtPesudoLabel2::GenMirLabelIdx(mirBuilder, labelIdxOuter, labelIdxInner));
  stmts.push_back(gotoNode);
  return stmts;
}

std::pair<uint32, uint32> FEIRStmtGoto2::GetLabelIdx() const {
  return std::make_pair(labelIdxOuter, labelIdxInner);
}

FEIRStmtGotoForC::FEIRStmtGotoForC(const std::string &name)
    : FEIRStmt(FEIRNodeKind::kStmtGoto),
      labelName(name) {}

std::list<StmtNode*> FEIRStmtGotoForC::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  LabelIdx label = mirBuilder.GetOrCreateMIRLabel(labelName);
  GotoNode *gotoNode = mirBuilder.CreateStmtGoto(OP_goto, label);
  ans.push_back(gotoNode);
  return ans;
}

std::string FEIRStmtGotoForC::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtIGoto ----------
FEIRStmtIGoto::FEIRStmtIGoto(UniqueFEIRExpr expr) : FEIRStmt(kStmtIGoto), targetExpr(std::move(expr)) {}

std::list<StmtNode*> FEIRStmtIGoto::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> stmts;
  stmts.emplace_back(mirBuilder.CreateStmtUnary(OP_igoto, targetExpr->GenMIRNode(mirBuilder)));
  return stmts;
}

// ---------- FEIRStmtCondGotoForC ----------
std::list<StmtNode*> FEIRStmtCondGotoForC::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  LabelIdx labelID = mirBuilder.GetOrCreateMIRLabel(labelName);
  BaseNode *condNode = expr->GenMIRNode(mirBuilder);
  CondGotoNode *gotoNode = mirBuilder.CreateStmtCondGoto(condNode, opCode, labelID);
  ans.push_back(gotoNode);
  return ans;
}

std::string FEIRStmtCondGotoForC::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtCondGoto ----------
FEIRStmtCondGoto::FEIRStmtCondGoto(Opcode argOp, uint32 argLabelIdx, UniqueFEIRExpr argExpr)
    : FEIRStmtGoto(argLabelIdx),
      op(argOp) {
  kind = FEIRNodeKind::kStmtCondGoto;
  SetExpr(std::move(argExpr));
}

void FEIRStmtCondGoto::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  expr->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRStmtCondGoto::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  return expr->CalculateDefs4AllUses(checkPoint, udChain);
}

std::list<StmtNode*> FEIRStmtCondGoto::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  BaseNode *condNode = expr->GenMIRNode(mirBuilder);
  CHECK_NULL_FATAL(stmtTarget);
  CondGotoNode *gotoNode = mirBuilder.CreateStmtCondGoto(condNode, op, stmtTarget->GetMIRLabelIdx());
  ans.push_back(gotoNode);
  return ans;
}

std::string FEIRStmtCondGoto::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtCondGoto2 ----------
FEIRStmtCondGoto2::FEIRStmtCondGoto2(Opcode argOp, uint32 qIdx0, uint32 qIdx1, UniqueFEIRExpr argExpr)
    : FEIRStmtGoto2(qIdx0, qIdx1), op(argOp), expr(std::move(argExpr)) {}

std::list<StmtNode*> FEIRStmtCondGoto2::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> stmts;
  BaseNode *condNode = expr->GenMIRNode(mirBuilder);
  CondGotoNode *gotoNode = mirBuilder.CreateStmtCondGoto(
      condNode, op, FEIRStmtPesudoLabel2::GenMirLabelIdx(mirBuilder, labelIdxOuter, labelIdxInner));
  stmts.push_back(gotoNode);
  return stmts;
}

// ---------- FEIRStmtSwitch ----------
FEIRStmtSwitch::FEIRStmtSwitch(UniqueFEIRExpr argExpr)
    : FEIRStmt(FEIRNodeKind::kStmtSwitch),
      defaultLabelIdx(0),
      defaultTarget(nullptr) {
  SetExpr(std::move(argExpr));
}

FEIRStmtSwitch::~FEIRStmtSwitch() {
  defaultTarget = nullptr;
}

void FEIRStmtSwitch::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  expr->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRStmtSwitch::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  return expr->CalculateDefs4AllUses(checkPoint, udChain);
}

std::list<StmtNode*> FEIRStmtSwitch::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  CaseVector switchTable(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  for (const std::pair<const int32, FEIRStmtPesudoLabel*> &targetPair : mapValueTargets) {
    CHECK_NULL_FATAL(targetPair.second);
    switchTable.push_back(std::make_pair(targetPair.first, targetPair.second->GetMIRLabelIdx()));
  }
  BaseNode *exprNode = expr->GenMIRNode(mirBuilder);
  CHECK_NULL_FATAL(defaultTarget);
  SwitchNode *switchNode = mirBuilder.CreateStmtSwitch(exprNode, defaultTarget->GetMIRLabelIdx(), switchTable);
  ans.push_back(switchNode);
  return ans;
}

std::string FEIRStmtSwitch::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtSwitch2 ----------
FEIRStmtSwitch2::FEIRStmtSwitch2(uint32 outerIdxIn, UniqueFEIRExpr argExpr)
    : FEIRStmt(FEIRNodeKind::kStmtSwitch),
      outerIdx(outerIdxIn),
      defaultLabelIdx(0),
      defaultTarget(nullptr) {
  SetExpr(std::move(argExpr));
}

FEIRStmtSwitch2::~FEIRStmtSwitch2() {
  defaultTarget = nullptr;
}

std::list<StmtNode*> FEIRStmtSwitch2::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  CaseVector switchTable(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  for (const std::pair<const int32, uint32> &valueLabelPair : mapValueLabelIdx) {
    switchTable.emplace_back(valueLabelPair.first,
                             FEIRStmtPesudoLabel2::GenMirLabelIdx(mirBuilder, outerIdx, valueLabelPair.second));
  }
  BaseNode *exprNode = expr->GenMIRNode(mirBuilder);
  SwitchNode *switchNode = mirBuilder.CreateStmtSwitch(exprNode,
      FEIRStmtPesudoLabel2::GenMirLabelIdx(mirBuilder, outerIdx, defaultLabelIdx), switchTable);
  ans.push_back(switchNode);
  return ans;
}

std::string FEIRStmtSwitch2::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtIf ----------
FEIRStmtIf::FEIRStmtIf(UniqueFEIRExpr argCondExpr, std::list<UniqueFEIRStmt> &argThenStmts)
    : FEIRStmt(FEIRNodeKind::kStmtIf) {
  SetCondExpr(std::move(argCondExpr));
  hasElse = false;
  SetThenStmts(argThenStmts);
}

FEIRStmtIf::FEIRStmtIf(UniqueFEIRExpr argCondExpr,
                       std::list<UniqueFEIRStmt> &argThenStmts,
                       std::list<UniqueFEIRStmt> &argElseStmts)
    : FEIRStmt(FEIRNodeKind::kStmtIf) {
  SetCondExpr(std::move(argCondExpr));
  SetThenStmts(argThenStmts);
  if (argElseStmts.empty()) {
    hasElse = false;
  } else {
    hasElse = true;
    SetElseStmts(argElseStmts);
  }
}

std::list<StmtNode*> FEIRStmtIf::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  BaseNode *condBase = condExpr->GenMIRNode(mirBuilder);
  IfStmtNode *stmt = nullptr;
  if (hasElse) {
    stmt = mirBuilder.CreateStmtIfThenElse(condBase);
  } else {
    stmt = mirBuilder.CreateStmtIf(condBase);
  }
  for (const auto &thenStmt : thenStmts) {
    for(auto thenNode : thenStmt->GenMIRStmts(mirBuilder)) {
      stmt->GetThenPart()->AddStatement(thenNode);
    }
  }
  if (hasElse) {
    for (const auto &elseStmt : elseStmts) {
      for(auto elseNode : elseStmt->GenMIRStmts(mirBuilder)) {
        stmt->GetElsePart()->AddStatement(elseNode);
      }
    }
  }
  return std::list<StmtNode*>({ stmt });
}

std::string FEIRStmtIf::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtSwitchForC ----------
FEIRStmtSwitchForC::FEIRStmtSwitchForC(UniqueFEIRExpr argCondExpr, bool argHasDefault)
    : FEIRStmt(FEIRNodeKind::kStmtSwitch),
      expr(std::move(argCondExpr)),
      hasDefault(argHasDefault) {}

void FEIRStmtSwitchForC::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  expr->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRStmtSwitchForC::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  return expr->CalculateDefs4AllUses(checkPoint, udChain);
}

std::list<StmtNode*> FEIRStmtSwitchForC::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  MIRModule &module = mirBuilder.GetMirModule();
  CaseVector *caseVec = module.CurFuncCodeMemPool()->New<CaseVector>(module.CurFuncCodeMemPoolAllocator()->Adapter());
  std::string endName = AstSwitchUtil::Instance().CreateEndOrExitLabelName();
  LabelIdx swDefaultLabel = mirBuilder.GetOrCreateMIRLabel(endName);  // end label
  AstSwitchUtil::Instance().PushNestedCaseVectors(std::pair<CaseVector*, LabelIdx>(caseVec, swDefaultLabel));
  BaseNode *exprNode = expr->GenMIRNode(mirBuilder);
  BlockNode *tempBlock = module.CurFuncCodeMemPool()->New<BlockNode>();
  CaseVector &switchTable = *AstSwitchUtil::Instance().GetTopOfNestedCaseVectors().first;
  for (auto &sub : subStmts) {
    for (auto mirStmt : sub->GenMIRStmts(mirBuilder)) {
      tempBlock->AddStatement(mirStmt);
    }
  }
  SwitchNode *mirSwitchStmt = mirBuilder.CreateStmtSwitch(exprNode, swDefaultLabel, switchTable);
  ans.push_back(mirSwitchStmt);
  for (auto &it : tempBlock->GetStmtNodes()) {
    ans.emplace_back(&it);
  }
  if (!hasDefault) {
    LabelIdx endLab = mirBuilder.GetOrCreateMIRLabel(endName);
    StmtNode *mirSwExitLabelStmt = mirBuilder.CreateStmtLabel(endLab);
    ans.push_back(mirSwExitLabelStmt);
  }
  LabelIdx exitLab = mirBuilder.GetOrCreateMIRLabel(breakLabelName);
  StmtNode *mirSwExitLabelStmt = mirBuilder.CreateStmtLabel(exitLab);
  ans.push_back(mirSwExitLabelStmt);
  AstSwitchUtil::Instance().PopNestedCaseVectors();
  return ans;
}

std::string FEIRStmtSwitchForC::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

FEIRStmtCaseForC::FEIRStmtCaseForC(int64 label)
    : FEIRStmt(FEIRNodeKind::kStmtCaseForC),
      lCaseLabel(label) {}

void FEIRStmtCaseForC::AddCaseTag2CaseVec(int64 lCaseTag, int64 rCaseTag) {
  auto pLabel = std::make_unique<FEIRStmtPesudoLabel>(lCaseLabel);
  for (int64 csTag = lCaseTag; csTag <= rCaseTag; ++csTag) {
    pesudoLabelMap.insert(std::pair<int64, std::unique_ptr<FEIRStmtPesudoLabel>>(csTag, std::move(pLabel)));
  }
}

std::list<StmtNode*> FEIRStmtCaseForC::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  CaseVector &caseVec = *AstSwitchUtil::Instance().GetTopOfNestedCaseVectors().first;
  StmtNode *mirLabelStmt = nullptr;
  for (auto &targetPair : GetPesudoLabelMap()) {
    targetPair.second->GenerateLabelIdx(mirBuilder);
    caseVec.push_back(std::make_pair(targetPair.first, targetPair.second->GetMIRLabelIdx()));
  }
  for (auto it : caseVec) {
    if (lCaseLabel == it.first) {
      mirLabelStmt = mirBuilder.CreateStmtLabel(it.second);
      ans.emplace_back(mirLabelStmt);
    }
  }
  for (auto &sub : subStmts) {
    ans.splice(ans.end(), sub.get()->GenMIRStmts(mirBuilder));
  }
  return ans;
}

std::string FEIRStmtCaseForC::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

FEIRStmtDefaultForC::FEIRStmtDefaultForC()
    : FEIRStmt(FEIRNodeKind::kStmtDefaultForC) {}

std::list<StmtNode*> FEIRStmtDefaultForC::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  StmtNode *mirLabelStmt = mirBuilder.CreateStmtLabel(AstSwitchUtil::Instance().GetTopOfNestedCaseVectors().second);
  ans.emplace_back(mirLabelStmt);
  for (auto &sub : subStmts) {
    ans.splice(ans.end(), sub.get()->GenMIRStmts(mirBuilder));
  }
  return ans;
}

std::string FEIRStmtDefaultForC::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtArrayStore ----------
FEIRStmtArrayStore::FEIRStmtArrayStore(UniqueFEIRExpr argExprElem, UniqueFEIRExpr argExprArray,
                                       UniqueFEIRExpr argExprIndex, UniqueFEIRType argTypeArray)
    : FEIRStmt(FEIRNodeKind::kStmtArrayStore),
      exprElem(std::move(argExprElem)),
      exprArray(std::move(argExprArray)),
      exprIndex(std::move(argExprIndex)),
      typeArray(std::move(argTypeArray)) {}

FEIRStmtArrayStore::FEIRStmtArrayStore(UniqueFEIRExpr argExprElem, UniqueFEIRExpr argExprArray,
                                       UniqueFEIRExpr argExprIndex, UniqueFEIRType argTypeArray,
                                       const std::string &argArrayName)
    : FEIRStmt(FEIRNodeKind::kStmtArrayStore),
      exprElem(std::move(argExprElem)),
      exprArray(std::move(argExprArray)),
      exprIndex(std::move(argExprIndex)),
      typeArray(std::move(argTypeArray)),
      arrayName(argArrayName) {}

FEIRStmtArrayStore::FEIRStmtArrayStore(UniqueFEIRExpr argExprElem, UniqueFEIRExpr argExprArray,
                                       UniqueFEIRExpr argExprIndex, UniqueFEIRType argTypeArray,
                                       UniqueFEIRType argTypeElem, const std::string &argArrayName)
    : FEIRStmt(FEIRNodeKind::kStmtArrayStore),
      exprElem(std::move(argExprElem)),
      exprArray(std::move(argExprArray)),
      exprIndex(std::move(argExprIndex)),
      typeArray(std::move(argTypeArray)),
      typeElem(std::move(argTypeElem)),
      arrayName(argArrayName) {}

FEIRStmtArrayStore::FEIRStmtArrayStore(UniqueFEIRExpr argExprElem, UniqueFEIRExpr argExprArray,
                                       std::list<UniqueFEIRExpr> &argExprIndexs, UniqueFEIRType argTypeArray,
                                       const std::string &argArrayName)
    : FEIRStmt(FEIRNodeKind::kStmtArrayStore),
      exprElem(std::move(argExprElem)),
      exprArray(std::move(argExprArray)),
      typeArray(std::move(argTypeArray)),
      arrayName(argArrayName) {
  SetIndexsExprs(argExprIndexs);
}

FEIRStmtArrayStore::FEIRStmtArrayStore(UniqueFEIRExpr argExprElem, UniqueFEIRExpr argExprArray,
                                       std::list<UniqueFEIRExpr> &argExprIndexs, UniqueFEIRType argTypeArray,
                                       UniqueFEIRExpr argExprStruct, UniqueFEIRType argTypeStruct,
                                       const std::string &argArrayName)
    : FEIRStmt(FEIRNodeKind::kStmtArrayStore),
      exprElem(std::move(argExprElem)),
      exprArray(std::move(argExprArray)),
      typeArray(std::move(argTypeArray)),
      exprStruct(std::move(argExprStruct)),
      typeStruct(std::move(argTypeStruct)),
      arrayName(argArrayName) {
  SetIndexsExprs(argExprIndexs);
}

void FEIRStmtArrayStore::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  exprArray->RegisterDFGNodes2CheckPoint(checkPoint);
  exprIndex->RegisterDFGNodes2CheckPoint(checkPoint);
  exprElem->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRStmtArrayStore::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  bool success = true;
  success = success && exprArray->CalculateDefs4AllUses(checkPoint, udChain);
  success = success && exprIndex->CalculateDefs4AllUses(checkPoint, udChain);
  success = success && exprElem->CalculateDefs4AllUses(checkPoint, udChain);
  return success;
}

void FEIRStmtArrayStore::InitTrans4AllVarsImpl() {
  CHECK_FATAL(exprArray->GetKind() == kExprDRead, "only support dread expr for exprArray");
  CHECK_FATAL(exprIndex->GetKind() == kExprDRead, "only support dread expr for exprIndex");
  CHECK_FATAL(exprElem->GetKind() == kExprDRead, "only support dread expr for exprElem");
  FEIRExprDRead *exprArrayDRead = static_cast<FEIRExprDRead*>(exprArray.get());
  FEIRExprDRead *exprElemDRead = static_cast<FEIRExprDRead*>(exprElem.get());
  exprArrayDRead->SetTrans(exprElemDRead->CreateTransArrayDimIncr());
  exprElemDRead->SetTrans(exprArrayDRead->CreateTransArrayDimDecr());
}

std::list<StmtNode*> FEIRStmtArrayStore::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  CHECK_FATAL(((exprIndex == nullptr) && (exprIndexs.size() != 0))||
              (exprIndex->GetKind() == kExprDRead) ||
              (exprIndex->GetKind() == kExprConst), "only support dread/const expr for exprIndex");
  MIRType *ptrMIRArrayType = typeArray->GenerateMIRType(false);
  BaseNode *arrayExpr = nullptr;
  MIRType *mIRElemType = nullptr;
  if (FEManager::GetModule().GetSrcLang() == kSrcLangC) {
    GenMIRStmtsImplForCPart(mirBuilder, ptrMIRArrayType, &mIRElemType, &arrayExpr);
  } else {
    BaseNode *addrBase = exprArray->GenMIRNode(mirBuilder);
    BaseNode *indexBn = exprIndex->GenMIRNode(mirBuilder);
    arrayExpr = mirBuilder.CreateExprArray(*ptrMIRArrayType, addrBase, indexBn);
    UniqueFEIRType typeArrayCloned = typeArray->Clone();
    if ((exprIndex->GetKind() != kExprConst) || (!FEOptions::GetInstance().IsAOT())) {
      (void)typeArrayCloned->ArrayDecrDim();
    }
    mIRElemType = typeArrayCloned->GenerateMIRType(true);
  }
  BaseNode *elemBn = exprElem->GenMIRNode(mirBuilder);
  IassignNode *stmt = nullptr;
  if ((FEManager::GetModule().GetSrcLang() == kSrcLangC) ||
      (exprIndex->GetKind() != kExprConst) || (!FEOptions::GetInstance().IsAOT())) {
    MIRType *ptrMIRElemType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mIRElemType, PTY_ptr);
    stmt = mirBuilder.CreateStmtIassign(*ptrMIRElemType, 0, arrayExpr, elemBn);
  } else {
    reinterpret_cast<ArrayNode*>(arrayExpr)->SetBoundsCheck(false);
    stmt = mirBuilder.CreateStmtIassign(*mIRElemType, 0, arrayExpr, elemBn);
  }
  return std::list<StmtNode*>({ stmt });
}

void FEIRStmtArrayStore::GenMIRStmtsImplForCPart(MIRBuilder &mirBuilder, MIRType *ptrMIRArrayType,
                                                 MIRType **mIRElemType, BaseNode **arrayExpr) const {
  if (typeElem == nullptr) {
    typeElem = std::make_unique<FEIRTypeNative>(*static_cast<MIRArrayType*>(ptrMIRArrayType)->GetElemType());
  }
  FieldID fieldID = 0;
  // array in struct
  if (typeStruct != nullptr) {
    fieldID = static_cast<FEIRExprDRead*>(exprArray.get())->GetFieldID();
    MIRType *ptrMIRStructType = typeStruct->GenerateMIRType(false);
    MIRStructType* mirStructType = static_cast<MIRStructType*>(ptrMIRStructType);
    // for no init, create the struct symbol
    (void)mirBuilder.GetOrCreateLocalDecl(arrayName, *mirStructType);
  }

  *mIRElemType = typeElem->GenerateMIRType(true);
  BaseNode *arrayAddrOfExpr = nullptr;
  MIRSymbol *mirSymbol = exprArray->GetVarUses().front()->GenerateMIRSymbol(mirBuilder);
  auto mirtype = mirSymbol->GetType();
  if (mirtype->GetKind() == kTypePointer) {
    arrayAddrOfExpr = exprArray->GenMIRNode(mirBuilder);
  } else {
    arrayAddrOfExpr = mirBuilder.CreateExprAddrof(fieldID, *mirSymbol);
  }
  if (exprIndex == nullptr) {
    std::vector<BaseNode*> nds;
    nds.push_back(arrayAddrOfExpr);
    for (auto &e : exprIndexs) {
      BaseNode *no = e->GenMIRNode(mirBuilder);
      nds.push_back(no);
    }
    *arrayExpr = mirBuilder.CreateExprArray(*ptrMIRArrayType, nds);
  } else {
    BaseNode *indexBn = exprIndex->GenMIRNode(mirBuilder);
    *arrayExpr = mirBuilder.CreateExprArray(*ptrMIRArrayType, arrayAddrOfExpr, indexBn);
  }
}

std::string FEIRStmtArrayStore::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  ss << " def : {";
  ss << " exprArray : " << exprArray->DumpDotString();
  ss << " exprIndex : " << exprIndex->DumpDotString();
  ss << " exprElem kind : " << GetFEIRNodeKindDescription(exprElem->GetKind()) << " " << exprElem->DumpDotString();
  ss << " } ";
  return ss.str();
}

// ---------- FEIRStmtFieldStore ----------
FEIRStmtFieldStore::FEIRStmtFieldStore(UniqueFEIRVar argVarObj, UniqueFEIRVar argVarField,
                                       FEStructFieldInfo &argFieldInfo, bool argIsStatic)
    : FEIRStmt(FEIRNodeKind::kStmtFieldStore),
      varObj(std::move(argVarObj)),
      varField(std::move(argVarField)),
      fieldInfo(argFieldInfo),
      isStatic(argIsStatic) {}
FEIRStmtFieldStore::FEIRStmtFieldStore(UniqueFEIRVar argVarObj, UniqueFEIRVar argVarField,
                                       FEStructFieldInfo &argFieldInfo, bool argIsStatic, int32 argDexFileHashCode)
    : FEIRStmt(FEIRNodeKind::kStmtFieldStore),
      varObj(std::move(argVarObj)),
      varField(std::move(argVarField)),
      fieldInfo(argFieldInfo),
      isStatic(argIsStatic),
      dexFileHashCode(argDexFileHashCode) {}

void FEIRStmtFieldStore::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  if (isStatic) {
    RegisterDFGNodes2CheckPointForStatic(checkPoint);
  } else {
    RegisterDFGNodes2CheckPointForNonStatic(checkPoint);
  }
}

void FEIRStmtFieldStore::RegisterDFGNodes2CheckPointForStatic(FEIRStmtCheckPoint &checkPoint) {
  checkPoint.RegisterDFGNode(varField);
}

void FEIRStmtFieldStore::RegisterDFGNodes2CheckPointForNonStatic(FEIRStmtCheckPoint &checkPoint) {
  checkPoint.RegisterDFGNode(varObj);
  checkPoint.RegisterDFGNode(varField);
}

bool FEIRStmtFieldStore::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  if (isStatic) {
    return CalculateDefs4AllUsesForStatic(checkPoint, udChain);
  } else {
    return CalculateDefs4AllUsesForNonStatic(checkPoint, udChain);
  }
}

bool FEIRStmtFieldStore::CalculateDefs4AllUsesForStatic(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  std::set<UniqueFEIRVar*> &defs4VarField = checkPoint.CalcuDef(varField);
  (void)udChain.insert(std::make_pair(&varField, defs4VarField));
  return (defs4VarField.size() > 0);
}

bool FEIRStmtFieldStore::CalculateDefs4AllUsesForNonStatic(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  std::set<UniqueFEIRVar*> &defs4VarObj = checkPoint.CalcuDef(varObj);
  (void)udChain.insert(std::make_pair(&varObj, defs4VarObj));
  std::set<UniqueFEIRVar*> &defs4VarField = checkPoint.CalcuDef(varField);
  (void)udChain.insert(std::make_pair(&varField, defs4VarField));
  return ((defs4VarObj.size() > 0) && (defs4VarField.size() > 0));
}

std::list<StmtNode*> FEIRStmtFieldStore::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  // prepare and find root
  fieldInfo.Prepare(mirBuilder, isStatic);
  if (isStatic) {
    return GenMIRStmtsImplForStatic(mirBuilder);
  } else {
    return GenMIRStmtsImplForNonStatic(mirBuilder);
  }
}

bool FEIRStmtFieldStore::NeedMCCForStatic(uint32 &typeID) const {
  // check type first
  const std::string &actualContainerName = fieldInfo.GetActualContainerName();
  typeID = FEManager::GetTypeManager().GetTypeIDFromMplClassName(actualContainerName, dexFileHashCode);
  if(typeID == UINT32_MAX) {
    return true;
  }

  // check type field second
  const std::string &fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(fieldInfo.GetFieldNameIdx());
  MIRStructType *currStructType = FEManager::GetTypeManager().GetStructTypeFromName(actualContainerName);
  const auto &fields = currStructType->GetStaticFields();
  for (auto f : fields) {
    const std::string &fieldNameIt = GlobalTables::GetStrTable().GetStringFromStrIdx(f.first);
    if(fieldName.compare(fieldNameIt) == 0) {
      return false;
    }
  }
  return true;
}

void FEIRStmtFieldStore::InitPrimTypeFuncNameIdxMap (std::map<PrimType, GStrIdx> &primTypeFuncNameIdxMap) const {
  primTypeFuncNameIdxMap = {
      { PTY_u1, FEUtils::GetMCCStaticFieldSetBoolIdx() },
      { PTY_i8, FEUtils::GetMCCStaticFieldSetByteIdx() },
      { PTY_i16, FEUtils::GetMCCStaticFieldSetShortIdx() },
      { PTY_u16, FEUtils::GetMCCStaticFieldSetCharIdx() },
      { PTY_i32, FEUtils::GetMCCStaticFieldSetIntIdx() },
      { PTY_i64, FEUtils::GetMCCStaticFieldSetLongIdx() },
      { PTY_f32, FEUtils::GetMCCStaticFieldSetFloatIdx() },
      { PTY_f64, FEUtils::GetMCCStaticFieldSetDoubleIdx() },
      { PTY_ref, FEUtils::GetMCCStaticFieldSetObjectIdx() },
  };
}
std::list<StmtNode*> FEIRStmtFieldStore::GenMIRStmtsImplForStatic(MIRBuilder &mirBuilder) const {
  CHECK_FATAL(fieldInfo.GetFieldNameIdx() != 0, "invalid name idx");
  std::list<StmtNode*> mirStmts;
  UniqueFEIRVar varTarget = std::make_unique<FEIRVarName>(fieldInfo.GetFieldNameIdx(), fieldInfo.GetType()->Clone());
  varTarget->SetGlobal(true);
  MIRSymbol *symSrc = varTarget->GenerateGlobalMIRSymbol(mirBuilder);
  UniqueFEIRExpr exprDRead = FEIRBuilder::CreateExprDRead(varField->Clone());
  UniqueFEIRStmt stmtDAssign = FEIRBuilder::CreateStmtDAssign(std::move(varTarget), std::move(exprDRead));
  mirStmts = stmtDAssign->GenMIRStmts(mirBuilder);
  if (!FEOptions::GetInstance().IsNoBarrier() && fieldInfo.IsVolatile()) {
    StmtNode *barrier = mirBuilder.GetMirModule().CurFuncCodeMemPool()->New<StmtNode>(OP_membarrelease);
    mirStmts.emplace_front(barrier);
    barrier = mirBuilder.GetMirModule().CurFuncCodeMemPool()->New<StmtNode>(OP_membarstoreload);
    mirStmts.emplace_back(barrier);
  }
  TyIdx containerTyIdx = fieldInfo.GetActualContainerType()->GenerateMIRType()->GetTypeIndex();
  if (!mirBuilder.GetCurrentFunction()->IsClinit() ||
      mirBuilder.GetCurrentFunction()->GetClassTyIdx() != containerTyIdx) {
    MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    uint32 typeID = UINT32_MAX;
    bool needMCCForStatic = false;
    if (FEOptions::GetInstance().IsAOT()) {
      needMCCForStatic = NeedMCCForStatic(typeID);
      if (!needMCCForStatic) {
        BaseNode *argNumExpr = mirBuilder.CreateIntConst(static_cast<int32>(typeID), PTY_i32);
        args.push_back(argNumExpr);
      } else {
        const auto &pt = fieldInfo.GetType()->GetPrimType();
        std::map<PrimType, GStrIdx> primTypeFuncNameIdxMap;
        InitPrimTypeFuncNameIdxMap(primTypeFuncNameIdxMap);
        const auto &itorFunc = primTypeFuncNameIdxMap.find(pt);
        CHECK_FATAL(itorFunc != primTypeFuncNameIdxMap.end(), "java type not support %d", pt);
        args.push_back(mirBuilder.CreateIntConst(static_cast<int32>(fieldInfo.GetFieldID()), PTY_i32));
        BaseNode *nodeSrc = mirBuilder.CreateExprDread(*symSrc);
        args.push_back(nodeSrc);
        MIRSymbol *retVarSym = nullptr;
        retVarSym = varField->GenerateLocalMIRSymbol(mirBuilder);
        StmtNode *stmtMCC = mirBuilder.CreateStmtCallAssigned(
            FEManager::GetTypeManager().GetMCCFunction(itorFunc->second)->GetPuidx(), MapleVector<BaseNode*>(args),
            retVarSym,
            OP_callassigned);
        mirStmts.clear();
        mirStmts.emplace_front(stmtMCC);
      }
    }
    if (!needMCCForStatic) {
      StmtNode *stmtClinitCheck = mirBuilder.CreateStmtIntrinsicCall(INTRN_JAVA_CLINIT_CHECK, std::move(args),
          containerTyIdx);
      mirStmts.emplace_front(stmtClinitCheck);
    }
  }
  return mirStmts;
}

std::list<StmtNode*> FEIRStmtFieldStore::GenMIRStmtsImplForNonStatic(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  FieldID fieldID = fieldInfo.GetFieldID();
  ASSERT(fieldID != 0, "invalid field ID");
  MIRStructType *structType = FEManager::GetTypeManager().GetStructTypeFromName(fieldInfo.GetStructName());
  CHECK_NULL_FATAL(structType);
  MIRType *ptrStructType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*structType, PTY_ref);
  UniqueFEIRExpr exprDReadObj = FEIRBuilder::CreateExprDRead(varObj->Clone());
  UniqueFEIRExpr exprDReadField = FEIRBuilder::CreateExprDRead(varField->Clone());
  BaseNode *nodeObj = exprDReadObj->GenMIRNode(mirBuilder);
  BaseNode *nodeField = exprDReadField->GenMIRNode(mirBuilder);
  StmtNode *stmt = mirBuilder.CreateStmtIassign(*ptrStructType, fieldID, nodeObj, nodeField);
  if (FEOptions::GetInstance().IsRC()) {
    bc::RCSetter::GetRCSetter().CollectInputStmtField(stmt, fieldInfo.GetElemNameIdx());
  }
  ans.emplace_back(stmt);
  if (!FEOptions::GetInstance().IsNoBarrier() && fieldInfo.IsVolatile()) {
    StmtNode *barrier = mirBuilder.GetMirModule().CurFuncCodeMemPool()->New<StmtNode>(OP_membarrelease);
    ans.emplace_front(barrier);
    barrier = mirBuilder.GetMirModule().CurFuncCodeMemPool()->New<StmtNode>(OP_membarstoreload);
    ans.emplace_back(barrier);
  }
  return ans;
}

std::string FEIRStmtFieldStore::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtFieldLoad ----------
FEIRStmtFieldLoad::FEIRStmtFieldLoad(UniqueFEIRVar argVarObj, UniqueFEIRVar argVarField,
                                     FEStructFieldInfo &argFieldInfo, bool argIsStatic)
    : FEIRStmtAssign(FEIRNodeKind::kStmtFieldLoad, std::move(argVarField)),
      varObj(std::move(argVarObj)),
      fieldInfo(argFieldInfo),
      isStatic(argIsStatic) {}

FEIRStmtFieldLoad::FEIRStmtFieldLoad(UniqueFEIRVar argVarObj, UniqueFEIRVar argVarField,
                                     FEStructFieldInfo &argFieldInfo,
                                     bool argIsStatic, int32 argDexFileHashCode)
    : FEIRStmtAssign(FEIRNodeKind::kStmtFieldLoad, std::move(argVarField)),
      varObj(std::move(argVarObj)),
      fieldInfo(argFieldInfo),
      isStatic(argIsStatic),
      dexFileHashCode(argDexFileHashCode) {}

void FEIRStmtFieldLoad::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  if (isStatic) {
    RegisterDFGNodes2CheckPointForStatic(checkPoint);
  } else {
    RegisterDFGNodes2CheckPointForNonStatic(checkPoint);
  }
}

bool FEIRStmtFieldLoad::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  if (!isStatic) {
    std::set<UniqueFEIRVar*> &defs = checkPoint.CalcuDef(varObj);
    (void)udChain.insert(std::make_pair(&varObj, defs));
    if (defs.size() == 0) {
      return false;
    }
  }
  return true;
}

void FEIRStmtFieldLoad::RegisterDFGNodes2CheckPointForStatic(FEIRStmtCheckPoint &checkPoint) {
  var->SetDef(true);
  checkPoint.RegisterDFGNode(var);
}

void FEIRStmtFieldLoad::RegisterDFGNodes2CheckPointForNonStatic(FEIRStmtCheckPoint &checkPoint) {
  checkPoint.RegisterDFGNode(varObj);
  var->SetDef(true);
  checkPoint.RegisterDFGNode(var);
}

std::list<StmtNode*> FEIRStmtFieldLoad::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  // prepare and find root
  fieldInfo.Prepare(mirBuilder, isStatic);
  if (isStatic) {
    return GenMIRStmtsImplForStatic(mirBuilder);
  } else {
    return GenMIRStmtsImplForNonStatic(mirBuilder);
  }
}

bool FEIRStmtFieldLoad::NeedMCCForStatic(uint32 &typeID) const {
  // check type first
  const std::string &actualContainerName = fieldInfo.GetActualContainerName();
  typeID = FEManager::GetTypeManager().GetTypeIDFromMplClassName(actualContainerName, dexFileHashCode);
  if(typeID == UINT32_MAX) {
    return true;
  }

  // check type field second
  const std::string &fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(fieldInfo.GetFieldNameIdx());
  MIRStructType *currStructType = FEManager::GetTypeManager().GetStructTypeFromName(actualContainerName);
  if (currStructType == nullptr) {
    return true;
  }
  const auto &fields = currStructType->GetStaticFields();
  if (fields.size() == 0) {
    return true;
  }
  for (auto f : fields) {
    const std::string &fieldNameIt = GlobalTables::GetStrTable().GetStringFromStrIdx(f.first);
    if(fieldName.compare(fieldNameIt) == 0) {
      return false;
    }
  }
  return true;
}

std::list<StmtNode*> FEIRStmtFieldLoad::GenMIRStmtsImplForStatic(MIRBuilder &mirBuilder) const {
  UniqueFEIRVar varTarget = std::make_unique<FEIRVarName>(fieldInfo.GetFieldNameIdx(), fieldInfo.GetType()->Clone());
  varTarget->SetGlobal(true);
  UniqueFEIRExpr exprDRead = FEIRBuilder::CreateExprDRead(std::move(varTarget));
  UniqueFEIRStmt stmtDAssign = FEIRBuilder::CreateStmtDAssign(var->Clone(), std::move(exprDRead));
  std::list<StmtNode*> mirStmts = stmtDAssign->GenMIRStmts(mirBuilder);
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());

  bool needMCCForStatic = false;
  if (FEOptions::GetInstance().IsAOT()) {
    uint32 typeID = UINT32_MAX;
    needMCCForStatic = NeedMCCForStatic(typeID);
    if (!needMCCForStatic) {
      BaseNode *argNumExpr = mirBuilder.CreateIntConst(static_cast<int32>(typeID), PTY_i32);
      args.push_back(argNumExpr);
    } else {
      auto pt = fieldInfo.GetType()->GetPrimType();
      std::map<PrimType, GStrIdx> primTypeFuncNameIdxMap = {
          { PTY_u1, FEUtils::GetMCCStaticFieldGetBoolIdx() },
          { PTY_i8, FEUtils::GetMCCStaticFieldGetByteIdx() },
          { PTY_i16, FEUtils::GetMCCStaticFieldGetShortIdx() },
          { PTY_u16, FEUtils::GetMCCStaticFieldGetCharIdx() },
          { PTY_i32, FEUtils::GetMCCStaticFieldGetIntIdx() },
          { PTY_i64, FEUtils::GetMCCStaticFieldGetLongIdx() },
          { PTY_f32, FEUtils::GetMCCStaticFieldGetFloatIdx() },
          { PTY_f64, FEUtils::GetMCCStaticFieldGetDoubleIdx() },
          { PTY_ref, FEUtils::GetMCCStaticFieldGetObjectIdx() },
      };
      auto itorFunc = primTypeFuncNameIdxMap.find(pt);
      CHECK_FATAL(itorFunc != primTypeFuncNameIdxMap.end(), "java type not support %d", pt);
      args.push_back(mirBuilder.CreateIntConst(static_cast<int32>(fieldInfo.GetFieldID()), PTY_i32));
      MIRSymbol *retVarSym = nullptr;
      retVarSym = var->GenerateLocalMIRSymbol(mirBuilder);
      StmtNode *stmtMCC = mirBuilder.CreateStmtCallAssigned(
          FEManager::GetTypeManager().GetMCCFunction(itorFunc->second)->GetPuidx(), MapleVector<BaseNode*>(args),
          retVarSym, OP_callassigned);
      mirStmts.clear();
      mirStmts.emplace_front(stmtMCC);
    }
  }
  if (!needMCCForStatic) {
    TyIdx containerTyIdx = fieldInfo.GetActualContainerType()->GenerateMIRType()->GetTypeIndex();
    if (!mirBuilder.GetCurrentFunction()->IsClinit() ||
        mirBuilder.GetCurrentFunction()->GetClassTyIdx() != containerTyIdx) {
      StmtNode *stmtClinitCheck = mirBuilder.CreateStmtIntrinsicCall(INTRN_JAVA_CLINIT_CHECK, std::move(args),
          containerTyIdx);
      mirStmts.emplace_front(stmtClinitCheck);
    }
  }
  if (!FEOptions::GetInstance().IsNoBarrier() && fieldInfo.IsVolatile()) {
    StmtNode *barrier = mirBuilder.GetMirModule().CurFuncCodeMemPool()->New<StmtNode>(OP_membaracquire);
    mirStmts.emplace_back(barrier);
  }
  return mirStmts;
}

std::list<StmtNode*> FEIRStmtFieldLoad::GenMIRStmtsImplForNonStatic(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  FieldID fieldID = fieldInfo.GetFieldID();
  ASSERT(fieldID != 0, "invalid field ID");
  MIRStructType *structType = FEManager::GetTypeManager().GetStructTypeFromName(fieldInfo.GetStructName());
  CHECK_NULL_FATAL(structType);
  MIRType *ptrStructType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*structType, PTY_ref);
  MIRType *fieldType = fieldInfo.GetType()->GenerateMIRTypeAuto(fieldInfo.GetSrcLang());
  UniqueFEIRExpr exprDReadObj = FEIRBuilder::CreateExprDRead(varObj->Clone());
  BaseNode *nodeObj = exprDReadObj->GenMIRNode(mirBuilder);
  BaseNode *nodeVal = mirBuilder.CreateExprIread(*fieldType, *ptrStructType, fieldID, nodeObj);
  MIRSymbol *valRet = var->GenerateLocalMIRSymbol(mirBuilder);
  StmtNode *stmt = mirBuilder.CreateStmtDassign(*valRet, 0, nodeVal);
  ans.emplace_back(stmt);
  if (!FEOptions::GetInstance().IsNoBarrier() && fieldInfo.IsVolatile()) {
    StmtNode *barrier = mirBuilder.GetMirModule().CurFuncCodeMemPool()->New<StmtNode>(OP_membaracquire);
    ans.emplace_back(barrier);
  }
  return ans;
}

std::string FEIRStmtFieldLoad::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtCallAssign ----------
std::map<Opcode, Opcode> FEIRStmtCallAssign::mapOpAssignToOp = FEIRStmtCallAssign::InitMapOpAssignToOp();
std::map<Opcode, Opcode> FEIRStmtCallAssign::mapOpToOpAssign = FEIRStmtCallAssign::InitMapOpToOpAssign();

FEIRStmtCallAssign::FEIRStmtCallAssign(FEStructMethodInfo &argMethodInfo, Opcode argMIROp, UniqueFEIRVar argVarRet,
                                       bool argIsStatic)
    : FEIRStmtAssign(FEIRNodeKind::kStmtCallAssign, std::move(argVarRet)),
      methodInfo(argMethodInfo),
      mirOp(argMIROp),
      isStatic(argIsStatic) {}

std::map<Opcode, Opcode> FEIRStmtCallAssign::InitMapOpAssignToOp() {
  std::map<Opcode, Opcode> ans;
  ans[OP_callassigned] = OP_call;
  ans[OP_virtualcallassigned] = OP_virtualcall;
  ans[OP_superclasscallassigned] = OP_superclasscall;
  ans[OP_interfacecallassigned] = OP_interfacecall;
  return ans;
}

std::map<Opcode, Opcode> FEIRStmtCallAssign::InitMapOpToOpAssign() {
  std::map<Opcode, Opcode> ans;
  ans[OP_call] = OP_callassigned;
  ans[OP_virtualcall] = OP_virtualcallassigned;
  ans[OP_superclasscall] = OP_superclasscallassigned;
  ans[OP_interfacecall] = OP_interfacecallassigned;
  return ans;
}

void FEIRStmtCallAssign::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  for (const UniqueFEIRExpr &exprArg : exprArgs) {
    exprArg->RegisterDFGNodes2CheckPoint(checkPoint);
  }
  if (!methodInfo.IsReturnVoid() && (var != nullptr)) {
    var->SetDef(true);
    checkPoint.RegisterDFGNode(var);
  }
}

bool FEIRStmtCallAssign::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  bool success = true;
  for (const UniqueFEIRExpr &exprArg : exprArgs) {
    success = success && exprArg->CalculateDefs4AllUses(checkPoint, udChain);
  }
  return success;
}

std::list<StmtNode*> FEIRStmtCallAssign::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  // If the struct is a class, we check it if external type directly.
  // If the struct is a array type, we check its baseType if external type.
  // FEUtils::GetBaseTypeName returns a class type itself or an arrayType's base type.
  if (methodInfo.GetSrcLang() == kSrcLangJava) {
    std::string baseStructName = FEUtils::GetBaseTypeName(methodInfo.GetStructName());
    bool isCreate = false;
    (void)FEManager::GetTypeManager().GetOrCreateClassOrInterfaceType(
        baseStructName, false, FETypeFlag::kSrcExtern, isCreate);
  }
  std::list<StmtNode*> ans;
  StmtNode *stmtCall = nullptr;
  // prepare and find root
  methodInfo.Prepare(mirBuilder, isStatic);
  if (methodInfo.IsJavaPolymorphicCall() || methodInfo.IsJavaDynamicCall()) {
    return GenMIRStmtsUseZeroReturn(mirBuilder);
  }
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  args.reserve(exprArgs.size());
  size_t index = 0;
  const std::string funcName = methodInfo.GetMirFunc()->GetName();
  for (const UniqueFEIRExpr &exprArg : exprArgs) {
    InsertNonnullCheckingInArgs(exprArg, index++, mirBuilder, ans, funcName);
    ENCChecker::InsertBoundaryAssignChecking(mirBuilder, ans, exprArg, srcFileIndex, srcFileLineNum);
    BaseNode *node = exprArg->GenMIRNode(mirBuilder);
    args.push_back(node);
  }
  PUIdx puIdx = methodInfo.GetPuIdx();
  // for inline optimize
  if (FEManager::GetModule().IsOptFunc(mirBuilder.GetCurrentFunction()) ||
      mirBuilder.GetCurrentFunction()->GetAttr(FUNCATTR_static)) {
    FEManager::GetModule().InsertInlineGlobal(puIdx);
  }
  MIRSymbol *retVarSym = nullptr;
  if (!methodInfo.IsReturnVoid() && var != nullptr) {
    retVarSym = var->GenerateLocalMIRSymbol(mirBuilder);
    InsertNonnullInRetVar(*retVarSym);
  }
  if (retVarSym == nullptr) {
    stmtCall = mirBuilder.CreateStmtCall(puIdx, std::move(args), mirOp);
  } else {
    stmtCall = mirBuilder.CreateStmtCallAssigned(puIdx, std::move(args), retVarSym, mirOp);
  }
  ans.push_back(stmtCall);
  return ans;
}


void FEIRStmtCallAssign::InsertNonnullInRetVar(MIRSymbol &retVarSym) const {
  if (methodInfo.GetMirFunc()->GetFuncAttrs().GetAttr(FUNCATTR_nonnull)) {
    TypeAttrs attrs = TypeAttrs();
    attrs.SetAttr(ATTR_nonnull);
    retVarSym.AddAttrs(attrs);
  }
}

void FEIRStmtCallAssign::InsertNonnullCheckingInArgs(const UniqueFEIRExpr &expr, size_t index,
                                                     MIRBuilder &mirBuilder, std::list<StmtNode*> &ans,
                                                     const std::string& funcName) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic()) {
    return;
  }
  if (index >= methodInfo.GetMirFunc()->GetParamSize()) {  // Skip variable parameter
    return;
  }
  if (!methodInfo.GetMirFunc()->GetNthParamAttr(index).GetAttr(ATTR_nonnull)) {
    return;
  }
  if (ENCChecker::HasNullExpr(expr)) {
    FE_ERR(kLncErr, "%s:%d error: null passed to a callee that requires a nonnull argument, idx: %d",
             FEManager::GetModule().GetFileNameFromFileNum(srcFileIndex).c_str(), srcFileLineNum, index + 1);
    return;
  }
  if ((expr->GetKind() == kExprDRead && expr->GetPrimType() == PTY_ptr) ||
      (expr->GetKind() == kExprIRead && expr->GetFieldID() != 0 && expr->GetPrimType() == PTY_ptr)) {
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtCallAssertNonnull>(OP_callassertnonnull, expr->Clone(),
                                                                      funcName, index);
    std::list<StmtNode*> stmts = stmt->GenMIRStmts(mirBuilder);
    ans.splice(ans.end(), stmts);
  }
}

std::list<StmtNode*> FEIRStmtCallAssign::GenMIRStmtsUseZeroReturn(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  if (methodInfo.IsReturnVoid()) {
    return ans;
  }
  const FEIRType *retType = methodInfo.GetReturnType();
  MIRType *mirRetType = retType->GenerateMIRTypeAuto(kSrcLangJava);
  MIRSymbol *mirRetSym = var->GenerateLocalMIRSymbol(mirBuilder);
  BaseNode *nodeZero;
  if (mirRetType->IsScalarType()) {
    switch (mirRetType->GetPrimType()) {
      case PTY_u1:
      case PTY_i8:
      case PTY_i16:
      case PTY_u16:
      case PTY_i32:
        nodeZero = mirBuilder.CreateIntConst(0, PTY_i32);
        break;
      case PTY_i64:
        nodeZero = mirBuilder.CreateIntConst(0, PTY_i64);
        break;
      case PTY_f32:
        nodeZero = mirBuilder.CreateFloatConst(0.0f);
        break;
      case PTY_f64:
        nodeZero = mirBuilder.CreateDoubleConst(0.0);
        break;
      default:
        nodeZero = mirBuilder.CreateIntConst(0, PTY_i32);
        break;
    }
  } else {
    nodeZero = mirBuilder.CreateIntConst(0, PTY_ref);
  }
  StmtNode *stmt = mirBuilder.CreateStmtDassign(mirRetSym->GetStIdx(), 0, nodeZero);
  ans.push_back(stmt);
  return ans;
}

Opcode FEIRStmtCallAssign::AdjustMIROp() const {
  if (methodInfo.IsReturnVoid()) {
    auto it = mapOpAssignToOp.find(mirOp);
    if (it != mapOpAssignToOp.end()) {
      return it->second;
    }
  } else {
    auto it = mapOpToOpAssign.find(mirOp);
    if (it != mapOpToOpAssign.end()) {
      return it->second;
    }
  }
  return mirOp;
}

std::string FEIRStmtCallAssign::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  if (var != nullptr) {
    ss << " def : " << var->GetNameRaw() << ", ";
  }
  if (exprArgs.size() > 0) {
    ss << " uses : ";
    for (const UniqueFEIRExpr &exprArg : exprArgs) {
      ss << exprArg->DumpDotString() << ", ";
    }
  }
  return ss.str();
}

// ---------- FEIRStmtICallAssign ----------
FEIRStmtICallAssign::FEIRStmtICallAssign()
    : FEIRStmtAssign(FEIRNodeKind::kStmtICallAssign, nullptr) {}

void FEIRStmtICallAssign::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  CHECK_FATAL(false, "NYI");
}

bool FEIRStmtICallAssign::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  bool success = true;
  for (const UniqueFEIRExpr &exprArg : exprArgs) {
    success = success && exprArg->CalculateDefs4AllUses(checkPoint, udChain);
  }
  return success;
}

void FEIRStmtICallAssign::InsertNonnullCheckingInArgs(MIRBuilder &mirBuilder, std::list<StmtNode*> &ans) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || exprArgs.size() <= 1) {
    return;
  }
  const MIRFuncType *funcType = FEUtils::GetFuncPtrType(*exprArgs.front()->GetType()->GenerateMIRType());
  if (funcType == nullptr) {
    return;
  }
  std::vector<TypeAttrs> attrsVec = funcType->GetParamAttrsList();
  int idx = -2; // the first arg is function pointer
  size_t size = funcType->GetParamAttrsList().size();
  for (const auto &expr : exprArgs) {
    ++idx;
    if (idx < 0 || idx >= size || !funcType->GetNthParamAttrs(idx).GetAttr(ATTR_nonnull)) {
      continue;
    }
    if (ENCChecker::HasNullExpr(expr)) {
      FE_ERR(kLncErr, "%s:%d error: null passed to a callee that requires a nonnull argument, idx: %d",
             FEManager::GetModule().GetFileNameFromFileNum(srcFileIndex).c_str(), srcFileLineNum, idx + 1);
      continue;
    }
    if ((expr->GetKind() == kExprDRead && expr->GetPrimType() == PTY_ptr) ||
        (expr->GetKind() == kExprIRead && expr->GetFieldID() != 0 && expr->GetPrimType() == PTY_ptr)) {
      UniqueFEIRStmt stmt = std::make_unique<FEIRStmtCallAssertNonnull>(
          OP_callassertnonnull, expr->Clone(), "function_pointer", idx);
      std::list<StmtNode*> stmts = stmt->GenMIRStmts(mirBuilder);
      ans.splice(ans.end(), stmts);
    }
  }
}

void FEIRStmtICallAssign::InsertNonnullInRetVar(MIRSymbol &retVarSym) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic() || exprArgs.size() < 1) {
    return;
  }
  const MIRFuncType *funcType = FEUtils::GetFuncPtrType(*exprArgs.front()->GetType()->GenerateMIRType());
  if (funcType == nullptr) {
    return;
  }
  if (funcType->GetRetAttrs().GetAttr(ATTR_nonnull)) {
    TypeAttrs attrs = TypeAttrs();
    attrs.SetAttr(ATTR_nonnull);
    retVarSym.AddAttrs(attrs);
  }
}

std::list<StmtNode*> FEIRStmtICallAssign::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  StmtNode *stmtICall = nullptr;
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  args.reserve(exprArgs.size());
  for (const UniqueFEIRExpr &exprArg : exprArgs) {
    BaseNode *node = exprArg->GenMIRNode(mirBuilder);
    ENCChecker::InsertBoundaryAssignChecking(mirBuilder, ans, exprArg, srcFileIndex, srcFileLineNum);
    args.push_back(node);
  }
  InsertNonnullCheckingInArgs(mirBuilder, ans);
  MIRSymbol *retVarSym = nullptr;
  if (var != nullptr) {
    retVarSym = var->GenerateLocalMIRSymbol(mirBuilder);
    InsertNonnullInRetVar(*retVarSym);
    stmtICall = mirBuilder.CreateStmtIcallAssigned(std::move(args), *retVarSym);
  } else {
    stmtICall = mirBuilder.CreateStmtIcall(std::move(args));
  }
  ans.push_back(stmtICall);
  return ans;
}

std::string FEIRStmtICallAssign::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  if (var != nullptr) {
    ss << " def : " << var->GetNameRaw() << ", ";
  }
  if (exprArgs.size() > 0) {
    ss << " uses : ";
    for (const UniqueFEIRExpr &exprArg : exprArgs) {
      ss << exprArg->DumpDotString() << ", ";
    }
  }
  return ss.str();
}

// ---------- FEIRStmtIntrinsicCallAssign ----------
FEIRStmtIntrinsicCallAssign::FEIRStmtIntrinsicCallAssign(MIRIntrinsicID id, UniqueFEIRType typeIn,
                                                         UniqueFEIRVar argVarRet)
    : FEIRStmtAssign(FEIRNodeKind::kStmtIntrinsicCallAssign, std::move(argVarRet)),
      intrinsicId(id),
      type(std::move(typeIn)) {}

FEIRStmtIntrinsicCallAssign::FEIRStmtIntrinsicCallAssign(MIRIntrinsicID id, UniqueFEIRType typeIn,
                                                         UniqueFEIRVar argVarRet,
                                                         std::unique_ptr<std::list<UniqueFEIRExpr>> exprListIn)
    : FEIRStmtAssign(FEIRNodeKind::kStmtIntrinsicCallAssign, std::move(argVarRet)),
      intrinsicId(id),
      type(std::move(typeIn)),
      exprList(std::move(exprListIn)) {}

FEIRStmtIntrinsicCallAssign::FEIRStmtIntrinsicCallAssign(MIRIntrinsicID id, const std::string &funcNameIn,
                                                         const std::string &protoIN,
                                                         std::unique_ptr<std::list<UniqueFEIRVar>> argsIn)
    : FEIRStmtAssign(FEIRNodeKind::kStmtIntrinsicCallAssign, nullptr),
      intrinsicId(id),
      funcName(funcNameIn),
      proto(protoIN),
      polyArgs(std::move(argsIn)) {}
FEIRStmtIntrinsicCallAssign::FEIRStmtIntrinsicCallAssign(MIRIntrinsicID id, const std::string &funcNameIn,
                                                         const std::string &protoIN,
                                                         std::unique_ptr<std::list<UniqueFEIRVar>> argsIn,
                                                         uint32 callerClassTypeIDIn, bool isInStaticFuncIn)
    : FEIRStmtAssign(FEIRNodeKind::kStmtIntrinsicCallAssign, nullptr),
      intrinsicId(id),
      funcName(funcNameIn),
      proto(protoIN),
      polyArgs(std::move(argsIn)),
      callerClassTypeID(callerClassTypeIDIn),
      isInStaticFunc(isInStaticFuncIn) {}

FEIRStmtIntrinsicCallAssign::FEIRStmtIntrinsicCallAssign(MIRIntrinsicID id, UniqueFEIRType typeIn,
                                                         UniqueFEIRVar argVarRet, uint32 typeIDIn)
    : FEIRStmtAssign(FEIRNodeKind::kStmtIntrinsicCallAssign, std::move(argVarRet)),
      intrinsicId(id),
      type(std::move(typeIn)),
      typeID(typeIDIn) {}

std::string FEIRStmtIntrinsicCallAssign::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

void FEIRStmtIntrinsicCallAssign::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  if ((var != nullptr) && (var.get() != nullptr)) {
    var->SetDef(true);
    checkPoint.RegisterDFGNode(var);
  }
}

std::list<StmtNode*> FEIRStmtIntrinsicCallAssign::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  StmtNode *stmtCall = nullptr;
  if (intrinsicId == INTRN_JAVA_CLINIT_CHECK) {
    MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    if (FEOptions::GetInstance().IsAOT()) {
      BaseNode *argNumExpr = mirBuilder.CreateIntConst(static_cast<int64>(typeID), PTY_i32);
      args.push_back(argNumExpr);
    }
    stmtCall = mirBuilder.CreateStmtIntrinsicCall(INTRN_JAVA_CLINIT_CHECK, std::move(args),
                                                  type->GenerateMIRType()->GetTypeIndex());
  } else if (intrinsicId == INTRN_JAVA_FILL_NEW_ARRAY) {
    MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    if (exprList != nullptr) {
      for (const auto &expr : *exprList) {
        BaseNode *node = expr->GenMIRNode(mirBuilder);
        args.push_back(node);
      }
    }
    MIRSymbol *retVarSym = nullptr;
    if ((var != nullptr) && (var.get() != nullptr)) {
      retVarSym = var->GenerateLocalMIRSymbol(mirBuilder);
    }
    stmtCall = mirBuilder.CreateStmtIntrinsicCallAssigned(INTRN_JAVA_FILL_NEW_ARRAY, std::move(args), retVarSym,
                                                          type->GenerateMIRType(true)->GetTypeIndex());
  } else if (intrinsicId == INTRN_JAVA_POLYMORPHIC_CALL) {
    return GenMIRStmtsForInvokePolyMorphic(mirBuilder);
  } else if (intrinsicId == INTRN_C_va_start || intrinsicId == INTRN_C_memcpy) {
    MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    if (exprList != nullptr) {
      for (const auto &expr : *exprList) {
        BaseNode *node = expr->GenMIRNode(mirBuilder);
        if (expr->IsAddrof()) {
          node = ReplaceAddrOfNode(node);  // addrof va_list
        }
        args.push_back(node);
      }
    }
    stmtCall = mirBuilder.CreateStmtIntrinsicCall(intrinsicId, std::move(args), TyIdx(0));
  } else if (intrinsicId == INTRN_C_memset) {
    MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    if (exprList != nullptr) {
      for (const auto &expr : *exprList) {
        BaseNode *node = expr->GenMIRNode(mirBuilder);
        args.push_back(node);
      }
    }
    stmtCall = mirBuilder.CreateStmtIntrinsicCall(INTRN_C_memset, std::move(args), TyIdx(0));
  } else if (intrinsicId >= INTRN_vector_zip_v2i32 && intrinsicId <= INTRN_vector_zip_v2f32) {
    MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    if (exprList != nullptr) {
      for (const auto &expr : *exprList) {
        BaseNode *node = expr->GenMIRNode(mirBuilder);
        args.push_back(node);
      }
    }
    MIRSymbol *retVarSym = nullptr;
    if ((var != nullptr) && (var.get() != nullptr)) {
      retVarSym = var->GenerateLocalMIRSymbol(mirBuilder);
    }
    stmtCall = mirBuilder.CreateStmtIntrinsicCallAssigned(intrinsicId, std::move(args), retVarSym);
  }
  // other intrinsic call should be implemented
  ans.emplace_back(stmtCall);
  return ans;
}

std::list<StmtNode*> FEIRStmtIntrinsicCallAssign::GenMIRStmtsForInvokePolyMorphic(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  StmtNode *stmtCall = nullptr;
  UniqueFEIRVar tmpVar;
  bool needRetypeRet = false;
  MIRSymbol *retVarSym = nullptr;
  if ((var != nullptr) && (var.get() != nullptr)) {
    PrimType ptyRet = var->GetTypeRef().GetPrimType();
    needRetypeRet = (ptyRet == PTY_f32 || ptyRet == PTY_f64);
    tmpVar = var->Clone();
    if (ptyRet == PTY_f32) {
      tmpVar->SetType(std::make_unique<FEIRTypeDefault>(PTY_i32,
                                                        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("I")));
    } else if (ptyRet == PTY_f64) {
      tmpVar->SetType(std::make_unique<FEIRTypeDefault>(PTY_i64,
                                                        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("J")));
    }
    retVarSym = tmpVar->GenerateLocalMIRSymbol(mirBuilder);
  }
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  ConstructArgsForInvokePolyMorphic(mirBuilder, args);
  stmtCall = mirBuilder.CreateStmtIntrinsicCallAssigned(INTRN_JAVA_POLYMORPHIC_CALL, std::move(args), retVarSym);
  ans.emplace_back(stmtCall);
  if (needRetypeRet) {
    UniqueFEIRStmt retypeStmt = FEIRBuilder::CreateStmtRetype(var->Clone(), std::move(tmpVar));
    std::list<StmtNode*> retypeMirStmts = retypeStmt->GenMIRStmts(mirBuilder);
    for (auto elem : retypeMirStmts) {
      ans.emplace_back(elem);
    }
  }
  return ans;
}

void FEIRStmtIntrinsicCallAssign::ConstructArgsForInvokePolyMorphic(MIRBuilder &mirBuilder,
                                                                    MapleVector<BaseNode*> &intrnCallargs) const {
  MIRSymbol *funcNameVal = FEManager::GetJavaStringManager().GetLiteralVar(funcName);
  if (funcNameVal == nullptr) {
    funcNameVal = FEManager::GetJavaStringManager().CreateLiteralVar(mirBuilder, funcName, false);
  }
  BaseNode *dreadExprFuncName = mirBuilder.CreateExprAddrof(0, *funcNameVal);
  dreadExprFuncName->SetPrimType(PTY_ptr);
  intrnCallargs.push_back(dreadExprFuncName);

  MIRSymbol *protoNameVal = FEManager::GetJavaStringManager().GetLiteralVar(proto);
  if (protoNameVal == nullptr) {
    protoNameVal = FEManager::GetJavaStringManager().CreateLiteralVar(mirBuilder, proto, false);
  }
  BaseNode *dreadExprProtoName = mirBuilder.CreateExprAddrof(0, *protoNameVal);
  dreadExprProtoName->SetPrimType(PTY_ptr);
  intrnCallargs.push_back(dreadExprProtoName);

  BaseNode *argNumExpr = mirBuilder.CreateIntConst(static_cast<int64>(polyArgs->size() - 1), PTY_i32);
  intrnCallargs.push_back(argNumExpr);

  bool isAfterMethodHandle = true;
  for (const auto &arg : *polyArgs) {
    intrnCallargs.push_back(mirBuilder.CreateExprDread(*(arg->GenerateMIRSymbol(mirBuilder))));
    if (FEOptions::GetInstance().IsAOT() && isAfterMethodHandle) {
      if (isInStaticFunc) {
        intrnCallargs.push_back(mirBuilder.CreateIntConst(static_cast<int32>(callerClassTypeID), PTY_i32));
      } else {
        std::unique_ptr<FEIRVar> varThisAsLocalVar = std::make_unique<FEIRVarName>(FEUtils::GetThisIdx(),
                                                                                   FEIRTypeDefault(PTY_ref).Clone());
        intrnCallargs.push_back(mirBuilder.CreateExprDread(*(varThisAsLocalVar->GenerateMIRSymbol(mirBuilder))));
      }
      isAfterMethodHandle = false;
    }
  }
}

// ---------- FEIRExpr ----------
FEIRExpr::FEIRExpr(FEIRNodeKind argKind)
    : kind(argKind),
      isNestable(true),
      isAddrof(false),
      hasException(false) {
  type = std::make_unique<FEIRTypeDefault>();
}

FEIRExpr::FEIRExpr(FEIRNodeKind argKind, std::unique_ptr<FEIRType> argType)
    : kind(argKind),
      isNestable(true),
      isAddrof(false),
      hasException(false) {
  SetType(std::move(argType));
}

bool FEIRExpr::IsNestableImpl() const {
  return isNestable;
}

bool FEIRExpr::IsAddrofImpl() const {
  return isAddrof;
}

bool FEIRExpr::HasExceptionImpl() const {
  return hasException;
}

std::vector<FEIRVar*> FEIRExpr::GetVarUsesImpl() const {
  return std::vector<FEIRVar*>();
}

std::string FEIRExpr::DumpDotString() const {
  std::stringstream ss;
  if (kind == FEIRNodeKind::kExprArrayLoad) {
    ss << " kExprArrayLoad: ";
  }
  std::vector<FEIRVar*> varUses = GetVarUses();
  ss << "[ ";
  for (FEIRVar *use : varUses) {
    ss << use->GetNameRaw() << ", ";
  }
  ss << "] ";
  return ss.str();
}

// ---------- FEIRExprConst ----------
FEIRExprConst::FEIRExprConst()
    : FEIRExpr(FEIRNodeKind::kExprConst) {
  ASSERT(type != nullptr, "type is nullptr");
  type->SetPrimType(PTY_i32);
  value.u64 = 0;
}

FEIRExprConst::FEIRExprConst(int64 val, PrimType argType)
    : FEIRExpr(FEIRNodeKind::kExprConst) {
  ASSERT(type != nullptr, "type is nullptr");
  type->SetPrimType(argType);
  value.i64 = val;
  CheckRawValue2SetZero();
}

FEIRExprConst::FEIRExprConst(uint64 val, PrimType argType)
    : FEIRExpr(FEIRNodeKind::kExprConst) {
  ASSERT(type != nullptr, "type is nullptr");
  type->SetPrimType(argType);
  value.u64 = val;
  CheckRawValue2SetZero();
}

FEIRExprConst::FEIRExprConst(uint32 val)
    : FEIRExpr(FEIRNodeKind::kExprConst) {
  type->SetPrimType(PTY_u32);
  value.u32 = val;
  CheckRawValue2SetZero();
}

FEIRExprConst::FEIRExprConst(float val)
    : FEIRExpr(FEIRNodeKind::kExprConst) {
  ASSERT(type != nullptr, "type is nullptr");
  type->SetPrimType(PTY_f32);
  value.f32 = val;
  CheckRawValue2SetZero();
}

FEIRExprConst::FEIRExprConst(double val)
    : FEIRExpr(FEIRNodeKind::kExprConst) {
  ASSERT(type != nullptr, "type is nullptr");
  type->SetPrimType(PTY_f64);
  value.f64 = val;
  CheckRawValue2SetZero();
}

std::unique_ptr<FEIRExpr> FEIRExprConst::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprConst>();
  FEIRExprConst *exprConst = static_cast<FEIRExprConst*>(expr.get());
  exprConst->value.u64 = value.u64;
  ASSERT(type != nullptr, "type is nullptr");
  exprConst->type->SetPrimType(type->GetPrimType());
  exprConst->CheckRawValue2SetZero();
  return expr;
}

BaseNode *FEIRExprConst::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  PrimType primType = GetPrimType();
  switch (primType) {
    case PTY_u1:
    case PTY_u8:
    case PTY_u16:
    case PTY_u32:
      return mirBuilder.CreateIntConst(value.u32, primType);
    case PTY_u64:
    case PTY_i8:
    case PTY_i16:
    case PTY_i32:
    case PTY_i64:
    case PTY_ref:
    case PTY_ptr:
      return mirBuilder.CreateIntConst(value.i64, primType);
    case PTY_f32:
      return mirBuilder.CreateFloatConst(value.f32);
    case PTY_f64:
      return mirBuilder.CreateDoubleConst(value.f64);
    default:
      ERR(kLncErr, "unsupported const kind");
      return nullptr;
  }
}

uint32 FEIRExprConst::HashImpl() const {
  return (static_cast<uint32>(kind) << kOpHashShift) + (type->Hash() << kTypeHashShift) +
         static_cast<uint32>(std::hash<uint64>{}(value.u64));
}

void FEIRExprConst::CheckRawValue2SetZero() {
  if (value.u64 == 0) {
    type->SetZero(true);
  }
}

// ---------- FEIRExprSizeOfType ----------
FEIRExprSizeOfType::FEIRExprSizeOfType(UniqueFEIRType ty)
    : FEIRExpr(FEIRNodeKind::kExprSizeOfType,
               FEIRTypeHelper::CreateTypeNative(*GlobalTables::GetTypeTable().GetPrimType(PTY_u32))),
      feirType(std::move(ty)) {}

std::unique_ptr<FEIRExpr> FEIRExprSizeOfType::CloneImpl() const {
  std::unique_ptr<FEIRExprSizeOfType> expr = std::make_unique<FEIRExprSizeOfType>(feirType->Clone());
  return expr;
}

BaseNode *FEIRExprSizeOfType::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  return mirBuilder.CreateExprSizeoftype(*(feirType->GenerateMIRTypeAuto()));
}

// ---------- FEIRExprDRead ----------
FEIRExprDRead::FEIRExprDRead(std::unique_ptr<FEIRVar> argVarSrc)
    : FEIRExpr(FEIRNodeKind::kExprDRead) {
  SetVarSrc(std::move(argVarSrc));
}

FEIRExprDRead::FEIRExprDRead(std::unique_ptr<FEIRType> argType, std::unique_ptr<FEIRVar> argVarSrc)
    : FEIRExpr(FEIRNodeKind::kExprDRead, std::move(argType)) {
  SetVarSrc(std::move(argVarSrc));
}

std::unique_ptr<FEIRExpr> FEIRExprDRead::CloneImpl() const {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprDRead>(type->Clone(), varSrc->Clone());
  if (fieldType != nullptr) {
    expr->SetFieldType(fieldType->Clone());
  }
  expr->SetFieldID(fieldID);
  return expr;
}

BaseNode *FEIRExprDRead::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MIRType *type = varSrc->GetType()->GenerateMIRTypeAuto();
  MIRSymbol *symbol = varSrc->GenerateMIRSymbol(mirBuilder);
  ASSERT(type != nullptr, "type is nullptr");
  PrimType regType = GetRegPrimType(type->GetPrimType());
  if (regType != type->GetPrimType()) {
    type = GlobalTables::GetTypeTable().GetPrimType(regType);
  }
  AddrofNode *node = mirBuilder.CreateExprDread(*type, *symbol);
  if (fieldID != 0) {
    CHECK_FATAL((type->GetKind() == MIRTypeKind::kTypeStruct || type->GetKind() == MIRTypeKind::kTypeUnion),
                "If fieldID is not 0, then the variable must be a structure");
    CHECK_NULL_FATAL(fieldType);
    MIRType *fieldMIRType = fieldType->GenerateMIRTypeAuto();
    regType = GetRegPrimType(fieldMIRType->GetPrimType());
    if (regType != fieldMIRType->GetPrimType()) {
      fieldMIRType = GlobalTables::GetTypeTable().GetPrimType(regType);
    }
    node = mirBuilder.CreateExprDread(*fieldMIRType, fieldID, *symbol);
  }
  return node;
}

void FEIRExprDRead::SetVarSrc(std::unique_ptr<FEIRVar> argVarSrc) {
  CHECK_FATAL(argVarSrc != nullptr, "input is nullptr");
  varSrc = std::move(argVarSrc);
  SetType(varSrc->GetType()->Clone());
}

std::vector<FEIRVar*> FEIRExprDRead::GetVarUsesImpl() const {
  return std::vector<FEIRVar*>({ varSrc.get() });
}

PrimType FEIRExprDRead::GetPrimTypeImpl() const {
  PrimType primType = type->GetPrimType();
  if (primType == PTY_agg && fieldID != 0) {
    CHECK_NULL_FATAL(fieldType);
    return fieldType->GetPrimType();
  }
  return primType;
}

void FEIRExprDRead::SetTypeImpl(std::unique_ptr<FEIRType> argType) {
  if (fieldID != 0) {
    fieldType = std::move(argType);
  } else {
    type = std::move(argType);
  }
}

FEIRType *FEIRExprDRead::GetTypeImpl() const {
  if (fieldID != 0) {
    CHECK_NULL_FATAL(fieldType);
    return fieldType.get();
  }
  return type.get();
}

const FEIRType &FEIRExprDRead::GetTypeRefImpl() const {
  return *GetTypeImpl();
}

// ---------- FEIRExprIRead ----------
std::unique_ptr<FEIRExpr> FEIRExprIRead::CloneImpl() const {
  std::unique_ptr<FEIRExprIRead> expr = std::make_unique<FEIRExprIRead>(type->Clone(), ptrType->Clone(),
                                                                        fieldID, subExpr->Clone());
  return expr;
}

std::vector<FEIRVar*> FEIRExprIRead::GetVarUsesImpl() const {
  return subExpr->GetVarUses();
}

BaseNode *FEIRExprIRead::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MIRType *returnType = type->GenerateMIRTypeAuto();
  MIRType *pointerType = ptrType->GenerateMIRTypeAuto();
  BaseNode *node = subExpr->GenMIRNode(mirBuilder);
  CHECK_FATAL(pointerType->IsMIRPtrType(), "Must be ptr type!");
  MIRPtrType *mirPtrType = static_cast<MIRPtrType*>(pointerType);
  MIRType *pointedMirType = mirPtrType->GetPointedType();
  if (fieldID != 0) {
    CHECK_FATAL((pointedMirType->GetKind() == MIRTypeKind::kTypeStruct ||
                 pointedMirType->GetKind() == MIRTypeKind::kTypeUnion),
                "If fieldID is not 0, then type must specify pointer to a structure");
  }
  return mirBuilder.CreateExprIread(*returnType, *mirPtrType, fieldID, node);
}

// ---------- FEIRExprAddrofConstArray ----------
FEIRExprAddrofConstArray::FEIRExprAddrofConstArray(const std::vector<uint32> &arrayIn, MIRType *typeIn)
    : FEIRExpr(FEIRNodeKind::kExprAddrof, FEIRTypeHelper::CreateTypeNative(*GlobalTables::GetTypeTable().GetPtrType())),
      arrayName(FEOptions::GetInstance().GetFuncInlineSize() != 0 ? FEUtils::GetSequentialName("const_array_") +
                FEUtils::GetFileNameHashStr(FEManager::GetManager().GetModule().GetFileName()) :
                FEUtils::GetSequentialName("const_array_")),
      elemType(typeIn) {
  std::copy(arrayIn.begin(), arrayIn.end(), std::back_inserter(array));
}

std::unique_ptr<FEIRExpr> FEIRExprAddrofConstArray::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprAddrofConstArray>(arrayName, array, elemType);
  return expr;
}

BaseNode *FEIRExprAddrofConstArray::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MIRType *arrayTypeWithSize = GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, array.size());
  MIRSymbol *arrayVar = mirBuilder.GetOrCreateGlobalDecl(arrayName, *arrayTypeWithSize);
  arrayVar->SetAttr(ATTR_readonly);
  arrayVar->SetStorageClass(kScFstatic);
  FEManager::GetModule().InsertInlineGlobal(arrayVar->GetStIdx().Idx());
  MIRModule &module = mirBuilder.GetMirModule();
  MIRAggConst *val = module.GetMemPool()->New<MIRAggConst>(module, *arrayTypeWithSize);
  for (uint32 i = 0; i < array.size(); ++i) {
    MIRConst *cst = module.GetMemPool()->New<MIRIntConst>(array[i], *elemType);
    val->PushBack(cst);
  }
  // This interface is only for string literal, 0 is added to the end of the string.
  MIRConst *cst0 = module.GetMemPool()->New<MIRIntConst>(0, *elemType);
  val->PushBack(cst0);
  arrayVar->SetKonst(val);
  BaseNode *nodeAddrof = mirBuilder.CreateExprAddrof(0, *arrayVar);
  return nodeAddrof;
}

// ---------- FEIRExprAddrOfLabel ---------
std::unique_ptr<FEIRExpr> FEIRExprAddrOfLabel::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprAddrOfLabel>(labelName, type->Clone());
  return expr;
}

BaseNode *FEIRExprAddrOfLabel::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  LabelIdx lbIdx = mirBuilder.GetOrCreateMIRLabel(labelName);
  BaseNode *mirNode = mirBuilder.GetCurrentFuncCodeMp()->New<AddroflabelNode>(static_cast<uint32>(lbIdx));
  mirNode->SetPrimType(PTY_ptr);
  return mirNode;
}

// ---------- FEIRExprIAddrof ----------
std::unique_ptr<FEIRExpr> FEIRExprIAddrof::CloneImpl() const {
  return std::make_unique<FEIRExprIAddrof>(ptrType->Clone(), fieldID, subExpr->Clone());
}

std::vector<FEIRVar*> FEIRExprIAddrof::GetVarUsesImpl() const {
  return subExpr->GetVarUses();
}

BaseNode *FEIRExprIAddrof::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MIRType *returnType = GlobalTables::GetTypeTable().GetPtrType();
  MIRType *pointerType = ptrType->GenerateMIRTypeAuto();
  BaseNode *node = subExpr->GenMIRNode(mirBuilder);
  CHECK_FATAL(pointerType->IsMIRPtrType(), "Must be ptr type!");
  MIRPtrType *mirPtrType = static_cast<MIRPtrType*>(pointerType);
  MIRType *pointedMirType = mirPtrType->GetPointedType();
  if (fieldID != 0) {
    CHECK_FATAL((pointedMirType->GetKind() == MIRTypeKind::kTypeStruct ||
                 pointedMirType->GetKind() == MIRTypeKind::kTypeUnion),
                "If fieldID is not 0, then type must specify pointer to a structure");
  }
  return mirBuilder.CreateExprIaddrof(*returnType, *mirPtrType, fieldID, node);
}

// ---------- FEIRExprAddrofVar ----------
std::unique_ptr<FEIRExpr> FEIRExprAddrofVar::CloneImpl() const {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprAddrofVar>(varSrc->Clone());
  expr->SetFieldID(fieldID);
  return expr;
}

BaseNode *FEIRExprAddrofVar::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MIRSymbol *varSymbol = varSrc->GenerateMIRSymbol(mirBuilder);
  if (cst != nullptr) {
    varSymbol->SetKonst(cst);
  }
  MIRType *type = varSrc->GetType()->GenerateMIRTypeAuto();
  if (fieldID != 0) {
    CHECK_FATAL((type->IsMIRStructType() || type->GetKind() == MIRTypeKind::kTypeUnion),
                "if fieldID is not 0, then the variable must be a structure");
  }
  AddrofNode *node = mirBuilder.CreateExprAddrof(fieldID, *varSymbol);
  return node;
}

std::vector<FEIRVar*> FEIRExprAddrofVar::GetVarUsesImpl() const {
  return std::vector<FEIRVar*>({ varSrc.get() });
}

// ---------- FEIRExprAddrofFunc ----------
std::unique_ptr<FEIRExpr> FEIRExprAddrofFunc::CloneImpl() const {
  auto expr = std::make_unique<FEIRExprAddrofFunc>(funcAddr);
  return expr;
}

BaseNode *FEIRExprAddrofFunc::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(funcAddr);
  MIRFunction *mirFunc = FEManager::GetTypeManager().GetMIRFunction(strIdx, false);
  CHECK_FATAL(mirFunc != nullptr, "can not get MIRFunction");
  if (FEManager::GetModule().IsOptFunc(mirBuilder.GetCurrentFunction()) ||
      mirBuilder.GetCurrentFunction()->GetAttr(FUNCATTR_static)) {
    mirBuilder.GetMirModule().InsertInlineGlobal(mirFunc->GetPuidx());
  }
  return mirBuilder.CreateExprAddroffunc(mirFunc->GetPuidx(),
                                         mirBuilder.GetMirModule().GetMemPool());
}

// ---------- FEIRExprAddrofArray ----------
FEIRExprAddrofArray::FEIRExprAddrofArray(UniqueFEIRType argTypeNativeArray, UniqueFEIRExpr argExprArray,
                                         std::string argArrayName, std::list<UniqueFEIRExpr> &argExprIndexs)
    : FEIRExpr(FEIRNodeKind::kExprAddrofArray,
               FEIRTypeHelper::CreateTypeNative(*GlobalTables::GetTypeTable().GetPtrType())),
      typeNativeArray(std::move(argTypeNativeArray)),
      exprArray(std::move(argExprArray)),
      arrayName(argArrayName) {
  SetIndexsExprs(argExprIndexs);
}

std::unique_ptr<FEIRExpr> FEIRExprAddrofArray::CloneImpl() const {
  UniqueFEIRType uTypeNativeArray = typeNativeArray->Clone();
  UniqueFEIRExpr uExprArray = exprArray->Clone();
  auto feirExprAddrofArray = std::make_unique<FEIRExprAddrofArray>(std::move(uTypeNativeArray),
                                                                   std::move(uExprArray), arrayName,
                                                                   exprIndexs);
  feirExprAddrofArray->SetIndexsExprs(exprIndexs);
  return feirExprAddrofArray;
}

std::vector<FEIRVar*> FEIRExprAddrofArray::GetVarUsesImpl() const {
  return exprArray->GetVarUses();
}

BaseNode *FEIRExprAddrofArray::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MIRType *ptrMIRArrayType = typeNativeArray->GenerateMIRType(false);
  BaseNode *nodeAddrof = nullptr;
  std::vector<BaseNode*> nds;
  if (!arrayName.empty()) {
    std::unique_ptr<FEIRVar> arrayVar = exprArray->GetVarUses().front()->Clone();
    MIRSymbol *mirSymbol = arrayVar->GenerateMIRSymbol(mirBuilder);
    if (ptrMIRArrayType->GetKind() == kTypeArray) {
      mirSymbol->SetTyIdx(ptrMIRArrayType->GetTypeIndex());
      nodeAddrof = mirBuilder.CreateExprAddrof(0, *mirSymbol);
    } else {
      nodeAddrof = mirBuilder.CreateDread(*mirSymbol, PTY_ptr);
    }
  } else {
    nodeAddrof = exprArray->GenMIRNode(mirBuilder);
  }
  nds.push_back(nodeAddrof);
  for (auto &e : exprIndexs) {
    BaseNode *no = e->GenMIRNode(mirBuilder);
    nds.push_back(no);
  }
  BaseNode *arrayExpr = mirBuilder.CreateExprArray(*ptrMIRArrayType, nds);
  return arrayExpr;
}

// ---------- FEIRExprRegRead ----------
FEIRExprRegRead::FEIRExprRegRead(PrimType pty, int32 regNumIn)
    : FEIRExpr(FEIRNodeKind::kExprRegRead), prmType(pty), regNum(regNumIn) {}

std::unique_ptr<FEIRExpr> FEIRExprRegRead::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprRegRead>(prmType, regNum);
  return expr;
}

BaseNode *FEIRExprRegRead::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  RegreadNode *node = mirBuilder.CreateExprRegread(prmType, regNum);
  return node;
}

void FEIRExprDRead::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  checkPoint.RegisterDFGNode(varSrc);
}

bool FEIRExprDRead::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  std::set<UniqueFEIRVar*> defs = checkPoint.CalcuDef(varSrc);
  (void)udChain.insert(std::make_pair(&varSrc, defs));
  return (defs.size() > 0);
}

// ---------- FEIRExprUnary ----------
std::map<Opcode, bool> FEIRExprUnary::mapOpNestable = FEIRExprUnary::InitMapOpNestableForExprUnary();

FEIRExprUnary::FEIRExprUnary(Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd)
    : FEIRExpr(kExprUnary),
      op(argOp) {
  SetOpnd(std::move(argOpnd));
  SetExprTypeByOp();
}

FEIRExprUnary::FEIRExprUnary(std::unique_ptr<FEIRType> argType, Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd)
    : FEIRExpr(kExprUnary, std::move(argType)),
      op(argOp) {
  SetOpnd(std::move(argOpnd));
}

std::map<Opcode, bool> FEIRExprUnary::InitMapOpNestableForExprUnary() {
  std::map<Opcode, bool> ans;
  ans[OP_abs] = true;
  ans[OP_bnot] = true;
  ans[OP_lnot] = true;
  ans[OP_neg] = true;
  ans[OP_recip] = true;
  ans[OP_sqrt] = true;
  ans[OP_gcmallocjarray] = false;
  return ans;
}

std::unique_ptr<FEIRExpr> FEIRExprUnary::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprUnary>(type->Clone(), op, opnd->Clone());
  return expr;
}

BaseNode *FEIRExprUnary::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
      TyIdx(static_cast<uint32>(GetTypeRef().GetPrimType())));
  ASSERT(mirType != nullptr, "mir type is nullptr");
  BaseNode *nodeOpnd = opnd->GenMIRNode(mirBuilder);
  BaseNode *expr = mirBuilder.CreateExprUnary(op, *mirType, nodeOpnd);
  return expr;
}

void FEIRExprUnary::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  opnd->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRExprUnary::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  return opnd->CalculateDefs4AllUses(checkPoint, udChain);
}

std::vector<FEIRVar*> FEIRExprUnary::GetVarUsesImpl() const {
  return opnd->GetVarUses();
}

void FEIRExprUnary::SetOpnd(std::unique_ptr<FEIRExpr> argOpnd) {
  CHECK_FATAL(argOpnd != nullptr, "opnd is nullptr");
  opnd = std::move(argOpnd);
}

const UniqueFEIRExpr &FEIRExprUnary::GetOpnd() const {
  return opnd;
}

void FEIRExprUnary::SetExprTypeByOp() {
  switch (op) {
    case OP_neg:
    case OP_bnot:
    case OP_lnot:
      type->SetPrimType(opnd->GetPrimType());
      break;
    default:
      break;
  }
}

// ---------- FEIRExprTypeCvt ----------
std::map<Opcode, bool> FEIRExprTypeCvt::mapOpNestable = FEIRExprTypeCvt::InitMapOpNestableForTypeCvt();
std::map<Opcode, FEIRExprTypeCvt::FuncPtrGenMIRNode> FEIRExprTypeCvt::funcPtrMapForParseExpr =
    FEIRExprTypeCvt::InitFuncPtrMapForParseExpr();

FEIRExprTypeCvt::FEIRExprTypeCvt(Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd)
    : FEIRExprUnary(argOp, std::move(argOpnd)) {}

FEIRExprTypeCvt::FEIRExprTypeCvt(std::unique_ptr<FEIRType> exprType, Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd)
    : FEIRExprUnary(std::move(exprType), argOp, std::move(argOpnd)) {}

std::unique_ptr<FEIRExpr> FEIRExprTypeCvt::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprTypeCvt>(type->Clone(), op, opnd->Clone());
  static_cast<FEIRExprTypeCvt*>(expr.get())->SetSrcPrimType(srcPrimType);
  return expr;
}

BaseNode *FEIRExprTypeCvt::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  auto ptrFunc = funcPtrMapForParseExpr.find(op);
  ASSERT(ptrFunc != funcPtrMapForParseExpr.end(), "unsupported op: %s", kOpcodeInfo.GetName(op).c_str());
  return (this->*(ptrFunc->second))(mirBuilder);
}

void FEIRExprTypeCvt::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  opnd->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRExprTypeCvt::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  return opnd->CalculateDefs4AllUses(checkPoint, udChain);
}

Opcode FEIRExprTypeCvt::ChooseOpcodeByFromVarAndToVar(const FEIRVar &fromVar, const FEIRVar &toVar) {
  if ((fromVar.GetType()->IsRef()) && (toVar.GetType()->IsRef())) {
    return OP_retype;
  }
  return OP_retype;
}

BaseNode *FEIRExprTypeCvt::GenMIRNodeMode1(MIRBuilder &mirBuilder) const {
  // MIR: op <to-type> <from-type> (<opnd0>)
  MIRType *mirTypeDst =
      GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(static_cast<uint32>(GetTypeRef().GetPrimType())));
  MIRType *mirTypeSrc = nullptr;
  if (srcPrimType == PTY_unknown) {
    mirTypeSrc = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(static_cast<uint32>(opnd->GetPrimType())));
  } else {
    mirTypeSrc = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(static_cast<uint32>(srcPrimType)));
  }
  BaseNode *nodeOpnd = opnd->GenMIRNode(mirBuilder);
  BaseNode *expr = mirBuilder.CreateExprTypeCvt(op, *mirTypeDst, *mirTypeSrc, nodeOpnd);
  return expr;
}

BaseNode *FEIRExprTypeCvt::GenMIRNodeMode2(MIRBuilder &mirBuilder) const {
  // MIR: op <prim-type> <float-type> (<opnd0>)
  PrimType primTypeSrc = opnd->GetPrimType();
  CHECK_FATAL(IsPrimitiveFloat(primTypeSrc), "from type must be float type");
  return GenMIRNodeMode1(mirBuilder);
}

BaseNode *FEIRExprTypeCvt::GenMIRNodeMode3(MIRBuilder &mirBuilder) const {
  // MIR: retype <prim-type> <type> (<opnd0>)
  MIRType *mirTypeDst = GetTypeRef().GenerateMIRType();
  MIRType *mirTypeSrc = opnd->GetTypeRef().GenerateMIRTypeAuto();
  BaseNode *nodeOpnd = opnd->GenMIRNode(mirBuilder);
  BaseNode *expr = mirBuilder.CreateExprRetype(*mirTypeDst, *mirTypeSrc, nodeOpnd);
  return expr;
}

std::map<Opcode, bool> FEIRExprTypeCvt::InitMapOpNestableForTypeCvt() {
  std::map<Opcode, bool> ans;
  ans[OP_ceil] = true;
  ans[OP_cvt] = true;
  ans[OP_floor] = true;
  ans[OP_retype] = true;
  ans[OP_round] = true;
  ans[OP_trunc] = true;
  return ans;
}

std::map<Opcode, FEIRExprTypeCvt::FuncPtrGenMIRNode> FEIRExprTypeCvt::InitFuncPtrMapForParseExpr() {
  std::map<Opcode, FuncPtrGenMIRNode> ans;
  ans[OP_ceil] = &FEIRExprTypeCvt::GenMIRNodeMode2;
  ans[OP_cvt] = &FEIRExprTypeCvt::GenMIRNodeMode1;
  ans[OP_floor] = &FEIRExprTypeCvt::GenMIRNodeMode2;
  ans[OP_retype] = &FEIRExprTypeCvt::GenMIRNodeMode3;
  ans[OP_round] = &FEIRExprTypeCvt::GenMIRNodeMode2;
  ans[OP_trunc] = &FEIRExprTypeCvt::GenMIRNodeMode2;
  return ans;
}

// ---------- FEIRExprExtractBits ----------
std::map<Opcode, bool> FEIRExprExtractBits::mapOpNestable = FEIRExprExtractBits::InitMapOpNestableForExtractBits();
std::map<Opcode, FEIRExprExtractBits::FuncPtrGenMIRNode> FEIRExprExtractBits::funcPtrMapForParseExpr =
    FEIRExprExtractBits::InitFuncPtrMapForParseExpr();

FEIRExprExtractBits::FEIRExprExtractBits(Opcode argOp, PrimType argPrimType, uint8 argBitOffset, uint8 argBitSize,
                                         std::unique_ptr<FEIRExpr> argOpnd)
    : FEIRExprUnary(argOp, std::move(argOpnd)),
      bitOffset(argBitOffset),
      bitSize(argBitSize) {
  CHECK_FATAL(IsPrimitiveInteger(argPrimType), "only integer type is supported");
  type->SetPrimType(argPrimType);
}

FEIRExprExtractBits::FEIRExprExtractBits(Opcode argOp, PrimType argPrimType, std::unique_ptr<FEIRExpr> argOpnd)
    : FEIRExprExtractBits(argOp, argPrimType, 0, 0, std::move(argOpnd)) {}

std::unique_ptr<FEIRExpr> FEIRExprExtractBits::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprExtractBits>(op, type->GetPrimType(), bitOffset, bitSize,
                                                                         opnd->Clone());
  return expr;
}

BaseNode *FEIRExprExtractBits::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  auto ptrFunc = funcPtrMapForParseExpr.find(op);
  ASSERT(ptrFunc != funcPtrMapForParseExpr.end(), "unsupported op: %s", kOpcodeInfo.GetName(op).c_str());
  return (this->*(ptrFunc->second))(mirBuilder);
}

void FEIRExprExtractBits::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  opnd->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRExprExtractBits::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  return opnd->CalculateDefs4AllUses(checkPoint, udChain);
}

std::map<Opcode, bool> FEIRExprExtractBits::InitMapOpNestableForExtractBits() {
  std::map<Opcode, bool> ans;
  ans[OP_extractbits] = true;
  ans[OP_sext] = true;
  ans[OP_zext] = true;
  return ans;
}

std::map<Opcode, FEIRExprExtractBits::FuncPtrGenMIRNode> FEIRExprExtractBits::InitFuncPtrMapForParseExpr() {
  std::map<Opcode, FuncPtrGenMIRNode> ans;
  ans[OP_extractbits] = &FEIRExprExtractBits::GenMIRNodeForExtrabits;
  ans[OP_sext] = &FEIRExprExtractBits::GenMIRNodeForExt;
  ans[OP_zext] = &FEIRExprExtractBits::GenMIRNodeForExt;
  return ans;
}

BaseNode *FEIRExprExtractBits::GenMIRNodeForExtrabits(MIRBuilder &mirBuilder) const {
  ASSERT(opnd != nullptr, "nullptr check");
  PrimType primTypeDst = GetTypeRef().GetPrimType();
  PrimType primTypeSrc = opnd->GetPrimType();
  CHECK_FATAL(FEUtils::IsInteger(primTypeDst), "dst type of extrabits must integer");
  CHECK_FATAL(FEUtils::IsInteger(primTypeSrc), "src type of extrabits must integer");
  uint8 widthDst = FEUtils::GetWidth(primTypeDst);
  uint8 widthSrc = FEUtils::GetWidth(primTypeSrc);
  CHECK_FATAL(widthDst >= bitSize, "dst width is not enough");
  CHECK_FATAL(widthSrc >= bitOffset + bitSize, "src width is not enough");
  MIRType *mirTypeDst = GetTypeRef().GenerateMIRTypeAuto();
  BaseNode *nodeOpnd = opnd->GenMIRNode(mirBuilder);
  BaseNode *expr = mirBuilder.CreateExprExtractbits(op, *mirTypeDst, bitOffset, bitSize, nodeOpnd);
  return expr;
}

BaseNode *FEIRExprExtractBits::GenMIRNodeForExt(MIRBuilder &mirBuilder) const {
  ASSERT(opnd != nullptr, "nullptr check");
  PrimType primTypeDst = GetTypeRef().GetPrimType();
  CHECK_FATAL(FEUtils::IsInteger(primTypeDst), "dst type of sext/zext must integer");
  uint8 widthDst = FEUtils::GetWidth(primTypeDst);
  // The bit size must be smaller than 32. This parameter is used only when the value of src or dst is smaller than 32.
  if (widthDst >= 32) {
    widthDst = FEUtils::GetWidth(opnd->GetPrimType());
  }
  BaseNode *nodeOpnd = opnd->GenMIRNode(mirBuilder);
  PrimType extPty;
  if (op == OP_zext) {
    extPty = GetUnsignedPrimType(GetRegPrimType(nodeOpnd->GetPrimType()));
  } else {
    extPty = GetSignedPrimType(GetRegPrimType(nodeOpnd->GetPrimType()));
  }
  MIRType *mirTypeDst = GlobalTables::GetTypeTable().GetPrimType(extPty);
  BaseNode *expr = mirBuilder.CreateExprExtractbits(op, *mirTypeDst, 0, widthDst, nodeOpnd);
  return expr;
}

// ---------- FEIRExprBinary ----------
std::map<Opcode, FEIRExprBinary::FuncPtrGenMIRNode> FEIRExprBinary::funcPtrMapForGenMIRNode =
    FEIRExprBinary::InitFuncPtrMapForGenMIRNode();

FEIRExprBinary::FEIRExprBinary(Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd0, std::unique_ptr<FEIRExpr> argOpnd1)
    : FEIRExpr(FEIRNodeKind::kExprBinary),
      op(argOp) {
  SetOpnd0(std::move(argOpnd0));
  SetOpnd1(std::move(argOpnd1));
  SetExprTypeByOp();
}

FEIRExprBinary::FEIRExprBinary(std::unique_ptr<FEIRType> exprType, Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd0,
                               std::unique_ptr<FEIRExpr> argOpnd1)
    : FEIRExpr(FEIRNodeKind::kExprBinary, std::move(exprType)),
      op(argOp) {
  SetOpnd0(std::move(argOpnd0));
  SetOpnd1(std::move(argOpnd1));
  if (FEManager::GetModule().GetSrcLang() != kSrcLangC) {
    SetExprTypeByOp();
  }
}

std::unique_ptr<FEIRExpr> FEIRExprBinary::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprBinary>(type->Clone(), op, opnd0->Clone(), opnd1->Clone());
  return expr;
}

BaseNode *FEIRExprBinary::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  auto ptrFunc = funcPtrMapForGenMIRNode.find(op);
  ASSERT(ptrFunc != funcPtrMapForGenMIRNode.end(), "unsupported op: %s", kOpcodeInfo.GetName(op).c_str());
  return (this->*(ptrFunc->second))(mirBuilder);
}

void FEIRExprBinary::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  opnd0->RegisterDFGNodes2CheckPoint(checkPoint);
  opnd1->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRExprBinary::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  bool success = true;
  success = success && opnd0->CalculateDefs4AllUses(checkPoint, udChain);
  success = success && opnd1->CalculateDefs4AllUses(checkPoint, udChain);
  return success;
}

std::vector<FEIRVar*> FEIRExprBinary::GetVarUsesImpl() const {
  std::vector<FEIRVar*> ans;
  for (FEIRVar *var : opnd0->GetVarUses()) {
    ans.push_back(var);
  }
  for (FEIRVar *var : opnd1->GetVarUses()) {
    ans.push_back(var);
  }
  return ans;
}

bool FEIRExprBinary::IsNestableImpl() const {
  return true;
}

bool FEIRExprBinary::IsAddrofImpl() const {
  return false;
}

void FEIRExprBinary::SetOpnd0(std::unique_ptr<FEIRExpr> argOpnd) {
  CHECK_FATAL(argOpnd != nullptr, "input is nullptr");
  opnd0 = std::move(argOpnd);
}

void FEIRExprBinary::SetOpnd1(std::unique_ptr<FEIRExpr> argOpnd) {
  CHECK_FATAL(argOpnd != nullptr, "input is nullptr");
  opnd1 = std::move(argOpnd);
}

const std::unique_ptr<FEIRExpr> &FEIRExprBinary::GetOpnd0() const {
  return opnd0;
}

const std::unique_ptr<FEIRExpr> &FEIRExprBinary::GetOpnd1() const {
  return opnd1;
}

bool FEIRExprBinary::IsComparative() const {
  switch (op) {
    case OP_cmp:
    case OP_cmpl:
    case OP_cmpg:
    case OP_eq:
    case OP_ge:
    case OP_gt:
    case OP_le:
    case OP_lt:
    case OP_ne:
    // Cand and cior do not need to be converted to comparison.
    case OP_cand:
    case OP_cior:
      return true;
    default:
      return false;
  }
}

std::map<Opcode, FEIRExprBinary::FuncPtrGenMIRNode> FEIRExprBinary::InitFuncPtrMapForGenMIRNode() {
  std::map<Opcode, FEIRExprBinary::FuncPtrGenMIRNode> ans;
  ans[OP_add] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_ashr] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_band] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_bior] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_bxor] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_cand] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_cior] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_cmp] = &FEIRExprBinary::GenMIRNodeCompare;
  ans[OP_cmpg] = &FEIRExprBinary::GenMIRNodeCompare;
  ans[OP_cmpl] = &FEIRExprBinary::GenMIRNodeCompare;
  ans[OP_div] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_eq] = &FEIRExprBinary::GenMIRNodeCompareU1;
  ans[OP_ge] = &FEIRExprBinary::GenMIRNodeCompareU1;
  ans[OP_gt] = &FEIRExprBinary::GenMIRNodeCompareU1;
  ans[OP_land] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_lior] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_le] = &FEIRExprBinary::GenMIRNodeCompareU1;
  ans[OP_lshr] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_lt] = &FEIRExprBinary::GenMIRNodeCompareU1;
  ans[OP_max] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_min] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_mul] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_ne] = &FEIRExprBinary::GenMIRNodeCompareU1;
  ans[OP_rem] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_shl] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_sub] = &FEIRExprBinary::GenMIRNodeNormal;
  ans[OP_ror] = &FEIRExprBinary::GenMIRNodeNormal;
  return ans;
}

BaseNode *FEIRExprBinary::GenMIRNodeNormal(MIRBuilder &mirBuilder) const {
  MIRType *mirTypeDst = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(static_cast<uint32>(type->GetPrimType())));
  UniqueFEIRExpr opnd0FEExpr = opnd0->Clone();
  UniqueFEIRExpr opnd1FEExpr = opnd1->Clone();
  if (op == OP_cand || op == OP_cior) {
    opnd0FEExpr = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(opnd0FEExpr));
    opnd1FEExpr = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(opnd1FEExpr));
  }
  BaseNode *nodeOpnd0 = opnd0FEExpr->GenMIRNode(mirBuilder);
  BaseNode *nodeOpnd1 = opnd1FEExpr->GenMIRNode(mirBuilder);
  BaseNode *expr = mirBuilder.CreateExprBinary(op, *mirTypeDst, nodeOpnd0, nodeOpnd1);
  return expr;
}

BaseNode *FEIRExprBinary::GenMIRNodeCompare(MIRBuilder &mirBuilder) const {
  BaseNode *nodeOpnd0 = opnd0->GenMIRNode(mirBuilder);
  BaseNode *nodeOpnd1 = opnd1->GenMIRNode(mirBuilder);
  MIRType *mirTypeSrc = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
      TyIdx(static_cast<uint32>(nodeOpnd0->GetPrimType())));
  MIRType *mirTypeDst = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(static_cast<uint32>(type->GetPrimType())));
  BaseNode *expr = mirBuilder.CreateExprCompare(op, *mirTypeDst, *mirTypeSrc, nodeOpnd0, nodeOpnd1);
  return expr;
}

BaseNode *FEIRExprBinary::GenMIRNodeCompareU1(MIRBuilder &mirBuilder) const {
  BaseNode *nodeOpnd0 = opnd0->GenMIRNode(mirBuilder);
  BaseNode *nodeOpnd1 = opnd1->GenMIRNode(mirBuilder);
  PrimType srcType0 = nodeOpnd0->GetPrimType();
  PrimType srcType1 = nodeOpnd1->GetPrimType();
  // We take the bigger one as srcType
  PrimType srcType = GetPrimTypeActualBitSize(srcType0) > GetPrimTypeActualBitSize(srcType1) ? srcType0 : srcType1;
  MIRType *mirTypeSrc = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(static_cast<uint32>(srcType)));
  // If the src type is a vector type, there is no need to use u1 to replace
  MIRType *mirTypeDst = type->GenerateMIRTypeAuto();
  if (!PrimitiveType(mirTypeDst->GetPrimType()).IsVector()) {
    mirTypeDst = GlobalTables::GetTypeTable().GetUInt1();
  }
  BaseNode *expr = mirBuilder.CreateExprCompare(op, *mirTypeDst, *mirTypeSrc, nodeOpnd0, nodeOpnd1);
  return expr;
}

void FEIRExprBinary::SetExprTypeByOp() {
  switch (op) {
    // Normal
    case OP_add:
    case OP_div:
    case OP_max:
    case OP_min:
    case OP_mul:
    case OP_rem:
    case OP_sub:
      SetExprTypeByOpNormal();
      break;
    // Shift
    case OP_ashr:
    case OP_lshr:
    case OP_shl:
    case OP_ror:
      SetExprTypeByOpShift();
      break;
    // Logic
    case OP_band:
    case OP_bior:
    case OP_bxor:
    case OP_cand:
    case OP_cior:
    case OP_land:
    case OP_lior:
      SetExprTypeByOpLogic();
      break;
    // Compare
    case OP_cmp:
    case OP_cmpl:
    case OP_cmpg:
    case OP_eq:
    case OP_ge:
    case OP_gt:
    case OP_le:
    case OP_lt:
    case OP_ne:
      SetExprTypeByOpCompare();
      break;
    default:
      break;
  }
}

void FEIRExprBinary::SetExprTypeByOpNormal() {
  PrimType primTypeOpnd0 = opnd0->GetPrimType();
  PrimType primTypeOpnd1 = opnd1->GetPrimType();
  // primTypeOpnd0 == PTY_void for addrof add
  if (primTypeOpnd0 == PTY_ptr || primTypeOpnd1 == PTY_ptr || primTypeOpnd0 == PTY_void) {
    type->SetPrimType(PTY_ptr);
    return;
  }
  type->SetPrimType(primTypeOpnd0);
}

void FEIRExprBinary::SetExprTypeByOpShift() {
  PrimType primTypeOpnd0 = opnd0->GetPrimType();
  PrimType primTypeOpnd1 = opnd1->GetPrimType();
  CHECK_FATAL(IsPrimitiveInteger(primTypeOpnd0), "logic's opnd0 must be integer");
  CHECK_FATAL(IsPrimitiveInteger(primTypeOpnd1), "logic's opnd1 must be integer");
  type->SetPrimType(primTypeOpnd0);
}

void FEIRExprBinary::SetExprTypeByOpLogic() {
  PrimType primTypeOpnd0 = opnd0->GetPrimType();
  CHECK_FATAL(IsPrimitiveInteger(primTypeOpnd0), "logic's opnds must be integer");
  type->SetPrimType(primTypeOpnd0);
}

void FEIRExprBinary::SetExprTypeByOpCompare() {
  type->SetPrimType(PTY_i32);
}

// ---------- FEIRExprTernary ----------
FEIRExprTernary::FEIRExprTernary(Opcode argOp, std::unique_ptr<FEIRExpr> argOpnd0, std::unique_ptr<FEIRExpr> argOpnd1,
                                 std::unique_ptr<FEIRExpr> argOpnd2)
    : FEIRExpr(FEIRNodeKind::kExprTernary),
      op(argOp) {
  SetOpnd(std::move(argOpnd0), 0);
  SetOpnd(std::move(argOpnd1), 1);
  SetOpnd(std::move(argOpnd2), 2);
  SetExprTypeByOp();
}

FEIRExprTernary::FEIRExprTernary(Opcode argOp, std::unique_ptr<FEIRType> argType, std::unique_ptr<FEIRExpr> argOpnd0,
                                 std::unique_ptr<FEIRExpr> argOpnd1, std::unique_ptr<FEIRExpr> argOpnd2)
    : FEIRExpr(FEIRNodeKind::kExprTernary, std::move(argType)),
      op(argOp) {
  SetOpnd(std::move(argOpnd0), 0);
  SetOpnd(std::move(argOpnd1), 1);
  SetOpnd(std::move(argOpnd2), 2);
}

std::unique_ptr<FEIRExpr> FEIRExprTernary::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprTernary>(op, type->Clone(), opnd0->Clone(), opnd1->Clone(),
                                                                     opnd2->Clone());
  return expr;
}

BaseNode *FEIRExprTernary::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MIRType *mirTypeDst = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(static_cast<uint32>(type->GetPrimType())));
  BaseNode *nodeOpnd0 = opnd0->GenMIRNode(mirBuilder);
  BaseNode *nodeOpnd1 = opnd1->GenMIRNode(mirBuilder);
  BaseNode *nodeOpnd2 = opnd2->GenMIRNode(mirBuilder);
  BaseNode *expr = mirBuilder.CreateExprTernary(op, *mirTypeDst, nodeOpnd0, nodeOpnd1, nodeOpnd2);
  return expr;
}

void FEIRExprTernary::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  opnd0->RegisterDFGNodes2CheckPoint(checkPoint);
  opnd1->RegisterDFGNodes2CheckPoint(checkPoint);
  opnd2->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRExprTernary::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  bool success = true;
  success = success && opnd0->CalculateDefs4AllUses(checkPoint, udChain);
  success = success && opnd1->CalculateDefs4AllUses(checkPoint, udChain);
  success = success && opnd2->CalculateDefs4AllUses(checkPoint, udChain);
  return success;
}

std::vector<FEIRVar*> FEIRExprTernary::GetVarUsesImpl() const {
  std::vector<FEIRVar*> ans;
  for (FEIRVar *var : opnd0->GetVarUses()) {
    ans.push_back(var);
  }
  for (FEIRVar *var : opnd1->GetVarUses()) {
    ans.push_back(var);
  }
  for (FEIRVar *var : opnd2->GetVarUses()) {
    ans.push_back(var);
  }
  return ans;
}

bool FEIRExprTernary::IsNestableImpl() const {
  return true;
}

bool FEIRExprTernary::IsAddrofImpl() const {
  return false;
}

void FEIRExprTernary::SetOpnd(std::unique_ptr<FEIRExpr> argOpnd, uint32 idx) {
  CHECK_FATAL(argOpnd != nullptr, "input is nullptr");
  switch (idx) {
    case 0:
      opnd0 = std::move(argOpnd);
      break;
    case 1:
      opnd1 = std::move(argOpnd);
      break;
    case 2:
      opnd2 = std::move(argOpnd);
      break;
    default:
      CHECK_FATAL(false, "index out of range");
  }
}

void FEIRExprTernary::SetExprTypeByOp() {
  PrimType primTypeOpnd1 = opnd1->GetPrimType();
  type->SetPrimType(primTypeOpnd1);
}

// ---------- FEIRExprNary ----------
FEIRExprNary::FEIRExprNary(Opcode argOp)
    : FEIRExpr(FEIRNodeKind::kExprNary),
      op(argOp) {}

std::vector<FEIRVar*> FEIRExprNary::GetVarUsesImpl() const {
  std::vector<FEIRVar*> ans;
  for (const std::unique_ptr<FEIRExpr> &opnd : opnds) {
    for (FEIRVar *var : opnd->GetVarUses()) {
      ans.push_back(var);
    }
  }
  return ans;
}

void FEIRExprNary::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  for (const std::unique_ptr<FEIRExpr> &opnd : opnds) {
    opnd->RegisterDFGNodes2CheckPoint(checkPoint);
  }
}

bool FEIRExprNary::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  bool success = true;
  for (const std::unique_ptr<FEIRExpr> &opnd : opnds) {
    success = success && opnd->CalculateDefs4AllUses(checkPoint, udChain);
  }
  return success;
}

void FEIRExprNary::AddOpnd(std::unique_ptr<FEIRExpr> argOpnd) {
  CHECK_FATAL(argOpnd != nullptr, "input opnd is nullptr");
  opnds.push_back(std::move(argOpnd));
}

void FEIRExprNary::AddOpnds(const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds) {
  for (const std::unique_ptr<FEIRExpr> &opnd : argOpnds) {
    ASSERT_NOT_NULL(opnd);
    AddOpnd(opnd->Clone());
  }
}

void FEIRExprNary::ResetOpnd() {
  opnds.clear();
}

// ---------- FEIRExprIntrinsicop ----------
FEIRExprIntrinsicop::FEIRExprIntrinsicop(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID)
    : FEIRExprNary(OP_intrinsicop),
      intrinsicID(argIntrinsicID) {
  kind = FEIRNodeKind::kExprIntrinsicop;
  SetType(std::move(exprType));
}

FEIRExprIntrinsicop::FEIRExprIntrinsicop(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID,
                                         std::unique_ptr<FEIRType> argParamType)
    : FEIRExprNary(OP_intrinsicopwithtype),
      intrinsicID(argIntrinsicID) {
  kind = FEIRNodeKind::kExprIntrinsicop;
  SetType(std::move(exprType));
  paramType = std::move(argParamType);
}

FEIRExprIntrinsicop::FEIRExprIntrinsicop(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID,
                                         const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds)
    : FEIRExprIntrinsicop(std::move(exprType), argIntrinsicID) {
  AddOpnds(argOpnds);
}

FEIRExprIntrinsicop::FEIRExprIntrinsicop(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID,
                                         std::unique_ptr<FEIRType> argParamType,
                                         const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds)
    : FEIRExprIntrinsicop(std::move(exprType), argIntrinsicID, std::move(argParamType)) {
  AddOpnds(argOpnds);
}

FEIRExprIntrinsicop::FEIRExprIntrinsicop(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID,
                                         std::unique_ptr<FEIRType> argParamType, uint32 argTypeID)
    : FEIRExprNary(OP_intrinsicopwithtype),
      intrinsicID(argIntrinsicID),
      typeID(argTypeID) {
  kind = FEIRNodeKind::kExprIntrinsicop;
  SetType(std::move(exprType));
  paramType = std::move(argParamType);
}

std::unique_ptr<FEIRExpr> FEIRExprIntrinsicop::CloneImpl() const {
  if (op == OP_intrinsicop) {
    return std::make_unique<FEIRExprIntrinsicop>(type->Clone(), intrinsicID, opnds);
  } else {
    CHECK_FATAL(paramType != nullptr, "error: param type is not set");
    return std::make_unique<FEIRExprIntrinsicop>(type->Clone(), intrinsicID, paramType->Clone(), opnds);
  }
}

BaseNode *FEIRExprIntrinsicop::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  for (const auto &e : opnds) {
    BaseNode *node = e->GenMIRNode(mirBuilder);
    args.emplace_back(node);
  }
  (void)typeID;
  MIRType *ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(
      *(type->GenerateMIRType()), paramType->IsRef() ? PTY_ref : PTY_ptr);
  BaseNode *expr = mirBuilder.CreateExprIntrinsicop(intrinsicID, op, *ptrType, std::move(args));
  return expr;
}

void FEIRExprIntrinsicop::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  for (const std::unique_ptr<FEIRExpr> &opnd : opnds) {
    opnd->RegisterDFGNodes2CheckPoint(checkPoint);
  }
}

bool FEIRExprIntrinsicop::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  bool success = true;
  for (const std::unique_ptr<FEIRExpr> &opnd : opnds) {
    success = success && opnd->CalculateDefs4AllUses(checkPoint, udChain);
  }
  return success;
}

bool FEIRExprIntrinsicop::IsNestableImpl() const {
  return false;
}

bool FEIRExprIntrinsicop::IsAddrofImpl() const {
  return false;
}

// ---------- FEIRExprIntrinsicopForC ----------
FEIRExprIntrinsicopForC::FEIRExprIntrinsicopForC(std::unique_ptr<FEIRType> exprType, MIRIntrinsicID argIntrinsicID,
                                                 const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds)
    : FEIRExprNary(OP_intrinsicop),
      intrinsicID(argIntrinsicID) {
  kind = FEIRNodeKind::kExprIntrinsicop;
  SetType(std::move(exprType));
  AddOpnds(argOpnds);
}

std::unique_ptr<FEIRExpr> FEIRExprIntrinsicopForC::CloneImpl() const {
  return std::make_unique<FEIRExprIntrinsicopForC>(type->Clone(), intrinsicID, opnds);
}

BaseNode *FEIRExprIntrinsicopForC::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  for (const auto &e : opnds) {
    BaseNode *node = e->GenMIRNode(mirBuilder);
    args.emplace_back(node);
  }
  return mirBuilder.CreateExprIntrinsicop(intrinsicID, op,
                                          *type->GenerateMIRTypeAuto(), MapleVector<BaseNode*>(args));
}

// ---------- FEIRExprJavaMerge ----------------
FEIRExprJavaMerge::FEIRExprJavaMerge(std::unique_ptr<FEIRType> mergedTypeArg,
                                     const std::vector<std::unique_ptr<FEIRExpr>> &argOpnds)
    : FEIRExprNary(OP_intrinsicop) {
  SetType(std::move(mergedTypeArg));
  AddOpnds(argOpnds);
}

std::unique_ptr<FEIRExpr> FEIRExprJavaMerge::CloneImpl() const {
  return std::make_unique<FEIRExprJavaMerge>(type->Clone(), opnds);
}

void FEIRExprJavaMerge::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  for (const std::unique_ptr<FEIRExpr> &opnd : opnds) {
    opnd->RegisterDFGNodes2CheckPoint(checkPoint);
  }
}

bool FEIRExprJavaMerge::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  bool success = true;
  for (const std::unique_ptr<FEIRExpr> &opnd : opnds) {
    success = success && opnd->CalculateDefs4AllUses(checkPoint, udChain);
  }
  return success;
}

BaseNode *FEIRExprJavaMerge::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  args.reserve(opnds.size());
  for (const auto &e : opnds) {
    BaseNode *node = e->GenMIRNode(mirBuilder);
    args.emplace_back(node);
  }
  // (intrinsicop u1 JAVA_MERGE (dread i32 %Reg0_I))
  IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[INTRN_JAVA_MERGE];
  MIRType *retType = intrinDesc->GetReturnType();
  BaseNode *intr = mirBuilder.CreateExprIntrinsicop(INTRN_JAVA_MERGE, op, *retType, std::move(args));
  intr->SetPrimType(type->GetPrimType());
  return intr;
}

// ---------- FEIRExprJavaNewInstance ----------
FEIRExprJavaNewInstance::FEIRExprJavaNewInstance(UniqueFEIRType argType)
    : FEIRExpr(FEIRNodeKind::kExprJavaNewInstance) {
  SetType(std::move(argType));
}

FEIRExprJavaNewInstance::FEIRExprJavaNewInstance(UniqueFEIRType argType, uint32 argTypeID)
    : FEIRExpr(FEIRNodeKind::kExprJavaNewInstance), typeID(argTypeID) {
  SetType(std::move(argType));
}

FEIRExprJavaNewInstance::FEIRExprJavaNewInstance(UniqueFEIRType argType, uint32 argTypeID, bool argIsRcPermanent)
    : FEIRExpr(FEIRNodeKind::kExprJavaNewInstance), typeID(argTypeID), isRcPermanent(argIsRcPermanent) {
  SetType(std::move(argType));
}

std::unique_ptr<FEIRExpr> FEIRExprJavaNewInstance::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprJavaNewInstance>(type->Clone());
  CHECK_NULL_FATAL(expr);
  return expr;
}

BaseNode *FEIRExprJavaNewInstance::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MIRType *mirType = type->GenerateMIRType(kSrcLangJava, false);
  BaseNode *expr = nullptr;
  MIRType *ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirType, PTY_ref);
  Opcode opMalloc = isRcPermanent ? OP_gcpermalloc : OP_gcmalloc;
  expr = mirBuilder.CreateExprGCMalloc(opMalloc, *ptrType, *mirType);
  CHECK_NULL_FATAL(expr);
  return expr;
}

// ---------- FEIRExprJavaNewArray ----------
FEIRExprJavaNewArray::FEIRExprJavaNewArray(UniqueFEIRType argArrayType, UniqueFEIRExpr argExprSize)
    : FEIRExpr(FEIRNodeKind::kExprJavaNewArray) {
  SetArrayType(std::move(argArrayType));
  SetExprSize(std::move(argExprSize));
}

FEIRExprJavaNewArray::FEIRExprJavaNewArray(UniqueFEIRType argArrayType, UniqueFEIRExpr argExprSize, uint32 argTypeID)
    : FEIRExpr(FEIRNodeKind::kExprJavaNewArray), typeID(argTypeID) {
  SetArrayType(std::move(argArrayType));
  SetExprSize(std::move(argExprSize));
}

FEIRExprJavaNewArray::FEIRExprJavaNewArray(UniqueFEIRType argArrayType, UniqueFEIRExpr argExprSize, uint32 argTypeID,
                                           bool argIsRcPermanent)
    : FEIRExpr(FEIRNodeKind::kExprJavaNewArray), typeID(argTypeID), isRcPermanent(argIsRcPermanent) {
  SetArrayType(std::move(argArrayType));
  SetExprSize(std::move(argExprSize));
}

std::unique_ptr<FEIRExpr> FEIRExprJavaNewArray::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprJavaNewArray>(arrayType->Clone(), exprSize->Clone());
  CHECK_NULL_FATAL(expr);
  return expr;
}

std::vector<FEIRVar*> FEIRExprJavaNewArray::GetVarUsesImpl() const {
  return exprSize->GetVarUses();
}

BaseNode *FEIRExprJavaNewArray::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  UniqueFEIRType elemType = FEIRBuilder::CreateArrayElemType(arrayType);
  MIRType *elemMirType = elemType->GenerateMIRType(kSrcLangJava, false);
  if (!elemMirType->IsScalarType()) {
    elemMirType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemMirType, PTY_ptr);
  }
  MIRType *jarrayType = GlobalTables::GetTypeTable().GetOrCreateJarrayType(*elemMirType);
  (void)typeID;
  MIRType *mirType = arrayType->GenerateMIRType(kSrcLangJava, false);
  MIRType *ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirType, PTY_ref);
  BaseNode *sizeNode = exprSize->GenMIRNode(mirBuilder);
  Opcode opMalloc = isRcPermanent ? OP_gcpermallocjarray : OP_gcmallocjarray;
  BaseNode *expr = mirBuilder.CreateExprJarrayMalloc(opMalloc, *ptrType, *jarrayType, sizeNode);
  CHECK_NULL_FATAL(expr);
  return expr;
}

void FEIRExprJavaNewArray::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  exprSize->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRExprJavaNewArray::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  return exprSize->CalculateDefs4AllUses(checkPoint, udChain);
}

// ---------- FEIRExprJavaArrayLength ----------
FEIRExprJavaArrayLength::FEIRExprJavaArrayLength(UniqueFEIRExpr argExprArray)
    : FEIRExpr(FEIRNodeKind::kExprJavaArrayLength) {
  SetExprArray(std::move(argExprArray));
}

std::unique_ptr<FEIRExpr> FEIRExprJavaArrayLength::CloneImpl() const {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprJavaArrayLength>(exprArray->Clone());
  CHECK_NULL_FATAL(expr);
  return expr;
}

std::vector<FEIRVar*> FEIRExprJavaArrayLength::GetVarUsesImpl() const {
  return exprArray->GetVarUses();
}

BaseNode *FEIRExprJavaArrayLength::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  BaseNode *arrayNode = exprArray->GenMIRNode(mirBuilder);
  MapleVector<BaseNode*> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  args.push_back(arrayNode);
  MIRType *retType = GlobalTables::GetTypeTable().GetInt32();
  return mirBuilder.CreateExprIntrinsicop(INTRN_JAVA_ARRAY_LENGTH, OP_intrinsicop, *retType, std::move(args));
}

void FEIRExprJavaArrayLength::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  exprArray->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRExprJavaArrayLength::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  return exprArray->CalculateDefs4AllUses(checkPoint, udChain);
}

// ---------- FEIRExprArrayLoad ----------
FEIRExprArrayLoad::FEIRExprArrayLoad(UniqueFEIRExpr argExprArray, UniqueFEIRExpr argExprIndex,
                                     UniqueFEIRType argTypeArray)
    : FEIRExpr(FEIRNodeKind::kExprArrayLoad),
      exprArray(std::move(argExprArray)),
      exprIndex(std::move(argExprIndex)),
      typeArray(std::move(argTypeArray)) {}

std::unique_ptr<FEIRExpr> FEIRExprArrayLoad::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprArrayLoad>(exprArray->Clone(), exprIndex->Clone(),
                                                                       typeArray->Clone());
  return expr;
}

BaseNode *FEIRExprArrayLoad::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  CHECK_FATAL(exprArray->GetKind() == kExprDRead, "only support dread expr for exprArray");
  CHECK_FATAL(exprIndex->GetKind() == kExprDRead, "only support dread expr for exprIndex");
  BaseNode *addrBase = exprArray->GenMIRNode(mirBuilder);
  BaseNode *indexBn = exprIndex->GenMIRNode(mirBuilder);
  MIRType *ptrMIRArrayType = typeArray->GenerateMIRType(false);

  BaseNode *arrayExpr = mirBuilder.CreateExprArray(*ptrMIRArrayType, addrBase, indexBn);
  UniqueFEIRType typeElem = typeArray->Clone();
  (void)typeElem->ArrayDecrDim();

  MIRType *mirElemType = typeElem->GenerateMIRType(true);
  MIRType *ptrMIRElemType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirElemType, PTY_ptr);
  BaseNode *elemBn = mirBuilder.CreateExprIread(*mirElemType, *ptrMIRElemType, 0, arrayExpr);
  return elemBn;
}

std::vector<FEIRVar*> FEIRExprArrayLoad::GetVarUsesImpl() const {
  std::vector<FEIRVar*> ans;
  for (FEIRVar *var : exprArray->GetVarUses()) {
    ans.push_back(var);
  }
  for (FEIRVar *var : exprIndex->GetVarUses()) {
    ans.push_back(var);
  }
  return ans;
}

void FEIRExprArrayLoad::RegisterDFGNodes2CheckPointImpl(FEIRStmtCheckPoint &checkPoint) {
  exprArray->RegisterDFGNodes2CheckPoint(checkPoint);
  exprIndex->RegisterDFGNodes2CheckPoint(checkPoint);
}

bool FEIRExprArrayLoad::CalculateDefs4AllUsesImpl(FEIRStmtCheckPoint &checkPoint, FEIRUseDefChain &udChain) {
  bool success = true;
  success = success && exprArray->CalculateDefs4AllUses(checkPoint, udChain);
  success = success && exprIndex->CalculateDefs4AllUses(checkPoint, udChain);
  return success;
}

// ---------- FEIRExprCStyleCast ----------
FEIRExprCStyleCast::FEIRExprCStyleCast(MIRType *src,
                                       MIRType *dest,
                                       UniqueFEIRExpr sub,
                                       bool isArr2Pty)
    : FEIRExpr(FEIRNodeKind::kExprCStyleCast),
      srcType(src),
      destType(dest),
      subExpr(std::move(sub)),
      isArray2Pointer(isArr2Pty) {
  CHECK_NULL_FATAL(dest);
  type = FEIRTypeHelper::CreateTypeNative(*dest);
}

std::unique_ptr<FEIRExpr> FEIRExprCStyleCast::CloneImpl() const {
  auto expr = std::make_unique<FEIRExprCStyleCast>(srcType, destType,
                                                   subExpr->Clone(), isArray2Pointer);
  expr->SetRefName(refName);
  return expr;
}

std::vector<FEIRVar*> FEIRExprCStyleCast::GetVarUsesImpl() const {
  return subExpr->GetVarUses();
}

BaseNode *FEIRExprCStyleCast::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  BaseNode *sub = subExpr.get()->GenMIRNode(mirBuilder);
  BaseNode *cvt = nullptr;
  if (isArray2Pointer) {
    auto *arrayType = static_cast<MIRArrayType*>(srcType);
    ASSERT(arrayType != nullptr, "ERROR:null pointer!");
    ArrayNode *arrayNode = mirBuilder.CreateExprArray(*arrayType);
    MIRSymbol *var = subExpr->GetVarUses().front()->GenerateMIRSymbol(mirBuilder);
    arrayNode->GetNopnd().push_back(mirBuilder.CreateExprAddrof(0, *var));
    for (uint8 i = 0; i < arrayType->GetDim(); ++i) {
      arrayNode->GetNopnd().push_back(mirBuilder.CreateIntConst(0, PTY_i32));
    }
    arrayNode->SetNumOpnds(static_cast<uint8>(arrayType->GetDim() + 1));
    return arrayNode;
  }
  auto isCvtNeeded = [&](const MIRType &fromNode, const MIRType &toNode, const BaseNode &baseNode) {
    if (fromNode.EqualTo(toNode)) {
      return false;
    }
    return true;
  };
  if (!isCvtNeeded(*srcType, *destType, *sub)) {
    return sub;
  }
  if (sub != nullptr && srcType != nullptr && destType != nullptr) {
    PrimType fromType = srcType->GetPrimType();
    PrimType toType = destType->GetPrimType();
    if (fromType == toType || toType == PTY_void || destType->GetKind() == kTypeUnion) {
      return sub;
    }
    if (IsPrimitiveFloat(fromType) && IsPrimitiveInteger(toType)) {
      if (toType == PTY_u1) {
        MIRType *mirTypeU1 = GlobalTables::GetTypeTable().GetUInt1();
        BaseNode *zeroNode = (fromType == PTY_f32) ? mirBuilder.CreateFloatConst(0) : mirBuilder.CreateDoubleConst(0);
        return mirBuilder.CreateExprCompare(OP_ne, *mirTypeU1, *srcType, sub, zeroNode);
      }
      cvt = mirBuilder.CreateExprTypeCvt(OP_trunc, *destType, *srcType, sub);
    } else {
      cvt = mirBuilder.CreateExprTypeCvt(OP_cvt, *destType, *srcType, sub);
    }
  }
  return cvt;
}

// ---------- FEIRExprAtomic ----------
FEIRExprAtomic::FEIRExprAtomic(MIRType *ty, MIRType *ref, UniqueFEIRExpr obj, ASTAtomicOp atomOp)
    : FEIRExpr(FEIRNodeKind::kExprAtomic),
      mirType(ty),
      refType(ref),
      objExpr(std::move(obj)),
      atomicOp(atomOp) {}

std::unique_ptr<FEIRExpr> FEIRExprAtomic::CloneImpl() const {
  std::unique_ptr<FEIRExpr> expr = std::make_unique<FEIRExprAtomic>(mirType, refType, objExpr->Clone(), atomicOp);
  static_cast<FEIRExprAtomic*>(expr.get())->SetVal1Type(val1Type);
  if (valExpr1.get() != nullptr) {
    static_cast<FEIRExprAtomic*>(expr.get())->SetVal1Expr(valExpr1->Clone());
  }
  static_cast<FEIRExprAtomic*>(expr.get())->SetVal1Type(val2Type);
  if (valExpr2.get() != nullptr) {
    static_cast<FEIRExprAtomic*>(expr.get())->SetVal1Expr(valExpr2->Clone());
  }
  return expr;
}

void FEIRExprAtomic::ProcessAtomicBinary(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                                         MIRSymbol &valueVar) const {
  BaseNode *constNode = valExpr1.get()->GenMIRNode(mirBuilder);
  BaseNode *ireadNode = mirBuilder.CreateExprIread(*refType, *mirType, 0, &lockNode);
  BaseNode *valueNode = mirBuilder.CreateExprDread(valueVar);
  StmtNode *storeStmt = mirBuilder.CreateStmtDassign(valueVar, 0, ireadNode);
  block.AddStatement(storeStmt);
  Opcode opcode = OP_add;
  if (atomicOp == kAtomicBinaryOpAdd) {
    opcode = OP_add;
  } else if (atomicOp == kAtomicBinaryOpSub) {
    opcode = OP_sub;
  } else if (atomicOp == kAtomicBinaryOpAnd) {
    opcode = OP_band;
  } else if (atomicOp == kAtomicBinaryOpOr) {
    opcode = OP_bior;
  } else if (atomicOp == kAtomicBinaryOpOr) {
    opcode = OP_bxor;
  } else {
  }
  BinaryNode *binNode = mirBuilder.CreateExprBinary(opcode, *val1Type, ireadNode, constNode);
  StmtNode *addStmt = mirBuilder.CreateStmtIassign(*mirType, 0, &lockNode, binNode);
  block.AddStatement(addStmt);
  if (kAtomicBinaryOpAdd <= atomicOp && atomicOp <= kAtomicBinaryOpXor) {
    BinaryNode *vbinNode = mirBuilder.CreateExprBinary(opcode, *val1Type, valueNode, constNode);
    addStmt = mirBuilder.CreateStmtDassign(valueVar, 0, vbinNode);
    block.AddStatement(addStmt);
  }
}

void FEIRExprAtomic::ProcessAtomicLoad(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                                       const MIRSymbol &valueVar) const {
  BaseNode *ireadNode = mirBuilder.CreateExprIread(*refType, *mirType, 0, &lockNode);
  StmtNode *loadStmt = mirBuilder.CreateStmtDassign(valueVar, 0, ireadNode);
  block.AddStatement(loadStmt);
}

void FEIRExprAtomic::ProcessAtomicStore(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode) const {
  BaseNode *constNode = valExpr1.get()->GenMIRNode(mirBuilder);
  StmtNode *storeStmt = mirBuilder.CreateStmtIassign(*mirType, 0, &lockNode, constNode);
  block.AddStatement(storeStmt);
}

void FEIRExprAtomic::ProcessAtomicExchange(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                                           const MIRSymbol &valueVar) const {
  BaseNode *ireadNode = mirBuilder.CreateExprIread(*refType, *mirType, 0, &lockNode);
  StmtNode *storeStmt = mirBuilder.CreateStmtDassign(valueVar, 0, ireadNode);
  block.AddStatement(storeStmt);
  BaseNode *constNode = valExpr1.get()->GenMIRNode(mirBuilder);
  StmtNode *setStmt = mirBuilder.CreateStmtIassign(*mirType, 0, &lockNode, constNode);
  block.AddStatement(setStmt);
}

void FEIRExprAtomic::ProcessAtomicCompareExchange(MIRBuilder &mirBuilder, BlockNode &block, BaseNode &lockNode,
                                                  const MIRSymbol *valueVar) const {
  valueVar = mirBuilder.GetOrCreateLocalDecl(FEUtils::GetSequentialName("valueVar").c_str(),
                                             *GlobalTables::GetTypeTable().GetUInt1());
  StmtNode *retStmt = mirBuilder.CreateStmtDassign(*valueVar, 0, mirBuilder.GetConstUInt1(false));
  block.AddStatement(retStmt);
  BaseNode *expectNode = mirBuilder.CreateExprIread(*refType, *mirType, 0, valExpr1.get()->GenMIRNode(mirBuilder));
  BaseNode *desiredNode = valExpr2.get()->GenMIRNode(mirBuilder);
  BaseNode *ireadNode = mirBuilder.CreateExprIread(*refType, *mirType, 0, &lockNode);
  BaseNode *cond = mirBuilder.CreateExprCompare(OP_eq, *(GlobalTables::GetTypeTable().GetUInt1()),
                                                *refType, expectNode, ireadNode);
  IfStmtNode *ifStmt = mirBuilder.CreateStmtIf(cond);
  block.AddStatement(ifStmt);
  StmtNode *setStmt = mirBuilder.CreateStmtIassign(*mirType, 0, &lockNode, desiredNode);
  ifStmt->GetThenPart()->AddStatement(setStmt);
  StmtNode *storeStmt = mirBuilder.CreateStmtDassign(*valueVar, 0, mirBuilder.GetConstUInt1(true));
  ifStmt->GetThenPart()->AddStatement(storeStmt);
}

BaseNode *FEIRExprAtomic::GenMIRNodeImpl(MIRBuilder &mirBuilder) const {
  MIRModule &module = FEManager::GetModule();
  BlockNode *block = module.CurFuncCodeMemPool()->New<BlockNode>();
  BaseNode *objNode = objExpr.get()->GenMIRNode(mirBuilder);
  MIRSymbol *lockVar = lock->GenerateMIRSymbol(mirBuilder);
  MIRSymbol *valueVar = val->GenerateMIRSymbol(mirBuilder);
  BaseNode *lockNode = mirBuilder.CreateExprDread(*lockVar);
  StmtNode *fetchStmt = mirBuilder.CreateStmtIassign(*mirType, 0, lockNode, objNode);
  ASSERT_NOT_NULL(fetchStmt);
  block->AddStatement(fetchStmt);
  NaryStmtNode *syncenter = mirBuilder.CreateStmtNary(OP_syncenter, lockNode);
  block->AddStatement(syncenter);
  switch (atomicOp) {
    case kAtomicBinaryOpAdd:
    case kAtomicBinaryOpSub:
    case kAtomicBinaryOpAnd:
    case kAtomicBinaryOpOr:
    case kAtomicBinaryOpXor: {
      ProcessAtomicBinary(mirBuilder, *block, *lockNode, *valueVar);
      break;
    }
    case kAtomicOpLoad: {
      ProcessAtomicLoad(mirBuilder, *block, *lockNode, *valueVar);
      break;
    }
    case kAtomicOpStore: {
      ProcessAtomicStore(mirBuilder, *block, *lockNode);
      break;
    }
    case kAtomicOpExchange: {
      ProcessAtomicExchange(mirBuilder, *block, *lockNode, *valueVar);
      break;
    }
    case kAtomicOpCompareExchange: {
      ProcessAtomicCompareExchange(mirBuilder, *block, *lockNode, valueVar);
      break;
    }
    default: {
      CHECK_FATAL(false, "atomic opcode not yet supported %u", static_cast<uint32>(atomicOp));
      break;
    }
  }
  NaryStmtNode *syncExit = mirBuilder.CreateStmtNary(OP_syncexit, lockNode);
  block->AddStatement(syncExit);
  return block;
}

// ---------- FEIRStmtPesudoLabel ----------
FEIRStmtPesudoLabel::FEIRStmtPesudoLabel(uint32 argLabelIdx)
    : FEIRStmt(kStmtPesudoLabel),
      labelIdx(argLabelIdx),
      mirLabelIdx(0) {}

std::list<StmtNode*> FEIRStmtPesudoLabel::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  StmtNode *stmtLabel = mirBuilder.CreateStmtLabel(mirLabelIdx);
  ans.push_back(stmtLabel);
  return ans;
}

void FEIRStmtPesudoLabel::GenerateLabelIdx(MIRBuilder &mirBuilder) {
  std::stringstream ss;
  ss << "label" << HIR2MPLEnv::GetInstance().GetGlobalLabelIdx();
  HIR2MPLEnv::GetInstance().IncrGlobalLabelIdx();
  mirLabelIdx = mirBuilder.GetOrCreateMIRLabel(ss.str());
}

std::string FEIRStmtPesudoLabel::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtPesudoLabel2 ----------
LabelIdx FEIRStmtPesudoLabel2::GenMirLabelIdx(MIRBuilder &mirBuilder, uint32 qIdx0, uint32 qIdx1) {
  std::string label = "L" + std::to_string(qIdx0) + "_" + std::to_string(qIdx1);
  return mirBuilder.GetOrCreateMIRLabel(label);
}

std::pair<uint32, uint32> FEIRStmtPesudoLabel2::GetLabelIdx() const {
  return std::make_pair(labelIdxOuter, labelIdxInner);
}

std::list<StmtNode*> FEIRStmtPesudoLabel2::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  StmtNode *stmtLabel = mirBuilder.CreateStmtLabel(GenMirLabelIdx(mirBuilder, labelIdxOuter, labelIdxInner));
  ans.push_back(stmtLabel);
  return ans;
}

// ---------- FEIRStmtPesudoLOC ----------
FEIRStmtPesudoLOC::FEIRStmtPesudoLOC(uint32 argSrcFileIdx, uint32 argLineNumber)
    : FEIRStmt(kStmtPesudoLOC),
      srcFileIdx(argSrcFileIdx),
      lineNumber(argLineNumber) {
  isAuxPre = true;
}

std::list<StmtNode*> FEIRStmtPesudoLOC::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  return std::list<StmtNode*>();
}

std::string FEIRStmtPesudoLOC::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtPesudoJavaTry ----------
FEIRStmtPesudoJavaTry::FEIRStmtPesudoJavaTry()
    : FEIRStmt(kStmtPesudoJavaTry) {}

std::list<StmtNode*> FEIRStmtPesudoJavaTry::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  MapleVector<LabelIdx> vec(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  for (FEIRStmtPesudoLabel *stmtLabel : catchTargets) {
    vec.push_back(stmtLabel->GetMIRLabelIdx());
  }
  StmtNode *stmtTry = mirBuilder.CreateStmtTry(vec);
  ans.push_back(stmtTry);
  return ans;
}

std::string FEIRStmtPesudoJavaTry::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtPesudoJavaTry2 ----------
FEIRStmtPesudoJavaTry2::FEIRStmtPesudoJavaTry2(uint32 outerIdxIn)
    : FEIRStmt(kStmtPesudoJavaTry), outerIdx(outerIdxIn) {}

std::list<StmtNode*> FEIRStmtPesudoJavaTry2::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  MapleVector<LabelIdx> vec(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  for (uint32 target : catchLabelIdxVec) {
    vec.push_back(FEIRStmtPesudoLabel2::GenMirLabelIdx(mirBuilder, outerIdx, target));
  }
  StmtNode *stmtTry = mirBuilder.CreateStmtTry(vec);
  ans.push_back(stmtTry);
  return ans;
}

std::string FEIRStmtPesudoJavaTry2::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtPesudoEndTry ----------
FEIRStmtPesudoEndTry::FEIRStmtPesudoEndTry()
    : FEIRStmt(kStmtPesudoEndTry) {
  isAuxPost = true;
}

std::list<StmtNode*> FEIRStmtPesudoEndTry::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  MemPool *mp = mirBuilder.GetCurrentFuncCodeMp();
  ASSERT(mp != nullptr, "mempool is nullptr");
  StmtNode *stmt = mp->New<StmtNode>(OP_endtry);
  ans.push_back(stmt);
  return ans;
}

std::string FEIRStmtPesudoEndTry::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtPesudoCatch ----------
FEIRStmtPesudoCatch::FEIRStmtPesudoCatch(uint32 argLabelIdx)
    : FEIRStmtPesudoLabel(argLabelIdx) {}

std::list<StmtNode*> FEIRStmtPesudoCatch::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  StmtNode *stmtLabel = mirBuilder.CreateStmtLabel(mirLabelIdx);
  ans.push_back(stmtLabel);
  MapleVector<TyIdx> vec(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  for (const UniqueFEIRType &type : catchTypes) {
    MIRType *mirType = type->GenerateMIRType(kSrcLangJava, true);
    vec.push_back(mirType->GetTypeIndex());
  }
  StmtNode *stmtCatch = mirBuilder.CreateStmtCatch(vec);
  ans.push_back(stmtCatch);
  return ans;
}

void FEIRStmtPesudoCatch::AddCatchTypeNameIdx(GStrIdx typeNameIdx) {
  UniqueFEIRType type = std::make_unique<FEIRTypeDefault>(PTY_ref, typeNameIdx);
  catchTypes.push_back(std::move(type));
}

std::string FEIRStmtPesudoCatch::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtPesudoCatch2 ----------
FEIRStmtPesudoCatch2::FEIRStmtPesudoCatch2(uint32 qIdx0, uint32 qIdx1)
    : FEIRStmtPesudoLabel2(qIdx0, qIdx1) {}

std::list<StmtNode*> FEIRStmtPesudoCatch2::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  StmtNode *stmtLabel = mirBuilder.CreateStmtLabel(
      FEIRStmtPesudoLabel2::GenMirLabelIdx(mirBuilder, GetLabelIdx().first, GetLabelIdx().second));
  ans.push_back(stmtLabel);
  MapleVector<TyIdx> vec(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
  for (const UniqueFEIRType &type : catchTypes) {
    MIRType *mirType = type->GenerateMIRType(kSrcLangJava, true);
    vec.push_back(mirType->GetTypeIndex());
  }
  StmtNode *stmtCatch = mirBuilder.CreateStmtCatch(vec);
  ans.push_back(stmtCatch);
  return ans;
}

void FEIRStmtPesudoCatch2::AddCatchTypeNameIdx(GStrIdx typeNameIdx) {
  UniqueFEIRType type;
  if (typeNameIdx == FEUtils::GetVoidIdx()) {
    type = std::make_unique<FEIRTypeDefault>(PTY_ref, bc::BCUtil::GetJavaThrowableNameMplIdx());
  } else {
    type = std::make_unique<FEIRTypeDefault>(PTY_ref, typeNameIdx);
  }
  catchTypes.push_back(std::move(type));
}

std::string FEIRStmtPesudoCatch2::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

FEIRStmtPesudoSafe::FEIRStmtPesudoSafe(bool isEnd)
    : FEIRStmt(kStmtPesudoSafe), end(isEnd) {}

std::list<StmtNode*> FEIRStmtPesudoSafe::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  MemPool *mp = mirBuilder.GetCurrentFuncCodeMp();
  ASSERT(mp != nullptr, "mempool is nullptr");
  Opcode op = end ? OP_endsafe : OP_safe;
  StmtNode *stmt = mp->New<StmtNode>(op);
  ans.push_back(stmt);
  FEFunction &curFEFunc = FEManager::GetCurrentFEFunction();
  if (end) {
    if (curFEFunc.GetSafeRegionFlag().empty() || !curFEFunc.GetSafeRegionFlag().top()) {
      CHECK_FATAL(false, "pop safe region error");
    }
    (void)curFEFunc.GetSafeRegionFlag().pop();
  } else {
    curFEFunc.GetSafeRegionFlag().push(true);
  }
  return ans;
}

std::string FEIRStmtPesudoSafe::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

FEIRStmtPesudoUnsafe::FEIRStmtPesudoUnsafe(bool isEnd)
    : FEIRStmt(kStmtPesudoUnsafe), end(isEnd) {}

std::list<StmtNode*> FEIRStmtPesudoUnsafe::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  MemPool *mp = mirBuilder.GetCurrentFuncCodeMp();
  ASSERT(mp != nullptr, "mempool is nullptr");
  Opcode op = end ? OP_endunsafe : OP_unsafe;
  StmtNode *stmt = mp->New<StmtNode>(op);
  ans.push_back(stmt);
  FEFunction &curFEFunc = FEManager::GetCurrentFEFunction();
  if (end) {
    if (curFEFunc.GetSafeRegionFlag().empty() || curFEFunc.GetSafeRegionFlag().top()) {
      CHECK_FATAL(false, "pop unsafe region error");
    }
    (void)curFEFunc.GetSafeRegionFlag().pop();
  } else {
    curFEFunc.GetSafeRegionFlag().push(false);
  }
  return ans;
}

std::string FEIRStmtPesudoUnsafe::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtPesudoComment ----------
FEIRStmtPesudoComment::FEIRStmtPesudoComment(FEIRNodeKind argKind)
    : FEIRStmt(argKind) {
  isAuxPre = true;
}

FEIRStmtPesudoComment::FEIRStmtPesudoComment(const std::string &argContent)
    : FEIRStmt(kStmtPesudoComment),
      content(argContent) {
  isAuxPre = true;
}

std::list<StmtNode*> FEIRStmtPesudoComment::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  StmtNode *stmt = mirBuilder.CreateStmtComment(content);
  ans.push_back(stmt);
  return ans;
}

std::string FEIRStmtPesudoComment::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtPesudoCommentForInst ----------
FEIRStmtPesudoCommentForInst::FEIRStmtPesudoCommentForInst()
    : FEIRStmtPesudoComment(kStmtPesudoCommentForInst) {
  isAuxPre = true;
}

std::list<StmtNode*> FEIRStmtPesudoCommentForInst::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  return ans;
}

std::string FEIRStmtPesudoCommentForInst::DumpDotStringImpl() const {
  std::stringstream ss;
  ss << "<stmt" << id << "> " << id << ": " << GetFEIRNodeKindDescription(kind);
  return ss.str();
}

// ---------- FEIRStmtIAssign ----------
std::list<StmtNode*> FEIRStmtIAssign::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> ans;
  MIRType *mirType = addrType->GenerateMIRTypeAuto();
  CHECK_FATAL(mirType->IsMIRPtrType(), "Must be ptr type");
  BaseNode *addrNode = addrExpr->GenMIRNode(mirBuilder);
  BaseNode *baseNode = baseExpr->GenMIRNode(mirBuilder);
  if (fieldID != 0) {
    MIRType * baseType = static_cast<MIRPtrType*>(mirType)->GetPointedType();
    CHECK_FATAL((baseType->GetKind() == MIRTypeKind::kTypeStruct || baseType->GetKind() == MIRTypeKind::kTypeUnion),
                "If fieldID is not 0, then the computed address must correspond to a structure");
    InsertNonnullChecking(mirBuilder, *baseType, ans);
    CheckNonnullArgsAndRetForFuncPtr(*baseType);
    CheckBoundaryArgsAndRetForFuncPtr(*baseType);
  }
  IassignNode *iAssignNode = mirBuilder.CreateStmtIassign(*mirType, fieldID, addrNode, baseNode);
  ans.emplace_back(iAssignNode);
  AssignBoundaryVarAndChecking(mirBuilder, ans);
  return ans;
}

void FEIRStmtIAssign::InsertNonnullChecking(MIRBuilder &mirBuilder, MIRType &baseType,
                                            std::list<StmtNode*> &ans) const {
  if (!FEOptions::GetInstance().IsNpeCheckDynamic()) {
    return;
  }
  FieldID tmpID = fieldID;
  FieldPair fieldPair = static_cast<MIRStructType&>(baseType).TraverseToFieldRef(tmpID);
  if (fieldPair.second.second.GetAttr(FLDATTR_nonnull)) {
    if (ENCChecker::HasNullExpr(baseExpr)) {
      FE_ERR(kLncErr, "%s:%d error: null assignment of nonnull pointer",
             FEManager::GetModule().GetFileNameFromFileNum(srcFileIndex).c_str(), srcFileLineNum);
      return;
    }
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtUseOnly>(OP_assignassertnonnull, baseExpr->Clone());
    std::list<StmtNode*> stmts = stmt->GenMIRStmts(mirBuilder);
    ans.splice(ans.end(), stmts);
  }
}

// ---------- FEIRStmtDoWhile ----------
std::list<StmtNode*> FEIRStmtDoWhile::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> stmts;
  auto *whileStmtNode = mirBuilder.GetCurrentFuncCodeMp()->New<WhileStmtNode>(opcode);
  BaseNode *mirCond = condExpr->GenMIRNode(mirBuilder);
  whileStmtNode->SetOpnd(mirCond, 0);
  auto *bodyBlock = mirBuilder.GetCurrentFuncCodeMp()->New<BlockNode>();
  for (auto &stmt : bodyStmts) {
    for (auto mirStmt : stmt->GenMIRStmts(mirBuilder)) {
      bodyBlock->AddStatement(mirStmt);
    }
  }
  whileStmtNode->SetBody(bodyBlock);
  stmts.emplace_back(whileStmtNode);
  return stmts;
}

std::list<StmtNode*> FEIRStmtBreak::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> stmts;
  CHECK_FATAL(!breakLabelName.empty(), "labelName is null!");
  LabelIdx labelIdx = mirBuilder.GetOrCreateMIRLabel(breakLabelName);
  GotoNode *gotoNode = mirBuilder.CreateStmtGoto(OP_goto, labelIdx);
  stmts.emplace_back(gotoNode);
  return stmts;
}

std::list<StmtNode*> FEIRStmtContinue::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> stmts;
  CHECK_FATAL(!labelName.empty(), "labelName is null!");
  LabelIdx labelIdx = mirBuilder.GetOrCreateMIRLabel(labelName);
  GotoNode *gotoNode = mirBuilder.CreateStmtGoto(OP_goto, labelIdx);
  stmts.emplace_back(gotoNode);
  return stmts;
}

std::list<StmtNode*> FEIRStmtLabel::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  return std::list<StmtNode*>{mirBuilder.CreateStmtLabel(mirBuilder.GetOrCreateMIRLabel(labelName))};
}

FEIRStmtAtomic::FEIRStmtAtomic(UniqueFEIRExpr expr)
    : FEIRStmt(FEIRNodeKind::kStmtAtomic),
      atomicExpr(std::move(expr)) {}

std::list<StmtNode*> FEIRStmtAtomic::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> stmts;
  FEIRExprAtomic *atom = static_cast<FEIRExprAtomic*>(atomicExpr.get());
  auto block = static_cast<BlockNode*>(atom->GenMIRNode(mirBuilder));
  for (auto &it : block->GetStmtNodes()) {
    stmts.emplace_back(&it);
  }
  return stmts;
}

bool FEIRStmtGCCAsm::HandleConstraintPlusQm(MIRBuilder &mirBuilder, AsmNode *asmNode, uint32 index) const {
  if (std::get<1>(outputs[index]) != "+Q" && std::get<1>(outputs[index]) != "+m") {
    return false;
  }
  FieldID fieldID = outputsExprs[index]->GetFieldID();
  MIRSymbol *sym = outputsExprs[index]->GetVarUses().front()->GenerateMIRSymbol(mirBuilder);

  CallReturnPair retPair(sym->GetStIdx(), RegFieldPair(fieldID, 0));
  asmNode->asmOutputs.emplace_back(retPair);
  UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(std::get<1>(outputs[index]));
  asmNode->outputConstraints.emplace_back(strIdx);

  BaseNode *node;
  if (outputsExprs[index]->GetKind() == kExprDRead) {
    node = static_cast<BaseNode*>(mirBuilder.CreateExprAddrof(fieldID, *sym));
  } else if (outputsExprs[index]->GetKind() == kExprIRead) {
    FEIRExprIRead *iread = static_cast<FEIRExprIRead*>(outputsExprs[index].get());
    if (iread->GetFieldID() == 0) {
      node = iread->GetClonedOpnd()->GenMIRNode(mirBuilder);
    } else {
      auto addrOfExpr = std::make_unique<FEIRExprIAddrof>(iread->GetClonedPtrType(), iread->GetFieldID(),
                                                          iread->GetClonedOpnd());
      node = addrOfExpr->GenMIRNode(mirBuilder);
    }
  } else {
    CHECK_FATAL(false, "FEIRStmtGCCAsm NYI.");
  }
  asmNode->PushOpnd(node);
  asmNode->inputConstraints.emplace_back(strIdx);
  return true;
}

std::list<StmtNode*> FEIRStmtGCCAsm::GenMIRStmtsImpl(MIRBuilder &mirBuilder) const {
  std::list<StmtNode*> stmts;
  std::list<StmtNode*> initStmts;
  MemPool *mp = mirBuilder.GetCurrentFuncCodeMp();
  AsmNode *asmNode = mp->New<AsmNode>(mirBuilder.GetCurrentFuncCodeMpAllocator());
  asmNode->asmString = asmStr;
  for (uint32 i = 0; i < inputs.size(); ++i) {
    BaseNode *node = inputsExprs[i]->GenMIRNode(mirBuilder);
    asmNode->PushOpnd(node);
    UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(inputs[i].second);
    asmNode->inputConstraints.emplace_back(strIdx);
  }
  for (uint32 i = 0; i < outputs.size(); ++i) {
    if (HandleConstraintPlusQm(mirBuilder, asmNode, i)) {
      continue;
    }
    FieldID fieldID = 0;
    MIRSymbol *sym = nullptr;
    UniqueFEIRVar asmOut;
    if (outputsExprs[i]->GetKind() == kExprDRead) {
      FEIRExprDRead *dread = static_cast<FEIRExprDRead*>(outputsExprs[i].get());
      fieldID = dread->GetFieldID();
      sym = dread->GetVarUses().front()->GenerateMIRSymbol(mirBuilder);
      asmOut = dread->GetVarUses().front()->Clone();
    } else if (outputsExprs[i]->GetKind() == kExprIRead) {
      FEIRExprIRead *iread = static_cast<FEIRExprIRead*>(outputsExprs[i].get());
      fieldID = iread->GetFieldID();
      asmOut = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("asm_out_"), iread->GetClonedRetType());
      sym = asmOut->GenerateLocalMIRSymbol(mirBuilder);
      UniqueFEIRExpr srcExpr = FEIRBuilder::CreateExprDRead(asmOut->Clone());
      auto stmt = FEIRBuilder::CreateStmtIAssign(iread->GetClonedPtrType(), iread->GetClonedOpnd(),
                                                 std::move(srcExpr), fieldID);
      std::list<StmtNode*> node = stmt->GenMIRStmts(mirBuilder);
      stmts.splice(stmts.end(), node);

      // The field ID is set to zero when a temporary variable is created for iread and sym is not a struct or union.
      if (!sym->GetType()->IsStructType()) {
        fieldID = 0;
      }
    } else {
      CHECK_FATAL(false, "FEIRStmtGCCAsm NYI.");
    }

    CallReturnPair retPair(sym->GetStIdx(), RegFieldPair(fieldID, 0));
    asmNode->asmOutputs.emplace_back(retPair);
    UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(std::get<1>(outputs[i]));
    asmNode->outputConstraints.emplace_back(strIdx);

    // If this is a read/write, copy the initial value into the temp before and added to the input list
    if (std::get<2>(outputs[i])) {
      auto stmt = FEIRBuilder::CreateStmtDAssign(asmOut->Clone(), outputsExprs[i]->Clone());
      std::list<StmtNode*> node = stmt->GenMIRStmts(mirBuilder);
      initStmts.splice(initStmts.end(), node);

      AddrofNode *rNode = mirBuilder.CreateExprDread(*sym);
      asmNode->PushOpnd(static_cast<BaseNode*>(rNode));
      asmNode->inputConstraints.emplace_back(strIdx);
    }
  }
  for (uint32 i = 0; i < clobbers.size(); ++i) {
    UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(clobbers[i]);
    asmNode->clobberList.emplace_back(strIdx);
  }
  for (uint32 i = 0; i < labels.size(); ++i) {
    LabelIdx label = mirBuilder.GetOrCreateMIRLabel(labels[i]);
    asmNode->gotoLabels.emplace_back(label);
  }
  if (isVolatile) {
    asmNode->SetQualifier(kASMvolatile);
  }
  if (isGoto) {
    asmNode->SetQualifier(kASMgoto);
  }
  stmts.emplace_front(asmNode);
  if (!initStmts.empty()) {
    stmts.splice(stmts.begin(), initStmts);
  }
  return stmts;
}
}  // namespace maple
