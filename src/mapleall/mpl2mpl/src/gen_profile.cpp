/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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
#include <ctime>
#include "gen_profile.h"
#include "ipa_phase_manager.h"
#include "namemangler.h"

// namespace maple
namespace maple {
void ProfileGen::CreateModProfDesc() {
  // Ref gcov_info
  MIRBuilder *mirBuilder = mod.GetMIRBuilder();
  MemPool *modMP = mod.GetMemPool();
  FieldVector modProfDescFields;                                                   // Field
  GStrIdx verStrIdx = mirBuilder->GetOrCreateStringIndex("version");               // 1
  GStrIdx pad4b1StrIdx = mirBuilder->GetOrCreateStringIndex("pad4b1");             // 2
  GStrIdx nextStrIdx = mirBuilder->GetOrCreateStringIndex("next");                 // 3
  GStrIdx stampStrIdx = mirBuilder->GetOrCreateStringIndex("stamp");               // 4
  GStrIdx chksumStrIdx = mirBuilder->GetOrCreateStringIndex("checksum");           // 5
  GStrIdx outputFNStrIdx = mirBuilder->GetOrCreateStringIndex("outputfile_name");  // 6
  GStrIdx mergeFuncsStrIdx = mirBuilder->GetOrCreateStringIndex("merge_funcs");    // 7
  GStrIdx nFuncsStrIdx = mirBuilder->GetOrCreateStringIndex("n_funcs");            // 8
  GStrIdx pad4b2StrIdx = mirBuilder->GetOrCreateStringIndex("pad4b2");             // 9
  GStrIdx funcDescTblStrIdx = mirBuilder->GetOrCreateStringIndex("func_desc_tbl"); // 10

  TyIdx i16TyIdx = GlobalTables::GetTypeTable().GetInt16()->GetTypeIndex();
  MIRType *u32Ty = GlobalTables::GetTypeTable().GetUInt32();
  TyIdx u32TyIdx = u32Ty->GetTypeIndex();
  TyIdx ptrTyIdx = GlobalTables::GetTypeTable().GetPtr()->GetTypeIndex();
  MIRType *voidTy = GlobalTables::GetTypeTable().GetVoid();
  MIRType *voidPtrTy = GlobalTables::GetTypeTable().GetVoidPtr();
  TyIdx charPtrTyIdx = GlobalTables::GetTypeTable().
                           GetOrCreatePointerType(TyIdx(PTY_u8))->GetTypeIndex();

  // Ref: __gcov_merge_add (gcov_type *, unsigned)
  MIRType *retTy = GlobalTables::GetTypeTable().GetVoid();
  std::vector<TyIdx> argTys;
  std::vector<TypeAttrs> argAttrs;
  MIRType *arg1Ty = GlobalTables::GetTypeTable().GetOrCreatePointerType(i16TyIdx); // need to verify on 7.5
  MIRType *arg2Ty = GlobalTables::GetTypeTable().GetUInt16();
  argTys.push_back(arg1Ty->GetTypeIndex());
  argTys.push_back(arg2Ty->GetTypeIndex());
  MIRType *funcTy = GlobalTables::GetTypeTable().GetOrCreateFunctionType(retTy->GetTypeIndex(), argTys, argAttrs);
  MIRType *funcPtrTy = GlobalTables::GetTypeTable().GetOrCreatePointerType(*funcTy, PTY_ptr);
  MIRType *arrOfPtrsTy = GlobalTables::GetTypeTable().GetOrCreateArrayType(*funcPtrTy, kMplModProfMergeFuncs);
  TyIdx arrOfPtrsTyIdx = arrOfPtrsTy->GetTypeIndex();

  /* Form descriptor type                                                                                      Field */
  modProfDescFields.emplace_back(FieldPair(verStrIdx, TyIdxFieldAttrPair(u32TyIdx, FieldAttrs())));          // 1
  modProfDescFields.emplace_back(FieldPair(pad4b1StrIdx, TyIdxFieldAttrPair(u32TyIdx, FieldAttrs())));       // 2
  modProfDescFields.emplace_back(FieldPair(nextStrIdx, TyIdxFieldAttrPair(ptrTyIdx, FieldAttrs())));         // 3
  modProfDescFields.emplace_back(FieldPair(stampStrIdx, TyIdxFieldAttrPair(u32TyIdx, FieldAttrs())));        // 4
  modProfDescFields.emplace_back(FieldPair(chksumStrIdx, TyIdxFieldAttrPair(u32TyIdx, FieldAttrs())));       // 5
  modProfDescFields.emplace_back(FieldPair(outputFNStrIdx, TyIdxFieldAttrPair(charPtrTyIdx, FieldAttrs()))); // 6
  modProfDescFields.emplace_back(
      FieldPair(mergeFuncsStrIdx, TyIdxFieldAttrPair(arrOfPtrsTyIdx, FieldAttrs())));                     // 7
  modProfDescFields.emplace_back(FieldPair(nFuncsStrIdx, TyIdxFieldAttrPair(u32TyIdx, FieldAttrs())));       // 8
  modProfDescFields.emplace_back(FieldPair(pad4b2StrIdx, TyIdxFieldAttrPair(u32TyIdx, FieldAttrs())));       // 9
  modProfDescFields.emplace_back(FieldPair(funcDescTblStrIdx, TyIdxFieldAttrPair(ptrTyIdx, FieldAttrs())));  // 10
  FieldVector parentFields;

  std::string srcFN = mod.GetFileName();
  std::string useName = FlatenName(srcFN);

  MIRType *modProfDescTy = GlobalTables::GetTypeTable().GetOrCreateStructType(
      namemangler::kprefixProfModDesc + useName + "_ty", modProfDescFields, parentFields, mod);

  MIRSymbol *modProfDescSym = mod.GetMIRBuilder()->CreateGlobalDecl(
      namemangler::kprefixProfModDesc + useName, *modProfDescTy, kScFstatic);

  // Initialization
  MIRAggConst *modProfDescSymMirConst = modMP->New<MIRAggConst>(mod, *modProfDescTy);

  MIRIntConst *verMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(kGCCVersion, *u32Ty);
  modProfDescSymMirConst->AddItem(verMirConst, 1); // checked by gcov

  MIRIntConst *zeroMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *u32Ty);
  modProfDescSymMirConst->AddItem(zeroMirConst, 2);

  MIRIntConst *nextMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *voidPtrTy);
  modProfDescSymMirConst->AddItem(nextMirConst, 3);

  uint32 timeStamp = time(nullptr);
  MIRIntConst *stampMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(timeStamp, *u32Ty);
  modProfDescSymMirConst->AddItem(stampMirConst, 4);

  uint32 cfgChkSum = 0;
  for (MIRFunction *f : validFuncs) {
    cfgChkSum ^= static_cast<uint32>((f->GetCFGChksum() >> 32) ^ (f->GetCFGChksum() & 0xffffffff));
  }
  MIRIntConst *checksumMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(cfgChkSum, *u32Ty);
  modProfDescSymMirConst->AddItem(checksumMirConst, 5);

  // Make the profile file name as fileName.gcda
  std::string profFileName = mod.GetProfileDataFileName();
  auto *profileFNMirConst =
      modMP->New<MIRStrConst>("./" + profFileName, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_a64)));
  modProfDescSymMirConst->AddItem(profileFNMirConst, 6);

  // Additional profiling (in addition to frequency profiling) should be added here
  MIRFunction *profFuncProtoTy =
      mod.GetMIRBuilder()->GetOrCreateFunction(namemangler::kMplMergeFuncAdd, voidTy->GetTypeIndex());
  profFuncProtoTy->AllocSymTab();
  MIRSymbol *profFuncArg1Sym = profFuncProtoTy->GetSymTab()->CreateSymbol(kScopeLocal);
  profFuncArg1Sym->SetTyIdx(arg1Ty->GetTypeIndex());
  profFuncProtoTy->AddArgument(profFuncArg1Sym);
  MIRSymbol *profFuncArg2Sym = profFuncProtoTy->GetSymTab()->CreateSymbol(kScopeLocal);
  profFuncArg2Sym->SetTyIdx(arg2Ty->GetTypeIndex());
  profFuncProtoTy->AddArgument(profFuncArg2Sym);
  MIRAddroffuncConst *profEdgeCountMirConst =
      modMP->New<MIRAddroffuncConst>(profFuncProtoTy->GetPuidx(), *funcPtrTy);
  MIRAggConst *mergeFuncsMirConst = modMP->New<MIRAggConst>(mod, *arrOfPtrsTy);

  mergeFuncsMirConst->AddItem(profEdgeCountMirConst, 0);
  mergeFuncsMirConst->AddItem(nextMirConst, 1);
  mergeFuncsMirConst->AddItem(nextMirConst, 2);
  mergeFuncsMirConst->AddItem(nextMirConst, 3);
  mergeFuncsMirConst->AddItem(nextMirConst, 4);
  mergeFuncsMirConst->AddItem(nextMirConst, 5);
  mergeFuncsMirConst->AddItem(nextMirConst, 6);
  mergeFuncsMirConst->AddItem(nextMirConst, 7);
  mergeFuncsMirConst->AddItem(nextMirConst, 8);

  modProfDescSymMirConst->AddItem(mergeFuncsMirConst, 7);

  uint32 numOfFunc = validFuncs.size();
  MIRIntConst *nfuncsMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(numOfFunc, *u32Ty);
  modProfDescSymMirConst->AddItem(nfuncsMirConst, 8);

  modProfDescSymMirConst->AddItem(zeroMirConst, 9);

  // func_desc_tbl will be fixed up later in fixup
  MIRAddrofConst *addrofConst = modMP->New<MIRAddrofConst>(
      modProfDescSym->GetStIdx(), 0, *GlobalTables::GetTypeTable().GetPtr());
  modProfDescSymMirConst->AddItem(addrofConst, 10);

  modProfDescSym->SetKonst(modProfDescSymMirConst);
  modProfDesc = modProfDescSym;
}

void ProfileGen::CreateFuncProfDesc() {
  // Ref: gcov_ctr_info
  //      gcov_fn_info
  MIRBuilder *mirBuilder = mod.GetMIRBuilder();
  MemPool *modMP = mod.GetMemPool();
  FieldVector parentFields;
  MIRType *u32Ty = GlobalTables::GetTypeTable().GetUInt32();
  MIRType *i64Ty = GlobalTables::GetTypeTable().GetInt64();
  MIRType *i64PtrTy = GlobalTables::GetTypeTable().GetOrCreatePointerType(i64Ty->GetTypeIndex());

  // Create counter info type                                            // Field
  GStrIdx nCtrStrIdx = mirBuilder->GetOrCreateStringIndex("num_ctr");    // 1
  GStrIdx ctrTblStrIdx = mirBuilder->GetOrCreateStringIndex("ctr_tbl");  // 2

  FieldVector ctrInfoFields;
  ctrInfoFields.emplace_back(FieldPair(nCtrStrIdx, TyIdxFieldAttrPair(u32Ty->GetTypeIndex(), FieldAttrs())));      // 1
  ctrInfoFields.emplace_back(FieldPair(ctrTblStrIdx, TyIdxFieldAttrPair(i64PtrTy->GetTypeIndex(), FieldAttrs()))); // 2
  MIRType *ctrDescTy = GlobalTables::GetTypeTable().GetOrCreateStructType(
      "__mpl_ctr_desc_ty", ctrInfoFields, parentFields, mod);

  // Create function profile descriptor type                                 // Field
  GStrIdx modDescStrIdx = mirBuilder->GetOrCreateStringIndex("mod_desc");    // 1
  GStrIdx funcIDStrIdx = mirBuilder->GetOrCreateStringIndex("func_id");      // 2
  GStrIdx lnChkStrIdx = mirBuilder->GetOrCreateStringIndex("lineno_chksum"); // 3
  GStrIdx cfgChkStrIdx = mirBuilder->GetOrCreateStringIndex("cfg_chksum");   // 4
  GStrIdx pad4bStrIdx = mirBuilder->GetOrCreateStringIndex("pad4b");         // 5
  GStrIdx ctrDescStrIdx = mirBuilder->GetOrCreateStringIndex("ctr_desc");    // 6

  MIRType *modProfDescPtrTy = GlobalTables::GetTypeTable().GetOrCreatePointerType(modProfDesc->GetTyIdx());
  MIRType *arrOfCtrDescTy = GlobalTables::GetTypeTable().GetOrCreateArrayType(*ctrDescTy, kMplFuncProfCtrInfoNum);

  FieldVector funcProfDescFields;                                                                             // Field
  funcProfDescFields.emplace_back(
      FieldPair(modDescStrIdx, TyIdxFieldAttrPair(modProfDescPtrTy->GetTypeIndex(), FieldAttrs())));    // 1
  funcProfDescFields.emplace_back(FieldPair(funcIDStrIdx,
                                            TyIdxFieldAttrPair(u32Ty->GetTypeIndex(), FieldAttrs())));  // 2
  funcProfDescFields.emplace_back(FieldPair(lnChkStrIdx,
                                            TyIdxFieldAttrPair(u32Ty->GetTypeIndex(), FieldAttrs())));  // 3
  funcProfDescFields.emplace_back(FieldPair(cfgChkStrIdx,
                                            TyIdxFieldAttrPair(u32Ty->GetTypeIndex(), FieldAttrs())));  // 4
  funcProfDescFields.emplace_back(FieldPair(pad4bStrIdx,
                                            TyIdxFieldAttrPair(u32Ty->GetTypeIndex(), FieldAttrs())));  // 5
  funcProfDescFields.emplace_back(
      FieldPair(ctrDescStrIdx, TyIdxFieldAttrPair(arrOfCtrDescTy->GetTypeIndex(), FieldAttrs())));      // 6

  MIRType *funcProfDescTy =
      GlobalTables::GetTypeTable().GetOrCreateStructType("__mpl_prof_desc_ty", funcProfDescFields, parentFields, mod);

  for (MIRFunction *f : validFuncs) {
    if (f->GetBody() == nullptr) {
      continue;
    }

    uint64  nCtrs = f->GetNumCtrs();
    MIRSymbol *ctrTblSym = f->GetProfCtrTbl();

    // Initialization of counter table
    MIRType *u64Ty = GlobalTables::GetTypeTable().GetUInt64();
    MIRIntConst *zeroMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *u64Ty);
    MIRType *arrOfUInt64Ty = GlobalTables::GetTypeTable().GetOrCreateArrayType(*u64Ty, nCtrs);
    MIRAggConst *initCtrTblMirConst = modMP->New<MIRAggConst>(mod, *arrOfUInt64Ty);

    if (ctrTblSym && nCtrs > 0) {
      for (uint32 i = 0; i < nCtrs; ++i) {
        initCtrTblMirConst->AddItem(zeroMirConst, i);
      }
      ctrTblSym->SetKonst(initCtrTblMirConst);
    }

    PUIdx puID = f->GetPuidx();

    // Create const func profile descriptor
    MIRAggConst *funcProfDescMirConst = modMP->New<MIRAggConst>(mod, *funcProfDescTy);

    MIRAddrofConst *modDescMirConst = modMP->New<MIRAddrofConst>(modProfDesc->GetStIdx(), 0,
                                                                 *GlobalTables::GetTypeTable().GetPtr());
    funcProfDescMirConst->AddItem(modDescMirConst, 1);

    uint32 funcID = puID;
    MIRIntConst *funcIDMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(funcID, *u32Ty);
    funcProfDescMirConst->AddItem(funcIDMirConst, 2);

    uint32 lineNoChkSum = static_cast<uint32>((
        f->GetFileLineNoChksum() >> 32) ^ (f->GetFileLineNoChksum() & 0xffffffff));
    MIRIntConst *lineNoChkSumMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(lineNoChkSum, *u32Ty);
    funcProfDescMirConst->AddItem(lineNoChkSumMirConst, 3);

    uint32 cfgChkSum = static_cast<uint32>((f->GetCFGChksum() >> 32) ^ (f->GetCFGChksum() & 0xffffffff));
    MIRIntConst *cfgChkSumMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(cfgChkSum, *u32Ty);
    funcProfDescMirConst->AddItem(cfgChkSumMirConst, 4);

    uint32 pad4b = 0;
    MIRIntConst *pad4bMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(pad4b, *u32Ty);
    funcProfDescMirConst->AddItem(pad4bMirConst, 5);

    MIRAggConst *ctrDescMirConst = modMP->New<MIRAggConst>(mod, *ctrDescTy);

    MIRIntConst *nCtrMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(nCtrs, *u32Ty);
    ctrDescMirConst->AddItem(nCtrMirConst, 1);

    MIRAddrofConst *ctrTblMirConst;
    if (ctrTblSym) {
      ctrTblMirConst = modMP->New<MIRAddrofConst>(ctrTblSym->GetStIdx(), 0, *GlobalTables::GetTypeTable().GetPtr());
      ctrDescMirConst->AddItem(ctrTblMirConst, 2);
    } else {
      MIRType *voidPtrTy = GlobalTables::GetTypeTable().GetVoidPtr();
      MIRIntConst *nullPtrMirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *voidPtrTy);
      ctrDescMirConst->AddItem(nullPtrMirConst, 2);
    }

    MIRType *mirType =
        GlobalTables::GetTypeTable().GetOrCreateArrayType(*ctrDescTy, kMplFuncProfCtrInfoNum);
    MIRAggConst *arrOfCtrDescMirConst = modMP->New<MIRAggConst>(mod, *mirType);

    // if kMplFuncProfCtrInfoNum > 1, more counter descriptors need to add
    arrOfCtrDescMirConst->AddItem(ctrDescMirConst, 0);
    funcProfDescMirConst->AddItem(arrOfCtrDescMirConst, 6);

    MIRSymbol *funcProfDescSym = mod.GetMIRBuilder()->CreateGlobalDecl(
        namemangler::kprefixProfFuncDesc + FlatenName(mod.GetFileName() + "_" + f->GetName()),
        *funcProfDescTy, kScFstatic);
    funcProfDescSym->SetKonst(funcProfDescMirConst);
    funcProfDescs.push_back(funcProfDescSym);
  }
}

void ProfileGen::CreateFuncProfDescTbl() {
  uint tblSize = validFuncs.size();
  if (tblSize == 0) {
    funcProfDescTbl = nullptr;
    return;
  }

  // Create function descriptor table
  MIRType *funcProfDescPtrTy = GlobalTables::GetTypeTable().GetOrCreatePointerType(funcProfDescs[0]->GetTyIdx());
  MIRType *arrOffuncProfDescPtrTy = GlobalTables::GetTypeTable().GetOrCreateArrayType(*funcProfDescPtrTy, tblSize);
  std::string fileName = mod.GetFileName();
  MIRSymbol *funcProfDescTblSym = mod.GetMIRBuilder()->CreateGlobalDecl(
      namemangler::kprefixProfFuncDescTbl + FlatenName(fileName), *arrOffuncProfDescPtrTy, kScFstatic);
  MIRAggConst *funcDescTblMirConst = mod.GetMemPool()->New<MIRAggConst>(mod, *arrOffuncProfDescPtrTy);

  for (size_t i = 0; i < tblSize; ++i) {
    MIRAddrofConst *funcProfDescMirConst = mod.GetMemPool()->New<MIRAddrofConst>(
        funcProfDescs[i]->GetStIdx(), 0, *GlobalTables::GetTypeTable().GetPtr());
    funcDescTblMirConst->AddItem(funcProfDescMirConst, i);
  }

  funcProfDescTblSym->SetKonst(funcDescTblMirConst);
  funcProfDescTbl = funcProfDescTblSym;
}

void ProfileGen::FixupDesc() {
  if (funcProfDescTbl == nullptr) {
    return;
  }

  MIRAggConst *modProfDescMirConst = static_cast<MIRAggConst *>(modProfDesc->GetKonst());
  MIRAddrofConst *funcProfDescTblAddr = mod.GetMemPool()->New<MIRAddrofConst>(
      funcProfDescTbl->GetStIdx(), 0, *GlobalTables::GetTypeTable().GetPtr());
  modProfDescMirConst->SetItem(9, funcProfDescTblAddr, 10);
}

// Ref: void gcov_init (struct gcov_info *info);
void ProfileGen::CreateInitProc() {
  MIRBuilder *mirBuilder = mod.GetMIRBuilder();
  MIRFunction *origFunc = mirBuilder->GetCurrentFunction();

  MIRType *voidTy = GlobalTables::GetTypeTable().GetVoid();
  ArgVector formals(mod.GetMPAllocator().Adapter());
  MIRFunction *mplProfInit = mirBuilder->CreateFunction(
      namemangler::kprefixProfInit + FlatenName(mod.GetFileName()), *voidTy, formals);
  mplProfInit->SetPuidxOrigin(mplProfInit->GetPuidx());
  mplProfInit->SetAttr(FUNCATTR_initialization);
  mirBuilder->SetCurrentFunction(*mplProfInit);

  MIRFunction *gccProfInitProtoTy = mirBuilder->GetOrCreateFunction(
      namemangler::kGCCProfInit, voidTy->GetTypeIndex());
  gccProfInitProtoTy->AllocSymTab();
  MIRSymbol *gccProfInitArgSym = gccProfInitProtoTy->GetSymTab()->CreateSymbol(kScopeLocal);
  MIRType *argPtrTy = GlobalTables::GetTypeTable().GetOrCreatePointerType(modProfDesc->GetTyIdx());
  gccProfInitArgSym->SetTyIdx(argPtrTy->GetTypeIndex());
  gccProfInitProtoTy->AddArgument(gccProfInitArgSym);

  MapleVector<BaseNode*> ActArg(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
  AddrofNode *addrModProfDesc = mirBuilder->CreateExprAddrof(0, *modProfDesc);
  ActArg.push_back(addrModProfDesc);
  CallNode *callGInit = mirBuilder->CreateStmtCall(gccProfInitProtoTy->GetPuidx(), ActArg);

  BlockNode *block = mplProfInit->GetCodeMemPool()->New<BlockNode>();
  block->AddStatement(callGInit);
  mplProfInit->SetBody(block);
  mod.AddFunction(mplProfInit);

  mirBuilder->SetCurrentFunction(*origFunc);
}

// Ref: void gcov_exit ();
void ProfileGen::CreateExitProc() {
  MIRBuilder *mirBuilder = mod.GetMIRBuilder();
  MIRFunction *origFunc = mirBuilder->GetCurrentFunction();
  MIRType *voidTy = GlobalTables::GetTypeTable().GetVoid();

  ArgVector formals(mod.GetMPAllocator().Adapter());
  MIRFunction *mplProfExit = mirBuilder->CreateFunction(
      namemangler::kprefixProfExit + FlatenName(mod.GetFileName()), *voidTy, formals);
  mplProfExit->SetPuidxOrigin(mplProfExit->GetPuidx());
  mplProfExit->SetAttr(FUNCATTR_termination);
  mirBuilder->SetCurrentFunction(*mplProfExit);

  MIRFunction *gccProfExitProtoTy = mirBuilder->GetOrCreateFunction(namemangler::kGCCProfExit, voidTy->GetTypeIndex());
  gccProfExitProtoTy->AllocSymTab();

  MapleVector<BaseNode*> ActArg(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
  CallNode *callGExit = mirBuilder->CreateStmtCall(gccProfExitProtoTy->GetPuidx(), ActArg);

  BlockNode *block = mplProfExit->GetCodeMemPool()->New<BlockNode>();
  block->AddStatement(callGExit);
  mplProfExit->SetBody(block);
  mod.AddFunction(mplProfExit);

  mirBuilder->SetCurrentFunction(*origFunc);
}

void ProfileGen::Run() {
  // Note: all internal symbols are named as prefix+mplfilename (+ funcname)

  // Create module profile descriptor (.LPBX0)
  CreateModProfDesc();

  // Create function profile descriptor including counter table
  CreateFuncProfDesc();

  // Create function profile descriptor table (.LPBX1)
  CreateFuncProfDescTbl();

  // Fixup symbol references among descriptors
  FixupDesc();

  // Create module profile init proc (_sub_I_00100_0)
  CreateInitProc();

  // Create module profiling exit proc (_sub_D_00100_1)
  CreateExitProc();
}

bool ProfileGenPM::PhaseRun(MIRModule &mod) {
  ProfileGen profileGen(mod);
  profileGen.Run();
  return false;
}

void ProfileGenPM::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  // Note: IpaSccPM calls MeProfGen to finish instrumentation based on MST
  aDep.AddRequired<IpaSccPM>();
  aDep.AddPreserved<IpaSccPM>();
}
}
