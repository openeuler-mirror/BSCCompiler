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
#include "cg_phasemanager.h"
#include <vector>
#include <string>
#include "cg_option.h"
#include "args.h"
#include "label_creation.h"
#include "driver_options.h"
#include "isel.h"
#include "offset_adjust.h"
#include "alignment.h"
#include "yieldpoint.h"
#include "emit.h"
#include "reg_alloc.h"
#include "target_info.h"
#include "standardize.h"
#include "cg_callgraph_reorder.h"
#if defined(TARGAARCH64) && TARGAARCH64
#include "aarch64_emitter.h"
#include "aarch64_cg.h"
#elif defined(TARGRISCV64) && TARGRISCV64
#include "riscv64_emitter.h"
#elif defined(TARGX86_64) && TARGX86_64
#include "x64_cg.h"
#include "x64_emitter.h"
#include "string_utils.h"
#endif

namespace maplebe {
#define JAVALANG (module.IsJavaModule())
#define CLANG (module.GetSrcLang() == kSrcLangC)

#define RELEASE(pointer)      \
  do {                        \
    if ((pointer) != nullptr) { \
      delete (pointer);         \
      (pointer) = nullptr;      \
    }                         \
  } while (0)

namespace {

void DumpMIRFunc(MIRFunction &func, const char *msg, bool printAlways = false, const char* extraMsg = nullptr) {
  bool dumpAll = (CGOptions::GetDumpPhases().find("*") != CGOptions::GetDumpPhases().end());
  bool dumpFunc = CGOptions::FuncFilter(func.GetName());
  if (printAlways || (dumpAll && dumpFunc)) {
    LogInfo::MapleLogger() << msg << '\n';
    func.Dump();
    if (extraMsg) {
      LogInfo::MapleLogger() << extraMsg << '\n';
    }
  }
}

} /* anonymous namespace */

// a tricky implementation of accessing TLS symbol
// call to __tls_get_addr has been arranged at init_array of specific so/exec
// the addres of tls sections (.tbss & .tdata) will be recorded in global accessable symbols (anchors)
// Here we store the anchor in the .data section of the object file, which is only copy in the vitual space.
// Therefor this scheme is only feasible in one thread programe.
void PrepareForWarmupDynamicTlsSingleThread(MIRModule &m) {
  if (m.GetTdataVarOffset().empty() && m.GetTbssVarOffset().empty()) {
    return;
  }

  auto *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_ptr));
  auto *anchorMirConstVar = m.GetMemPool()->New<MIRIntConst>(0, *ptrType);

  ArgVector formalsWarmup(m.GetMPAllocator().Adapter());
  MIRType *voidTy = GlobalTables::GetTypeTable().GetVoid();
  auto *tlsWarmup = m.GetMIRBuilder()->CreateFunction("__tls_address_warmup_" + m.GetTlsAnchorHashString(),
      *voidTy, formalsWarmup);
  tlsWarmup->SetWithSrc(false);
  auto *warmupFuncBody = tlsWarmup->GetCodeMempool()->New<BlockNode>();
  MIRSymbol *tempAnchorSym = nullptr;
  AddrofNode *tlsAddrNode = nullptr;
  DassignNode *dassignNode = nullptr;

  if (!m.GetTdataVarOffset().empty()) {
    auto *tdataAnchorSym = m.GetMIRBuilder()->GetOrCreateGlobalDecl("tdata_addr_" + m.GetTlsAnchorHashString(),
        *ptrType);
    tdataAnchorSym->SetKonst(anchorMirConstVar);
    tempAnchorSym = m.GetMIRBuilder()->GetOrCreateGlobalDecl(".tdata_start_" + m.GetTlsAnchorHashString(), *ptrType);
    tempAnchorSym->SetIsDeleted();
    tlsAddrNode = tlsWarmup->GetCodeMempool()->New<AddrofNode>(OP_addrof, PTY_ptr, tempAnchorSym->GetStIdx(), 0);
    dassignNode = tlsWarmup->GetCodeMempool()->New<DassignNode>();
    dassignNode->SetStIdx(tdataAnchorSym->GetStIdx());
    dassignNode->SetFieldID(0);
    dassignNode->SetOpnd(tlsAddrNode, 0);
    warmupFuncBody->AddStatement(dassignNode);
  }

  if (!m.GetTbssVarOffset().empty()) {
    auto *tbssAnchorSym = m.GetMIRBuilder()->GetOrCreateGlobalDecl("tbss_addr_" + m.GetTlsAnchorHashString(), *ptrType);
    tbssAnchorSym->SetKonst(anchorMirConstVar);
    tempAnchorSym = m.GetMIRBuilder()->GetOrCreateGlobalDecl(".tbss_start_" + m.GetTlsAnchorHashString(), *ptrType);
    tempAnchorSym->SetIsDeleted();
    tlsAddrNode = tlsWarmup->GetCodeMempool()->New<AddrofNode>(OP_addrof, PTY_ptr, tempAnchorSym->GetStIdx(), 0);
    dassignNode = tlsWarmup->GetCodeMempool()->New<DassignNode>();
    dassignNode->SetStIdx(tbssAnchorSym->GetStIdx());
    dassignNode->SetFieldID(0);
    dassignNode->SetOpnd(tlsAddrNode, 0);
    warmupFuncBody->AddStatement(dassignNode);
  }

  if (opts::aggressiveTlsWarmupFunction.IsEnabledByUser()) {
    tlsWarmup->SetBody(warmupFuncBody);
    m.AddFunction(tlsWarmup);
    auto *tarFunc = m.GetMIRBuilder()->GetFunctionFromName(m.GetTlsWarmupFunction());
    CHECK_FATAL(tarFunc != nullptr, "Tls Warmup Function does not exist in the module!");
    MapleVector<BaseNode*> arg(m.GetMIRBuilder()->GetCurrentFuncCodeMpAllocator()->Adapter());
    CallNode *callWarmup = m.GetMIRBuilder()->CreateStmtCall("__tls_address_warmup_" + m.GetTlsAnchorHashString(), arg);
    tarFunc->GetBody()->InsertFirst(callWarmup);
  } else {
    // put warmup func into .init_array for common testcase
    tlsWarmup->SetBody(warmupFuncBody);
    tlsWarmup->SetAttr(FUNCATTR_section);
    tlsWarmup->GetFuncAttrs().SetPrefixSectionName(".init_array");
    m.AddFunction(tlsWarmup);
  }
}

// Another tricky implementation of accessing TLS symbol
// This scheme is only feasible for lto module.
// There is only one anchor for each thread, which is store in a "thread pointer related" position.
// The actual position depends on the underlying implemention of system tls lib.
// Now We choose to put the anchor in a reserved space of TBC struct, for elibc.

// offsetTbcReservedForX86 is the offset from X86 dtv (reserved space in aarch64) to aarch64 dtv in elibc
// in elibc, the position of X86 dtv can be used to store TLS anchor for each thread.
// The TBC struct is defined in cbf/task/task_base/include/task_base_pub.h, the layout as below
// TBC_AARCH64
// {self_ptr X86_dtv(null for aarch64) prev_dtv .............................................. aarch64_dtv}
//                   |                                                                              |
//                   |                                                                              |
//                   v-------------------------------224B-------------------------------------------v

// TBC_RESERVED_FOR_X86 dtv is not safe for it could be used by other hack.
// If We could put a tls symbol in the first(?) module of system,
// We could also get a safe anchor which could produce simple tls access pattern.
// offsetManualAnchorSymbol is the offset from dtv[0] to the artifical anchor.
// Now this implementation is not accurate yet, so here is just a preliminary work.
// Const defined in cg_option.h

void PrepareForWarmupDynamicTlsMutiThread(MIRModule &m) {
  if (m.GetTlsVarOffset().empty()) {
    return;
  }
  auto *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_ptr));
  auto *anchorMirConst = m.GetMemPool()->New<MIRIntConst>(0, *ptrType);

  ArgVector formals(m.GetMPAllocator().Adapter());
  MIRType *voidTy = GlobalTables::GetTypeTable().GetVoid();
  auto *tlsWarmup = m.GetMIRBuilder()->CreateFunction("__tls_address_warmup_" + m.GetTlsAnchorHashString(),
      *voidTy, formals);
  tlsWarmup->SetWithSrc(false);
  auto *warmupBody = tlsWarmup->GetCodeMempool()->New<BlockNode>();
  MIRSymbol *tempWarmupSym = nullptr;
  AddrofNode *tlsAddrNode = nullptr;
  DassignNode *dassignNode = nullptr;

  MIRBuilder *mirBuilder = m.GetMIRBuilder();
  MemPool *memPool = tlsWarmup->GetCodeMempool();
  TypeTable& tab = GlobalTables::GetTypeTable();
  uint32 oldTypeTableSize = tab.GetTypeTableSize();
  MIRType *u64Type = tab.GetPrimType(PTY_u64);
  MIRType *singlePtrType = tab.GetOrCreatePointerType(*u64Type);
  MIRType *doublePtrType = tab.GetOrCreatePointerType(*singlePtrType);

  MIRSymbol *tempAnchorSym = nullptr;
  ConstvalNode *gotOffset = nullptr;
  ConstvalNode *dtvPtrSize = nullptr;
  ConstvalNode *anchorOffset = nullptr;
  MIRSymbol *idx = nullptr;
  MIRSymbol *base = nullptr;

  MIRSymbol *tempVal = m.GetMIRBuilder()->GetOrCreateGlobalDecl("tls_addr_" + m.GetTlsAnchorHashString(), *ptrType);
  tempVal->SetKonst(anchorMirConst);
  tempWarmupSym = m.GetMIRBuilder()->GetOrCreateGlobalDecl(".tls_start_" + m.GetTlsAnchorHashString(), *ptrType);
  tempWarmupSym->SetIsDeleted();
  tlsAddrNode = tlsWarmup->GetCodeMempool()->New<AddrofNode>(OP_addrof, PTY_ptr, tempWarmupSym->GetStIdx(), 0);
  dassignNode = tlsWarmup->GetCodeMempool()->New<DassignNode>();
  dassignNode->SetStIdx(tempVal->GetStIdx());
  dassignNode->SetFieldID(0);
  dassignNode->SetOpnd(tlsAddrNode, 0);
  warmupBody->AddStatement(dassignNode);

  tempAnchorSym = mirBuilder->GetOrCreateGlobalDecl(".tls_start_" + m.GetTlsAnchorHashString(), *ptrType);
  tempAnchorSym->SetIsDeleted();
  DreadNode *warmupNode = memPool->New<DreadNode>(OP_dread, PTY_u64, tempAnchorSym->GetStIdx(), 0);
  gotOffset = mirBuilder->CreateIntConst(offsetTlsParamEntry, PTY_u64);
  BinaryNode *addNode = memPool->New<BinaryNode>(OP_add, PTY_u64, warmupNode, gotOffset);
  TypeCvtNode *cvtNode = memPool->New<TypeCvtNode>(OP_cvt, PTY_ptr, PTY_u64, addNode);
  IreadNode *ireadNode = memPool->New<IreadNode>(OP_iread, PTY_ptr, doublePtrType->GetTypeIndex(), 0, cvtNode);
  IreadNode *ireadNode1 = memPool->New<IreadNode>(OP_iread, PTY_u64, singlePtrType->GetTypeIndex(), 0, ireadNode);
  idx = mirBuilder->GetOrCreateGlobalDecl(".tls_idx_" + m.GetTlsAnchorHashString(), *u64Type);

  DassignNode *dassignNode1 = memPool->New<DassignNode>(ireadNode1, idx->GetStIdx(), 0);
  warmupBody->AddStatement(dassignNode1);

  BinaryNode *addNode3 = memPool->New<BinaryNode>(OP_add, PTY_u64, ireadNode, gotOffset);
  TypeCvtNode *cvtNode4 = memPool->New<TypeCvtNode>(OP_cvt, PTY_ptr, PTY_u64, addNode3);
  IreadNode *ireadNode5 = memPool->New<IreadNode>(OP_iread, PTY_u64, singlePtrType->GetTypeIndex(), 0, cvtNode4);

  DreadNode *dreadNode1 = memPool->New<DreadNode>(OP_dread, PTY_u64, idx->GetStIdx(), 0);
  dtvPtrSize = mirBuilder->CreateIntConst(lslDtvEntrySize, PTY_u64);
  BinaryNode *shlNode = memPool->New<BinaryNode>(OP_shl, PTY_u64, dreadNode1, dtvPtrSize);
  MapleVector<BaseNode*> args0(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
  IntrinsicopNode *threadPtr =
      mirBuilder->CreateExprIntrinsicop(maple::INTRN_C___tls_get_thread_pointer, OP_intrinsicop, *u64Type, args0);
  TypeCvtNode *cvtNode1 = memPool->New<TypeCvtNode>(OP_cvt, PTY_ptr, PTY_u64, threadPtr);
  IreadNode *ireadNode2 = memPool->New<IreadNode>(OP_iread, PTY_u64, singlePtrType->GetTypeIndex(), 0, cvtNode1);
  BinaryNode *addNode2 = memPool->New<BinaryNode>(OP_add, PTY_u64, ireadNode2, shlNode);
  TypeCvtNode *cvtNode2 = memPool->New<TypeCvtNode>(OP_cvt, PTY_ptr, PTY_u64, addNode2);
  IreadNode *ireadNode3 = memPool->New<IreadNode>(OP_iread, PTY_u64, singlePtrType->GetTypeIndex(), 0, cvtNode2);
  BinaryNode *addNode4 = memPool->New<BinaryNode>(OP_add, PTY_u64, ireadNode3, ireadNode5);
  base = mirBuilder->GetOrCreateGlobalDecl(".tls_base_" + m.GetTlsAnchorHashString(), *u64Type);
  DassignNode *dassignNode2 = memPool->New<DassignNode>(addNode4, base->GetStIdx(), 0);
  warmupBody->AddStatement(dassignNode2);

  if (opts::aggressiveTlsSafeAnchor) {
    TypeCvtNode *cvtNode3 = memPool->New<TypeCvtNode>(OP_cvt, PTY_ptr, PTY_u64, threadPtr);
    IreadNode *ireadNode4 = memPool->New<IreadNode>(OP_iread, PTY_u64, singlePtrType->GetTypeIndex(), 0, cvtNode3);
    anchorOffset = mirBuilder->CreateIntConst(offsetManualAnchorSymbol, PTY_u64);
    BinaryNode *addNode1 = memPool->New<BinaryNode>(OP_add, PTY_u64, ireadNode4, anchorOffset);
    TypeCvtNode *cvtNode5 = memPool->New<TypeCvtNode>(OP_cvt, PTY_ptr, PTY_u64, addNode1);
    DreadNode *dreadNode2 = memPool->New<DreadNode>(OP_dread, PTY_u64, base->GetStIdx(), 0);
    IassignNode *iassignNode = memPool->New<IassignNode>(singlePtrType->GetTypeIndex(), 0, cvtNode5, dreadNode2);
    warmupBody->AddStatement(iassignNode);
  } else {
    anchorOffset = mirBuilder->CreateIntConst(offsetTbcReservedForX86, PTY_u64);
    BinaryNode *subNode = memPool->New<BinaryNode>(OP_sub, PTY_u64, threadPtr, anchorOffset);
    TypeCvtNode *cvtNode3 = memPool->New<TypeCvtNode>(OP_cvt, PTY_ptr, PTY_u64, subNode);
    DreadNode *dreadNode2 = memPool->New<DreadNode>(OP_dread, PTY_u64, base->GetStIdx(), 0);
    IassignNode *iassignNode = memPool->New<IassignNode>(singlePtrType->GetTypeIndex(), 0, cvtNode3, dreadNode2);
    warmupBody->AddStatement(iassignNode);
  }

if (opts::aggressiveTlsWarmupFunction.IsEnabledByUser()) {
    tlsWarmup->SetBody(warmupBody);
    m.AddFunction(tlsWarmup);
    auto *tarFunc = m.GetMIRBuilder()->GetFunctionFromName(m.GetTlsWarmupFunction());
    CHECK_FATAL(tarFunc != nullptr, "Tls Warmup Function does not exist in the module!");
    MapleVector<BaseNode*> arg(m.GetMIRBuilder()->GetCurrentFuncCodeMpAllocator()->Adapter());
    CallNode *callWarmup = m.GetMIRBuilder()->CreateStmtCall("__tls_address_warmup_" + m.GetTlsAnchorHashString(), arg);
    tarFunc->GetBody()->InsertFirst(callWarmup);
  } else {
    // put warmup func into .init_array for common testcase
    tlsWarmup->SetBody(warmupBody);
    tlsWarmup->SetAttr(FUNCATTR_section);
    tlsWarmup->GetFuncAttrs().SetPrefixSectionName(".init_array");
    m.AddFunction(tlsWarmup);
  }

  uint32 newTypeTableSize = tab.GetTypeTableSize();
  if (newTypeTableSize != oldTypeTableSize) {
    if (Globals::GetInstance() && Globals::GetInstance()->GetBECommon()) {
      Globals::GetInstance()->GetBECommon()->AddNewTypeAfterBecommon(oldTypeTableSize, newTypeTableSize);
    }
  }
}


// calculate all local dynamic TLS offset from the anchor
void CalculateWarmupDynamicTLS(MIRModule &m) {
  size_t size = GlobalTables::GetGsymTable().GetSymbolTableSize();
  MIRType *mirType = nullptr;
  uint64 tdataOffset = 0;
  uint64 tbssOffset = 0;
  MapleMap<const MIRSymbol*, uint64> &tdataVarOffset = m.GetTdataVarOffset();
  MapleMap<const MIRSymbol*, uint64> &tbssVarOffset = m.GetTbssVarOffset();

  for (auto it = m.GetFunctionList().begin(); it != m.GetFunctionList().end(); ++it) {
    MIRFunction *mirFunc = *it;
    if (mirFunc->GetBody() == nullptr || mirFunc->GetSymTab() == nullptr) {
      continue;
    }
    MIRSymbolTable *lSymTab = mirFunc->GetSymTab();
    size_t lsize = lSymTab->GetSymbolTableSize();
    for (size_t i = 0; i < lsize; i++) {
      const MIRSymbol *mirSymbol = lSymTab->GetSymbolFromStIdx(static_cast<uint32>(i));
      mirType = mirSymbol->GetType();
      uint64 align = 0;
      if (mirType->GetKind() == kTypeStruct || mirType->GetKind() == kTypeClass ||
          mirType->GetKind() == kTypeArray || mirType->GetKind() == kTypeUnion) {
        align = k8ByteSize;
      } else {
        align = mirType->GetAlign();
      }
      if (mirSymbol == nullptr || mirSymbol->GetStorageClass() != kScPstatic) {
        continue;
      }
      if (mirSymbol->IsThreadLocal()) {
        if (!mirSymbol->IsConst()) {
          tbssOffset = RoundUp(tbssOffset, align);
          tbssVarOffset[mirSymbol] = tbssOffset;
          tbssOffset += mirType->GetSize();
        } else {
          tdataOffset = RoundUp(tdataOffset, align);
          tdataVarOffset[mirSymbol] = tdataOffset;
          tdataOffset += mirType->GetSize();
        }
      }
    }
  }

  for (size_t i = 0; i < size; ++i) {
    const MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(static_cast<uint32>(i));
    if (mirSymbol == nullptr || mirSymbol->GetStorageClass() == kScExtern) {
      continue;
    }
    mirType = mirSymbol->GetType();
    uint64 align = 0;
    if (mirType->GetKind() == kTypeStruct || mirType->GetKind() == kTypeClass ||
        mirType->GetKind() == kTypeArray || mirType->GetKind() == kTypeUnion) {
      align = k8ByteSize;
    } else {
      align = mirType->GetAlign();
    }
    if (mirSymbol->IsThreadLocal()) {
      if (!mirSymbol->IsConst()) {
        tbssOffset = RoundUp(tbssOffset, align);
        tbssVarOffset[mirSymbol] = tbssOffset;
        tbssOffset += mirType->GetSize();
      } else {
        tdataOffset = RoundUp(tdataOffset, align);
        tdataVarOffset[mirSymbol] = tdataOffset;
        tdataOffset += mirType->GetSize();
      }
    }
  }
}

void CgFuncPM::GenerateOutPutFile(MIRModule &m) const {
  CHECK_FATAL(cg != nullptr, "cg is null");
  CHECK_FATAL(cg->GetEmitter(), "emitter is null");
  if (!cgOptions->SuppressFileInfo()) {
    cg->GetEmitter()->EmitFileInfo(m.GetInputFileName());
  }
  if (cgOptions->WithDwarf()) {
    cg->GetEmitter()->EmitDIHeader();
  }
  InitProfile(m);
}

bool CgFuncPM::FuncLevelRun(CGFunc &cgFunc, AnalysisDataManager &serialADM) {
  bool changed = false;
  for (size_t i = 0; i < phasesSequence.size(); ++i) {
    SolveSkipFrom(CGOptions::GetSkipFromPhase(), i);
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << "---Run MplCG " << (curPhase->IsAnalysis() ? "analysis" : "transform")
                             << " Phase [ " << curPhase->PhaseName() << " ]---\n";
    }
    if (curPhase->IsAnalysis()) {
      changed = RunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(*curPhase, serialADM, cgFunc) || changed;
    } else  {
      changed = RunTransformPhase<MapleFunctionPhase<CGFunc>, CGFunc>(*curPhase, serialADM, cgFunc) || changed;
      DumpFuncCGIR(cgFunc, curPhase->PhaseName());
    }
    SolveSkipAfter(CGOptions::GetSkipAfterPhase(), i);
  }
  return changed;
}

void CgFuncPM::PostOutPut(MIRModule &m) const {
  cg->GetEmitter()->EmitHugeSoRoutines(true);
  if (cgOptions->WithDwarf()) {
    cg->GetEmitter()->EmitDIFooter();
  }
  /* Emit global info */
  EmitGlobalInfo(m);
}

void MarkUsedStaticSymbol(const StIdx &symbolIdx);
std::map<StIdx, bool> visitedSym;

void CollectStaticSymbolInVar(MIRConst *mirConst) {
  if (mirConst->GetKind() == kConstAddrof) {
    auto *addrSymbol = static_cast<MIRAddrofConst*>(mirConst);
    MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(addrSymbol->GetSymbolIndex().Idx(), true);
    if (sym != nullptr) {
      MarkUsedStaticSymbol(sym->GetStIdx());
    }
  } else if (mirConst->GetKind() == kConstAggConst) {
    auto &constVec = static_cast<MIRAggConst*>(mirConst)->GetConstVec();
    for (auto &cst : constVec) {
      CollectStaticSymbolInVar(cst);
    }
  }
}

void MarkUsedStaticSymbol(const StIdx &symbolIdx) {
  if (!symbolIdx.IsGlobal()) {
    return;
  }
  MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolIdx.Idx(), true);
  if (symbol == nullptr) {
    return;
  }
  if (visitedSym[symbolIdx]) {
    return;
  } else {
    visitedSym[symbolIdx] = true;
  }
  symbol->ResetIsDeleted();
  if (symbol->IsConst()) {
    auto *konst = symbol->GetKonst();
    CollectStaticSymbolInVar(konst);
  }
}

void RecursiveMarkUsedStaticSymbol(const BaseNode *baseNode) {
  if (baseNode == nullptr) {
    return;
  }
  Opcode op = baseNode->GetOpCode();
  switch (op) {
    case OP_block: {
      const BlockNode *blk = static_cast<const BlockNode*>(baseNode);
      for (auto &stmt : blk->GetStmtNodes()) {
        RecursiveMarkUsedStaticSymbol(&stmt);
      }
      break;
    }
    case OP_dassign: {
      const DassignNode *dassignNode = static_cast<const DassignNode*>(baseNode);
      MarkUsedStaticSymbol(dassignNode->GetStIdx());
      break;
    }
    case OP_addrof:
    case OP_addrofoff:
    case OP_dread: {
      const AddrofNode *dreadNode = static_cast<const AddrofNode*>(baseNode);
      MarkUsedStaticSymbol(dreadNode->GetStIdx());
      break;
    }
    default: {
      break;
    }
  }
  for (size_t i = 0; i < baseNode->NumOpnds(); ++i) {
    RecursiveMarkUsedStaticSymbol(baseNode->Opnd(i));
  }
}

void CollectStaticSymbolInFunction(MIRFunction &func) {
  RecursiveMarkUsedStaticSymbol(func.GetBody());
}

void CgFuncPM::SweepUnusedStaticSymbol(MIRModule &m) const {
  if (!m.IsCModule()) {
    return;
  }
  size_t size = GlobalTables::GetGsymTable().GetSymbolTableSize();
  for (size_t i = 0; i < size; ++i) {
    MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(static_cast<uint32>(i));
    if (mirSymbol != nullptr && (mirSymbol->GetSKind() == kStVar || mirSymbol->GetSKind() == kStConst) &&
        (mirSymbol->GetStorageClass() == kScFstatic || mirSymbol->GetStorageClass() == kScPstatic) &&
        !mirSymbol->GetAccessByMem()) {
      mirSymbol->SetIsDeleted();
    }
  }

  visitedSym.clear();
  /* scan all funtions  */
  std::vector<MIRFunction*> &funcTable = GlobalTables::GetFunctionTable().GetFuncTable();
  /* don't optimize this loop to iterator or range-base loop
   * because AddCallGraphNode(mirFunc) will change GlobalTables::GetFunctionTable().GetFuncTable()
   */
  for (size_t index = 0; index < funcTable.size(); ++index) {
    MIRFunction *mirFunc = funcTable.at(index);
    if (mirFunc == nullptr || mirFunc->GetBody() == nullptr) {
      continue;
    }
    m.SetCurFunction(mirFunc);
    CollectStaticSymbolInFunction(*mirFunc);
    /* scan function symbol declaration
     * find addrof static const */
    MIRSymbolTable *funcSymTab = mirFunc->GetSymTab();
    if (funcSymTab) {
      size_t localSymSize = funcSymTab->GetSymbolTableSize();
      for (uint32 i = 0; i < localSymSize; ++i) {
        MIRSymbol *st = funcSymTab->GetSymbolFromStIdx(i);
        if (st && st->IsConst()) {
          MIRConst *mirConst = st->GetKonst();
          CollectStaticSymbolInVar(mirConst);
        }
      }
    }
  }
  /* scan global symbol declaration
   * find addrof static const */
  auto &symbolSet = m.GetSymbolSet();
  for (auto sit = symbolSet.begin(); sit != symbolSet.end(); ++sit) {
    MIRSymbol *s = GlobalTables::GetGsymTable().GetSymbolFromStidx(sit->Idx(), true);
    if (s->IsConst()) {
      MIRConst *mirConst = s->GetKonst();
      CollectStaticSymbolInVar(mirConst);
    }
  }
}

void InitFunctionPriority(std::map<std::string, uint32> &prioritylist) {
  std::string reorderAlgo = CGOptions::GetFunctionReorderAlgorithm();
  if (!reorderAlgo.empty()) {
    std::string reorderProfile = CGOptions::GetFunctionReorderProfile();
    if (reorderProfile.empty()) {
      LogInfo::MapleLogger() << "WARN: function reorder need profile"
                             << "\n";
    }
    if (reorderAlgo == "call-chain-clustering") {
      prioritylist = ReorderAccordingProfile(reorderProfile);
    } else {
      LogInfo::MapleLogger() << "WARN: function reorder algorithm no support"
                             << "\n";
    }
    return;
  }
  if (CGOptions::GetFunctionPriority() != "") {
    std::string fileName = CGOptions::GetFunctionPriority();
    std::ifstream in(fileName);
    if (!in.is_open()) {
      if (errno != ENOENT && errno != EACCES) {
        LogInfo::MapleLogger() << "WARN: instrumentation white list(" << "), failed to open " << fileName << '\n';
      }
      return;
    }
    std::string line;
    while (std::getline(in, line)) {
      std::vector<std::string> context;
      std::istringstream iss(line);
      std::string word;
      while (iss >> word) {
        context.push_back(word);
      }
      constexpr size_t priorityFormat = 2;
      if (context.size() == priorityFormat) {
        prioritylist.insert(std::make_pair(context[0], std::stoi(context[1])));
      } else {
        CHECK_FATAL(false, "unexpected format in Function Priority File");
      }
    }
  }
}

static void MarkFunctionPriority(std::map<std::string, uint32> &prioritylist, CGFunc &f) {
  if (!prioritylist.empty()) {
    if (prioritylist.count(f.GetName()) != 0) {
      f.SetPriority(prioritylist[f.GetName()]);
    }
  }
}

static std::optional<MapleList<MIRFunction *>> ReorderFunction(MIRModule &m,
                                                               const std::map<std::string, uint32> &priorityList) {
  if (!opts::linkerTimeOpt.IsEnabledByUser()) {
    return std::nullopt;
  }
  if (priorityList.empty()) {
    return std::nullopt;
  }
  MapleList<MIRFunction *> reorderdFunctionList(m.GetMPAllocator().Adapter());
  std::vector<MIRFunction *> hotFunctions(priorityList.size());
  for (auto f : m.GetFunctionList()) {
    if (priorityList.find(f->GetName()) != priorityList.end()) {
      CHECK_FATAL(hotFunctions.size() > priorityList.at(f->GetName()) - 1, "func priority out of range");
      hotFunctions[priorityList.at(f->GetName()) - 1] = f;
    } else {
      reorderdFunctionList.push_back(f);
    }
  }
  std::copy(hotFunctions.begin(), hotFunctions.end(), std::back_inserter(reorderdFunctionList));
  return reorderdFunctionList;
}

/* =================== new phase manager ===================  */
bool CgFuncPM::PhaseRun(MIRModule &m) {
  CreateCGAndBeCommon(m);
  bool changed = false;
  if (cgOptions->IsRunCG()) {
    GenerateOutPutFile(m);

    /* Run the cg optimizations phases */
    PrepareLower(m);
    std::map<std::string, uint32> priorityList;
    InitFunctionPriority(priorityList);

    auto reorderedFunctions = ReorderFunction(m, priorityList);

    if (opts::aggressiveTlsLocalDynamicOpt) {
      m.SetTlsAnchorHashString();
      PrepareForWarmupDynamicTlsSingleThread(m);
    } else if (opts::aggressiveTlsLocalDynamicOptMultiThread) {
      m.SetTlsAnchorHashString();
      PrepareForWarmupDynamicTlsMutiThread(m);
    }

    uint32 countFuncId = 0;
    unsigned long rangeNum = 0;

    auto userDefinedOptLevel = cgOptions->GetOptimizeLevel();
    cg->EnrollTargetPhases(this);

    auto admMempool = AllocateMemPoolInPhaseManager("cg phase manager's analysis data manager mempool");
    auto *serialADM = GetManagerMemPool()->New<AnalysisDataManager>(*(admMempool.get()));
    auto *funcList = &m.GetFunctionList();
    if (reorderedFunctions) {
      funcList = &reorderedFunctions.value();
    }
    for (auto it = funcList->begin(); it != funcList->end(); ++it) {
      ASSERT(serialADM->CheckAnalysisInfoEmpty(), "clean adm before function run");
      MIRFunction *mirFunc = *it;
      if (mirFunc->GetBody() == nullptr) {
        if (mirFunc->GetAttr(FUNCATTR_visibility_hidden)) {
          (void)cg->GetEmitter()->Emit("\t.hidden\t").Emit(mirFunc->GetName()).Emit("\n");
        } else if (mirFunc->GetAttr(FUNCATTR_visibility_protected)) {
          (void)cg->GetEmitter()->Emit("\t.protected\t").Emit(mirFunc->GetName()).Emit("\n");
        }
        continue;
      }

      if (userDefinedOptLevel == CGOptions::kLevel2 && m.HasPartO2List()) {
        if (m.IsInPartO2List(mirFunc->GetNameStrIdx())) {
          cgOptions->EnableO2();
        } else {
          cgOptions->EnableO0();
        }
        ClearAllPhases();
        cg->EnrollTargetPhases(this);
        cg->UpdateCGOptions(*cgOptions);
        Globals::GetInstance()->SetOptimLevel(cgOptions->GetOptimizeLevel());
      }
      if (!IsQuiet()) {
        LogInfo::MapleLogger() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Optimizing Function  < " << mirFunc->GetName()
                               << " id=" << mirFunc->GetPuidxOrigin() << " >---\n";
      }
      /* LowerIR. */
      m.SetCurFunction(mirFunc);

      if (cg->DoConstFold()) {
        DumpMIRFunc(*mirFunc, "************* before ConstantFold **************");
        ConstantFold cf(m);
        (void)cf.Simplify(mirFunc->GetBody());
      }

      if (m.GetFlavor() != MIRFlavor::kFlavorLmbc) {
        DoFuncCGLower(m, *mirFunc);
      }
      /* create CGFunc */
      MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(mirFunc->GetStIdx().Idx());
      auto funcMp = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, funcSt->GetName());
      auto stackMp = std::make_unique<StackMemPool>(funcMp->GetCtrler(), "");
      MapleAllocator funcScopeAllocator(funcMp.get());
      mirFunc->SetPuidxOrigin(++countFuncId);
      CGFunc *cgFunc = cg->CreateCGFunc(m, *mirFunc, *beCommon, *funcMp, *stackMp, funcScopeAllocator, countFuncId);
      CHECK_FATAL(cgFunc != nullptr, "Create CG Function failed in cg_phase_manager");
      CG::SetCurCGFunc(*cgFunc);
      MarkFunctionPriority(priorityList, *cgFunc);
      if (cgOptions->WithDwarf() && cgFunc->GetWithSrc()) {
        cgFunc->SetDebugInfo(m.GetDbgInfo());
      }
      /* Run the cg optimizations phases. */
      if (CGOptions::UseRange() && rangeNum >= CGOptions::GetRangeBegin() && rangeNum <= CGOptions::GetRangeEnd()) {
        CGOptions::EnableInRange();
      }
      changed = FuncLevelRun(*cgFunc, *serialADM);
      /* Delete mempool. */
      mirFunc->ReleaseCodeMemory();
      ++rangeNum;
      CGOptions::DisableInRange();
    }
    PostOutPut(m);
  } else {
    LogInfo::MapleLogger(kLlErr) << "Skipped generating .s because -no-cg is given" << '\n';
  }
  RELEASE(cg);
  RELEASE(beCommon);
  RELEASE(cgLower);
  return changed;
}

void CgFuncPM::DumpFuncCGIR(const CGFunc &f, const std::string &phaseName) const {
  if (CGOptions::DumpPhase(phaseName) && CGOptions::FuncFilter(f.GetName())) {
    LogInfo::MapleLogger() << "\n******** CG IR After " << phaseName << ": *********\n";
    f.DumpCGIR();
  }
}

void CgFuncPM::EmitGlobalInfo(MIRModule &m) const {
  EmitDuplicatedAsmFunc(m);
  EmitFastFuncs(m);
  if (cgOptions->IsGenerateObjectMap()) {
    cg->GenerateObjectMaps(*beCommon);
  }
  cg->GetEmitter()->EmitGlobalVariable();
  EmitDebugInfo(m);
  cg->GetEmitter()->CloseOutput();
}

void CgFuncPM::InitProfile(MIRModule &m)  const {
  if (!CGOptions::IsProfileDataEmpty()) {
    uint32 dexNameIdx = m.GetFileinfo(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename"));
    const std::string &dexName = GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(dexNameIdx));
    bool deCompressSucc = m.GetProfile().DeCompress(CGOptions::GetProfileData(), dexName);
    if (!deCompressSucc) {
      LogInfo::MapleLogger() << "WARN: DeCompress() " << CGOptions::GetProfileData() << "failed in mplcg()\n";
    }
  }
  if (!CGOptions::GetLitePgoWhiteList().empty()) {
    bool handleSucc = m.GetLiteProfile().HandleLitePgoWhiteList(CGOptions::GetLitePgoWhiteList());
    if (!handleSucc) {
      LogInfo::MapleLogger() << "WARN: Handle lite-pgo white list file " <<
                                CGOptions::GetLitePgoWhiteList() <<
                                "failed in mplcg\n";
    }
  }
  if (!CGOptions::GetLiteProfile().empty()) {
    bool handleSucc = m.GetLiteProfile().HandleLitePGOFile(CGOptions::GetLiteProfile(), m);
    CHECK_FATAL(handleSucc, "Error: Handle Lite PGO input file ",
                CGOptions::GetLiteProfile().c_str(), "failed in mplcg");
  }
}

void CgFuncPM::CreateCGAndBeCommon(MIRModule &m) {
  ASSERT(cgOptions != nullptr, "New cg phase manager running FAILED  :: cgOptions unset");
#if TARGAARCH64 || TARGRISCV64
  cg = new AArch64CG(m, *cgOptions, cgOptions->GetEHExclusiveFunctionNameVec(), CGOptions::GetCyclePatternMap());
  cg->SetEmitter(*m.GetMemPool()->New<AArch64AsmEmitter>(*cg, m.GetOutputFileName()));
#elif defined(TARGARM32) && TARGARM32
  cg = new Arm32CG(m, *cgOptions, cgOptions->GetEHExclusiveFunctionNameVec(), CGOptions::GetCyclePatternMap());
  cg->SetEmitter(*m.GetMemPool()->New<Arm32AsmEmitter>(*cg, m.GetOutputFileName()));
#elif defined(TARGX86_64) && TARGX86_64
  cg = new X64CG(m, *cgOptions);
  cg->SetEmitter(*m.GetMemPool()->New<X64Emitter>(*cg, m.GetOutputFileName()));
#else
#error "unknown platform"
#endif
  /*
   * Must be done before creating any BECommon instances.
   *
   * BECommon, when constructed, will calculate the type, size and align of all types.  As a side effect, it will also
   * lower ptr and ref types into a64. That will drop the information of what a ptr or ref points to.
   *
   * All metadata generation passes which depend on the pointed-to type must be done here.
   */
  cg->GenPrimordialObjectList(m.GetBaseName());
  if (CGOptions::DoLiteProfGen()) {
    CGProfGen::CreateProfInitExitFunc(m);
    CGProfGen::CreateProfFileSym(m, CGOptions::GetInstrumentationOutPutPath(), "__mpl_pgo_dump_filename");
    CGProfGen::CreateChildTimeSym(m, "__mpl_pgo_sleep_time");
  }
  /* We initialize a couple of BECommon's tables using the size information of GlobalTables.type_table_.
   * So, BECommon must be allocated after all the parsing is done and user-defined types are all acounted.
   */
  beCommon = new BECommon(m);
  Globals::GetInstance()->SetBECommon(*beCommon);
  Globals::GetInstance()->SetTarget(*cg);

  /* If a metadata generation pass depends on object layout it must be done after creating BECommon. */
  cg->GenExtraTypeMetadata(cgOptions->GetClassListFile(), m.GetBaseName());

#if TARGAARCH64
  if (!m.IsCModule()) {
    CGOptions::SetFramePointer(CGOptions::kAllFP);
  }
#endif
}

void CgFuncPM::PrepareLower(MIRModule &m) {
  mirLower = GetManagerMemPool()->New<MIRLower>(m, nullptr);
  mirLower->Init();
  cgLower = new CGLowerer(m,
      *beCommon, *GetPhaseMemPool(), cg->GenerateExceptionHandlingCode(), cg->GenerateVerboseCG());
  cgLower->RegisterBuiltIns();
  if (m.IsJavaModule()) {
    cgLower->InitArrayClassCacheTableIndex();
  }
  cgLower->RegisterExternalLibraryFunctions();
  cgLower->SetCheckLoadStore(CGOptions::IsCheckArrayStore());
  if (cg->IsStackProtectorStrong() || cg->IsStackProtectorAll() || m.HasPartO2List()) {
    cg->AddStackGuardvar();
  }
}

void CgFuncPM::DoFuncCGLower(const MIRModule &m, MIRFunction &mirFunc) const {
  if (m.GetFlavor() <= kFeProduced) {
    mirLower->SetLowerCG();
    mirLower->SetMirFunc(&mirFunc);
    mirLower->SetOptLevel(static_cast<uint8>(CGOptions::GetInstance().GetOptimizeLevel()));

    DumpMIRFunc(mirFunc, "************* before MIRLowerer **************");
    mirLower->LowerFunc(mirFunc);
  }

  bool isNotQuiet = !CGOptions::IsQuiet();
  DumpMIRFunc(mirFunc, "************* before CGLowerer **************", isNotQuiet);

  cgLower->LowerFunc(mirFunc);

  DumpMIRFunc(mirFunc, "************* after  CGLowerer **************", isNotQuiet,
              "************* end    CGLowerer **************");
}

void CgFuncPM::EmitDuplicatedAsmFunc(MIRModule &m) const {
  if (CGOptions::IsDuplicateAsmFileEmpty()) {
    return;
  }

  std::ifstream duplicateAsmFileFD(CGOptions::GetDuplicateAsmFile());

  if (!duplicateAsmFileFD.is_open()) {
    duplicateAsmFileFD.close();
    ERR(kLncErr, " %s open failed!", CGOptions::GetDuplicateAsmFile().c_str());
    return;
  }
  std::string contend;
  bool onlyForFramework = false;
  bool isFramework = IsFramework(m);

  while (getline(duplicateAsmFileFD, contend)) {
    if (contend.compare("#Libframework_start") == 0) {
      onlyForFramework = true;
    }

    if (contend.compare("#Libframework_end") == 0) {
      onlyForFramework = false;
    }

    if (onlyForFramework && !isFramework) {
      continue;
    }

    (void)cg->GetEmitter()->Emit(contend + "\n");
  }
  duplicateAsmFileFD.close();
}

void CgFuncPM::EmitFastFuncs(const MIRModule &m) const {
  if (CGOptions::IsFastFuncsAsmFileEmpty() || !(m.IsJavaModule())) {
    return;
  }

  struct stat buffer;
  if (stat(CGOptions::GetFastFuncsAsmFile().c_str(), &buffer) != 0) {
    return;
  }

  std::ifstream fastFuncsAsmFileFD(CGOptions::GetFastFuncsAsmFile());
  if (fastFuncsAsmFileFD.is_open()) {
    std::string contend;
    (void)cg->GetEmitter()->Emit("#define ENABLE_LOCAL_FAST_FUNCS 1\n");

    while (getline(fastFuncsAsmFileFD, contend)) {
      (void)cg->GetEmitter()->Emit(contend + "\n");
    }
  }
  fastFuncsAsmFileFD.close();
}

void CgFuncPM::EmitDebugInfo(const MIRModule &m) const {
  if (!cgOptions->WithDwarf()) {
    return;
  }
  cg->GetEmitter()->SetupDBGInfo(m.GetDbgInfo());
  cg->GetEmitter()->EmitDIHeaderFileInfo();
  cg->GetEmitter()->EmitDIDebugInfoSection(m.GetDbgInfo());
  cg->GetEmitter()->EmitDIDebugAbbrevSection(m.GetDbgInfo());
  cg->GetEmitter()->EmitDIDebugARangesSection();
  cg->GetEmitter()->EmitDIDebugRangesSection();
  cg->GetEmitter()->EmitDIDebugLineSection();
  cg->GetEmitter()->EmitDIDebugStrSection();
}

bool CgFuncPM::IsFramework(MIRModule &m) const {
  auto &funcList = m.GetFunctionList();
  for (auto it = funcList.cbegin(); it != funcList.cend(); ++it) {
    MIRFunction *mirFunc = *it;
    ASSERT(mirFunc != nullptr, "nullptr check");
    if (mirFunc->GetBody() != nullptr &&
        mirFunc->GetName() == "Landroid_2Fos_2FParcel_3B_7CnativeWriteString_7C_28JLjava_2Flang_2FString_3B_29V") {
      return true;
    }
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgFuncPM, cgFuncPhaseManager)
/* register codegen common phases */
MAPLE_TRANSFORM_PHASE_REGISTER(CgLayoutFrame, layoutstackframe)
MAPLE_TRANSFORM_PHASE_REGISTER(CgCreateLabel, createstartendlabel)
MAPLE_ANALYSIS_PHASE_REGISTER(AbstractBuilder, abstarctbuilder)
MAPLE_TRANSFORM_PHASE_REGISTER(InstructionSelector, instructionselector)
MAPLE_TRANSFORM_PHASE_REGISTER(InstructionStandardize, instructionstandardize)
MAPLE_TRANSFORM_PHASE_REGISTER(CgMoveRegArgs, moveargs)
MAPLE_TRANSFORM_PHASE_REGISTER(CgRegAlloc, regalloc)
MAPLE_TRANSFORM_PHASE_REGISTER(CgAlignAnalysis, alignanalysis)
MAPLE_TRANSFORM_PHASE_REGISTER(CgFrameFinalize, framefinalize)
MAPLE_TRANSFORM_PHASE_REGISTER(CgYieldPointInsertion, yieldpoint)
MAPLE_TRANSFORM_PHASE_REGISTER(CgGenProEpiLog, generateproepilog)
}  /* namespace maplebe */
