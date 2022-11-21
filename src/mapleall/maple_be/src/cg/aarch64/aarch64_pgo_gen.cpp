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
#include "aarch64_pgo_gen.h"
#include "aarch64_cg.h"
namespace maplebe {

void AArch64ProfGen::CreateClearIcall(BB &bb, const std::string &symName) {
  auto *mirBuilder = f->GetFunction().GetModule()->GetMIRBuilder();
  auto *voidPtrType = GlobalTables::GetTypeTable().GetVoidPtr();
  auto *funcPtrSym = mirBuilder->GetOrCreateGlobalDecl(symName, *voidPtrType);
  funcPtrSym->SetAttr(ATTR_weak);  // weak symbol
  CHECK_FATAL(!bb.IsEmpty() || bb.IsUnreachable(), "empty first BB?");

  auto *a64Func = static_cast<AArch64CGFunc*>(f);
  RegOperand &tempReg = a64Func->GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
  StImmOperand &funcPtrSymOpnd = a64Func->CreateStImmOperand(*funcPtrSym, 0, 0);
  auto &adrpInsn = f->GetInsnBuilder()->BuildInsn(MOP_xadrp, tempReg, funcPtrSymOpnd);

  Insn *firstI = bb.GetFirstMachineInsn();

  while (firstI && firstI->GetNextMachineInsn() && firstI->GetNextMachineInsn()->IsStore()) {
    firstI = firstI->GetPreviousMachineInsn();
  }

  if (firstI) {
    (void)bb.InsertInsnAfter(*firstI, adrpInsn);
  } else {
    bb.InsertInsnBegin(adrpInsn);
  }

  /* load func ptr */
  OfstOperand &funcPtrSymOfst = a64Func->CreateOfstOpnd(*funcPtrSym, 0, 0);
  MemOperand *ldrOpnd = a64Func->CreateMemOperand(k64BitSize, tempReg, funcPtrSymOfst, *funcPtrSym);
  auto &ldrSymInsn = f->GetInsnBuilder()->BuildInsn(MOP_xldr, tempReg, *ldrOpnd);
  (void)bb.InsertInsnAfter(adrpInsn, ldrSymInsn);

  Insn *getAddressInsn = &ldrSymInsn;
  if (CGOptions::IsPIC()) {
    MemOperand *ldrOpndPic = a64Func->CreateMemOperand(k64BitSize, tempReg,
                                                       a64Func->CreateImmOperand(0, maplebe::k32BitSize, false));
    auto &ldrSymPicInsn = f->GetInsnBuilder()->BuildInsn(MOP_xldr, tempReg, *ldrOpndPic);
    (void)bb.InsertInsnAfter(ldrSymInsn, ldrSymPicInsn);
    getAddressInsn = &ldrSymPicInsn;
  }
  /* call weak symbol */
  auto &bInsn = f->GetInsnBuilder()->BuildInsn(MOP_xblr, tempReg);
  (void)bb.InsertInsnAfter(*getAddressInsn, bInsn);
}

void AArch64ProfGen::CreateIcallForWeakSymbol(BB &bb, const std::string &symName) {
  auto *mirBuilder = f->GetFunction().GetModule()->GetMIRBuilder();
  auto *voidPtrType = GlobalTables::GetTypeTable().GetVoidPtr();
  auto *funcPtrSym = mirBuilder->GetOrCreateGlobalDecl(symName, *voidPtrType);
  funcPtrSym->SetAttr(ATTR_weak);  // weak symbol

  CHECK_FATAL(!bb.IsEmpty() || bb.IsUnreachable(), "empty exit BB?");

  Insn *lastI = bb.GetLastMachineInsn();
  if (lastI) {
    CHECK_FATAL(lastI->GetMachineOpcode() == MOP_xret || lastI->GetMachineOpcode() == MOP_tail_call_opt_xbl,
        "check this return bb");
  }

  while (lastI && lastI->GetPreviousMachineInsn() &&
      (lastI->GetPreviousMachineInsn()->IsLoad() ||
      lastI->GetMachineOpcode() == MOP_pseudo_ret_int || lastI->GetMachineOpcode() == MOP_pseudo_ret_float)) {
    lastI = lastI->GetPreviousMachineInsn();
  }

  auto *a64Func = static_cast<AArch64CGFunc*>(f);
  /* load func ptr page */
  RegOperand &tempReg = a64Func->GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
  StImmOperand &funcPtrSymOpnd = a64Func->CreateStImmOperand(*funcPtrSym, 0, 0);
  auto &adrpInsn = f->GetInsnBuilder()->BuildInsn(MOP_xadrp, tempReg, funcPtrSymOpnd);

  if (lastI) {
    (void)bb.InsertInsnBefore(*lastI, adrpInsn);
  } else {
    bb.InsertInsnBegin(adrpInsn);
  }

  /* load func ptr */
  OfstOperand &funcPtrSymOfst = a64Func->CreateOfstOpnd(*funcPtrSym, 0, 0);
  MemOperand *ldrOpnd = a64Func->CreateMemOperand(k64BitSize, tempReg, funcPtrSymOfst, *funcPtrSym);
  Insn &ldrSymInsn = f->GetInsnBuilder()->BuildInsn(MOP_xldr, tempReg, *ldrOpnd);
  (void)bb.InsertInsnAfter(adrpInsn, ldrSymInsn);

  Insn *getAddressInsn = &ldrSymInsn;
  if (CGOptions::IsPIC()) {
    MemOperand *ldrOpndPic = a64Func->CreateMemOperand(k64BitSize, tempReg,
                                                       a64Func->CreateImmOperand(0, maplebe::k32BitSize, false));
    CHECK_FATAL(ldrOpndPic != nullptr, "create mem failed");
    Insn &ldrSymPicInsn = f->GetInsnBuilder()->BuildInsn(MOP_xldr, tempReg, *ldrOpndPic);
    (void)bb.InsertInsnAfter(ldrSymInsn, ldrSymPicInsn);
    getAddressInsn = &ldrSymPicInsn;
  }
  /* call weak symbol */
  Insn &bInsn = f->GetInsnBuilder()->BuildInsn(MOP_xblr, tempReg);
  (void)bb.InsertInsnAfter(*getAddressInsn, bInsn);
}

void AArch64ProfGen::InstrumentBB(BB &bb, MIRSymbol &countTab, uint32 offset) {
  auto *a64Func = static_cast<AArch64CGFunc*>(f);
  StImmOperand &funcPtrSymOpnd = a64Func->CreateStImmOperand(countTab, 0, 0);
  CHECK_FATAL(offset <= kMaxPimm8, "profile BB number out of range!!!");
  /* skip size */
  uint32 ofStByteSize = (offset + 1U) << 3U;
  ImmOperand &countOfst = a64Func->CreateImmOperand(ofStByteSize, k64BitSize, false);
  auto &counterInsn = f->GetInsnBuilder()->BuildInsn(MOP_c_counter, funcPtrSymOpnd, countOfst);
  bb.InsertInsnBegin(counterInsn);
}
}
