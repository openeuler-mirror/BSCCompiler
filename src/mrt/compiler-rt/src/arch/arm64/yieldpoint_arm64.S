//---------------------------------------------------------------
// This file defines assembly functions to support yieldpoints.
//---------------------------------------------------------------
  .text

//---------------------------------------------------------------
// MRT_YieldpointStub() will called when yieldpoint is taken.
//
// void MRT_YieldpointStub() {
//    save_live_registers();
//    lr = MRT_GetThreadYieldpointPC();
//    MRT_YieldpointHandler(last_fp);
//    restore_live_registers();
//    return to lr;
// }
//
//         fp ---> --------  0
//                 |  q31 |
//                 |  ... |
//                 |  q0  |
//                 --------
//                 |  x29 |
//                 |  x28 |
//                 |  ... |
//                 |  x0  |
//                 --------
//                 |  x30 | lr
//                 |  x29 | fp
//         sp ---> --------  -0x300
//
//---------------------------------------------------------------
  .align 2
  .global MRT_YieldpointStub
  .type MRT_YieldpointStub, %function
MRT_YieldpointStub:
  .cfi_startproc
  sub  sp, sp, 0x300
  .cfi_def_cfa_offset 0x300

  // save fp,lr on stack, follow the calling convention.
  stp  x29, x30, [sp]
  .cfi_offset 29, -0x300
  .cfi_offset 30, -0x2f8

  // save integer registers except LR(x30)
  stp  x0,  x1,  [sp, 0x10]
  stp  x2,  x3,  [sp, 0x20]
  stp  x4,  x5,  [sp, 0x30]
  stp  x6,  x7,  [sp, 0x40]
  stp  x8,  x9,  [sp, 0x50]
  stp  x10, x11, [sp, 0x60]
  stp  x12, x13, [sp, 0x70]
  stp  x14, x15, [sp, 0x80]
  stp  x16, x17, [sp, 0x90]
  stp  x18, x19, [sp, 0xa0]
  stp  x20, x21, [sp, 0xb0]
  stp  x22, x23, [sp, 0xc0]
  stp  x24, x25, [sp, 0xd0]
  stp  x26, x27, [sp, 0xe0]
  stp  x28, x29, [sp, 0xf0]

  // save scalar registers
  stp  q0,  q1,  [sp, 0x100]
  stp  q2,  q3,  [sp, 0x120]
  stp  q4,  q5,  [sp, 0x140]
  stp  q6,  q7,  [sp, 0x160]
  stp  q8,  q9,  [sp, 0x180]
  stp  q10, q11, [sp, 0x1a0]
  stp  q12, q13, [sp, 0x1c0]
  stp  q14, q15, [sp, 0x1e0]
  stp  q16, q17, [sp, 0x200]
  stp  q18, q19, [sp, 0x220]
  stp  q20, q21, [sp, 0x240]
  stp  q22, q23, [sp, 0x260]
  stp  q24, q25, [sp, 0x280]
  stp  q26, q27, [sp, 0x2a0]
  stp  q28, q29, [sp, 0x2c0]
  stp  q30, q31, [sp, 0x2e0]

  // set current fp
  add  x29, sp, 0
  .cfi_def_cfa_register 29

  // save condition flags
  mrs  x28, NZCV

  // set LR as the PC of yieldpoint,
  // and save it to the proper slot of stack.
  bl   MRT_GetThreadYieldpointPC
  mov  x30, x0
  str  x30, [sp, 0x08]

  // call yieldpoint handler with current fp
  mov  x0,  x29
  bl   MRT_YieldpointHandler

  // restore condition flags
  msr  NZCV, x28

  // restore integer registers except lr
  ldp  x0,  x1,  [sp, 0x10]
  ldp  x2,  x3,  [sp, 0x20]
  ldp  x4,  x5,  [sp, 0x30]
  ldp  x6,  x7,  [sp, 0x40]
  ldp  x8,  x9,  [sp, 0x50]
  ldp  x10, x11, [sp, 0x60]
  ldp  x12, x13, [sp, 0x70]
  ldp  x14, x15, [sp, 0x80]
  ldp  x16, x17, [sp, 0x90]
  ldp  x18, x19, [sp, 0xa0]
  ldp  x20, x21, [sp, 0xb0]
  ldp  x22, x23, [sp, 0xc0]
  ldp  x24, x25, [sp, 0xd0]
  ldp  x26, x27, [sp, 0xe0]
  ldp  x28, x29, [sp, 0xf0]

  // restore scalar registers
  ldp  q0,  q1,  [sp, 0x100]
  ldp  q2,  q3,  [sp, 0x120]
  ldp  q4,  q5,  [sp, 0x140]
  ldp  q6,  q7,  [sp, 0x160]
  ldp  q8,  q9,  [sp, 0x180]
  ldp  q10, q11, [sp, 0x1a0]
  ldp  q12, q13, [sp, 0x1c0]
  ldp  q14, q15, [sp, 0x1e0]
  ldp  q16, q17, [sp, 0x200]
  ldp  q18, q19, [sp, 0x220]
  ldp  q20, q21, [sp, 0x240]
  ldp  q22, q23, [sp, 0x260]
  ldp  q24, q25, [sp, 0x280]
  ldp  q26, q27, [sp, 0x2a0]
  ldp  q28, q29, [sp, 0x2c0]
  ldp  q30, q31, [sp, 0x2e0]

  // restore fp, lr
  ldp  x29, x30, [sp]

  // restore stack pointer.
  add  sp, sp, 0x300

  .cfi_restore 29
  .cfi_restore 30
  .cfi_def_cfa 31, 0
  ret
  .cfi_endproc
  .size MRT_YieldpointStub, .-MRT_YieldpointStub
