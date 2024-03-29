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
#include "cg_pgo_gen.h"
#include "optimize_common.h"
#include "instrument.h"
#include "itab_util.h"
#include "cg.h"

namespace maplebe {
uint64 CGProfGen::counterIdx = 0;
std::string AppendModSpecSuffix(const MIRModule &m) {
  std::string specSuffix = "_";
  specSuffix = specSuffix + std::to_string(DJBHash(m.GetEntryFuncName().c_str()) + m.GetNumFuncs());
  return specSuffix;
}

static inline void CreateFuncInfo(
    MIRModule &m, MIRType *funcProfInfoTy, const std::vector<MIRFunction*> &validFuncs, std::vector<MIRSymbol*> &syms) {
  MIRType *voidPtrTy = GlobalTables::GetTypeTable().GetVoidPtr();
  MIRType *u32Ty = GlobalTables::GetTypeTable().GetUInt32();
  for (MIRFunction *f : validFuncs) {
    auto *funcInfoMirConst = m.GetMemPool()->New<MIRAggConst>(m, *funcProfInfoTy);

    PUIdx puID = f->GetPuidx();
    auto *funcNameMirConst =
        m.GetMemPool()->New<MIRStrConst>(f->GetName(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_a64)));
    funcInfoMirConst->AddItem(funcNameMirConst, 1);
    /* counter array and cfg hash will be fixed up later after instrumentation analysis */
    MIRIntConst *zeroMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *u32Ty);
    funcInfoMirConst->AddItem(zeroMirConst, 2);
    funcInfoMirConst->AddItem(zeroMirConst, 3);
    MIRIntConst *voidMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *voidPtrTy);
    funcInfoMirConst->AddItem(voidMirConst, 4);

    MIRSymbol *funcInfoSym = m.GetMIRBuilder()->CreateGlobalDecl(
        namemangler::kprefixProfFuncDesc + std::to_string(puID), *funcProfInfoTy, kScFstatic);
    funcInfoSym->SetKonst(funcInfoMirConst);
    syms.emplace_back(funcInfoSym);
  }
};

static inline MIRSymbol *GetOrCreateFuncInfoTbl(MIRModule &m, const std::vector<MIRFunction*> &validFuncs) {
  std::string srcFN = m.GetFileName();
  std::string useName = namemangler::kprefixProfCtrTbl + LiteProfile::FlatenName(srcFN);
  auto nameStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(useName);
  MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(nameStrIdx);
  if (sym != nullptr) {
    return sym;
  }

  FieldVector parentFields;
  FieldVector fields;
  //  Create function profile info type                                                      // Field
  GStrIdx funcNameStrIdx = m.GetMIRBuilder()->GetOrCreateStringIndex("func_name");       // 1
  GStrIdx cfgHashStrIdx = m.GetMIRBuilder()->GetOrCreateStringIndex("cfg_hash");         // 2
  GStrIdx cntNumStrIdx = m.GetMIRBuilder()->GetOrCreateStringIndex("counter_num");       // 3
  GStrIdx cntArrayStrIdx = m.GetMIRBuilder()->GetOrCreateStringIndex("counter_array");   // 4

  MIRType *voidPtrTy = GlobalTables::GetTypeTable().GetVoidPtr();
  MIRType *u32Ty = GlobalTables::GetTypeTable().GetUInt32();
  TyIdx charPtrTyIdx = GlobalTables::GetTypeTable().GetOrCreatePointerType(TyIdx(PTY_u8))->GetTypeIndex();
  fields.emplace_back(funcNameStrIdx, TyIdxFieldAttrPair(charPtrTyIdx, FieldAttrs()));
  fields.emplace_back(cfgHashStrIdx, TyIdxFieldAttrPair(u32Ty->GetTypeIndex(), FieldAttrs()));
  fields.emplace_back(cntNumStrIdx, TyIdxFieldAttrPair(u32Ty->GetTypeIndex(), FieldAttrs()));
  fields.emplace_back(cntArrayStrIdx, TyIdxFieldAttrPair(voidPtrTy->GetTypeIndex(), FieldAttrs()));

  MIRType *funcProfInfoTy =
      GlobalTables::GetTypeTable().GetOrCreateStructType("__mpl_func_info_ty", fields, parentFields, m);

  std::vector<MIRSymbol *> allFuncInfoSym;
  CreateFuncInfo(m, funcProfInfoTy, validFuncs, allFuncInfoSym);

  MIRType *funcProfInfoPtrTy = GlobalTables::GetTypeTable().GetOrCreatePointerType(funcProfInfoTy->GetTypeIndex());
  MIRType *arrOfFuncInfoPtrTy = GlobalTables::GetTypeTable().GetOrCreateArrayType(
      *funcProfInfoPtrTy, static_cast<uint32>(validFuncs.size()));
  sym = m.GetMIRBuilder()->CreateGlobalDecl(useName, *arrOfFuncInfoPtrTy, kScFstatic);
  auto *funcInfoTblMirConst = m.GetMemPool()->New<MIRAggConst>(m, *arrOfFuncInfoPtrTy);
  for (size_t i = 0; i < allFuncInfoSym.size(); ++i) {
    auto *funcInfoMirConst = m.GetMemPool()->New<MIRAddrofConst>(
        allFuncInfoSym[i]->GetStIdx(), 0, *GlobalTables::GetTypeTable().GetPtr());
    funcInfoTblMirConst->AddItem(funcInfoMirConst, static_cast<uint32>(i));
  }
  sym->SetKonst(funcInfoTblMirConst);
  return sym;
}

static inline MIRSymbol *GetOrCreateModuleInfo(MIRModule &m, const std::vector<MIRFunction *>&validFuncs) {
  std::string srcFN = m.GetFileName();
  std::string useName = namemangler::kprefixProfModDesc  + LiteProfile::FlatenName(srcFN);
  auto nameStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(useName);
  MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(nameStrIdx);
  if (sym != nullptr) {
    return sym;
  }

  FieldVector parentFields;
  FieldVector fields;
  /* Create module scope profile descriptor type */                                        // Field
  GStrIdx modNameStrIdx = m.GetMIRBuilder()->GetOrCreateStringIndex("mod_hash");        // 1
  GStrIdx nextModStrIdx = m.GetMIRBuilder()->GetOrCreateStringIndex("next_mod");        // 2
  GStrIdx funcNumStrIdx = m.GetMIRBuilder()->GetOrCreateStringIndex("func_number");     // 3
  GStrIdx funcPtrStrIdx = m.GetMIRBuilder()->GetOrCreateStringIndex("func_info_ptr");   // 4

  MIRType *u64Ty = GlobalTables::GetTypeTable().GetUInt64();
  TyIdx u64TyIdx = u64Ty->GetTypeIndex();
  MIRType *u32Ty = GlobalTables::GetTypeTable().GetUInt32();
  TyIdx u32TyIdx = u32Ty->GetTypeIndex();
  TyIdx ptrTyIdx = GlobalTables::GetTypeTable().GetPtr()->GetTypeIndex();
  MIRType *voidPtrTy = GlobalTables::GetTypeTable().GetVoidPtr();
  fields.emplace_back(FieldPair(modNameStrIdx, TyIdxFieldAttrPair(u32TyIdx, FieldAttrs())));
  fields.emplace_back(FieldPair(nextModStrIdx, TyIdxFieldAttrPair(ptrTyIdx, FieldAttrs())));
  fields.emplace_back(FieldPair(funcNumStrIdx, TyIdxFieldAttrPair(u64TyIdx, FieldAttrs())));
  fields.emplace_back(FieldPair(funcPtrStrIdx, TyIdxFieldAttrPair(ptrTyIdx, FieldAttrs())));

  MIRType *modInfoTy = GlobalTables::GetTypeTable().GetOrCreateStructType(useName + "_ty", fields, parentFields, m);
  sym = m.GetMIRBuilder()->CreateGlobalDecl(useName, *modInfoTy, kScFstatic);

  /* Initialize fields */
  auto *modProfSymMirConst = m.GetMemPool()->New<MIRAggConst>(m, *modInfoTy);
  MIRIntConst *modHashMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
      DJBHash(LiteProfile::FlatenName(m.GetFileName()).c_str()), *u32Ty);
  modProfSymMirConst->AddItem(modHashMirConst, 1);
  MIRIntConst *nextMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *voidPtrTy);
  modProfSymMirConst->AddItem(nextMirConst, 2);
  MIRIntConst *funcNumMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(validFuncs.size(), *u64Ty);
  modProfSymMirConst->AddItem(funcNumMirConst, 3);
  MIRSymbol *funcTblSym = GetOrCreateFuncInfoTbl(m, validFuncs);
  auto *tblConst = m.GetMemPool()->New<MIRAddrofConst>(
      funcTblSym->GetStIdx(), 0, *GlobalTables::GetTypeTable().GetPtr());
  modProfSymMirConst->AddItem(tblConst, 4);

  sym->SetKonst(modProfSymMirConst);
  return sym;
}

void CGProfGen::CreateProfFileSym(MIRModule &m, std::string &outputPath, const std::string &symName) {
  auto *mirBuilder = m.GetMIRBuilder();
  auto *charPtrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_a64));
  if (!outputPath.empty() && outputPath.back() != '/') {
    outputPath.append("/");
  }
  const std::string finalName = outputPath + "mpl_lite_pgo.data";
  auto *modNameMirConst = m.GetMemPool()->New<MIRStrConst>(finalName, *charPtrType);
  auto *funcPtrSym = mirBuilder->GetOrCreateGlobalDecl(symName, *charPtrType);
  funcPtrSym->SetAttr(ATTR_weak);  // weak symbol
  funcPtrSym->SetKonst(modNameMirConst);
}

void CGProfGen::CreateChildTimeSym(maple::MIRModule &m, const std::string &symName) {
  auto *mirBuilder = m.GetMIRBuilder();
  auto *u32Type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u32));
  auto *sleepTimeMirConst = m.GetMemPool()->New<MIRIntConst>(0, *u32Type);
  auto *funcPtrSym = mirBuilder->GetOrCreateGlobalDecl(symName, *u32Type);
  funcPtrSym->SetAttr(ATTR_weak);  // weak symbol
  funcPtrSym->SetKonst(sleepTimeMirConst);

  // if time is set. wait_fork is required to be set
  auto *u8Type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u8));
  auto *waitForkMirConst = m.GetMemPool()->New<MIRIntConst>(0, *u8Type);
  auto *waitForkSym = mirBuilder->GetOrCreateGlobalDecl("__mpl_pgo_wait_forks", *u8Type);
  waitForkSym->SetAttr(ATTR_weak);  // weak symbol
  waitForkSym->SetKonst(waitForkMirConst);
}

void CGProfGen::CreateProfInitExitFunc(MIRModule &m) {
  std::vector<MIRFunction *> validFuncs;
  for (MIRFunction *mirF : std::as_const(m.GetFunctionList())) {
    validFuncs.push_back(mirF);
  }
  /* call entry in libmplpgo.so */
  auto profEntry = std::string("__" + LiteProfile::FlatenName(m.GetFileName()) + AppendModSpecSuffix(m) + "_init");
  ArgVector formals(m.GetMPAllocator().Adapter());
  MIRType *voidTy = GlobalTables::GetTypeTable().GetVoid();
  auto *newEntry  = m.GetMIRBuilder()->CreateFunction(profEntry, *voidTy, formals);
  newEntry->SetWithSrc(false);
  m.SetCurFunction(newEntry);
  auto *initInSo = m.GetMIRBuilder()->GetOrCreateFunction("__mpl_pgo_init", TyIdx(PTY_void));
  initInSo->SetWithSrc(false);
  auto *entryBody = newEntry->GetCodeMempool()->New<BlockNode>();
  auto &entryAlloc = newEntry->GetCodeMempoolAllocator();
  auto *callMINode = newEntry->GetCodeMempool()->New<CallNode>(entryAlloc, OP_call, initInSo->GetPuidx());
  MapleVector<BaseNode*> arg(entryAlloc.Adapter());
  arg.push_back(m.GetMIRBuilder()->CreateExprAddrof(0, *GetOrCreateModuleInfo(m, validFuncs)));
  callMINode->SetOpnds(arg);
  entryBody->AddStatement(callMINode);
  newEntry->SetBody(entryBody);
  newEntry->SetAttr(FUNCATTR_section);
  newEntry->GetFuncAttrs().SetPrefixSectionName(".init_array");
  m.AddFunction(newEntry);

  /* call setup in mplpgo.so if it needs mutex && dump in child process */
  auto profSetup = std::string("__" + LiteProfile::FlatenName(m.GetFileName()) + AppendModSpecSuffix(m) +  "_setup");
  auto *newSetup = m.GetMIRBuilder()->CreateFunction(profSetup, *voidTy, formals);
  newSetup->SetWithSrc(false);
  m.SetCurFunction(newSetup);
  auto *setupInSo = m.GetMIRBuilder()->GetOrCreateFunction("__mpl_pgo_setup", TyIdx(PTY_void));
  setupInSo->SetWithSrc(false);
  auto *setupBody = newSetup->GetCodeMempool()->New<BlockNode>();
  auto &setupAlloc = newSetup->GetCodeMempoolAllocator();
  setupBody->AddStatement(newSetup->GetCodeMempool()->New<CallNode>(setupAlloc, OP_call, setupInSo->GetPuidx()));
  newSetup->SetBody(setupBody);
  newSetup->SetAttr(FUNCATTR_section);
  newSetup->GetFuncAttrs().SetPrefixSectionName(".init_array");
  m.AddFunction(newSetup);

  /* call exit in libmplpgo.so */
  auto profExit = std::string("__" + LiteProfile::FlatenName(m.GetFileName()) + AppendModSpecSuffix(m) + "_exit");
  auto *newExit = m.GetMIRBuilder()->CreateFunction(profExit, *voidTy, formals);
  newExit->SetWithSrc(false);
  m.SetCurFunction(newExit);
  auto *exitInSo = m.GetMIRBuilder()->GetOrCreateFunction("__mpl_pgo_exit", TyIdx(PTY_void));
  exitInSo->SetWithSrc(false);
  auto *exitBody = newExit->GetCodeMempool()->New<BlockNode>();
  auto &exitAlloc = newExit->GetCodeMempoolAllocator();
  exitBody->AddStatement(newExit->GetCodeMempool()->New<CallNode>(exitAlloc, OP_call, exitInSo->GetPuidx()));
  newExit->SetBody(exitBody);
  newExit->SetAttr(FUNCATTR_section);
  newExit->GetFuncAttrs().SetPrefixSectionName(".fini_array");
  m.AddFunction(newExit);
}

void CGProfGen::InstrumentFunction() {
  instrumenter.PrepareInstrumentInfo(f->GetFirstBB(), f->GetCommonExitBB());
  std::vector<maplebe::BB*> iBBs;
  instrumenter.GetInstrumentBBs(iBBs, f->GetFirstBB());

  /* skip large bb function currently due to offset in ldr/store */
  if (iBBs.size() > kMaxPimm8) {
    /* fixed by adding jumping label between large counter section */
    return;
  }

  if (f->GetFunction().GetAttr(FUNCATTR_section)) {
    const std::string &sectionName = f->GetFunction().GetAttrs().GetPrefixSectionName();
    if (sectionName == ".init_array" || sectionName == ".fini_array") {
      return;
    }
  }

  uint32 oldTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
  CGCFG *cfg = f->GetTheCFG();
  CHECK_FATAL(cfg != nullptr, "exit");

  MIRSymbol *bbCounterTab = GetOrCreateFuncCounter(f->GetFunction(),
      static_cast<uint32>(iBBs.size()), cfg->ComputeCFGHash());
  BECommon *be = Globals::GetInstance()->GetBECommon();
  uint32 newTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
  if (newTypeTableSize != oldTypeTableSize) {
    ASSERT(be, "get be in CGProfGen::InstrumentFunction() failed");
    be->AddNewTypeAfterBecommon(oldTypeTableSize, newTypeTableSize);
  }
  for (uint32 i = 0; i < iBBs.size(); ++i) {
    InstrumentBB(*iBBs[i], *bbCounterTab, i);
  }
}

void CGProfGen::CreateProfileCalls() {
  static bool created = false;
  if (created) {
    return;
  }
  created = true;

  /* Create symbol for recording call times */
  auto *mirBuilder = f->GetFunction().GetModule()->GetMIRBuilder();

  auto *initInSo = mirBuilder->GetOrCreateFunction("__mpl_pgo_dump_wrapper", TyIdx(PTY_void));

  /*
   * provide calling abort in specific BB
   * CreateCallForAbort(${bbid);
   */
  /* Insert SaveProfile */
  for (auto *bb : f->GetCommonExitBB()->GetPreds()) {
    CreateCallForDump(*bb, *initInSo->GetFuncSymbol());
  }
}

void CgPgoGen::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgLiveAnalysis>();
}

bool CgPgoGen::PhaseRun(maplebe::CGFunc &f) {
  if (!LiteProfile::IsInWhiteList(f.GetName()) && f.GetName() != CGOptions::GetLitePgoOutputFunction()) {
    return false;
  }
  if (f.NumBBs() > LiteProfile::GetBBNoThreshold()) {
    LogInfo::MapleLogger() << "Oops! The total number of BBs(" << f.NumBBs() << ") of [" << f.GetName() <<
        "] exceeds the limit(" << LiteProfile::GetBBNoThreshold() << "), " <<
        "we can not currently support instrumentation\n";
    return false;
  }

  auto *live = GET_ANALYSIS(CgLiveAnalysis, f);
  /* revert liveanalysis result container. */
  ASSERT(live != nullptr, "nullptr check");
  live->ResetLiveSet();

  CGProfGen *cgProfGeg = f.GetCG()->CreateCGProfGen(*GetPhaseMemPool(), f);
  cgProfGeg->InstrumentFunction();
  if (!CGOptions::GetLitePgoOutputFunction().empty() && f.GetName() == CGOptions::GetLitePgoOutputFunction()) {
    cgProfGeg->CreateProfileCalls();
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgPgoGen, cgpgogen)
}
