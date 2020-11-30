# ++++++++WARNING++++++++WARNING++++++++WARNING++++++++
# +
# + NEED TO BE CAREFUL WHEN THIS FILE IS MODIFIED.
# +
# + This file has been hand modified to preserve x11 whenever it can.
# + It is accomplished by observing x29 == sp, and x11 is saved
# + in the same stack location as x29 instead of x29.
# + Further all .cfi directive for 29 is changed to 11
# + In addition, there is no need to perform 'mv x29, sp' and the
# + corresponding .cfe for 29.
# +
# + If any of the string functions is used by a MRT* function then
# + internal to the string function the usage of x11 is prohibited.
# + See strcmp as example.
# +
# ++++++++WARNING++++++++WARNING++++++++WARNING++++++++

#ifndef ENABLE_ASSERT_RC_NZ
#define ENABLE_ASSERT_RC_NZ 1
#endif
// following constant should be same with size.h
#define RC_COLOR_CLEAR_MASK  0x3fffffff
#define RC_CYCLE_COLOR_BROWN 0xc0000000
#define STRONG_RC_BITS_MASK  0x0000ffff
#define RC_BITS_MSB_INDEX    15
#define WEAK_COLLECTED_INDEX 29

#define WEAK_RC_ONE          0x00400000
#define WEAK_RC_BITS_MASK    0x1fc00000
#define WEAK_COLLECT         0x20000000
#define RESURRECT_STRONG_MASK  0x003fffff

        .text
        .align  2
        .p2align 3,,7
#ifndef ENABLE_LOCAL_FAST_FUNCS
        .section .text.MCC_Array_Boundary_Check,"ax",@progbits
        .global MCC_Array_Boundary_Check
#else
        .local  MCC_Array_Boundary_Check
#endif
        .type   MCC_Array_Boundary_Check, %function
MCC_Array_Boundary_Check:
.LFB9064:
        .cfi_startproc
        stp     x11, x30, [sp, -32]!
        .cfi_def_cfa_offset 32
        .cfi_offset 11, -32
        .cfi_offset 30, -24
#       add     x29, sp, 0
#       .cfi_def_cfa_register 29
        cbz     x0, .L840
        ldr     w2, [x0, 12]
        tbnz    w1, #31, .L838
        cmp     w1, w2
        bge     .L838
        ldp     x11, x30, [sp], 32
        .cfi_remember_state
        .cfi_restore 30
        .cfi_restore 11
        .cfi_def_cfa 31, 0
        ret
        .p2align 3
.L838:
        .cfi_restore_state
#   WARNING The value stored at [sp+0] should be x29
#           which has not been changed by this func
#       ldr     x1, [sp]
#   store index and length
        stp x1, x2, [sp, 16]
        mov x1, x29
        mov     x0, x30
        bl      MRT_SetRiskyUnwindContext
#   restore index and length
        ldp      x11, x30, [sp], 16
        ldp      x1, x0, [sp], 16
        .cfi_remember_state
        .cfi_restore 30
        .cfi_restore 11
        .cfi_def_cfa 31, 0
        b       MRT_ThrowArrayIndexOutOfBoundsException
        .p2align 3
.L840:
        .cfi_restore_state
#   WARNING The value stored at [sp+0] should be x29
#           which has not been changed by this func
#       ldr     x1, [sp]
        mov x1, x29
        mov     x0, x30
        bl      MRT_SetRiskyUnwindContext
        ldp     x11, x30, [sp], 32
        .cfi_restore 30
        .cfi_restore 11
        .cfi_def_cfa 31, 0
        b       MRT_ThrowNullPointerExceptionUnw
        .cfi_endproc
.LFE9064:
        .size   MCC_Array_Boundary_Check, .-MCC_Array_Boundary_Check

        .section .rodata, "a",%progbits
        .local .L.str.interface.itab
        .p2align 3
        .type   .L.str.interface.itab,@object
.L.str.interface.itab:
        .asciz  "java/lang/AbstractMethodError"
        .size   .L.str.interface.itab, 30

        .text
#ifndef ENABLE_LOCAL_FAST_FUNCS
        .section .text.MCC_getFuncPtrFromItab // -- Begin function MCC_getFuncPtrFromItab,"ax",@progbits
        .global MCC_getFuncPtrFromItab // -- Begin function MCC_getFuncPtrFromItab
#else
        .local MCC_getFuncPtrFromItab // -- Begin function MCC_getFuncPtrFromItab
#endif
        .p2align 3
        .type   MCC_getFuncPtrFromItab,@function
MCC_getFuncPtrFromItab:     // @MCC_getFuncPtrFromItab
        .cfi_startproc
// %bb.0:
        sub     sp, sp, #64             // =64
        stp     x22, x21, [sp, #16]     // 16-byte Folded Spill
        stp     x20, x19, [sp, #32]     // 16-byte Folded Spill
        stp     x29, x30, [sp, #48]     // 16-byte Folded Spill
        add     x29, sp, #48            // =48
        .cfi_def_cfa w29, 16
        .cfi_offset w30, -8
        .cfi_offset w29, -16
        .cfi_offset w19, -24
        .cfi_offset w20, -32
        .cfi_offset w21, -40
        .cfi_offset w22, -48
        ldr     w8, [x0, #92]
        cbz     w8, .LBB131_4
// %bb.1:
        cmp     w1, #203                // =203
        add     x10, x8, #8             // =8
        b.hi    .LBB131_7
.LBB131_2:                              // =>This Inner Loop Header: Depth=1
        ldr     w8, [x10], #8
        cmp     w8, w1
        b.ne    .LBB131_2
// %bb.3:
        ldur    w0, [x10, #-4]
        b       .LBB131_6
.LBB131_4:
        adrp    x0, .L.str.interface.itab
        add     x0, x0, :lo12:.L.str.interface.itab
        mov     x1, x2
.LBB131_5:
        bl      MRT_ThrowNewExceptionUnw
        mov     x0, xzr
.LBB131_6:
        ldp     x29, x30, [sp, #48]     // 16-byte Folded Reload
        ldp     x20, x19, [sp, #32]     // 16-byte Folded Reload
        ldp     x22, x21, [sp, #16]     // 16-byte Folded Reload
        add     sp, sp, #64             // =64
        ret
.LBB131_7:
        ldr     w9, [x8]
        mov     w11, wzr
        and     w12, w9, #0xffff
.LBB131_8:                              // =>This Inner Loop Header: Depth=1
        add     w13, w11, w12
        and     w14, w13, #0xfffffffe
        ldr     w14, [x10, w14, uxtw #2]
        cmp     w14, w1
        b.eq    .LBB131_10
// %bb.9:                               //   in Loop: Header=BB131_8 Depth=1
        lsr     w15, w13, #1
        cmp     w14, w1
        sub     w14, w15, #1            // =1
        csel    w12, w12, w14, lo
        csinc   w11, w11, w15, hs
        cmp     w11, w12
        b.ls    .LBB131_8
.LBB131_10:
        orr     w11, w13, #0x1
        ldr     w0, [x10, w11, uxtw #2]
        cmp     w0, #1                  // =1
        b.ne    .LBB131_6
// %bb.11:
        stp     x8, x2, [sp]            // 16-byte Folded Spill
        ubfx    x8, x9, #15, #17
        cmp     w9, #0                  // =0
        and     w8, w8, #0xfffe
        orr     w10, wzr, #0xfffffffe
        csel    w8, w8, w10, lt
        cbz     w8, .LBB131_15
// %bb.12:
        ubfiz   x9, x9, #1, #16
        add     x20, x9, #2             // =2
        ldr     x9, [sp]                // 8-byte Folded Reload
        mov     x19, xzr
        mov     w21, w8
        add     x9, x9, x20, lsl #2
        sub     x22, x9, #8             // =8
.LBB131_13:                             // =>This Inner Loop Header: Depth=1
        add     x8, x22, x19, lsl #2
        ldr     w1, [x8, #8]
        ldr     x0, [sp, #8]            // 8-byte Folded Reload
        bl      strcmp
        cbz     w0, .LBB131_16
// %bb.14:                              //   in Loop: Header=BB131_13 Depth=1
        add     x19, x19, #2            // =2
        cmp     x19, x21
        b.lo    .LBB131_13
.LBB131_15:
        ldr     x1, [sp, #8]            // 8-byte Folded Reload
        adrp    x0, .L.str.interface.itab
        add     x0, x0, :lo12:.L.str.interface.itab
        b       .LBB131_5
.LBB131_16:
        ldr     x8, [sp]                // 8-byte Folded Reload
        add     x8, x8, x20, lsl #2
        add     x8, x8, x19, lsl #2
        ldr     w0, [x8, #4]
        b       .LBB131_6
.Lfunc_end131:
        .size   MCC_getFuncPtrFromItab, .Lfunc_end131-MCC_getFuncPtrFromItab
        .cfi_endproc


#define STRING_LENGTH_OFFSET 8
#define STRING_HASH_OFFSET 12

        .section        .rodata
        .align  2
.Lmethod_desc.string_equals:
        .word __methods_compact__string_equals - .
        .short 16
        .short 0
        .section  .java_text,"ax"
#ifndef ENABLE_LOCAL_FAST_FUNCS
        .global  __string_equals
#else
        .local  __string_equals
#endif
        .type   __string_equals, %function
        .align 2
        .word .Lmethod_desc.string_equals - .
__string_equals:
        .cfi_startproc
        .cfi_personality 155, DW.ref.__mpl_personality_v0
        ldp     x4, x2, [x0], #16               // read class shadow and count of this, replace assertnonnull
        cbz     x1, .Label.string_equals_false  // if the other is null
        cmp     x0, x1
        beq     .Label.string_equals_true       // same address, return true
        ldp     x5, x3, [x1], #16               // read class shadow and count of the other
        cmp     w4, w5
        bne     .Label.string_equals_false      // if not the same class
        cmp     w2, w3                          //  Length and compression flag are different
        bne     .Label.string_equals_morecheck
        and     w3, w2, #1
        lsr     w2, w2, w3                      // get the number of bytes
        cbz     w2, .Label.string_equals_true   //  zero length
        add     w2, w2, #7
        lsr     w5, w2, 4
        cbz     w5, .Label.string_equals_tail2  // size is shorter than 8
        add     x5, x0, x5, lsl 4
.Label.string_equals_loophead:
        ldp     x3, x6, [x0], #16
        ldp     x4, x7, [x1], #16
        cmp     x3, x4
        bne     .Label.string_equals_false
        cmp     x6, x7
        bne     .Label.string_equals_false
        cmp     x0, x5
        bne     .Label.string_equals_loophead
.Label.string_equals_tail:
        tbz     w2, #3, .Label.string_equals_true
.Label.string_equals_tail2:
        ldr     x3, [x0]
        ldr     x2, [x1]
        cmp     x3, x2
        bne     .Label.string_equals_false
.Label.string_equals_true:
        mov     w0, 1
        ret
.Label.string_equals_false:
        mov     w0, 0
        ret
.Label.string_equals_morecheck:         // check for compressed bits are diff byte size are equal
        eor     w2, w2, w3
        cmp     w2, #1
        bne     .Label.string_equals_false
        tbnz    w3, #0, .Label.string_equals_loop2
        mov     x2, x0
        mov     x0, x1
        mov     x1, x2    // swap x0 and x1 to make sure that x0 is normal and x1 is compressed
.Label.string_equals_loop2:
        lsr     w3, w3, #1
        // x0: normal, x1; compressed, w3: length
.Label.string_equals_loop2_head:
        subs    w3, w3, #1
        tbnz    w3, #31, .Label.string_equals_true
        ldrh    w4, [x0,w3,SXTW #1]  // normal string
        ldrb    w5, [x1,w3,SXTW #0]  // compressed string
        cmp     w4, w5
        beq     .Label.string_equals_loop2_head
        b       .Label.string_equals_false
       .cfi_endproc
.Label.end.__string_equals:
        .size   __string_equals, .-__string_equals
        .word 0x55555555

        .data
        .align  3
        .type  __ref__string_equals, %object
        .local __ref__string_equals
__ref_string_equals:
        .quad  __string_equals
        .size  __ref_string_equals, . - __ref_string_equals

        .data
        .align  3
        .type  __methods_compact__string_equals, %object
        .local __methods_compact__string_equals
__methods_compact__string_equals:
        .long   0                            // methodInVtabIndex
        .long   0                            // declaringClass
        .long   0                            // addr
        .long   0                            // mod
        .long   __namestr__string_equals - . // name
        .long   0                            // signature
        .long   0                            // annotation
        .short  0                            // flag
        .short  2                            // argsize
        .data
        .type __namestr__string_equals, %object
        .local __namestr__string_equals
__namestr__string_equals:
        .string "__string_equals"

        .section        .rodata
        .align  2
.Lmethod_desc.string_hashCode:
        .word __methods_compact_string_hashCode - .
        .short 16
        .short 0
        .section  .java_text,"ax"
        .align 2
#ifndef ENABLE_LOCAL_FAST_FUNCS
        .global __string_hashCode
#else
        .local  __string_hashCode
#endif
        .type   __string_hashCode, %function
        .word .Lmethod_desc.string_hashCode - .
__string_hashCode:
.Label.__string_hashCode9:
        .cfi_startproc
        .cfi_personality 155, DW.ref.__mpl_personality_v0
        ldr     w2, [x0,#STRING_HASH_OFFSET]         // Ljava/lang/String;.hash
        cbnz    w2, .Label.__string_hashCode1
        ldr     w1, [x0,#STRING_LENGTH_OFFSET]          // length + flag for compressed string
        lsr     w4, w1, #1
        cbz     w4, .Label.__string_hashCode1
        mov     w5, #0               // start offset
        add     x3, x0, #16          // start address
        tbz     w1, #0, .Label.__string_hashCode4
.Label.__string_hashCode3:
        ldrb    w6, [x3,w5,SXTW #0]  // compressed string
        lsl     w1, w2, #5
        add     w5, w5, #1
        sub     w1, w1, w2
        cmp     w5, w4
        add     w2, w1, w6
        blt     .Label.__string_hashCode3
        b       .Label.__string_hashCode6
.Label.__string_hashCode4:
        ldrh    w6, [x3,w5,SXTW #1]  // normal string
        lsl     w1, w2, #5
        add     w5, w5, #1
        sub     w1, w1, w2
        cmp     w5, w4
        add     w2, w1, w6
        blt     .Label.__string_hashCode4
.Label.__string_hashCode6:
        str     w2, [x0,#12]         // hash = h
.Label.__string_hashCode1:
        mov     w0, w2
.Label.__string_hashCode12:
        ret
.Label.__string_hashCode11:
        b       .Label.__string_hashCode12
.Label.__string_hashCode10:
        .cfi_endproc
.Label.end.__string_hashCode:
        .size   __string_hashCode, .-__string_hashCode
        .word 0x55555555

        .data
        .align  3
        .type  __ref_string_hashCode, %object
        .local __ref_string_hashCode
__ref_string_hashCode:
        .quad  __string_hashCode
        .size  __ref_string_hashCode, . - __ref_string_hashCode

        .data
        .align  3
        .type  __methods_compact_string_hashCode, %object
        .local __methods_compact_string_hashCode
__methods_compact_string_hashCode:
        .long   0                              // methodInVtabIndex
        .long   0                              // declaringClass
        .long   0                              // addr
        .long   0                              // mod
        .long   __namestr__string_hashCode - . // name
        .long   0                              // signature
        .long   0                              // annotation
        .short  0                              // flag
        .short  1                              // argsize

        .data
        .type __namestr__string_hashCode, %object
        .local __namestr__string_hashCode
__namestr__string_hashCode:
        .string "__string_hashCode"

        // The following implementation is android specific.
        // ARM64-linux will call into libs-fast directly
        // (with the cost of an extra plt)
#ifdef __ANDROID__
        .text
        .align   2
        .p2align 3,,7
#ifndef ENABLE_LOCAL_FAST_FUNCS
        .section .text.MCC_PreNativeCall,"ax",@progbits
        .global  MCC_PreNativeCall
#else
        .local  MCC_PreNativeCall
#endif
        .type   MCC_PreNativeCall, %function
MCC_PreNativeCall:
.Lfunc_begin2:
        .cfi_startproc
        mrs     x8, TPIDR_EL0
        ldr     x8, [x8, #56]
        ldr     x8, [x8, #56]
        ldr     x9, [x8]
        mov     x0, x8
        ldr     x1, [x9, #8]
        br      x1
.Lfunc_end2:
        .size   MCC_PreNativeCall, .Lfunc_end2-MCC_PreNativeCall
        .cfi_endproc

        .text
        .align   2
        .p2align 3,,7
#ifndef ENABLE_LOCAL_FAST_FUNCS
        .section .text.MCC_PostNativeCall,"ax",@progbits
        .global  MCC_PostNativeCall
#else
        .local  MCC_PostNativeCall
#endif
        .type   MCC_PostNativeCall, %function
MCC_PostNativeCall:                     // @MCC_PostNativeCall
.Lfunc_begin3:
        .cfi_startproc
        mrs     x8, TPIDR_EL0
        ldr     x8, [x8, #56]
        mov     x1, x0
        ldr     x8, [x8, #56]
        ldr     x9, [x8]
        mov     x0, x8
        ldr     x2, [x9, #16]
        br      x2
.Lfunc_end3:
        .size   MCC_PostNativeCall, .Lfunc_end3-MCC_PostNativeCall
        .cfi_endproc

#endif // __ANDROID__

#ifdef __ANDROID__
        .text
#ifndef ENABLE_LOCAL_FAST_FUNCS
        .section .text.MCC_SetRiskyUnwindContext // -- Begin function MCC_SetRiskyUnwindContext,"ax",@progbits
        .global  MCC_SetRiskyUnwindContext // -- Begin function MCC_SetRiskyUnwindContext
#else
        .local MCC_SetRiskyUnwindContext // -- Begin function MCC_SetRiskyUnwindContext
#endif
        .p2align 2
        .type   MCC_SetRiskyUnwindContext,@function
MCC_SetRiskyUnwindContext:
.Lfunc_begin18:
        .cfi_startproc
        mrs     x8, TPIDR_EL0
        ldr     x8, [x8, #56]
        cbz     x8, .LBB18_2
        ldr     x8, [x8]
.LBB18_2:
        orr     w9, wzr, #0x2
        stp     x1, x0, [x8, #96]
        str     xzr, [x8, #136]
        stp     xzr, xzr, [x8, #112]
        str     w9, [x8, #80]
        ret
.Lfunc_end18:
        .size   MCC_SetRiskyUnwindContext, .Lfunc_end18-MCC_SetRiskyUnwindContext
        .cfi_endproc

        .text
#ifndef ENABLE_LOCAL_FAST_FUNCS
        .section .text.MCC_SetReliableUnwindContext // -- Begin function MCC_SetReliableUnwindContext,"ax",@progbits
        .global  MCC_SetReliableUnwindContext // -- Begin function MCC_SetReliableUnwindContext
#else
        .local MCC_SetReliableUnwindContext // -- Begin function MCC_SetReliableUnwindContext
#endif
        .p2align 2
        .type   MCC_SetReliableUnwindContext,@function
MCC_SetReliableUnwindContext:     // @MCC_SetReliableUnwindContext
.Lfunc_begin19:
        .cfi_startproc
        mrs     x8, TPIDR_EL0
        ldr     x8, [x8, #56]
        orr     w9, wzr, #0x1
        ldr     x8, [x8]
        str     w9, [x8, #80]
        ret
.Lfunc_end19:
        .size   MCC_SetReliableUnwindContext, .Lfunc_end19-MCC_SetReliableUnwindContext
        .cfi_endproc

//libmrtbase/include/thread-offsets.h

        .text
        .align  2
        .p2align 3,,7
#ifndef ENABLE_LOCAL_FAST_FUNCS
        .section .text.MCC_SyncExitFast,"ax",@progbits
        .global MCC_SyncExitFast
#else
        .local MCC_SyncExitFast
#endif
        .type  MCC_SyncExitFast, %function
MCC_SyncExitFast:
.Lfunc_begin4:
        .cfi_startproc
        adrp    x8, :got:_ZN5maple7IThread11is_started_E
        ldr     x8, [x8, :got_lo12:_ZN5maple7IThread11is_started_E]
        ldr     x8, [x8]
        ldrb    w8, [x8]
        cmp     w8, #1
        b.ne    .LBB4_7
        mrs     x8, TPIDR_EL0
        ldr     x8, [x8, #56]
        cbz     x8, .LBB4_7
        cbz     x0, .LBB4_7
        ldr     x8, [x8, #56]
        cbz     x8, .LBB4_7
        add     x9, x0, #4
        ldar    w10, [x9]
        ldr     w11, [x8, #8]
        and     w12, w10, #0xc000ffff
        cmp     w11, w12
        b.ne    .LBB4_7
        ldaxr   w12, [x9]
        and     w11, w10, #0x10000000
        sub     w13, w10, #16, lsl #12  // =65536
        tst     w10, #0xfff0000
        csel    w11, w11, w13, eq
        cmp     w12, w10
        b.ne    .LBB4_8
        stlxr   w10, w11, [x9]
        cbz     w10, .LBB4_9
.LBB4_7:
        b       MRT_BuiltinSyncExit
.LBB4_8:
        clrex
        b       MRT_BuiltinSyncExit
.LBB4_9:
        ldr     x9, [x8]
        mov     x0, x8
        ldr     x1, [x9, #304]
        br      x1
.Lfunc_end4:
        .size   MCC_SyncExitFast, .Lfunc_end4-MCC_SyncExitFast
        .cfi_endproc

        .text
        .align  2
        .p2align 3,,7
#ifndef ENABLE_LOCAL_FAST_FUNCS
        .section .text.MCC_SyncEnterFast2,"ax",@progbits
        .global MCC_SyncEnterFast2
#else
        .local MCC_SyncEnterFast2
#endif
        .type  MCC_SyncEnterFast2, %function
MCC_SyncEnterFast2:
.Lfunc_begin0:
        .cfi_startproc
        adrp    x8, :got:_ZN5maple7IThread11is_started_E
        ldr     x8, [x8, :got_lo12:_ZN5maple7IThread11is_started_E]
        mov     x1, x0
        ldr     x8, [x8]
        ldrb    w8, [x8]
        cmp     w8, #1
        b.ne    .LBB0_13
        mrs     x8, TPIDR_EL0
        ldr     x8, [x8, #56]
        cbz     x8, .LBB0_13
        cbz     x1, .LBB0_13
        ldr     x0, [x8, #56]
        cbz     x0, .LBB0_13
        add     x8, x1, #4
        ldar    w9, [x8]
        ldr     w10, [x0, #8]
        tst     w9, #0xefffffff
        b.ne    .LBB0_8
        ldaxr   w11, [x8]
        cmp     w11, w9
        b.ne    .LBB0_12
        orr     x9, x10, x9
        stlxr   w10, w9, [x8]
        cbnz    w10, .LBB0_13
.LBB0_7:
        ldr     x8, [x0]
        orr     w3, wzr, #0x1
        mov     x2, x30
        ldr     x4, [x8, #288]
        br      x4
.LBB0_8:

        and     w11, w9, #0xc000ffff
        cmp     w10, w11
        b.ne    .LBB0_13
        and     w10, w9, #0xfff0000
        orr     w11, wzr, #0xfff0000
        cmp     w10, w11
        b.eq    .LBB0_13

        ldaxr   w10, [x8]
        cmp     w10, w9
        b.ne    .LBB0_12
        add     w9, w9, #16, lsl #12    // =65536
        stlxr   w10, w9, [x8]
        cbnz    w10, .LBB0_13
        b       .LBB0_7
.LBB0_12:
        clrex
.LBB0_13:
        mov     x0, x1
        b       MRT_BuiltinSyncEnter
.Lfunc_end0:
        .size   MCC_SyncEnterFast2, .Lfunc_end0-MCC_SyncEnterFast2
        .cfi_endproc
#endif // __ANDROID__

#ifdef ENABLE_LOCAL_FAST_FUNCS
#if !DISABLE_RC_DUPLICATE
        .text
        .align  2
        .p2align 3,,7
        .local  MCC_DecRefResetPair
        .type  MCC_DecRefResetPair, %function
MCC_DecRefResetPair:
        .cfi_startproc
        adrp    x2, gcIsGCOnly
        ldr     x2, [x2, :lo12:gcIsGCOnly]
        cbz     x2, .L_RC_DECREF_RESET_PAIR
        str     xzr, [x0]
        str     xzr, [x1]
        ret
.L_RC_DECREF_RESET_PAIR:
.LResetPairSTART:
        stp     x20, x30, [sp, -16]!
        .cfi_def_cfa_offset 16
        .cfi_offset 20, -16
        .cfi_offset 30, -8
        ldr     x2, [x0]
        mov     x20, x1
        str     xzr, [x0]
        mov     x0, x2
        bl      MCC_DecRef_NaiveRCFast
        ldr     x0, [x20]
        str     xzr, [x20]
        bl      MCC_DecRef_NaiveRCFast
        ldp     x20, x30, [sp], 16
        .cfi_remember_state
        .cfi_restore 30
        .cfi_restore 20
        .cfi_def_cfa 31, 0
        ret
        .cfi_endproc
        .size   MCC_DecRefResetPair, .-MCC_DecRefResetPair

        .text
        .align  2
        .p2align 3,,7
        .local  MCC_IncDecRefReset
        .type   MCC_IncDecRefReset, %function
MCC_IncDecRefReset:
        .cfi_startproc
        adrp    x2, gcIsGCOnly
        ldr     x2, [x2, :lo12:gcIsGCOnly]
        cbz     x2, .L_RC_IncDec_Reset
        str     xzr, [x1]
        ret
.L_RC_IncDec_Reset:
        ldr     x2, [x1]
        str     xzr, [x1]
        mov     x1, x2
        b       MCC_IncDecRef_NaiveRCFast
        .cfi_endproc
        .size   MCC_IncDecRefReset, .-MCC_IncDecRefReset

        .text
        .align  2
        .p2align 3,,7
        .local  MCC_IncRef_NaiveRCFast
        .type   MCC_IncRef_NaiveRCFast, %function
MCC_IncRef_NaiveRCFast:
.LFB7178:
        .cfi_startproc
    # WARNING - presersing x11
    #         - replacing x29 with sp
    #         - there should be no usage of x29
        adrp    x1, gcIsGCOnly
        ldr     x1, [x1, :lo12:gcIsGCOnly]
        cbz     x1, .L_RC_INC
        ret
.L_RC_INC:
        sub     x1, x0, #16, lsl #12    // x1 = x0 - 65536
        mov     w2, #2147418111         // x2 = 2^31 - 65537
        cmp     x1, x2                  // if (x1 >= x2)
        bhi     .L81                    //   goto .L81
        stp     x11, x30, [sp, -32]!
        .cfi_def_cfa_offset 32
        .cfi_offset 11, -32
        .cfi_offset 30, -24
        sub     x2, x0, #4
#       add     x29, sp, 0
#       .cfi_def_cfa_register 29
        .p2align 2
.L73:
        ldar    w4, [x2]
        tbnz    w4, #RC_BITS_MSB_INDEX, .L80
        and     w1, w4, RC_COLOR_CLEAR_MASK
        add     w1, w1, 1
        ldaxr   w3, [x2]
        cmp     w3, w4
        bne     .L73
        stlxr   w5, w1, [x2]
        cbnz w5, .L73
#if ENABLE_ASSERT_RC_NZ
        tst     x4, STRONG_RC_BITS_MASK  // check inc from 0
        beq     .L83
#endif
.L80:
        ldp     x11, x30, [sp], 32
        .p2align 2
.L81:
        .cfi_remember_state
        .cfi_restore 30
        .cfi_restore 11
        .cfi_def_cfa 31, 0
        ret
        .p2align 3
.L83:
        .cfi_restore_state
        bl      MRT_BuiltinAbortSaferegister
        .cfi_endproc
.LFE7178:
        .size   MCC_IncRef_NaiveRCFast, .-MCC_IncRef_NaiveRCFast

        .text
        .align  2
        .p2align 3,,7
        .local  MCC_ClearLocalStackRef
        .type   MCC_ClearLocalStackRef, %function
MCC_ClearLocalStackRef:
        .cfi_startproc
        adrp    x1, gcIsGCOnly
        ldr     x1, [x1, :lo12:gcIsGCOnly]
        cbz     x1, .L_RC_CLEAR_LOCAL
        str     xzr, [x0]
        ret
.L_RC_CLEAR_LOCAL:
        ldr     x1, [x0]
        str     xzr, [x0]
        mov     x0, x1
        b       MCC_DecRef_NaiveRCFast
        .cfi_endproc
        .size   MCC_ClearLocalStackRef, .-MCC_ClearLocalStackRef

        .text
        .align  2
        .p2align 3,,7
        .local  MCC_LoadRefField_NaiveRCFast
        .type   MCC_LoadRefField_NaiveRCFast, %function
MCC_LoadRefField_NaiveRCFast:
.LFB7197:
        .cfi_startproc
    # WARNING - presersing x11
    #         - replacing x29 with sp
    #         - there should be no usage of x29
        cbz     x0, .L_RC_LOAD_REF
        adrp    x3, gcIsGCOnly
        ldr     x3, [x3, :lo12:gcIsGCOnly]
        cbz     x3, .L_RC_LOAD_REF
#ifdef USE_32BIT_REF
        ldr     w0, [x1] // load 32bit-ref-field
#else
        ldr     x0, [x1] // load 64bit-ref-field
#endif //USE_32BIT_REF
        ret
.L_RC_LOAD_REF:
        stp     x11, x30, [sp, -64]!
        .cfi_def_cfa_offset 64
        .cfi_offset 11, -64
        .cfi_offset 30, -56
#       add     x29, sp, 0
#       .cfi_def_cfa_register 29
.LEHB15:
        stp     x19, x20, [sp, 16]
        .cfi_offset 19, -48
        .cfi_offset 20, -40
        mov     x19, x0
        str     x21, [sp, 32]
        .cfi_offset 21, -32
        mov     x20, x1
        mov     x21, x30
        cbz     x0, .L185
.L174:
#ifdef USE_32BIT_REF
        ldr     w1, [x20] // load 32bit-ref-field
#else
        ldr     x1, [x20] // load 64bit-ref-field
#endif //USE_32BIT_REF
        tbnz    x1, #0, .L178
        mov     x0, x1
        sub     x2, x1, #16, lsl #12    // x2 = x1 - 65536
        mov     w3, #2147418111         // x3 = 2^31 - 65537
        cmp     x2, x3                  // if (x2 < x3)
        bls     .L186                   //   goto .L186
        .p2align 2
.L175:
        ldp     x19, x20, [sp, 16]
        ldr     x21, [sp, 32]
.LEHE15:
        ldp     x11, x30, [sp], 64
        .cfi_remember_state
        .cfi_restore 30
        .cfi_restore 11
        .cfi_restore 21
        .cfi_restore 19
        .cfi_restore 20
        .cfi_def_cfa 31, 0
        ret
        .p2align 3
.L186:
        .cfi_restore_state
        sub     x1, x1, #4
        ldar    w2, [x1]
        tbnz    w2, #RC_BITS_MSB_INDEX, .L175
        ands    w3, w2, STRONG_RC_BITS_MASK
        str     w2, [sp, 60]
        beq     .L178
        ands    w3, w2, RC_COLOR_CLEAR_MASK
        add     w3, w3, 1
        ldaxr   w4, [x1]
        cmp     w4, w2
        bne     .L183
        stlxr   w5, w3, [x1]
        cmp     w5, 0
.L183:
        beq     .L175
        str     w4, [sp, 60]
.L178:
#   WARNING The value stored at [sp+0] should be x29
#           which has not been changed by this func
#       ldr     x1, [sp]
        mov     x1, x29
        mov     x0, x21
.LEHB16:
        bl      MRT_SetRiskyUnwindContext
        mov     x1, x20
        mov     x0, x19
        bl      MRT_LoadRefField

        mov     x19, x0
        bl      MRT_SetReliableUnwindContextStatus
        mov     x0, x19

        ldp     x19, x20, [sp, 16]
        ldr     x21, [sp, 32]
        ldp     x11, x30, [sp], 64
        .cfi_remember_state
        .cfi_restore 20
        .cfi_restore 19
        .cfi_restore 21
        .cfi_restore 11
        .cfi_restore 30
        .cfi_def_cfa 31, 0
        ret
        .p2align 3
.L185:
        .cfi_restore_state
#   WARNING The value stored at [sp+0] should be x29
#           which has not been changed by this func
#       ldr     x1, [sp]
        mov     x1, x29
        mov     x0, x30
        bl      MRT_SetRiskyUnwindContext
        bl      MRT_ThrowNullPointerExceptionUnw
.LEHE16:
        b      .L174
        .cfi_endproc
.LFE7197:
        .size   MCC_LoadRefField_NaiveRCFast, .-MCC_LoadRefField_NaiveRCFast

        .text
        .align  2
        .p2align 3,,7
        .local MCC_CleanupLocalStackRef_NaiveRCFast
        .type   MCC_CleanupLocalStackRef_NaiveRCFast, %function
MCC_CleanupLocalStackRef_NaiveRCFast:
        .cfi_startproc
        adrp    x2, gcIsGCOnly
        ldr     x2, [x2, :lo12:gcIsGCOnly]
        cbz     x2, .L_RC_CLEANUP
        ret
.L_RC_CLEANUP:
#if CONFIG_JSAN
        stp     x29, x30, [sp, -32]!
        .cfi_def_cfa_offset 32
        .cfi_offset 29, -32
        .cfi_offset 30, -24
        mov     x29, sp
        str     x11, [x29, 16]
        .cfi_offset 11, -16
        bl      MCC_CleanupLocalStackRef
        ldr     x11, [x29, 16]
        ldp     x29, x30, [sp], 32
        .cfi_restore 11
        .cfi_restore 29
        .cfi_restore 30
        .cfi_def_cfa 31, 0
        ret
        .cfi_endproc
#else
.LFB7168:
    # WARNING - presersing x11
    #         - replacing x29 with sp
    #         - there should be no usage of x29
        cbz     x1, .L150
        stp     x11, x30, [sp, -48]!
        .cfi_def_cfa_offset 48
        .cfi_offset 11, -48
        .cfi_offset 30, -40
#       add     x29, sp, 0
#       .cfi_def_cfa_register 29
        stp     x19, x20, [sp, 16]
        .cfi_offset 19, -32
        .cfi_offset 20, -24
        mov     x20, x1
        str     x21, [sp, 32]
        .cfi_offset 21, -16
        mov     x19, 0
        mov     x21, x0
        b       .L120
        .p2align 3
.L101:
        cmp     x20, x19
        beq     .L110
.L120:
        ldr     x0, [x21, x19, lsl 3]
        add     x19, x19, 1
        cbz     x0, .L101
        bl      MCC_DecRef_NaiveRCFast
        cmp     x20, x19
        bne     .L120
.L110:
        ldp     x19, x20, [sp, 16]
        ldr     x21, [sp, 32]
        ldp     x11, x30, [sp], 48
        .cfi_restore 30
        .cfi_restore 11
        .cfi_restore 21
        .cfi_restore 19
        .cfi_restore 20
        .cfi_def_cfa 31, 0
        ret
.L150:
        ret
        .cfi_endproc
.LFE7168:
#endif
        .size   MCC_CleanupLocalStackRef_NaiveRCFast, .-MCC_CleanupLocalStackRef_NaiveRCFast

        .text
        .align  2
        .p2align 3,,7
        .local MCC_CleanupLocalStackRefSkip_NaiveRCFast
        .type   MCC_CleanupLocalStackRefSkip_NaiveRCFast, %function
MCC_CleanupLocalStackRefSkip_NaiveRCFast:
        .cfi_startproc
        adrp    x3, gcIsGCOnly
        ldr     x3, [x3, :lo12:gcIsGCOnly]
        cbz     x3, .L_RC_CLEANUP_SKIP
        ret
.L_RC_CLEANUP_SKIP:
#if CONFIG_JSAN
        stp     x29, x30, [sp, -32]!
        .cfi_def_cfa_offset 32
        .cfi_offset 29, -32
        .cfi_offset 30, -24
        mov     x29, sp
        str     x11, [x29, 16]
        bl      MCC_CleanupLocalStackRefSkip
        .cfi_offset 11, -16
        ldr     x11, [x29, 16]
        ldp     x29, x30, [sp], 32
        .cfi_restore 11
        .cfi_restore 29
        .cfi_restore 30
        .cfi_def_cfa 31, 0
        ret
        .cfi_endproc
#else
.LFB7169:
    # WARNING - presersing x11
    #         - replacing x29 with sp
    #         - there should be no usage of x29
        cbz     x1, .L121
        stp     x11, x30, [sp, -48]!
        .cfi_def_cfa_offset 48
        .cfi_offset 11, -48
        .cfi_offset 30, -40
#       add     x29, sp, 0
#       .cfi_def_cfa_register 29
        stp     x19, x20, [sp, 16]
        .cfi_offset 19, -32
        .cfi_offset 20, -24
        mov     x20, x1
        stp     x21, x22, [sp, 32]
        .cfi_offset 21, -16
        .cfi_offset 22, -8
        mov     x19, 0
        mov     x22, x0
        mov     x21, x2
        b       .L118
        .p2align 3
.L113:
        add     x19, x19, 1
        cmp     x20, x19
        beq     .L122
.L118:
        cmp     x21, x19
        beq     .L113
        ldr     x0, [x22, x19, lsl 3]
        cbz     x0, .L113
        bl      MCC_DecRef_NaiveRCFast
        add     x19, x19, 1
        cmp     x20, x19
        bne     .L118
.L122:
        ldp     x19, x20, [sp, 16]
        ldp     x21, x22, [sp, 32]
        ldp     x11, x30, [sp], 48
        .cfi_restore 30
        .cfi_restore 11
        .cfi_restore 21
        .cfi_restore 22
        .cfi_restore 19
        .cfi_restore 20
        .cfi_def_cfa 31, 0
        ret
.L121:
        ret
        .cfi_endproc
.LFE7169:
#endif
        .size   MCC_CleanupLocalStackRefSkip_NaiveRCFast, .-MCC_CleanupLocalStackRefSkip_NaiveRCFast

        .text
        .align  2
        .p2align 3,,7
        .local MCC_DecRef_NaiveRCFast
        .type   MCC_DecRef_NaiveRCFast, %function
MCC_DecRef_NaiveRCFast:
        .cfi_startproc
        adrp    x1, gcIsGCOnly
        ldr     x1, [x1, :lo12:gcIsGCOnly]
        cbz     x1, .L_RC_DEC_REF
        ret
.L_RC_DEC_REF:
#if CONFIG_JSAN
        stp     x29, x30, [sp, -32]!
        .cfi_def_cfa_offset 32
        .cfi_offset 29, -32
        .cfi_offset 30, -24
        mov     x29, sp
        str     x11, [x29, 16]
        .cfi_offset 11, -16
        bl      MRT_DecRef
        ldr     x11, [x29, 16]
        ldp     x29, x30, [sp], 32
        .cfi_restore 11
        .cfi_restore 29
        .cfi_restore 30
        .cfi_def_cfa 31, 0
        ret
        .cfi_endproc
#else
.LFB7179:
        sub     x1, x0, #16, lsl #12    // x1 = x0 - 65536
        mov     w2, #2147418111         // x2 = 2^31 - 65537
        cmp     x1, x2                  // if (x1 >= x2)
        bhi     .L76                    //   goto .L76
        ldr     w2, [x0, -8]
        tbnz    w2, #31, .L79
        sub     x1, x0, #4
.L91:
        ldxr    w2, [x1]
        tbnz    w2, #RC_BITS_MSB_INDEX, .L76
        sub     w3, w2, #1
        orr     w3, w3, #RC_CYCLE_COLOR_BROWN
        // start check if weak collected bit can set
        // 1. weak collected bit not set
        tbnz     w3, #WEAK_COLLECTED_INDEX, .L95
        // 2. newHeader's weak rc > 1
        and     w4, w3, WEAK_RC_BITS_MASK
        cmp     w4, WEAK_RC_ONE
        ble .L95
        // 3. newHeader's strong and resurrect rc is zero
        and     w4, w3, RESURRECT_STRONG_MASK
        cbnz    w4, .L95
        // 4. set weak collected bit
        orr w3, w3, #WEAK_COLLECT
        // end weak collected bit check
.L95:
        stxr    w4, w3, [x1]
        cbnz    w4, .L91
        // old rcheader in w2 and new rcheader in w3
        // as imm is too long, split release operation
        // check strong rc first if old strong rc is 1, if not return
        and     w1, w2, STRONG_RC_BITS_MASK
        cmp     w1, 1
        bne .L94
        // check if release object
        and w1, w3, RC_COLOR_CLEAR_MASK
        cmp w1, WEAK_RC_ONE
        beq     .L92
        eor w1, w1, #WEAK_COLLECT
        cbz w1, .L92
        // check if weak collected
        tbz     w3, #WEAK_COLLECTED_INDEX, .L94
        tbz     w2, #WEAK_COLLECTED_INDEX, .L93
.L94:
#if ENABLE_ASSERT_RC_NZ
  and   w1, w2, STRONG_RC_BITS_MASK
        cbz     w1, .L309
#endif
.L76:
        ret
        .p2align 3
.L79:
        b       MRT_DecRef
.L309:
        stp     x29, x30, [sp, -16]!
        .cfi_def_cfa_offset 16
        .cfi_offset 29, -16
        .cfi_offset 30, -8
        add     x29, sp, 0
        .cfi_def_cfa_register 29
        bl      MRT_BuiltinAbortSaferegister
        .p2align 3
.L93:
        .cfi_def_cfa 31, 0
        .cfi_restore 29
        .cfi_restore 30
        b       MRT_CollectWeakObj
.L92:
        .cfi_def_cfa 31, 0
        .cfi_restore 29
        .cfi_restore 30
        b       MRT_ReleaseObj
        .cfi_endproc
.LFE7179:
#endif
        .size   MCC_DecRef_NaiveRCFast, .-MCC_DecRef_NaiveRCFast
        .text
        .align  2
        .p2align 3,,7
        .local MCC_IncDecRef_NaiveRCFast
        .type   MCC_IncDecRef_NaiveRCFast, %function
MCC_IncDecRef_NaiveRCFast:
        .cfi_startproc
        adrp    x2, gcIsGCOnly
        ldr     x2, [x2, :lo12:gcIsGCOnly]
        cbz     x2, .L_RC_INC_DEC
        ret
.L_RC_INC_DEC:
#if CONFIG_JSAN
        stp     x29, x30, [sp, -32]!
        .cfi_def_cfa_offset 32
        .cfi_offset 29, -32
        .cfi_offset 30, -24
        mov     x29, sp
        str     x11, [x29, 16]
        .cfi_offset 11, -16
        bl      MRT_IncDecRef
        ldr     x11, [x29, 16]
        ldp     x29, x30, [sp], 32
        .cfi_restore 11
        .cfi_restore 29
        .cfi_restore 30
        .cfi_def_cfa 31, 0
        ret
        .cfi_endproc
#else
.LFB7170:
    # WARNING - presersing x11
    #         - replacing x29 with sp
    #         - there should be no usage of x29
        cmp     x0, x1
        beq     .L142
        stp     x11, x30, [sp, -32]!
        .cfi_def_cfa_offset 32
        .cfi_offset 11, -32
        .cfi_offset 30, -24
        mov     x2, x0
        mov     x0, x1
        mov     w7, #2147418111         // x7 = 2^31 - 65537
        sub     x6, x2, #16, lsl #12    // x6 = x2 - 65536
        cmp     x6, x7                  // if (x6 < x7)
        bls     .L123                   //   goto .L123
        .p2align 2
.L130:
        sub     x6, x0, #16, lsl #12    // x6 = x0 - 65536
        cmp     x6, x7                  // if (x6 >= x7)
        bhi     .L107                   //   goto .L107
        ldr     w1, [x0, -8]
        tbnz    w1, #31, .L114
        sub     x2, x0, #4
.L119:
        ldxr    w1, [x2]
        tbnz    w1, #RC_BITS_MSB_INDEX, .L107
        sub     w3, w1, #1
        orr     w3, w3, #RC_CYCLE_COLOR_BROWN
        // start check if weak collected bit can set
        // 1. weak collected bit not set
        tbnz    w3, #WEAK_COLLECTED_INDEX, .L128
        // 2. newHeader's weak rc > 1
        and     w4, w3, WEAK_RC_BITS_MASK
        cmp     w4, WEAK_RC_ONE
        ble .L128
        // 3. newHeader's strong and resurrect rc is zero
        and     w4, w3, RESURRECT_STRONG_MASK
        cbnz    w4, .L128
        // 4. set weak collected bit
        orr w3, w3, #WEAK_COLLECT
        // end weak collected bit check
.L128:
        stxr    w4, w3, [x2]
        cbnz    w4, .L119
        // old rcheader in w1 and new rcheader in w3
  // as imm is too long, split release operation
  // check strong rc first if old strong rc is 1, if not return
        and     w4, w1, STRONG_RC_BITS_MASK
        cmp     w4, 1
        bne .L127
  // check if release object
        and w4, w3, RC_COLOR_CLEAR_MASK
        cmp w4, WEAK_RC_ONE
        beq     .L124
        eor w4, w4, #WEAK_COLLECT
        cbz w4, .L124
        // check if weak collected
        tbz     w3, #WEAK_COLLECTED_INDEX, .L127
        tbz     w1, #WEAK_COLLECTED_INDEX, .L126
.L127:
#if ENABLE_ASSERT_RC_NZ
  and   w1, w1, STRONG_RC_BITS_MASK  // check old strong is not zero
        cbz     w1, .L343
#endif
.L107:
        ldp     x11, x30, [sp], 32
        .cfi_remember_state
        .cfi_restore 30
        .cfi_restore 11
        .cfi_def_cfa 31, 0
        ret
.L114:
        .cfi_restore_state
        bl      MRT_DecRef
        ldp     x11, x30, [sp], 32
        .cfi_remember_state
        .cfi_restore 11
        .cfi_restore 30
        .cfi_def_cfa 31, 0
        ret
.L123:
        .cfi_restore_state
        add     x5, sp, 32
        sub     x3, x2, #4
        str     wzr, [x5, -4]!
        .p2align 2
.L112:
        ldar    w1, [x3]
        tbnz    w1, #RC_BITS_MSB_INDEX, .L130
        and     w2, w1, RC_COLOR_CLEAR_MASK
        add     w2, w2, 1
        str     w1, [sp, 28]
        ldaxr   w4, [x3]
        cmp     w4, w1
        bne     .L140
        stlxr   w8, w2, [x3]
        cmp     w8, 0
.L140:
        bne     .L125
        ldr     w1, [sp, 28]
        tst     x1, STRONG_RC_BITS_MASK
        bne     .L130
.L343:
        bl     MRT_BuiltinAbortSaferegister
        .p2align 3
.L125:
        str     w4, [x5]
        b       .L112
.L142:
        .cfi_def_cfa 31, 0
        .cfi_restore 11
        .cfi_restore 30
        ret
.L124:
        .cfi_def_cfa 29, 32
        .cfi_offset 11, -32
        .cfi_offset 30, -24
        bl      MRT_ReleaseObj
        ldp     x11, x30, [sp], 32
        .cfi_restore 11
        .cfi_restore 30
        .cfi_def_cfa 31, 0
        ret
.L126:
        .cfi_def_cfa 29, 32
        .cfi_offset 11, -32
        .cfi_offset 30, -24
        bl      MRT_CollectWeakObj
        ldp     x11, x30, [sp], 32
        .cfi_restore 11
        .cfi_restore 30
        .cfi_def_cfa 31, 0
        ret
        .cfi_endproc
.LFE7170:
#endif // CONFIG_JSAN
        .size   MCC_IncDecRef_NaiveRCFast, .-MCC_IncDecRef_NaiveRCFast

.section .data.maple.gcInfo,"aw",%progbits
    .type   gcIsGCOnly, @object
    .p2align 3
    .local gcIsGCOnly
gcIsGCOnly:
    .quad   0
    .quad   0x4d634734  // maigc
    .size   gcIsGCOnly,.-gcIsGCOnly
// Add global symbol REF_gcIsGCOnly for indirect reference to gcIsGCOnly.
// Add this indirect because of lld relocation limitation
    .type   REF_gcIsGCOnly, @object
    .p2align 3
    .global REF_gcIsGCOnly
REF_gcIsGCOnly:
    .quad   gcIsGCOnly - .
    .size   REF_gcIsGCOnly,.-REF_gcIsGCOnly
#endif // !DISABLE_RC_DUPLICATE
#endif
