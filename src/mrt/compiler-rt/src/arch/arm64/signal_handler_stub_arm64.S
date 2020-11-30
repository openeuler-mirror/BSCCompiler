#define cfi_adjust_cfa_offset(off)	.cfi_adjust_cfa_offset off
#define cfi_rel_offset(reg, off)	.cfi_rel_offset reg, off
#define cfi_restore(reg)		.cfi_restore reg
#define cfi_def_cfa_register(reg)	.cfi_def_cfa_register reg

#define SignalStubFrameSize           (0x2c0)

// This signal handler stub relies on MRT_PrepareToHandleJavaSignal to put continuation pc in x17.
// Continuation pc describes the control flow after signal handling completes.

        .text
        .align 2
        .global HandleJavaSignalStub
        .hidden HandleJavaSignalStub
        .type HandleJavaSignalStub, %function
HandleJavaSignalStub:
        .cfi_startproc
        .cfi_return_column x17
        sub  sp, sp, # SignalStubFrameSize
        cfi_adjust_cfa_offset (SignalStubFrameSize)
        stp  x29, x17, [sp]
        cfi_rel_offset (x29, 0)
        cfi_rel_offset (x17, 8)

        mov  x29, sp
        cfi_def_cfa_register (x29)

        // save all non callee-saved registers which may be contaminated by calling
        // the real signal handler.
        stp  x0, x1, [sp, #0x10]
        cfi_rel_offset (x0, 0x10)
        cfi_rel_offset (x1, 0x18)

        stp  x2, x3, [sp, #0x20]
        cfi_rel_offset (x2, 0x20)
        cfi_rel_offset (x3, 0x28)

        stp  x4, x5, [sp, #0x30]
        cfi_rel_offset (x4, 0x30)
        cfi_rel_offset (x5, 0x38)

        stp  x6, x7, [sp, #0x40]
        cfi_rel_offset (x6, 0x40)
        cfi_rel_offset (x7, 0x48)

        stp  x8, x9, [sp, #0x50]
        cfi_rel_offset (x8, 0x50)
        cfi_rel_offset (x9, 0x58)

        stp  x10, x11, [sp, #0x60]
        cfi_rel_offset (x10, 0x60)
        cfi_rel_offset (x11, 0x68)

        stp  x12, x13, [sp, #0x70]
        cfi_rel_offset (x12, 0x70)
        cfi_rel_offset (x13, 0x78)

        stp  x14, x15, [sp, #0x80]
        cfi_rel_offset (x14, 0x80)
        cfi_rel_offset (x15, 0x88)

        stp  x16, x30, [sp, #0x90]
        cfi_rel_offset (x16, 0x90)
        cfi_rel_offset (x30, 0x98)

        mrs x17, nzcv
        stp  x18, x17, [sp, #0xa0]
        cfi_rel_offset (x18, 0xa0)
        cfi_rel_offset (nzcv, 0xa8)

        stp   q0,  q1, [sp, #0xc0]
        stp   q2,  q3, [sp, #0xe0]
        stp   q4,  q5, [sp, #0x100]
        stp   q6,  q7, [sp, #0x120]
        stp   q8,  q9, [sp, #0x140]
        stp  q10, q11, [sp, #0x160]
        stp  q12, q13, [sp, #0x180]
        stp  q14, q15, [sp, #0x1a0]
        stp  q16, q17, [sp, #0x1c0]
        stp  q18, q19, [sp, #0x1e0]
        stp  q20, q21, [sp, #0x200]
        stp  q22, q23, [sp, #0x220]
        stp  q24, q25, [sp, #0x240]
        stp  q26, q27, [sp, #0x260]
        stp  q28, q29, [sp, #0x280]
        stp  q30, q31, [sp, #0x2a0]

        bl InvokeJavaSignalHandler
        cbz   x0, .L_nochangex17
        str   x0, [sp, #0x8]
.L_nochangex17:

        bl MRT_SetReliableUnwindContextStatus

        bl IsThrowingExceptionByRet
        cbz   x0,  .L_noexception
        str   x0, [sp, #0x8]
.L_noexception:
        ldp   q0,  q1, [sp, #0xc0]
        ldp   q2,  q3, [sp, #0xe0]
        ldp   q4,  q5, [sp, #0x100]
        ldp   q6,  q7, [sp, #0x120]
        ldp   q8,  q9, [sp, #0x140]
        ldp  q10, q11, [sp, #0x160]
        ldp  q12, q13, [sp, #0x180]
        ldp  q14, q15, [sp, #0x1a0]
        ldp  q16, q17, [sp, #0x1c0]
        ldp  q18, q19, [sp, #0x1e0]
        ldp  q20, q21, [sp, #0x200]
        ldp  q22, q23, [sp, #0x220]
        ldp  q24, q25, [sp, #0x240]
        ldp  q26, q27, [sp, #0x260]
        ldp  q28, q29, [sp, #0x280]
        ldp  q30, q31, [sp, #0x2a0]

        ldp  x0, x1, [sp, #0x10]
        cfi_restore (x0)
        cfi_restore (x1)

        ldp  x2, x3, [sp, #0x20]
        cfi_restore (x2)
        cfi_restore (x3)

        ldp  x4, x5, [sp, #0x30]
        cfi_restore (x4)
        cfi_restore (x5)

        ldp  x6, x7, [sp, #0x40]
        cfi_restore (x6)
        cfi_restore (x7)

        ldp  x8, x9, [sp, #0x50]
        cfi_restore (x8)
        cfi_restore (x9)

        ldp  x10, x11, [sp, #0x60]
        cfi_restore (x10)
        cfi_restore (x11)

        ldp  x12, x13, [sp, #0x70]
        cfi_restore (x12)
        cfi_restore (x13)

        ldp  x14, x15, [sp, #0x80]
        cfi_restore (x14)
        cfi_restore (x15)

        ldp  x16, x30, [sp, #0x90]
        cfi_restore (x16)
        cfi_restore (x30)

        ldp  x18, x17, [sp, #0xa0]
        cfi_restore (x18)
        msr  nzcv, x17
        cfi_restore(nzcv)

        ldp     x29, x17, [sp]
        cfi_restore (x29)
        cfi_restore (x17)

        add  sp, sp, # SignalStubFrameSize
        cfi_adjust_cfa_offset (-SignalStubFrameSize)

        // we backup PC with x17 in PrepareToHandleJavaSignal already
        ret x17
        .cfi_endproc
        .size HandleJavaSignalStub, .-HandleJavaSignalStub
