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

/* ################# "peephole" gTest ###############
* ####################################################################### */

#include <climits>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include "cg_phasemanager.h"
#include "cg.h"
#include "peep.h"
#include "common_utils.h"
#include "aarch64_peep.h"
#include "triple.h"
#include "mir_preg.h"
#include "gtest/gtest.h"
#include "instrument.h"

using namespace maplebe;
#define TARGAARCH64 1

class Init {
 public:
  Init(MemPool *usePool, MapleAllocator *funcScopeAllocator)
      : bb(usePool->New<maplebe::BB>(0, *funcScopeAllocator)),
        mirModule(usePool->New<MIRModule>()),
        patternMap(usePool->New<std::unordered_map<std::string, std::vector<std::string>>>()),
        opts(usePool->New<CGOptions>()),
        nameVec(usePool->New<std::vector<std::string>>()),
        aArch64Cg(usePool->New<AArch64CG>(*mirModule, *opts, *nameVec, *patternMap)),
        mirFunc(mirModule->GetMIRBuilder()->GetOrCreateFunction("mod_hash", TyIdx(PTY_void))),
        beCommon(usePool->New<BECommon>(*mirModule)),
        stackMemPool(usePool->New<StackMemPool>(memPoolCtrler, "stname")),
        mirPreg(usePool->New<MIRPreg>()),
        localMapleAllocator((usePool->New<LocalMapleAllocator>(*stackMemPool))) {
    mirFunc->AllocSymTab();
    mirFunc->AllocPregTab();
    mirFunc->AllocLabelTab();
    aarchCGFunc = usePool->New<AArch64CGFunc>(*mirModule, *aArch64Cg, *mirFunc,
                                              *beCommon, *usePool,
                                              *stackMemPool,
                                              *funcScopeAllocator, 0);
    aarchCGFunc->SetCurBB(*bb);
    Globals::GetInstance()->SetTarget(*aArch64Cg);
  }
  ~Init() {}

  AArch64CGFunc *GetAArch64CGFunc() {
    return aarchCGFunc;
  }

 private:
  maplebe::BB *bb = nullptr;
  MIRModule *mirModule = nullptr;
  std::unordered_map<std::string, std::vector<std::string>> *patternMap = nullptr;
  CGOptions *opts = nullptr;
  std::vector<std::string> *nameVec = nullptr;
  AArch64CG *aArch64Cg = nullptr;
  MIRFunction *mirFunc = nullptr;
  BECommon *beCommon = nullptr;
  StackMemPool *stackMemPool = nullptr;
  MIRPreg *mirPreg = nullptr;
  LocalMapleAllocator *localMapleAllocator = nullptr;
  AArch64CGFunc *aarchCGFunc = nullptr;
};

/* test peep pattern
 * combine {rev / rev16} & {tbz / tbnz} ---> {tbz / tbnz}
 * rev16  w0, w0
 * tbz    w0, #16    ===>   tbz w0, #24
*/
TEST(peep, normRevTbzToTbzPattern16) {
  Triple::GetTriple().Init();
  maple::MemPool *usePool = new MemPool(memPoolCtrler, "usepool");
  maple::MapleAllocator *funcScopeAllocator = new MapleAllocator(usePool);
  Init init(usePool, funcScopeAllocator);
  AArch64CGFunc *aarchCGFunc = init.GetAArch64CGFunc();

  // rev16  R0, R0
  MOperator mOp = MOP_wrevrr16;
  RegOperand &regOpnd0 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R0, maple::k64BitSize, kRegTyInt, 0);
  maplebe::Insn &insn = aarchCGFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd0, regOpnd0);
  aarchCGFunc->GetCurBB()->AppendInsn(insn);

  // tbz    R0, #16
  MOperator mOp1 = MOP_wtbz;
  RegOperand &regOpnd1 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R0, maple::k64BitSize, kRegTyInt);
  ImmOperand ImmOpnd0(maplebe::k16BitSize, maplebe::k6BitSize, 0);
  LabelOperand &labelOpnd = aarchCGFunc->GetOpndBuilder()->CreateLabel(".L.", maple::k1BitSize, usePool);
  maplebe::Insn &insn2 = aarchCGFunc->GetInsnBuilder()->BuildInsn(mOp1, regOpnd1, ImmOpnd0, labelOpnd);
  aarchCGFunc->GetCurBB()->AppendInsn(insn2);

  // test NormRevTbzToTbzPattern
  NormRevTbzToTbzPattern normrevTbzToTbzPattern(*aarchCGFunc, *aarchCGFunc->GetCurBB(), insn);
  normrevTbzToTbzPattern.Run(*aarchCGFunc->GetCurBB(), insn);
  Insn *newInsn = aarchCGFunc->GetCurBB()->GetLastMachineInsn();
  MOperator mop = newInsn->GetMachineOpcode();
  RegOperand opnd1 = static_cast<RegOperand&>(newInsn->GetOperand(kInsnFirstOpnd));
  ImmOperand &newImmOpnd1 = static_cast<ImmOperand&>(newInsn->GetOperand(kInsnSecondOpnd));
  ASSERT_EQ(mop, MOP_wtbz);
  ASSERT_EQ(opnd1.GetRegisterNumber(), R0);
  ASSERT_EQ(newImmOpnd1.GetValue(), maplebe::k24BitSize);
  delete usePool;
  delete funcScopeAllocator;
}

TEST(peep, normRevTbzToTbzPattern40) {
  Triple::GetTriple().Init();
  maple::MemPool *usePool = new MemPool(memPoolCtrler, "usepool");
  maple::MapleAllocator *funcScopeAllocator = new MapleAllocator(usePool);
  Init init(usePool, funcScopeAllocator);
  AArch64CGFunc *aarchCGFunc = init.GetAArch64CGFunc();

  MOperator mOp = MOP_wrevrr16;
  RegOperand &regOpnd0 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R0, maple::k64BitSize, kRegTyInt, 0);
  maplebe::Insn &insn = aarchCGFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd0, regOpnd0);
  aarchCGFunc->GetCurBB()->AppendInsn(insn);

  MOperator mOp1 = MOP_xtbz;
  RegOperand &regOpnd1 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R0, maple::k64BitSize, kRegTyInt);
  ImmOperand ImmOpnd0(maplebe::k40BitSize, maplebe::k6BitSize, 0);
  LabelOperand &labelOpnd = aarchCGFunc->GetOpndBuilder()->CreateLabel(".L.", maple::k1BitSize, usePool);
  maplebe::Insn &insn2 = aarchCGFunc->GetInsnBuilder()->BuildInsn(mOp1, regOpnd1, ImmOpnd0, labelOpnd);
  aarchCGFunc->GetCurBB()->AppendInsn(insn2);

  NormRevTbzToTbzPattern normrevTbzToTbzPattern(*aarchCGFunc, *aarchCGFunc->GetCurBB(), insn);
  normrevTbzToTbzPattern.Run(*aarchCGFunc->GetCurBB(), insn);
  Insn *newInsn = aarchCGFunc->GetCurBB()->GetLastMachineInsn();
  MOperator mop = newInsn->GetMachineOpcode();
  RegOperand opnd1 = static_cast<RegOperand&>(newInsn->GetOperand(kInsnFirstOpnd));
  ImmOperand &newImmOpnd1 = static_cast<ImmOperand&>(newInsn->GetOperand(kInsnSecondOpnd));
  ASSERT_EQ(mop, MOP_xtbz);
  ASSERT_EQ(opnd1.GetRegisterNumber(), R0);
  ASSERT_EQ(newImmOpnd1.GetValue(), maplebe::k32BitSize);
  delete usePool;
  delete funcScopeAllocator;
}

TEST(peep, normRevTbzToTbzPattern56) {
  Triple::GetTriple().Init();
  maple::MemPool *usePool = new MemPool(memPoolCtrler, "usepool");
  maple::MapleAllocator *funcScopeAllocator = new MapleAllocator(usePool);
  Init init(usePool, funcScopeAllocator);
  AArch64CGFunc *aarchCGFunc = init.GetAArch64CGFunc();

  MOperator mOp = MOP_wrevrr16;
  RegOperand &regOpnd0 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R0, maple::k64BitSize, kRegTyInt, 0);
  maplebe::Insn &insn = aarchCGFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd0, regOpnd0);
  aarchCGFunc->GetCurBB()->AppendInsn(insn);

  MOperator mOp1 = MOP_xtbz;
  RegOperand &regOpnd1 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R0, maple::k64BitSize, kRegTyInt);
  ImmOperand ImmOpnd0(maplebe::k56BitSize, maplebe::k6BitSize, 0);
  LabelOperand &labelOpnd = aarchCGFunc->GetOpndBuilder()->CreateLabel(".L.", maple::k1BitSize, usePool);
  maplebe::Insn &insn2 = aarchCGFunc->GetInsnBuilder()->BuildInsn(mOp1, regOpnd1, ImmOpnd0, labelOpnd);
  aarchCGFunc->GetCurBB()->AppendInsn(insn2);

  NormRevTbzToTbzPattern normrevTbzToTbzPattern(*aarchCGFunc, *aarchCGFunc->GetCurBB(), insn);
  normrevTbzToTbzPattern.Run(*aarchCGFunc->GetCurBB(), insn);
  Insn *newInsn = aarchCGFunc->GetCurBB()->GetLastMachineInsn();
  MOperator mop = newInsn->GetMachineOpcode();
  RegOperand opnd1 = static_cast<RegOperand&>(newInsn->GetOperand(kInsnFirstOpnd));
  ImmOperand &newImmOpnd1 = static_cast<ImmOperand&>(newInsn->GetOperand(kInsnSecondOpnd));
  ASSERT_EQ(mop, MOP_xtbz);
  ASSERT_EQ(opnd1.GetRegisterNumber(), R0);
  ASSERT_EQ(newImmOpnd1.GetValue(), maplebe::k48BitSize);
  delete usePool;
  delete funcScopeAllocator;
}

TEST(peep, normRevTbzToTbzPattern0) {
  Triple::GetTriple().Init();
  maple::MemPool *usePool = new MemPool(memPoolCtrler, "usepool");
  maple::MapleAllocator *funcScopeAllocator = new MapleAllocator(usePool);
  Init init(usePool, funcScopeAllocator);
  AArch64CGFunc *aarchCGFunc = init.GetAArch64CGFunc();

  MOperator mOp = MOP_wrevrr16;
  RegOperand &regOpnd0 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R0, maple::k64BitSize, kRegTyInt, 0);
  maplebe::Insn &insn = aarchCGFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd0, regOpnd0);
  aarchCGFunc->GetCurBB()->AppendInsn(insn);

  MOperator mOp1 = MOP_wtbz;
  RegOperand &regOpnd1 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R0, maple::k64BitSize, kRegTyInt);
  ImmOperand ImmOpnd0(maplebe::k0BitSize, maplebe::k6BitSize, 0);
  LabelOperand &labelOpnd = aarchCGFunc->GetOpndBuilder()->CreateLabel(".L.", maple::k1BitSize, usePool);
  maplebe::Insn &insn2 = aarchCGFunc->GetInsnBuilder()->BuildInsn(mOp1, regOpnd1, ImmOpnd0, labelOpnd);
  aarchCGFunc->GetCurBB()->AppendInsn(insn2);

  NormRevTbzToTbzPattern normrevTbzToTbzPattern(*aarchCGFunc, *aarchCGFunc->GetCurBB(), insn);
  normrevTbzToTbzPattern.Run(*aarchCGFunc->GetCurBB(), insn);
  Insn *newInsn = aarchCGFunc->GetCurBB()->GetLastMachineInsn();
  MOperator mop = newInsn->GetMachineOpcode();
  RegOperand opnd1 = static_cast<RegOperand&>(newInsn->GetOperand(kInsnFirstOpnd));
  ImmOperand &newImmOpnd1 = static_cast<ImmOperand&>(newInsn->GetOperand(kInsnSecondOpnd));
  ASSERT_EQ(mop, MOP_wtbz);
  ASSERT_EQ(opnd1.GetRegisterNumber(), R0);
  ASSERT_EQ(newImmOpnd1.GetValue(), maplebe::k8BitSize);
  delete usePool;
  delete funcScopeAllocator;
}
