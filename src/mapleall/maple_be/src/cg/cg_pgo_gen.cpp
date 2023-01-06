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
std::string FlatenName(const std::string &name) {
  std::string filteredName = name;
  size_t startPos = name.find_last_of("/") == std::string::npos ? 0 : name.find_last_of("/") + 1U; // skip /
  size_t endPos = name.find_last_of(".") == std::string::npos ? 0 : name.find_last_of(".");
  CHECK_FATAL(endPos > startPos, "invalid module name");
  filteredName = filteredName.substr(startPos, endPos - startPos);
  std::replace(filteredName.begin(), filteredName.end(), '-', '_');
  return filteredName;
}

static inline std::vector<MIRSymbol*> CreateFuncInfo(
    MIRModule &m, MIRType *funcProfInfoTy, const std::vector<MIRFunction*>&validFuncs) {
  MIRType *voidPtrTy = GlobalTables::GetTypeTable().GetVoidPtr();
  MIRType *u64Ty = GlobalTables::GetTypeTable().GetUInt64();
  std::vector<MIRSymbol *> allFuncInfoSym;
  for (MIRFunction *f : validFuncs) {
    auto *funcInfoMirConst = m.GetMemPool()->New<MIRAggConst>(m, *funcProfInfoTy);

    PUIdx puID = f->GetPuidx();
    auto *funcNameMirConst =
        m.GetMemPool()->New<MIRStrConst>(f->GetName(), *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_a64)));
    funcInfoMirConst->AddItem(funcNameMirConst, 1);
    /* counter array will be fixed up later after instrumentation analysis */
    MIRIntConst *zeroMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(puID, *u64Ty);
    funcInfoMirConst->AddItem(zeroMirConst, 2);
    MIRIntConst *voidMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *voidPtrTy);
    funcInfoMirConst->AddItem(voidMirConst, 3);

    MIRSymbol *funcInfoSym = m.GetMIRBuilder()->CreateGlobalDecl(
        namemangler::kprefixProfFuncDesc + std::to_string(puID), *funcProfInfoTy, kScFstatic);
    funcInfoSym->SetKonst(funcInfoMirConst);
    allFuncInfoSym.emplace_back(funcInfoSym);
  }
  return allFuncInfoSym;
};

static inline MIRSymbol *GetOrCreateFuncInfoTbl(MIRModule &m, const std::vector<MIRFunction*> &validFuncs) {
  std::string srcFN = m.GetFileName();
  std::string useName = namemangler::kprefixProfCtrTbl + FlatenName(srcFN);
  auto nameStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(useName);
  MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(nameStrIdx);
  if (sym != nullptr) {
    return sym;
  }

  FieldVector parentFields;
  FieldVector fields;
  //  Create function profile info type                                                      // Field
  GStrIdx funcNameStrIdx = m.GetMIRBuilder()->GetOrCreateStringIndex("func_name");       // 1
  GStrIdx cntNumStrIdx = m.GetMIRBuilder()->GetOrCreateStringIndex("counter_num");       // 2
  GStrIdx cntArrayStrIdx = m.GetMIRBuilder()->GetOrCreateStringIndex("counter_array");   // 3

  MIRType *voidPtrTy = GlobalTables::GetTypeTable().GetVoidPtr();
  MIRType *u64Ty = GlobalTables::GetTypeTable().GetUInt64();
  TyIdx charPtrTyIdx = GlobalTables::GetTypeTable().GetOrCreatePointerType(TyIdx(PTY_u8))->GetTypeIndex();
  fields.emplace_back(funcNameStrIdx, TyIdxFieldAttrPair(charPtrTyIdx, FieldAttrs()));
  fields.emplace_back(cntNumStrIdx, TyIdxFieldAttrPair(u64Ty->GetTypeIndex(), FieldAttrs()));
  fields.emplace_back(cntArrayStrIdx, TyIdxFieldAttrPair(voidPtrTy->GetTypeIndex(), FieldAttrs()));

  MIRType *funcProfInfoTy =
      GlobalTables::GetTypeTable().GetOrCreateStructType("__mpl_func_info_ty", fields, parentFields, m);

  std::vector<MIRSymbol *> allFuncInfoSym = CreateFuncInfo(m, funcProfInfoTy, validFuncs);

  MIRType *funcProfInfoPtrTy = GlobalTables::GetTypeTable().GetOrCreatePointerType(funcProfInfoTy->GetTypeIndex());
  MIRType *arrOfFuncInfoPtrTy = GlobalTables::GetTypeTable().GetOrCreateArrayType(
      *funcProfInfoPtrTy, validFuncs.size());
  sym = m.GetMIRBuilder()->CreateGlobalDecl(useName, *arrOfFuncInfoPtrTy, kScFstatic);
  auto *funcInfoTblMirConst = m.GetMemPool()->New<MIRAggConst>(m, *arrOfFuncInfoPtrTy);
  for (size_t i = 0; i < allFuncInfoSym.size(); ++i) {
    auto *funcInfoMirConst = m.GetMemPool()->New<MIRAddrofConst>(
        allFuncInfoSym[i]->GetStIdx(), 0, *GlobalTables::GetTypeTable().GetPtr());
    funcInfoTblMirConst->AddItem(funcInfoMirConst, i);
  }
  sym->SetKonst(funcInfoTblMirConst);
  return sym;
}

static inline MIRSymbol *GetOrCreateModuleInfo(MIRModule &m, const std::vector<MIRFunction *>&validFuncs) {
  std::string srcFN = m.GetFileName();
  std::string useName = namemangler::kprefixProfModDesc  + FlatenName(srcFN);
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
      DJBHash(m.GetFileName().c_str()), *u32Ty);
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

void CGProfGen::CreateProfFileSym(MIRModule &m, const std::string &outputPath, const std::string &symName) {
  auto *mirBuilder = m.GetMIRBuilder();
  auto *charPtrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_a64));
  const std::string finalName = outputPath + "mpl_lite_pgo.data";
  auto *modNameMirConst =m.GetMemPool()->New<MIRStrConst>(finalName, *charPtrType);
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
  auto *waitForkMirConst =m.GetMemPool()->New<MIRIntConst>(0, *u8Type);
  auto *waitForkSym = mirBuilder->GetOrCreateGlobalDecl("__mpl_pgo_wait_forks", *u8Type);
  waitForkSym->SetAttr(ATTR_weak);  // weak symbol
  waitForkSym->SetKonst(waitForkMirConst);
}

void CGProfGen::CreateProfInitExitFunc(MIRModule &m) {
  std::vector<MIRFunction *> validFuncs;
  for (MIRFunction *f : std::as_const(m.GetFunctionList())) {
    validFuncs.push_back(f);
  }
  /* call entry in mplpgo.so */
  auto profEntry = std::string("__" + FlatenName(m.GetFileName()) + "_init");
  ArgVector formals(m.GetMPAllocator().Adapter());
  MIRType *voidTy = GlobalTables::GetTypeTable().GetVoid();
  auto *newEntry  = m.GetMIRBuilder()->CreateFunction(profEntry, *voidTy, formals);
  auto *initInSo = m.GetMIRBuilder()->GetOrCreateFunction("__mpl_pgo_init", TyIdx(PTY_void));
  auto *entryBody = newEntry->GetCodeMempool()->New<BlockNode>();
  auto &entryAlloc = newEntry->GetCodeMempoolAllocator();
  auto *callMINode = newEntry->GetCodeMempool()->New<CallNode>(entryAlloc, OP_call, initInSo->GetPuidx());
  MapleVector<BaseNode*> arg(entryAlloc.Adapter());
  arg.push_back(m.GetMIRBuilder()->CreateExprAddrof(0, *GetOrCreateModuleInfo(m, validFuncs)));
  callMINode->SetOpnds(arg);
  entryBody->AddStatement(callMINode);
  newEntry->SetBody(entryBody);
  newEntry->SetAttr(FUNCATTR_initialization);
  m.AddFunction(newEntry);

  /* call setup in mplpgo.so if it needs mutex && dump in child process */
  auto profSetup = std::string("__" + FlatenName(m.GetFileName()) + "_setup");
  auto *newSetup = m.GetMIRBuilder()->CreateFunction(profSetup, *voidTy, formals);
  auto *setupInSo = m.GetMIRBuilder()->GetOrCreateFunction("__mpl_pgo_setup", TyIdx(PTY_void));
  auto *setupBody = newSetup->GetCodeMempool()->New<BlockNode>();
  auto &setupAlloc = newSetup->GetCodeMempoolAllocator();
  setupBody->AddStatement(newSetup->GetCodeMempool()->New<CallNode>(setupAlloc, OP_call, setupInSo->GetPuidx()));
  newSetup->SetBody(setupBody);
  newSetup->SetAttr(FUNCATTR_initialization);
  m.AddFunction(newSetup);

  /* call exit in mplpgo.so */
  auto profExit = std::string("__" + FlatenName(m.GetFileName()) + "_exit");
  auto *newExit = m.GetMIRBuilder()->CreateFunction(profExit, *voidTy, formals);
  auto *exitInSo = m.GetMIRBuilder()->GetOrCreateFunction("__mpl_pgo_exit", TyIdx(PTY_void));
  auto *exitBody = newExit->GetCodeMempool()->New<BlockNode>();
  auto &exitAlloc = newExit->GetCodeMempoolAllocator();
  exitBody->AddStatement(newExit->GetCodeMempool()->New<CallNode>(exitAlloc, OP_call, exitInSo->GetPuidx()));
  newExit->SetBody(exitBody);
  newExit->SetAttr(FUNCATTR_termination);
  m.AddFunction(newExit);
}

void CGProfGen::InstrumentFunction() {
  instrumenter.PrepareInstrumentInfo(f->GetFirstBB(), f->GetCommonExitBB());
  std::vector<maplebe::BB*> iBBs;
  instrumenter.GetInstrumentBBs(iBBs, f->GetFirstBB());

  /* skip large bb function currently due to offset in ldr/store */
  if (iBBs.size() > kMaxPimm8) {
    /* fixed by adding jumping label between large counter section */
    /* fixed by adding jumping label between large counter section */
    CHECK_FATAL_FALSE("due to offset in ldr/store. currently stop");
    return;
  }

  if (f->GetFunction().GetAttr(FUNCATTR_termination) || f->GetFunction().GetAttr(FUNCATTR_initialization)) {
    return;
  }

  uint32 oldTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
  MIRSymbol *bbCounterTab = GetOrCreateFuncCounter(f->GetFunction(), static_cast<uint32>(iBBs.size()));
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
  CHECK_FATAL(f.NumBBs() < LiteProfile::GetBBNoThreshold(), "stop ! bb out of range!");

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
