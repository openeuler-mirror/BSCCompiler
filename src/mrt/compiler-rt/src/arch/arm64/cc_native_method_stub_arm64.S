// This file defines stub to support calling java native methods.
// "cc_" means "compiled code" by maple.

//------------------------------------------------------------------------
//  native wrapper      num of args   enter saferegion  copy stack args
//------------------------------------------------------------------------
//  MCC_CallFastNative       any           no             no
//  MCC_CallFastNativeExt    any           no             no
//  MCC_CallSlowNative       <=8           yes            no
//  MCC_CallSlowNativeExt    any           yes            yes
//------------------------------------------------------------------------

//---------------------------------------------------------------
// MCC_CallFastNative() calls a fast native function from
// managed code. We use x9 register to pass the native function pointer.
//
// MCC_CallFastNative(...) {
//   br x9; // call the_native_function;
// }
//---------------------------------------------------------------

  .text
  .align 2
  .global MCC_CallFastNative
  .global MCC_CallFastNativeExt
  .type MCC_CallFastNative, %function
  .type MCC_CallFastNativeExt, %function
MCC_CallFastNative:
MCC_CallFastNativeExt:
  .cfi_startproc
  // call the native function.
  br  x9
  .cfi_endproc
  .size MCC_CallFastNative, .-MCC_CallFastNative

//---------------------------------------------------------------
// MCC_CallSlowNative() calls a slow or blocking native
// function from managed code. The number of arguments
// of the native function should less than or equal to 8.
//
// We use x9 register to pass the native function pointer.
//
// This stub will save callee-saved registers to stack and
// enter saferegion before calling the native function:
//
// MCC_CallSlowNative(the_native_function, ...) {
//   save_callee_saved_registers(); // r19..r28
//   stateChanged = MRT_EnterSaferegion();
//   blr x9; // call the_native_function;
//   if (stateChanged) MRT_LeaveSaferegion();
//   restore_callee_saved_registers();
//   return ret;
// }
//
// caller fp,sp -> --------  0
//                 |  x28 |
//                 |  x27 | [sp, 0x50]
//                 |  ... |
//                 |  x19 | [sp, 0x10]
//                 --------
//                 |  x30 | lr
//                 |  x29 | fp
//       fp, sp -> --------  -0x60
//
//---------------------------------------------------------------
  .macro MCC_CallSlowNative, funcName, argSize
  .align 2
  .global \funcName
  .type \funcName, %function
\funcName:
  .cfi_startproc
  stp   x29, x30, [sp, -0x60]!
  .cfi_def_cfa_offset 0x60
  .cfi_offset 29, -0x60
  .cfi_offset 30, -0x58
  add   x29, sp, 0
  .cfi_def_cfa_register 29

  // save callee-saved registers.
  stp  x19, x20, [x29, 0x10]
  .cfi_rel_offset x19, 0x10
  .cfi_rel_offset x20, 0x18

  stp  x21, x22, [x29, 0x20]
  .cfi_rel_offset x21, 0x20
  .cfi_rel_offset x22, 0x28

  stp  x23, x24, [x29, 0x30]
  .cfi_rel_offset x23, 0x30
  .cfi_rel_offset x24, 0x38

  stp  x25, x26, [x29, 0x40]
  .cfi_rel_offset x25, 0x40
  .cfi_rel_offset x26, 0x48

  stp  x27, x28, [x29, 0x50]
  .cfi_rel_offset x27, 0x50
  .cfi_rel_offset x28, 0x58

  // save args to callee-saved reigsters.
  mov  x19, x9  // func ptr
  .if \argSize == 1
  mov  x20, x0  // arg0
  .endif
  .if \argSize == 2
  mov  x20, x0  // arg0
  mov  x21, x1  // arg1
  .endif
  .if \argSize == 3
  mov  x20, x0  // arg0
  mov  x21, x1  // arg1
  mov  x22, x2  // arg2
  .endif
  .if \argSize == 4
  mov  x20, x0  // arg0
  mov  x21, x1  // arg1
  mov  x22, x2  // arg2
  mov  x23, x3  // arg3
  .endif
  .if \argSize == 5
  mov  x20, x0  // arg0
  mov  x21, x1  // arg1
  mov  x22, x2  // arg2
  mov  x23, x3  // arg3
  mov  x24, x4  // arg4
  .endif
  .if \argSize == 6
  mov  x20, x0  // arg0
  mov  x21, x1  // arg1
  mov  x22, x2  // arg2
  mov  x23, x3  // arg3
  mov  x24, x4  // arg4
  mov  x25, x5  // arg5
  .endif
  .if \argSize == 7
  mov  x20, x0  // arg0
  mov  x21, x1  // arg1
  mov  x22, x2  // arg2
  mov  x23, x3  // arg3
  mov  x24, x4  // arg4
  mov  x25, x5  // arg5
  mov  x26, x6  // arg6
  .endif
  .if \argSize == 8
  mov  x20, x0  // arg0
  mov  x21, x1  // arg1
  mov  x22, x2  // arg2
  mov  x23, x3  // arg3
  mov  x24, x4  // arg4
  mov  x25, x5  // arg5
  mov  x26, x6  // arg6
  mov  x27, x7  // arg7
  .endif

#if CONFIG_ASAN
  stp d0,  d1,  [sp,-16]!
  stp d2,  d3,  [sp,-16]!
  stp d4,  d5,  [sp,-16]!
  stp d6,  d7,  [sp,-16]!
#endif

  // save last java frame
  ldr  x1, [x29]
  mov  x0, x30
  bl   MRT_SetRiskyUnwindContext

  // enter saferegion.
  mov  x0, #0
  bl   MRT_EnterSaferegion

  // save result.
  mov  x28, x0
#if CONFIG_ASAN
  ldp d6,  d7,  [sp], 16
  ldp d4,  d5,  [sp], 16
  ldp d2,  d3,  [sp], 16
  ldp d0,  d1,  [sp], 16
#endif

  // restore args.
  .if \argSize == 1
  mov  x0,  x20
  .endif
  .if \argSize == 2
  mov  x0,  x20
  mov  x1,  x21
  .endif
  .if \argSize == 3
  mov  x0,  x20
  mov  x1,  x21
  mov  x2,  x22
  .endif
  .if \argSize == 4
  mov  x0,  x20
  mov  x1,  x21
  mov  x2,  x22
  mov  x3,  x23
  .endif
  .if \argSize == 5
  mov  x0,  x20
  mov  x1,  x21
  mov  x2,  x22
  mov  x3,  x23
  mov  x4,  x24
  .endif
  .if \argSize == 6
  mov  x0,  x20
  mov  x1,  x21
  mov  x2,  x22
  mov  x3,  x23
  mov  x4,  x24
  mov  x5,  x25
  .endif
  .if \argSize == 7
  mov  x0,  x20
  mov  x1,  x21
  mov  x2,  x22
  mov  x3,  x23
  mov  x4,  x24
  mov  x5,  x25
  mov  x6,  x26
  .endif
  .if \argSize == 8
  mov  x0,  x20
  mov  x1,  x21
  mov  x2,  x22
  mov  x3,  x23
  mov  x4,  x24
  mov  x5,  x25
  mov  x6,  x26
  mov  x7,  x27
  .endif

  // call native function.
  blr  x19

  // do not call MRT_LeaveSaferegion
  // if MRT_EnterSaferegion returns 0
  cbz  w28, .AFTER_CALL_SLOW_NATIVE_\argSize

  // save return value.
  mov  x28, x0
  fmov x27, d0

  // leave saferegion
  bl   MRT_LeaveSaferegion

  // restore return value
  mov  x0,  x28
  fmov d0,  x27

.AFTER_CALL_SLOW_NATIVE_\argSize:
  // restore callee-saved registers.
  ldp  x19, x20, [x29, 0x10]
  .cfi_restore 19
  .cfi_restore 20
  ldp  x21, x22, [x29, 0x20]
  .cfi_restore 21
  .cfi_restore 22
  ldp  x23, x24, [x29, 0x30]
  .cfi_restore 23
  .cfi_restore 24
  ldp  x25, x26, [x29, 0x40]
  .cfi_restore 25
  .cfi_restore 26
  ldp  x27, x28, [x29, 0x50]
  .cfi_restore 27
  .cfi_restore 28

  // restore fp, lr, sp
  ldp   x29, x30, [sp], 0x60
  .cfi_restore 29
  .cfi_restore 30
  .cfi_def_cfa 31, 0
  ret
  .cfi_endproc
  .size \funcName, .-\funcName
  .endm

MCC_CallSlowNative MCC_CallSlowNative0, 0
MCC_CallSlowNative MCC_CallSlowNative1, 1
MCC_CallSlowNative MCC_CallSlowNative2, 2
MCC_CallSlowNative MCC_CallSlowNative3, 3
MCC_CallSlowNative MCC_CallSlowNative4, 4
MCC_CallSlowNative MCC_CallSlowNative5, 5
MCC_CallSlowNative MCC_CallSlowNative6, 6
MCC_CallSlowNative MCC_CallSlowNative7, 7
MCC_CallSlowNative MCC_CallSlowNative8, 8
//---------------------------------------------------------------
// MCC_CallSlowNativeExt() calls a slow or blocking native
// function from managed code. this function accept arbitrary number
// of arguments of the native function.
//
// We use x9 register to pass the native function pointer.
//
// MCC_CallSlowNativeExt(...) {
//   save_callee_saved_registers(); // x19..x28
//   copy_stack_args();             // copy stack args if found
//   stateChanged = MRT_EnterSaferegion();
//   blr x9; // call the_native_function;
//   if (stateChanged) MRT_LeaveSaferegion();
//   restore_callee_saved_registers();
//   return ret;
// }
//
//  caller fp ---> --------
//                 | stack|
//                 | args |
//  caller sp ---> --------  0
//                 |  x28 |
//                 |  x27 |
//                 |  ... |
//                 |  x19 |
//                 --------
//                 |  x30 | lr
//                 |  x29 | fp
//         fp -->  --------  -0x60
//                 | stack|
//                 | args | copy from caller
//         sp ---> --------
//
//---------------------------------------------------------------
  .align 2
  .global MCC_CallSlowNativeExt
  .type MCC_CallSlowNativeExt, %function
MCC_CallSlowNativeExt:
  .cfi_startproc
  stp   x29, x30, [sp, -0x60]!
  .cfi_def_cfa_offset 0x60
  .cfi_offset 29, -0x60
  .cfi_offset 30, -0x58
  add   x29, sp, 0
  .cfi_def_cfa_register 29

  // save callee-saved registers.
  stp  x19, x20, [x29, 0x10]
  stp  x21, x22, [x29, 0x20]
  stp  x23, x24, [x29, 0x30]
  stp  x25, x26, [x29, 0x40]
  stp  x27, x28, [x29, 0x50]

  // copy stack args.
  ldr  x19, [x29]               // x19 = caller FP
  add  x20, sp, 0x60            // x20 = caller SP
.LOOP_COPY_ARG2:                // for(;;) {
  cmp  x19, x20                 //   if (x19 == x20)
  beq  .COPY_ARG_END2           //     break;
  ldp  x27, x28, [x19, -0x10]!  //   x19-=0x10; load [x19] into x27, x28;
  stp  x27, x28, [sp,  -0x10]!  //   sp-=0x10;  store x27,x28 to [sp];
  b    .LOOP_COPY_ARG2          // }
.COPY_ARG_END2:

  // save args to callee-saved reigsters.
  mov  x19, x9  // func ptr
  mov  x20, x0  // arg0
  mov  x21, x1  // arg1
  mov  x22, x2  // arg2
  mov  x23, x3  // arg3
  mov  x24, x4  // arg4
  mov  x25, x5  // arg5
  mov  x26, x6  // arg6
  mov  x27, x7  // arg7

#if CONFIG_ASAN
  stp d0,  d1,  [sp,-16]!
  stp d2,  d3,  [sp,-16]!
  stp d4,  d5,  [sp,-16]!
  stp d6,  d7,  [sp,-16]!
#endif

   // save last java frame
  ldr  x1, [x29]
  mov  x0, x30
  bl   MRT_SetRiskyUnwindContext

  // enter saferegioni.
  mov  x0, #0
  bl   MRT_EnterSaferegion

  // save result.
  mov  x28, x0

  // restore args.
  mov  x0,  x20
  mov  x1,  x21
  mov  x2,  x22
  mov  x3,  x23
  mov  x4,  x24
  mov  x5,  x25
  mov  x6,  x26
  mov  x7,  x27


#if CONFIG_ASAN
  ldp d6,  d7,  [sp], 16
  ldp d4,  d5,  [sp], 16
  ldp d2,  d3,  [sp], 16
  ldp d0,  d1,  [sp], 16
#endif

  // call native function.
  blr  x19

  // do not call MRT_LeaveSaferegion
  // if MRT_EnterSaferegion returns 0
  cbz  w28, .AFTER_CALL_NATIVE2

  // save return value.
  mov  x28, x0
  fmov x27, d0

  // leave saferegion
  bl   MRT_LeaveSaferegion

  // restore return value
  mov  x0,  x28
  fmov d0,  x27

.AFTER_CALL_NATIVE2:
  // restore callee-saved registers.
  ldp  x19, x20, [x29, 0x10]
  ldp  x21, x22, [x29, 0x20]
  ldp  x23, x24, [x29, 0x30]
  ldp  x25, x26, [x29, 0x40]
  ldp  x27, x28, [x29, 0x50]

  // set sp to fp.
  mov sp, x29

  // restore fp, lr, sp
  ldp   x29, x30, [sp], 0x60
  .cfi_restore 30
  .cfi_restore 29
  .cfi_def_cfa 31, 0
  ret
  .cfi_endproc
  .size MCC_CallSlowNativeExt, .-MCC_CallSlowNativeExt
