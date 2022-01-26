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
#include "aarch64_insn.h"
#include <fstream>
#include "aarch64_cg.h"
#include "insn.h"
#include "metadata_layout.h"

namespace maplebe {
namespace {
constexpr uint32 kClinitInsnCount = 4;
constexpr uint32 kAdrpLdrInsnCount = 2;
constexpr uint32 kLazyBindingRoutineInsnCount = 1;
constexpr uint32 kClinitTailInsnCount = 2;
constexpr uint32 kLazyLdrInsnCount = 2;
constexpr uint32 kLazyLdrStaticInsnCount = 3;
constexpr uint32 kCheckThrowPendingExceptionInsnCount = 5;
constexpr uint32 kArrayClassCacheLoadCount = 3;
}

uint32 AArch64Insn::GetResultNum() const {
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  uint32 resNum = 0;
  for (size_t i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsDef()) {
      ++resNum;
    }
  }
  return resNum;
}

uint32 AArch64Insn::GetOpndNum() const {
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  uint32 srcNum = 0;
  for (size_t i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsUse()) {
      ++srcNum;
    }
  }
  return srcNum;
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
void AArch64Insn::EmitCompareAndSwapInt(Emitter &emitter) const {
  /* MOP_compare_and_swapI and MOP_compare_and_swapL have 8 operands */
  ASSERT(opnds.size() > kInsnEighthOpnd, "ensure the operands number");
  const MOperator mOp = GetMachineOpcode();
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  Operand *temp0 = opnds[kInsnSecondOpnd];
  Operand *temp1 = opnds[kInsnThirdOpnd];
  Operand *obj = opnds[kInsnFourthOpnd];
  Operand *offset = opnds[kInsnFifthOpnd];
  /* add       xt, x1, x2 */
  emitter.Emit("\tadd\t");
  temp0->Emit(emitter, nullptr);
  emitter.Emit(", ");
  obj->Emit(emitter, nullptr);
  emitter.Emit(", ");
  offset->Emit(emitter, nullptr);
  emitter.Emit("\n");
  Operand *label1 = opnds[kInsnEighthOpnd];
  /* label1: */
  label1->Emit(emitter, nullptr);
  emitter.Emit(":\n");
  /* ldaxr     ws, [xt] */
  emitter.Emit("\tldaxr\t");
  temp1->Emit(emitter, nullptr);
  emitter.Emit(", [");
  temp0->Emit(emitter, nullptr);
  emitter.Emit("]\n");
  Operand *expectedValue = opnds[kInsnSixthOpnd];
  OpndProp *expectedValueProp = md->operand[kInsnSixthOpnd];
  /* cmp       ws, w3 */
  emitter.Emit("\tcmp\t");
  temp1->Emit(emitter, nullptr);
  emitter.Emit(", ");
  expectedValue->Emit(emitter, expectedValueProp);
  emitter.Emit("\n");
  constexpr uint32 kInsnNinethOpnd = 8;
  Operand *label2 = opnds[kInsnNinethOpnd];
  /* b.ne      label2 */
  emitter.Emit("\tbne\t");
  label2->Emit(emitter, nullptr);
  emitter.Emit("\n");
  Operand *newValue = opnds[kInsnSeventhOpnd];
  /* stlxr     ws, w4, [xt] */
  emitter.Emit("\tstlxr\t");
  emitter.Emit(AArch64CG::intRegNames[AArch64CG::kR32List][static_cast<RegOperand*>(temp1)->GetRegisterNumber()]);
  emitter.Emit(", ");
  newValue->Emit(emitter, nullptr);
  emitter.Emit(", [");
  temp0->Emit(emitter, nullptr);
  emitter.Emit("]\n");
  /* cbnz      ws, label1 */
  emitter.Emit("\tcbnz\t");
  emitter.Emit(AArch64CG::intRegNames[AArch64CG::kR32List][static_cast<RegOperand*>(temp1)->GetRegisterNumber()]);
  emitter.Emit(", ");
  label1->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* label2: */
  label2->Emit(emitter, nullptr);
  emitter.Emit(":\n");
  Operand *retVal = opnds[kInsnFirstOpnd];
  /* cset      x0, eq */
  emitter.Emit("\tcset\t");
  retVal->Emit(emitter, nullptr);
  emitter.Emit(", EQ\n");
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
void AArch64Insn::EmitStringIndexOf(Emitter &emitter) const {
  /* MOP_string_indexof has 18 operands */
  ASSERT(opnds.size() == 18, "ensure the operands number");
  Operand *patternLengthOpnd = opnds[kInsnFifthOpnd];
  Operand *srcLengthOpnd = opnds[kInsnThirdOpnd];
  const std::string patternLengthReg =
      AArch64CG::intRegNames[AArch64CG::kR64List][static_cast<RegOperand*>(patternLengthOpnd)->GetRegisterNumber()];
  const std::string srcLengthReg =
      AArch64CG::intRegNames[AArch64CG::kR64List][static_cast<RegOperand*>(srcLengthOpnd)->GetRegisterNumber()];
  /* cmp       w4, w2 */
  emitter.Emit("\tcmp\t");
  patternLengthOpnd->Emit(emitter, nullptr);
  emitter.Emit(", ");
  srcLengthOpnd->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* the 16th operand of MOP_string_indexof is Label.NOMATCH */
  Operand *labelNoMatch = opnds[16];
  /* b.gt      Label.NOMATCH */
  emitter.Emit("\tb.gt\t");
  labelNoMatch->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* sub       w2, w2, w4 */
  emitter.Emit("\tsub\t");
  srcLengthOpnd->Emit(emitter, nullptr);
  emitter.Emit(", ");
  srcLengthOpnd->Emit(emitter, nullptr);
  emitter.Emit(", ");
  patternLengthOpnd->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* sub       w4, w4, #8 */
  emitter.Emit("\tsub\t");
  patternLengthOpnd->Emit(emitter, nullptr);
  emitter.Emit(", ");
  patternLengthOpnd->Emit(emitter, nullptr);
  emitter.Emit(", #8\n");
  /* the 10th operand of MOP_string_indexof is w10 */
  Operand *resultTmp = opnds[10];
  /* mov       w10, w2 */
  emitter.Emit("\tmov\t");
  resultTmp->Emit(emitter, nullptr);
  emitter.Emit(", ");
  srcLengthOpnd->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* uxtw      x4, w4 */
  emitter.Emit("\tuxtw\t").Emit(patternLengthReg);
  emitter.Emit(", ");
  patternLengthOpnd->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* uxtw      x2, w2 */
  emitter.Emit("\tuxtw\t").Emit(srcLengthReg);
  emitter.Emit(", ");
  srcLengthOpnd->Emit(emitter, nullptr);
  emitter.Emit("\n");
  Operand *patternStringBaseOpnd = opnds[kInsnFourthOpnd];
  /* add       x3, x3, x4 */
  emitter.Emit("\tadd\t");
  patternStringBaseOpnd->Emit(emitter, nullptr);
  emitter.Emit(", ");
  patternStringBaseOpnd->Emit(emitter, nullptr);
  emitter.Emit(", ").Emit(patternLengthReg);
  emitter.Emit("\n");
  Operand *srcStringBaseOpnd = opnds[kInsnSecondOpnd];
  /* add       x1, x1, x2 */
  emitter.Emit("\tadd\t");
  srcStringBaseOpnd->Emit(emitter, nullptr);
  emitter.Emit(", ");
  srcStringBaseOpnd->Emit(emitter, nullptr);
  emitter.Emit(", ").Emit(srcLengthReg);
  emitter.Emit("\n");
  /* neg       x4, x4 */
  emitter.Emit("\tneg\t").Emit(patternLengthReg);
  emitter.Emit(", ").Emit(patternLengthReg);
  emitter.Emit("\n");
  /* neg       x2, x2 */
  emitter.Emit("\tneg\t").Emit(srcLengthReg);
  emitter.Emit(", ").Emit(srcLengthReg);
  emitter.Emit("\n");
  Operand *first = opnds[kInsnSixthOpnd];
  /* ldr       x5, [x3,x4] */
  emitter.Emit("\tldr\t");
  first->Emit(emitter, nullptr);
  emitter.Emit(", [");
  patternStringBaseOpnd->Emit(emitter, nullptr);
  emitter.Emit(",").Emit(patternLengthReg);
  emitter.Emit("]\n");
  /* the 11th operand of MOP_string_indexof is Label.FIRST_LOOP */
  Operand *labelFirstLoop = opnds[11];
  /* .Label.FIRST_LOOP: */
  labelFirstLoop->Emit(emitter, nullptr);
  emitter.Emit(":\n");
  /* the 7th operand of MOP_string_indexof is x7 */
  Operand *ch2 = opnds[7];
  /* ldr       x7, [x1,x2] */
  emitter.Emit("\tldr\t");
  ch2->Emit(emitter, nullptr);
  emitter.Emit(", [");
  srcStringBaseOpnd->Emit(emitter, nullptr);
  emitter.Emit(",").Emit(srcLengthReg);
  emitter.Emit("]\n");
  /* cmp       x5, x7 */
  emitter.Emit("\tcmp\t");
  first->Emit(emitter, nullptr);
  emitter.Emit(", ");
  ch2->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* the 13th operand of MOP_string_indexof is Label.STR1_LOOP */
  Operand *labelStr1Loop = opnds[13];
  /* b.eq      .Label.STR1_LOOP */
  emitter.Emit("\tb.eq\t");
  labelStr1Loop->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* the 12th operand of MOP_string_indexof is Label.STR2_NEXT */
  Operand *labelStr2Next = opnds[12];
  /* .Label.STR2_NEXT: */
  labelStr2Next->Emit(emitter, nullptr);
  emitter.Emit(":\n");
  /* adds      x2, x2, #1 */
  emitter.Emit("\tadds\t").Emit(srcLengthReg);
  emitter.Emit(", ").Emit(srcLengthReg);
  emitter.Emit(", #1\n");
  /* b.le      .Label.FIRST_LOOP */
  emitter.Emit("\tb.le\t");
  labelFirstLoop->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* b         .Label.NOMATCH */
  emitter.Emit("\tb\t");
  labelNoMatch->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* .Label.STR1_LOOP: */
  labelStr1Loop->Emit(emitter, nullptr);
  emitter.Emit(":\n");
  /* the 8th operand of MOP_string_indexof is x8 */
  Operand *tmp1 = opnds[8];
  /* adds      x8, x4, #8 */
  emitter.Emit("\tadds\t");
  tmp1->Emit(emitter, nullptr);
  emitter.Emit(", ").Emit(patternLengthReg);
  emitter.Emit(", #8\n");
  /* the 9th operand of MOP_string_indexof is x9 */
  Operand *tmp2 = opnds[9];
  /* add       x9, x2, #8 */
  emitter.Emit("\tadd\t");
  tmp2->Emit(emitter, nullptr);
  emitter.Emit(", ").Emit(srcLengthReg);
  emitter.Emit(", #8\n");
  /* the 15th operand of MOP_string_indexof is Label.LAST_WORD */
  Operand *labelLastWord = opnds[15];
  /* b.ge      .Label.LAST_WORD */
  emitter.Emit("\tb.ge\t");
  labelLastWord->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* the 14th operand of MOP_string_indexof is Label.STR1_NEXT */
  Operand *labelStr1Next = opnds[14];
  /* .Label.STR1_NEXT: */
  labelStr1Next->Emit(emitter, nullptr);
  emitter.Emit(":\n");
  /* the 6th operand of MOP_string_indexof is x6 */
  Operand *ch1 = opnds[6];
  /* ldr       x6, [x3,x8] */
  emitter.Emit("\tldr\t");
  ch1->Emit(emitter, nullptr);
  emitter.Emit(", [");
  patternStringBaseOpnd->Emit(emitter, nullptr);
  emitter.Emit(",");
  tmp1->Emit(emitter, nullptr);
  emitter.Emit("]\n");
  /* ldr       x7, [x1,x9] */
  emitter.Emit("\tldr\t");
  ch2->Emit(emitter, nullptr);
  emitter.Emit(", [");
  srcStringBaseOpnd->Emit(emitter, nullptr);
  emitter.Emit(",");
  tmp2->Emit(emitter, nullptr);
  emitter.Emit("]\n");
  /* cmp       x6, x7 */
  emitter.Emit("\tcmp\t");
  ch1->Emit(emitter, nullptr);
  emitter.Emit(", ");
  ch2->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* b.ne      .Label.STR2_NEXT */
  emitter.Emit("\tb.ne\t");
  labelStr2Next->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* adds      x8, x8, #8 */
  emitter.Emit("\tadds\t");
  tmp1->Emit(emitter, nullptr);
  emitter.Emit(", ");
  tmp1->Emit(emitter, nullptr);
  emitter.Emit(", #8\n");
  /* add       x9, x9, #8 */
  emitter.Emit("\tadd\t");
  tmp2->Emit(emitter, nullptr);
  emitter.Emit(", ");
  tmp2->Emit(emitter, nullptr);
  emitter.Emit(", #8\n");
  /* b.lt      .Label.STR1_NEXT */
  emitter.Emit("\tb.lt\t");
  labelStr1Next->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* .Label.LAST_WORD: */
  labelLastWord->Emit(emitter, nullptr);
  emitter.Emit(":\n");
  /* ldr       x6, [x3] */
  emitter.Emit("\tldr\t");
  ch1->Emit(emitter, nullptr);
  emitter.Emit(", [");
  patternStringBaseOpnd->Emit(emitter, nullptr);
  emitter.Emit("]\n");
  /* sub       x9, x1, x4 */
  emitter.Emit("\tsub\t");
  tmp2->Emit(emitter, nullptr);
  emitter.Emit(", ");
  srcStringBaseOpnd->Emit(emitter, nullptr);
  emitter.Emit(", ").Emit(patternLengthReg);
  emitter.Emit("\n");
  /* ldr       x7, [x9,x2] */
  emitter.Emit("\tldr\t");
  ch2->Emit(emitter, nullptr);
  emitter.Emit(", [");
  tmp2->Emit(emitter, nullptr);
  emitter.Emit(", ").Emit(srcLengthReg);
  emitter.Emit("]\n");
  /* cmp       x6, x7 */
  emitter.Emit("\tcmp\t");
  ch1->Emit(emitter, nullptr);
  emitter.Emit(", ");
  ch2->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* b.ne      .Label.STR2_NEXT */
  emitter.Emit("\tb.ne\t");
  labelStr2Next->Emit(emitter, nullptr);
  emitter.Emit("\n");
  Operand *retVal = opnds[kInsnFirstOpnd];
  /* add       w0, w10, w2 */
  emitter.Emit("\tadd\t");
  retVal->Emit(emitter, nullptr);
  emitter.Emit(", ");
  resultTmp->Emit(emitter, nullptr);
  emitter.Emit(", ");
  srcLengthOpnd->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* the 17th operand of MOP_string_indexof Label.ret */
  Operand *labelRet = opnds[17];
  /* b         .Label.ret */
  emitter.Emit("\tb\t");
  labelRet->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* .Label.NOMATCH: */
  labelNoMatch->Emit(emitter, nullptr);
  emitter.Emit(":\n");
  /* mov       w0, #-1 */
  emitter.Emit("\tmov\t");
  retVal->Emit(emitter, nullptr);
  emitter.Emit(", #-1\n");
  /* .Label.ret: */
  labelRet->Emit(emitter, nullptr);
  emitter.Emit(":\n");
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
void AArch64Insn::EmitGetAndAddInt(Emitter &emitter) const {
  ASSERT(opnds.size() > kInsnEighthOpnd, "ensure the oprands number");
  emitter.Emit("\t//\tstart of Unsafe.getAndAddInt.\n");
  Operand *tempOpnd0 = opnds[kInsnSecondOpnd];
  Operand *tempOpnd1 = opnds[kInsnThirdOpnd];
  Operand *tempOpnd2 = opnds[kInsnFourthOpnd];
  Operand *objOpnd = opnds[kInsnFifthOpnd];
  Operand *offsetOpnd = opnds[kInsnSixthOpnd];
  Operand *deltaOpnd = opnds[kInsnSeventhOpnd];
  Operand *labelOpnd = opnds[kInsnEighthOpnd];
  /* emit add. */
  emitter.Emit("\t").Emit("add").Emit("\t");
  tempOpnd0->Emit(emitter, nullptr);
  emitter.Emit(", ");
  objOpnd->Emit(emitter, nullptr);
  emitter.Emit(", ");
  offsetOpnd->Emit(emitter, nullptr);
  emitter.Emit("\n");
  /* emit label. */
  labelOpnd->Emit(emitter, nullptr);
  emitter.Emit(":\n");
  Operand *retVal = opnds[kInsnFirstOpnd];
  const MOperator mOp = GetMachineOpcode();
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  OpndProp *retProp = md->operand[kInsnFirstOpnd];
  /* emit ldaxr */
  emitter.Emit("\t").Emit("ldaxr").Emit("\t");
  retVal->Emit(emitter, retProp);
  emitter.Emit(", [");
  tempOpnd0->Emit(emitter, nullptr);
  emitter.Emit("]\n");
  /* emit add. */
  emitter.Emit("\t").Emit("add").Emit("\t");
  tempOpnd1->Emit(emitter, retProp);
  emitter.Emit(", ");
  retVal->Emit(emitter, retProp);
  emitter.Emit(", ");
  deltaOpnd->Emit(emitter, retProp);
  emitter.Emit("\n");
  /* emit stlxr. */
  emitter.Emit("\t").Emit("stlxr").Emit("\t");
  tempOpnd2->Emit(emitter, nullptr);
  emitter.Emit(", ");
  tempOpnd1->Emit(emitter, retProp);
  emitter.Emit(", [");
  tempOpnd0->Emit(emitter, nullptr);
  emitter.Emit("]\n");
  /* emit cbnz. */
  emitter.Emit("\t").Emit("cbnz").Emit("\t");
  tempOpnd2->Emit(emitter, nullptr);
  emitter.Emit(", ");
  labelOpnd->Emit(emitter, nullptr);
  emitter.Emit("\n");
  emitter.Emit("\t//\tend of Unsafe.getAndAddInt.\n");
}

/*
 * intrinsic_get_set_int w0, xt, ws, x1, x2, w3, label
 * add    xt, x1, x2
 * label:
 * ldaxr  w0, [xt]
 * stlxr  ws, w3, [xt]
 * cbnz   ws, label
 */
void AArch64Insn::EmitGetAndSetInt(Emitter &emitter) const {
  /* MOP_get_and_setI and MOP_get_and_setL have 7 operands */
  ASSERT(opnds.size() > kInsnSeventhOpnd, "ensure the operands number");
  Operand *tempOpnd0 = opnds[kInsnSecondOpnd];
  Operand *tempOpnd1 = opnds[kInsnThirdOpnd];
  Operand *objOpnd = opnds[kInsnFourthOpnd];
  Operand *offsetOpnd = opnds[kInsnFifthOpnd];
  /* add    x1, x1, x2 */
  emitter.Emit("\tadd\t");
  tempOpnd0->Emit(emitter, nullptr);
  emitter.Emit(", ");
  objOpnd->Emit(emitter, nullptr);
  emitter.Emit(", ");
  offsetOpnd->Emit(emitter, nullptr);
  emitter.Emit("\n");
  Operand *labelOpnd = opnds[kInsnSeventhOpnd];
  /* label: */
  labelOpnd->Emit(emitter, nullptr);
  emitter.Emit(":\n");
  Operand *retVal = opnds[kInsnFirstOpnd];
  /* ldaxr  w0, [xt] */
  emitter.Emit("\tldaxr\t");
  retVal->Emit(emitter, nullptr);
  emitter.Emit(", [");
  tempOpnd0->Emit(emitter, nullptr);
  emitter.Emit("]\n");
  Operand *newValueOpnd = opnds[kInsnSixthOpnd];
  /* stlxr  ws, w3, [xt] */
  emitter.Emit("\tstlxr\t");
  tempOpnd1->Emit(emitter, nullptr);
  emitter.Emit(", ");
  newValueOpnd->Emit(emitter, nullptr);
  emitter.Emit(", [");
  tempOpnd0->Emit(emitter, nullptr);
  emitter.Emit("]\n");
  /* cbnz   w2, label */
  emitter.Emit("\tcbnz\t");
  tempOpnd1->Emit(emitter, nullptr);
  emitter.Emit(", ");
  labelOpnd->Emit(emitter, nullptr);
  emitter.Emit("\n");
}

void AArch64Insn::EmitCounter(const CG &cg, Emitter &emitter) const {
  /*
   * adrp    x1, __profile_bb_table$$GetBoolean_dex+4
   * ldr     w17, [x1, #:lo12:__profile_bb_table$$GetBoolean_dex+4]
   * add     w17, w17, #1
   * str     w17, [x1, #:lo12:__profile_bb_table$$GetBoolean_dex+4]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_counter];

  Operand *opnd0 = opnds[kInsnFirstOpnd];
  Operand *opnd1 = opnds[kInsnSecondOpnd];
  OpndProp *prop0 = md->operand[kInsnFirstOpnd];
  StImmOperand *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Insn::EmitCounter");
  /* emit nop for breakpoint */
  if (cg.GetCGOptions().WithDwarf()) {
    emitter.Emit("\t").Emit("nop").Emit("\n");
  }

  /* emit adrp */
  emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(",");
  emitter.Emit(stImmOpnd->GetName());
  emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  emitter.Emit("\n");
  /* emit ldr */
  emitter.Emit("\t").Emit("ldr").Emit("\tw17, [");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(",");
  emitter.Emit("#");
  emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
  emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  emitter.Emit("]");
  emitter.Emit("\n");
  /* emit add */
  emitter.Emit("\t").Emit("add").Emit("\tw17, w17, #1");
  emitter.Emit("\n");
  /* emit str */
  emitter.Emit("\t").Emit("str").Emit("\tw17, [");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(",");
  emitter.Emit("#");
  emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
  emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  emitter.Emit("]");
  emitter.Emit("\n");
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
    auto tenth = static_cast<char>(newRegno / kDecimalMax);
    strToEmit += '0' + tenth;
    newRegno -= (kDecimalMax * tenth);
  }
  strToEmit += newRegno + '0';
}

void AArch64Insn::EmitInlineAsm(const CG &cg, Emitter &emitter) const {
  (void)cg;
  emitter.Emit("\t//Inline asm begin\n\t");
  auto &list1 = static_cast<AArch64ListOperand&>(GetOperand(kAsmOutputListOpnd));
  std::vector<RegOperand *> outOpnds;
  for (auto *regOpnd : list1.GetOperands()) {
    outOpnds.push_back(regOpnd);
  }
  auto &list2 = static_cast<AArch64ListOperand&>(GetOperand(kAsmInputListOpnd));
  std::vector<RegOperand *> inOpnds;
  for (auto *regOpnd : list2.GetOperands()) {
    inOpnds.push_back(regOpnd);
  }
  auto &list6 = static_cast<ListConstraintOperand&>(GetOperand(kAsmOutputRegPrefixOpnd));
  auto &list7 = static_cast<ListConstraintOperand&>(GetOperand(kAsmInputRegPrefixOpnd));
  MapleString asmStr = static_cast<StringOperand&>(this->GetOperand(kAsmStringOpnd)).GetComment();
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
  for (int i = 0; i < asmStr.length(); ++i) {
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
  emitter.Emit(stringToEmit);
  emitter.Emit("\n\t//Inline asm end\n");
}

void AArch64Insn::EmitClinit(const CG &cg, Emitter &emitter) const {
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

  Operand *opnd0 = opnds[0];
  Operand *opnd1 = opnds[1];
  OpndProp *prop0 = md->operand[0];
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Insn::EmitClinit");
  /* emit nop for breakpoint */
  if (cg.GetCGOptions().WithDwarf()) {
    emitter.Emit("\t").Emit("nop").Emit("\n");
  }

  if (stImmOpnd->GetSymbol()->IsMuidDataUndefTab()) {
    /* emit adrp */
    emitter.Emit("\t").Emit("adrp").Emit("\t");
    opnd0->Emit(emitter, prop0);
    emitter.Emit(",");
    emitter.Emit(stImmOpnd->GetName());
    emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    emitter.Emit("\n");
    /* emit ldr */
    emitter.Emit("\t").Emit("ldr").Emit("\t");
    opnd0->Emit(emitter, prop0);
    emitter.Emit(",");
    emitter.Emit("[");
    opnd0->Emit(emitter, prop0);
    emitter.Emit(",");
    emitter.Emit("#");
    emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
    emitter.Emit("+").Emit(stImmOpnd->GetOffset());
    emitter.Emit("]");
    emitter.Emit("\n");
  } else {
    /* adrp    x3, _PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B */
    emitter.Emit("\tadrp\t");
    opnd0->Emit(emitter, prop0);
    emitter.Emit(",");
    (void)emitter.Emit(namemangler::kPtrPrefixStr + stImmOpnd->GetName());
    emitter.Emit("\n");

    /* ldr     x3, [x3, #:lo12:_PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B] */
    emitter.Emit("\tldr\t");
    opnd0->Emit(emitter, prop0);
    emitter.Emit(", [");
    opnd0->Emit(emitter, prop0);
    emitter.Emit(", #:lo12:");
    (void)emitter.Emit(namemangler::kPtrPrefixStr + stImmOpnd->GetName());
    emitter.Emit("]\n");
  }
  /* emit "ldr  x0,[x0,#48]" */
  emitter.Emit("\t").Emit("ldr").Emit("\t");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(",");
  emitter.Emit("[");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(",#");
  emitter.Emit(static_cast<uint32>(ClassMetadata::OffsetOfInitState()));
  emitter.Emit("]");
  emitter.Emit("\n");

  /* emit "ldr  xzr, [x0]" */
  emitter.Emit("\t").Emit("ldr\txzr, [");
  opnd0->Emit(emitter, prop0);
  emitter.Emit("]\n");
}

void AArch64Insn::EmitAdrpLdr(const CG &cg, Emitter &emitter) const {
  /*
   * adrp    xd, _PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B
   * ldr     xd, [xd, #:lo12:_PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_adrp_ldr];

  Operand *opnd0 = opnds[0];
  Operand *opnd1 = opnds[1];
  OpndProp *prop0 = md->operand[0];
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Insn::EmitAdrpLdr");
  /* emit nop for breakpoint */
  if (cg.GetCGOptions().WithDwarf()) {
    emitter.Emit("\t").Emit("nop").Emit("\n");
  }

  /* adrp    xd, _PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B */
  emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(", ");
  emitter.Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  emitter.Emit("\n");

  /* ldr     xd, [xd, #:lo12:_PTR__cinf_Ljava_2Futil_2Fconcurrent_2Fatomic_2FAtomicInteger_3B] */
  emitter.Emit("\tldr\t");
  static_cast<AArch64RegOperand*>(opnd0)->SetRefField(true);
  opnd0->Emit(emitter, prop0);
  static_cast<AArch64RegOperand*>(opnd0)->SetRefField(false);
  emitter.Emit(", ");
  emitter.Emit("[");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(",");
  emitter.Emit("#");
  emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  emitter.Emit("]\n");
}

void AArch64Insn::EmitAdrpLabel(Emitter &emitter) const {
  /* adrp    xd, label
   * add     xd, xd, #lo12:label
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_adrp_label];

  Operand *opnd0 = opnds[0];
  Operand *opnd1 = opnds[1];
  OpndProp *prop0 = static_cast<AArch64OpndProp*>(md->operand[0]);
  auto lidx = static_cast<ImmOperand *>(opnd1)->GetValue();

  /* adrp    xd, label */
  emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(", ");
  const char *idx;
  idx = strdup(std::to_string(Globals::GetInstance()->GetBECommon()->GetMIRModule().CurFunction()->GetPuidx()).c_str());
  emitter.Emit(".L.").Emit(idx).Emit("__").Emit(lidx).Emit("\n");

  /* add     xd, xd, #lo12:label */
  emitter.Emit("\tadd\t");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(", ");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(", ");
  emitter.Emit(":lo12:").Emit(".L.").Emit(idx).Emit("__").Emit(lidx).Emit("\n");
  emitter.Emit("\n");
}

void AArch64Insn::EmitLazyBindingRoutine(Emitter &emitter) const {
  /* ldr xzr, [xs] */
  const AArch64MD *md = &AArch64CG::kMd[MOP_adrp_ldr];

  Operand *opnd0 = opnds[0];
  OpndProp *prop0 = md->operand[0];

  /* emit "ldr  xzr,[xs]" */
#ifdef USE_32BIT_REF
  emitter.Emit("\t").Emit("ldr").Emit("\twzr, [");
#else
  emitter.Emit("\t").Emit("ldr").Emit("\txzr, [");
#endif /* USE_32BIT_REF */
  opnd0->Emit(emitter, prop0);
  emitter.Emit("]");
  emitter.Emit("\t// Lazy binding\n");
}

void AArch64Insn::EmitClinitTail(Emitter &emitter) const {
  /*
   * ldr x17, [xs, #112]
   * ldr wzr, [x17]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_clinit_tail];

  Operand *opnd0 = opnds[0];
  OpndProp *prop0 = md->operand[0];

  /* emit "ldr  x17,[xs,#112]" */
  emitter.Emit("\t").Emit("ldr").Emit("\tx17, [");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(", #");
  emitter.Emit(static_cast<uint32>(ClassMetadata::OffsetOfInitState()));
  emitter.Emit("]");
  emitter.Emit("\n");

  /* emit "ldr  xzr, [x17]" */
  emitter.Emit("\t").Emit("ldr\txzr, [x17]\n");
}

void AArch64Insn::EmitLazyLoad(Emitter &emitter) const {
  /*
   * ldr wd, [xs]  # xd and xs should be differenct register
   * ldr wd, [xd]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_lazy_ldr];

  Operand *opnd0 = opnds[0];
  Operand *opnd1 = opnds[1];
  OpndProp *prop0 = md->operand[0];
  OpndProp *prop1 = md->operand[1];

  /* emit  "ldr wd, [xs]" */
  emitter.Emit("\t").Emit("ldr\t");
#ifdef USE_32BIT_REF
  opnd0->Emit(emitter, prop0);
#else
  opnd0->Emit(emitter, prop1);
#endif
  emitter.Emit(", [");
  opnd1->Emit(emitter, prop1);
  emitter.Emit("]\t// lazy load.\n");

  /* emit "ldr wd, [xd]" */
  emitter.Emit("\t").Emit("ldr\t");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(", [");
  opnd0->Emit(emitter, prop1);
  emitter.Emit("]\t// lazy load.\n");
}

void AArch64Insn::EmitLazyLoadStatic(Emitter &emitter) const {
  /* adrp xd, :got:__staticDecoupleValueOffset$$xxx+offset
   * ldr wd, [xd, #:got_lo12:__staticDecoupleValueOffset$$xxx+offset]
   * ldr wzr, [xd]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_lazy_ldr_static];

  Operand *opnd0 = opnds[0];
  Operand *opnd1 = opnds[1];
  AArch64OpndProp *prop0 = md->GetOperand(0);
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Insn::EmitLazyLoadStatic");

  /* emit "adrp xd, :got:__staticDecoupleValueOffset$$xxx+offset" */
  emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(", ");
  emitter.Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  emitter.Emit("\t// lazy load static.\n");

  /* emit "ldr wd, [xd, #:got_lo12:__staticDecoupleValueOffset$$xxx+offset]" */
  emitter.Emit("\tldr\t");
  static_cast<AArch64RegOperand*>(opnd0)->SetRefField(true);
#ifdef USE_32BIT_REF
  AArch64OpndProp prop2(prop0->GetOperandType(), prop0->GetRegProp(), prop0->GetSize() / 2);
  opnd0->Emit(emitter, &prop2); /* ldr wd, ... for emui */
#else
  opnd0->Emit(emitter, prop0);  /* ldr xd, ... for qemu */
#endif /* USE_32BIT_REF */
  static_cast<AArch64RegOperand*>(opnd0)->SetRefField(false);
  emitter.Emit(", ");
  emitter.Emit("[");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(",");
  emitter.Emit("#");
  emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  emitter.Emit("]\t// lazy load static.\n");

  /* emit "ldr wzr, [xd]" */
  emitter.Emit("\t").Emit("ldr\twzr, [");
  opnd0->Emit(emitter, prop0);
  emitter.Emit("]\t// lazy load static.\n");
}

void AArch64Insn::EmitArrayClassCacheLoad(Emitter &emitter) const {
  /* adrp xd, :got:__arrayClassCacheTable$$xxx+offset
   * ldr wd, [xd, #:got_lo12:__arrayClassCacheTable$$xxx+offset]
   * ldr wzr, [xd]
   */
  const AArch64MD *md = &AArch64CG::kMd[MOP_arrayclass_cache_ldr];
  uint32 opndIndex = 0;
  uint32 propIndex = 0;
  Operand *opnd0 = opnds[opndIndex++];
  Operand *opnd1 = opnds[opndIndex++];
  AArch64OpndProp *prop0 = md->GetOperand(propIndex++);
  auto *stImmOpnd = static_cast<StImmOperand*>(opnd1);
  CHECK_FATAL(stImmOpnd != nullptr, "stImmOpnd is null in AArch64Insn::EmitLazyLoadStatic");

  /* emit "adrp xd, :got:__arrayClassCacheTable$$xxx+offset" */
  emitter.Emit("\t").Emit("adrp").Emit("\t");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(", ");
  emitter.Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  emitter.Emit("\t// load array class.\n");

  /* emit "ldr wd, [xd, #:got_lo12:__arrayClassCacheTable$$xxx+offset]" */
  emitter.Emit("\tldr\t");
  static_cast<AArch64RegOperand*>(opnd0)->SetRefField(true);
#ifdef USE_32BIT_REF
  AArch64OpndProp prop2(prop0->GetOperandType(), prop0->GetRegProp(), prop0->GetSize() / 2);
  opnd0->Emit(emitter, &prop2); /* ldr wd, ... for emui */
#else
  opnd0->Emit(emitter, prop0);  /* ldr xd, ... for qemu */
#endif /* USE_32BIT_REF */
  static_cast<AArch64RegOperand*>(opnd0)->SetRefField(false);
  emitter.Emit(", ");
  emitter.Emit("[");
  opnd0->Emit(emitter, prop0);
  emitter.Emit(",");
  emitter.Emit("#");
  emitter.Emit(":lo12:").Emit(stImmOpnd->GetName());
  if (stImmOpnd->GetOffset() != 0) {
    emitter.Emit("+").Emit(stImmOpnd->GetOffset());
  }
  emitter.Emit("]\t// load array class.\n");

  /* emit "ldr wzr, [xd]" */
  emitter.Emit("\t").Emit("ldr\twzr, [");
  opnd0->Emit(emitter, prop0);
  emitter.Emit("]\t// check resolve array class.\n");
}

void AArch64Insn::EmitCheckThrowPendingException(const CG& cg, Emitter &emitter) const {
  /*
   * mrs x16, TPIDR_EL0
   * ldr x16, [x16, #64]
   * ldr x16, [x16, #8]
   * cbz x16, .lnoexception
   * bl MCC_ThrowPendingException
   * .lnoexception:
   */
  emitter.Emit("\t").Emit("mrs").Emit("\tx16, TPIDR_EL0");
  emitter.Emit("\n");
  emitter.Emit("\t").Emit("ldr").Emit("\tx16, [x16, #64]");
  emitter.Emit("\n");
  emitter.Emit("\t").Emit("ldr").Emit("\tx16, [x16, #8]");
  emitter.Emit("\n");
  emitter.Emit("\t").Emit("cbz").Emit("\tx16, .lnoeh.").Emit(cg.GetCurCGFunc()->GetName());
  emitter.Emit("\n");
  emitter.Emit("\t").Emit("bl").Emit("\tMCC_ThrowPendingException");
  emitter.Emit("\n");
  emitter.Emit(".lnoeh.").Emit(cg.GetCurCGFunc()->GetName()).Emit(":");
  emitter.Emit("\n");
}

void AArch64Insn::PrepareVectorOperand(AArch64RegOperand *regOpnd, uint32 &compositeOpnds) const {
  AArch64Insn *insn = const_cast<AArch64Insn*>(this);
  VectorRegSpec* vecSpec = static_cast<AArch64VectorInsn*>(insn)->GetAndRemoveRegSpecFromList();
  compositeOpnds = vecSpec->compositeOpnds ? vecSpec->compositeOpnds : compositeOpnds;
  regOpnd->SetVecLanePosition(vecSpec->vecLane);
  switch (mOp) {
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

void AArch64Insn::Emit(const CG &cg, Emitter &emitter) const {
  emitter.SetCurrentMOP(mOp);
  const AArch64MD *md = &AArch64CG::kMd[mOp];

  if (!cg.GenerateVerboseAsm() && !cg.GenerateVerboseCG() && mOp == MOP_comment) {
    return;
  }

  switch (mOp) {
    case MOP_clinit: {
      EmitClinit(cg, emitter);
      emitter.IncreaseJavaInsnCount(kClinitInsnCount);
      return;
    }
    case MOP_adrp_ldr: {
      emitter.IncreaseJavaInsnCount(kAdrpLdrInsnCount);
      EmitAdrpLdr(cg, emitter);
      if (CGOptions::IsLazyBinding() && !cg.IsLibcore()) {
        EmitLazyBindingRoutine(emitter);
        emitter.IncreaseJavaInsnCount(kLazyBindingRoutineInsnCount + kAdrpLdrInsnCount);
      }
      return;
    }
    case MOP_counter: {
      EmitCounter(cg, emitter);
      return;
    }
    case MOP_asm: {
      EmitInlineAsm(cg, emitter);
      return;
    }
    case MOP_clinit_tail: {
      EmitClinitTail(emitter);
      emitter.IncreaseJavaInsnCount(kClinitTailInsnCount);
      return;
    }
    case MOP_lazy_ldr: {
      EmitLazyLoad(emitter);
      emitter.IncreaseJavaInsnCount(kLazyLdrInsnCount);
      return;
    }
    case MOP_adrp_label: {
      EmitAdrpLabel(emitter);
      return;
    }
    case MOP_lazy_tail: {
      /* No need to emit this pseudo instruction. */
      return;
    }
    case MOP_lazy_ldr_static: {
      EmitLazyLoadStatic(emitter);
      emitter.IncreaseJavaInsnCount(kLazyLdrStaticInsnCount);
      return;
    }
    case MOP_arrayclass_cache_ldr: {
      EmitArrayClassCacheLoad(emitter);
      emitter.IncreaseJavaInsnCount(kArrayClassCacheLoadCount);
      return;
    }
    case MOP_get_and_addI:
    case MOP_get_and_addL: {
      EmitGetAndAddInt(emitter);
      return;
    }
    case MOP_get_and_setI:
    case MOP_get_and_setL: {
      EmitGetAndSetInt(emitter);
      return;
    }
    case MOP_compare_and_swapI:
    case MOP_compare_and_swapL: {
      EmitCompareAndSwapInt(emitter);
      return;
    }
    case MOP_string_indexof: {
      EmitStringIndexOf(emitter);
      return;
    }
    case MOP_pseudo_none: {
      return;
    }
    default:
      break;
  }

  if (CGOptions::IsNativeOpt() && mOp == MOP_xbl) {
    auto *nameOpnd = static_cast<FuncNameOperand*>(opnds[0]);
    if (nameOpnd->GetName() == "MCC_CheckThrowPendingException") {
      EmitCheckThrowPendingException(cg, emitter);
      emitter.IncreaseJavaInsnCount(kCheckThrowPendingExceptionInsnCount);
      return;
    }
  }

  std::string format(md->format);
  emitter.Emit("\t").Emit(md->name).Emit("\t");
  size_t opndSize = GetOperandSize();
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

  bool isRefField = (opndSize == 0) ? false : CheckRefField(static_cast<size_t>(static_cast<uint>(seq[0])), true);
  if (mOp != MOP_comment) {
    emitter.IncreaseJavaInsnCount();
  }
  uint32 compositeOpnds = 0;
  for (uint32 i = 0; i < commaNum; ++i) {
    if (seq[i] == -1) {
      continue;
    }
    if (prefix[i].length() > 0) {
      emitter.Emit(prefix[i]);
    }
    if (emitter.NeedToDealWithHugeSo() && (mOp ==  MOP_xbl || mOp == MOP_tail_call_opt_xbl)) {
      auto *nameOpnd = static_cast<FuncNameOperand*>(opnds[0]);
      /* Suport huge so here
       * As the PLT section is just before java_text section, when java_text section is larger
       * then 128M, instrunction of "b" and "bl" would fault to branch to PLT stub functions. Here, to save
       * instuctions space, we change the branch target to a local target within 120M address, and add non-plt
       * call to the target function.
       */
      emitter.InsertHugeSoTarget(nameOpnd->GetName());
      emitter.Emit(nameOpnd->GetName() + emitter.HugeSoPostFix());
      break;
    }
    AArch64RegOperand *regOpnd = static_cast<AArch64RegOperand*>(opnds[static_cast<uint32>(seq[i])]);
    if (regOpnd != nullptr &&
        static_cast<const AArch64OpndProp *>(md->operand[static_cast<uint32>(seq[i])])->IsVectorOperand()) {
      regOpnd->SetVecLanePosition(-1);
      regOpnd->SetVecLaneSize(0);
      regOpnd->SetVecElementSize(0);
      if (IsVectorOp()) {
        PrepareVectorOperand(regOpnd, compositeOpnds);
        if (compositeOpnds != 0) {
          emitter.Emit("{");
        }
      }
    }
    opnds[seq[i]]->Emit(emitter, md->operand[seq[i]]);
    if (compositeOpnds == 1) {
      emitter.Emit("}");
    }
    if (compositeOpnds > 0) {
      --compositeOpnds;
    }
    /* reset opnd0 ref-field flag, so following instruction has correct register */
    if (isRefField && (i == 0)) {
      static_cast<AArch64RegOperand*>(opnds[seq[0]])->SetRefField(false);
    }
    /* Temporary comment the label:.Label.debug.callee */
    if (i != (commaNum - 1)) {
      emitter.Emit(", ");
    }
    const uint32 commaNumForEmitLazy = 2;
    if (!CGOptions::IsLazyBinding() || cg.IsLibcore() || (mOp != MOP_wldr && mOp != MOP_xldr) ||
        commaNum != commaNumForEmitLazy || i != 1 || !opnds[seq[1]]->IsMemoryAccessOperand()) {
      continue;
    }
    /*
     * Only check the last operand of ldr in lo12 mode.
     * Check the second operand, if it's [AArch64MemOperand::kAddrModeLo12Li]
     */
    auto *memOpnd = static_cast<AArch64MemOperand*>(opnds[seq[1]]);
    if (memOpnd == nullptr || memOpnd->GetAddrMode() != AArch64MemOperand::kAddrModeLo12Li) {
      continue;
    }
    const MIRSymbol *sym = memOpnd->GetSymbol();
    if (sym->IsMuidFuncDefTab() || sym->IsMuidFuncUndefTab() ||
        sym->IsMuidDataDefTab() || sym->IsMuidDataUndefTab()) {
      emitter.Emit("\n");
      EmitLazyBindingRoutine(emitter);
      emitter.IncreaseJavaInsnCount(kLazyBindingRoutineInsnCount);
    }
  }
  if (cg.GenerateVerboseCG() || (cg.GenerateVerboseAsm() && mOp == MOP_comment)) {
    const char *comment = GetComment().c_str();
    if (comment != nullptr && strlen(comment) > 0) {
      (void)emitter.Emit("\t\t// ").Emit(comment);
    }
  }

  emitter.Emit("\n");
}

/* set opnd0 ref-field flag, so we can emit the right register */
bool AArch64Insn::CheckRefField(size_t opndIndex, bool isEmit) const {
  if (IsAccessRefField() && AccessMem()) {
    Operand *opnd0 = opnds[opndIndex];
    if (opnd0->IsRegister()) {
      if (isEmit) {
        static_cast<AArch64RegOperand*>(opnd0)->SetRefField(true);
      }
      return true;
    }
  }
  return false;
}

uint8 AArch64Insn::GetLoadStoreSize() const {
  if (IsLoadStorePair()) {
    return k16ByteSize;
  }
  /* These are the loads and stores possible from PickLdStInsn() */
  switch (mOp) {
  case MOP_wldarb:
  case MOP_wldaxrb:
  case MOP_wldrb:
  case MOP_wldrsb:
  case MOP_xldrsb:
  case MOP_wstrb:
  case MOP_wstlrb:
  case MOP_wstlxrb:
    return k1ByteSize;
  case MOP_wldrh:
  case MOP_wldarh:
  case MOP_wldaxrh:
  case MOP_wldrsh:
  case MOP_xldrsh:
  case MOP_wstrh:
  case MOP_wstlrh:
  case MOP_wstlxrh:
    return k2ByteSize;
  case MOP_sldr:
  case MOP_wldr:
  case MOP_wldar:
  case MOP_wldaxr:
  case MOP_sstr:
  case MOP_wstr:
  case MOP_wstlr:
  case MOP_wstlxr:
  case MOP_xldrsw:
    return k4ByteSize;
  case MOP_dstr:
  case MOP_xstr:
  case MOP_xstlr:
  case MOP_xstlxr:
  case MOP_wstp:
  case MOP_sstp:
  case MOP_dldr:
  case MOP_xldr:
  case MOP_xldar:
  case MOP_xldaxr:
  case MOP_wldp:
  case MOP_sldp:
    return k8ByteSize;
  case MOP_xldp:
  case MOP_xldpsw:
  case MOP_dldp:
  case MOP_qldr:
  case MOP_xstp:
  case MOP_dstp:
  case MOP_qstr:
    return k16ByteSize;

  default:
    this->Dump();
    CHECK_FATAL(false, "Unsupported load/store op");
  }
}

Operand *AArch64Insn::GetResult(uint32 id) const {
  ASSERT(id < GetResultNum(), "index out of range");
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  uint32 tempIdx = 0;
  Operand* resOpnd = nullptr;
  for (uint32 i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsDef()) {
      if (tempIdx == id) {
        resOpnd = opnds[i];
        break;
      } else {
        ++tempIdx;
      }
    }
  }
  return resOpnd;
}

void AArch64Insn::SetOpnd(uint32 id, Operand &opnd) {
  ASSERT(id < GetOpndNum(), "index out of range");
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  uint32 tempIdx = 0;
  for (uint32 i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsUse()) {
      if (tempIdx == id) {
        opnds[i] = &opnd;
        return;
      } else {
        ++tempIdx;
      }
    }
  }
}

void AArch64Insn::SetResult(uint32 id, Operand &opnd) {
  ASSERT(id < GetResultNum(), "index out of range");
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  uint32 tempIdx = 0;
  for (uint32 i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsDef()) {
      if (tempIdx == id) {
        opnds[i] = &opnd;
        return;
      } else {
        ++tempIdx;
      }
    }
  }
}

Operand *AArch64Insn::GetOpnd(uint32 id) const {
  ASSERT(id < GetOpndNum(), "index out of range");
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  Operand *resOpnd = nullptr;
  uint32 tempIdx = 0;
  for (uint32 i = 0; i < opnds.size(); ++i) {
    if (md->GetOperand(i)->IsUse()) {
      if (tempIdx == id) {
        resOpnd = opnds[i];
        break;
      } else {
        ++tempIdx;
      }
    }
  }
  return resOpnd;
}
/* Return the first memory access operand. */
Operand *AArch64Insn::GetMemOpnd() const {
  for (int32 i = 0; i < opnds.size(); ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.IsMemoryAccessOperand()) {
      return &opnd;
    }
  }
  return nullptr;
}

/* Set the first memory access operand. */
void AArch64Insn::SetMemOpnd(MemOperand *memOpnd) {
  for (uint32 i = 0; i < static_cast<uint32>(opnds.size()); ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.IsMemoryAccessOperand()) {
      SetOperand(i, *memOpnd);
      return;
    }
  }
}

bool AArch64Insn::IsRegDefOrUse(regno_t regNO) const {
  uint32 opndNum = GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.IsList()) {
      auto &listOpnd = static_cast<ListOperand&>(opnd);
      for (auto listElem : listOpnd.GetOperands()) {
        RegOperand *regOpnd = static_cast<RegOperand*>(listElem);
        ASSERT(regOpnd != nullptr, "parameter operand must be RegOperand");
        if (regNO == regOpnd->GetRegisterNumber()) {
          return true;
        }
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
      RegOperand *base = memOpnd.GetBaseRegister();
      RegOperand *index = memOpnd.GetIndexRegister();
      if ((base != nullptr && base->GetRegisterNumber() == regNO) ||
          (index != nullptr && index->GetRegisterNumber() == regNO)) {
        return true;
      }
    } else if (opnd.IsConditionCode()) {
      if (regNO == kRFLAG) {
        return true;
      }
    } else if (opnd.IsRegister()) {
      if (static_cast<RegOperand&>(opnd).GetRegisterNumber() == regNO) {
        return true;
      }
    }
  }
  return false;
}

bool AArch64Insn::IsRegDefined(regno_t regNO) const {
  return GetDefRegs().count(regNO);
}

std::set<uint32> AArch64Insn::GetDefRegs() const {
  std::set<uint32> defRegNOs;
  size_t opndNum = opnds.size();
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = GetOperand(i);
    AArch64OpndProp *regProp = static_cast<AArch64OpndProp*>(md->operand[i]);
    bool isDef = regProp->IsDef();
    if (!isDef && !opnd.IsMemoryAccessOperand()) {
      continue;
    }
    if (opnd.IsList()) {
      for (auto *op : static_cast<AArch64ListOperand&>(opnd).GetOperands()) {
        ASSERT(op != nullptr, "invalid operand in list operand");
        defRegNOs.emplace(op->GetRegisterNumber());
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
      RegOperand *base = memOpnd.GetBaseRegister();
      if (base != nullptr) {
        if (memOpnd.GetAddrMode() == AArch64MemOperand::kAddrModeBOi &&
            (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed())) {
          ASSERT(!defRegNOs.count(base->GetRegisterNumber()), "duplicate def in one insn");
          defRegNOs.emplace(base->GetRegisterNumber());
        }
      }
    } else if (opnd.IsConditionCode() || opnd.IsRegister()) {
      ASSERT(!defRegNOs.count(static_cast<RegOperand&>(opnd).GetRegisterNumber()), "duplicate def in one insn");
      defRegNOs.emplace(static_cast<RegOperand&>(opnd).GetRegisterNumber());
    }
  }
  return defRegNOs;
}

uint32 AArch64Insn::GetBothDefUseOpnd() const {
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  size_t opndNum = opnds.size();
  uint32 opndIdx = kInsnMaxOpnd;
  for (uint32 i = 0; i < opndNum; ++i) {
    auto *opndProp = static_cast<AArch64OpndProp*>(md->operand[i]);
    if (opndProp->IsRegUse() && opndProp->IsDef()) {
      ASSERT(opndIdx == kInsnMaxOpnd, "Do not support in aarch64 yet");
      opndIdx = i;
    }
    if (opnds[i]->IsMemoryAccessOperand()) {
      auto *a64MemOpnd = static_cast<AArch64MemOperand*>(opnds[i]);
      if (!a64MemOpnd->IsIntactIndexed()) {
        ASSERT(opndIdx == kInsnMaxOpnd, "Do not support in aarch64 yet");
        opndIdx = i;
      }
    }
  }
  return opndIdx;
}

bool AArch64Insn::IsVolatile() const {
  return AArch64CG::kMd[mOp].IsVolatile();
}

bool AArch64Insn::IsMemAccessBar() const {
  return AArch64CG::kMd[mOp].IsMemAccessBar();
}

bool AArch64Insn::IsBranch() const {
  return AArch64CG::kMd[mOp].IsBranch();
}

bool AArch64Insn::IsCondBranch() const {
  return AArch64CG::kMd[mOp].IsCondBranch();
}

bool AArch64Insn::IsUnCondBranch() const {
  return AArch64CG::kMd[mOp].IsUnCondBranch();
}

bool AArch64Insn::IsCall() const {
  return AArch64CG::kMd[mOp].IsCall();
}

bool AArch64Insn::IsVectorOp() const {
  return AArch64CG::kMd[mOp].IsVectorOp();
}

bool AArch64Insn::HasLoop() const {
  return AArch64CG::kMd[mOp].HasLoop();
}

bool AArch64Insn::IsSpecialIntrinsic() const {
  switch (mOp) {
    case MOP_vwdupur:
    case MOP_vwdupvr:
    case MOP_vxdupur:
    case MOP_vxdupvr:
    case MOP_vduprv:
    case MOP_vwinsur:
    case MOP_vxinsur:
    case MOP_vwinsvr:
    case MOP_vxinsvr:
    case MOP_get_and_addI:
    case MOP_get_and_addL:
    case MOP_compare_and_swapI:
    case MOP_compare_and_swapL:
    case MOP_string_indexof:
    case MOP_lazy_ldr:
    case MOP_get_and_setI:
    case MOP_get_and_setL: {
      return true;
    }
    default: {
      return false;
    }
  }
}

bool AArch64Insn::IsAsmInsn() const {
  return (mOp == MOP_asm);
}

bool AArch64Insn::IsTailCall() const {
  return (mOp == MOP_tail_call_opt_xbl || mOp == MOP_tail_call_opt_xblr);
}

bool AArch64Insn::IsClinit() const {
  return (mOp == MOP_clinit || mOp == MOP_clinit_tail || mOp == MOP_adrp_ldr);
}

bool AArch64Insn::IsLazyLoad() const {
  return (mOp == MOP_lazy_ldr) || (mOp == MOP_lazy_ldr_static) || (mOp == MOP_lazy_tail);
}

bool AArch64Insn::IsAdrpLdr() const {
  return mOp == MOP_adrp_ldr;
}

bool AArch64Insn::IsArrayClassCache() const {
  return mOp == MOP_arrayclass_cache_ldr;
}

bool AArch64Insn::CanThrow() const {
  return AArch64CG::kMd[mOp].CanThrow();
}

bool AArch64Insn::IsMemAccess() const {
  return AArch64CG::kMd[mOp].IsMemAccess();
}

bool AArch64Insn::MayThrow() {
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  if (md->IsMemAccess() && !IsLoadLabel()) {
    auto *aarchMemOpnd = static_cast<AArch64MemOperand*>(GetMemOpnd());
    ASSERT(aarchMemOpnd != nullptr, "CG invalid memory operand.");
    RegOperand *baseRegister = aarchMemOpnd->GetBaseRegister();
    if (baseRegister != nullptr &&
        (baseRegister->GetRegisterNumber() == RFP || baseRegister->GetRegisterNumber() == RSP)) {
      return false;
    }
  }
  return md->CanThrow();
}

bool AArch64Insn::IsCallToFunctionThatNeverReturns() {
  if (IsIndirectCall()) {
    return false;
  }
  auto *target = static_cast<FuncNameOperand*>(GetCallTargetOperand());
  CHECK_FATAL(target != nullptr, "target is null in AArch64Insn::IsCallToFunctionThatNeverReturns");
  const MIRSymbol *funcSt = target->GetFunctionSymbol();
  ASSERT(funcSt->GetSKind() == kStFunc, "funcst must be a function name symbol");
  MIRFunction *func = funcSt->GetFunction();
  return func->NeverReturns();
}

bool AArch64Insn::IsDMBInsn() const {
  return AArch64CG::kMd[mOp].IsDMB();
}

bool AArch64Insn::IsMove() const {
  return AArch64CG::kMd[mOp].IsMove();
}

bool AArch64Insn::IsMoveRegReg() const {
  return mOp == MOP_xmovrr || mOp == MOP_wmovrr || mOp == MOP_xvmovs || mOp  == MOP_xvmovd;
}

bool AArch64Insn::IsPhi() const {
  return AArch64CG::kMd[mOp].IsPhi();
}

bool AArch64Insn::IsLoad() const {
  return AArch64CG::kMd[mOp].IsLoad();
}

bool AArch64Insn::IsLoadLabel() const {
  return (mOp == MOP_wldli || mOp == MOP_xldli || mOp == MOP_sldli || mOp == MOP_dldli);
}

bool AArch64Insn::IsStore() const {
  return AArch64CG::kMd[mOp].IsStore();
}

bool AArch64Insn::IsLoadPair() const {
  return AArch64CG::kMd[mOp].IsLoadPair();
}

bool AArch64Insn::IsStorePair() const {
  return AArch64CG::kMd[mOp].IsStorePair();
}

bool AArch64Insn::IsLoadStorePair() const {
  return AArch64CG::kMd[mOp].IsLoadStorePair();
}

bool AArch64Insn::IsLoadAddress() const {
  return AArch64CG::kMd[mOp].IsLoadAddress();
}

bool AArch64Insn::IsAtomic() const {
  return AArch64CG::kMd[mOp].IsAtomic();
}

bool AArch64Insn::IsPartDef() const {
  return AArch64CG::kMd[mOp].IsPartDef();
}

bool AArch64Insn::OpndIsDef(uint32 id) const {
  return AArch64CG::kMd[mOp].GetOperand(id)->IsDef();
}

bool AArch64Insn::OpndIsUse(uint32 id) const {
  return AArch64CG::kMd[mOp].GetOperand(id)->IsUse();
}

uint32 AArch64Insn::GetLatencyType() const {
  return AArch64CG::kMd[mOp].GetLatencyType();
}

uint32 AArch64Insn::GetAtomicNum() const {
  return AArch64CG::kMd[mOp].GetAtomicNum();
}

bool AArch64Insn::IsYieldPoint() const {
  /*
   * It is a yieldpoint if loading from a dedicated
   * register holding polling page address:
   * ldr  wzr, [RYP]
   */
  if (IsLoad() && !IsLoadLabel()) {
    auto mem = static_cast<MemOperand*>(GetOpnd(0));
    return (mem != nullptr && mem->GetBaseRegister() != nullptr && mem->GetBaseRegister()->GetRegisterNumber() == RYP);
  }
  return false;
}
/* Return the copy operand id of reg1 if it is an insn who just do copy from reg1 to reg2.
 * i. mov reg2, reg1
 * ii. add/sub reg2, reg1, 0/zero register
 * iii. mul reg2, reg1, 1
 */
int32 AArch64Insn::CopyOperands() const {
  if (mOp >= MOP_xmovrr  && mOp <= MOP_xvmovrv) {
    return 1;
  }
  if (mOp == MOP_vmovuu || mOp == MOP_vmovvv) {
    return 1;
  }
  if ((mOp >= MOP_xaddrrr && mOp <= MOP_ssub) || (mOp >= MOP_xlslrri6 && mOp <= MOP_wlsrrrr)) {
    Operand &opnd2 = GetOperand(kInsnThirdOpnd);
    if (opnd2.IsIntImmediate()) {
      auto &immOpnd = static_cast<ImmOperand&>(opnd2);
      if (immOpnd.IsZero()) {
        return 1;
      }
    }
  }
  if (mOp > MOP_xmulrrr && mOp <= MOP_xvmuld) {
    Operand &opnd2 = GetOperand(kInsnThirdOpnd);
    if (opnd2.IsIntImmediate()) {
      auto &immOpnd = static_cast<ImmOperand&>(opnd2);
      if (immOpnd.GetValue() == 1) {
        return 1;
      }
    }
  }
  return -1;
}

void AArch64Insn::CheckOpnd(Operand &opnd, OpndProp &prop) const {
  (void)opnd;
  (void)prop;
#if DEBUG
  auto &mopd = static_cast<AArch64OpndProp&>(prop);
  switch (opnd.GetKind()) {
    case Operand::kOpdRegister:
      ASSERT(mopd.IsRegister(), "expect reg");
      break;
    case Operand::kOpdOffset:
    case Operand::kOpdImmediate:
      ASSERT(mopd.GetOperandType() == Operand::kOpdImmediate, "expect imm");
      break;
    case Operand::kOpdFPImmediate:
      ASSERT(mopd.GetOperandType() == Operand::kOpdFPImmediate, "expect fpimm");
      break;
    case Operand::kOpdFPZeroImmediate:
      ASSERT(mopd.GetOperandType() == Operand::kOpdFPZeroImmediate, "expect fpzero");
      break;
    case Operand::kOpdMem:
      ASSERT(mopd.GetOperandType() == Operand::kOpdMem, "expect mem");
      break;
    case Operand::kOpdBBAddress:
      ASSERT(mopd.GetOperandType() == Operand::kOpdBBAddress, "expect address");
      break;
    case Operand::kOpdList:
      ASSERT(mopd.GetOperandType() == Operand::kOpdList, "expect list operand");
      break;
    case Operand::kOpdCond:
      ASSERT(mopd.GetOperandType() == Operand::kOpdCond, "expect cond operand");
      break;
    case Operand::kOpdShift:
      ASSERT(mopd.GetOperandType() == Operand::kOpdShift, "expect LSL operand");
      break;
    case Operand::kOpdStImmediate:
      ASSERT(mopd.GetOperandType() == Operand::kOpdStImmediate, "expect symbol name (literal)");
      break;
    case Operand::kOpdString:
      ASSERT(mopd.GetOperandType() == Operand::kOpdString, "expect a string");
      break;
    default:
      ASSERT(false, "NYI");
      break;
  }
#endif
}

/*
 * Precondition: The given insn is a jump instruction.
 * Get the jump target label operand index from the given instruction.
 * Note: MOP_xbr is a jump instruction, but the target is unknown at compile time,
 * because a register instead of label. So we don't take it as a branching instruction.
 * Howeer for special long range branch patch, the label is installed in this case.
 */
uint32 AArch64Insn::GetJumpTargetIdx() const {
  return GetJumpTargetIdxFromMOp(mOp);
}

uint32 AArch64Insn::GetJumpTargetIdxFromMOp(MOperator mOp) const {
  switch (mOp) {
    /* unconditional jump */
    case MOP_xuncond: {
      return kOperandPosition0;
    }
    case MOP_xbr: {
      CHECK_FATAL(opnds[1] != 0, "ERR");
      return kOperandPosition1;
    }
    /* conditional jump */
    case MOP_bmi:
    case MOP_bvc:
    case MOP_bls:
    case MOP_blt:
    case MOP_ble:
    case MOP_blo:
    case MOP_beq:
    case MOP_bpl:
    case MOP_bhs:
    case MOP_bvs:
    case MOP_bhi:
    case MOP_bgt:
    case MOP_bge:
    case MOP_bne:
    case MOP_wcbz:
    case MOP_xcbz:
    case MOP_wcbnz:
    case MOP_xcbnz: {
      return kOperandPosition1;
    }
    case MOP_wtbz:
    case MOP_xtbz:
    case MOP_wtbnz:
    case MOP_xtbnz: {
      return kOperandPosition2;
    }
    default:
      CHECK_FATAL(false, "Not a jump insn");
  }
  return kOperandPosition0;
}

MOperator AArch64Insn::FlipConditionOp(MOperator originalOp, uint32 &targetIdx) {
  targetIdx = 1;
  switch (originalOp) {
    case AArch64MOP_t::MOP_beq:
      return AArch64MOP_t::MOP_bne;
    case AArch64MOP_t::MOP_bge:
      return AArch64MOP_t::MOP_blt;
    case AArch64MOP_t::MOP_bgt:
      return AArch64MOP_t::MOP_ble;
    case AArch64MOP_t::MOP_bhi:
      return AArch64MOP_t::MOP_bls;
    case AArch64MOP_t::MOP_bhs:
      return AArch64MOP_t::MOP_blo;
    case AArch64MOP_t::MOP_ble:
      return AArch64MOP_t::MOP_bgt;
    case AArch64MOP_t::MOP_blo:
      return AArch64MOP_t::MOP_bhs;
    case AArch64MOP_t::MOP_bls:
      return AArch64MOP_t::MOP_bhi;
    case AArch64MOP_t::MOP_blt:
      return AArch64MOP_t::MOP_bge;
    case AArch64MOP_t::MOP_bne:
      return AArch64MOP_t::MOP_beq;
    case AArch64MOP_t::MOP_bpl:
      return AArch64MOP_t::MOP_bmi;
    case AArch64MOP_t::MOP_xcbnz:
      return AArch64MOP_t::MOP_xcbz;
    case AArch64MOP_t::MOP_wcbnz:
      return AArch64MOP_t::MOP_wcbz;
    case AArch64MOP_t::MOP_xcbz:
      return AArch64MOP_t::MOP_xcbnz;
    case AArch64MOP_t::MOP_wcbz:
      return AArch64MOP_t::MOP_wcbnz;
    case AArch64MOP_t::MOP_wtbnz:
      targetIdx = GetJumpTargetIdxFromMOp(AArch64MOP_t::MOP_wtbz);
      return AArch64MOP_t::MOP_wtbz;
    case AArch64MOP_t::MOP_wtbz:
      targetIdx = GetJumpTargetIdxFromMOp(AArch64MOP_t::MOP_wtbnz);
      return AArch64MOP_t::MOP_wtbnz;
    case AArch64MOP_t::MOP_xtbnz:
      targetIdx = GetJumpTargetIdxFromMOp(AArch64MOP_t::MOP_xtbz);
      return AArch64MOP_t::MOP_xtbz;
    case AArch64MOP_t::MOP_xtbz:
      targetIdx = GetJumpTargetIdxFromMOp(AArch64MOP_t::MOP_xtbnz);
      return AArch64MOP_t::MOP_xtbnz;
    default:
      break;
  }
  return AArch64MOP_t::MOP_undef;
}

bool AArch64Insn::Check() const {
#if DEBUG
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  if (md == nullptr) {
    return false;
  }
  for (uint32 i = 0; i < GetOperandSize(); ++i) {
    Operand &opnd = GetOperand(i);
    /* maybe if !opnd, break ? */
    CheckOpnd(opnd, *(md->operand[i]));
  }
  return true;
#else
  return false;
#endif
}

void AArch64Insn::Dump() const {
  const AArch64MD *md = &AArch64CG::kMd[mOp];
  ASSERT(md != nullptr, "md should not be nullptr");

  LogInfo::MapleLogger() << "< " << GetId() << " > ";
  LogInfo::MapleLogger() << md->name << "(" << mOp << ")";

  for (uint32 i = 0; i < GetOperandSize(); ++i) {
    Operand &opnd = GetOperand(i);
    LogInfo::MapleLogger() << " (opnd" << i << ": ";
    opnd.Dump();
    LogInfo::MapleLogger() << ")";
  }

  if (IsVectorOp()) {
    const AArch64VectorInsn *vInsn = static_cast<const AArch64VectorInsn*>(this);
    if (vInsn->GetNumOfRegSpec() != 0) {
      LogInfo::MapleLogger() << " (vecSpec: " << vInsn->GetNumOfRegSpec() << ")";
    }
  }
  LogInfo::MapleLogger() << "\n";
}

bool AArch64Insn::IsDefinition() const {
  /* check if we are seeing ldp or not */
  ASSERT(AArch64CG::kMd[mOp].GetOperand(1) == nullptr ||
         !AArch64CG::kMd[mOp].GetOperand(1)->IsRegDef(), "check if we are seeing ldp or not");
  if (AArch64CG::kMd[mOp].GetOperand(0) == nullptr) {
    return false;
  }
  return AArch64CG::kMd[mOp].GetOperand(0)->IsRegDef();
}

bool AArch64Insn::IsDestRegAlsoSrcReg() const {
  auto *prop0 = static_cast<AArch64OpndProp*>(AArch64CG::kMd[mOp].GetOperand(0));
  ASSERT(prop0 != nullptr, "expect a AArch64OpndProp");
  return prop0->IsRegDef() && prop0->IsRegUse();
}
}  /* namespace maplebe */
