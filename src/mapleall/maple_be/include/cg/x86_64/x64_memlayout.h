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

#ifndef MAPLEBE_INCLUDE_CG_X86_64_MEMLAYOUT_H
#define MAPLEBE_INCLUDE_CG_X86_64_MEMLAYOUT_H

#include "memlayout.h"
#include "x64_abi.h"

namespace maplebe {
class X64SymbolAlloc : public SymbolAlloc {
 public:
  X64SymbolAlloc() = default;

  ~X64SymbolAlloc() = default;

  void SetRegisters(bool isR) {
    isRegister = isR;
  }

  inline bool IsRegister() const {
    return isRegister;
  }

 private:
  bool isRegister = false;
};
/*
 * On X64, stack frames are structured as follows:
 *
 * The stack grows downward -- full descending (SP points
 * to a filled slot).
 *
 * Any of the parts of a frame is optional, i.e., it is
 * possible to write a caller-callee pair in such a way
 * that the particular part is absent in the frame.
 *
 * Before a call is made, the frame looks like:
 * |                            |
 * ||----------------------------|
 * | args passed on the stack   | (we call them up-formals)
 * ||----------------------------|<- Stack Pointer
 * |                            |
 *
 * Right after a call is made
 * |                            |
 * ||----------------------------|
 * | args passed on the stack   |
 * ||----------------------------|<- Stack Pointer
 * | PREV_FP, PREV_LR           |
 * ||----------------------------|<- Frame Pointer
 *
 * After the prologue has run,
 * |                            |
 * ||----------------------------|
 * | args passed on the stack   |
 * ||----------------------------|
 * | PREV_FP, PREV_LR           |
 * ||----------------------------|<- Frame Pointer
 * | GR Arg Save Area           |
 * ||----------------------------|
 * | VR Arg Save Area           |
 * ||----------------------------|
 * | callee-saved registers     |
 * ||----------------------------|
 * | empty space. should have   |
 * | at least 16-byte alignment |
 * ||----------------------------|
 * | local variables            |
 * ||----------------------------|<- Stack Pointer
 * | red zone                   |
 *
 * callee-saved registers include
 *  1. rbx rbp r12 r14 r14 r15
 *  2. XMM0-XMM7
 */

class X64MemLayout : public MemLayout {
 public:
  X64MemLayout(BECommon &b, MIRFunction &f, MapleAllocator &mallocator)
      : MemLayout(b, f, mallocator, kX64StackPtrAlignment) {}

  ~X64MemLayout() override = default;

  uint32 ComputeStackSpaceRequirementForCall(StmtNode &stmtNode, int32 &aggCopySize, bool isIcall) override;
  void LayoutStackFrame(int32 &structCopySize, int32 &maxParmStackSize) override;

  uint64 StackFrameSize() const;

  const MemSegment &Locals() const {
    return segLocals;
  }
  /*
   * "Pseudo-registers can be regarded as local variables of a
   * primitive type whose addresses are never taken"
   */
  virtual void AssignSpillLocationsToPseudoRegisters() override;

  uint32 GetSizeOfSpillReg() const {
    return segSpillReg.GetSize();
  }

  uint32 GetSizeOfLocals() const {
    return segLocals.GetSize();
  }

  void SetSizeOfGRSaveArea(uint32 sz) {
    segGrSaveArea.SetSize(sz);
  }

  uint32 GetSizeOfGRSaveArea() const {
    return segGrSaveArea.GetSize();
  }

  inline void SetSizeOfVRSaveArea(uint32 sz) {
    segVrSaveArea.SetSize(sz);
  }

  uint32 GetSizeOfVRSaveArea() const {
    return segVrSaveArea.GetSize();
  }

  int32 GetGRSaveAreaBaseLoc();
  int32 GetVRSaveAreaBaseLoc();
 private:
  /* Layout function */
  void LayoutFormalParams();
  void LayoutLocalVariables();
  void LayoutVarargParams();

  /* util function */
  void SetSizeAlignForTypeIdx(uint32 typeIdx, uint32 &size, uint32 &align) const;
  void LayoutReturnRef(int32 &structCopySize, int32 &maxParmStackSize);

  MemSegment segLocals = MemSegment(kMsLocals);  /* these are accessed via Frame Pointer */
  MemSegment segGrSaveArea = MemSegment(kMsGrSaveArea);
  MemSegment segVrSaveArea = MemSegment(kMsVrSaveArea);

  SymbolAlloc *CreateSymbolAlloc() const override {
    return memAllocator->GetMemPool()->New<X64SymbolAlloc>();
  }
};
}
#endif // MAPLEBE_INCLUDE_CG_X86_64_MEMLAYOUT_H
