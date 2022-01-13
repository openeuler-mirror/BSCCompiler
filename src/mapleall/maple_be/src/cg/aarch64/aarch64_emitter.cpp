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
  emitter.Emit("\t.word " + methodDescLabel + "-.\n");
  emitter.IncreaseJavaInsnCount();
}

void AArch64AsmEmitter::EmitRefToMethodInfo(FuncEmitInfo &funcEmitInfo, Emitter &emitter) {
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  if (cgFunc.GetFunction().GetModule()->IsJavaModule()) {
    std::string labelName = ".Label.name." + cgFunc.GetFunction().GetName();
    emitter.Emit("\t.word " + labelName + " - .\n");
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
  emitter.Emit("\t.section\t.rodata\n");
  emitter.Emit("\t.align\t2\n");
  std::string methodInfoLabel;
  GetMethodLabel(cgFunc.GetFunction().GetName(), methodInfoLabel);
  emitter.Emit(methodInfoLabel + ":\n");
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
  emitter.Emit("\t.short ").Emit(refOffset).Emit("\n");
  emitter.Emit("\t.short ").Emit(refNum).Emit("\n");
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
  ehFunc->GetLSDACallSiteTable()->SortCallSiteTable([&aarchCGFunc](LSDACallSite *a, LSDACallSite *b) {
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
  const char *puIdx = strdup(std::to_string(pIdx).c_str());
  const std::string &labelName = cgFunc.GetFunction().GetLabelTab()->GetName(labIdx);
  if (currCG->GenerateVerboseCG()) {
    emitter.Emit(".L.").Emit(puIdx).Emit("__").Emit(labIdx).Emit(":\t//label order ").Emit(label.GetLabelOrder());
    if (!labelName.empty() && labelName.at(0) != '@') {
      /* If label name has @ as its first char, it is not from MIR */
      emitter.Emit(", MIR: @").Emit(labelName).Emit("\n");
    } else {
      emitter.Emit("\n");
    }
  } else {
    emitter.Emit(".L.").Emit(puIdx).Emit("__").Emit(labIdx).Emit(":\n");
  }
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
      for (uint32 i = 0; i < opndNum; ++i) {
        if (insn->GetMachineOpcode() == MOP_asm) {
          if (i == kAsmInputListOpnd || i == kAsmOutputListOpnd || i == kAsmClobberListOpnd) {
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
          auto &listOpnd = static_cast<ListOperand&>(opnd);
          for (auto op : listOpnd.GetOperands()) {
            referedRegs.insert(op->GetRegisterNumber());
          }
        } else if (opnd.IsMemoryAccessOperand()) {
          auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
          RegOperand *base = memOpnd.GetBaseRegister();
          RegOperand *offset = memOpnd.GetIndexRegister();
          if (base != nullptr) {
            referedRegs.insert(base->GetRegisterNumber());
          }
          if (offset != nullptr) {
            referedRegs.insert(offset->GetRegisterNumber());
          }
        } else if (opnd.IsRegister()) {
          RegType regType = static_cast<RegOperand&>(opnd).GetRegisterType();
          if (regType == kRegTyCc || regType == kRegTyVary) {
            continue;
          }
          referedRegs.insert(static_cast<RegOperand&>(opnd).GetRegisterNumber());
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
  emitter.Emit("\n");
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
  } else {
    (void)emitter.Emit("\t.text\n");
  }
  (void)emitter.Emit("\t.align 5\n");
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
            emitter.Emit(contend + "\n");
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
    bool isExternFunction = false;
  /* should refer to function attribute */
    isExternFunction = kJniNativeFuncList.find(funcStName) != kJniNativeFuncList.end();
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
      emitter.Emit("#    freq:").Emit(bb->GetFrequency()).Emit("\n");
    }
    /* emit bb headers */
    if (bb->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
      if (aarchCGFunc.GetMirModule().IsCModule() && bb->IsBBNeedAlign() && bb->GetAlignNopNum() != kAlignMovedFlag) {
        uint32 power = bb->GetAlignPower();
        emitter.Emit("\t.p2align ").Emit(power).Emit("\n");
      }
      EmitBBHeaderLabel(funcEmitInfo, funcName, bb->GetLabIdx());
    }

    FOR_BB_INSNS(insn, bb) {
      insn->Emit(*currCG, emitter);
    }
  }
  if (CGOptions::IsMapleLinker()) {
    /* Emit a label for calculating method size */
    (void)emitter.Emit(".Label.end." + funcStName + ":\n");
  }
  (void)emitter.Emit("\t.size\t" + funcStName + ", .-").Emit(funcStName + "\n");

  EHFunc *ehFunc = cgFunc.GetEHFunc();
  /* emit LSDA */
  if (ehFunc != nullptr) {
    if (!cgFunc.GetHasProEpilogue()) {
      emitter.Emit("\t.word 0x55555555\n");
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
  uint32 size = cgFunc.GetFunction().GetSymTab()->GetSymbolTableSize();
  for (size_t i = 0; i < size; ++i) {
    MIRSymbol *st = cgFunc.GetFunction().GetSymTab()->GetSymbolFromStIdx(i);
    if (st == nullptr) {
      continue;
    }
    MIRStorageClass storageClass = st->GetStorageClass();
    MIRSymKind symKind = st->GetSKind();
    if (storageClass == kScPstatic && symKind == kStConst) {
      emitter.Emit("\t.align 3\n" + st->GetName() + ":\n");
      if (st->GetKonst()->GetKind() == kConstStr16Const) {
        MIRStr16Const *str16Const = safe_cast<MIRStr16Const>(st->GetKonst());
        emitter.EmitStr16Constant(*str16Const);
        emitter.Emit("\n");
        continue;
      }
      if (st->GetKonst()->GetKind() == kConstStrConst) {
        MIRStrConst *strConst = safe_cast<MIRStrConst>(st->GetKonst());
        emitter.EmitStrConstant(*strConst);
        emitter.Emit("\n");
        continue;
      }

      switch (st->GetKonst()->GetType().GetPrimType()) {
        case PTY_u32: {
          MIRIntConst *intConst = safe_cast<MIRIntConst>(st->GetKonst());
          emitter.Emit("\t.long ").Emit(static_cast<uint32>(intConst->GetValue())).Emit("\n");
          emitter.IncreaseJavaInsnCount();
          break;
        }
        case PTY_f32: {
          MIRFloatConst *floatConst = safe_cast<MIRFloatConst>(st->GetKonst());
          emitter.Emit("\t.word ").Emit(static_cast<uint32>(floatConst->GetIntValue())).Emit("\n");
          emitter.IncreaseJavaInsnCount();
          break;
        }
        case PTY_f64: {
          MIRDoubleConst *doubleConst = safe_cast<MIRDoubleConst>(st->GetKonst());
          auto emitF64 = [&](int64 first, int64 second) {
            emitter.Emit("\t.word ").Emit(first).Emit("\n");
            emitter.IncreaseJavaInsnCount();
            emitter.Emit("\t.word ").Emit(second).Emit("\n");
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
    emitter.Emit("\n");
    emitter.Emit("\t.align 3\n");
    emitter.IncreaseJavaInsnCount(0, true); /* just aligned */
    emitter.Emit(st->GetName() + ":\n");
    MIRAggConst *arrayConst = safe_cast<MIRAggConst>(st->GetKonst());
    CHECK_FATAL(arrayConst != nullptr, "null ptr check");
    PUIdx pIdx = cgFunc.GetMirModule().CurFunction()->GetPuidx();
    const std::string &idx = strdup(std::to_string(pIdx).c_str());
    for (size_t i = 0; i < arrayConst->GetConstVec().size(); i++) {
      MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));
      CHECK_FATAL(lblConst != nullptr, "null ptr check");
      (void)emitter.Emit("\t.quad\t.L." + idx).Emit("__").Emit(lblConst->GetValue());
      (void)emitter.Emit(" - " + st->GetName() + "\n");
      emitter.IncreaseJavaInsnCount(kQuadInsnCount);
    }
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
          emitter.Emit(contend + "\n");
        }
      }
    }
    emitter.IncreaseJavaInsnCount(kBinSearchInsnCount);
  }

  for (const auto &mpPair : cgFunc.GetLabelAndValueMap()) {
    LabelOperand &labelOpnd = aarchCGFunc.GetOrCreateLabelOperand(mpPair.first);
    labelOpnd.Emit(emitter, nullptr);
    emitter.Emit(":\n");
    emitter.Emit("\t.quad ").Emit(mpPair.second).Emit("\n");
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
}  /* namespace maplebe */
