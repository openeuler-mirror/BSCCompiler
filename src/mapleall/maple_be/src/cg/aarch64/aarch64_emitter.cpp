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
#include "cg_irbuilder.h"

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
  for (size_t i = ehFunc->GetEHTyTableSize(); i > 0; i--) {
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ehFunc->GetEHTyTableMember(i - 1));
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

void AArch64AsmEmitter::RecordRegInfo(FuncEmitInfo &funcEmitInfo) const {
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
        CHECK_FATAL(targetOpnd != nullptr, "target is null in AArch64Emitter::IsCallToFunctionThatNeverReturns");
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
        if (!safeCheck) {
          mirFunc.SetReferedRegsValid(false);
          return;
        }
      }
      if (referedRegs.size() == kMaxRegNum) {
        break;
      }
      uint32 opndNum = insn->GetOperandSize();
      const InsnDesc *md = &AArch64CG::kMd[insn->GetMachineOpcode()];
      for (uint32 i = 0; i < opndNum; ++i) {
        if (insn->GetMachineOpcode() == MOP_asm) {
          if (i == kAsmOutputListOpnd || i == kAsmClobberListOpnd) {
            for (auto &opnd : static_cast<const ListOperand &>(insn->GetOperand(i)).GetOperands()) {
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
          bool isDef = md->GetOpndDes(i)->IsRegDef();
          if (isDef) {
            referedRegs.insert(static_cast<RegOperand&>(opnd).GetRegisterNumber());
          }
        }
      }
    }
  }
  (void)referedRegs.insert(R16);
  (void)referedRegs.insert(R17);
  (void)referedRegs.insert(R18);
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

/* if the last insn is call, then insert nop */
static void InsertNopAfterLastCall(AArch64CGFunc &cgFunc) {
    bool found = false;
    FOR_ALL_BB_REV(bb, &cgFunc) {
      FOR_BB_INSNS_REV(insn, bb) {
        if (insn->IsMachineInstruction()) {
          if (insn->IsCall()) {
            Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_nop);
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
}

void AArch64AsmEmitter::EmitCallWithLocalAlias(Emitter &emitter, const std::string &funcName,
                                               const std::string &mdName) const {
  std::string funcAliasName = funcName + ".localalias";
  // emit call instruction
  (void)emitter.Emit("\t").Emit(mdName).Emit("\t");
  (void)emitter.Emit(funcAliasName).Emit("\n");
}

void HandleSpecificSec(Emitter &emitter, CGFunc &cgFunc) {
  const std::string &sectionName = cgFunc.GetFunction().GetAttrs().GetPrefixSectionName();
  if (sectionName == "pkt_fwd" && cgFunc.GetPriority() != 0) {
    (void)emitter.Emit("\t.section  .text.hot.").Emit(cgFunc.GetPriority()).Emit("\n");
    return;
  }
  emitter.Emit("\t.section\t" + sectionName);
  if (cgFunc.GetPriority() != 0) {
    if (!opts::linkerTimeOpt.IsEnabledByUser()) {
        emitter.Emit(".").Emit(cgFunc.GetPriority());
    }
  }
  bool isInInitArray = sectionName == ".init_array";
  bool isInFiniArray = sectionName == ".fini_array";
  if (isInFiniArray || isInInitArray) {
    emitter.Emit(",\"aw\"\n");
  } else {
    emitter.Emit(",\"ax\",@progbits\n");
  }
  /*
   * Register function pointer in init/fini array
   * Set function body in text
   */
  if (isInInitArray || isInFiniArray) {
    emitter.Emit("\t.quad\t").Emit(cgFunc.GetName()).Emit("\n");
    emitter.Emit("\t.text\n");
  }
}

static void EmitLocalAliasOfFuncName(Emitter &emitter, const std::string &funcName) {
  auto funcAliasName = funcName + ".localalias";
  (void)emitter.Emit("\t.set\t");
  (void)emitter.Emit(funcAliasName).Emit(", ");
  (void)emitter.Emit(funcName).Emit("\n");
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
  (void)emitter.Emit("\n");
  EmitMethodDesc(funcEmitInfo, emitter);
  /* emit java code to the java section. */
  if (cgFunc.GetFunction().IsJava()) {
    std::string sectionName = namemangler::kMuidJavatextPrefixStr;
    (void)emitter.Emit("\t.section  ." + sectionName + ",\"ax\"\n");
  } else if (cgFunc.GetFunction().GetAttr(FUNCATTR_section)) {
    HandleSpecificSec(emitter, cgFunc);
  } else if (CGOptions::IsFunctionSections()) {
    (void)emitter.Emit("\t.section  .text.").Emit(cgFunc.GetName()).Emit(",\"ax\",@progbits\n");
  } else if (cgFunc.GetFunction().GetAttr(FUNCATTR_constructor_priority)) {
    (void)emitter.Emit("\t.section\t.text.startup").Emit(",\"ax\",@progbits\n");
  } else if (cgFunc.GetPriority() != 0) {
    (void)emitter.Emit("\t.section  .text.hot.").Emit(cgFunc.GetPriority()).Emit("\n");
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
  std::string funcOriginName = funcStName;
  // When entry bb is cold, hot bbs are set before entry bb.
  // Function original name label should be set next to entry bb, so we name the label before hot bbs "func.hot".
  if (cgFunc.IsEntryCold()) {
    funcStName = funcOriginName + ".hot";
  }
  MIRFunction *func = funcSt->GetFunction();
  if (func->GetAttr(FUNCATTR_weak)) {
    (void)emitter.Emit("\t.weak\t" + funcStName + "\n");
    if (currCG->GetMIRModule()->IsJavaModule()) {
      (void)emitter.Emit("\t.hidden\t" + funcStName + "\n");
    }
  } else if (func->GetAttr(FUNCATTR_local)) {
    (void)emitter.Emit("\t.local\t" + funcStName + "\n");
  } else if (func && (!func->IsJava()) && func->IsStatic()) {
    // nothing
  } else {
    /* should refer to function attribute */
    (void)emitter.Emit("\t.globl\t").Emit(funcStName).Emit("\n");
    /* if no visibility set individually, set it to be same as the -fvisibility value */
    CHECK_NULL_FATAL(func);
    if (!func->IsStatic() && func->IsDefaultVisibility()) {
      switch (CGOptions::GetVisibilityType()) {
        case CGOptions::kHiddenVisibility:
          func->SetAttr(FUNCATTR_visibility_hidden);
          break;
        case CGOptions::kProtectedVisibility:
          func->SetAttr(FUNCATTR_visibility_protected);
          break;
        default:
          func->SetAttr(FUNCATTR_visibility_default);
          break;
      }
    }
    if (!currCG->GetMIRModule()->IsCModule() || func->GetAttr(FUNCATTR_visibility_hidden)) {
      (void)emitter.Emit("\t.hidden\t").Emit(funcStName).Emit("\n");
    } else if (func->GetAttr(FUNCATTR_visibility_protected)) {
      (void)emitter.Emit("\t.protected\t").Emit(funcStName).Emit("\n");
    }
  }
  (void)emitter.Emit("\t.type\t" + funcStName + ", %function\n");
  /* add these messege , solve the simpleperf tool error */
  EmitRefToMethodDesc(funcEmitInfo, emitter);
  (void)emitter.Emit(funcStName + ":\n");

  if (cgFunc.GetFunction().IsJava()) {
    InsertNopAfterLastCall(aarchCGFunc);
  }

  RecordRegInfo(funcEmitInfo);

  /* set hot-cold section boundary */
  BB *boundaryBB = nullptr;
  BB *lastBB = nullptr;
  FOR_ALL_BB(bb, &aarchCGFunc) {
    if (bb->IsInColdSection() && boundaryBB == nullptr) {
      boundaryBB = bb;
      // fix startlable and endlabe in splited funcs
      CG *cg = cgFunc.GetCG();
      if (cg->GetCGOptions().WithDwarf() && cgFunc.GetWithSrc()) {
        LabelIdx endLblIdxHot = cgFunc.CreateLabel();
        BB *newEndBB = cgFunc.CreateNewBB();
        BB *firstBB = cgFunc.GetFirstBB();
        newEndBB->AddLabel(endLblIdxHot);
        if (lastBB) {
          lastBB->AppendBB(*newEndBB);
        }
        DebugInfo *di = cg->GetMIRModule()->GetDbgInfo();
        DBGDie *fdie = di->GetFuncDie(func);
        fdie->SetAttr(DW_AT_high_pc, endLblIdxHot);
        CG::SetFuncWrapLabels(func, std::make_pair(firstBB->GetLabIdx(), endLblIdxHot));
      }
    }
    if (boundaryBB && !bb->IsInColdSection()) {
      LogInfo::MapleLogger() << " ==== in Func " << aarchCGFunc.GetName() << " ====\n";
      LogInfo::MapleLogger() << " boundaryBB : " << boundaryBB->GetId() << "\n";
      bb->Dump();
      CHECK_FATAL_FALSE("cold section is not pure!");
    }
    lastBB = bb;
  }

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
        EmitAArch64DbgInsn(funcEmitInfo, emitter, *insn);
      } else {
        EmitAArch64Insn(emitter, *insn);
      }
    }
    if (boundaryBB && bb->GetNext() == boundaryBB) {
      (void)emitter.Emit("\t.size\t" + funcStName + ", . - " + funcStName + "\n");
      if (CGOptions::IsNoSemanticInterposition() && !cgFunc.GetFunction().IsStatic() &&
          cgFunc.GetFunction().IsDefaultVisibility()) {
        EmitLocalAliasOfFuncName(emitter, funcStName);
      }
      // When entry bb is cold, the func label before cold section should be function original name.
      funcStName = cgFunc.IsEntryCold() ? funcOriginName : funcOriginName + ".cold";
      std::string sectionName = ".text.unlikely." + funcStName;
      (void)emitter.Emit("\t.section  " + sectionName + ",\"ax\"\n");
      (void)emitter.Emit("\t.align 5\n");
      // Because entry bb will be called, so the label next to entry bb should be global for caller.
      if (cgFunc.IsEntryCold()) {
        (void)emitter.Emit("\t.globl\t").Emit(funcStName).Emit("\n");
        (void)emitter.Emit("\t.type\t" + funcStName + ", %function\n");
      }
      (void)emitter.Emit(funcStName + ":\n");
      funcStName = funcOriginName;
    }
  }
  if (CGOptions::IsMapleLinker()) {
    /* Emit a label for calculating method size */
    (void)emitter.Emit(".Label.end." + funcStName + ":\n");
  }
  if (cgFunc.GetExitBBLost()) {
    EmitAArch64CfiInsn(emitter, cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_endproc));
  }
  if (boundaryBB) {
    if (cgFunc.IsEntryCold()) {
      (void)emitter.Emit("\t.size\t" + funcStName + ", .-").Emit(funcStName + "\n");
    } else {
      (void)emitter.Emit("\t.size\t" + funcStName + ".cold, .-").Emit(funcStName + ".cold\n");
    }
  } else {
    (void)emitter.Emit("\t.size\t" + funcStName + ", .-").Emit(funcStName + "\n");
  }

  if (!boundaryBB && CGOptions::IsNoSemanticInterposition() && !cgFunc.GetFunction().IsStatic() &&
      cgFunc.GetFunction().IsDefaultVisibility()) {
    EmitLocalAliasOfFuncName(emitter, funcStName);
  }
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
    (void)emitter.Emit("\t.quad ").Emit(static_cast<int64>(mpPair.second)).Emit("\n");
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

void AArch64AsmEmitter::EmitAArch64Insn(maplebe::Emitter &emitter, Insn &insn) const {
  MOperator mOp = insn.GetMachineOpcode();
  emitter.SetCurrentMOP(mOp);
  const InsnDesc *md = insn.GetDesc();

  if (!GetCG()->GenerateVerboseAsm() && !GetCG()->GenerateVerboseCG() && insn.IsComment()) {
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
    case MOP_c_counter: {
      EmitCCounter(emitter, insn);
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
    case MOP_pseudo_none: {
      return;
    }
    case MOP_tlsload_tdata: {
      EmitCTlsLoadTdata(emitter, insn);
      return;
    }
    case MOP_tlsload_tbss: {
      EmitCTlsLoadTbss(emitter, insn);
      return;
    }
    case MOP_tls_desc_call: {
      EmitCTlsDescCall(emitter, insn);
      return;
    }
    case MOP_tls_desc_call_warmup: {
      EmitCTlsDescCallWarmup(emitter, insn);
      return;
    }
    case MOP_tls_desc_rel: {
      EmitCTlsDescRel(emitter, insn);
      return;
    }
    case MOP_tls_desc_got: {
      EmitCTlsDescGot(emitter, insn);
      return;
    }
    case MOP_sync_lock_test_setI:
    case MOP_sync_lock_test_setL: {
      EmitSyncLockTestSet(emitter, insn);
      return;
    }
    case MOP_prefetch: {
      EmitPrefetch(emitter, insn);
      return;
    }
    default:
      break;
  }

  if (CGOptions::IsNativeOpt() && mOp == MOP_xbl) {
    auto *nameOpnd = static_cast<FuncNameOperand*>(&insn.GetOperand(kInsnFirstOpnd));
    if (nameOpnd->GetName() == "MCC_CheckThrowPendingException") {
      EmitCheckThrowPendingException(emitter);
      emitter.IncreaseJavaInsnCount(md->GetAtomicNum());
      return;
    }
  }
  /* if fno-semantic-interposition is enabled, print function alias instead */
  if ((md->IsCall() || md->IsTailCall()) && insn.GetOperand(kInsnFirstOpnd).IsFuncNameOpnd() &&
      CGOptions::IsNoSemanticInterposition()) {
    const MIRSymbol *funcSymbol = static_cast<FuncNameOperand&>(insn.GetOperand(kInsnFirstOpnd)).GetFunctionSymbol();
    MIRFunction *mirFunc = funcSymbol->GetFunction();
    if (mirFunc && !mirFunc->IsStatic() && mirFunc->HasBody() && mirFunc->IsDefaultVisibility()) {
      EmitCallWithLocalAlias(emitter, funcSymbol->GetName(), md->GetName());
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
      seq[index++] = static_cast<int32>(c) - kZeroAsciiNum;
      ++commaNum;
    } else if (c != ',') {
      prefix[index].push_back(c);
    }
  }

  bool isRefField = (opndSize == 0) ? false : CheckInsnRefField(insn, static_cast<uint32>(seq[0]));
  if (insn.IsComment()) {
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
      if ((md->opndMD[static_cast<uint32>(seq[i])])->IsVectorOperand()) {
        regOpnd->SetVecLanePosition(-1);
        regOpnd->SetVecLaneSize(0);
        regOpnd->SetVecElementSize(0);
        PrepareVectorOperand(regOpnd, compositeOpnds, insn);
        if (compositeOpnds != 0) {
          (void)emitter.Emit("{");
        }
      }
    }
    A64OpndEmitVisitor visitor(emitter, md->opndMD[static_cast<uint32>(seq[i])]);

    insn.GetOperand(static_cast<uint32>(seq[i])).Accept(visitor);
    if (compositeOpnds == 1) {
      (void)emitter.Emit("}");
    }
    if (compositeOpnds > 0) {
      --compositeOpnds;
    }
    /* reset opnd0 ref-field flag, so following instruction has correct register */
    if (isRefField && (i == 0)) {
      static_cast<RegOperand*>(&insn.GetOperand(static_cast<uint32>(seq[0])))->SetRefField(false);
    }
    /* Temporary comment the label:.Label.debug.callee */
    if (i != (commaNum - 1)) {
      (void)emitter.Emit(", ");
    }
    const uint32 commaNumForEmitLazy = 2;
    if (!CGOptions::IsLazyBinding() || GetCG()->IsLibcore() || (mOp != MOP_wldr && mOp != MOP_xldr) ||
        commaNum != commaNumForEmitLazy || i != 1 ||
        !insn.GetOperand(static_cast<uint32>(seq[1])).IsMemoryAccessOperand()) {
      continue;
    }
    /*
     * Only check the last operand of ldr in lo12 mode.
     * Check the second operand, if it's [AArch64MemOperand::kAddrModeLo12Li]
     */
    auto *memOpnd = static_cast<MemOperand*>(&insn.GetOperand(static_cast<uint32>(seq[1])));
    if (memOpnd == nullptr || memOpnd->GetAddrMode() != MemOperand::kLo12Li) {
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
  if (GetCG()->GenerateVerboseCG() || (GetCG()->GenerateVerboseAsm() && insn.IsComment())) {
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
  const InsnDesc *md = &AArch64CG::kMd[MOP_clinit];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  const OpndDesc *prop0 = md->opndMD[0];
  A64OpndEmitVisitor visitor(emitter, prop0);
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Emitter::EmitClinit");
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
    CHECK_FATAL(regno >= 35, "The input type must be float.");
    newRegno = regno - fpBase;
  }
  if (newRegno > (kDecimalMax - 1)) {
    uint32 tenth = newRegno / kDecimalMax;
    strToEmit += static_cast<char>(kZeroAsciiNum + tenth);
    newRegno -= (kDecimalMax * tenth);
  }
  strToEmit += static_cast<char>(static_cast<int32>(newRegno) + kZeroAsciiNum);
}

void AArch64AsmEmitter::EmitInlineAsm(Emitter &emitter, const Insn &insn) const {
  (void)emitter.Emit("\t//Inline asm begin\n\t");
  auto &list1 = static_cast<const ListOperand&>(insn.GetOperand(kAsmOutputListOpnd));
  std::vector<RegOperand *> outOpnds;
  for (auto *regOpnd : list1.GetOperands()) {
    outOpnds.push_back(regOpnd);
  }
  auto &list2 = static_cast<const ListOperand&>(insn.GetOperand(kAsmInputListOpnd));
  std::vector<RegOperand *> inOpnds;
  for (auto *regOpnd : list2.GetOperands()) {
    inOpnds.push_back(regOpnd);
  }
  auto &list6 = static_cast<ListConstraintOperand&>(insn.GetOperand(kAsmOutputRegPrefixOpnd));
  auto &list7 = static_cast<ListConstraintOperand&>(insn.GetOperand(kAsmInputRegPrefixOpnd));
  MapleString asmStr = static_cast<StringOperand&>(insn.GetOperand(kAsmStringOpnd)).GetComment();
  std::string stringToEmit;
  auto isMemAccess = [](char c)->bool {
    return c == '[';
  };
  auto emitRegister = [&stringToEmit, &isMemAccess](const char *p, bool isInt, uint32 regNO, bool unDefRegSize)->void {
    if (isMemAccess(p[0])) {
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
          auto val = static_cast<uint32>(static_cast<int32>(c) - kZeroAsciiNum);
          if (asmStr[i + 1] >= '0' && asmStr[i + 1] <= '9') {
            val = val * kDecimalMax + static_cast<uint32>(static_cast<int32>(asmStr[++i]) - kZeroAsciiNum);
          }
          if (val < outOpnds.size()) {
            const char *prefix = list6.stringList[val]->GetComment().c_str();
            RegOperand *opnd = outOpnds[val];
            emitRegister(prefix, opnd->IsOfIntClass(), opnd->GetRegisterNumber(), true);
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
              emitRegister(prefix, opnd->IsOfIntClass(), opnd->GetRegisterNumber(), true);
            }
          }
        } else if (c == '{') {
          c = asmStr[++i];
          CHECK_FATAL(((c >= '0') && (c <= '9')), "Inline asm : invalid register constraint number");
          auto val = static_cast<uint32>(static_cast<unsigned char>(c)) -
                     static_cast<uint32>(static_cast<unsigned char>('0'));
          if (asmStr[i + 1] >= '0' && asmStr[i + 1] <= '9') {
            val = val * kDecimalMax + static_cast<uint32>(static_cast<unsigned char>(asmStr[++i])) -
                  static_cast<uint32>(static_cast<unsigned char>('0'));
          }
          regno_t regno;
          bool isAddr = false;
          if (val < outOpnds.size()) {
            RegOperand *opnd = outOpnds[val];
            regno = opnd->GetRegisterNumber();
            isAddr = isMemAccess(list6.stringList[val]->GetComment().c_str()[0]);
          } else {
            val -= static_cast<uint32>(outOpnds.size());
            CHECK_FATAL(val < inOpnds.size(), "Inline asm : invalid register constraint number");
            RegOperand *opnd = inOpnds[val];
            regno = opnd->GetRegisterNumber();
            isAddr = isMemAccess(list7.stringList[val]->GetComment().c_str()[0]);
          }
          c = asmStr[++i];
          CHECK_FATAL(c == ':', "Parsing error in inline asm string during emit");
          c = asmStr[++i];
          std::string prefix(1, c);
          if (c == 'a' || isAddr) {
            prefix = "[x";
          }
          emitRegister(prefix.c_str(), true, regno, false);
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
  const InsnDesc *md = &AArch64CG::kMd[MOP_clinit_tail];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);

  const OpndDesc *prop0 = md->opndMD[0];
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
  const InsnDesc *md = &AArch64CG::kMd[MOP_lazy_ldr];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  const OpndDesc *prop0 = md->opndMD[0];
  const OpndDesc *prop1 = md->opndMD[1];
  A64OpndEmitVisitor visitor(emitter, prop0);
  A64OpndEmitVisitor visitor1(emitter, prop1);

  /* emit  "ldr wd, [xs]" */
  (void)emitter.Emit("\t").Emit("ldr\t");
#ifdef USE_32BIT_REF
  opnd0->Accept(visitor);
#else
  opnd0->Accept(visitor1);
#endif
  (void)emitter.Emit(", [");
  opnd1->Accept(visitor1);
  (void)emitter.Emit("]\t// lazy load.\n");

  /* emit "ldr wd, [xd]" */
  (void)emitter.Emit("\t").Emit("ldr\t");
  opnd0->Accept(visitor);
  (void)emitter.Emit(", [");
  opnd1->Accept(visitor1);
  (void)emitter.Emit("]\t// lazy load.\n");
}

void AArch64AsmEmitter::EmitCounter(Emitter &emitter, const Insn &insn) const {
  /*
   * adrp    x1, __profile_bb_table$$GetBoolean_dex+4
   * ldr     w17, [x1, #:lo12:__profile_bb_table$$GetBoolean_dex+4]
   * add     w17, w17, #1
   * str     w17, [x1, #:lo12:__profile_bb_table$$GetBoolean_dex+4]
   */
  const InsnDesc *md = &AArch64CG::kMd[MOP_counter];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  const OpndDesc *prop0 = md->opndMD[kInsnFirstOpnd];
  A64OpndEmitVisitor visitor(emitter, prop0);
  StImmOperand *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Emitter::EmitCounter");
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

void AArch64AsmEmitter::EmitCCounter(Emitter &emitter, const Insn &insn) const {
  const InsnDesc *md = &AArch64CG::kMd[MOP_c_counter];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  Operand *opnd2 = &insn.GetOperand(kInsnThirdOpnd);
  const OpndDesc *prop1 = md->opndMD[kInsnSecondOpnd];
  const OpndDesc *prop2 = md->opndMD[kInsnThirdOpnd];
  bool hasFreeReg = static_cast<RegOperand*>(opnd2)->GetRegisterNumber() != R30;

  CHECK_FATAL(static_cast<RegOperand*>(opnd2)->GetRegisterNumber() != R16, "check this case");
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd0);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Insn::EmitCounter");
  /* spill linker register */
  if (!hasFreeReg) {
    (void)emitter.Emit("\tstr\tx30, [sp, #-16]!\n");
  }
  /* get counter symbol address */
  (void)emitter.Emit("\tadrp\tx16, ");
  (void)emitter.Emit(stImmOpnd->GetName()).Emit("\n");

  (void)emitter.Emit("\tadd\tx16, x16, :lo12:");
  (void)emitter.Emit(stImmOpnd->GetName());
  (void)emitter.Emit("\n");

  A64OpndEmitVisitor visitor(emitter, prop1);
  A64OpndEmitVisitor visitor2(emitter, prop2);
  /* load current count */
  if (!hasFreeReg) {
    (void)emitter.Emit("\tldr\tx30, [x16, ");
  } else {
    (void)emitter.Emit("\tldr\t");
    opnd2->Accept(visitor2);
    (void)emitter.Emit(", [x16, ");
  }
  opnd1->Accept(visitor);
  (void)emitter.Emit("]\n");
  /* increment */
  if (!hasFreeReg) {
    (void)emitter.Emit("\tadd\tx30, x30, #1\n");
  } else {
    (void)emitter.Emit("\tadd\t");
    opnd2->Accept(visitor2);
    (void)emitter.Emit(", ");
    opnd2->Accept(visitor2);
    (void)emitter.Emit(", #1\n");
  }
  /* str new count */
  if (!hasFreeReg) {
    (void)emitter.Emit("\tstr\tx30, [x16, ");
  } else {
    (void)emitter.Emit("\tstr\t");
    opnd2->Accept(visitor2);
    (void)emitter.Emit(", [x16, ");
  }
  opnd1->Accept(visitor);
  (void)emitter.Emit("]\n");

  /* reload linker register */
  if (!hasFreeReg) {
    (void)emitter.Emit("\tldr\tx30, [sp], #16\n");
  }
}

void AArch64AsmEmitter::EmitAdrpLabel(Emitter &emitter, const Insn &insn) const {
  /* adrp    xd, label
   * add     xd, xd, #lo12:label
   */
  const InsnDesc *md = &AArch64CG::kMd[MOP_adrp_label];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  const OpndDesc *prop0 = md->opndMD[0];
  A64OpndEmitVisitor visitor(emitter, prop0);
  auto lidx = static_cast<ImmOperand *>(opnd1)->GetValue();

  /* adrp    xd, label */
  (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Accept(visitor);
  (void)emitter.Emit(", ");
  char *idx = strdup(
      std::to_string(Globals::GetInstance()->GetBECommon()->GetMIRModule().CurFunction()->GetPuidx()).c_str());
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
  const InsnDesc *md = &AArch64CG::kMd[MOP_adrp_ldr];
  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  const OpndDesc *prop0 = md->opndMD[0];
  A64OpndEmitVisitor visitor(emitter, prop0);
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Emitter::EmitAdrpLdr");
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
  const InsnDesc *md = &AArch64CG::kMd[MOP_lazy_ldr_static];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  const OpndDesc *prop0 = md->GetOpndDes(0);
  A64OpndEmitVisitor visitor(emitter, prop0);
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Emitter::EmitLazyLoadStatic");

  /* emit "adrp xd, :got:__staticDecoupleValueOffset$$xxx+offset" */
  (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Accept(visitor);
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
  const OpndDesc prop2(prop0->GetOperandType(), prop0->GetRegProp(), prop0->GetSize() / 2);
  opnd0->Emit(emitter, &prop2); /* ldr wd, ... for emui */
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
  (void)emitter.Emit("]\t// lazy load static.\n");

  /* emit "ldr wzr, [xd]" */
  (void)emitter.Emit("\t").Emit("ldr\twzr, [");
  opnd0->Accept(visitor);
  (void)emitter.Emit("]\t// lazy load static.\n");
}

void AArch64AsmEmitter::EmitArrayClassCacheLoad(Emitter &emitter, const Insn &insn) const {
  /* adrp xd, :got:__arrayClassCacheTable$$xxx+offset
   * ldr wd, [xd, #:got_lo12:__arrayClassCacheTable$$xxx+offset]
   * ldr wzr, [xd]
   */
  const InsnDesc *md = &AArch64CG::kMd[MOP_arrayclass_cache_ldr];
  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
  const OpndDesc *prop0 = md->GetOpndDes(kInsnFirstOpnd);
  A64OpndEmitVisitor visitor(emitter, prop0);
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Emitter::EmitLazyLoadStatic");

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
  const OpndDesc prop2(prop0->GetOperandType(), prop0->GetRegProp(), prop0->GetSize() / 2);
  A64OpndEmitVisitor visitor2(emitter, prop2);
  opnd0->Accept(visitor2); /* ldr wd, ... for emui */
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
  const InsnDesc *insnDesc = insn.GetDesc();
  A64OpndEmitVisitor visitor(emitter, nullptr);
  A64OpndEmitVisitor visitor1(emitter, insnDesc->GetOpndDes(kInsnSecondOpnd));
  A64OpndEmitVisitor visitor3(emitter, insnDesc->GetOpndDes(kInsnFourthOpnd));
  A64OpndEmitVisitor visitor4(emitter, insnDesc->GetOpndDes(kInsnFifthOpnd));
  A64OpndEmitVisitor visitor5(emitter, insnDesc->GetOpndDes(kInsnSixthOpnd));

  /* emit add. */
  (void)emitter.Emit("\t").Emit("add").Emit("\t");
  tempOpnd0->Accept(visitor1);
  (void)emitter.Emit(", ");
  objOpnd->Accept(visitor4);
  (void)emitter.Emit(", ");
  offsetOpnd->Accept(visitor5);
  (void)emitter.Emit("\n");
  /* emit label. */
  labelOpnd->Accept(visitor);
  (void)emitter.Emit(":\n");
  Operand *retVal = &insn.GetOperand(kInsnFirstOpnd);
  const MOperator mOp = insn.GetMachineOpcode();
  const InsnDesc *md = &AArch64CG::kMd[mOp];
  const OpndDesc *retProp = md->opndMD[kInsnFirstOpnd];
  A64OpndEmitVisitor retVisitor(emitter, retProp);
  /* emit ldaxr */
  (void)emitter.Emit("\t").Emit("ldaxr").Emit("\t");
  retVal->Accept(retVisitor);
  (void)emitter.Emit(", [");
  tempOpnd0->Accept(visitor1);
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
  tempOpnd2->Accept(visitor3);
  (void)emitter.Emit(", ");
  tempOpnd1->Accept(retVisitor);
  (void)emitter.Emit(", [");
  tempOpnd0->Accept(visitor1);
  (void)emitter.Emit("]\n");
  /* emit cbnz. */
  (void)emitter.Emit("\t").Emit("cbnz").Emit("\t");
  tempOpnd2->Accept(visitor3);
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
  A64OpndEmitVisitor visitor0(emitter, insn.GetDesc()->GetOpndDes(kInsnFirstOpnd));
  A64OpndEmitVisitor visitor1(emitter, insn.GetDesc()->GetOpndDes(kInsnSecondOpnd));
  A64OpndEmitVisitor visitor2(emitter, insn.GetDesc()->GetOpndDes(kInsnThirdOpnd));
  A64OpndEmitVisitor visitor3(emitter, insn.GetDesc()->GetOpndDes(kInsnFourthOpnd));
  A64OpndEmitVisitor visitor4(emitter, insn.GetDesc()->GetOpndDes(kInsnFifthOpnd));
  A64OpndEmitVisitor visitor5(emitter, insn.GetDesc()->GetOpndDes(kInsnSixthOpnd));

  /* add    x1, x1, x2 */
  (void)emitter.Emit("\tadd\t");
  tempOpnd0->Accept(visitor1);
  (void)emitter.Emit(", ");
  objOpnd->Accept(visitor3);
  (void)emitter.Emit(", ");
  offsetOpnd->Accept(visitor4);
  (void)emitter.Emit("\n");
  Operand *labelOpnd = &insn.GetOperand(kInsnSeventhOpnd);
  /* label: */
  labelOpnd->Accept(visitor);
  (void)emitter.Emit(":\n");
  Operand *retVal = &insn.GetOperand(kInsnFirstOpnd);
  /* ldaxr  w0, [xt] */
  (void)emitter.Emit("\tldaxr\t");
  retVal->Accept(visitor0);
  (void)emitter.Emit(", [");
  tempOpnd0->Accept(visitor1);
  (void)emitter.Emit("]\n");
  Operand *newValueOpnd = &insn.GetOperand(kInsnSixthOpnd);
  /* stlxr  ws, w3, [xt] */
  (void)emitter.Emit("\tstlxr\t");
  tempOpnd1->Accept(visitor2);
  (void)emitter.Emit(", ");
  newValueOpnd->Accept(visitor5);
  (void)emitter.Emit(", [");
  tempOpnd0->Accept(visitor1);
  (void)emitter.Emit("]\n");
  /* cbnz   w2, label */
  (void)emitter.Emit("\tcbnz\t");
  tempOpnd1->Accept(visitor2);
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
  const InsnDesc *insnDesc = insn.GetDesc();
  const std::string patternLengthReg =
      AArch64CG::intRegNames[AArch64CG::kR64List][static_cast<RegOperand*>(patternLengthOpnd)->GetRegisterNumber()];
  const std::string srcLengthReg =
      AArch64CG::intRegNames[AArch64CG::kR64List][static_cast<RegOperand*>(srcLengthOpnd)->GetRegisterNumber()];
  A64OpndEmitVisitor visitor(emitter, nullptr);
  A64OpndEmitVisitor visitor4(emitter, insnDesc->GetOpndDes(kInsnFifthOpnd));
  /* cmp       w4, w2 */
  (void)emitter.Emit("\tcmp\t");
  patternLengthOpnd->Accept(visitor4);
  (void)emitter.Emit(", ");
  A64OpndEmitVisitor visitor2(emitter, insnDesc->GetOpndDes(kInsnThirdOpnd));
  srcLengthOpnd->Accept(visitor2);
  (void)emitter.Emit("\n");
  /* the 16th operand of MOP_string_indexof is Label.NOMATCH */
  Operand *labelNoMatch = &insn.GetOperand(16);
  /* b.gt      Label.NOMATCH */
  (void)emitter.Emit("\tb.gt\t");
  labelNoMatch->Accept(visitor);
  (void)emitter.Emit("\n");
  /* sub       w2, w2, w4 */
  (void)emitter.Emit("\tsub\t");
  srcLengthOpnd->Accept(visitor2);
  (void)emitter.Emit(", ");
  srcLengthOpnd->Accept(visitor2);
  (void)emitter.Emit(", ");
  patternLengthOpnd->Accept(visitor4);
  (void)emitter.Emit("\n");
  /* sub       w4, w4, #8 */
  (void)emitter.Emit("\tsub\t");
  patternLengthOpnd->Accept(visitor4);
  (void)emitter.Emit(", ");
  patternLengthOpnd->Accept(visitor4);
  (void)emitter.Emit(", #8\n");
  /* the 10th operand of MOP_string_indexof is w10 */
  Operand *resultTmp = &insn.GetOperand(10);
  A64OpndEmitVisitor visitor10(emitter, insnDesc->GetOpndDes(kEARetTempNameSize));
  /* mov       w10, w2 */
  (void)emitter.Emit("\tmov\t");
  resultTmp->Accept(visitor10);
  (void)emitter.Emit(", ");
  srcLengthOpnd->Accept(visitor2);
  (void)emitter.Emit("\n");
  /* uxtw      x4, w4 */
  (void)emitter.Emit("\tuxtw\t").Emit(patternLengthReg);
  (void)emitter.Emit(", ");
  patternLengthOpnd->Accept(visitor4);
  (void)emitter.Emit("\n");
  /* uxtw      x2, w2 */
  (void)emitter.Emit("\tuxtw\t").Emit(srcLengthReg);
  (void)emitter.Emit(", ");
  srcLengthOpnd->Accept(visitor2);
  (void)emitter.Emit("\n");
  Operand *patternStringBaseOpnd = &insn.GetOperand(kInsnFourthOpnd);
  A64OpndEmitVisitor visitor3(emitter, insnDesc->GetOpndDes(kInsnFourthOpnd));
  /* add       x3, x3, x4 */
  (void)emitter.Emit("\tadd\t");
  patternStringBaseOpnd->Accept(visitor3);
  (void)emitter.Emit(", ");
  patternStringBaseOpnd->Accept(visitor3);
  (void)emitter.Emit(", ").Emit(patternLengthReg);
  (void)emitter.Emit("\n");
  Operand *srcStringBaseOpnd = &insn.GetOperand(kInsnSecondOpnd);
  A64OpndEmitVisitor visitor1(emitter, insnDesc->GetOpndDes(kInsnSecondOpnd));
  /* add       x1, x1, x2 */
  (void)emitter.Emit("\tadd\t");
  srcStringBaseOpnd->Accept(visitor1);
  (void)emitter.Emit(", ");
  srcStringBaseOpnd->Accept(visitor1);
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
  A64OpndEmitVisitor visitor5(emitter, insnDesc->GetOpndDes(kInsnSixthOpnd));
  /* ldr       x5, [x3,x4] */
  (void)emitter.Emit("\tldr\t");
  first->Accept(visitor5);
  (void)emitter.Emit(", [");
  patternStringBaseOpnd->Accept(visitor3);
  (void)emitter.Emit(",").Emit(patternLengthReg);
  (void)emitter.Emit("]\n");
  /* the 11th operand of MOP_string_indexof is Label.FIRST_LOOP */
  Operand *labelFirstLoop = &insn.GetOperand(11);
  /* .Label.FIRST_LOOP: */
  labelFirstLoop->Accept(visitor);
  (void)emitter.Emit(":\n");
  /* the 7th operand of MOP_string_indexof is x7 */
  Operand *ch2 = &insn.GetOperand(7);
  A64OpndEmitVisitor visitor7(emitter, insnDesc->GetOpndDes(kInsnEighthOpnd));
  /* ldr       x7, [x1,x2] */
  (void)emitter.Emit("\tldr\t");
  ch2->Accept(visitor7);
  (void)emitter.Emit(", [");
  srcStringBaseOpnd->Accept(visitor1);
  (void)emitter.Emit(",").Emit(srcLengthReg);
  (void)emitter.Emit("]\n");
  /* cmp       x5, x7 */
  (void)emitter.Emit("\tcmp\t");
  first->Accept(visitor5);
  (void)emitter.Emit(", ");
  ch2->Accept(visitor7);
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
  tmp1->Accept(visitor7);
  (void)emitter.Emit(", ").Emit(patternLengthReg);
  (void)emitter.Emit(", #8\n");
  /* the 9th operand of MOP_string_indexof is x9 */
  Operand *tmp2 = &insn.GetOperand(9);
  A64OpndEmitVisitor visitor9(emitter, insnDesc->GetOpndDes(k9ByteSize));
  /* add       x9, x2, #8 */
  (void)emitter.Emit("\tadd\t");
  tmp2->Accept(visitor9);
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
  A64OpndEmitVisitor visitor6(emitter, insnDesc->GetOpndDes(kInsnSeventhOpnd));
  /* ldr       x6, [x3,x8] */
  (void)emitter.Emit("\tldr\t");
  ch1->Accept(visitor6);
  (void)emitter.Emit(", [");
  patternStringBaseOpnd->Accept(visitor3);
  (void)emitter.Emit(",");
  tmp1->Accept(visitor7);
  (void)emitter.Emit("]\n");
  /* ldr       x7, [x1,x9] */
  (void)emitter.Emit("\tldr\t");
  ch2->Accept(visitor7);
  (void)emitter.Emit(", [");
  srcStringBaseOpnd->Accept(visitor1);
  (void)emitter.Emit(",");
  tmp2->Accept(visitor9);
  (void)emitter.Emit("]\n");
  /* cmp       x6, x7 */
  (void)emitter.Emit("\tcmp\t");
  ch1->Accept(visitor6);
  (void)emitter.Emit(", ");
  ch2->Accept(visitor7);
  (void)emitter.Emit("\n");
  /* b.ne      .Label.STR2_NEXT */
  (void)emitter.Emit("\tb.ne\t");
  labelStr2Next->Accept(visitor);
  (void)emitter.Emit("\n");
  /* adds      x8, x8, #8 */
  (void)emitter.Emit("\tadds\t");
  tmp1->Accept(visitor7);
  (void)emitter.Emit(", ");
  tmp1->Accept(visitor7);
  (void)emitter.Emit(", #8\n");
  /* add       x9, x9, #8 */
  (void)emitter.Emit("\tadd\t");
  tmp2->Accept(visitor9);
  (void)emitter.Emit(", ");
  tmp2->Accept(visitor9);
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
  ch1->Accept(visitor6);
  (void)emitter.Emit(", [");
  patternStringBaseOpnd->Accept(visitor3);
  (void)emitter.Emit("]\n");
  /* sub       x9, x1, x4 */
  (void)emitter.Emit("\tsub\t");
  tmp2->Accept(visitor9);
  (void)emitter.Emit(", ");
  srcStringBaseOpnd->Accept(visitor1);
  (void)emitter.Emit(", ").Emit(patternLengthReg);
  (void)emitter.Emit("\n");
  /* ldr       x7, [x9,x2] */
  (void)emitter.Emit("\tldr\t");
  ch2->Accept(visitor7);
  (void)emitter.Emit(", [");
  tmp2->Accept(visitor9);
  (void)emitter.Emit(", ").Emit(srcLengthReg);
  (void)emitter.Emit("]\n");
  /* cmp       x6, x7 */
  (void)emitter.Emit("\tcmp\t");
  ch1->Accept(visitor6);
  (void)emitter.Emit(", ");
  ch2->Accept(visitor7);
  (void)emitter.Emit("\n");
  /* b.ne      .Label.STR2_NEXT */
  (void)emitter.Emit("\tb.ne\t");
  labelStr2Next->Accept(visitor);
  (void)emitter.Emit("\n");
  Operand *retVal = &insn.GetOperand(kInsnFirstOpnd);
  A64OpndEmitVisitor visitor0(emitter, insnDesc->GetOpndDes(kInsnFirstOpnd));
  /* add       w0, w10, w2 */
  (void)emitter.Emit("\tadd\t");
  retVal->Accept(visitor0);
  (void)emitter.Emit(", ");
  resultTmp->Accept(visitor10);
  (void)emitter.Emit(", ");
  srcLengthOpnd->Accept(visitor2);
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
  retVal->Accept(visitor0);
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
  const InsnDesc *md = &AArch64CG::kMd[mOp];
  Operand *temp0 = &insn.GetOperand(kInsnSecondOpnd);
  Operand *temp1 = &insn.GetOperand(kInsnThirdOpnd);
  Operand *obj = &insn.GetOperand(kInsnFourthOpnd);
  Operand *offset = &insn.GetOperand(kInsnFifthOpnd);
  A64OpndEmitVisitor visitor(emitter, nullptr);
  A64OpndEmitVisitor visitor1(emitter, md->GetOpndDes(kInsnSecondOpnd));
  A64OpndEmitVisitor visitor2(emitter, md->GetOpndDes(kInsnThirdOpnd));
  A64OpndEmitVisitor visitor3(emitter, md->GetOpndDes(kInsnFourthOpnd));
  A64OpndEmitVisitor visitor4(emitter, md->GetOpndDes(kInsnFifthOpnd));
  A64OpndEmitVisitor visitor6(emitter, md->GetOpndDes(kInsnSeventhOpnd));

  /* add       xt, x1, x2 */
  (void)emitter.Emit("\tadd\t");
  temp0->Accept(visitor1);
  (void)emitter.Emit(", ");
  obj->Accept(visitor3);
  (void)emitter.Emit(", ");
  offset->Accept(visitor4);
  (void)emitter.Emit("\n");
  Operand *label1 = &insn.GetOperand(kInsnEighthOpnd);
  /* label1: */
  label1->Accept(visitor);
  (void)emitter.Emit(":\n");
  /* ldaxr     ws, [xt] */
  (void)emitter.Emit("\tldaxr\t");
  temp1->Accept(visitor2);
  (void)emitter.Emit(", [");
  temp0->Accept(visitor1);
  (void)emitter.Emit("]\n");
  Operand *expectedValue = &insn.GetOperand(kInsnSixthOpnd);
  const OpndDesc *expectedValueProp = md->opndMD[kInsnSixthOpnd];
  /* cmp       ws, w3 */
  (void)emitter.Emit("\tcmp\t");
  temp1->Accept(visitor2);
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
  newValue->Accept(visitor6);
  (void)emitter.Emit(", [");
  temp0->Accept(visitor1);
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
  A64OpndEmitVisitor visitor0(emitter, md->GetOpndDes(kInsnFirstOpnd));
  /* cset      x0, eq */
  (void)emitter.Emit("\tcset\t");
  retVal->Accept(visitor0);
  (void)emitter.Emit(", EQ\n");
}

void AArch64AsmEmitter::EmitCTlsDescRel(Emitter &emitter, const Insn &insn) const {
  const InsnDesc *md = &AArch64CG::kMd[MOP_tls_desc_rel];
  Operand *result = &insn.GetOperand(kInsnFirstOpnd);
  Operand *src = &insn.GetOperand(kInsnSecondOpnd);
  Operand *symbol = &insn.GetOperand(kInsnThirdOpnd);
  auto stImmOpnd = static_cast<StImmOperand*>(symbol);
  std::string symName = stImmOpnd->GetName();
  symName += stImmOpnd->GetSymbol()->GetStorageClass() == kScPstatic ?
      std::to_string(emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx()) : "";
  A64OpndEmitVisitor resultVisitor(emitter, md->opndMD[0]);
  A64OpndEmitVisitor srcVisitor(emitter, md->opndMD[1]);
  (void)emitter.Emit("\t").Emit("add").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", ");
  src->Accept(srcVisitor);
  (void)emitter.Emit(", #:tprel_hi12:").Emit(symName).Emit(", lsl #12\n");
  (void)emitter.Emit("\t").Emit("add").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", ");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", #:tprel_lo12_nc:").Emit(symName).Emit("\n");
}
void AArch64AsmEmitter::EmitCTlsDescCall(Emitter &emitter, const Insn &insn) const {
  const InsnDesc *md = &AArch64CG::kMd[MOP_tls_desc_call];
  Operand *func = &insn.GetOperand(kInsnSecondOpnd);
  Operand *symbol = &insn.GetOperand(kInsnThirdOpnd);
  const OpndDesc *prop = md->opndMD[kInsnSecondOpnd];
  auto *stImmOpnd = static_cast<StImmOperand*>(symbol);
  std::string symName = stImmOpnd->GetName();
  symName += stImmOpnd->GetSymbol()->GetStorageClass() == kScPstatic ?
      std::to_string(emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx()) : "";
  A64OpndEmitVisitor funcVisitor(emitter, prop);
  /*  adrp    x0, :tlsdesc:symbol */
  (void)emitter.Emit("\t").Emit("adrp\tx0, :tlsdesc:").Emit(symName).Emit("\n");
  /*  ldr x1, [x0, #tlsdesc_lo12:symbol] */
  (void)emitter.Emit("\t").Emit("ldr").Emit("\t");
  func->Accept(funcVisitor);
  (void)emitter.Emit(", [x0, #:tlsdesc_lo12:").Emit(symName).Emit("]\n");
  /*  add x0 ,#tlsdesc_lo12:symbol */
  (void)emitter.Emit("\t").Emit("add\tx0, x0, :tlsdesc_lo12:").Emit(symName).Emit("\n");
  /* .tlsdesccall <symbolName> */
  (void)emitter.Emit("\t").Emit(".tlsdesccall").Emit("\t").Emit(symName).Emit("\n");
  /* blr xd */
  (void)emitter.Emit("\t").Emit("blr").Emit("\t");
  func->Accept(funcVisitor);
  (void)emitter.Emit("\n");
}

void AArch64AsmEmitter::EmitCTlsDescCallWarmup(Emitter &emitter, const Insn &insn) const {
  const InsnDesc *md = &AArch64CG::kMd[MOP_tls_desc_call_warmup];
  Operand *result = &insn.GetOperand(kInsnFirstOpnd);
  const OpndDesc *resultProp = md->opndMD[kInsnFirstOpnd];
  Operand *reg = &insn.GetOperand(kInsnSecondOpnd);
  const OpndDesc *regProp = md->opndMD[kInsnSecondOpnd];
  Operand *symbol = &insn.GetOperand(kInsnThirdOpnd);
  auto *stImmOpnd = static_cast<StImmOperand*>(symbol);
  std::string symName = stImmOpnd->GetName();
  symName += stImmOpnd->GetSymbol()->GetStorageClass() == kScPstatic ?
      std::to_string(emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx()) : "";
  A64OpndEmitVisitor resultVisitor(emitter, resultProp);
  A64OpndEmitVisitor regVisitor(emitter, regProp);
  // adrp    result, :tlsdesc:symbol
  (void)emitter.Emit("\t").Emit("adrp\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", :tlsdesc:").Emit(symName).Emit("\n");
  // ldr reg, [result, #tlsdesc_lo12:symbol]
  (void)emitter.Emit("\t").Emit("ldr").Emit("\t");
  reg->Accept(regVisitor);
  (void)emitter.Emit(", [");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", #:tlsdesc_lo12:").Emit(symName).Emit("]\n");
  // add result ,#tlsdesc_lo12:symbol
  (void)emitter.Emit("\t").Emit("add\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", ");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", :tlsdesc_lo12:").Emit(symName).Emit("\n");
  // .tlsdesccall <symbolName>
  (void)emitter.Emit("\t").Emit(".tlsdesccall").Emit("\t").Emit(symName).Emit("\n");
  // placeholder
  // may not be needed if TLS image is bigger than reserved space
  (void)emitter.Emit("\t").Emit("nop").Emit("\n");
}

void AArch64AsmEmitter::EmitCTlsLoadTdata(Emitter &emitter, const Insn &insn) const {
  const InsnDesc *md = &AArch64CG::kMd[MOP_tlsload_tdata];
  Operand *result = &insn.GetOperand(kInsnFirstOpnd);
  A64OpndEmitVisitor resultVisitor(emitter, md->opndMD[0]);
  (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", :got:tdata_addr_" + GetCG()->GetMIRModule()->GetTlsAnchorHashString() + "\n");
  (void)emitter.Emit("\t").Emit("ldr").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", [");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", #:got_lo12:tdata_addr_" + GetCG()->GetMIRModule()->GetTlsAnchorHashString() + "]\n");
  (void)emitter.Emit("\t").Emit("ldr").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", [");
  result->Accept(resultVisitor);
  (void)emitter.Emit("]\n");
}

void AArch64AsmEmitter::EmitCTlsLoadTbss(Emitter &emitter, const Insn &insn) const {
  const InsnDesc *md = &AArch64CG::kMd[MOP_tlsload_tbss];
  Operand *result = &insn.GetOperand(kInsnFirstOpnd);
  A64OpndEmitVisitor resultVisitor(emitter, md->opndMD[0]);
  (void)emitter.Emit("\t").Emit("adrp").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", :got:tbss_addr_" + GetCG()->GetMIRModule()->GetTlsAnchorHashString() + "\n");
  (void)emitter.Emit("\t").Emit("ldr").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", [");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", #:got_lo12:tbss_addr_" + GetCG()->GetMIRModule()->GetTlsAnchorHashString() + "]\n");
  (void)emitter.Emit("\t").Emit("ldr").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", [");
  result->Accept(resultVisitor);
  (void)emitter.Emit("]\n");
}

void AArch64AsmEmitter::EmitCTlsDescGot(Emitter &emitter, const Insn &insn) const {
  const InsnDesc *md = &AArch64CG::kMd[MOP_tls_desc_got];
  Operand *result = &insn.GetOperand(kInsnFirstOpnd);
  Operand *symbol = &insn.GetOperand(kInsnSecondOpnd);
  auto stImmOpnd = static_cast<StImmOperand*>(symbol);
  std::string symName = stImmOpnd->GetName();
  symName += stImmOpnd->GetSymbol()->GetStorageClass() == kScPstatic ?
             std::to_string(emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx()) : "";
  A64OpndEmitVisitor resultVisitor(emitter, md->opndMD[0]);
  emitter.Emit("\t").Emit("adrp").Emit("\t");
  result->Accept(resultVisitor);
  emitter.Emit(", :gottprel:").Emit(symName).Emit("\n");
  emitter.Emit("\t").Emit("ldr").Emit("\t");
  result->Accept(resultVisitor);
  emitter.Emit(", [");
  result->Accept(resultVisitor);
  emitter.Emit(", #:gottprel_lo12:").Emit(symName).Emit("]\n");
}

void AArch64AsmEmitter::EmitSyncLockTestSet(Emitter &emitter, const Insn &insn) const {
  const InsnDesc *md = &AArch64CG::kMd[insn.GetMachineOpcode()];
  auto *result = &insn.GetOperand(kInsnFirstOpnd);
  auto *temp = &insn.GetOperand(kInsnSecondOpnd);
  auto *addr = &insn.GetOperand(kInsnThirdOpnd);
  auto *value = &insn.GetOperand(kInsnFourthOpnd);
  auto *label = &insn.GetOperand(kInsnFifthOpnd);
  A64OpndEmitVisitor resultVisitor(emitter, md->opndMD[kInsnFirstOpnd]);
  A64OpndEmitVisitor tempVisitor(emitter, md->opndMD[kInsnSecondOpnd]);
  A64OpndEmitVisitor addrVisitor(emitter, md->opndMD[kInsnThirdOpnd]);
  A64OpndEmitVisitor valueVisitor(emitter, md->opndMD[kInsnFourthOpnd]);
  A64OpndEmitVisitor labelVisitor(emitter, md->opndMD[kInsnFifthOpnd]);
  /* label: */
  label->Accept(labelVisitor);
  (void)emitter.Emit(":\n");
  /* ldxr x0, [x2] */
  (void)emitter.Emit("\t").Emit("ldxr").Emit("\t");
  result->Accept(resultVisitor);
  (void)emitter.Emit(", [");
  addr->Accept(addrVisitor);
  (void)emitter.Emit("]\n");
  /* stxr w1, x3, [x2] */
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
  /* dmb ish */
  (void)emitter.Emit("\t").Emit("dmb").Emit("\t").Emit("ish").Emit("\n");
}

void AArch64AsmEmitter::EmitCheckThrowPendingException(Emitter &emitter) const {
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
  const InsnDesc *md = &AArch64CG::kMd[MOP_adrp_ldr];

  Operand *opnd0 = &insn.GetOperand(kInsnFirstOpnd);
  const OpndDesc *prop0 = md->opndMD[0];
  A64OpndEmitVisitor visitor(emitter, prop0);

  /* emit "ldr  xzr,[xs]" */
#ifdef USE_32BIT_REF
  (void)emitter.Emit("\t").Emit("ldr").Emit("\twzr, [");
#else
  (void)emitter.Emit("\t").Emit("ldr").Emit("\txzr, [");
#endif /* USE_32BIT_REF */
  opnd0->Accept(visitor);
  (void)emitter.Emit("]");
  (void)emitter.Emit("\t// Lazy binding\n");
}

void AArch64AsmEmitter::PrepareVectorOperand(RegOperand *regOpnd, uint32 &compositeOpnds, Insn &insn) const {
  auto *vecSpec = insn.GetAndRemoveRegSpecFromList();
  compositeOpnds = (vecSpec->compositeOpnds > 0) ? vecSpec->compositeOpnds : compositeOpnds;
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

void AArch64AsmEmitter::EmitAArch64CfiInsn(Emitter &emitter, const Insn &insn) const {
  MOperator mOp = insn.GetMachineOpcode();
  CfiDescr &cfiDescr = cfiDescrTable[mOp];
  (void)emitter.Emit("\t").Emit(cfiDescr.name);
  for (uint32 i = 0; i < cfiDescr.opndCount; ++i) {
    (void)emitter.Emit(" ");
    Operand &curOperand = insn.GetOperand(i);
    cfi::CFIOpndEmitVisitor cfiOpndEmitVisitor(emitter);
    curOperand.Accept(cfiOpndEmitVisitor);
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

void AArch64AsmEmitter::EmitAArch64DbgInsn(FuncEmitInfo &funcEmitInfo, Emitter &emitter, const Insn &insn) const {
  MOperator mOp = insn.GetMachineOpcode();
  DbgDescr &dbgDescr = dbgDescrTable[mOp];
  switch (mOp) {
    case mpldbg::OP_DBG_scope: {
      unsigned scopeId = static_cast<unsigned>(static_cast<ImmOperand&>(insn.GetOperand(0)).GetValue());
      (void)emitter.Emit(".LScp." + std::to_string(scopeId));
      unsigned val = static_cast<unsigned>(static_cast<ImmOperand&>(insn.GetOperand(1)).GetValue());
      (void)emitter.Emit((val == 0) ? "B:" : "E:");

      CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
      MIRFunction &mirFunc = cgFunc.GetFunction();
      EmitStatus status = (val == 0) ? kBeginEmited : kEndEmited;
      cgFunc.GetCG()->GetMIRModule()->GetDbgInfo()->SetFuncScopeIdStatus(&mirFunc, scopeId, status);
      break;
    }
    default: {
      (void)emitter.Emit("\t.").Emit(dbgDescr.name);
      for (uint32 i = 0; i < dbgDescr.opndCount; ++i) {
        (void)emitter.Emit(" ");
        Operand &curOperand = insn.GetOperand(i);
        mpldbg::DBGOpndEmitVisitor dbgOpndEmitVisitor(emitter);
        curOperand.Accept(dbgOpndEmitVisitor);
      }
      break;
    }
  }
  (void)emitter.Emit("\n");
}

void AArch64AsmEmitter::EmitPrefetch(Emitter &emitter, const Insn &insn) const {
  constexpr int32 rwLimit = 2; // 2: the value of opnd1 cannot exceed 2
  constexpr int32 localityLimit = 4; // 4: the value of opnd2 cannot exceed 4
  static const std::string PRFOP[rwLimit][localityLimit] = {{"pldl1strm", "pldl3keep", "pldl2keep", "pldl1keep"},
                                                            {"pstl1strm", "pstl3keep", "pstl2keep", "pstl1keep"}};
  const InsnDesc *md = &AArch64CG::kMd[insn.GetMachineOpcode()];
  auto *addr = &insn.GetOperand(kInsnFirstOpnd);
  auto *rw = &insn.GetOperand(kInsnSecondOpnd);
  auto *locality = &insn.GetOperand(kInsnThirdOpnd);
  A64OpndEmitVisitor addrVisitor(emitter, md->opndMD[kInsnFirstOpnd]);
  int64 rwConstVal = static_cast<ImmOperand*>(rw)->GetValue();
  int64 localityConstVal = static_cast<ImmOperand*>(locality)->GetValue();
  CHECK_FATAL((rwConstVal < rwLimit) && (rwConstVal >= 0) &&
              (localityConstVal < localityLimit) && (localityConstVal >= 0), "wrong opnd");
  (void)emitter.Emit("\tprfm\t").Emit(PRFOP[rwConstVal][localityConstVal]).Emit(", [");
  addr->Accept(addrVisitor);
  (void)emitter.Emit("]\n");
}

bool AArch64AsmEmitter::CheckInsnRefField(const Insn &insn, uint32 opndIndex) const {
  if (insn.IsAccessRefField() && insn.AccessMem()) {
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
