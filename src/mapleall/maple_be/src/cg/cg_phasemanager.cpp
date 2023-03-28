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
// the TLSLD entry of the module will be recorded in a global accessable symbol
void PrepareForWarmupDynamicTLS(MIRModule &m) {
  if (m.GetTdataVarOffset().empty() && m.GetTbssVarOffset().empty()) {
    return;
  }
  auto *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_ptr));
  auto *anchorMirConst = m.GetMemPool()->New<MIRIntConst>(0, *ptrType);

  ArgVector formals(m.GetMPAllocator().Adapter());
  MIRType *voidTy = GlobalTables::GetTypeTable().GetVoid();
  auto *tlsWarmup = m.GetMIRBuilder()->CreateFunction("__tls_address_warmup_" + m.GetTlsAnchorHashString(), *voidTy, formals);
  auto *warmupBody = tlsWarmup->GetCodeMempool()->New<BlockNode>();
  MIRSymbol *tempAnchorSym = nullptr;
  AddrofNode *tlsAddrNode = nullptr;
  DassignNode *dassignNode = nullptr;

  if (!m.GetTdataVarOffset().empty()) {
    auto *tdataAnchorSym = m.GetMIRBuilder()->GetOrCreateGlobalDecl("tdata_addr_" + m.GetTlsAnchorHashString(), *ptrType);
    tdataAnchorSym->SetKonst(anchorMirConst);
    tempAnchorSym = m.GetMIRBuilder()->GetOrCreateGlobalDecl(".tdata_start_" + m.GetTlsAnchorHashString(), *ptrType);
    tempAnchorSym->SetIsDeleted();
    tlsAddrNode = tlsWarmup->GetCodeMempool()->New<AddrofNode>(OP_addrof, PTY_ptr, tempAnchorSym->GetStIdx(), 0);
    dassignNode = tlsWarmup->GetCodeMempool()->New<DassignNode>();
    dassignNode->SetStIdx(tdataAnchorSym->GetStIdx());
    dassignNode->SetFieldID(0);
    dassignNode->SetOpnd(tlsAddrNode, 0);
    warmupBody->AddStatement(dassignNode);
  }

  if (!m.GetTbssVarOffset().empty()) {
    auto *tbssAnchorSym = m.GetMIRBuilder()->GetOrCreateGlobalDecl("tbss_addr_" + m.GetTlsAnchorHashString(), *ptrType);
    tbssAnchorSym->SetKonst(anchorMirConst);
    tempAnchorSym = m.GetMIRBuilder()->GetOrCreateGlobalDecl(".tbss_start_" + m.GetTlsAnchorHashString(), *ptrType);
    tempAnchorSym->SetIsDeleted();
    tlsAddrNode = tlsWarmup->GetCodeMempool()->New<AddrofNode>(OP_addrof, PTY_ptr, tempAnchorSym->GetStIdx(), 0);
    dassignNode = tlsWarmup->GetCodeMempool()->New<DassignNode>();
    dassignNode->SetStIdx(tbssAnchorSym->GetStIdx());
    dassignNode->SetFieldID(0);
    dassignNode->SetOpnd(tlsAddrNode, 0);
    warmupBody->AddStatement(dassignNode);
  }

  tlsWarmup->SetBody(warmupBody);
  tlsWarmup->SetAttr(FUNCATTR_section);
  tlsWarmup->GetFuncAttrs().SetPrefixSectionName(".init_array");
  m.AddFunction(tlsWarmup);
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
    if (mirFunc->GetBody() == nullptr) {
      continue;
    }
    MIRSymbolTable *lSymTab = mirFunc->GetSymTab();
    if (lSymTab == nullptr) {
      continue;
    }
    size_t lsize = lSymTab->GetSymbolTableSize();
    for (size_t i = 0; i < lsize; i++) {
      const MIRSymbol *mirSymbol = lSymTab->GetSymbolFromStIdx(static_cast<uint32>(i));
      if (mirSymbol == nullptr || mirSymbol->GetStorageClass() != kScPstatic) {
        continue;
      }
      if (mirSymbol->IsThreadLocal()) {
        mirType = mirSymbol->GetType();
        if (!mirSymbol->IsConst()) {
          tbssOffset = RoundUp(tbssOffset, mirType->GetAlign());
          tbssVarOffset[mirSymbol] = tbssOffset;
          tbssOffset += mirType->GetSize();
        } else {
          tdataOffset = RoundUp(tdataOffset, mirType->GetAlign());
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
    if (mirSymbol->IsThreadLocal()) {
      mirType = mirSymbol->GetType();
      if (!mirSymbol->IsConst()) {
        tbssOffset = RoundUp(tbssOffset, mirType->GetAlign());
        tbssVarOffset[mirSymbol] = tbssOffset;
        tbssOffset += mirType->GetSize();
      } else {
        tdataOffset = RoundUp(tdataOffset, mirType->GetAlign());
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
        (mirSymbol->GetStorageClass() == kScFstatic || mirSymbol->GetStorageClass() == kScPstatic)) {
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
#ifdef RA_PERF_ANALYSIS
#include "reg_alloc_lsra.h"
#endif

bool CgFuncPM::PhaseRun(MIRModule &m) {
  CreateCGAndBeCommon(m);
  bool changed = false;
  /* reserve static symbol for debugging */
  if (!cgOptions->WithDwarf()) {
    SweepUnusedStaticSymbol(m);
  }
  if (cgOptions->IsRunCG()) {
    GenerateOutPutFile(m);

    /* Run the cg optimizations phases */
    PrepareLower(m);
    std::map<std::string, uint32> priorityList;
    InitFunctionPriority(priorityList);

    auto reorderedFunctions = ReorderFunction(m, priorityList);

    if (CGOptions::IsShlib() && CGOptions::GetTLSModel() == CGOptions::kLocalDynamicTLSModel) {
      m.SetTlsAnchorHashString();
      CalculateWarmupDynamicTLS(m);
      PrepareForWarmupDynamicTLS(m);
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
      if (cgOptions->WithDwarf()) {
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
#ifdef RA_PERF_ANALYSIS
    if (cgOptions->IsEnableTimePhases()) {
      printLSRATime();
      printRATime();
    }
#endif
  } else {
    LogInfo::MapleLogger(kLlErr) << "Skipped generating .s because -no-cg is given" << '\n';
  }
  RELEASE(cg);
  RELEASE(beCommon);
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
    bool handleSucc = m.GetLiteProfile().HandleLitePGOFile(CGOptions::GetLiteProfile(), m.GetFileName());
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
  cgLower = GetManagerMemPool()->New<CGLowerer>(m,
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
    mirLower->SetOptLevel(CGOptions::GetInstance().GetOptimizeLevel());

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
MAPLE_TRANSFORM_PHASE_REGISTER(CgIsolateFastPath, isolatefastpath)
}  /* namespace maplebe */
