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
#ifndef MAPLEBE_INCLUDE_CG_MEMLAYOUT_H
#define MAPLEBE_INCLUDE_CG_MEMLAYOUT_H

/* C++ headers. */
#include <utility>
#include "becommon.h"
#include "mir_function.h"
#include "mir_nodes.h"  /* StmtNode */

namespace maplebe {
using regno_t = uint32;
enum MemSegmentKind : uint8 {
  kMsUnknown,
  /*
   * Function arguments that are not passed through registers
   * are passed to the callee through stack.
   */
  kMsArgsStkPassed,
  /*
   * In between MS_args_stackpassed and kMsArgsRegpassed,
   * we store call-saved registers if any.
   */
  /*
   * Args passed via registers according to the architecture-specific ABI
   * may need be stored in stack.
   * 1) In the unoptimized version, we implement a model (similar to GCC -O0)
   *    where all the values are initially stored in the memory and
   *    loaded into registers when needed, and stored back to the memory when
   *    their uses are done.
   * 2) In an optimized version, some register-passed values may need to be
   *    spilled into memory. We allocate the space in this Memory segment.
   *    (or we may allocate them in caller-saved; may be this is better...)
   */
  kMsArgsRegPassed,
  /*
   * GR/VR Save areas for unnamed arguments under vararg functions
   */
  kMsGrSaveArea,
  kMsVrSaveArea,
  /* local (auto) variables */
  kMsRefLocals,
  kMsLocals,
  kMsSpillReg,
  /*
   * In between kMsLocals and MS_args_to_stackpass, we allocate
   * a register-spill area and space for caller-saved registers
   */
  /*
   * When a function calls another which takes some arguments
   * that cannot be passed through registers, it is the caller's
   * responsibility to allocate space for those arguments in memory.
   */
  kMsArgsToStkPass,
  /* The red zone stack area will not be modified by the exception signal. */
  kMsRedZone,
};

enum StackProtectKind : uint8 {
  kNone = 0,
  kAddrofStack = 0x1,
  /* if a callee has return agg type which size over 16bytes */
  kRetureStackSlot = 0x2,
};

class CGFunc;

/* keeps track of the allocation of a memory segment */
class MemSegment {
 public:
  explicit MemSegment(MemSegmentKind memSegKind) : kind(memSegKind), size(0) {}

  ~MemSegment() = default;

  uint32 GetSize() const {
    return size;
  }

  void SetSize(uint32 memSize) {
    size = memSize;
  }

  MemSegmentKind GetMemSegmentKind() const {
    return kind;
  }

 private:
  MemSegmentKind kind;
  uint32 size;  /* size is negative if allocated offsets are negative */
};  /* class MemSegment */

/* describes where a symbol is allocated */
class SymbolAlloc {
 public:
  SymbolAlloc() = default;

  ~SymbolAlloc() = default;

  const MemSegment *GetMemSegment() const {
    return memSegment;
  }

  void SetMemSegment(const MemSegment &memSeg) {
    memSegment = &memSeg;
  }

  int64 GetOffset() const {
    return offset;
  }

  void SetOffset(int64 off) {
    offset = off;
  }

 protected:
  const MemSegment *memSegment = nullptr;
  int64 offset = 0;
};  /* class SymbolAlloc */

class MemLayout {
 public:
  MemLayout(BECommon &beCommon, MIRFunction &mirFunc, MapleAllocator &mallocator)
      : be(beCommon),
        mirFunction(&mirFunc),
        segArgsStkPassed(kMsArgsStkPassed),
        segArgsRegPassed(kMsArgsRegPassed),
        segArgsToStkPass(kMsArgsToStkPass),
        symAllocTable(mallocator.Adapter()),
        spillLocTable(mallocator.Adapter()),
        spillRegLocMap(mallocator.Adapter()),
        localRefLocMap(std::less<StIdx>(), mallocator.Adapter()),
        memAllocator(&mallocator) {
    symAllocTable.resize(mirFunc.GetSymTab()->GetSymbolTableSize());
  }

  virtual ~MemLayout() = default;

  void SetCurrFunction(CGFunc &func) {
    cgFunc = &func;
  }

  /*
   * Returns stack space required for a call
   * which is used to pass arguments that cannot be
   * passed through registers
   */
  virtual uint32 ComputeStackSpaceRequirementForCall(StmtNode &stmtNode, int32 &aggCopySize, bool isIcall) = 0;

  /*
   * Go over all outgoing calls in the function body and get the maximum space
   * needed for storing the actuals based on the actual parameters and the ABI.
   * These are usually those arguments that cannot be passed
   * through registers because a call passes more than 8 arguments, or
   * they cannot be fit in a pair of registers.
   */
  uint32 FindLargestActualArea(int32 &aggCopySize);

  virtual void LayoutStackFrame(int32 &structCopySize, int32 &maxParmStackSize) = 0;

  /*
   * "Pseudo-registers can be regarded as local variables of a
   * primitive type whose addresses are never taken"
   */
  virtual void AssignSpillLocationsToPseudoRegisters() = 0;

  virtual SymbolAlloc *AssignLocationToSpillReg(regno_t vrNum) = 0;

  virtual int32 GetCalleeSaveBaseLoc() const {
    return 0;
  }

  SymbolAlloc *GetSymAllocInfo(uint32 stIdx) {
    ASSERT(stIdx < symAllocTable.size(), "out of symAllocTable's range");
    return symAllocTable[stIdx];
  }

  void SetSymAllocInfo(uint32 stIdx, SymbolAlloc &symAlloc) {
    ASSERT(stIdx < symAllocTable.size(), "out of symAllocTable's range");
    symAllocTable[stIdx] = &symAlloc;
  }

  const SymbolAlloc *GetSpillLocOfPseduoRegister(PregIdx index) const {
    return spillLocTable.at(index);
  }

  SymbolAlloc *GetLocOfSpillRegister(regno_t vrNum) {
    SymbolAlloc *loc = nullptr;
    auto pos = spillRegLocMap.find(vrNum);
    if (pos == spillRegLocMap.end()) {
      loc = AssignLocationToSpillReg(vrNum);
    } else {
      loc = pos->second;
    }
    return loc;
  }

  uint32 SizeOfArgsToStackPass() const {
    return segArgsToStkPass.GetSize();
  }

  uint32 SizeOfArgsRegisterPassed() const {
    return segArgsRegPassed.GetSize();
  }

  BECommon &GetBECommon() {
    return be;
  }

  MIRFunction *GetMIRFunction() {
    return mirFunction;
  }

  const MemSegment &GetSegArgsStkPassed() const {
    return segArgsStkPassed;
  }

  const MemSegment &GetSegArgsRegPassed() const {
    return segArgsRegPassed;
  }

  const MemSegment &GetSegArgsToStkPass() const {
    return segArgsToStkPass;
  }

  const MapleVector<SymbolAlloc*> &GetSymAllocTable() const {
    return symAllocTable;
  }

  void SetSpillRegLocInfo(regno_t regNum, SymbolAlloc &symAlloc) {
    spillRegLocMap[regNum] = &symAlloc;
  }

  const MapleMap<StIdx, SymbolAlloc*> &GetLocalRefLocMap() const {
    return localRefLocMap;
  }

  void SetLocalRegLocInfo(StIdx idx, SymbolAlloc &symAlloc) {
    localRefLocMap[idx] = &symAlloc;
  }

  bool IsLocalRefLoc(const MIRSymbol &symbol) const {
    return localRefLocMap.find(symbol.GetStIdx()) != localRefLocMap.end();
  }

 protected:
  BECommon &be;
  MIRFunction *mirFunction;
  MemSegment segArgsStkPassed;
  MemSegment segArgsRegPassed;
  MemSegment segArgsToStkPass;
  MapleVector<SymbolAlloc*> symAllocTable;  /* index is stindex from StIdx */
  MapleVector<SymbolAlloc*> spillLocTable;  /* index is preg idx */
  MapleUnorderedMap<regno_t, SymbolAlloc*> spillRegLocMap;
  MapleMap<StIdx, SymbolAlloc*> localRefLocMap;  /* localrefvar formals. real address passed in stack. */
  MapleAllocator *memAllocator;
  CGFunc *cgFunc = nullptr;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_MEMLAYOUT_H */
