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
#include "aarch64_rce.h"

namespace maplebe {
void AArch64RedundantComputeElim::Run() {
  FOR_ALL_BB(bb, cgFunc) {
    if (bb->IsUnreachable()) {
      continue;
    }
    bool opt;
    kGcount = 0;
    do {
      /* reset hashSeed and hashSet */
      g_hashSeed = 0;
      candidates.clear();
      opt = DoOpt(bb);
      ++kGcount;
    } while (opt);
  }
  if (CG_RCE_DUMP) {
    DumpHash();
  }
}

bool AArch64RedundantComputeElim::CheckFakeOptmization(const Insn &existInsn) const {
  /* insns such as {movrr & zxt/sxt & ...} are optimized by prop */
  MOperator mop = existInsn.GetMachineOpcode();
  if (mop == MOP_wmovrr || mop == MOP_xmovrr || (mop >= MOP_xsxtb32 && mop <= MOP_xuxtw64)) {
    return false;
  }
  /* patterns such as {movz ... movk ...} are optimized by LoadFloatPointPattern in cgprepeephole */
  if (mop == MOP_wmovzri16 || mop == MOP_xmovzri16) {
    auto &dstOpnd = static_cast<RegOperand&>(existInsn.GetOperand(kInsnFirstOpnd));
    VRegVersion *dstVersion = ssaInfo->FindSSAVersion(dstOpnd.GetRegisterNumber());
    ASSERT(dstVersion, "get ssa version failed");
    for (auto useIt : dstVersion->GetAllUseInsns()) {
      ASSERT(useIt.second, "get DUInsnInfo failed");
      Insn *useInsn = useIt.second->GetInsn();
      ASSERT(useInsn, "get useInsn by ssaVersion failed");
      if (useInsn->GetMachineOpcode() == MOP_wmovkri16 || useInsn->GetMachineOpcode() == MOP_xmovkri16) {
        return false;
      }
    }
  }
  return true;
}

void AArch64RedundantComputeElim::CheckCondition(const Insn &existInsn, const Insn &curInsn) {
  if (!CheckFakeOptmization(existInsn)) {
    doOpt = false;
    return;
  }
  RegOperand *existDefOpnd = nullptr;
  RegOperand *curDefOpnd = nullptr;
  /*
   * the case as following is 'fake' redundancy opportunity:
   *                              [BB8]       --------------------
   *                     phi: R138, (R136<7>, R146<12>)          |
   *                     phi: R139, (R131<7>, R147<12>)          |
   *                         /                \                  |
   *                        /                  \                 |
   *                      [BB9]  ----------- [BB12]              |
   *                                      sub R146, R139, #1     |
   *                                      sub R147, R139, #1     |
   *                                      cbnz R147, label    ----
   */
  uint32 opndNum = existInsn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    if (existInsn.OpndIsDef(i) && existInsn.OpndIsUse(i) && curInsn.OpndIsDef(i) && curInsn.OpndIsUse(i)) {
      doOpt = true;
      return;
    }
    if (!existInsn.GetOperand(i).IsRegister()) {
      CHECK_FATAL(!curInsn.GetOperand(i).IsRegister(), "check the two insns");
      continue;
    }
    if (existInsn.OpndIsDef(i) && curInsn.OpndIsDef(i)) {
      existDefOpnd = &static_cast<RegOperand&>(existInsn.GetOperand(i));
      curDefOpnd = &static_cast<RegOperand&>(curInsn.GetOperand(i));
      /* def points without use are not processed */
      VRegVersion *existDefVer = ssaInfo->FindSSAVersion(existDefOpnd->GetRegisterNumber());
      VRegVersion *curDefVer = ssaInfo->FindSSAVersion(curDefOpnd->GetRegisterNumber());
      CHECK_FATAL(existDefVer && curDefVer, "get ssa version failed");
      if (existDefVer->GetAllUseInsns().empty() || curDefVer->GetAllUseInsns().empty()) {
        doOpt = false;
        return;
      }
    }
    if (existInsn.OpndIsUse(i) && curInsn.OpndIsUse(i)) {
      auto &existUseOpnd = static_cast<RegOperand&>(existInsn.GetOperand(i));
      Insn *predInsn = ssaInfo->GetDefInsn(existUseOpnd);
      if (predInsn != nullptr && predInsn->IsPhi()) {
        auto &phiOpnd = static_cast<const PhiOperand&>(predInsn->GetOperand(kInsnSecondOpnd));
        for (auto &phiListIt : phiOpnd.GetOperands()) {
          if (phiListIt.second == nullptr) {
            continue;
          }
          regno_t phiRegNO = phiListIt.second->GetRegisterNumber();
          CHECK_FATAL(existDefOpnd && curDefOpnd, "check the def of this insn");
          if (phiRegNO == existDefOpnd->GetRegisterNumber() || phiRegNO == curDefOpnd->GetRegisterNumber()) {
            doOpt = false;
            return;
          }
        }
      }
    }
  }
  /*
   * 1) avoid spills for propagating across calls:
   *      mov R100, #1
   *      ...
   *      bl func
   *      mov R103, #1
   *      str R103, [mem]
   *
   * 2) avoid cc redefine between two same insns that define cc
   */
  CHECK_FATAL(existDefOpnd != nullptr && curDefOpnd != nullptr, "invalied defOpnd");
  bool isDefCC = (existDefOpnd->IsOfCC() || curDefOpnd->IsOfCC());
  for (const Insn *cursor = &existInsn; cursor != nullptr && cursor != &curInsn; cursor = cursor->GetNext()) {
    if (!cursor->IsMachineInstruction()) {
      continue;
    }
    if (cursor->IsCall()) {
      doOpt = false;
      return;
    }
    if (isDefCC && cursor->GetOperand(kInsnFirstOpnd).IsRegister() &&
        static_cast<RegOperand&>(cursor->GetOperand(kInsnFirstOpnd)).IsOfCC()) {
      doOpt = false;
      return;
    }
  }
}

std::size_t AArch64RedundantComputeElim::ComputeDefUseHash(const Insn &insn, const RegOperand *replaceOpnd) const {
  std::size_t hashSeed = 0;
  std::string hashS = std::to_string(insn.GetMachineOpcode());
  uint32 opndNum = insn.GetOperandSize();
  hashS += std::to_string(opndNum);
  for (uint i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    if (insn.OpndIsDef(i) && insn.OpndIsUse(i)) {
      CHECK_FATAL(opnd.IsRegister(), "must be");
      hashS += (replaceOpnd == nullptr ?
        static_cast<RegOperand&>(opnd).GetHashContent() : replaceOpnd->GetHashContent());
    } else if (insn.OpndIsUse(i)) {
      Operand::OperandType opndKind = opnd.GetKind();
      if (opndKind == Operand::kOpdImmediate) {
        hashS += static_cast<ImmOperand &>(opnd).GetHashContent();
      } else if (opndKind == Operand::kOpdExtend) {
        hashS += static_cast<ExtendShiftOperand &>(opnd).GetHashContent();
      } else if (opndKind == Operand::kOpdShift) {
        hashS += static_cast<BitShiftOperand &>(opnd).GetHashContent();
      } else {
        hashS += std::to_string(++hashSeed);
      }
    }
  }
  return std::hash<std::string>{}(hashS);
}

DUInsnInfo *AArch64RedundantComputeElim::GetDefUseInsnInfo(VRegVersion &defVersion) {
  DUInsnInfo *defUseInfo = nullptr;
  for (auto duInfoIt : defVersion.GetAllUseInsns()) {
    ASSERT(duInfoIt.second, "get DUInsnInfo failed");
    Insn *useInsn = duInfoIt.second->GetInsn();
    ASSERT(useInsn, "get useInsn by ssaVersion failed");
    for (auto &opndIt : as_const(duInfoIt.second->GetOperands())) {
      if (opndIt.first == useInsn->GetBothDefUseOpnd()) {
        if (defUseInfo != nullptr) {
          doOpt = false;
          return nullptr;
        } else {
          defUseInfo = duInfoIt.second;
          break;
        }
      }
    }
  }
  return defUseInfo;
}

/*
 * useInsns of existVersion that have both def & use must check complete chain
 * For example:
 *   movn  R143, #2
 *   movk  R143(use){implicit-def: R144}, #32767, LSL #16
 *   movk  R144(use){implicit-def: R145}, #65534, LSL #32
 *   ...
 *   movn  R152, #2                                         ===> can not be optimized
 *   movk  R152(use){implicit-def: R153), #32767, LSL #16
 *   movk  R153(use){implicit-def: R154), #8198, LSL #48    ----> different
 */
void AArch64RedundantComputeElim::CheckBothDefAndUseChain(RegOperand *curDstOpnd, RegOperand *existDstOpnd) {
  if (curDstOpnd == nullptr || existDstOpnd == nullptr) {
    doOpt = false;
    return;
  }
  VRegVersion *existDefVer = ssaInfo->FindSSAVersion(existDstOpnd->GetRegisterNumber());
  VRegVersion *curDefVer = ssaInfo->FindSSAVersion(curDstOpnd->GetRegisterNumber());
  CHECK_FATAL(existDefVer && curDefVer, "get ssa version failed");
  DUInsnInfo *existInfo = GetDefUseInsnInfo(*existDefVer);
  DUInsnInfo *curInfo = GetDefUseInsnInfo(*curDefVer);
  if (existInfo == nullptr) {
    return;
  }
  if (curInfo == nullptr) {
    doOpt = false;
    return;
  }
  if (existInfo->GetOperands().size() > 1 || curInfo->GetOperands().size() > 1 ||
      existInfo->GetOperands().begin()->first != curInfo->GetOperands().begin()->first) {
    doOpt = false;
    return;
  }
  uint32 opndIdx = existInfo->GetOperands().cbegin()->first;
  Insn *existUseInsn = existInfo->GetInsn();
  Insn *curUseInsn = curInfo->GetInsn();
  CHECK_FATAL(existUseInsn && curUseInsn, "get useInsn failed");
  if (existUseInsn->OpndIsDef(opndIdx) && curUseInsn->OpndIsDef(opndIdx)) {
    std::size_t existHash = ComputeDefUseHash(*existUseInsn, nullptr);
    std::size_t curHash = ComputeDefUseHash(*curUseInsn, existDstOpnd);
    if (existHash != curHash) {
      doOpt = false;
      return;
    } else {
      curDstOpnd = curUseInsn->GetSSAImpDefOpnd();
      existDstOpnd = existUseInsn->GetSSAImpDefOpnd();
      CheckBothDefAndUseChain(curDstOpnd, existDstOpnd);
    }
  } else if (existUseInsn->OpndIsDef(opndIdx)) {
    doOpt = false;
    return;
  }
}

bool AArch64RedundantComputeElim::IsBothDefUseCase(VRegVersion &version) const {
  for (auto infoIt : version.GetAllUseInsns()) {
    ASSERT(infoIt.second != nullptr, "get duInsnInfo failed");
    Insn *useInsn = infoIt.second->GetInsn();
    ASSERT(useInsn != nullptr, "get useInsn failed");
    for (auto &opndIt : as_const(infoIt.second->GetOperands())) {
      if (useInsn->GetBothDefUseOpnd() == opndIt.first) {
        return true;
      }
    }
  }
  return false;
}

bool AArch64RedundantComputeElim::DoOpt(BB *bb) {
  bool optimize = false;
  FOR_BB_INSNS(insn, bb) {
    if (!insn->IsMachineInstruction() || insn->GetOperandSize() == 0) {
      continue;
    }
    doOpt = true;
    if (cgFunc->GetName() == "expand_shift" && insn->GetId() == 144) {
      std::cout << "add logical shift!" << std::endl;
    }
    auto iter = candidates.find(insn);
    if (iter != candidates.end()) {
      /* during iteration, avoid repeated processing of insn, which may access wrong duInsnInfo of ssaVersion */
      if (insn->HasProcessedRHS()) {
        continue;
      }
      Operand *curDst = &insn->GetOperand(kInsnFirstOpnd);
      ASSERT(curDst->IsRegister(), "must be");
      auto *curDstOpnd = static_cast<RegOperand*>(curDst);
      regno_t curRegNO = curDstOpnd->GetRegisterNumber();
      if (!insn->IsRegDefined(curRegNO) || static_cast<RegOperand*>(curDstOpnd)->IsPhysicalRegister()) {
        continue;
      }
      Insn *existInsn = *iter;
      Operand *existDst = &existInsn->GetOperand(kInsnFirstOpnd);
      ASSERT(existDst->IsRegister(), "must be");
      auto *existDstOpnd = static_cast<RegOperand*>(existDst);
      regno_t existRegNO = (existDstOpnd)->GetRegisterNumber();
      if (!existInsn->IsRegDefined(existRegNO) || static_cast<RegOperand*>(existDstOpnd)->IsPhysicalRegister()) {
        continue;
      }
      /* two insns have both def & use opnd */
      if (insn->OpndIsUse(kInsnFirstOpnd) && existInsn->OpndIsUse(kInsnFirstOpnd)) {
        curDstOpnd = insn->GetSSAImpDefOpnd();
        existDstOpnd = existInsn->GetSSAImpDefOpnd();
        CHECK_FATAL(curDstOpnd && existDstOpnd, "get ssa implicit def opnd failed");
      }
      VRegVersion *existDefVer = ssaInfo->FindSSAVersion(existDstOpnd->GetRegisterNumber());
      VRegVersion *curDefVer = ssaInfo->FindSSAVersion(curDstOpnd->GetRegisterNumber());
      CHECK_FATAL(existDefVer && curDefVer, "get ssa version failed");
      isBothDefUse = IsBothDefUseCase(*existDefVer);
      if (isBothDefUse) {
        CheckBothDefAndUseChain(curDstOpnd, existDstOpnd);
      }
      if (!doOpt) {
        continue;
      }
      CheckCondition(*existInsn, *insn);
      if (!doOpt) {
        continue;
      }
      if (CG_RCE_DUMP) {
        Dump(existInsn, insn);
      }
      Optimize(*bb, *insn, *curDstOpnd, *existDstOpnd);
      optimize = true;
    } else {
      (void)candidates.insert(insn);
    }
  }
  return optimize;
}

MOperator AArch64RedundantComputeElim::GetNewMop(const RegOperand &curDstOpnd, const RegOperand &existDstOpnd) const {
  MOperator newMop = MOP_undef;
  bool is64Bit = (curDstOpnd.GetSize() == k64BitSize);
  if (curDstOpnd.IsOfFloatOrSIMDClass() && existDstOpnd.IsOfIntClass()) {
    newMop = (is64Bit ? MOP_xvmovdr : MOP_xvmovsr);
  } else if (curDstOpnd.IsOfIntClass() && existDstOpnd.IsOfFloatOrSIMDClass()) {
    newMop = (is64Bit ? MOP_xvmovrd : MOP_xvmovrs);
  } else if (curDstOpnd.IsOfFloatOrSIMDClass() && existDstOpnd.IsOfFloatOrSIMDClass()) {
    newMop = (is64Bit ? MOP_xvmovd : MOP_xvmovs);
  } else {
    newMop = (is64Bit ? MOP_xmovrr : MOP_wmovrr);
  }
  return newMop;
}

void AArch64RedundantComputeElim::Optimize(BB &curBB, Insn &curInsn,
                                           RegOperand &curDstOpnd, RegOperand &existDstOpnd) const {
  /*
   * 1) useInsns of existVersion that have both def & use need replace ssaVersion first.
   * 2) other cases can be replaced by mov.
   */
  if (isBothDefUse) {
    VRegVersion *existDefVersion = ssaInfo->FindSSAVersion(existDstOpnd.GetRegisterNumber());
    VRegVersion *curDefVersion = ssaInfo->FindSSAVersion(curDstOpnd.GetRegisterNumber());
    CHECK_FATAL(existDefVersion && curDefVersion, "get ssa version failed");
    ssaInfo->ReplaceAllUse(curDefVersion, existDefVersion);
    curInsn.SetProcessRHS();
  } else {
    MOperator newMop = GetNewMop(curDstOpnd, existDstOpnd);
    Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, curDstOpnd, existDstOpnd);
    curBB.ReplaceInsn(curInsn, newInsn);
    ssaInfo->ReplaceInsn(curInsn, newInsn);
    newInsn.SetProcessRHS();
  }
}

void AArch64RedundantComputeElim::DumpHash() const {
  LogInfo::MapleLogger() << "\n";
  LogInfo::MapleLogger() << ">>>>>> Insn Hash For " << cgFunc->GetName() << " <<<<<<\n";
  FOR_ALL_BB(bb, cgFunc) {
    g_hashSeed = 0;
    if (bb->IsUnreachable()) {
      continue;
    }
    cgFunc->DumpBBInfo(bb);
    FOR_BB_INSNS(insn, bb) {
      LogInfo::MapleLogger() << "[primal form] ";
      insn->Dump();
      if (insn->IsMachineInstruction()) {
        LogInfo::MapleLogger() << "{ RHSHashCode: " << InsnRHSHash{}(insn) << " }\n";
      }
    }
  }
}
} /* namespace maplebe */