/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#if TARGAARCH64
#include "aarch64_reaching.h"
#include "aarch64_isa.h"
#elif defined(TARGRISCV64) && TARGRISCV64
#include "riscv64_reaching.h"
#endif
#if TARGARM32
#include "arm32_reaching.h"
#endif
#include "cg_option.h"
#include "cgfunc.h"
#include "cg.h"

/*
 * This phase build bb->in and bb->out infomation for stack memOperand and RegOperand. each bit in DataInfo
 * represent whether the register or memory is live or not. to save storage space, the offset of stack is divided
 * by 4, since the offset is a multiple 4.
 * this algorithm mainly include 2 parts:
 *   1. initialize each BB
 *     (1) insert pseudoInsns for function parameters, ehBB, and return R0/V0
 *     (2) init bb->gen, bb->use, bb->out
 *   2. build in and out
 *     (1) In[BB] = Union all of out[Parents(bb)]
 *     (2) Out[BB] = gen[BB] union in[BB]
 * aditionally, this phase provide several commen funcfion about data flow. users can call these functions in
 * optimization phase conveniently.
 */
namespace maplebe {
ReachingDefinition::ReachingDefinition(CGFunc &func, MemPool &memPool)
    : AnalysisResult(&memPool), cgFunc(&func), rdAlloc(&memPool), stackMp(func.GetStackMemPool()),
      pseudoInsns(rdAlloc.Adapter()), kMaxBBNum(cgFunc->NumBBs() + 1), normalBBSet(rdAlloc.Adapter()),
      cleanUpBBSet(rdAlloc.Adapter()) {}

/* check whether the opnd is stack register or not */
bool ReachingDefinition::IsFrameReg(const Operand &opnd) const {
  if (!opnd.IsRegister()) {
    return false;
  }
  auto &reg = static_cast<const RegOperand&>(opnd);
  return cgFunc->IsFrameReg(reg);
}

/* intialize bb->out, bb->out only include generated DataInfo */
void ReachingDefinition::InitOut(const BB &bb) {
  if ((mode & kRDRegAnalysis) != 0) {
    *regOut[bb.GetId()] = *regGen[bb.GetId()];
  }
  if ((mode & kRDMemAnalysis) != 0) {
    *memOut[bb.GetId()] = *memGen[bb.GetId()];
  }
}

/* when DataInfo will not be used later, they should be cleared. */
void ReachingDefinition::ClearDefUseInfo() {
  for (auto insn : pseudoInsns) {
    /* Keep return pseudo to extend the return register liveness to 'ret'.
     * Backward propagation can move the return register definition far from the return.
     */
#ifndef TARGX86_64
    if (insn->GetMachineOpcode() == MOP_pseudo_ret_int || insn->GetMachineOpcode() == MOP_pseudo_ret_float) {
      continue;
    }
#endif
    insn->GetBB()->RemoveInsn(*insn);
  }
  FOR_ALL_BB(bb, cgFunc) {
    delete (regGen[bb->GetId()]);
    regGen[bb->GetId()] = nullptr;
    delete (regUse[bb->GetId()]);
    regUse[bb->GetId()] = nullptr;
    delete (regIn[bb->GetId()]);
    regIn[bb->GetId()] = nullptr;
    delete (regOut[bb->GetId()]);
    regOut[bb->GetId()] = nullptr;
    delete (memGen[bb->GetId()]);
    memGen[bb->GetId()] = nullptr;
    delete (memUse[bb->GetId()]);
    memUse[bb->GetId()] = nullptr;
    delete (memIn[bb->GetId()]);
    memIn[bb->GetId()] = nullptr;
    delete (memOut[bb->GetId()]);
    memOut[bb->GetId()] = nullptr;
  }
  regGen.clear();
  regGen.shrink_to_fit();
  regUse.clear();
  regUse.shrink_to_fit();
  regIn.clear();
  regIn.shrink_to_fit();
  regOut.clear();
  regOut.shrink_to_fit();
  memGen.clear();
  memGen.shrink_to_fit();
  memUse.clear();
  memUse.shrink_to_fit();
  memIn.clear();
  memIn.shrink_to_fit();
  memOut.clear();
  memOut.shrink_to_fit();
  cgFunc->SetRD(nullptr);
}

/*
 * find used insns for register.
 *  input:
 *    insn: the insn in which register is defined
 *    regNO: the No of register
 *    isRegNO: this argument is used to form function overloading
 *  return:
 *    the set of used insns for register
 */
InsnSet ReachingDefinition::FindUseForRegOpnd(Insn &insn, uint32 indexOrRegNO, bool isRegNO) const {
  InsnSet useInsnSet;
  uint32 regNO = indexOrRegNO;
  if (!isRegNO) {
    Operand &opnd = insn.GetOperand(indexOrRegNO);
    auto &regOpnd = static_cast<RegOperand&>(opnd);
    regNO = regOpnd.GetRegisterNumber();
  }
  /* register may be redefined in current bb */
  bool findFinish = FindRegUseBetweenInsn(regNO, insn.GetNext(), insn.GetBB()->GetLastInsn(), useInsnSet);
  std::vector<bool> visitedBB(kMaxBBNum, false);
  if (findFinish || !regOut[insn.GetBB()->GetId()]->TestBit(regNO)) {
    if (!insn.GetBB()->GetEhSuccs().empty()) {
      DFSFindUseForRegOpnd(*insn.GetBB(), regNO, visitedBB, useInsnSet, true);
    }
  } else {
    DFSFindUseForRegOpnd(*insn.GetBB(), regNO, visitedBB, useInsnSet, false);
  }

  if (!insn.GetBB()->IsCleanup() && firstCleanUpBB != nullptr) {
    if (regUse[firstCleanUpBB->GetId()]->TestBit(regNO)) {
      findFinish = FindRegUseBetweenInsn(regNO, firstCleanUpBB->GetFirstInsn(),
                                         firstCleanUpBB->GetLastInsn(), useInsnSet);
      if (findFinish || !regOut[firstCleanUpBB->GetId()]->TestBit(regNO)) {
        return useInsnSet;
      }
    }
    DFSFindUseForRegOpnd(*firstCleanUpBB, regNO, visitedBB, useInsnSet, false);
  }

  return useInsnSet;
}

/*
 * find used insns for register iteratively.
 *  input:
 *    startBB: find used insns starting from startBB
 *    regNO: the No of register to be find
 *    visitedBB: record these visited BB
 *    useInsnSet: used insns of register is saved in this set
 */
void ReachingDefinition::DFSFindUseForRegOpnd(const BB &startBB, uint32 regNO, std::vector<bool> &visitedBB,
                                              InsnSet &useInsnSet, bool onlyFindForEhSucc = false) const {
  if (!onlyFindForEhSucc) {
    for (auto succBB : startBB.GetSuccs()) {
      if (!regIn[succBB->GetId()]->TestBit(regNO)) {
        continue;
      }
      if (visitedBB[succBB->GetId()]) {
        continue;
      }
      visitedBB[succBB->GetId()] = true;
      bool findFinish = false;
      if (regUse[succBB->GetId()]->TestBit(regNO)) {
        findFinish = FindRegUseBetweenInsn(regNO, succBB->GetFirstInsn(), succBB->GetLastInsn(), useInsnSet);
      } else if (regGen[succBB->GetId()]->TestBit(regNO)) {
        findFinish = true;
      }
      if (!findFinish && regOut[succBB->GetId()]->TestBit(regNO)) {
        DFSFindUseForRegOpnd(*succBB, regNO, visitedBB, useInsnSet, false);
      }
    }
  }

  for (auto ehSuccBB : startBB.GetEhSuccs()) {
    if (!regIn[ehSuccBB->GetId()]->TestBit(regNO)) {
      continue;
    }
    if (visitedBB[ehSuccBB->GetId()]) {
      continue;
    }
    visitedBB[ehSuccBB->GetId()] = true;

    bool findFinish = false;
    if (regUse[ehSuccBB->GetId()]->TestBit(regNO)) {
      findFinish = FindRegUseBetweenInsn(regNO, ehSuccBB->GetFirstInsn(), ehSuccBB->GetLastInsn(), useInsnSet);
    } else if (regGen[ehSuccBB->GetId()]->TestBit(regNO)) {
      findFinish = true;
    }
    if (!findFinish && regOut[ehSuccBB->GetId()]->TestBit(regNO)) {
      DFSFindUseForRegOpnd(*ehSuccBB, regNO, visitedBB, useInsnSet, false);
    }
  }
}

/* check whether register defined in regDefInsn has used insns */
bool ReachingDefinition::RegHasUsePoint(uint32 regNO, Insn &regDefInsn) const {
  InsnSet useInsnSet;
  bool findFinish = FindRegUseBetweenInsn(regNO, regDefInsn.GetNext(), regDefInsn.GetBB()->GetLastInsn(), useInsnSet);
  if (!useInsnSet.empty()) {
    return true;
  }
  if (!findFinish) {
    std::vector<bool> visitedBB(kMaxBBNum, false);
    return RegIsUsedInOtherBB(*regDefInsn.GetBB(), regNO, visitedBB);
  }
  return false;
}

/* check whether register is used in other BB except startBB */
bool ReachingDefinition::RegIsUsedInOtherBB(const BB &startBB, uint32 regNO, std::vector<bool> &visitedBB) const {
  InsnSet useInsnSet;
  for (auto succBB : startBB.GetSuccs()) {
    if (!regIn[succBB->GetId()]->TestBit(regNO)) {
      continue;
    }
    if (visitedBB[succBB->GetId()]) {
      continue;
    }
    visitedBB[succBB->GetId()] = true;
    bool findFinish = false;
    if (regUse[succBB->GetId()]->TestBit(regNO)) {
      if (!regGen[succBB->GetId()]->TestBit(regNO)) {
        return true;
      }
      useInsnSet.clear();
      findFinish = FindRegUseBetweenInsn(regNO, succBB->GetFirstInsn(), succBB->GetLastInsn(), useInsnSet);
      if (!useInsnSet.empty()) {
        return true;
      }
    } else if (regGen[succBB->GetId()]->TestBit(regNO)) {
      findFinish = true;
    }
    if (!findFinish && regOut[succBB->GetId()]->TestBit(regNO)) {
      if (RegIsUsedInOtherBB(*succBB, regNO, visitedBB)) {
        return true;
      }
    }
  }

  for (auto ehSuccBB : startBB.GetEhSuccs()) {
    if (!regIn[ehSuccBB->GetId()]->TestBit(regNO)) {
      continue;
    }
    if (visitedBB[ehSuccBB->GetId()]) {
      continue;
    }
    visitedBB[ehSuccBB->GetId()] = true;

    bool findFinish = false;
    if (regUse[ehSuccBB->GetId()]->TestBit(regNO)) {
      if (!regGen[ehSuccBB->GetId()]->TestBit(regNO)) {
        return true;
      }
      useInsnSet.clear();
      findFinish = FindRegUseBetweenInsn(regNO, ehSuccBB->GetFirstInsn(), ehSuccBB->GetLastInsn(), useInsnSet);
      if (!useInsnSet.empty()) {
        return true;
      }
    } else if (regGen[ehSuccBB->GetId()]->TestBit(regNO)) {
      findFinish = true;
    }
    if (!findFinish && regOut[ehSuccBB->GetId()]->TestBit(regNO)) {
      if (RegIsUsedInOtherBB(*ehSuccBB, regNO, visitedBB)) {
        return true;
      }
    }
  }

  return false;
}

bool ReachingDefinition::RegIsUsedInCleanUpBB(uint32 regNO) const {
  if (firstCleanUpBB == nullptr) {
    return false;
  }
  InsnSet useInsnSet;
  if (regUse[firstCleanUpBB->GetId()]->TestBit(regNO)) {
    bool findFinish = FindRegUseBetweenInsn(regNO, firstCleanUpBB->GetFirstInsn(),
                                            firstCleanUpBB->GetLastInsn(), useInsnSet);
    if (!useInsnSet.empty()) {
      return true;
    }
    if (findFinish) {
      return false;
    }
  }

  std::vector<bool> visitedBB(kMaxBBNum, false);
  DFSFindUseForRegOpnd(*firstCleanUpBB, regNO, visitedBB, useInsnSet, false);
  if (useInsnSet.empty()) {
    return true;
  }

  return false;
}

/*
 * find used insns for stack memory operand iteratively.
 *  input:
 *    startBB: find used insns starting from startBB
 *    offset: the offset of memory to be find
 *    visitedBB: record these visited BB
 *    useInsnSet: used insns of stack memory operand is saved in this set
 */
void ReachingDefinition::DFSFindUseForMemOpnd(const BB &startBB, uint32 offset, std::vector<bool> &visitedBB,
                                              InsnSet &useInsnSet, bool onlyFindForEhSucc = false) const {
  if (!onlyFindForEhSucc) {
    for (auto succBB : startBB.GetSuccs()) {
      if (!memIn[succBB->GetId()]->TestBit(offset / kMemZoomSize)) {
        continue;
      }
      if (visitedBB[succBB->GetId()]) {
        continue;
      }
      visitedBB[succBB->GetId()] = true;
      bool findFinish = false;
      if (memUse[succBB->GetId()]->TestBit(offset / kMemZoomSize)) {
        findFinish = FindMemUseBetweenInsn(offset, succBB->GetFirstInsn(), succBB->GetLastInsn(), useInsnSet);
      } else if (memGen[succBB->GetId()]->TestBit(offset / kMemZoomSize)) {
        findFinish = true;
      }
      if (!findFinish && memOut[succBB->GetId()]->TestBit(offset / kMemZoomSize)) {
        DFSFindUseForMemOpnd(*succBB, offset, visitedBB, useInsnSet);
      }
    }
  }

  for (auto ehSuccBB : startBB.GetEhSuccs()) {
    if (!memIn[ehSuccBB->GetId()]->TestBit(offset / kMemZoomSize)) {
      continue;
    }
    if (visitedBB[ehSuccBB->GetId()]) {
      continue;
    }
    visitedBB[ehSuccBB->GetId()] = true;
    bool findFinish = false;
    if (memUse[ehSuccBB->GetId()]->TestBit(offset / kMemZoomSize)) {
      findFinish = FindMemUseBetweenInsn(offset, ehSuccBB->GetFirstInsn(), ehSuccBB->GetLastInsn(), useInsnSet);
    } else if (memGen[ehSuccBB->GetId()]->TestBit(offset / kMemZoomSize)) {
      findFinish = true;
    }
    if (!findFinish && memOut[ehSuccBB->GetId()]->TestBit(offset / kMemZoomSize)) {
      DFSFindUseForMemOpnd(*ehSuccBB, offset, visitedBB, useInsnSet);
    }
  }
}

/* Out[BB] = gen[BB] union in[BB]. if bb->out changed, return true. */
bool ReachingDefinition::GenerateOut(const BB &bb) {
  bool outInfoChanged = false;
  if ((mode & kRDRegAnalysis) != 0) {
    LocalMapleAllocator alloc(stackMp);
    DataInfo &bbRegOutBak = regOut[bb.GetId()]->Clone(alloc);
    *regOut[bb.GetId()] = *(regIn[bb.GetId()]);
    regOut[bb.GetId()]->OrBits(*regGen[bb.GetId()]);
    if (!regOut[bb.GetId()]->IsEqual(bbRegOutBak)) {
      outInfoChanged = true;
    }
  }

  if ((mode & kRDMemAnalysis) != 0) {
    LocalMapleAllocator alloc(stackMp);
    DataInfo &bbMemOutBak = memOut[bb.GetId()]->Clone(alloc);
    *memOut[bb.GetId()] = *memIn[bb.GetId()];
    memOut[bb.GetId()]->OrBits(*memGen[bb.GetId()]);
    if (!memOut[bb.GetId()]->IsEqual(bbMemOutBak)) {
      outInfoChanged = true;
    }
  }
  return outInfoChanged;
}

bool ReachingDefinition::GenerateOut(const BB &bb, const std::set<uint32> &infoIndex, const bool isReg) {
  bool outInfoChanged = false;
  if (isReg) {
    for (auto index : infoIndex) {
      uint64 bbRegOutBak = regOut[bb.GetId()]->GetElem(index);
      regOut[bb.GetId()]->SetElem(index, regIn[bb.GetId()]->GetElem(index));
      regOut[bb.GetId()]->OrDesignateBits(*regGen[bb.GetId()], index);
      if (!outInfoChanged && (bbRegOutBak != regOut[bb.GetId()]->GetElem(index))) {
        outInfoChanged = true;
      }
    }
  } else {
    for (auto index : infoIndex) {
      uint64 bbMemOutBak = memOut[bb.GetId()]->GetElem(index);
      memOut[bb.GetId()]->SetElem(index, memIn[bb.GetId()]->GetElem(index));
      memOut[bb.GetId()]->OrDesignateBits(*memGen[bb.GetId()], index);
      if (bbMemOutBak != memOut[bb.GetId()]->GetElem(index)) {
        outInfoChanged = true;
      }
    }
  }
  return outInfoChanged;
}


/* In[BB] = Union all of out[Parents(bb)]. return true if bb->in changed. */
bool ReachingDefinition::GenerateIn(const BB &bb) {
  bool inInfoChanged = false;
  if ((mode & kRDRegAnalysis) != 0) {
    LocalMapleAllocator alloc(stackMp);
    DataInfo &bbRegInBak = regIn[bb.GetId()]->Clone(alloc);
    for (auto predBB : bb.GetPreds()) {
      regIn[bb.GetId()]->OrBits(*regOut[predBB->GetId()]);
    }
    for (auto predEhBB : bb.GetEhPreds()) {
      regIn[bb.GetId()]->OrBits(*regOut[predEhBB->GetId()]);
    }

    if (!regIn[bb.GetId()]->IsEqual(bbRegInBak)) {
      inInfoChanged = true;
    }
  }
  if ((mode & kRDMemAnalysis) != 0) {
    LocalMapleAllocator alloc(stackMp);
    DataInfo &memInBak = memIn[bb.GetId()]->Clone(alloc);
    for (auto predBB : bb.GetPreds()) {
      memIn[bb.GetId()]->OrBits(*memOut[predBB->GetId()]);
    }
    for (auto predEhBB : bb.GetEhPreds()) {
      memIn[bb.GetId()]->OrBits(*memOut[predEhBB->GetId()]);
    }

    if (!memIn[bb.GetId()]->IsEqual(memInBak)) {
      inInfoChanged = true;
    }
  }
  return inInfoChanged;
}

/* In[BB] = Union all of out[Parents(bb)]. return true if bb->in changed. */
bool ReachingDefinition::GenerateIn(const BB &bb, const std::set<uint32> &infoIndex, const bool isReg) {
  bool inInfoChanged = false;

  if (isReg) {
    for (auto index : infoIndex) {
      uint64 bbRegInBak = regIn[bb.GetId()]->GetElem(index);
      regIn[bb.GetId()]->SetElem(index, 0ULL);
      for (auto predBB : bb.GetPreds()) {
        regIn[bb.GetId()]->OrDesignateBits(*regOut[predBB->GetId()], index);
      }
      for (auto predEhBB : bb.GetEhPreds()) {
        regIn[bb.GetId()]->OrDesignateBits(*regOut[predEhBB->GetId()], index);
      }

      if (bbRegInBak != regIn[bb.GetId()]->GetElem(index)) {
        inInfoChanged = true;
      }
    }
  } else {
    for (auto index : infoIndex) {
      uint64 bbMemInBak = memIn[bb.GetId()]->GetElem(index);
      memIn[bb.GetId()]->SetElem(index, 0ULL);
      for (auto predBB : bb.GetPreds()) {
        memIn[bb.GetId()]->OrDesignateBits(*memOut[predBB->GetId()], index);
      }
      for (auto predEhBB : bb.GetEhPreds()) {
        memIn[bb.GetId()]->OrDesignateBits(*memOut[predEhBB->GetId()], index);
      }

      if (bbMemInBak != memIn[bb.GetId()]->GetElem(index)) {
        inInfoChanged = true;
      }
    }
  }
  return inInfoChanged;
}


/* In[firstCleanUpBB] = Union all of out[bbNormalSet] */
bool ReachingDefinition::GenerateInForFirstCleanUpBB() {
  CHECK_NULL_FATAL(firstCleanUpBB);
  if ((mode & kRDRegAnalysis) != 0) {
    regIn[firstCleanUpBB->GetId()]->ResetAllBit();
  }
  if ((mode & kRDMemAnalysis) != 0) {
    memIn[firstCleanUpBB->GetId()]->ResetAllBit();
  }

  for (auto normalBB : normalBBSet) {
    if ((mode & kRDRegAnalysis) != 0) {
      regIn[firstCleanUpBB->GetId()]->OrBits(*regOut[normalBB->GetId()]);
    }

    if ((mode & kRDMemAnalysis) != 0) {
      memIn[firstCleanUpBB->GetId()]->OrBits(*memOut[normalBB->GetId()]);
    }
  }

  return ((regIn[firstCleanUpBB->GetId()] != nullptr && regIn[firstCleanUpBB->GetId()]->Size() > 0) ||
          (memIn[firstCleanUpBB->GetId()] != nullptr && memIn[firstCleanUpBB->GetId()]->Size() > 0));
}

bool ReachingDefinition::GenerateInForFirstCleanUpBB(bool isReg, const std::set<uint32> &infoIndex) {
  CHECK_NULL_FATAL(firstCleanUpBB);
  bool inInfoChanged = false;
  if (isReg) {
    for (auto index : infoIndex) {
      uint64 regInElemBak = regIn[firstCleanUpBB->GetId()]->GetElem(index);
      regIn[firstCleanUpBB->GetId()]->SetElem(index, 0ULL);
      for (auto &normalBB : normalBBSet) {
        regIn[firstCleanUpBB->GetId()]->OrDesignateBits(*regOut[normalBB->GetId()], index);
      }
      if (!inInfoChanged && (regIn[firstCleanUpBB->GetId()]->GetElem(index) != regInElemBak)) {
        inInfoChanged = true;
      }
    }
  } else {
    for (auto index : infoIndex) {
      uint64 memInElemBak = memIn[firstCleanUpBB->GetId()]->GetElem(index);
      memIn[firstCleanUpBB->GetId()]->SetElem(index, 0ULL);
      for (auto &normalBB : normalBBSet) {
        memIn[firstCleanUpBB->GetId()]->OrDesignateBits(*memOut[normalBB->GetId()], index);
      }
      if (!inInfoChanged && (memIn[firstCleanUpBB->GetId()]->GetElem(index) != memInElemBak)) {
        inInfoChanged = true;
      }
    }
  }
  return inInfoChanged;
}

/* allocate memory for DataInfo of bb */
void ReachingDefinition::InitRegAndMemInfo(const BB &bb) {
  if ((mode & kRDRegAnalysis) != 0) {
    const uint32 kMaxRegCount = cgFunc->GetMaxVReg();
    regGen[bb.GetId()] = new DataInfo(kMaxRegCount, rdAlloc);
    regUse[bb.GetId()] = new DataInfo(kMaxRegCount, rdAlloc);
    regIn[bb.GetId()] = new DataInfo(kMaxRegCount, rdAlloc);
    regOut[bb.GetId()] = new DataInfo(kMaxRegCount, rdAlloc);
  }

  if ((mode & kRDMemAnalysis) != 0) {
    const int32 kStackSize = GetStackSize();
    memGen[bb.GetId()] = new DataInfo((kStackSize / kMemZoomSize), rdAlloc);
    memUse[bb.GetId()] = new DataInfo((kStackSize / kMemZoomSize), rdAlloc);
    memIn[bb.GetId()] = new DataInfo((kStackSize / kMemZoomSize), rdAlloc);
    memOut[bb.GetId()] = new DataInfo((kStackSize / kMemZoomSize), rdAlloc);
  }
}

// Insert pseudoInsns for function parameters, ehBB, and return R0/V0. init bb->gen, bb->use, bb->out.
// We already insert pseudo-ret-insns in exit BBs in handleFunction.
void ReachingDefinition::Initialize() {
  InitDataSize();
  FOR_ALL_BB(bb, cgFunc) {
    InitRegAndMemInfo(*bb);
  }
  FOR_ALL_BB(bb, cgFunc) {
    if (bb == cgFunc->GetFirstBB()) {
      InitStartGen();
    }
    if (!bb->GetEhPreds().empty()) {
      InitEhDefine(*bb);
    }
    InitGenUse(*bb);
    InitOut(*bb);

    if (bb->IsCleanup()) {
      (void)cleanUpBBSet.insert(bb);
    } else {
      (void)normalBBSet.insert(bb);
    }
  }
  maxInsnNO = 0;
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        insn->SetId(maxInsnNO);
        continue;
      }
      insn->SetId(maxInsnNO);
      maxInsnNO += kInsnNoInterval;
    }
  }
}

void ReachingDefinition::InitDataSize() {
  /* to visit vec[cgFunc->NumBBs()], size should be cgFunc->NumBBs() + 1 */
  const uint32 dataSize = cgFunc->NumBBs() + 1;
  regIn.resize(dataSize);
  regOut.resize(dataSize);
  regGen.resize(dataSize);
  regUse.resize(dataSize);
  memIn.resize(dataSize);
  memOut.resize(dataSize);
  memGen.resize(dataSize);
  memUse.resize(dataSize);
}

/* compute bb->in, bb->out for each BB execpt cleanup BB */
void ReachingDefinition::BuildInOutForFuncBody() {
  std::unordered_set<BB*> normalBBSetBak(normalBBSet.begin(), normalBBSet.end());
  std::unordered_set<BB*>::iterator setItr;
  while (!normalBBSetBak.empty()) {
    setItr = normalBBSetBak.begin();
    BB *bb = *setItr;
    ASSERT(bb != nullptr, "null ptr check");
    (void)normalBBSetBak.erase(setItr);

    if (GenerateIn(*bb)) {
      if (GenerateOut(*bb)) {
        for (auto succ : bb->GetSuccs()) {
          (void)normalBBSetBak.insert(succ);
        }

        for (auto ehSucc : bb->GetEhSuccs()) {
          (void)normalBBSetBak.insert(ehSucc);
        }
      }
    }
  }
  ASSERT(normalBBSetBak.empty(), "CG internal error.");
}

void ReachingDefinition::BuildInOutForFuncBodyBFS() {
  std::vector<bool> inQueued(kMaxBBNum, false);
  std::vector<bool> firstVisited(kMaxBBNum, true);
  std::queue<BB*> worklist;
  for (auto *bb : normalBBSet) {
    worklist.push(bb);
    inQueued[bb->GetId()] = true;
  }

  while (!worklist.empty()) {
    auto *curBB = worklist.front();
    worklist.pop();
    inQueued[curBB->GetId()] = false;
    if (GenerateOut(*curBB) || firstVisited[curBB->GetId()]) {
      firstVisited[curBB->GetId()] = false;
      GenerateIn(*curBB, worklist, inQueued);
    }
  }
}

/* In[BB] = Union all of out[Parents(bb)]. add succ to worklist, if it is changed and not in worklist */
void ReachingDefinition::GenerateIn(const BB &bb, std::queue<BB*> &worklist, std::vector<bool> &inQueued) {
  if ((mode & kRDRegAnalysis) != 0) {
    for (auto *succBB : bb.GetSuccs()) {
      if (regIn[succBB->GetId()]->OrBitsCheck(*regOut[bb.GetId()]) && !inQueued[succBB->GetId()]) {
        worklist.push(succBB);
        inQueued[succBB->GetId()] = true;
      }
    }
    for (auto succEhBB : bb.GetEhSuccs()) {
      if (regIn[succEhBB->GetId()]->OrBitsCheck(*regOut[bb.GetId()]) && !inQueued[succEhBB->GetId()]) {
        worklist.push(succEhBB);
        inQueued[succEhBB->GetId()] = true;
      }
    }
  }
  if ((mode & kRDMemAnalysis) != 0) {
    for (auto *succBB : bb.GetSuccs()) {
      if (memIn[succBB->GetId()]->OrBitsCheck(*memOut[bb.GetId()]) && !inQueued[succBB->GetId()]) {
        worklist.push(succBB);
        inQueued[succBB->GetId()] = true;
      }
    }
    for (auto succEhBB : bb.GetEhSuccs()) {
      if (memIn[succEhBB->GetId()]->OrBitsCheck(*memOut[bb.GetId()]) && !inQueued[succEhBB->GetId()]) {
        worklist.push(succEhBB);
        inQueued[succEhBB->GetId()] = true;
      }
    }
  }
}

/* if bb->out changed, update in and out */
void ReachingDefinition::UpdateInOut(BB &changedBB) {
  InitGenUse(changedBB, false);
  if (!GenerateOut(changedBB)) {
    return;
  }

  std::unordered_set<BB*> bbSet;
  std::unordered_set<BB*>::iterator setItr;

  for (auto succ : changedBB.GetSuccs()) {
    (void)bbSet.insert(succ);
  }

  for (auto ehSucc : changedBB.GetEhSuccs()) {
    (void)bbSet.insert(ehSucc);
  }

  while (!bbSet.empty()) {
    setItr = bbSet.begin();
    BB *bb = *setItr;
    ASSERT(bb != nullptr, "null ptr check");
    bbSet.erase(setItr);

    if (GenerateIn(*bb)) {
      if (GenerateOut(*bb)) {
        for (auto succ : bb->GetSuccs()) {
          (void)bbSet.insert(succ);
        }

        for (auto ehSucc : bb->GetEhSuccs()) {
          (void)bbSet.insert(ehSucc);
        }
      }
    }
  }

  if (!changedBB.IsCleanup() && firstCleanUpBB != nullptr) {
    BuildInOutForCleanUpBB();
  }
}

void ReachingDefinition::UpdateInOut(BB &changedBB, bool isReg) {
  std::set<uint32> changedInfoIndex;
  if (isReg) {
    LocalMapleAllocator alloc(stackMp);
    DataInfo &genInfoBak = regGen[changedBB.GetId()]->Clone(alloc);
    InitGenUse(changedBB, false);
    genInfoBak.EorBits(*regGen[changedBB.GetId()]);
    genInfoBak.GetNonZeroElemsIndex(changedInfoIndex);
  } else {
    LocalMapleAllocator alloc(stackMp);
    DataInfo &genInfoBak = memGen[changedBB.GetId()]->Clone(alloc);
    InitGenUse(changedBB, false);
    genInfoBak.EorBits(*memGen[changedBB.GetId()]);
    genInfoBak.GetNonZeroElemsIndex(changedInfoIndex);
  }
  if (changedInfoIndex.empty()) {
    return;
  }
  if (!GenerateOut(changedBB, changedInfoIndex, isReg)) {
    return;
  }
  std::set<BB*, BBIdCmp> bbSet;
  std::set<BB*, BBIdCmp>::iterator setItr;
  for (auto &succ : changedBB.GetSuccs()) {
    (void)bbSet.insert(succ);
  }

  for (auto &ehSucc : changedBB.GetEhSuccs()) {
    (void)bbSet.insert(ehSucc);
  }
  while (!bbSet.empty()) {
    setItr = bbSet.begin();
    BB *bb = *setItr;
    bbSet.erase(setItr);
    if (GenerateIn(*bb, changedInfoIndex, isReg)) {
      if (GenerateOut(*bb, changedInfoIndex, isReg)) {
        for (auto &succ : bb->GetSuccs()) {
          (void)bbSet.insert(succ);
        }
        for (auto &ehSucc : bb->GetEhSuccs()) {
          (void)bbSet.insert(ehSucc);
        }
      }
    }
  }

  if (!changedBB.IsCleanup() && firstCleanUpBB != nullptr) {
    BuildInOutForCleanUpBB(isReg, changedInfoIndex);
  }
}


/* compute bb->in, bb->out for cleanup BBs */
void ReachingDefinition::BuildInOutForCleanUpBB() {
  ASSERT(firstCleanUpBB != nullptr, "firstCleanUpBB must not be nullptr");
  if (GenerateInForFirstCleanUpBB()) {
    GenerateOut(*firstCleanUpBB);
  }
  std::unordered_set<BB*> cleanupBBSetBak(cleanUpBBSet.begin(), cleanUpBBSet.end());
  std::unordered_set<BB*>::iterator setItr;

  while (!cleanupBBSetBak.empty()) {
    setItr = cleanupBBSetBak.begin();
    BB *bb = *setItr;
    cleanupBBSetBak.erase(setItr);
    if (GenerateIn(*bb)) {
      if (GenerateOut(*bb)) {
        for (auto succ : bb->GetSuccs()) {
          (void)cleanupBBSetBak.insert(succ);
        }
        for (auto ehSucc : bb->GetEhSuccs()) {
          (void)cleanupBBSetBak.insert(ehSucc);
        }
      }
    }
  }
  ASSERT(cleanupBBSetBak.empty(), "CG internal error.");
}

void ReachingDefinition::BuildInOutForCleanUpBB(bool isReg, const std::set<uint32> &index) {
  ASSERT(firstCleanUpBB != nullptr, "firstCleanUpBB must not be nullptr");
  if (GenerateInForFirstCleanUpBB(isReg, index)) {
    GenerateOut(*firstCleanUpBB, index, isReg);
  }
  std::unordered_set<BB*> cleanupBBSetBak(cleanUpBBSet.begin(), cleanUpBBSet.end());
  std::unordered_set<BB*>::iterator setItr;
  while (!cleanupBBSetBak.empty()) {
    setItr = cleanupBBSetBak.begin();
    BB *bb = *setItr;
    cleanupBBSetBak.erase(setItr);
    if (GenerateIn(*bb, index, isReg)) {
      if (GenerateOut(*bb, index, isReg)) {
        for (auto &succ : bb->GetSuccs()) {
          (void)cleanupBBSetBak.insert(succ);
        }
        for (auto &ehSucc : bb->GetEhSuccs()) {
          (void)cleanupBBSetBak.insert(ehSucc);
        }
      }
    }
  }
  ASSERT(cleanupBBSetBak.empty(), "CG internal error.");
}

/* entry for ReachingDefinition Analysis, mode represent to analyze RegOperand, MemOperand or both of them */
void ReachingDefinition::AnalysisStart() {
  if (!cgFunc->GetFirstBB()) {
    return;
  }
  stackSize = static_cast<uint32>(GetStackSize());
  Initialize();
  /* Build in/out for function body first. (Except cleanup bb) */
  BuildInOutForFuncBodyBFS();
  /* If cleanup bb exists, build in/out for cleanup bbs. firstCleanUpBB->in = Union all non-cleanup bb's out. */
  if (firstCleanUpBB != nullptr) {
    BuildInOutForCleanUpBB();
  }
  cgFunc->SetRD(this);
}

/* check whether currentBB can reach endBB according to control flow */
bool ReachingDefinition::CanReachEndBBFromCurrentBB(const BB &currentBB, const BB &endBB,
                                                    std::vector<bool> &traversedBBSet) const {
  if (&currentBB == &endBB) {
    return true;
  }
  for (auto predBB : endBB.GetPreds()) {
    if (traversedBBSet[predBB->GetId()]) {
      continue;
    }
    traversedBBSet[predBB->GetId()] = true;
    if (predBB == &currentBB) {
      return true;
    }
    if (CanReachEndBBFromCurrentBB(currentBB, *predBB, traversedBBSet)) {
      return true;
    }
  }
  for (auto ehPredBB : endBB.GetEhPreds()) {
    if (traversedBBSet[ehPredBB->GetId()]) {
      continue;
    }
    traversedBBSet[ehPredBB->GetId()] = true;
    if (ehPredBB == &currentBB) {
      return true;
    }
    if (CanReachEndBBFromCurrentBB(currentBB, *ehPredBB, traversedBBSet)) {
      return true;
    }
  }
  return false;
}

/* check whether register may be redefined form startBB to endBB */
bool ReachingDefinition::IsLiveInAllPathBB(uint32 regNO, const BB &startBB, const BB &endBB,
                                           std::vector<bool> &visitedBB, bool isFirstNo) const {
  for (auto succ : startBB.GetSuccs()) {
    if (visitedBB[succ->GetId()]) {
      continue;
    }
    visitedBB[succ->GetId()] = true;
    if (isFirstNo && CheckRegLiveinReturnBB(regNO, *succ)) {
      return false;
    }
    std::vector<bool> traversedPathSet(kMaxBBNum, false);
    bool canReachEndBB = true;
    if (regGen[succ->GetId()]->TestBit(regNO)) {
      canReachEndBB = CanReachEndBBFromCurrentBB(*succ, endBB, traversedPathSet);
      if (canReachEndBB) {
        return false;
      }
    }
    if (!canReachEndBB) {
      continue;
    }
    bool isLive = IsLiveInAllPathBB(regNO, *succ, endBB, visitedBB, isFirstNo);
    if (!isLive) {
      return false;
    }
  }

  for (auto ehSucc : startBB.GetEhSuccs()) {
    if (visitedBB[ehSucc->GetId()]) {
      continue;
    }
    visitedBB[ehSucc->GetId()] = true;
    if (isFirstNo && CheckRegLiveinReturnBB(regNO, *ehSucc)) {
      return false;
    }
    std::vector<bool> traversedPathSet(kMaxBBNum, false);
    bool canReachEndBB = true;
    if (regGen[ehSucc->GetId()]->TestBit(regNO)) {
      canReachEndBB = CanReachEndBBFromCurrentBB(*ehSucc, endBB, traversedPathSet);
      if (canReachEndBB) {
        return false;
      }
    }
    if (!canReachEndBB) {
      continue;
    }
    bool isLive = IsLiveInAllPathBB(regNO, *ehSucc, endBB, visitedBB, isFirstNo);
    if (!isLive) {
      return false;
    }
  }
  return true;
}

/* Check if the reg is used in return BB */
bool ReachingDefinition::CheckRegLiveinReturnBB(uint32 regNO, const BB &bb) const {
#if TARGAARCH64 || TARGRISCV64
  if (bb.GetKind() == BB::kBBReturn) {
    PrimType returnType = cgFunc->GetFunction().GetReturnType()->GetPrimType();
    regno_t returnReg = R0;
    if (IsPrimitiveFloat(returnType)) {
      returnReg = V0;
    } else if (IsPrimitiveInteger(returnType)) {
      returnReg = R0;
    }
    if (regNO == returnReg) {
      return true;
    }
  }
#endif
  return false;
}

bool ReachingDefinition::RegIsUsedIncaller(uint32 regNO, Insn &startInsn, Insn &endInsn) const {
  if (startInsn.GetBB() != endInsn.GetBB()) {
    return false;
  }
  if (startInsn.GetNext() == &endInsn || &startInsn == &endInsn) {
    return false;
  }
  auto regDefVec = FindRegDefBetweenInsn(regNO, startInsn.GetNext(), endInsn.GetPrev());
  if (!regDefVec.empty()) {
    return false;
  }
  if (IsCallerSavedReg(regNO) && startInsn.GetNext() != nullptr &&
      KilledByCallBetweenInsnInSameBB(*startInsn.GetNext(), *(startInsn.GetBB()->GetLastInsn()), regNO)) {
    return true;
  }
  if (CheckRegLiveinReturnBB(regNO, *startInsn.GetBB())) {
    return true;
  }
  return false;
}

/* check whether control flow can reach endInsn from startInsn */
bool ReachingDefinition::RegIsLiveBetweenInsn(uint32 regNO, Insn &startInsn, Insn &endInsn, bool isBack,
    bool isFirstNo) const {
  ASSERT(&startInsn != &endInsn, "startInsn is not equal to endInsn");
  if (startInsn.GetBB() == endInsn.GetBB()) {
    /* register is difined more than once */
    if (startInsn.GetId() > endInsn.GetId()) {
      if (!isBack) {
        return false;
      } else {
        return true;
      }
    }
    if (startInsn.GetNext() == &endInsn) {
      return true;
    }
    if (regGen[startInsn.GetBB()->GetId()]->TestBit(regNO)) {
      std::vector<Insn*> regDefVec;
      if (isBack) {
        regDefVec = FindRegDefBetweenInsn(regNO, startInsn.GetNext(), endInsn.GetPrev());
      } else {
        regDefVec = FindRegDefBetweenInsn(regNO, &startInsn, endInsn.GetPrev());
      }
      if (!regDefVec.empty()) {
        return false;
      }
    }
    if (IsCallerSavedReg(regNO) && KilledByCallBetweenInsnInSameBB(*startInsn.GetNext(), *endInsn.GetPrev(), regNO)) {
      return false;
    }
    return true;
  }

  if (&startInsn != startInsn.GetBB()->GetLastInsn() &&
      regGen[startInsn.GetBB()->GetId()]->TestBit(regNO) &&
      !FindRegDefBetweenInsn(regNO, startInsn.GetNext(), startInsn.GetBB()->GetLastInsn()).empty()) {
    return false;
  }

  if (&startInsn != startInsn.GetBB()->GetLastInsn() &&
      IsCallerSavedReg(regNO) &&
      KilledByCallBetweenInsnInSameBB(*startInsn.GetNext(), *startInsn.GetBB()->GetLastInsn(), regNO)) {
    return false;
  }

  if (&endInsn != endInsn.GetBB()->GetFirstInsn() &&
      regGen[endInsn.GetBB()->GetId()]->TestBit(regNO) &&
      !FindRegDefBetweenInsn(regNO, endInsn.GetBB()->GetFirstInsn(), endInsn.GetPrev()).empty()) {
    return false;
  }

  if (&endInsn != endInsn.GetBB()->GetFirstInsn() &&
      IsCallerSavedReg(regNO) &&
      KilledByCallBetweenInsnInSameBB(*endInsn.GetBB()->GetFirstInsn(), *endInsn.GetPrev(), regNO)) {
    return false;
  }

  std::vector<bool> visitedBB(kMaxBBNum, false);
  return IsLiveInAllPathBB(regNO, *startInsn.GetBB(), *endInsn.GetBB(), visitedBB, isFirstNo);
}

static bool SetDefInsnVecForAsm(Insn &insn, uint32 index, uint32 regNO, std::vector<Insn *> &defInsnVec) {
  for (const auto reg : static_cast<ListOperand&>(insn.GetOperand(index)).GetOperands()) {
    if (static_cast<RegOperand *>(reg)->GetRegisterNumber() == regNO) {
      defInsnVec.emplace_back(&insn);
      return true;
    }
  }
  return false;
}

std::vector<Insn*> ReachingDefinition::FindRegDefBetweenInsn(uint32 regNO, Insn *startInsn, Insn *endInsn,
    bool findAll, bool analysisDone) const {
  std::vector<Insn*> defInsnVec;
  if (startInsn == nullptr || endInsn == nullptr) {
    return defInsnVec;
  }

  ASSERT(startInsn->GetBB() == endInsn->GetBB(), "two insns must be in a same BB");
  if (analysisDone && !regGen[startInsn->GetBB()->GetId()]->TestBit(regNO)) {
    return defInsnVec;
  }

  for (Insn *insn = endInsn; insn != nullptr && insn != startInsn->GetPrev(); insn = insn->GetPrev()) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }

    if (insn->IsAsmInsn()) {
      if (SetDefInsnVecForAsm(*insn, kAsmOutputListOpnd, regNO, defInsnVec) ||
          SetDefInsnVecForAsm(*insn, kAsmClobberListOpnd, regNO, defInsnVec)) {
        if (findAll) {
          defInsnVec.emplace_back(insn);
        } else {
          return defInsnVec;
        }
      }
    }
    if (insn->IsCall() && IsRegKilledByCallInsn(*insn, regNO)) {
      defInsnVec.emplace_back(insn);
      if (!findAll) {
        return defInsnVec;
      }
    }
    if (insn->IsRegDefined(regNO)) {
      defInsnVec.emplace_back(insn);
      if (!findAll) {
        return defInsnVec;
      }
    }
  }
  return defInsnVec;
}

bool ReachingDefinition::RegIsUsedOrDefBetweenInsn(uint32 regNO, Insn &startInsn, Insn &endInsn) const {
  ASSERT(&startInsn != &endInsn, "startInsn is not equal to endInsn");
  if (startInsn.GetBB() == endInsn.GetBB()) {
    /* register is difined more than once */
    if (startInsn.GetId() > endInsn.GetId()) {
      return false;
    }
    if (startInsn.GetNext() == &endInsn) {
      return true;
    }
    if (regGen[startInsn.GetBB()->GetId()]->TestBit(regNO) &&
        !FindRegDefBetweenInsn(regNO, startInsn.GetNext(), endInsn.GetPrev()).empty()) {
      return false;
    }
    if (regUse[startInsn.GetBB()->GetId()]->TestBit(regNO)) {
      InsnSet useInsnSet;
      FindRegUseBetweenInsn(regNO, startInsn.GetNext(), endInsn.GetPrev(), useInsnSet);
      if (!useInsnSet.empty()) {
        return false;
      }
    }
    if (IsCallerSavedReg(regNO) && KilledByCallBetweenInsnInSameBB(*startInsn.GetNext(), *endInsn.GetPrev(), regNO)) {
      return false;
    }
    return true;
  }

  if (&startInsn != startInsn.GetBB()->GetLastInsn() &&
      regGen[startInsn.GetBB()->GetId()]->TestBit(regNO) &&
      !FindRegDefBetweenInsn(regNO, startInsn.GetNext(), startInsn.GetBB()->GetLastInsn()).empty()) {
    return false;
  }

  if (regUse[startInsn.GetBB()->GetId()]->TestBit(regNO)) {
    InsnSet useInsnSet;
    FindRegUseBetweenInsn(regNO, startInsn.GetNext(), startInsn.GetBB()->GetLastInsn(), useInsnSet);
    if (!useInsnSet.empty()) {
      return false;
    }
  }

  if (&startInsn != startInsn.GetBB()->GetLastInsn() &&
      IsCallerSavedReg(regNO) &&
      KilledByCallBetweenInsnInSameBB(*startInsn.GetNext(), *startInsn.GetBB()->GetLastInsn(), regNO)) {
    return false;
  }

  if (&endInsn != endInsn.GetBB()->GetFirstInsn() &&
      regGen[endInsn.GetBB()->GetId()]->TestBit(regNO) &&
      !FindRegDefBetweenInsn(regNO, endInsn.GetBB()->GetFirstInsn(), endInsn.GetPrev()).empty()) {
    return false;
  }

  if (regUse[startInsn.GetBB()->GetId()]->TestBit(regNO)) {
    InsnSet useInsnSet;
    FindRegUseBetweenInsn(regNO, endInsn.GetBB()->GetFirstInsn(), endInsn.GetPrev(), useInsnSet);
    if (!useInsnSet.empty()) {
      return false;
    }
  }

  if (&endInsn != endInsn.GetBB()->GetFirstInsn() &&
      IsCallerSavedReg(regNO) &&
      KilledByCallBetweenInsnInSameBB(*endInsn.GetBB()->GetFirstInsn(), *endInsn.GetPrev(), regNO)) {
    return false;
  }

  std::vector<bool> visitedBB(kMaxBBNum, false);
  return IsUseOrDefInAllPathBB(regNO, *startInsn.GetBB(), *endInsn.GetBB(), visitedBB);
}

/* check whether register may be redefined form in the same BB */
bool ReachingDefinition::IsUseOrDefBetweenInsn(uint32 regNO, const BB &curBB,
                                               const Insn &startInsn, Insn &endInsn) const {
  if (regGen[curBB.GetId()]->TestBit(regNO)) {
    if (!FindRegDefBetweenInsn(regNO, startInsn.GetNext(), endInsn.GetPrev()).empty()) {
      return false;
    }
  }
  if (regUse[curBB.GetId()]->TestBit(regNO)) {
    InsnSet useInsnSet;
    FindRegUseBetweenInsn(regNO, startInsn.GetNext(), endInsn.GetPrev(), useInsnSet);
    if (!useInsnSet.empty()) {
      return false;
    }
  }
  return true;
}

/* check whether register may be redefined form startBB to endBB */
bool ReachingDefinition::IsUseOrDefInAllPathBB(uint32 regNO, const BB &startBB, const BB &endBB,
                                               std::vector<bool> &visitedBB) const {
  for (auto succ : startBB.GetSuccs()) {
    if (visitedBB[succ->GetId()] || succ == &endBB) {
      continue;
    }
    visitedBB[succ->GetId()] = true;
    std::vector<bool> traversedPathSet(kMaxBBNum, false);
    bool canReachEndBB = true;
    if (regGen[succ->GetId()]->TestBit(regNO) || regUse[succ->GetId()]->TestBit(regNO) ||
        (succ->HasCall() && IsCallerSavedReg(regNO))) {
      canReachEndBB = CanReachEndBBFromCurrentBB(*succ, endBB, traversedPathSet);
      if (canReachEndBB) {
        return false;
      }
    }
    if (!canReachEndBB) {
      continue;
    }
    bool isLive = IsUseOrDefInAllPathBB(regNO, *succ, endBB, visitedBB);
    if (!isLive) {
      return false;
    }
  }

  for (auto ehSucc : startBB.GetEhSuccs()) {
    if (visitedBB[ehSucc->GetId()]) {
      continue;
    }
    visitedBB[ehSucc->GetId()] = true;
    std::vector<bool> traversedPathSet(kMaxBBNum, false);
    bool canReachEndBB = true;
    if (regGen[ehSucc->GetId()]->TestBit(regNO) || regUse[ehSucc->GetId()]->TestBit(regNO)) {
      canReachEndBB = CanReachEndBBFromCurrentBB(*ehSucc, endBB, traversedPathSet);
      if (canReachEndBB) {
        return false;
      }
    }
    if (!canReachEndBB) {
      continue;
    }
    bool isLive = IsUseOrDefInAllPathBB(regNO, *ehSucc, endBB, visitedBB);
    if (!isLive) {
      return false;
    }
  }
  return true;
}

bool ReachingDefinition::HasCallBetweenInsnInSameBB(const Insn &startInsn, const Insn &endInsn) const {
  ASSERT(startInsn.GetBB() == endInsn.GetBB(), "two insns must be in same bb");
  for (const Insn *insn = &startInsn; insn != endInsn.GetNext(); insn = insn->GetNext()) {
    if (insn->IsMachineInstruction() && insn->IsCall()) {
      return true;
    }
  }
  return false;
}

/* operand is only defined in startBB, and only used in endBB.
 * so traverse from endBB to startBB, all paths reach startBB finally.
 * startBB and endBB are different, and call insns in both of them are not counted.
 * whether startBB and endBB are in a loop is not counted.
 */
bool ReachingDefinition::HasCallInPath(const BB &startBB, const BB &endBB, std::vector<bool> &visitedBB) const {
  ASSERT(&startBB != &endBB, "startBB and endBB are not counted");
  std::queue<const BB*> bbQueue;
  bbQueue.push(&endBB);
  visitedBB[endBB.GetId()] = true;
  while (!bbQueue.empty()) {
    const BB *bb = bbQueue.front();
    bbQueue.pop();
    for (auto predBB : bb->GetPreds()) {
      if (predBB == &startBB || visitedBB[predBB->GetId()]) {
        continue;
      }
      if (predBB->HasCall()) {
        return true;
      }
      visitedBB[predBB->GetId()] = true;
      bbQueue.push(predBB);
    }
    for (auto ehPredBB : bb->GetEhPreds()) {
      if (ehPredBB == &startBB || visitedBB[ehPredBB->GetId()]) {
        continue;
      }
      if (ehPredBB->HasCall()) {
        return true;
      }
      visitedBB[ehPredBB->GetId()] = true;
      bbQueue.push(ehPredBB);
    }
  }
  return false;
}

/* because of time cost, this function is not precise, BB in loop is not counted */
bool ReachingDefinition::HasCallBetweenDefUse(const Insn &defInsn, const Insn &useInsn) const {
  if (defInsn.GetBB()->GetId() == useInsn.GetBB()->GetId()) {
    if (&useInsn == defInsn.GetNext()) {
      return false;
    }
    if (useInsn.GetId() > defInsn.GetId()) {
      return HasCallBetweenInsnInSameBB(defInsn, *useInsn.GetPrev());
    }
    /* useInsn is in front of defInsn, we think there is call insn between them conservatively */
    return true;
  }
  /* check defInsn->GetBB() */
  if (&defInsn != defInsn.GetBB()->GetLastInsn() &&
      defInsn.GetBB()->HasCall() &&
      HasCallBetweenInsnInSameBB(*defInsn.GetNext(), *defInsn.GetBB()->GetLastInsn())) {
    return true;
  }
  /* check useInsn->GetBB() */
  if (&useInsn != useInsn.GetBB()->GetFirstInsn() &&
      useInsn.GetBB()->HasCall() &&
      HasCallBetweenInsnInSameBB(*useInsn.GetBB()->GetFirstInsn(), *useInsn.GetPrev())) {
    return true;
  }
  std::vector<bool> visitedBB(kMaxBBNum, false);
  return HasCallInPath(*defInsn.GetBB(), *useInsn.GetBB(), visitedBB);
}

void ReachingDefinition::EnlargeRegCapacity(uint32 size) {
  FOR_ALL_BB(bb, cgFunc) {
    regIn[bb->GetId()]->EnlargeCapacityToAdaptSize(size);
    regOut[bb->GetId()]->EnlargeCapacityToAdaptSize(size);
    regGen[bb->GetId()]->EnlargeCapacityToAdaptSize(size);
    regUse[bb->GetId()]->EnlargeCapacityToAdaptSize(size);
  }
}

void ReachingDefinition::DumpInfo(const BB &bb, DumpType flag) const {
  const DataInfo *info = nullptr;
  switch (flag) {
    case kDumpRegGen:
      LogInfo::MapleLogger() << "    regGen:\n";
      info = regGen[bb.GetId()];
      break;
    case kDumpRegUse:
      LogInfo::MapleLogger() << "    regUse:\n";
      info = regUse[bb.GetId()];
      break;
    case kDumpRegIn:
      LogInfo::MapleLogger() << "    regIn:\n";
      info = regIn[bb.GetId()];
      break;
    case kDumpRegOut:
      LogInfo::MapleLogger() << "    regOut:\n";
      info = regOut[bb.GetId()];
      break;
    case kDumpMemGen:
      LogInfo::MapleLogger() << "    memGen:\n";
      info = memGen[bb.GetId()];
      break;
    case kDumpMemIn:
      LogInfo::MapleLogger() << "    memIn:\n";
      info = memIn[bb.GetId()];
      break;
    case kDumpMemOut:
      LogInfo::MapleLogger() << "    memOut:\n";
      info = memOut[bb.GetId()];
      break;
    case kDumpMemUse:
      LogInfo::MapleLogger() << "    memUse:\n";
      info = memUse[bb.GetId()];
      break;
    default:
      return;
  }
  ASSERT(info != nullptr, "null ptr check");
  uint32 count = 1;
  LogInfo::MapleLogger() << "        ";
  for (uint32 i = 0; i != info->Size(); ++i) {
    if (info->TestBit(i)) {
      count += 1;
      if (kDumpMemGen <= flag && flag <= kDumpMemUse) {
        /* Each element i means a 4 byte stack slot. */
        LogInfo::MapleLogger() << (i * 4) << " ";
      } else {
        LogInfo::MapleLogger() << i << " ";
      }
      /* 10 output per line */
      if (count % 10 == 0) {
        LogInfo::MapleLogger() << "\n";
        LogInfo::MapleLogger() << "        ";
      }
    }
  }

  LogInfo::MapleLogger() << "\n";
}

void ReachingDefinition::DumpBBCGIR(const BB &bb) const {
  if (bb.IsCleanup()) {
    LogInfo::MapleLogger() << "[is_cleanup] ";
  }
  if (bb.IsUnreachable()) {
    LogInfo::MapleLogger() << "[unreachable] ";
  }
  if (bb.GetSuccs().size() != 0) {
    LogInfo::MapleLogger() << "      succs: ";
    for (auto *succBB : bb.GetSuccs()) {
      LogInfo::MapleLogger() << succBB->GetId() << " ";
    }
  }
  if (bb.GetEhSuccs().size() != 0) {
    LogInfo::MapleLogger() << "      eh_succs: ";
    for (auto *ehSuccBB : bb.GetEhSuccs()) {
      LogInfo::MapleLogger() << ehSuccBB->GetId() << " ";
    }
  }
  LogInfo::MapleLogger() << "\n";

  FOR_BB_INSNS_CONST(insn, &bb) {
    LogInfo::MapleLogger() << "        ";
    insn->Dump();
  }
  LogInfo::MapleLogger() << "\n";
}

void ReachingDefinition::Dump(uint32 flag) const {
  MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(cgFunc->GetFunction().GetStIdx().Idx());
  ASSERT(mirSymbol != nullptr, "get symbol in function failed in ReachingDefinition::Dump");
  LogInfo::MapleLogger() << "\n----  Reaching definition analysis for " << mirSymbol->GetName();
  LogInfo::MapleLogger() << " ----\n";
  FOR_ALL_BB(bb, cgFunc) {
    LogInfo::MapleLogger() << "  === BB_" << bb->GetId() << " ===\n";

    if ((flag & kDumpBBCGIR) != 0) {
      DumpBBCGIR(*bb);
    }

    if ((flag & kDumpRegIn) != 0) {
      DumpInfo(*bb, kDumpRegIn);
    }

    if ((flag & kDumpRegUse) != 0) {
      DumpInfo(*bb, kDumpRegUse);
    }

    if ((flag & kDumpRegGen) != 0) {
      DumpInfo(*bb, kDumpRegGen);
    }

    if ((flag & kDumpRegOut) != 0) {
      DumpInfo(*bb, kDumpRegOut);
    }

    if ((flag & kDumpMemIn) != 0) {
      DumpInfo(*bb, kDumpMemIn);
    }

    if ((flag & kDumpMemGen) != 0) {
      DumpInfo(*bb, kDumpMemGen);
    }

    if ((flag & kDumpMemOut) != 0) {
      DumpInfo(*bb, kDumpMemOut);
    }

    if ((flag & kDumpMemUse) != 0) {
      DumpInfo(*bb, kDumpMemUse);
    }
  }
  LogInfo::MapleLogger() << "------------------------------------------------------\n";
}

bool CgReachingDefinition::PhaseRun(maplebe::CGFunc &f) {
#if TARGAARCH64 || TARGRISCV64
  reachingDef = GetPhaseAllocator()->New<AArch64ReachingDefinition>(f, *GetPhaseMemPool());
#endif
#if defined(TARGARM32) && TARGARM32
  reachingDef = GetPhaseAllocator()->New<Arm32ReachingDefinition>(f, *GetPhaseMemPool());
#endif
  reachingDef->SetAnalysisMode(kRDAllAnalysis);
  reachingDef->AnalysisStart();
  return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgReachingDefinition, reachingdefinition)

bool CgClearRDInfo::PhaseRun(maplebe::CGFunc &f) {
  if (f.GetRDStatus()) {
    f.GetRD()->ClearDefUseInfo();
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgClearRDInfo, clearrdinfo)
}  /* namespace maplebe */
