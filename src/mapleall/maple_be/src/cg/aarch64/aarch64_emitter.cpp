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
#include "aarch64_emitter.h"
#include <sys/stat.h>
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"
#include "metadata_layout.h"
#include "cfi.h"
#include "dbg.h"

namespace {
using namespace maple;
const std::unordered_set<std::string> kJniNativeFuncList = {
  "Landroid_2Fos_2FParcel_3B_7CnativeWriteString_7C_28JLjava_2Flang_2FString_3B_29V_native",
  "Landroid_2Fos_2FParcel_3B_7CnativeReadString_7C_28J_29Ljava_2Flang_2FString_3B_native",
  "Landroid_2Fos_2FParcel_3B_7CnativeWriteInt_7C_28JI_29V_native",
  "Landroid_2Fos_2FParcel_3B_7CnativeReadInt_7C_28J_29I_native",
  "Landroid_2Fos_2FParcel_3B_7CnativeWriteInterfaceToken_7C_28JLjava_2Flang_2FString_3B_29V_native",
  "Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native"
};
constexpr uint32 kBinSearchInsnCount = 56;
// map func name to <filename, insnCount> pair
using Func2CodeInsnMap = std::unordered_map<std::string, std::pair<std::string, uint32>>;
Func2CodeInsnMap func2CodeInsnMap {
    { "Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I",
      { "maple/mrt/codetricks/arch/arm64/hashCode.s", 29 } },
    { "Ljava_2Flang_2FString_3B_7Cequals_7C_28Ljava_2Flang_2FObject_3B_29Z",
      { "maple/mrt/codetricks/arch/arm64/stringEquals.s", 50 } }
};
constexpr uint32 kQuadInsnCount = 2;

void GetMethodLabel(const std::string &methodName, std::string &methodLabel) {
  methodLabel = ".Lmethod_desc." + methodName;
}
}

namespace maplebe {
using namespace maple;

void AArch64AsmEmitter::EmitRefToMethodDesc(FuncEmitInfo &funcEmitInfo, Emitter &emitter) {
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  if (!cgFunc.GetFunction().IsJava()) {
    return;
  }
  std::string methodDescLabel;
  GetMethodLabel(cgFunc.GetFunction().GetName(), methodDescLabel);
  (void)emitter.Emit("\t.word " + methodDescLabel + "-.\n");
  emitter.IncreaseJavaInsnCount();
}

void AArch64AsmEmitter::EmitRefToMethodInfo(FuncEmitInfo &funcEmitInfo, Emitter &emitter) {
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  if (cgFunc.GetFunction().GetModule()->IsJavaModule()) {
    std::string labelName = ".Label.name." + cgFunc.GetFunction().GetName();
    (void)emitter.Emit("\t.word " + labelName + " - .\n");
  }
}

/*
 * emit java method description which contains address and size of local reference area
 * as well as method metadata.
 */
void AArch64AsmEmitter::EmitMethodDesc(FuncEmitInfo &funcEmitInfo, Emitter &emitter) {
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  if (!cgFunc.GetFunction().IsJava()) {
    return;
  }
  (void)emitter.Emit("\t.section\t.rodata\n");
  (void)emitter.Emit("\t.align\t2\n");
  std::string methodInfoLabel;
  GetMethodLabel(cgFunc.GetFunction().GetName(), methodInfoLabel);
  (void)emitter.Emit(methodInfoLabel + ":\n");
  EmitRefToMethodInfo(funcEmitInfo, emitter);
  /* local reference area */
  AArch64MemLayout *memLayout = static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout());
  int32 refOffset = memLayout->GetRefLocBaseLoc();
  uint32 refNum = memLayout->GetSizeOfRefLocals() / kOffsetAlign;
  /* for ea usage */
  AArch64CGFunc &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  IntrinsiccallNode *cleanEANode = aarchCGFunc.GetCleanEANode();
  if (cleanEANode != nullptr) {
    refNum += static_cast<uint32>(cleanEANode->NumOpnds());
    refOffset -= static_cast<int32>(cleanEANode->NumOpnds() * kIntregBytelen);
  }
  (void)emitter.Emit("\t.short ").Emit(refOffset).Emit("\n");
  (void)emitter.Emit("\t.short ").Emit(refNum).Emit("\n");
}

/* the fast_exception_handling lsda */
void AArch64AsmEmitter::EmitFastLSDA(FuncEmitInfo &funcEmitInfo) {
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  AArch64CGFunc &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();

  Emitter *emitter = currCG->GetEmitter();
  PUIdx pIdx = currCG->GetMIRModule()->CurFunction()->GetPuidx();
  const std::string &idx = strdup(std::to_string(pIdx).c_str());
  /*
   * .word 0xFFFFFFFF
   * .word .Label.LTest_3B_7C_3Cinit_3E_7C_28_29V3-func_start_label
   */
  (void)emitter->Emit("\t.word 0xFFFFFFFF\n");
  (void)emitter->Emit("\t.word .L." + idx + "__");
  if (aarchCGFunc.NeedCleanup()) {
    emitter->Emit(cgFunc.GetCleanupLabel()->GetLabelIdx());
  } else {
    ASSERT(!cgFunc.GetExitBBsVec().empty(), "exitbbsvec is empty in AArch64AsmEmitter::EmitFastLSDA");
    emitter->Emit(cgFunc.GetExitBB(0)->GetLabIdx());
  }
  emitter->Emit("-.L." + idx + "__")
      .Emit(cgFunc.GetStartLabel()->GetLabelIdx())
      .Emit("\n");
  emitter->IncreaseJavaInsnCount();
}

/* the normal gcc_except_table */
void AArch64AsmEmitter::EmitFullLSDA(FuncEmitInfo &funcEmitInfo) {
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  AArch64CGFunc &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  EHFunc *ehFunc = cgFunc.GetEHFunc();
  Emitter *emitter = currCG->GetEmitter();
  /* emit header */
  emitter->Emit("\t.align 3\n");
  emitter->Emit("\t.section .gcc_except_table,\"a\",@progbits\n");
  emitter->Emit("\t.align 3\n");
  /* emit LSDA header */
  LSDAHeader *lsdaHeader = ehFunc->GetLSDAHeader();
  emitter->EmitStmtLabel(lsdaHeader->GetLSDALabel()->GetLabelIdx());
  emitter->Emit("\t.byte ").Emit(lsdaHeader->GetLPStartEncoding()).Emit("\n");
  emitter->Emit("\t.byte ").Emit(lsdaHeader->GetTTypeEncoding()).Emit("\n");
  emitter->Emit("\t.uleb128 ");
  emitter->EmitLabelPair(lsdaHeader->GetTTypeOffset());
  emitter->EmitStmtLabel(lsdaHeader->GetTTypeOffset().GetStartOffset()->GetLabelIdx());
  /* emit call site table */
  emitter->Emit("\t.byte ").Emit(lsdaHeader->GetCallSiteEncoding()).Emit("\n");
  /* callsite table size */
  emitter->Emit("\t.uleb128 ");
  emitter->EmitLabelPair(ehFunc->GetLSDACallSiteTable()->GetCSTable());
  /* callsite start */
  emitter->EmitStmtLabel(ehFunc->GetLSDACallSiteTable()->GetCSTable().GetStartOffset()->GetLabelIdx());
  ehFunc->GetLSDACallSiteTable()->SortCallSiteTable([&aarchCGFunc](const LSDACallSite *a, const LSDACallSite *b) {
    CHECK_FATAL(a != nullptr, "nullptr check");
    CHECK_FATAL(b != nullptr, "nullptr check");
    LabelIDOrder id1 = aarchCGFunc.GetLabelOperand(a->csStart.GetEndOffset()->GetLabelIdx())->GetLabelOrder();
    LabelIDOrder id2 = aarchCGFunc.GetLabelOperand(b->csStart.GetEndOffset()->GetLabelIdx())->GetLabelOrder();
    /* id1 and id2 should not be default value -1u */
    CHECK_FATAL(id1 != 0xFFFFFFFF, "illegal label order assigned");
    CHECK_FATAL(id2 != 0xFFFFFFFF, "illegal label order assigned");
    return id1 < id2;
  });
  const MapleVector<LSDACallSite*> &callSiteTable = ehFunc->GetLSDACallSiteTable()->GetCallSiteTable();
  for (size_t i = 0; i < callSiteTable.size(); ++i) {
    LSDACallSite *lsdaCallSite = callSiteTable[i];
    emitter->Emit("\t.uleb128 ");
    emitter->EmitLabelPair(lsdaCallSite->csStart);

    emitter->Emit("\t.uleb128 ");
    emitter->EmitLabelPair(lsdaCallSite->csLength);

    if (lsdaCallSite->csLandingPad.GetStartOffset()) {
      emitter->Emit("\t.uleb128 ");
      emitter->EmitLabelPair(lsdaCallSite->csLandingPad);
    } else {
      ASSERT(lsdaCallSite->csAction == 0, "csAction error!");
      emitter->Emit("\t.uleb128 ");
      if (aarchCGFunc.NeedCleanup()) {
        /* if landing pad is 0, we emit this call site as cleanup code */
        LabelPair cleaupCode;
        cleaupCode.SetStartOffset(cgFunc.GetStartLabel());
        cleaupCode.SetEndOffset(cgFunc.GetCleanupLabel());
        emitter->EmitLabelPair(cleaupCode);
      } else if (cgFunc.GetFunction().IsJava()) {
        ASSERT(!cgFunc.GetExitBBsVec().empty(), "exitbbsvec is empty in AArch64Emitter::EmitFullLSDA");
        PUIdx pIdx = cgFunc.GetMirModule().CurFunction()->GetPuidx();
        const std::string &idx = strdup(std::to_string(pIdx).c_str());
        (void)emitter->Emit(".L." + idx).Emit("__").Emit(cgFunc.GetExitBB(0)->GetLabIdx());
        (void)emitter->Emit(" - .L." + idx).Emit("__").Emit(cgFunc.GetStartLabel()->GetLabelIdx()).Emit("\n");
      } else {
        emitter->Emit("0\n");
      }
    }
    emitter->Emit("\t.uleb128 ").Emit(lsdaCallSite->csAction).Emit("\n");
  }

  /*
   * quick hack: insert a call site entry for the whole function body.
   * this will hand in any pending (uncaught) exception to its caller. Note that
   * __gxx_personality_v0 in libstdc++ is coded so that if exception table exists,
   * the call site table must have an entry for any possibly raised exception,
   * otherwise __cxa_call_terminate will be invoked immediately, thus the caller
   * does not get the chance to take charge.
   */
  if (aarchCGFunc.NeedCleanup() || cgFunc.GetFunction().IsJava()) {
    /* call site for clean-up */
    LabelPair funcStart;
    funcStart.SetStartOffset(cgFunc.GetStartLabel());
    funcStart.SetEndOffset(cgFunc.GetStartLabel());
    emitter->Emit("\t.uleb128 ");
    emitter->EmitLabelPair(funcStart);
    LabelPair funcLength;
    funcLength.SetStartOffset(cgFunc.GetStartLabel());
    funcLength.SetEndOffset(cgFunc.GetCleanupLabel());
    emitter->Emit("\t.uleb128 ");
    emitter->EmitLabelPair(funcLength);
    LabelPair cleaupCode;
    cleaupCode.SetStartOffset(cgFunc.GetStartLabel());
    cleaupCode.SetEndOffset(cgFunc.GetCleanupLabel());
    emitter->Emit("\t.uleb128 ");
    if (aarchCGFunc.NeedCleanup()) {
      emitter->EmitLabelPair(cleaupCode);
    } else {
      ASSERT(!cgFunc.GetExitBBsVec().empty(), "exitbbsvec is empty in AArch64AsmEmitter::EmitFullLSDA");
      PUIdx pIdx = cgFunc.GetMirModule().CurFunction()->GetPuidx();
      const std::string &idx = strdup(std::to_string(pIdx).c_str());
      (void)emitter->Emit(".L." + idx).Emit("__").Emit(cgFunc.GetExitBB(0)->GetLabIdx());
      (void)emitter->Emit(" - .L." + idx).Emit("__").Emit(cgFunc.GetStartLabel()->GetLabelIdx()).Emit("\n");
    }
    emitter->Emit("\t.uleb128 0\n");
    if (!cgFunc.GetFunction().IsJava()) {
      /* call site for stack unwind */
      LabelPair unwindStart;
      unwindStart.SetStartOffset(cgFunc.GetStartLabel());
      unwindStart.SetEndOffset(cgFunc.GetCleanupLabel());
      emitter->Emit("\t.uleb128 ");
      emitter->EmitLabelPair(unwindStart);
      LabelPair unwindLength;
      unwindLength.SetStartOffset(cgFunc.GetCleanupLabel());
      unwindLength.SetEndOffset(cgFunc.GetEndLabel());
      emitter->Emit("\t.uleb128 ");
      emitter->EmitLabelPair(unwindLength);
      emitter->Emit("\t.uleb128 0\n");
      emitter->Emit("\t.uleb128 0\n");
    }
  }
  /* callsite end label */
  emitter->EmitStmtLabel(ehFunc->GetLSDACallSiteTable()->GetCSTable().GetEndOffset()->GetLabelIdx());
  /* tt */
  const LSDAActionTable *lsdaActionTable = ehFunc->GetLSDAActionTable();
  for (size_t i = 0; i < lsdaActionTable->Size(); ++i) {
    LSDAAction *lsdaAction = lsdaActionTable->GetActionTable().at(i);
    emitter->Emit("\t.byte ").Emit(lsdaAction->GetActionIndex()).Emit("\n");
    emitter->Emit("\t.byte ").Emit(lsdaAction->GetActionFilter()).Emit("\n");
  }
  emitter->Emit("\t.align 3\n");
  for (int32 i = ehFunc->GetEHTyTableSize() - 1; i >= 0; i--) {
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ehFunc->GetEHTyTableMember(i));
    MIRTypeKind typeKind = mirType->GetKind();
    if (((typeKind == kTypeScalar) && (mirType->GetPrimType() == PTY_void)) || (typeKind == kTypeStructIncomplete) ||
        (typeKind == kTypeInterfaceIncomplete)) {
      continue;
    }
    CHECK_FATAL((typeKind == kTypeClass) || (typeKind == kTypeClassIncomplete), "NYI");
    const std::string &tyName = GlobalTables::GetStrTable().GetStringFromStrIdx(mirType->GetNameStrIdx());
    std::string dwRefString(".LDW.ref.");
    dwRefString += CLASSINFO_PREFIX_STR;
    dwRefString += tyName;
    dwRefString += " - .";
    emitter->Emit("\t.4byte " + dwRefString + "\n");
  }
  /* end of lsda */
  emitter->EmitStmtLabel(lsdaHeader->GetTTypeOffset().GetEndOffset()->GetLabelIdx());
}

void AArch64AsmEmitter::EmitBBHeaderLabel(FuncEmitInfo &funcEmitInfo, const std::string &name, LabelIdx labIdx) {
  (void)name;
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  AArch64CGFunc &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  Emitter &emitter = *(currCG->GetEmitter());
  LabelOperand &label = aarchCGFunc.GetOrCreateLabelOperand(labIdx);
  /* if label order is default value -1, set new order */
  if (label.GetLabelOrder() == 0xFFFFFFFF) {
    label.SetLabelOrder(currCG->GetLabelOrderCnt());
    currCG->IncreaseLabelOrderCnt();
  }
  PUIdx pIdx = currCG->GetMIRModule()->CurFunction()->GetPuidx();
  char *puIdx = strdup(std::to_string(pIdx).c_str());
  const std::string &labelName = cgFunc.GetFunction().GetLabelTab()->GetName(labIdx);
  if (currCG->GenerateVerboseCG()) {
    (void)emitter.Emit(".L.").Emit(puIdx).Emit("__").Emit(labIdx).Emit(":\t//label order ").Emit(label.GetLabelOrder());
    if (!labelName.empty() && labelName.at(0) != '@') {
      /* If label name has @ as its first char, it is not from MIR */
      (void)emitter.Emit(", MIR: @").Emit(labelName).Emit("\n");
    } else {
      (void)emitter.Emit("\n");
    }
  } else {
    (void)emitter.Emit(".L.").Emit(puIdx).Emit("__").Emit(labIdx).Emit(":\n");
  }
  free(puIdx);
  puIdx = nullptr;
}

void AArch64AsmEmitter::EmitJavaInsnAddr(FuncEmitInfo &funcEmitInfo) {
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  if (cgFunc.GetFunction().IsJava()) {
    Emitter *emitter = cgFunc.GetCG()->GetEmitter();
    /* emit a comment of current address from the begining of java text section */
    std::stringstream ss;
    ss << "\n\t// addr: 0x" << std::hex << (emitter->GetJavaInsnCount() * kInsnSize) << "\n";
    cgFunc.GetCG()->GetEmitter()->Emit(ss.str());
  }
}

void AArch64AsmEmitter::RecordRegInfo(FuncEmitInfo &funcEmitInfo) {
  if (!CGOptions::DoIPARA() || funcEmitInfo.GetCGFunc().GetFunction().IsJava()) {
    return;
  }
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  AArch64CGFunc &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);

  std::set<regno_t> referedRegs;
  MIRFunction &mirFunc = cgFunc.GetFunction();
  FOR_ALL_BB_REV(bb, &aarchCGFunc) {
    FOR_BB_INSNS_REV(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->IsCall() || insn->IsTailCall()) {
        auto *targetOpnd = insn->GetCallTargetOperand();
        bool safeCheck = false;
        CHECK_FATAL(targetOpnd != nullptr, "target is null in AArch64Insn::IsCallToFunctionThatNeverReturns");
        if (targetOpnd->IsFuncNameOpnd()) {
          FuncNameOperand *target = static_cast<FuncNameOperand*>(targetOpnd);
          const MIRSymbol *funcSt = target->GetFunctionSymbol();
          ASSERT(funcSt->GetSKind() == kStFunc, "funcst must be a function name symbol");
          MIRFunction *func = funcSt->GetFunction();
          if (func != nullptr && func->IsReferedRegsValid()) {
            safeCheck = true;
            for (auto preg : func->GetReferedRegs()) {
              referedRegs.insert(preg);
            }
          }
        }
        if (!safeCheck){
          mirFunc.SetReferedRegsValid(false);
          return;
        }
      }
      if (referedRegs.size() == kMaxRegNum) {
        break;
      }
      uint32 opndNum = insn->GetOperandSize();
      const AArch64MD *md = &AArch64CG::kMd[insn->GetMachineOpcode()];
      for (uint32 i = 0; i < opndNum; ++i) {
        if (insn->GetMachineOpcode() == MOP_asm) {
          if (i == kAsmOutputListOpnd || i == kAsmClobberListOpnd) {
            for (auto opnd : static_cast<ListOperand &>(insn->GetOperand(i)).GetOperands()) {
              if (opnd->IsRegister()) {
                referedRegs.insert(static_cast<RegOperand *>(opnd)->GetRegisterNumber());
              }
            }
          }
          continue;
        }
        Operand &opnd = insn->GetOperand(i);
        if (opnd.IsList()) {
          /* all use, skip it */
        } else if (opnd.IsMemoryAccessOperand()) {
          auto &memOpnd = static_cast<MemOperand&>(opnd);
          RegOperand *base = memOpnd.GetBaseRegister();
          if (!memOpnd.IsIntactIndexed()) {
            referedRegs.insert(base->GetRegisterNumber());
          }
        } else if (opnd.IsRegister()) {
          RegType regType = static_cast<RegOperand&>(opnd).GetRegisterType();
          if (regType == kRegTyCc || regType == kRegTyVary) {
            continue;
          }
          bool isDef = md->GetOperand(static_cast<int>(i))->IsRegDef();
          if (isDef) {
            referedRegs.insert(static_cast<RegOperand&>(opnd).GetRegisterNumber());
          }
        }
      }
    }
  }
  mirFunc.SetReferedRegsValid(true);
#ifdef DEBUG
  for (auto reg : referedRegs) {
    if (reg > kMaxRegNum) {
      ASSERT(0, "unexpected preg");
    }
  }
#endif
  mirFunc.CopyReferedRegs(referedRegs);
}

void AArch64AsmEmitter::Run(FuncEmitInfo &funcEmitInfo) {
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  AArch64CGFunc &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  /* emit header of this function */
  Emitter &emitter = *currCG->GetEmitter();
  // insert for  __cxx_global_var_init
  if (cgFunc.GetName() == "__cxx_global_var_init") {
    (void)emitter.Emit("\t.section\t.init_array,\"aw\"\n");
    (void)emitter.Emit("\t.quad\t").Emit(cgFunc.GetName()).Emit("\n");
  }
  if (cgFunc.GetFunction().GetAttr(FUNCATTR_initialization)) {
    (void)emitter.Emit("\t.section\t.init_array,\"aw\"\n");
    (void)emitter.Emit("\t.quad\t").Emit(cgFunc.GetName()).Emit("\n");
  }
  if (cgFunc.GetFunction().GetAttr(FUNCATTR_termination)) {
    (void)emitter.Emit("\t.section\t.fini_array,\"aw\"\n");
    (void)emitter.Emit("\t.quad\t").Emit(cgFunc.GetName()).Emit("\n");
  }
  (void)emitter.Emit("\n");
  EmitMethodDesc(funcEmitInfo, emitter);
  /* emit java code to the java section. */
  if (cgFunc.GetFunction().IsJava()) {
    std::string sectionName = namemangler::kMuidJavatextPrefixStr;
    (void)emitter.Emit("\t.section  ." + sectionName + ",\"ax\"\n");
  } else if (cgFunc.GetFunction().GetAttr(FUNCATTR_section)) {
    const std::string &sectionName = cgFunc.GetFunction().GetAttrs().GetPrefixSectionName();
    (void)emitter.Emit("\t.section  " + sectionName).Emit(",\"ax\",@progbits\n");
  } else if (CGOptions::IsFunctionSections()) {
    (void)emitter.Emit("\t.section  .text.").Emit(cgFunc.GetName()).Emit(",\"ax\",@progbits\n");
  } else if (cgFunc.GetFunction().GetAttr(FUNCATTR_constructor_priority)) {
    (void)emitter.Emit("\t.section\t.text.startup").Emit(",\"ax\",@progbits\n");
  } else {
    (void)emitter.Emit("\t.text\n");
  }
  if (CGOptions::GetFuncAlignPow() != 0) {
    (void)emitter.Emit("\t.align ").Emit(CGOptions::GetFuncAlignPow()).Emit("\n");
  }
  MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(cgFunc.GetFunction().GetStIdx().Idx());
  const std::string &funcName = std::string(cgFunc.GetShortFuncName().c_str());

  // manually replace function with optimized assembly language
  if (CGOptions::IsReplaceASM()) {
    auto it = func2CodeInsnMap.find(funcSt->GetName());
    if (it != func2CodeInsnMap.end()) {
      std::string optFile = it->second.first;
      struct stat buffer;
      if (stat(optFile.c_str(), &buffer) == 0) {
        std::ifstream codetricksFd(optFile);
        if (!codetricksFd.is_open()) {
          ERR(kLncErr, " %s open failed!", optFile.c_str());
          LogInfo::MapleLogger() << "wrong" << '\n';
        } else {
          std::string contend;
          while (getline(codetricksFd, contend)) {
            (void)emitter.Emit(contend + "\n");
          }
        }
      }
      emitter.IncreaseJavaInsnCount(it->second.second);
#ifdef EMIT_INSN_COUNT
      EmitJavaInsnAddr(funcEmitInfo);
#endif /* ~EMIT_INSN_COUNT */
      return;
    }
  }
  std::string funcStName = funcSt->GetName();
  if (funcSt->GetFunction()->GetAttr(FUNCATTR_weak)) {
    (void)emitter.Emit("\t.weak\t" + funcStName + "\n");
    (void)emitter.Emit("\t.hidden\t" + funcStName + "\n");
  } else if (funcSt->GetFunction()->GetAttr(FUNCATTR_local)) {
    (void)emitter.Emit("\t.local\t" + funcStName + "\n");
  } else if (funcSt->GetFunction() && (funcSt->GetFunction()->IsJava() == false) && funcSt->GetFunction()->IsStatic()) {
    // nothing
  } else {
  /* should refer to function attribute */
    (void)emitter.Emit("\t.globl\t").Emit(funcSt->GetName()).Emit("\n");
    if (!currCG->GetMIRModule()->IsCModule()) {
      (void)emitter.Emit("\t.hidden\t").Emit(funcSt->GetName()).Emit("\n");
    }
  }
  (void)emitter.Emit("\t.type\t" + funcStName + ", %function\n");
  /* add these messege , solve the simpleperf tool error */
  EmitRefToMethodDesc(funcEmitInfo, emitter);
  (void)emitter.Emit(funcStName + ":\n");

  /* if the last  insn is call, then insert nop */
  bool found = false;
  FOR_ALL_BB_REV(bb, &aarchCGFunc) {
    FOR_BB_INSNS_REV(insn, bb) {
      if (insn->IsMachineInstruction()) {
        if (insn->IsCall()) {
          Insn &newInsn = currCG->BuildInstruction<AArch64Insn>(MOP_nop);
          bb->InsertInsnAfter(*insn, newInsn);
        }
        found = true;
        break;
      }
    }
    if (found) {
      break;
    }
  }

  RecordRegInfo(funcEmitInfo);

  /* emit instructions */
  FOR_ALL_BB(bb, &aarchCGFunc) {
    if (bb->IsUnreachable()) {
      continue;
    }
    if (currCG->GenerateVerboseCG()) {
      (void)emitter.Emit("#    freq:").Emit(bb->GetFrequency()).Emit("\n");
    }
    /* emit bb headers */
    if (bb->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
      if (aarchCGFunc.GetMirModule().IsCModule() && bb->IsBBNeedAlign() && bb->GetAlignNopNum() != kAlignMovedFlag) {
        uint32 power = bb->GetAlignPower();
        (void)emitter.Emit("\t.p2align ").Emit(power).Emit("\n");
      }
      EmitBBHeaderLabel(funcEmitInfo, funcName, bb->GetLabIdx());
    }

    FOR_BB_INSNS(insn, bb) {
      if (insn->IsCfiInsn()) {
        EmitAArch64CfiInsn(emitter, *insn);
      } else if (insn->IsDbgInsn()) {
        EmitAArch64DbgInsn(emitter, *insn);
      } else {
        EmitAArch64Insn(emitter, *insn);
      }
    }
  }
  if (CGOptions::IsMapleLinker()) {
    /* Emit a label for calculating method size */
    (void)emitter.Emit(".Label.end." + funcStName + ":\n");
  }
  (void)emitter.Emit("\t.size\t" + funcStName + ", .-").Emit(funcStName + "\n");

  auto constructorAttr = funcSt->GetFunction()->GetAttrs().GetConstructorPriority();
  if (constructorAttr != -1) {
    (void)emitter.Emit("\t.section\t.init_array." + std::to_string(constructorAttr) + ",\"aw\"\n");
    (void)emitter.Emit("\t.align 3\n");
    (void)emitter.Emit("\t.xword\t" + funcStName + "\n");
  }

  EHFunc *ehFunc = cgFunc.GetEHFunc();
  /* emit LSDA */
  if (cgFunc.GetFunction().IsJava() && (ehFunc != nullptr)) {
    if (!cgFunc.GetHasProEpilogue()) {
      (void)emitter.Emit("\t.word 0x55555555\n");
      emitter.IncreaseJavaInsnCount();
    } else if (ehFunc->NeedFullLSDA()) {
      LSDAHeader *lsdaHeader = ehFunc->GetLSDAHeader();
      PUIdx pIdx = emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
      const std::string &idx = strdup(std::to_string(pIdx).c_str());
      /*  .word .Label.lsda_label-func_start_label */
      (void)emitter.Emit("\t.word .L." + idx).Emit("__").Emit(lsdaHeader->GetLSDALabel()->GetLabelIdx());
      (void)emitter.Emit("-.L." + idx).Emit("__").Emit(cgFunc.GetStartLabel()->GetLabelIdx()).Emit("\n");
      emitter.IncreaseJavaInsnCount();
    } else if (ehFunc->NeedFastLSDA()) {
      EmitFastLSDA(funcEmitInfo);
    }
  }
  uint32 size = static_cast<uint32>(cgFunc.GetFunction().GetSymTab()->GetSymbolTableSize());
  for (uint32 i = 0; i < size; ++i) {
    MIRSymbol *st = cgFunc.GetFunction().GetSymTab()->GetSymbolFromStIdx(i);
    if (st == nullptr) {
      continue;
    }
    MIRStorageClass storageClass = st->GetStorageClass();
    MIRSymKind symKind = st->GetSKind();
    if (storageClass == kScPstatic && symKind == kStConst) {
      (void)emitter.Emit("\t.align 3\n");
      (void)emitter.Emit(st->GetName() + ":\n");
      if (st->GetKonst()->GetKind() == kConstStr16Const) {
        MIRStr16Const *str16Const = safe_cast<MIRStr16Const>(st->GetKonst());
        emitter.EmitStr16Constant(*str16Const);
        (void)emitter.Emit("\n");
        continue;
      }
      if (st->GetKonst()->GetKind() == kConstStrConst) {
        MIRStrConst *strConst = safe_cast<MIRStrConst>(st->GetKonst());
        emitter.EmitStrConstant(*strConst);
        (void)emitter.Emit("\n");
        continue;
      }

      switch (st->GetKonst()->GetType().GetPrimType()) {
        case PTY_u32: {
          MIRIntConst *intConst = safe_cast<MIRIntConst>(st->GetKonst());
          (void)emitter.Emit("\t.long ").Emit(static_cast<uint32>(intConst->GetValue())).Emit("\n");
          emitter.IncreaseJavaInsnCount();
          break;
        }
        case PTY_f32: {
          MIRFloatConst *floatConst = safe_cast<MIRFloatConst>(st->GetKonst());
          (void)emitter.Emit("\t.word ").Emit(static_cast<uint32>(floatConst->GetIntValue())).Emit("\n");
          emitter.IncreaseJavaInsnCount();
          break;
        }
        case PTY_f64: {
          MIRDoubleConst *doubleConst = safe_cast<MIRDoubleConst>(st->GetKonst());
          auto emitF64 = [&](int64 first, int64 second) {
            (void)emitter.Emit("\t.word ").Emit(first).Emit("\n");
            emitter.IncreaseJavaInsnCount();
            (void)emitter.Emit("\t.word ").Emit(second).Emit("\n");
            emitter.IncreaseJavaInsnCount();
          };
          if (CGOptions::IsBigEndian()) {
            emitF64(doubleConst->GetIntHigh32(), doubleConst->GetIntLow32());
          } else {
            emitF64(doubleConst->GetIntLow32(), doubleConst->GetIntHigh32());
          }
          break;
        }
        default:
          ASSERT(false, "NYI");
          break;
      }
    }
  }

  for (auto &it : cgFunc.GetEmitStVec()) {
    /* emit switch table only here */
    MIRSymbol *st = it.second;
    ASSERT(st->IsReadOnly(), "NYI");
    (void)emitter.Emit("\n");
    (void)emitter.Emit("\t.align 3\n");
    emitter.IncreaseJavaInsnCount(0, true); /* just aligned */
    (void)emitter.Emit(st->GetName() + ":\n");
    MIRAggConst *arrayConst = safe_cast<MIRAggConst>(st->GetKonst());
    CHECK_FATAL(arrayConst != nullptr, "null ptr check");
    PUIdx pIdx = cgFunc.GetMirModule().CurFunction()->GetPuidx();
    char *idx = strdup(std::to_string(pIdx).c_str());
    for (size_t i = 0; i < arrayConst->GetConstVec().size(); i++) {
      MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));
      CHECK_FATAL(lblConst != nullptr, "null ptr check");
      (void)emitter.Emit("\t.quad\t.L.").Emit(idx).Emit("__").Emit(lblConst->GetValue());
      (void)emitter.Emit(" - " + st->GetName() + "\n");
      emitter.IncreaseJavaInsnCount(kQuadInsnCount);
    }
    free(idx);
    idx = nullptr;
  }
  /* insert manually optimized assembly language */
  if (funcSt->GetName() == "Landroid_2Futil_2FContainerHelpers_3B_7C_3Cinit_3E_7C_28_29V") {
    std::string optFile = "maple/mrt/codetricks/arch/arm64/ContainerHelpers_binarySearch.s";
    struct stat buffer;
    if (stat(optFile.c_str(), &buffer) == 0) {
      std::ifstream binarySearchFileFD(optFile);
      if (!binarySearchFileFD.is_open()) {
        ERR(kLncErr, " %s open failed!", optFile.c_str());
      } else {
        std::string contend;
        while (getline(binarySearchFileFD, contend)) {
          (void)emitter.Emit(contend + "\n");
        }
      }
    }
    emitter.IncreaseJavaInsnCount(kBinSearchInsnCount);
  }

  for (const auto &mpPair : cgFunc.GetLabelAndValueMap()) {
    LabelOperand &labelOpnd = aarchCGFunc.GetOrCreateLabelOperand(mpPair.first);
    A64OpndEmitVisitor visitor(emitter, nullptr);
    labelOpnd.Accept(visitor);
    (void)emitter.Emit(":\n");
    (void)emitter.Emit("\t.quad ").Emit(mpPair.second).Emit("\n");
    emitter.IncreaseJavaInsnCount(kQuadInsnCount);
  }

  if (ehFunc != nullptr && ehFunc->NeedFullLSDA()) {
    EmitFullLSDA(funcEmitInfo);
  }
#ifdef EMIT_INSN_COUNT
  if (cgFunc.GetFunction().IsJava()) {
    EmitJavaInsnAddr(funcEmitInfo);
  }
#endif /* ~EMIT_INSN_COUNT */
}

void AArch64AsmEmitter::EmitAArch64Insn(maplebe::Emitter &emitter, Insn &insn) {
  MOperator mOp = insn.GetMachineOpcode();
  emitter.SetCurrentMOP(mOp);
  const AArch64MD *md = &AArch64CG::kMd[mOp];

  if (!GetCG()->GenerateVerboseAsm() && !GetCG()->GenerateVerboseCG() && mOp == MOP_comment) {
    return;
  }

  switch (mOp) {
    case MOP_clinit: {
      EmitClinit(emitter, insn);
      emitter.IncreaseJavaInsnCount(md->GetAtomicNum());
      return;
    }
    case MOP_adrp_ldr: {
      uint32 adrpldrInsnCount = md->GetAtomicNum();
      emitter.IncreaseJavaInsnCount(adrpldrInsnCount);
      EmitAdrpLdr(emitter, insn);
      if (CGOptions::IsLazyBinding() && !GetCG()->IsLibcore()) {
        EmitLazyBindingRoutine(emitter, insn);
        emitter.IncreaseJavaInsnCount(adrpldrInsnCount + 1);
      }
      return;
    }
    case MOP_counter: {
      EmitCounter(emitter, insn);
      return;
    }
    case MOP_asm: {
      EmitInlineAsm(emitter, insn);
      return;
    }
    case MOP_clinit_tail: {
      EmitClinitTail(emitter, insn);
      emitter.IncreaseJavaInsnCount(md->GetAtomicNum());
      return;
    }
    case MOP_lazy_ldr: {
      EmitLazyLoad(emitter, insn);
      emitter.IncreaseJavaInsnCount(md->GetAtomicNum());
      return;
    }
    case MOP_adrp_label: {
      EmitAdrpLabel(emitter, insn);
      return;
    }
    case MOP_lazy_tail: {
      /* No need to emit this pseudo instruction. */
      return;
    }
    case MOP_lazy_ldr_static: {
      EmitLazyLoadStatic(emitter, insn);
      emitter.IncreaseJavaInsnCount(md->GetAtomicNum());
      return;
    }
    case MOP_arrayclass_cache_ldr: {
      EmitArrayClassCacheLoad(emitter, insn);
      emitter.IncreaseJavaInsnCount(md->GetAtomicNum());
      return;
    }
    case MOP_get_and_addI:
    case MOP_get_and_addL: {
      EmitGetAndAddInt(emitter, insn);
      return;
    }
    case MOP_get_and_setI:
    case MOP_get_and_setL: {
      EmitGetAndSetInt(emitter, insn);
      return;
    }
    case MOP_compare_and_swapI:
    case MOP_compare_and_swapL: {
      EmitCompareAndSwapInt(emitter, insn);
      return;
    }
    case MOP_string_indexof: {
      EmitStringIndexOf(emitter, insn);
      return;
    }
    case MOP_pseudo_none:
    case MOP_pseduo_tls_release: {
      return;
    }
    case MOP_tls_desc_call: {
      EmitCTlsDescCall(emitter, insn);
      return;
    }
    case MOP_tls_desc_rel: {
      EmitCTlsDescRel(emitter, insn);
      return;
    }
    case MOP_sync_lock_test_setI:
    case MOP_sync_lock_test_setL: {
      EmitSyncLockTestSet(emitter, insn);
      return;
    }
    default:
      break;
  }

  if (CGOptions::IsNativeOpt() && mOp == MOP_xbl) {
    auto *nameOpnd = static_cast<FuncNameOperand*>(&insn.GetOperand(kInsnFirstOpnd));
    if (nameOpnd->GetName() == "MCC_CheckThrowPendingException") {
      EmitCheckThrowPendingException(emitter, insn);
      emitter.IncreaseJavaInsnCount(md->GetAtomicNum());
      return;
    }
  }

  std::string format(md->format);
  (void)emitter.Emit("\t").Emit(md->name).Emit("\t");
  size_t opndSize = insn.GetOperandSize();
  std::vector<int32> seq(opndSize, -1);
  std::vector<std::string> prefix(opndSize);  /* used for print prefix like "*" in icall *rax */
  uint32 index = 0;
  uint32 commaNum = 0;
  for (uint32 i = 0; i < format.length(); ++i) {
    char c = format[i];
    if (c >= '0' && c <= '5') {
      seq[index++] = c - '0';
      ++commaNum;
    } else if (c != ',') {
      prefix[index].push_back(c);
    }
  }

  bool isRefField = (opndSize == 0) ? false : CheckInsnRefField(insn, static_cast<size_t>(static_cast<uint>(seq[0])));
  if (mOp != MOP_comment) {
    emitter.IncreaseJavaInsnCount();
  }
  uint32 compositeOpnds = 0;
  for (uint32 i = 0; i < commaNum; ++i) {
    if (seq[i] == -1) {
      continue;
    }
    if (prefix[i].length() > 0) {
      (void)emitter.Emit(prefix[i]);
    }
    if (emitter.NeedToDealWithHugeSo() && (mOp ==  MOP_xbl || mOp == MOP_tail_call_opt_xbl)) {
      auto *nameOpnd = static_cast<FuncNameOperand*>(&insn.GetOperand(kInsnFirstOpnd));
      /* Suport huge so here
       * As the PLT section is just before java_text section, when java_text section is larger
       * then 128M, instrunction of "b" and "bl" would fault to branch to PLT stub functions. Here, to save
       * instuctions space, we change the branch target to a local target within 120M address, and add non-plt
       * call to the target function.
       */
      emitter.InsertHugeSoTarget(nameOpnd->GetName());
      (void)emitter.Emit(nameOpnd->GetName() + emitter.HugeSoPostFix());
      break;
    }
    auto *opnd = &insn.GetOperand(static_cast<uint32>(seq[i]));
    if (opnd && opnd->IsRegister()) {
      auto *regOpnd = static_cast<RegOperand*>(opnd);
      if ((md->operand[static_cast<uint32>(seq[i])])->IsVectorOperand()) {
        regOpnd->SetVecLanePosition(-1);
        regOpnd->SetVecLaneSize(0);
        regOpnd->SetVecElementSize(0);
        if (insn.IsVectorOp()) {
          PrepareVectorOperand(regOpnd, compositeOpnds, insn);
          if (compositeOpnds != 0) {
            (void)emitter.Emit("{");
          }
        }
      }
    }
    A64OpndEmitVisitor visitor(emitter, md->operand[seq[i]]);

    insn.GetOperand(seq[i]).Accept(visitor);
    if (compositeOpnds == 1) {
      (void)emitter.Emit("}");
    }
    if (compositeOpnds > 0) {
      --compositeOpnds;
    }
    /* reset opnd0 ref-field flag, so following instruction has correct register */
    if (isRefField && (i == 0)) {
      static_cast<RegOperand*>(&insn.GetOperand(seq[0]))->SetRefField(false);
    }
    /* Temporary comment the label:.Label.debug.callee */
    if (i != (commaNum - 1)) {
      (void)emitter.Emit(", ");
    }
    const uint32 commaNumForEmitLazy = 2;
    if (!CGOptions::IsLazyBinding() || GetCG()->IsLibcore() || (mOp != MOP_wldr && mOp != MOP_xldr) ||
        commaNum != commaNumForEmitLazy || i != 1 || !insn.GetOperand(seq[1]).IsMemoryAccessOperand()) {
      continue;
    }
    /*
     * Only check the last operand of ldr in lo12 mode.
     * Check the second operand, if it's [AArch64MemOperand::kAddrModeLo12Li]
     */
    auto *memOpnd = static_cast<MemOperand*>(&insn.GetOperand(seq[1]));
    if (memOpnd == nullptr || memOpnd->GetAddrMode() != MemOperand::kAddrModeLo12Li) {
      continue;
    }
    const MIRSymbol *sym = memOpnd->GetSymbol();
    if (sym->IsMuidFuncDefTab() || sym->IsMuidFuncUndefTab() ||
        sym->IsMuidDataDefTab() || sym->IsMuidDataUndefTab()) {
      (void)emitter.Emit("\n");
      EmitLazyBindingRoutine(emitter, insn);
      emitter.IncreaseJavaInsnCount(1);
    }
  }
  if (GetCG()->GenerateVerboseCG() || (GetCG()->GenerateVerboseAsm() && mOp == MOP_comment)) {
    const char *comment = insn.GetComment().c_str();
    if (comment != nullptr && strlen(comment) > 0) {
      (void)emitter.Emit("\t\t// ").Emit(comment);
    }
  }

  (void)emitter.Emit("\n");
}

void AArch64AsmEmitter::EmitClinit(Emitter &emitter, const Insn &insn) const {
  /*
   * adrp    x3, __muid_data_undef_tab$$GetBoolean_dex+144
   * ldr     x3, [x3, #:lo12:__muid_data_undef_tab$$GetBoolean_dex+144]
   * or,
   * adrp    x3, _PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B
   * ldr     x3, [x3, #:lo12:_PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B]
   *
   * ldr x3, [x3,#112]
   * ldr wzr, [x3]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_clinit];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  OpndProp *prop0 = md->operand[0];
  A64OpndEmitVisitor visitor(emitter, prop0);
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Insn::EmitClinit");
  /* emit nop for breakpoint */
  if (GetCG()->GetCGOptions().WithDwarf()) {
    (void)emitter.Emit("\t").Emit("nop").Emit("\n");
  }

  if (stImmOpnd->GetSymbol()->IsMuidDataUndefTab()) {
    /* emit adrp */
    (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
    opnd0->Accept(visitor);
    (void)emitter.Emit(",");
    (void)emitter.Emit(stImmOpnd->GetName());
    (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    (void)emitter.Emit("\n");
    /* emit ldr */
    (void)emitter.Emit("\t").Emit("ldr").Emit("\t");
    opnd0->Accept(visitor);
    (void)emitter.Emit(",");
    (void)emitter.Emit("[");
    opnd0->Accept(visitor);
    (void)emitter.Emit(",");
    (void)emitter.Emit("#");
    (void)emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
    (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    (void)emitter.Emit("]");
    (void)emitter.Emit("\n");
  } else {
    /* adrp    x3, _PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B */
    (void)emitter.Emit("\tadrp\t");
    opnd0->Accept(visitor);
    (void)emitter.Emit(",");
    (void)emitter.Emit(namemangler::kPtrPrefixStr + stImmOpnd->GetName());
    (void)emitter.Emit("\n");

    /* ldr     x3, [x3, #:lo12:_PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B] */
    (void)emitter.Emit("\tldr\t");
    opnd0->Accept(visitor);
    (void)emitter.Emit(", [");
    opnd0->Accept(visitor);
    (void)emitter.Emit(", #:lo12:");
    (void)emitter.Emit(namemangler::kPtrPrefixStr + stImmOpnd->GetName());
    (void)emitter.Emit("]\n");
  }
  /* emit "ldr  x0,[x0,#48]" */
  (void)emitter.Emit("\t").Emit("ldr").Emit("\t");
  opnd0->Accept(visitor);
  (void)emitter.Emit(",");
  (void)emitter.Emit("[");
  opnd0->Accept(visitor);
  (void)emitter.Emit(",#");
  (void)emitter.Emit(static_cast<uint32>(ClassMetadata::OffsetOfInitState()));
  (void)emitter.Emit("]");
  (void)emitter.Emit("\n");

  /* emit "ldr  xzr, [x0]" */
  (void)emitter.Emit("\t").Emit("ldr\txzr, [");
  opnd0->Accept(visitor);
  (void)emitter.Emit("]\n");
}

static void AsmStringOutputRegNum(
    bool isInt, uint32 regno, uint32 intBase, uint32 fpBase, std::string &strToEmit) {
  regno_t newRegno;
  if (isInt) {
    newRegno = regno - intBase;
  } else {
    newRegno = regno - fpBase;
  }
  if (newRegno > (kDecimalMax - 1)) {
    uint32 tenth = newRegno / kDecimalMax;
    strToEmit += '0' + static_cast<char>(tenth);
    newRegno -= (kDecimalMax * tenth);
  }
  strToEmit += newRegno + '0';
}

void AArch64AsmEmitter::EmitInlineAsm(Emitter &emitter, const Insn &insn) const {
  (void)emitter.Emit("\t//Inline asm begin\n\t");
  auto &list1 = static_cast<ListOperand&>(insn.GetOperand(kAsmOutputListOpnd));
  std::vector<RegOperand *> outOpnds;
  for (auto *regOpnd : list1.GetOperands()) {
    outOpnds.push_back(regOpnd);
  }
  auto &list2 = static_cast<ListOperand&>(insn.GetOperand(kAsmInputListOpnd));
  std::vector<RegOperand *> inOpnds;
  for (auto *regOpnd : list2.GetOperands()) {
    inOpnds.push_back(regOpnd);
  }
  auto &list6 = static_cast<ListConstraintOperand&>(insn.GetOperand(kAsmOutputRegPrefixOpnd));
  auto &list7 = static_cast<ListConstraintOperand&>(insn.GetOperand(kAsmInputRegPrefixOpnd));
  MapleString asmStr = static_cast<StringOperand&>(insn.GetOperand(kAsmStringOpnd)).GetComment();
  std::string stringToEmit;
  size_t sidx = 0;
  auto IsMemAccess = [](char c)->bool {
    return c == '[';
  };
  auto EmitRegister = [&](const char *p, bool isInt, uint32 regNO, bool unDefRegSize)->void {
    if (IsMemAccess(p[0])) {
      stringToEmit += "[x";
      AsmStringOutputRegNum(isInt, regNO, R0, V0, stringToEmit);
      stringToEmit += "]";
    } else {
      ASSERT((p[0] == 'w' || p[0] == 'x' || p[0] == 's' || p[0] == 'd' || p[0] == 'v'), "Asm invalid register type");
      if ((p[0] == 'w' || p[0] == 'x') && unDefRegSize) {
        stringToEmit += 'x';
      } else {
        stringToEmit += p[0];
      }
      if (!unDefRegSize) {
        isInt = (p[0] == 'w' || p[0] == 'x');
      }
      AsmStringOutputRegNum(isInt, regNO, R0, V0, stringToEmit);
    }
  };
  for (size_t i = 0; i < asmStr.length(); ++i) {
    switch (asmStr[i]) {
      case '$': {
        char c = asmStr[++i];
        if ((c >= '0') && (c <= '9')) {
          auto val = static_cast<uint32>(c - '0');
          if (asmStr[i + 1] >= '0' && asmStr[i + 1] <= '9') {
            val = val * kDecimalMax + static_cast<uint32>(asmStr[++i] - '0');
          }
          if (val < outOpnds.size()) {
            const char *prefix = list6.stringList[val]->GetComment().c_str();
            RegOperand *opnd = outOpnds[val];
            EmitRegister(prefix, opnd->IsOfIntClass(), opnd->GetRegisterNumber(), true);
          } else {
            val -= static_cast<uint32>(outOpnds.size());
            CHECK_FATAL(val < inOpnds.size(), "Inline asm : invalid register constraint number");
            RegOperand *opnd = inOpnds[val];
            /* input is a immediate */
            const char *prefix = list7.stringList[val]->GetComment().c_str();
            if (prefix[0] == 'i') {
              stringToEmit += '#';
              for (size_t k = 1; k < list7.stringList[val]->GetComment().length(); ++k) {
                stringToEmit += prefix[k];
              }
            } else {
              EmitRegister(prefix, opnd->IsOfIntClass(), opnd->GetRegisterNumber(), true);
            }
          }
        } else if (c == '{') {
          c = asmStr[++i];
          CHECK_FATAL(((c >= '0') && (c <= '9')), "Inline asm : invalid register constraint number");
          auto val = static_cast<uint32>(c - '0');
          if (asmStr[i + 1] >= '0' && asmStr[i + 1] <= '9') {
            val = val * kDecimalMax + static_cast<uint32>(asmStr[++i] - '0');
          }
          regno_t regno;
          bool isAddr = false;
          if (val < outOpnds.size()) {
            RegOperand *opnd = outOpnds[val];
            regno = opnd->GetRegisterNumber();
            isAddr = IsMemAccess(list6.stringList[val]->GetComment().c_str()[0]);
          } else {
            val -= static_cast<uint32>(outOpnds.size());
            CHECK_FATAL(val < inOpnds.size(), "Inline asm : invalid register constraint number");
            RegOperand *opnd = inOpnds[val];
            regno = opnd->GetRegisterNumber();
            isAddr = IsMemAccess(list7.stringList[val]->GetComment().c_str()[0]);
          }
          c = asmStr[++i];
          CHECK_FATAL(c == ':', "Parsing error in inline asm string during emit");
          c = asmStr[++i];
          std::string prefix(1, c);
          if (c == 'a' || isAddr) {
            prefix = "[x";
          }
          EmitRegister(prefix.c_str(), true, regno, false);
          c = asmStr[++i];
          CHECK_FATAL(c == '}', "Parsing error in inline asm string during emit");
        }
        break;
      }
      case '\n': {
        stringToEmit += "\n\t";
        break;
      }
      default:
        stringToEmit += asmStr[i];
        sidx++;
    }
  }
  (void)emitter.Emit(stringToEmit);
  (void)emitter.Emit("\n\t//Inline asm end\n");
}

void AArch64AsmEmitter::EmitClinitTail(Emitter &emitter, const Insn &insn) const {
  /*
   * ldr x17, [xs, #112]
   * ldr wzr, [x17]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_clinit_tail];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);

  OpndProp *prop0 = md->operand[0];
  A64OpndEmitVisitor visitor(emitter, prop0);

  /* emit "ldr  x17,[xs,#112]" */
  (void)emitter.Emit("\t").Emit("ldr").Emit("\tx17, [");
  opnd0->Accept(visitor);
  (void)emitter.Emit(", #");
  (void)emitter.Emit(static_cast<uint32>(ClassMetadata::OffsetOfInitState()));
  (void)emitter.Emit("]");
  (void)emitter.Emit("\n");

  /* emit "ldr  xzr, [x17]" */
  (void)emitter.Emit("\t").Emit("ldr\txzr, [x17]\n");
}

void AArch64AsmEmitter::EmitLazyLoad(Emitter &emitter, const Insn &insn) const {
  /*
   * ldr wd, [xs]  # xd and xs should be differenct register
   * ldr wd, [xd]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_lazy_ldr];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  OpndProp *prop0 = md->operand[0];
  OpndProp *prop1 = md->operand[1];

  /* emit  "ldr wd, [xs]" */
  (void)emitter.Emit("\t").Emit("ldr\t");
#ifdef USE_32BIT_REF
  opnd0->Emit(emitter, prop0);
#else
  opnd0->Emit(emitter, prop1);
#endif
  (void)emitter.Emit(", [");
  opnd1->Emit(emitter, prop1);
  (void)emitter.Emit("]\t// lazy load.\n");

  /* emit "ldr wd, [xd]" */
  (void)emitter.Emit("\t").Emit("ldr\t");
  opnd0->Emit(emitter, prop0);
  (void)emitter.Emit(", [");
  opnd0->Emit(emitter, prop1);
  (void)emitter.Emit("]\t// lazy load.\n");
}

void AArch64AsmEmitter::EmitCounter(Emitter &emitter, const Insn &insn) const {
  /*
   * adrp    x1, __profile_bb_table$$GetBoolean_dex+4
   * ldr     w17, [x1, #:lo12:__profile_bb_table$$GetBoolean_dex+4]
   * add     w17, w17, #1
   * str     w17, [x1, #:lo12:__profile_bb_table$$GetBoolean_dex+4]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_counter];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  OpndProp *prop0 = md->operand[kInsnFirstOpnd];
  A64OpndEmitVisitor visitor(emitter, prop0);
  StImmOperand *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Insn::EmitCounter");
  /* emit nop for breakpoint */
  if (GetCG()->GetCGOptions().WithDwarf()) {
    (void)emitter.Emit("\t").Emit("nop").Emit("\n");
  }

  /* emit adrp */
  (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Accept(visitor);
  (void)emitter.Emit(",");
  (void)emitter.Emit(stImmOpnd->GetName());
  (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  (void)emitter.Emit("\n");
  /* emit ldr */
  (void)emitter.Emit("\t").Emit("ldr").Emit("\tw17, [");
  opnd0->Accept(visitor);
  (void)emitter.Emit(",");
  (void)emitter.Emit("#");
  (void)emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
  (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  (void)emitter.Emit("]");
  (void)emitter.Emit("\n");
  /* emit add */
  (void)emitter.Emit("\t").Emit("add").Emit("\tw17, w17, #1");
  (void)emitter.Emit("\n");
  /* emit str */
  (void)emitter.Emit("\t").Emit("str").Emit("\tw17, [");
  opnd0->Accept(visitor);
  (void)emitter.Emit(",");
  (void)emitter.Emit("#");
  (void)emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
  (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  (void)emitter.Emit("]");
  (void)emitter.Emit("\n");
}

void AArch64AsmEmitter::EmitAdrpLabel(Emitter &emitter, const Insn &insn) const {
  /* adrp    xd, label
   * add     xd, xd, #lo12:label
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_adrp_label];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  OpndProp *prop0 = md->operand[0];
  A64OpndEmitVisitor visitor(emitter, prop0);
  auto lidx = static_cast<ImmOperand *>(opnd1)->GetValue();

  /* adrp    xd, label */
  (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Accept(visitor);
  (void)emitter.Emit(", ");
  char *idx;
  idx = strdup(std::to_string(Globals::GetInstance()->GetBECommon()->GetMIRModule().CurFunction()->GetPuidx()).c_str());
  (void)emitter.Emit(".L.").Emit(idx).Emit("__").Emit(lidx).Emit("\n");

  /* add     xd, xd, #lo12:label */
  (void)emitter.Emit("\tadd\t");
  opnd0->Accept(visitor);
  (void)emitter.Emit(", ");
  opnd0->Accept(visitor);
  (void)emitter.Emit(", ");
  (void)emitter.Emit(":lo12:").Emit(".L.").Emit(idx).Emit("__").Emit(lidx).Emit("\n");
  (void)emitter.Emit("\n");
  free(idx);
  idx = nullptr;
}

void AArch64AsmEmitter::EmitAdrpLdr(Emitter &emitter, const Insn &insn) const {
  /*
   * adrp    xd, _PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B
   * ldr     xd, [xd, #:lo12:_PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_adrp_ldr];
  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  OpndProp *prop0 = md->operand[0];
  A64OpndEmitVisitor visitor(emitter, prop0);
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Insn::EmitAdrpLdr");
  /* emit nop for breakpoint */
  if (GetCG()->GetCGOptions().WithDwarf()) {
    (void)emitter.Emit("\t").Emit("nop").Emit("\n");
  }

  /* adrp    xd, _PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B */
  (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Accept(visitor);
  (void)emitter.Emit(", ");
  (void)emitter.Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  (void)emitter.Emit("\n");

  /* ldr     xd, [xd, #:lo12:_PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B] */
  (void)emitter.Emit("\tldr\t");
  static_cast<RegOperand*>(opnd0)->SetRefField(true);
  opnd0->Accept(visitor);
  static_cast<RegOperand*>(opnd0)->SetRefField(false);
  (void)emitter.Emit(", ");
  (void)emitter.Emit("[");
  opnd0->Accept(visitor);
  (void)emitter.Emit(",");
  (void)emitter.Emit("#");
  (void)emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  (void)emitter.Emit("]\n");
}

void AArch64AsmEmitter::EmitLazyLoadStatic(Emitter &emitter, const Insn &insn) const {
  /* adrp xd, :got:__staticDecoupleValueOffset$$xxx+offset
   * ldr wd, [xd, #:got_lo12:__staticDecoupleValueOffset$$xxx+offset]
   * ldr wzr, [xd]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_lazy_ldr_static];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  OpndProp *prop0 = md->GetOperand(0);
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Insn::EmitLazyLoadStatic");

  /* emit "adrp xd, :got:__staticDecoupleValueOffset$$xxx+offset" */
  (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Emit(emitter, prop0);
  (void)emitter.Emit(", ");
  (void)emitter.Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  (void)emitter.Emit("\t// lazy load static.\n");

  /* emit "ldr wd, [xd, #:got_lo12:__staticDecoupleValueOffset$$xxx+offset]" */
  (void)emitter.Emit("\tldr\t");
  static_cast<RegOperand*>(opnd0)->SetRefField(true);
#ifdef USE_32BIT_REF
  OpndProp prop2(prop0->GetOperandType(), prop0->GetRegProp(), prop0->GetSize() / 2);
  opnd0->Emit(emitter, &prop2); /* ldr wd, ... for emui */
#else
  opnd0->Emit(emitter, prop0);  /* ldr xd, ... for qemu */
#endif /* USE_32BIT_REF */
  static_cast<RegOperand*>(opnd0)->SetRefField(false);
  (void)emitter.Emit(", ");
  (void)emitter.Emit("[");
  opnd0->Emit(emitter, prop0);
  (void)emitter.Emit(",");
  (void)emitter.Emit("#");
  (void)emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  (void)emitter.Emit("]\t// lazy load static.\n");

  /* emit "ldr wzr, [xd]" */
  (void)emitter.Emit("\t").Emit("ldr\twzr, [");
  opnd0->Emit(emitter, prop0);
  (void)emitter.Emit("]\t// lazy load static.\n");
}

void AArch64AsmEmitter::EmitArrayClassCacheLoad(Emitter &emitter, const Insn &insn) const {
  /* adrp xd, :got:__arrayClassCacheTable$$xxx+offset
   * ldr wd, [xd, #:got_lo12:__arrayClassCacheTable$$xxx+offset]
   * ldr wzr, [xd]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_arrayclass_cache_ldr];
  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  OpndProp *prop0 = md->GetOperand(kInsnFirstOpnd);
  A64OpndEmitVisitor visitor(emitter, prop0);
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Insn::EmitLazyLoadStatic");

  /* emit "adrp xd, :got:__arrayClassCacheTable$$xxx+offset" */
  (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Accept(visitor);
  (void)emitter.Emit(", ");
  (void)emitter.Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  (void)emitter.Emit("\t// load array class.\n");

  /* emit "ldr wd, [xd, #:got_lo12:__arrayClassCacheTable$$xxx+offset]" */
  (void)emitter.Emit("\tldr\t");
  static_cast<RegOperand*>(opnd0)->SetRefField(true);
#ifdef USE_32BIT_REF
  OpndProp prop2(prop0->GetOperandType(), prop0->GetRegProp(), prop0->GetSize() / 2);
  A64OpndEmitVisitor visitor2(emitter, prop2);
  opnd0->Accept(visitor2);; /* ldr wd, ... for emui */
#else
  opnd0->Accept(visitor);  /* ldr xd, ... for qemu */
#endif /* USE_32BIT_REF */
  static_cast<RegOperand*>(opnd0)->SetRefField(false);
  (void)emitter.Emit(", ");
  (void)emitter.Emit("[");
  opnd0->Accept(visitor);
  (void)emitter.Emit(",");
  (void)emitter.Emit("#");
  (void)emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    (void)emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  (void)emitter.Emit("]\t// load array class.\n");

  /* emit "ldr wzr, [xd]" */
  (void)emitter.Emit("\t").Emit("ldr\twzr, [");
  opnd0->Accept(visitor);
  (void)emitter.Emit("]\t// check resolve array class.\n");
}

/*
 * intrinsic_get_add_int w0, xt, wt, ws, x1, x2, w3, label
 * add    xt, x1, x2
 * label:
 * ldaxr  w0, [xt]
 * add    wt, w0, w3
 * stlxr  ws, wt, [xt]
 * cbnz   ws, label
 */
void AArch64AsmEmitter::EmitGetAndAddInt(Emitter &emitter, const Insn &insn) const {
  ASSERT(insn.GetOperandSize() > kInsnEighthOpnd, "ensure the oprands number");
  (void)emitter.Emit("\t//\tstart of Unsafe.getAndAddInt.\n");
  Operand *tempOpnd0 = &insn.GetOperand(kInsnSecondOpnd);
  Operand *tempOpnd1 = &insn.GetOperand(kInsnThirdOpnd);
  Operand *tempOpnd2 = &insn.GetOperand(kInsnFourthOpnd);
  Operand *objOpnd = &insn.GetOperand(kInsnFifthOpnd);
  Operand *offsetOpnd = &insn.GetOperand(kInsnSixthOpnd);
  Operand *deltaOpnd = &insn.GetOperand(kInsnSeventhOpnd);
  Operand *labelOpnd = &insn.GetOperand(kInsnEighthOpnd);
  A64OpndEmitVisitor visitor(emitter, nullptr);
  /* emit add. */
  (void)emitter.Emit("\t").Emit("add").Emit("\t");
  tempOpnd0->Accept(visitor);
  (void)emitter.Emit(", ");
  objOpnd->Accept(visitor);
  (void)emitter.Emit(", ");
  offsetOpnd->Accept(visitor);
  (void)emitter.Emit("\n");
  /* emit label. */
  labelOpnd->Accept(visitor);
  (void)emitter.Emit(":\n");
  Operand *retVal = &insn.GetOperand(kInsnFirstOpnd);
  const MOperator mOp = insn.GetMachineOpcode();
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  OpndProp *retProp = md->operand[kInsnFirstOpnd];
  A64OpndEmitVisitor retVisitor(emitter, retProp);
  /* emit ldaxr */
  (void)emitter.Emit("\t").Emit("ldaxr").Emit("\t");
  retVal->Accept(retVisitor);
  (void)emitter.Emit(", [");
  tempOpnd0->Accept(visitor);
  (void)emitter.Emit("]\n");
  /* emit add. */
  (void)emitter.Emit("\t").Emit("add").Emit("\t");
  tempOpnd1->Accept(retVisitor);
  (void)emitter.Emit(", ");
  retVal->Accept(retVisitor);
  (void)emitter.Emit(", ");
  deltaOpnd->Accept(retVisitor);
  (void)emitter.Emit("\n");
  /* emit stlxr. */
  (void)emitter.Emit("\t").Emit("stlxr").Emit("\t");
  tempOpnd2->Accept(visitor);
  (void)emitter.Emit(", ");
  tempOpnd1->Accept(retVisitor);
  (void)emitter.Emit(", [");
  tempOpnd0->Accept(visitor);
  (void)emitter.Emit("]\n");
  /* emit cbnz. */
  (void)emitter.Emit("\t").Emit("cbnz").Emit("\t");
  tempOpnd2->Accept(visitor);
  (void)emitter.Emit(", ");
  labelOpnd->Accept(visitor);
  (void)emitter.Emit("\n");
  (void)emitter.Emit("\t//\tend of Unsafe.getAndAddInt.\n");
}

/*
 * intrinsic_get_set_int w0, xt, ws, x1, x2, w3, label
 * add    xt, x1, x2
 * label:
 * ldaxr  w0, [xt]
 * stlxr  ws, w3, [xt]
 * cbnz   ws, label
 */
void AArch64AsmEmitter::EmitGetAndSetInt(Emitter &emitter, const Insn &insn) const {
  /* MOP_get_and_setI and MOP_get_and_setL have 7 operands */
  ASSERT(insn.GetOperandSize() > kInsnSeventhOpnd, "ensure the operands number");
  Operand *tempOpnd0 = &insn.GetOperand(kInsnSecondOpnd);
  Operand *tempOpnd1 = &insn.GetOperand(kInsnThirdOpnd);
  Operand *objOpnd = &insn.GetOperand(kInsnFourthOpnd);
  Operand *offsetOpnd = &insn.GetOperand(kInsnFifthOpnd);
  A64OpndEmitVisitor visitor(emitter, nullptr);
  /* add    x1, x1, x2 */
  (void)emitter.Emit("\tadd\t");
  tempOpnd0->Accept(visitor);
  (void)emitter.Emit(", ");
  objOpnd->Accept(visitor);
  (void)emitter.Emit(", ");
  offsetOpnd->Accept(visitor);
  (void)emitter.Emit("\n");
  Operand *labelOpnd = &insn.GetOperand(kInsnSeventhOpnd);
  /* label: */
  labelOpnd->Accept(visitor);
  (void)emitter.Emit(":\n");
  Operand *retVal = &insn.GetOperand(kInsnFirstOpnd);
  /* ldaxr  w0, [xt] */
  (void)emitter.Emit("\tldaxr\t");
  retVal->Accept(visitor);
  (void)emitter.Emit(", [");
  tempOpnd0->Accept(visitor);
  (void)emitter.Emit("]\n");
  Operand *newValueOpnd = &insn.GetOperand(kInsnSixthOpnd);
  /* stlxr  ws, w3, [xt] */
  (void)emitter.Emit("\tstlxr\t");
  tempOpnd1->Accept(visitor);
  (void)emitter.Emit(", ");
  newValueOpnd->Accept(visitor);
  (void)emitter.Emit(", [");
  tempOpnd0->Accept(visitor);
  (void)emitter.Emit("]\n");
  /* cbnz   w2, label */
  (void)emitter.Emit("\tcbnz\t");
  tempOpnd1->Accept(visitor);
  (void)emitter.Emit(", ");
  labelOpnd->Accept(visitor);
  (void)emitter.Emit("\n");
}

/*
 * intrinsic_string_indexof w0, x1, w2, x3, w4, x5, x6, x7, x8, x9, w10,
 *                          Label.FIRST_LOOP, Label.STR2_NEXT, Label.STR1_LOOP,
 *                          Label.STR1_NEXT, Label.LAST_WORD, Label.NOMATCH, Label.RET
 * cmp       w4, w2
 * b.gt      .Label.NOMATCH
 * sub       w2, w2, w4
 * sub       w4, w4, #8
 * mov       w10, w2
 * uxtw      x4, w4
 * uxtw      x2, w2
 * add       x3, x3, x4
 * add       x1, x1, x2
 * neg       x4, x4
 * neg       x2, x2
 * ldr       x5, [x3,x4]
 * .Label.FIRST_LOOP:
 * ldr       x7, [x1,x2]
 * cmp       x5, x7
 * b.eq      .Label.STR1_LOOP
 * .Label.STR2_NEXT:
 * adds      x2, x2, #1
 * b.le      .Label.FIRST_LOOP
 * b         .Label.NOMATCH
 * .Label.STR1_LOOP:
 * adds      x8, x4, #8
 * add       x9, x2, #8
 * b.ge      .Label.LAST_WORD
 * .Label.STR1_NEXT:
 * ldr       x6, [x3,x8]
 * ldr       x7, [x1,x9]
 * cmp       x6, x7
 * b.ne      .Label.STR2_NEXT
 * adds      x8, x8, #8
 * add       x9, x9, #8
 * b.lt      .Label.STR1_NEXT
 * .Label.LAST_WORD:
 * ldr       x6, [x3]
 * sub       x9, x1, x4
 * ldr       x7, [x9,x2]
 * cmp       x6, x7
 * b.ne      .Label.STR2_NEXT
 * add       w0, w10, w2
 * b         .Label.RET
 * .Label.NOMATCH:
 * mov       w0, #-1
 * .Label.RET:
 */
void AArch64AsmEmitter::EmitStringIndexOf(Emitter &emitter, const Insn &insn) const {
  /* MOP_string_indexof has 18 operands */
  ASSERT(insn.GetOperandSize() == 18, "ensure the operands number");
  Operand *patternLengthOpnd = &insn.GetOperand(kInsnFifthOpnd);
  Operand *srcLengthOpnd = &insn.GetOperand(kInsnThirdOpnd);
  const std::string patternLengthReg =
      AArch64CG::intRegNames[AArch64CG::kR64List][static_cast<RegOperand*>(patternLengthOpnd)->GetRegisterNumber()];
  const std::string srcLengthReg =
      AArch64CG::intRegNames[AArch64CG::kR64List][static_cast<RegOperand*>(srcLengthOpnd)->GetRegisterNumber()];
  A64OpndEmitVisitor visitor(emitter, nullptr);
  /* cmp       w4, w2 */
  (void)emitter.Emit("\tcmp\t");
  patternLengthOpnd->Accept(visitor);
  (void)emitter.Emit(", ");
  srcLengthOpnd->Accept(visitor);
  (void)emitter.Emit("\n");
  /* the 16th operand of MOP_string_indexof is Label.NOMATCH */
  Operand *labelNoMatch = &insn.GetOperand(16);
  /* b.gt      Label.NOMATCH */
  (void)emitter.Emit("\tb.gt\t");
  labelNoMatch->Accept(visitor);
  (void)emitter.Emit("\n");
  /* sub       w2, w2, w4 */
  (void)emitter.Emit("\tsub\t");
  srcLengthOpnd->Accept(visitor);
  (void)emitter.Emit(", ");
  srcLengthOpnd->Accept(visitor);
  (void)emitter.Emit(", ");
  patternLengthOpnd->Accept(visitor);
  (void)emitter.Emit("\n");
  /* sub       w4, w4, #8 */
  (void)emitter.Emit("\tsub\t");
  patternLengthOpnd->Accept(visitor);
  (void)emitter.Emit(", ");
  patternLengthOpnd->Accept(visitor);
  (void)emitter.Emit(", #8\n");
  /* the 10th operand of MOP_string_indexof is w10 */
  Operand *resultTmp = &insn.GetOperand(10);
  /* mov       w10, w2 */
  (void)emitter.Emit("\tmov\t");
  resultTmp->Accept(visitor);
  (void)emitter.Emit(", ");
  srcLengthOpnd->Accept(visitor);
  (void)emitter.Emit("\n");
  /* uxtw      x4, w4 */
  (void)emitter.Emit("\tuxtw\t").Emit(patternLengthReg);
  (void)emitter.Emit(", ");
  patternLengthOpnd->Accept(visitor);
  (void)emitter.Emit("\n");
  /* uxtw      x2, w2 */
  (void)emitter.Emit("\tuxtw\t").Emit(srcLengthReg);
  (void)emitter.Emit(", ");
  srcLengthOpnd->Accept(visitor);
  (void)emitter.Emit("\n");
  Operand *patternStringBaseOpnd = &insn.GetOperand(kInsnFourthOpnd);
  /* add       x3, x3, x4 */
  (void)emitter.Emit("\tadd\t");
  patternStringBaseOpnd->Accept(visitor);
  (void)emitter.Emit(", ");
  patternStringBaseOpnd->Accept(visitor);
  (void)emitter.Emit(", ").Emit(patternLengthReg);
  (void)emitter.Emit("\n");
  Operand *srcStringBaseOpnd = &insn.GetOperand(kInsnSecondOpnd);
  /* add       x1, x1, x2 */
  (void)emitter.Emit("\tadd\t");
  srcStringBaseOpnd->Accept(visitor);
  (void)emitter.Emit(", ");
  srcStringBaseOpnd->Accept(visitor);
  (void)emitter.Emit(", ").Emit(srcLengthReg);
  (void)emitter.Emit("\n");
  /* neg       x4, x4 */
  (void)emitter.Emit("\tneg\t").Emit(patternLengthReg);
  (void)emitter.Emit(", ").Emit(patternLengthReg);
  (void)emitter.Emit("\n");
  /* neg       x2, x2 */
  (void)emitter.Emit("\tneg\t").Emit(srcLengthReg);
  (void)emitter.Emit(", ").Emit(srcLengthReg);
  (void)emitter.Emit("\n");
  Operand *first = &insn.GetOperand(kInsnSixthOpnd);
  /* ldr       x5, [x3,x4] */
  (void)emitter.Emit("\tldr\t");
  first->Accept(visitor);
  (void)emitter.Emit(", [");
  patternStringBaseOpnd->Accept(visitor);
  (void)emitter.Emit(",").Emit(patternLengthReg);
  (void)emitter.Emit("]\n");
  /* the 11th operand of MOP_string_indexof is Label.FIRST_LOOP */
  Operand *labelFirstLoop = &insn.GetOperand(11);
  /* .Label.FIRST_LOOP: */
  labelFirstLoop->Accept(visitor);
  (void)emitter.Emit(":\n");
  /* the 7th operand of MOP_string_indexof is x7 */
  Operand *ch2 = &insn.GetOperand(7);
  /* ldr       x7, [x1,x2] */
  (void)emitter.Emit("\tldr\t");
  ch2->Accept(visitor);
  (void)emitter.Emit(", [");
  srcStringBaseOpnd->Accept(visitor);
  (void)emitter.Emit(",").Emit(srcLengthReg);
  (void)emitter.Emit("]\n");
  /* cmp       x5, x7 */
  (void)emitter.Emit("\tcmp\t");
  first->Accept(visitor);
  (void)emitter.Emit(", ");
  ch2->Accept(visitor);
  (void)emitter.Emit("\n");
  /* the 13th operand of MOP_string_indexof is Label.STR1_LOOP */
  Operand *labelStr1Loop = &insn.GetOperand(13);
  /* b.eq      .Label.STR1_LOOP */
  (void)emitter.Emit("\tb.eq\t");
  labelStr1Loop->Accept(visitor);
  (void)emitter.Emit("\n");
  /* the 12th operand of MOP_string_indexof is Label.STR2_NEXT */
  Operand *labelStr2Next = &insn.GetOperand(12);
  /* .Label.STR2_NEXT: */
  labelStr2Next->Accept(visitor);
  (void)emitter.Emit(":\n");
  /* adds      x2, x2, #1 */
  (void)emitter.Emit("\tadds\t").Emit(srcLengthReg);
  (void)emitter.Emit(", ").Emit(srcLengthReg);
  (void)emitter.Emit(", #1\n");
  /* b.le      .Label.FIRST_LOOP */
  (void)emitter.Emit("\tb.le\t");
  labelFirstLoop->Accept(visitor);
  (void)emitter.Emit("\n");
  /* b         .Label.NOMATCH */
  (void)emitter.Emit("\tb\t");
  labelNoMatch->Accept(visitor);
  (void)emitter.Emit("\n");
  /* .Label.STR1_LOOP: */
  labelStr1Loop->Accept(visitor);
  (void)emitter.Emit(":\n");
  /* the 8th operand of MOP_string_indexof is x8 */
  Operand *tmp1 = &insn.GetOperand(kInsnEighthOpnd);
  /* adds      x8, x4, #8 */
  (void)emitter.Emit("\tadds\t");
  tmp1->Accept(visitor);
  (void)emitter.Emit(", ").Emit(patternLengthReg);
  (void)emitter.Emit(", #8\n");
  /* the 9th operand of MOP_string_indexof is x9 */
  Operand *tmp2 = &insn.GetOperand(9);
  /* add       x9, x2, #8 */
  (void)emitter.Emit("\tadd\t");
  tmp2->Accept(visitor);
  (void)emitter.Emit(", ").Emit(srcLengthReg);
  (void)emitter.Emit(", #8\n");
  /* the 15th operand of MOP_string_indexof is Label.LAST_WORD */
  Operand *labelLastWord = &insn.GetOperand(15);
  /* b.ge      .Label.LAST_WORD */
  (void)emitter.Emit("\tb.ge\t");
  labelLastWord->Accept(visitor);
  (void)emitter.Emit("\n");
  /* the 14th operand of MOP_string_indexof is Label.STR1_NEXT */
  Operand *labelStr1Next = &insn.GetOperand(14);
  /* .Label.STR1_NEXT: */
  labelStr1Next->Accept(visitor);
  (void)emitter.Emit(":\n");
  /* the 6th operand of MOP_string_indexof is x6 */
  Operand *ch1 = &insn.GetOperand(6);
  /* ldr       x6, [x3,x8] */
  (void)emitter.Emit("\tldr\t");
  ch1->Accept(visitor);
  (void)emitter.Emit(", [");
  patternStringBaseOpnd->Accept(visitor);
  (void)emitter.Emit(",");
  tmp1->Accept(visitor);
  (void)emitter.Emit("]\n");
  /* ldr       x7, [x1,x9] */
  (void)emitter.Emit("\tldr\t");
  ch2->Accept(visitor);
  (void)emitter.Emit(", [");
  srcStringBaseOpnd->Accept(visitor);
  (void)emitter.Emit(",");
  tmp2->Accept(visitor);
  (void)emitter.Emit("]\n");
  /* cmp       x6, x7 */
  (void)emitter.Emit("\tcmp\t");
  ch1->Accept(visitor);
  (void)emitter.Emit(", ");
  ch2->Accept(visitor);
  (void)emitter.Emit("\n");
  /* b.ne      .Label.STR2_NEXT */
  (void)emitter.Emit("\tb.ne\t");
  labelStr2Next->Accept(visitor);
  (void)emitter.Emit("\n");
  /* adds      x8, x8, #8 */
  (void)emitter.Emit("\tadds\t");
  tmp1->Accept(visitor);
  (void)emitter.Emit(", ");
  tmp1->Accept(visitor);
  (void)emitter.Emit(", #8\n");
  /* add       x9, x9, #8 */
  (void)emitter.Emit("\tadd\t");
  tmp2->Accept(visitor);
  (void)emitter.Emit(", ");
  tmp2->Accept(visitor);
  (void)emitter.Emit(", #8\n");
  /* b.lt      .Label.STR1_NEXT */
  (void)emitter.Emit("\tb.lt\t");
  labelStr1Next->Accept(visitor);
  (void)emitter.Emit("\n");
  /* .Label.LAST_WORD: */
  labelLastWord->Accept(visitor);
  (void)emitter.Emit(":\n");
  /* ldr       x6, [x3] */
  (void)emitter.Emit("\tldr\t");
  ch1->Accept(visitor);
  (void)emitter.Emit(", [");
  patternStringBaseOpnd->Accept(visitor);
  (void)emitter.Emit("]\n");
  /* sub       x9, x1, x4 */
  (void)emitter.Emit("\tsub\t");
  tmp2->Accept(visitor);
  (void)emitter.Emit(", ");
  srcStringBaseOpnd->Accept(visitor);
  (void)emitter.Emit(", ").Emit(patternLengthReg);
  (void)emitter.Emit("\n");
  /* ldr       x7, [x9,x2] */
  (void)emitter.Emit("\tldr\t");
  ch2->Accept(visitor);
  (void)emitter.Emit(", [");
  tmp2->Accept(visitor);
  (void)emitter.Emit(", ").Emit(srcLengthReg);
  (void)emitter.Emit("]\n");
  /* cmp       x6, x7 */
  (void)emitter.Emit("\tcmp\t");
  ch1->Accept(visitor);
  (void)emitter.Emit(", ");
  ch2->Accept(visitor);
  (void)emitter.Emit("\n");
  /* b.ne      .Label.STR2_NEXT */
  (void)emitter.Emit("\tb.ne\t");
  labelStr2Next->Accept(visitor);
  (void)emitter.Emit("\n");
  Operand *retVal = &insn.GetOperand(kInsnFirstOpnd);
  /* add       w0, w10, w2 */
  (void)emitter.Emit("\tadd\t");
  retVal->Accept(visitor);
  (void)emitter.Emit(", ");
  resultTmp->Accept(visitor);
  (void)emitter.Emit(", ");
  srcLengthOpnd->Accept(visitor);
  (void)emitter.Emit("\n");
  /* the 17th operand of MOP_string_indexof Label.ret */
  Operand *labelRet = &insn.GetOperand(17);
  /* b         .Label.ret */
  (void)emitter.Emit("\tb\t");
  labelRet->Accept(visitor);
  (void)emitter.Emit("\n");
  /* .Label.NOMATCH: */
  labelNoMatch->Accept(visitor);
  (void)emitter.Emit(":\n");
  /* mov       w0, #-1 */
  (void)emitter.Emit("\tmov\t");
  retVal->Accept(visitor);
  (void)emitter.Emit(", #-1\n");
  /* .Label.ret: */
  labelRet->Accept(visitor);
  (void)emitter.Emit(":\n");
}

/*
 * intrinsic_compare_swap_int x0, xt, xs, x1, x2, w3, w4, lable1, label2
 * add       xt, x1, x2
 * label1:
 * ldaxr     ws, [xt]
 * cmp       ws, w3
 * b.ne      label2
 * stlxr     ws, w4, [xt]
 * cbnz      ws, label1
 * label2:
 * cset      x0, eq
 */
void AArch64AsmEmitter::EmitCompareAndSwapInt(Emitter &emitter, const Insn &insn) const {
  /* MOP_compare_and_swapI and MOP_compare_and_swapL have 8 operands */
  ASSERT(insn.GetOperandSize() > kInsnEighthOpnd, "ensure the operands number");
  const MOperator mOp = insn.GetMachineOpcode();
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  Operand *temp0 = &insn.GetOperand(kInsnSecondOpnd);
  Operand *temp1 = &insn.GetOperand(kInsnThirdOpnd);
  Operand *obj = &insn.GetOperand(kInsnFourthOpnd);
  Operand *offset = &insn.GetOperand(kInsnFifthOpnd);
  A64OpndEmitVisitor visitor(emitter, nullptr);
  /* add       xt, x1, x2 */
  (void)emitter.Emit("\tadd\t");
  temp0->Accept(visitor);
  (void)emitter.Emit(", ");
  obj->Accept(visitor);
  (void)emitter.Emit(", ");
  offset->Accept(visitor);
  (void)emitter.Emit("\n");
  Operand *label1 = &insn.GetOperand(kInsnEighthOpnd);
  /* label1: */
  label1->Accept(visitor);
  (void)emitter.Emit(":\n");
  /* ldaxr     ws, [xt] */
  (void)emitter.Emit("\tldaxr\t");
  temp1->Accept(visitor);
  (void)emitter.Emit(", [");
  temp0->Accept(visitor);
  (void)emitter.Emit("]\n");
  Operand *expectedValue = &insn.GetOperand(kInsnSixthOpnd);
  OpndProp *expectedValueProp = md->operand[kInsnSixthOpnd];
  /* cmp       ws, w3 */
  (void)emitter.Emit("\tcmp\t");
  temp1->Accept(visitor);
  (void)emitter.Emit(", ");
  A64OpndEmitVisitor visitorExpect(emitter, expectedValueProp);
  expectedValue->Accept(visitorExpect);
  (void)emitter.Emit("\n");
  constexpr uint32 kInsnNinethOpnd = 8;
  Operand *label2 = &insn.GetOperand(kInsnNinethOpnd);
  /* b.ne      label2 */
  (void)emitter.Emit("\tbne\t");
  label2->Accept(visitor);
  (void)emitter.Emit("\n");
  Operand *newValue = &insn.GetOperand(kInsnSeventhOpnd);
  /* stlxr     ws, w4, [xt] */
  (void)emitter.Emit("\tstlxr\t");
  (void)emitter.Emit(AArch64CG::intRegNames[AArch64CG::kR32List][static_cast<RegOperand*>(temp1)->GetRegisterNumber()]);
  (void)emitter.Emit(", ");
  newValue->Accept(visitor);
  (void)emitter.Emit(", [");
  temp0->Accept(visitor);
  (void)emitter.Emit("]\n");
  /* cbnz      ws, label1 */
  (void)emitter.Emit("\tcbnz\t");
  (void)emitter.Emit(AArch64CG::intRegNames[AArch64CG::kR32List][static_cast<RegOperand*>(temp1)->GetRegisterNumber()]);
  (void)emitter.Emit(", ");
  label1->Accept(visitor);
  (void)emitter.Emit("\n");
  /* label2: */
  label2->Accept(visitor);
  (void)emitter.Emit(":\n");
  Operand *retVal = &insn.GetOperand(kInsnFirstOpnd);
  /* cset      x0, eq */
  (void)emitter.Emit("\tcset\t");
  retVal->Accept(visitor);
  (void)emitter.Emit(", EQ\n");
}

void AArch64AsmEmitter::EmitCTlsDescRel(Emitter &emitter, const Insn &insn) const {
  const AArch64MD *md = &AArch64CG::kMd[MOP_tls_desc_rel];
  Operand *result = &insn.GetOperand(kInsnFirstOpnd);
  Operand *src = &insn.GetOperand(kInsnSecondOpnd);
  Operand *symbol = &insn.GetOperand(kInsnThirdOpnd);
  auto stImmOpnd = static_cast<StImmOperand*>(symbol);
  A64OpndEmitVisitor resultVisitor(emitter, md->operand[0]);
  A64OpndEmitVisitor srcVisitor(emitter, md->operand[1]);
  (void)emitter.Emit("\t").Emit("add").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", ");
  src->Accept(srcVisitor);
  (void)emitter.Emit(", #:tprel_hi12:").Emit(stImmOpnd->GetName()).Emit(", lsl #12\n");
  (void)emitter.Emit("\t").Emit("add").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", ");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", #:tprel_lo12_nc:").Emit(stImmOpnd->GetName()).Emit("\n");
}
void AArch64AsmEmitter::EmitCTlsDescCall(Emitter &emitter, const Insn &insn) const {
  const AArch64MD *md = &AArch64CG::kMd[MOP_tls_desc_call];
  Operand *func = &insn.GetOperand(kInsnFirstOpnd);
  Operand *symbol = &insn.GetOperand(kInsnThirdOpnd);
  OpndProp *prop = md->operand[kInsnFirstOpnd];
  auto stImmOpnd = static_cast<StImmOperand*>(symbol);
  const std::string &symName = stImmOpnd->GetName();
  A64OpndEmitVisitor funcVisitor(emitter, prop);
  /*  adrp    x0, :tlsdesc:symbol */
  emitter.Emit("\t").Emit("adrp\tx0, :tlsdesc:").Emit(symName).Emit("\n");
  /*  ldr x1, [x0, #tlsdesc_lo12:symbol] */
  emitter.Emit("\t").Emit("ldr").Emit("\t");
  func->Accept(funcVisitor);
  emitter.Emit(", [x0, #:tlsdesc_lo12:").Emit(symName).Emit("]\n");
  /*  add x0 ,#tlsdesc_lo12:symbol */
  emitter.Emit("\t").Emit("add\tx0, x0, :tlsdesc_lo12:").Emit(symName).Emit("\n");
  /* .tlsdesccall <symbolName> */
  (void)emitter.Emit("\t").Emit(".tlsdesccall").Emit("\t").Emit(symName).Emit("\n");
  /* blr xd*/
  (void)emitter.Emit("\t").Emit("blr").Emit("\t");
  func->Accept(funcVisitor);
  (void)emitter.Emit("\n");
}
void AArch64AsmEmitter::EmitSyncLockTestSet(Emitter &emitter, const Insn &insn) const {
  const AArch64MD *md = &AArch64CG::kMd[insn.GetMachineOpcode()];
  auto *result = &insn.GetOperand(kInsnFirstOpnd);
  auto *temp = &insn.GetOperand(kInsnSecondOpnd);
  auto *addr = &insn.GetOperand(kInsnThirdOpnd);
  auto *value = &insn.GetOperand(kInsnFourthOpnd);
  auto *label = &insn.GetOperand(kInsnFifthOpnd);
  A64OpndEmitVisitor resultVisitor(emitter, md->operand[kInsnFirstOpnd]);
  A64OpndEmitVisitor tempVisitor(emitter, md->operand[kInsnSecondOpnd]);
  A64OpndEmitVisitor addrVisitor(emitter, md->operand[kInsnThirdOpnd]);
  A64OpndEmitVisitor valueVisitor(emitter, md->operand[kInsnFourthOpnd]);
  A64OpndEmitVisitor labelVisitor(emitter, md->operand[kInsnFifthOpnd]);
  /* label: */
  label->Accept(labelVisitor);
  (void)emitter.Emit(":\n");
  /* ldxr x0, [x2] */
  (void)emitter.Emit("\t").Emit("ldxr").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", [");
  addr->Accept(addrVisitor);
  (void)emitter.Emit("]\n");
  /* stxr w1, x3, [x2]*/
  (void)emitter.Emit("\t").Emit("stxr").Emit("\t");
  temp->Accept(tempVisitor);
  (void)emitter.Emit(", ");
  value->Accept(valueVisitor);
  (void)emitter.Emit(", [");
  addr->Accept(addrVisitor);
  (void)emitter.Emit("]\n");
  /* cbnz w1, label */
  (void)emitter.Emit("\t").Emit("cbnz").Emit("\t");
  temp->Accept(tempVisitor);
  (void)emitter.Emit(", ");
  label->Accept(labelVisitor);
  (void)emitter.Emit("\n");
  /* dmb ish*/
  (void)emitter.Emit("\t").Emit("dmb").Emit("\t").Emit("ish").Emit("\n");
}

void AArch64AsmEmitter::EmitCheckThrowPendingException(Emitter &emitter, Insn &insn) const {
  /*
   * mrs x16, TPIDR_EL0
   * ldr x16, [x16, #64]
   * ldr x16, [x16, #8]
   * cbz x16, .lnoexception
   * bl MCC_ThrowPendingException
   * .lnoexception:
   */
  (void)emitter.Emit("\t").Emit("mrs").Emit("\tx16, TPIDR_EL0");
  (void)emitter.Emit("\n");
  (void)emitter.Emit("\t").Emit("ldr").Emit("\tx16, [x16, #64]");
  (void)emitter.Emit("\n");
  (void)emitter.Emit("\t").Emit("ldr").Emit("\tx16, [x16, #8]");
  (void)emitter.Emit("\n");
  (void)emitter.Emit("\t").Emit("cbz").Emit("\tx16, .lnoeh.").Emit(maplebe::CG::GetCurCGFunc()->GetName());
  (void)emitter.Emit("\n");
  (void)emitter.Emit("\t").Emit("bl").Emit("\tMCC_ThrowPendingException");
  (void)emitter.Emit("\n");
  (void)emitter.Emit(".lnoeh.").Emit(maplebe::CG::GetCurCGFunc()->GetName()).Emit(":");
  (void)emitter.Emit("\n");
}

void AArch64AsmEmitter::EmitLazyBindingRoutine(Emitter &emitter, const Insn &insn) const {
  /* ldr xzr, [xs] */
  const AArch64MD *md = &AArch64CG::kMd[MOP_adrp_ldr];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  OpndProp *prop0 = md->operand[0];

  /* emit "ldr  xzr,[xs]" */
#ifdef USE_32BIT_REF
  (void)emitter.Emit("\t").Emit("ldr").Emit("\twzr, [");
#else
  (void)emitter.Emit("\t").Emit("ldr").Emit("\txzr, [");
#endif /* USE_32BIT_REF */
  opnd0->Emit(emitter, prop0);
  (void)emitter.Emit("]");
  (void)emitter.Emit("\t// Lazy binding\n");
}

void AArch64AsmEmitter::PrepareVectorOperand(RegOperand *regOpnd, uint32 &compositeOpnds, Insn &insn) const {
  VectorRegSpec* vecSpec = static_cast<AArch64VectorInsn&>(insn).GetAndRemoveRegSpecFromList();
  compositeOpnds = vecSpec->compositeOpnds ? vecSpec->compositeOpnds : compositeOpnds;
  regOpnd->SetVecLanePosition(vecSpec->vecLane);
  switch (insn.GetMachineOpcode()) {
    case MOP_vanduuu:
    case MOP_vxoruuu:
    case MOP_voruuu:
    case MOP_vnotuu:
    case MOP_vextuuui: {
      regOpnd->SetVecLaneSize(k8ByteSize);
      regOpnd->SetVecElementSize(k8BitSize);
      break;
    }
    case MOP_vandvvv:
    case MOP_vxorvvv:
    case MOP_vorvvv:
    case MOP_vnotvv:
    case MOP_vextvvvi: {
      regOpnd->SetVecLaneSize(k16ByteSize);
      regOpnd->SetVecElementSize(k8BitSize);
      break;
    }
    default: {
      regOpnd->SetVecLaneSize(vecSpec->vecLaneMax);
      regOpnd->SetVecElementSize(vecSpec->vecElementSize);
      break;
    }
  }
}

struct CfiDescr {
  const std::string name;
  uint32 opndCount;
  /* create 3 OperandType array to store cfi instruction's operand type */
  std::array<Operand::OperandType, 3> opndTypes;
};

static CfiDescr cfiDescrTable[cfi::kOpCfiLast + 1] = {
#define CFI_DEFINE(k, sub, n, o0, o1, o2) \
  { ".cfi_" #k, n, { Operand::kOpd##o0, Operand::kOpd##o1, Operand::kOpd##o2 } },
  #define ARM_DIRECTIVES_DEFINE(k, sub, n, o0, o1, o2) \
  { "." #k, n, { Operand::kOpd##o0, Operand::kOpd##o1, Operand::kOpd##o2 } },
  #include "cfi.def"
  #undef CFI_DEFINE
#undef ARM_DIRECTIVES_DEFINE
  { ".cfi_undef", 0, { Operand::kOpdUndef, Operand::kOpdUndef, Operand::kOpdUndef } }
};

void AArch64AsmEmitter::EmitAArch64CfiInsn(Emitter &emitter, const Insn &insn) {
  MOperator mOp = insn.GetMachineOpcode();
  CfiDescr &cfiDescr = cfiDescrTable[mOp];
  (void)emitter.Emit("\t").Emit(cfiDescr.name);
  for (uint32 i = 0; i < cfiDescr.opndCount; ++i) {
    (void)emitter.Emit(" ");
    Operand &curOperand = insn.GetOperand(i);
    curOperand.Emit(emitter, nullptr);
    if (i < (cfiDescr.opndCount - 1)) {
      (void)emitter.Emit(",");
    }
  }
  (void)emitter.Emit("\n");
}

struct DbgDescr {
  const std::string name;
  uint32 opndCount;
  /* create 3 OperandType array to store dbg instruction's operand type */
  std::array<Operand::OperandType, 3> opndTypes;
};

static DbgDescr dbgDescrTable[mpldbg::kOpDbgLast + 1] = {
#define DBG_DEFINE(k, sub, n, o0, o1, o2) \
  { #k, n, { Operand::kOpd##o0, Operand::kOpd##o1, Operand::kOpd##o2 } },
#include "dbg.def"
#undef DBG_DEFINE
  { "undef", 0, { Operand::kOpdUndef, Operand::kOpdUndef, Operand::kOpdUndef } }
};

void AArch64AsmEmitter::EmitAArch64DbgInsn(Emitter &emitter, const Insn &insn) {
  MOperator mOp = insn.GetMachineOpcode();
  DbgDescr &dbgDescr = dbgDescrTable[mOp];
  (void)emitter.Emit("\t.").Emit(dbgDescr.name);
  for (uint32 i = 0; i < dbgDescr.opndCount; ++i) {
    (void)emitter.Emit(" ");
    Operand &curOperand = insn.GetOperand(i);
    curOperand.Emit(emitter, nullptr);
  }
  (void)emitter.Emit("\n");
}

bool AArch64AsmEmitter::CheckInsnRefField(Insn &insn, size_t opndIndex) const {
  if (insn.IsAccessRefField() && static_cast<AArch64Insn&>(insn).AccessMem()) {
    Operand &opnd0 = insn.GetOperand(opndIndex);
    if (opnd0.IsRegister()) {
      static_cast<RegOperand&>(opnd0).SetRefField(true);
      return true;
    }
  }
  return false;
}

/* new phase manager */
bool CgEmission::PhaseRun(maplebe::CGFunc &f) {
  Emitter *emitter = f.GetCG()->GetEmitter();
  CHECK_NULL_FATAL(emitter);
  AsmFuncEmitInfo funcEmitInfo(f);
  emitter->EmitLocalVariable(f);
  static_cast<AArch64AsmEmitter*>(emitter)->Run(funcEmitInfo);
  emitter->EmitHugeSoRoutines();
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgEmission, cgemit)
}  /* namespace maplebe */
