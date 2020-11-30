#define cfi_adjust_cfa_offset(off)      .cfi_adjust_cfa_offset off
#define cfi_rel_offset(reg, off)        .cfi_rel_offset reg, off
#define cfi_restore(reg)                .cfi_restore reg
#define cfi_def_cfa_register(reg)       .cfi_def_cfa_register reg

////////////////////////////////////////////////////////////////////////////////
// there are 2 kinds of stubs to invoke a compiled java method: ForwardStub and BoxedStub.
//
// ForwardStub simply forwards arguments passed by runtime, i.e., arguments for compiled java method are passed
// according to C/C++ calling convention, which usually means efficiency.
//
// BoxedStub takes a general way: maple runtime first boxes all java arguments into an array composed of first integer
// arguments which are passed via general registers, then floating-point arguments which are passed via fp registers,
// and arguments which can not be passed by register (they will be passed on stack).
// BoxedStub unboxes all the arguments according to C/C++ calling convention.
////////////////////////////////////////////////////////////////////////////////

#define ForwardStubFrameSize      (8 * 18)
#define ForwardStubCalleeSaveArea (8 * 8)

// R2CForwardStubXX builds a stub frame to invoke the target java_method according to the previous frame which invokes
// R2CForwardStubXX(java_method, std::forward<Args>(args)...).
// R means runtime, while C means compiled java method. XX indicates the return type of this java method.

// On execution of "bl R2CForwardStubXX", the frame layout of stack(growing downwards) looks like:
// x0: the entry point of Java method to be invoked
// x1~x7: hold the first 7 arguments arg0~arg6 if existed
// x30: return address of "bl R2CForwardStubXX"
// all on-stack arguments are addressable by SP as the frame layout shows.
// arg7 will be passed to x7 from caller's stack after R2CForwardStub is built.
//                 |  ...         |
//                 |  x30         | lr for the caller of R2CForwardStubXX
// caller fp  -->  |  x29         |
//                 |  ...         |
//                 | arg11        |
//                 | arg10        |
//                 | arg9         |
//                 | arg8         |
// caller sp  -->  | arg7         |

// the frame layout of stack(growing downwards) after R2CForwardStub frame is built looks like:
//                 |  ...         |
//                 |  x30         | lr for the caller of R2CForwardStubXX
// caller fp  -->  |  x29         |
//                 |  ...         |
//                 | arg11        |
//                 | arg10        |
//                 | arg9         |
//                 | arg8         |
// caller sp  -->  | arg7         |
// callee saved    | r28          | <== R2CForwardStub frame starts from here
//                 | r27          |
//                 | r26          |
//                 | r25          |
//                 | r24          |
//                 | r23          |
//                 | r22          |
//                 | r21          |
//                 | r20          |
// callee saved    | r19          |
// unwind context  | direct call  | directly invoke callee java method
//                 | shadowframe  | the information of caller frame which is interpreted
//                 | UC Status    | unwind context status of caller frame
//                 | Context LR   | LR of unwind context frame
//                 | Context FP   | FP of unwind context frame
// unwind context  | Context PC   | PC of unwind context frame
//                 | x30          |
//   stub fp  -->  | caller fp    |
//                 |  ...         |
//                 | arg11        |
//                 | arg10        |
//                 | arg9         |
//   stub sp  -->  | arg8         | <== R2CForwardStub frame ends at here

        .text
        .align 2
        .global R2CForwardStubLong
        .hidden R2CForwardStubLong
        .global R2CForwardStubVoid
        .hidden R2CForwardStubVoid
        .global R2CForwardStubFloat
        .hidden R2CForwardStubFloat
        .global R2CForwardStubDouble
        .hidden R2CForwardStubDouble
        .type R2CForwardStubVoid  , %function
        .type R2CForwardStubLong  , %function
        .type R2CForwardStubFloat , %function
        .type R2CForwardStubDouble, %function
R2CForwardStubLong:
R2CForwardStubVoid:
R2CForwardStubFloat:
R2CForwardStubDouble:
        .cfi_startproc
        .cfi_personality 155, DW.ref.__n2j_stub_personality
        add  x30, x30, #1
        stp  x29, x30, [sp,  #-ForwardStubFrameSize]!
        cfi_adjust_cfa_offset (ForwardStubFrameSize)
        cfi_rel_offset (x29, 0)
        cfi_rel_offset (x30, 8)

        // save all used callee-saved registers.
        stp  x19, x20, [sp, #ForwardStubCalleeSaveArea]
        cfi_rel_offset (x19, ForwardStubCalleeSaveArea)
        cfi_rel_offset (x20, ForwardStubCalleeSaveArea+8)

        stp  x21, x22, [sp, #ForwardStubCalleeSaveArea+0x10]
        cfi_rel_offset (x21, ForwardStubCalleeSaveArea+0x10)
        cfi_rel_offset (x22, ForwardStubCalleeSaveArea+0x18)

        stp  x23, x24, [sp, #ForwardStubCalleeSaveArea+0x20]
        cfi_rel_offset (x23, ForwardStubCalleeSaveArea+0x20)
        cfi_rel_offset (x24, ForwardStubCalleeSaveArea+0x28)

        stp  x25, x26, [sp, #ForwardStubCalleeSaveArea+0x30]
        cfi_rel_offset (x25, ForwardStubCalleeSaveArea+0x30)
        cfi_rel_offset (x26, ForwardStubCalleeSaveArea+0x38)

        stp  x27, x28, [sp, #ForwardStubCalleeSaveArea+0x40]
        cfi_rel_offset (x27, ForwardStubCalleeSaveArea+0x40)
        cfi_rel_offset (x28, ForwardStubCalleeSaveArea+0x48)

        // x19 <- previous fp
        mov  x19, x29

        mov  x20, x0 // the entry point of Java method

        mov  x21, x1
        mov  x22, x2
        mov  x23, x3
        mov  x24, x4
        mov  x25, x5
        mov  x26, x6
        mov  x27, x7

        // x28 <- previous sp
        add  x28, sp, #ForwardStubFrameSize

        mov  x29, sp
        cfi_def_cfa_register (x29)

        // frame info: tls -> stub
        mov  x0, x29
        bl MRT_SaveLastUnwindContext

        // java code is always reliable for stack unwinding
        bl MRT_SetReliableUnwindContextStatus

        // prepare arguments for invoking target Java method
        mov  x0, x21
        mov  x1, x22
        mov  x2, x23
        mov  x3, x24
        mov  x4, x25
        mov  x5, x26
        mov  x6, x27

        // passing on-stack arguments. start from arg8.
        // put the 8th argument into register x7 from on-stack
        ldr x7, [x28], #8
        add x19, x19, #8

        // copy arg9, arg10, ... (if existed)
.L_copy:
        cmp x19, x28
        ble .L_copy_end
        ldp x25, x26, [x19, #-16]!
        // SP is always 16 byte-aligned.
        stp x25, x26, [sp,  #-16]!
        b .L_copy
.L_copy_end:

        // set x19 to the polling page address
        adrp x19, REF_globalPollingPage
        add  x19, x19, :lo12:REF_globalPollingPage
        ldr  x19, [x19]
        ldr  x19, [x19]

        blr  x20

        /* keep potential return value */
        mov  x28, x0
        fmov x27, d0

        /* restore last_java_frame */
        mov  x0, x29
        bl MRT_RestoreLastUnwindContext

        /* set potential return value */
        mov  x0, x28
        fmov d0, x27

        mov  sp, x29
        cfi_def_cfa_register (sp)

        // restore all used callee-saved registers.
        ldp  x19, x20, [sp, #ForwardStubCalleeSaveArea]
        cfi_restore (x19)
        cfi_restore (x20)
        ldp  x21, x22, [sp, #ForwardStubCalleeSaveArea+0x10]
        cfi_restore (x21)
        cfi_restore (x22)
        ldp  x23, x24, [sp, #ForwardStubCalleeSaveArea+0x20]
        cfi_restore (x23)
        cfi_restore (x24)
        ldp  x25, x26, [sp, #ForwardStubCalleeSaveArea+0x30]
        cfi_restore (x25)
        cfi_restore (x26)
        ldp  x27, x28, [sp, #ForwardStubCalleeSaveArea+0x40]
        cfi_restore (x27)
        cfi_restore (x28)

        ldp     x29, x30, [sp], #ForwardStubFrameSize
        cfi_adjust_cfa_offset (-ForwardStubFrameSize)
        cfi_restore (x29)
        cfi_restore (x30)
        sub  x30, x30, #1
        ret
        .cfi_endproc
        .size R2CForwardStubLong, .-R2CForwardStubLong

////////////////////////////////////////////////////////////////////////////////

#define BoxedStubFrameSize      (8 * 16)
#define BoxedStubCalleeSaveArea (8 * 8)

#define JVALUE_SIZE 8

// extern"C" void R2CBoxedStubXXX(void *func_ptr,       x0
//                                     jvalue    *argJvalue, x1
//                                     uint_32   stackSize,      x2
//                                     uint_32   dregSize        x3)

// On execution of "bl R2CBoxedStub", the top frame of stack(growing downwards) looks like:
// x0: the entry point of Java method to be invoked
// x1: argJvalue
// x2: stackSize
// x3: dregSize
//                 |  ...        |
//                 |  x30        |
//    sp, fp  -->  |  x29        | fp for the caller of R2CBoxedStub

// this routine builds a stub frame to invoke java method
// x20: the entry point of Java method to be invoked
// x0~x7: hold the first 8 integer arguments if existed
// d0~d7: hold the first 8 float arguments if existed
//                 |  ...        |
//                 |  x30        |
// caller fp  -->  |  x29        |
// callee saved    |  ---        |
//                 |  ---        |
//                 |  r24        |
//                 |  r23        |
//                 |  r22        |
//                 |  r21        |
//                 |  r20        |
// callee saved    |  r19        |
// unwind context  | direct call | directly invoke callee java method
//                 | shadowframe | the information of caller frame which is interpreted
//                 | UCStatus    | unwind context status of caller frame
//                 | Caller LR   | LR of caller frame
//                 | Caller FP   | FP of caller frame
// unwind context  | Caller PC   | PC of caller frame
//                 | x30         |
// stub sp,fp -->  | caller fp   |

        .text
        .align 2
        .global R2CBoxedStubLong
        .hidden R2CBoxedStubLong
        .global R2CBoxedStubVoid
        .hidden R2CBoxedStubVoid
        .global R2CBoxedStubFloat
        .hidden R2CBoxedStubFloat
        .global R2CBoxedStubDouble
        .hidden R2CBoxedStubDouble
        .type R2CBoxedStubLong, %function
        .type R2CBoxedStubVoid, %function
        .type R2CBoxedStubFloat, %function
        .type R2CBoxedStubDouble, %function
R2CBoxedStubLong:
R2CBoxedStubVoid:
R2CBoxedStubFloat:
R2CBoxedStubDouble:
        .cfi_startproc
        .cfi_personality 155, DW.ref.__n2j_stub_personality
        add  x30, x30, #1
        stp  x29, x30, [sp,  #-BoxedStubFrameSize]!
        cfi_adjust_cfa_offset (BoxedStubFrameSize)
        cfi_rel_offset (x29, 0)
        cfi_rel_offset (x30, 8)

        // save all used callee-saved registers.
        stp  x19, x20, [sp, #BoxedStubCalleeSaveArea]
        cfi_rel_offset (x19, BoxedStubCalleeSaveArea)
        cfi_rel_offset (x20, BoxedStubCalleeSaveArea+8)

        stp  x21, x22, [sp, #BoxedStubCalleeSaveArea+0x10]
        cfi_rel_offset (x21, BoxedStubCalleeSaveArea+0x10)
        cfi_rel_offset (x22, BoxedStubCalleeSaveArea+0x18)

        stp  x23, x24, [sp, #BoxedStubCalleeSaveArea+0x20]
        cfi_rel_offset (x23, BoxedStubCalleeSaveArea+0x20)
        cfi_rel_offset (x24, BoxedStubCalleeSaveArea+0x28)

        mov  x20, x0 // the entry point of Java method

        mov  x21, x1 // jvalue start
        mov  x22, x2 // stack size
        mov  x23, x3 // dreg size

        mov  x29, sp
        cfi_def_cfa_register (x29)

        // frame info: tls -> stub
        mov  x0, x29
        bl MRT_SaveLastUnwindContext

        // java code is always reliable for stack unwinding
        bl MRT_SetReliableUnwindContextStatus

        cbz  x22, .LskipStack
        // copy parameter to stack
        // size align to 16 byte.
        add  x22, x22, #(16 - 1)
        and  x22, x22, #0xFFFFFFFFFFFFFFF0;
        sub  sp, sp, x22
        mov  x0, sp
        add  x24, x21, #128 // 16 * 8, jvalus offset 16
        add  x22, x24, x22
.LCopy:
        cmp  x24, x22
        bge  .LCopyEnd
        ldp  x1, x2, [x24], #16
        stp  x1, x2, [x0], #16
        b .LCopy
.LCopyEnd:
.LskipStack:
        ldp  x0, x1, [x21, #0]
        ldp  x2, x3, [x21, #16]
        ldp  x4, x5, [x21, #32]
        ldp  x6, x7, [x21, #48]

        cbz  x23, .LcallFunction
        ldp  d0, d1, [x21, #64 +  0]
        ldp  d2, d3, [x21, #64 + 16]
        ldp  d4, d5, [x21, #64 + 32]
        ldp  d6, d7, [x21, #64 + 48]

.LcallFunction:
        // set x19 to the polling page address
        adrp x19, REF_globalPollingPage
        add  x19, x19, :lo12:REF_globalPollingPage
        ldr  x19, [x19]
        ldr  x19, [x19]

        blr  x20

        /* keep potential return value */
        mov  x20, x0
        fmov  x19, d0

        /* restore last_java_frame */
        mov  x0, x29
        bl MRT_RestoreLastUnwindContext

        /* set potential return value */
        mov  x0, x20
        fmov  d0, x19

        mov  sp, x29
        cfi_def_cfa_register (sp)

        // restore all used callee-saved registers.
        ldp  x19, x20, [sp, #BoxedStubCalleeSaveArea]
        cfi_restore (x19)
        cfi_restore (x20)
        ldp  x21, x22, [sp, #BoxedStubCalleeSaveArea+0x10]
        cfi_restore (x21)
        cfi_restore (x22)
        ldp  x23, x24, [sp, #BoxedStubCalleeSaveArea+0x20]
        cfi_restore (x23)
        cfi_restore (x24)

        ldp     x29, x30, [sp], #BoxedStubFrameSize
        cfi_adjust_cfa_offset (-BoxedStubFrameSize)
        cfi_restore (x29)
        cfi_restore (x30)
        sub  x30, x30, #1
        ret
        .cfi_endproc
        .size R2CBoxedStubLong, .-R2CBoxedStubLong

////////////////////////////////////////////////////////////////////////////////

        .hidden DW.ref.__n2j_stub_personality
        .weak DW.ref.__n2j_stub_personality
        .section .data.DW.ref.__n2j_stub_personality,"awG",%progbits,DW.ref.__n2j_stub_personality,comdat
        .align 3
        .type DW.ref.__n2j_stub_personality, %object
        .size DW.ref.__n2j_stub_personality,8
DW.ref.__n2j_stub_personality:
        .xword __n2j_stub_personality

///////////////////////////////////////////////////////////////////////////////
        // Add this section for lld relocation limitation
.section .data.ref.lld_relocation_limitation,"aw",%progbits
   .type REF_globalPollingPage,@object
   .align 3
   .local REF_globalPollingPage
REF_globalPollingPage:
   .xword globalPollingPage
   .size REF_globalPollingPage,.-REF_globalPollingPage
