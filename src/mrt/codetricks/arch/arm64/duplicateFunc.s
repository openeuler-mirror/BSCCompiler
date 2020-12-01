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
        .p2align 4
        .local strlen
        .type   strlen, %function
strlen:
        and     x4, x0, 4096 - 1
        mov     x8, 0x0101010101010101
        cmp     x4, 4096 - 16
        b.gt    .Lpage_cross
        ldp     x2, x3, [x0]
        sub     x4, x2, x8
        orr     x5, x2, 0x7f7f7f7f7f7f7f7f
        sub     x6, x3, x8
        orr     x7, x3, 0x7f7f7f7f7f7f7f7f
        bics    x4, x4, x5
        bic     x5, x6, x7
        ccmp    x5, 0, 0, eq
        beq     .Lmain_loop_entry

        csel    x4, x4, x5, cc
        mov     x0, 8
        rev     x4, x4
        clz     x4, x4
        csel    x0, xzr, x0, cc
        add     x0, x0, x4, lsr 3
        ret
        .p2align 4
.Lmain_loop_entry:
        bic     x1, x0, 15
        sub     x1, x1, 16
.Lmain_loop:
        ldp     x2, x3, [x1, 32]!
.Lpage_cross_entry:
        sub     x4, x2, x8
        sub     x6, x3, x8
        orr     x5, x4, x6
        tst     x5, x8, lsl 7
        bne     1f
        ldp     x2, x3, [x1, 16]
        sub     x4, x2, x8
        sub     x6, x3, x8
        orr     x5, x4, x6
        tst     x5, x8, lsl 7
        beq     .Lmain_loop
        add     x1, x1, 16
1:
        orr     x5, x2, 0x7f7f7f7f7f7f7f7f
        orr     x7, x3, 0x7f7f7f7f7f7f7f7f
        bics    x4, x4, x5
        bic     x5, x6, x7
        ccmp    x5, 0, 0, eq
        beq     .Lnonascii_loop
.Ltail:
        csel    x4, x4, x5, cc
        sub     x0, x1, x0
        rev     x4, x4
        add     x5, x0, 8
        clz     x4, x4
        csel    x0, x0, x5, cc
        add     x0, x0, x4, lsr 3
        ret
.Lnonascii_loop:
        ldp     x2, x3, [x1, 16]!
        sub     x4, x2, x8
        orr     x5, x2, 0x7f7f7f7f7f7f7f7f
        sub     x6, x3, x8
        orr     x7, x3, 0x7f7f7f7f7f7f7f7f
        bics    x4, x4, x5
        bic     x5, x6, x7
        ccmp    x5, 0, 0, eq
        bne     .Ltail
        ldp     x2, x3, [x1, 16]!
        sub     x4, x2, x8
        orr     x5, x2, 0x7f7f7f7f7f7f7f7f
        sub     x6, x3, x8
        orr     x7, x3, 0x7f7f7f7f7f7f7f7f
        bics    x4, x4, x5
        bic     x5, x6, x7
        ccmp    x5, 0, 0, eq
        beq     .Lnonascii_loop
        b       .Ltail
.Lpage_cross:
        bic     x1, x0, 15
        ldp     x2, x3, [x1]
        lsl     x4, x0, 3
        mov     x7, -1
        lsl     x4, x7, x4
        orr     x4, x4, 0x8080808080808080
        orn     x2, x2, x4
        orn     x5, x3, x4
        tst     x0, 8
        csel    x2, x2, x7, eq
        csel    x3, x3, x5, eq
        b       .Lpage_cross_entry
        .size   strlen, .-strlen

        .text
        .align  2
        .p2align 6
        .local strncmp
        .type   strncmp, %function
strncmp:
        cbz     x2, .Lret0_strncmp
        eor     x8, x0, x1
        mov     x11, #0x0101010101010101
        tst     x8, #7
        b.ne    .Lmisaligned8_strncmp
        ands    x8, x0, #7
        b.ne    .Lmutual_align_strncmp
        sub     x13, x2, #1
        lsr     x13, x13, #3
.Lloop_aligned_strncmp:
        ldr     x3, [x0], #8
        ldr     x4, [x1], #8
.Lstart_realigned_strncmp:
        subs    x13, x13, #1
        sub     x8, x3, x11
        orr     x9, x3, #0x7f7f7f7f7f7f7f7f
        eor     x6, x3, x4
        csinv   x15, x6, xzr, pl
        bics    x5, x8, x9
        ccmp    x15, #0, #0, eq
        b.eq    .Lloop_aligned_strncmp
        tbz     x13, #63, .Lnot_x2_strncmp
        ands    x2, x2, #7
        b.eq    .Lnot_x2_strncmp
        lsl     x2, x2, #3
        mov     x14, #~0
        lsl     x14, x14, x2
        bic     x3, x3, x14
        bic     x4, x4, x14
        orr     x5, x5, x14
.Lnot_x2_strncmp:
        orr     x7, x6, x5
//#ifndef __AARCH64EB__
        rev     x7, x7
        rev     x3, x3
        clz     x12, x7
        rev     x4, x4
        lsl     x3, x3, x12
        lsl     x4, x4, x12
        lsr     x3, x3, #56
        sub     x0, x3, x4, lsr #56
        ret
.Lmutual_align_strncmp:
        bic     x0, x0, #7
        bic     x1, x1, #7
        ldr     x3, [x0], #8
        neg     x10, x8, lsl #3
        ldr     x4, [x1], #8
        mov     x9, #~0
        sub     x13, x2, #1
        lsr     x9, x9, x10
        and     x10, x13, #7
        lsr     x13, x13, #3
        add     x2, x2, x8
        add     x10, x10, x8
        orr     x3, x3, x9
        orr     x4, x4, x9
        add     x13, x13, x10, lsr #3
        b       .Lstart_realigned_strncmp
.Lret0_strncmp:
        mov     x0, #0
        ret
        .p2align 6
.Lmisaligned8_strncmp:
        sub     x2, x2, #1
1:
        ldrb    w3, [x0], #1
        ldrb    w4, [x1], #1
        subs    x2, x2, #1
        ccmp    w3, #1, #0, cs
        ccmp    w3, w4, #0, cs
        b.eq    1b
        sub     x0, x3, x4
        ret
        .size   strncmp, .-strncmp

        .text
        .align  2
        .p2align 4
        .local memcpy
        .type   memcpy, %function
memcpy:
        prfm    PLDL1KEEP, [x1]
        add     x4, x1, x2
        add     x5, x0, x2
        cmp     x2, 16
        b.ls    .Lcopy16
        cmp     x2, 96
        b.hi    .Lcopy_long
        sub     x9, x2, 1
        ldp     x6, x7, [x1]
        tbnz    x9, 6, .Lcopy96
        ldp     x12, x13, [x4, -16]
        tbz     x9, 5, 1f
        ldp     x8, x9, [x1, 16]
        ldp     x10, x11, [x4, -32]
        stp     x8, x9, [x0, 16]
        stp     x10, x11, [x5, -32]
1:
        stp     x6, x7, [x0]
        stp     x12, x13, [x5, -16]
        ret
        .p2align 4
.Lcopy16:
        cmp     x2, 8
        b.lo    1f
        ldr     x6, [x1]
        ldr     x7, [x4, -8]
        str     x6, [x0]
        str     x7, [x5, -8]
        ret
        .p2align 4
1:
        tbz     x2, 2, 1f
        ldr     w6, [x1]
        ldr     w7, [x4, -4]
        str     w6, [x0]
        str     w7, [x5, -4]
        ret
1:
        cbz     x2, 2f
        lsr     x9, x2, 1
        ldrb    w6, [x1]
        ldrb    w7, [x4, -1]
        ldrb    w8, [x1, x9]
        strb    w6, [x0]
        strb    w8, [x0, x9]
        strb    w7, [x5, -1]
2:      ret
        .p2align 4
.Lcopy96:
        ldp     x8, x9, [x1, 16]
        ldp     x10, x11, [x1, 32]
        ldp     x12, x13, [x1, 48]
        ldp     x1, x2, [x4, -32]
        ldp     x4, x3, [x4, -16]
        stp     x6, x7, [x0]
        stp     x8, x9, [x0, 16]
        stp     x10, x11, [x0, 32]
        stp     x12, x13, [x0, 48]
        stp     x1, x2, [x5, -32]
        stp     x4, x3, [x5, -16]
        ret
        .p2align 4
.Lcopy_long:
        and     x9, x0, 15
        bic     x3, x0, 15
        ldp     x12, x13, [x1]
        sub     x1, x1, x9
        add     x2, x2, x9
        ldp     x6, x7, [x1, 16]
        stp     x12, x13, [x0]
        ldp     x8, x9, [x1, 32]
        ldp     x10, x11, [x1, 48]
        ldp     x12, x13, [x1, 64]!
        subs    x2, x2, 128 + 16
        b.ls    2f
1:
        stp     x6, x7, [x3, 16]
        ldp     x6, x7, [x1, 16]
        stp     x8, x9, [x3, 32]
        ldp     x8, x9, [x1, 32]
        stp     x10, x11, [x3, 48]
        ldp     x10, x11, [x1, 48]
        stp     x12, x13, [x3, 64]!
        ldp     x12, x13, [x1, 64]!
        subs    x2, x2, 64
        b.hi    1b
2:
        ldp     x1, x2, [x4, -64]
        stp     x6, x7, [x3, 16]
        ldp     x6, x7, [x4, -48]
        stp     x8, x9, [x3, 32]
        ldp     x8, x9, [x4, -32]
        stp     x10, x11, [x3, 48]
        ldp     x10, x11, [x4, -16]
        stp     x12, x13, [x3, 64]
        stp     x1, x2, [x5, -64]
        stp     x6, x7, [x5, -48]
        stp     x8, x9, [x5, -32]
        stp     x10, x11, [x5, -16]
        ret
        .size   memcpy, .-memcpy

        .text
        .align  2
        .p2align 6
        .local memmove
        .type   memmove, %function
memmove:
        cmp     x0, x1
        b.lo    .Ldownwards
        add     x3, x1, x2
        cmp     x0, x3
        b.hs    memcpy
        add     x6, x0, x2
        add     x1, x1, x2
        cmp     x2, #64
        b.ge    .Lmov_not_short_up
.Ltail63up:
        ands    x3, x2, #0x30
        b.eq    .Ltail15up
        sub     x6, x6, x3
        sub     x1, x1, x3
        cmp     w3, #0x20
        b.eq    1f
        b.lt    2f
        ldp     x7, x8, [x1, #32]
        stp     x7, x8, [x6, #32]
1:
        ldp     x7, x8, [x1, #16]
        stp     x7, x8, [x6, #16]
2:
        ldp     x7, x8, [x1]
        stp     x7, x8, [x6]
.Ltail15up:
        tbz     x2, #3, 1f
        ldr     x3, [x1, #-8]!
        str     x3, [x6, #-8]!
1:
        tbz     x2, #2, 1f
        ldr     w3, [x1, #-4]!
        str     w3, [x6, #-4]!
1:
        tbz     x2, #1, 1f
        ldrh    w3, [x1, #-2]!
        strh    w3, [x6, #-2]!
1:
        tbz     x2, #0, 1f
        ldrb    w3, [x1, #-1]
        strb    w3, [x6, #-1]
1:
        ret
.Lmov_not_short_up:
        ands    x4, x1, #15
        b.eq    2f
        sub     x2, x2, x4
        tbz     x4, #3, 1f
        ldr     x3, [x1, #-8]!
        str     x3, [x6, #-8]!
1:
        tbz     x4, #2, 1f
        ldr     w3, [x1, #-4]!
        str     w3, [x6, #-4]!
1:
        tbz     x4, #1, 1f
        ldrh    w3, [x1, #-2]!
        strh    w3, [x6, #-2]!
1:
        tbz     x4, #0, 1f
        ldrb    w3, [x1, #-1]!
        strb    w3, [x6, #-1]!
1:
        cmp     x2, #63
        b.le    .Ltail63up
2:
        subs    x2, x2, #128
        b.ge    .Lmov_body_large_up
        ldp     x7, x8, [x1, #-64]!
        ldp     x9, x10, [x1, #16]
        ldp     x11, x12, [x1, #32]
        ldp     x13, x14, [x1, #48]
        stp     x7, x8, [x6, #-64]!
        stp     x9, x10, [x6, #16]
        stp     x11, x12, [x6, #32]
        stp     x13, x14, [x6, #48]
        tst     x2, #0x3f
        b.ne    .Ltail63up
        ret
        .p2align 6
.Lmov_body_large_up:
        ldp     x7, x8, [x1, #-16]
        ldp     x9, x10, [x1, #-32]
        ldp     x11, x12, [x1, #-48]
        ldp     x13, x14, [x1, #-64]!
1:
        stp     x7, x8, [x6, #-16]
        ldp     x7, x8, [x1, #-16]
        stp     x9, x10, [x6, #-32]
        ldp     x9, x10, [x1, #-32]
        stp     x11, x12, [x6, #-48]
        ldp     x11, x12, [x1, #-48]
        stp     x13, x14, [x6, #-64]!
        ldp     x13, x14, [x1, #-64]!
        subs    x2, x2, #64
        b.ge    1b
        stp     x7, x8, [x6, #-16]
        stp     x9, x10, [x6, #-32]
        stp     x11, x12, [x6, #-48]
        stp     x13, x14, [x6, #-64]!
        tst     x2, #0x3f
        b.ne    .Ltail63up
        ret
.Ldownwards:
        sub     x3, x1, #16
        cmp     x0, x3
        b.ls    memcpy
        mov     x6, x0
        cmp     x2, #64
        b.ge    .Lmov_not_short_down
.Ltail63down:
        ands    x3, x2, #0x30
        b.eq    .Ltail15down
        add     x6, x6, x3
        add     x1, x1, x3
        cmp     w3, #0x20
        b.eq    1f
        b.lt    2f
        ldp     x7, x8, [x1, #-48]
        stp     x7, x8, [x6, #-48]
1:
        ldp     x7, x8, [x1, #-32]
        stp     x7, x8, [x6, #-32]
2:
        ldp     x7, x8, [x1, #-16]
        stp     x7, x8, [x6, #-16]
.Ltail15down:
        tbz     x2, #3, 1f
        ldr     x3, [x1], #8
        str     x3, [x6], #8
1:
        tbz     x2, #2, 1f
        ldr     w3, [x1], #4
        str     w3, [x6], #4
1:
        tbz     x2, #1, 1f
        ldrh    w3, [x1], #2
        strh    w3, [x6], #2
1:
        tbz     x2, #0, 1f
        ldrb    w3, [x1]
        strb    w3, [x6]
1:
        ret
.Lmov_not_short_down:
        neg     x4, x1
        ands    x4, x4, #15
        b.eq    2f
        sub     x2, x2, x4
        tbz     x4, #3, 1f
        ldr     x3, [x1], #8
        str     x3, [x6], #8
1:
        tbz     x4, #2, 1f
        ldr     w3, [x1], #4
        str     w3, [x6], #4
1:
        tbz     x4, #1, 1f
        ldrh    w3, [x1], #2
        strh    w3, [x6], #2
1:
        tbz     x4, #0, 1f
        ldrb    w3, [x1], #1
        strb    w3, [x6], #1
1:
        cmp     x2, #63
        b.le    .Ltail63down
2:
        subs    x2, x2, #128
        b.ge    .Lmov_body_large_down
        ldp     x7, x8, [x1]
        ldp     x9, x10, [x1, #16]
        ldp     x11, x12, [x1, #32]
        ldp     x13, x14, [x1, #48]
        stp     x7, x8, [x6]
        stp     x9, x10, [x6, #16]
        stp     x11, x12, [x6, #32]
        stp     x13, x14, [x6, #48]
        tst     x2, #0x3f
        add     x1, x1, #64
        add     x6, x6, #64
        b.ne    .Ltail63down
        ret
        .p2align 6
.Lmov_body_large_down:
        ldp     x7, x8, [x1, #0]
        sub     x6, x6, #16
        ldp     x9, x10, [x1, #16]
        ldp     x11, x12, [x1, #32]
        ldp     x13, x14, [x1, #48]!
1:
        stp     x7, x8, [x6, #16]
        ldp     x7, x8, [x1, #16]
        stp     x9, x10, [x6, #32]
        ldp     x9, x10, [x1, #32]
        stp     x11, x12, [x6, #48]
        ldp     x11, x12, [x1, #48]
        stp     x13, x14, [x6, #64]!
        ldp     x13, x14, [x1, #64]!
        subs    x2, x2, #64
        b.ge    1b
        stp     x7, x8, [x6, #16]
        stp     x9, x10, [x6, #32]
        stp     x11, x12, [x6, #48]
        stp     x13, x14, [x6, #64]
        add     x1, x1, #16
        add     x6, x6, #64 + 16
        tst     x2, #0x3f
        b.ne    .Ltail63down
        ret
        .size   memmove, .-memmove

        .text
        .align  2
        .p2align  6
        .local strcmp
        .type   strcmp, %function
strcmp:
        eor     x7, x0, x1
        mov     x10, #0x0101010101010101
        tst     x7, #7
        b.ne    .Lmisaligned8_strcmp
        ands    x7, x0, #7
        b.ne    .Lmutual_align_strcmp
.Lloop_aligned_strcmp:
        ldr     x2, [x0], #8
        ldr     x3, [x1], #8
.Lstart_realigned_strcmp:
        sub     x7, x2, x10
        orr     x8, x2, #0x7f7f7f7f7f7f7f7f
        eor     x5, x2, x3
        bic     x4, x7, x8
        orr     x6, x5, x4
        cbz     x6, .Lloop_aligned_strcmp
//#ifndef __AARCH64EB__
        rev     x6, x6
        rev     x2, x2
        # WARNING
        # Change register usage from x11 to x9
        # Preserve x11 for issue #547
        clz     x9, x6
        rev     x3, x3
        lsl     x2, x2, x9
        lsl     x3, x3, x9
        lsr     x2, x2, #56
        sub     x0, x2, x3, lsr #56
        ret
.Lmutual_align_strcmp:
        bic     x0, x0, #7
        bic     x1, x1, #7
        lsl     x7, x7, #3
        ldr     x2, [x0], #8
        neg     x7, x7
        ldr     x3, [x1], #8
        mov     x8, #~0
        lsr     x8, x8, x7
        orr     x2, x2, x8
        orr     x3, x3, x8
        b       .Lstart_realigned_strcmp
.Lmisaligned8_strcmp:
        ldrb    w2, [x0], #1
        ldrb    w3, [x1], #1
        cmp     w2, #1
        ccmp    w2, w3, #0, cs
        b.eq    .Lmisaligned8_strcmp
        sub     x0, x2, x3
        ret
        .size   strcmp, .-strcmp

        .text
        .align  2
        .p2align 6
        .local memcmp
        .type   memcmp, %function
memcmp:
        cbz     x2, .Lret0
        eor     x8, x0, x1
        tst     x8, #7
        b.ne    .Lmisaligned8
        ands    x8, x0, #7
        b.ne    .Lmutual_align
        add     x12, x2, #7
        lsr     x12, x12, #3
.Lloop_aligned:
        ldr     x3, [x0], #8
        ldr     x4, [x1], #8
.Lstart_realigned:
        subs    x12, x12, #1
        eor     x6, x3, x4
        csinv   x7, x6, xzr, ne
        cbz     x7, .Lloop_aligned
        cbnz    x12, .Lnot_limit
        ands    x2, x2, #7
        b.eq    .Lnot_limit
        lsl     x2, x2, #3
        mov     x13, #~0
        lsl     x13, x13, x2
        bic     x3, x3, x13
        bic     x4, x4, x13
        orr     x6, x6, x13
.Lnot_limit:
        rev     x6, x6
        rev     x3, x3
        rev     x4, x4
        clz     x11, x6
        lsl     x3, x3, x11
        lsl     x4, x4, x11
        lsr     x3, x3, #56
        sub     x0, x3, x4, lsr #56
        ret
.Lmutual_align:
        bic     x0, x0, #7
        bic     x1, x1, #7
        add     x2, x2, x8      /* Adjust the limit for the extra.  */
        lsl     x8, x8, #3          /* Bytes beyond alignment -> bits.  */
        ldr     x3, [x0], #8
        neg     x8, x8              /* Bits to alignment -64.  */
        ldr     x4, [x1], #8
        mov     x9, #~0
        lsr     x9, x9, x8
        add     x12, x2, #7
        orr     x3, x3, x9
        orr     x4, x4, x9
        lsr     x12, x12, #3
        b       .Lstart_realigned
.Lret0:
        mov     x0, #0
        ret
        .p2align 6
.Lmisaligned8:
        sub     x2, x2, #1
1:
        ldrb    w3, [x0], #1
        ldrb    w4, [x1], #1
        subs    x2, x2, #1
        ccmp    w3, w4, #0, cs
        b.eq    1b
        sub     x0, x3, x4
        ret
        .size   memcmp, .-memcmp

        .text
        .align  2
        .p2align 6
        .local memcmpMpl
        .type   memcmpMpl, %function
        # only for jstring comparison.
        # return 1 for equal, return 0 for not-equal
memcmpMpl:
        cbz     x2, .Lret1Mpl
.Lmutual_alignMpl:
#ifdef USE_32BIT_REF
        add     x12, x2, #7
        ldr     x3, [x0], #8  // string content is always 8B aligned
        ldr     x4, [x1], #8  // string content is always 8B aligned
#else
        bic     x0, x0, #7
        bic     x1, x1, #7
        mov     x8, #4         // Note, the string content offset is hard coded here: bad
        add     x2, x2, x8      /* Adjust the limit for the extra.  */
        lsl     x8, x8, #3          /* Bytes beyond alignment -> bits.  */
        ldr     x3, [x0], #8
        neg     x8, x8              /* Bits to alignment -64.  */
        ldr     x4, [x1], #8
        mov     x9, #~0
        lsr     x9, x9, x8
        add     x12, x2, #7
        orr     x3, x3, x9
        orr     x4, x4, x9
#endif //USE_32BIT_REF
        lsr     x12, x12, #3
        b       .Lstart_realignedMpl
.Lloop_alignedMpl:
        ldr     x3, [x0], #8
        ldr     x4, [x1], #8
.Lstart_realignedMpl:
        subs    x12, x12, #1
        eor     x6, x3, x4
        csinv   x7, x6, xzr, ne
        cbz     x7, .Lloop_alignedMpl
        cbnz    x12, .Lret0Mpl
        ands    x2, x2, #7
        b.eq    .Lnot_limitMpl
        lsl     x2, x2, #3
        mov     x13, #~0
        lsl     x13, x13, x2
        bic     x3, x3, x13
        bic     x4, x4, x13
        orr     x6, x6, x13
.Lnot_limitMpl:
        rev     x6, x6
        rev     x3, x3
        rev     x4, x4
        clz     x11, x6
        lsl     x3, x3, x11
        lsl     x4, x4, x11
        lsr     x3, x3, #56
        sub     x0, x3, x4, lsr #56
        cbnz    x0, .Lret0Mpl
.Lret1Mpl:
        mov     x0, #1
        ret
        .p2align 6
.Lret0Mpl:
        mov     x0, #0
        ret
        .p2align 6
        .size   memcmpMpl, .-memcmpMpl

#Libframework_start
        .section        .text.signalExceptionForError,"ax",@progbits
        .globl  signalExceptionForError // -- Begin function signalExceptionForError
        .p2align        2
        .type   signalExceptionForError,@function
signalExceptionForError:                // @signalExceptionForError
        .cfi_startproc
// %bb.0:
        sub     sp, sp, #80             // =80
        str     x23, [sp, #16]          // 8-byte Folded Spill
        stp     x22, x21, [sp, #32]     // 16-byte Folded Spill
        stp     x20, x19, [sp, #48]     // 16-byte Folded Spill
        stp     x29, x30, [sp, #64]     // 16-byte Folded Spill
        add     x29, sp, #64            // =64
        .cfi_def_cfa w29, 16
        .cfi_offset w30, -8
        .cfi_offset w29, -16
        .cfi_offset w19, -24
        .cfi_offset w20, -32
        .cfi_offset w21, -40
        .cfi_offset w22, -48
        .cfi_offset w23, -64
        mrs     x23, TPIDR_EL0
        ldr     x8, [x23, #40]
        mov     w20, w3
        mov     w21, w2
        mov     x19, x0
        cmn     w2, #23                 // =23
        str     x8, [sp, #8]
        b.gt    .LBB0_4
// %bb.1:
        orr     w8, wzr, #0x80000000
        cmp     w21, w8
        b.eq    .LBB0_8_signalExceptionForError
// %bb.2:
        cmn     w21, #38                // =38
        b.ne    .LBB0_7_signalExceptionForError
// %bb.3:
        adrp    x1, .L.str.3
        add     x1, x1, :lo12:.L.str.3
        b       .LBB0_10
.LBB0_4:
        cmn     w21, #22                // =22
        b.eq    .LBB0_9
// %bb.5:
        cmn     w21, #12                // =12
        b.ne    .LBB0_7_signalExceptionForError
// %bb.6:
        adrp    x1, .L.str.2
        add     x1, x1, :lo12:.L.str.2
        b       .LBB0_10
.LBB0_7_signalExceptionForError:
        adrp    x22, .L.str.5
        add     x22, x22, :lo12:.L.str.5
        orr     w0, wzr, #0x6
        mov     x1, xzr
        mov     x2, x22
        mov     w3, w21
        bl      __android_log_print
        mov     x0, sp
        bl      _ZN7android7String8C1Ev
        mov     x0, sp
        mov     x1, x22
        mov     w2, w21
        bl      _ZN7android7String812appendFormatEPKcz
        ldr     x2, [sp]
        adrp    x8, .L.str
        adrp    x9, .L.str.6
        add     x8, x8, :lo12:.L.str
        add     x9, x9, :lo12:.L.str.6
        tst     w20, #0x1
        csel    x1, x9, x8, ne
        mov     x0, x19
        bl      jniThrowException
        mov     x0, sp
        bl      _ZN7android7String8D1Ev
        ldr     x8, [x23, #40]
        ldr     x9, [sp, #8]
        cmp     x8, x9
        b.eq    .LBB0_12_signalExceptionForError
        b       .LBB0_13_signalExceptionForError
.LBB0_8_signalExceptionForError:
        adrp    x1, .L.str
        adrp    x2, .L.str.1
        add     x1, x1, :lo12:.L.str
        add     x2, x2, :lo12:.L.str.1
        mov     x0, x19
        b       .LBB0_11
.LBB0_9:
        adrp    x1, .L.str.4
        add     x1, x1, :lo12:.L.str.4
.LBB0_10:
        mov     x0, x19
        mov     x2, xzr
.LBB0_11:
        bl      jniThrowException
        ldr     x8, [x23, #40]
        ldr     x9, [sp, #8]
        cmp     x8, x9
        b.ne    .LBB0_13_signalExceptionForError
.LBB0_12_signalExceptionForError:
        ldp     x29, x30, [sp, #64]     // 16-byte Folded Reload
        ldp     x20, x19, [sp, #48]     // 16-byte Folded Reload
        ldp     x22, x21, [sp, #32]     // 16-byte Folded Reload
        ldr     x23, [sp, #16]          // 8-byte Folded Reload
        add     sp, sp, #80             // =80
        ret
.LBB0_13_signalExceptionForError:
        bl      __stack_chk_fail
.Lfunc_end0_signalExceptionForError:
        .size   signalExceptionForError, .Lfunc_end0_signalExceptionForError-signalExceptionForError
        .cfi_endproc
                                        // -- End function
        .section        .text.Landroid_2Fos_2FParcel_3B_7CnativeWriteString_7C_28JLjava_2Flang_2FString_3B_29V_native,"ax",@progbits
        .globl  Landroid_2Fos_2FParcel_3B_7CnativeWriteString_7C_28JLjava_2Flang_2FString_3B_29V_native // -- Begin function Landroid_2Fos_2FParcel_3B_7CnativeWriteString_7C_28JLjava_2Flang_2FString_3B_29V_native
        .p2align        2
        .type   Landroid_2Fos_2FParcel_3B_7CnativeWriteString_7C_28JLjava_2Flang_2FString_3B_29V_native,@function
Landroid_2Fos_2FParcel_3B_7CnativeWriteString_7C_28JLjava_2Flang_2FString_3B_29V_native: // @Landroid_2Fos_2FParcel_3B_7CnativeWriteString_7C_28JLjava_2Flang_2FString_3B_29V_native
        .cfi_startproc
// %bb.0:
        str     x28, [sp, #-80]!        // 8-byte Folded Spill
        stp     x24, x23, [sp, #16]     // 16-byte Folded Spill
        stp     x22, x21, [sp, #32]     // 16-byte Folded Spill
        stp     x20, x19, [sp, #48]     // 16-byte Folded Spill
        stp     x29, x30, [sp, #64]     // 16-byte Folded Spill
        add     x29, sp, #64            // =64
        sub     sp, sp, #528            // =528
        .cfi_def_cfa w29, 16
        .cfi_offset w30, -8
        .cfi_offset w29, -16
        .cfi_offset w19, -24
        .cfi_offset w20, -32
        .cfi_offset w21, -40
        .cfi_offset w22, -48
        .cfi_offset w23, -56
        .cfi_offset w24, -64
        .cfi_offset w28, -80
        mrs     x24, TPIDR_EL0
        ldr     x8, [x24, #40]
        stur    x8, [x29, #-72]
        cbz     x0, .LBB1_25
// %bb.1:
        mov     x21, x1
        mov     x19, x0
        cbz     x1, .LBB1_5
// %bb.2:
        ldr     w9, [x21, #8]
        tbnz    w9, #31, .LBB1_28
// %bb.3:
        asr     w8, w9, #1
        add     x20, x21, #16           // =16
        tbnz    w9, #0, .LBB1_6
// %bb.4:
        sxtw    x2, w8
        mov     x0, x19
        mov     x1, x20
        bl      _ZN7android6Parcel13writeString16EPKDsm
        mov     w19, w0
        cbnz    w19, .LBB1_24
        b       .LBB1_25
.LBB1_5:
        mov     x0, x19
        mov     x1, xzr
        mov     x2, xzr
        bl      _ZN7android6Parcel13writeString16EPKDsm
        mov     w19, w0
        cbnz    w19, .LBB1_24
        b       .LBB1_25
.LBB1_6:
        cmp     w9, #513                // =513
        b.gt    .LBB1_15
// %bb.7:
        cmp     w9, #2                  // =2
        sxtw    x2, w8
        b.lt    .LBB1_14
// %bb.8:
        add     x9, sp, #8              // =8
        add     x10, x9, x2, lsl #1
        orr     x11, x9, #0x2
        cmp     x10, x11
        csel    x10, x10, x11, hi
        mvn     x11, x9
        add     x10, x10, x11
        lsr     x10, x10, #1
        add     x11, x10, #1            // =1
        cmp     x11, #16                // =16
        add     x10, sp, #8             // =8
        b.lo    .LBB1_12
// %bb.9:
        and     x12, x11, #0xfffffffffffffff0
        add     x13, sp, #8             // =8
        add     x20, x20, x12
        add     x10, x13, x12, lsl #1
        add     x13, x13, #16           // =16
        add     x14, x21, #24           // =24
        mov     x15, x12
.LBB1_10:                               // =>This Inner Loop Header: Depth=1
        ldp     d0, d1, [x14, #-8]
        subs    x15, x15, #16           // =16
        add     x14, x14, #16           // =16
        ushll   v0.8h, v0.8b, #0
        ushll   v1.8h, v1.8b, #0
        stp     q0, q1, [x13, #-16]
        add     x13, x13, #32           // =32
        b.ne    .LBB1_10
// %bb.11:
        cmp     x11, x12
        b.eq    .LBB1_14
.LBB1_12:
        add     x8, x9, w8, sxtw #1
.LBB1_13:                               // =>This Inner Loop Header: Depth=1
        ldrb    w9, [x20], #1
        strh    w9, [x10], #2
        cmp     x10, x8
        b.lo    .LBB1_13
.LBB1_14:
        add     x1, sp, #8              // =8
        mov     x0, x19
        bl      _ZN7android6Parcel13writeString16EPKDsm
        mov     w19, w0
        cbnz    w19, .LBB1_24
        b       .LBB1_25
.LBB1_15:
        sxtw    x22, w8
        orr     w1, wzr, #0x2
        mov     x0, x22
        bl      calloc
        cbz     x0, .LBB1_23
// %bb.16:
        add     x8, x0, x22, lsl #1
        mov     x23, x0
        cmp     x8, x0
        b.ls    .LBB1_22
// %bb.17:
        lsl     x9, x22, #1
        sub     x9, x9, #1              // =1
        lsr     x9, x9, #1
        add     x10, x9, #1             // =1
        cmp     x10, #16                // =16
        mov     x9, x23
        b.lo    .LBB1_21
// %bb.18:
        and     x11, x10, #0xfffffffffffffff0
        add     x12, x23, #16           // =16
        add     x20, x20, x11
        add     x9, x23, x11, lsl #1
        add     x13, x21, #24           // =24
        mov     x14, x11
.LBB1_19:                               // =>This Inner Loop Header: Depth=1
        ldp     d0, d1, [x13, #-8]
        subs    x14, x14, #16           // =16
        add     x13, x13, #16           // =16
        ushll   v0.8h, v0.8b, #0
        ushll   v1.8h, v1.8b, #0
        stp     q0, q1, [x12, #-16]
        add     x12, x12, #32           // =32
        b.ne    .LBB1_19
// %bb.20:
        cmp     x10, x11
        b.eq    .LBB1_22
.LBB1_21:                               // =>This Inner Loop Header: Depth=1
        ldrb    w10, [x20], #1
        strh    w10, [x9], #2
        cmp     x9, x8
        b.lo    .LBB1_21
.LBB1_22:
        mov     x0, x19
        mov     x1, x23
        mov     x2, x22
        bl      _ZN7android6Parcel13writeString16EPKDsm
        mov     w19, w0
        mov     x0, x23
        bl      free
        cbnz    w19, .LBB1_24
        b       .LBB1_25
.LBB1_23:
        mov     w19, #-12
.LBB1_24:
        //APP
        mrs     x8, TPIDR_EL0
        //NO_APP
        ldr     x8, [x8, #56]
        ldr     x0, [x8, #56]
        ldr     x8, [x0]
        ldr     x8, [x8]
        blr     x8
        mov     w2, w19
        mov     w3, wzr
        bl      signalExceptionForError
        bl      MRT_CheckThrowPendingExceptionUnw
.LBB1_25:
        ldr     x8, [x24, #40]
        ldur    x9, [x29, #-72]
        cmp     x8, x9
        b.ne    .LBB1_27
// %bb.26:
        add     sp, sp, #528            // =528
        ldp     x29, x30, [sp, #64]     // 16-byte Folded Reload
        ldp     x20, x19, [sp, #48]     // 16-byte Folded Reload
        ldp     x22, x21, [sp, #32]     // 16-byte Folded Reload
        ldp     x24, x23, [sp, #16]     // 16-byte Folded Reload
        ldr     x28, [sp], #80          // 8-byte Folded Reload
        ret
.LBB1_27:
        bl      __stack_chk_fail
.LBB1_28:
        bl      __ubsan_handle_implicit_conversion_minimal_abort
.Lfunc_end1:
        .size   Landroid_2Fos_2FParcel_3B_7CnativeWriteString_7C_28JLjava_2Flang_2FString_3B_29V_native, .Lfunc_end1-Landroid_2Fos_2FParcel_3B_7CnativeWriteString_7C_28JLjava_2Flang_2FString_3B_29V_native
        .cfi_endproc
                                        // -- End function
        .section        .text.Landroid_2Fos_2FParcel_3B_7CnativeWriteInt_7C_28JI_29V_native,"ax",@progbits
        .globl  Landroid_2Fos_2FParcel_3B_7CnativeWriteInt_7C_28JI_29V_native // -- Begin function Landroid_2Fos_2FParcel_3B_7CnativeWriteInt_7C_28JI_29V_native
        .p2align        2
        .type   Landroid_2Fos_2FParcel_3B_7CnativeWriteInt_7C_28JI_29V_native,@function
Landroid_2Fos_2FParcel_3B_7CnativeWriteInt_7C_28JI_29V_native: // @Landroid_2Fos_2FParcel_3B_7CnativeWriteInt_7C_28JI_29V_native
        .cfi_startproc
// %bb.0:
        str     x19, [sp, #-32]!        // 8-byte Folded Spill
        stp     x29, x30, [sp, #16]     // 16-byte Folded Spill
        add     x29, sp, #16            // =16
        .cfi_def_cfa w29, 16
        .cfi_offset w30, -8
        .cfi_offset w29, -16
        .cfi_offset w19, -32
        cbz     x0, .LBB2_3
        bl      _ZN7android6Parcel10writeInt32Ei
        cbz     w0, .LBB2_3
        mrs     x8, TPIDR_EL0
        ldr     x8, [x8, #56]
        mov     w19, w0
        ldr     x0, [x8, #56]
        ldr     x8, [x0]
        ldr     x8, [x8]
        blr     x8
        mov     w2, w19
        mov     w3, wzr
        bl      signalExceptionForError
        ldp     x29, x30, [sp, #16]     // 16-byte Folded Reload
        ldr     x19, [sp], #32          // 8-byte Folded Reload
        b       MRT_CheckThrowPendingExceptionUnw
.LBB2_3:
        ldp     x29, x30, [sp, #16]     // 16-byte Folded Reload
        ldr     x19, [sp], #32          // 8-byte Folded Reload
        ret
.Lfunc_end2_WriteInt:
        .size   Landroid_2Fos_2FParcel_3B_7CnativeWriteInt_7C_28JI_29V_native, .Lfunc_end2_WriteInt-Landroid_2Fos_2FParcel_3B_7CnativeWriteInt_7C_28JI_29V_native
        .cfi_endproc
                                        // -- End function
        .section        .text.Landroid_2Fos_2FParcel_3B_7CnativeWriteFloat_7C_28JF_29V_native,"ax",@progbits
        .globl  Landroid_2Fos_2FParcel_3B_7CnativeWriteFloat_7C_28JF_29V_native // -- Begin function Landroid_2Fos_2FParcel_3B_7CnativeWriteFloat_7C_28JF_29V_native
        .p2align        2
        .type   Landroid_2Fos_2FParcel_3B_7CnativeWriteFloat_7C_28JF_29V_native,@function
Landroid_2Fos_2FParcel_3B_7CnativeWriteFloat_7C_28JF_29V_native: // @Landroid_2Fos_2FParcel_3B_7CnativeWriteFloat_7C_28JF_29V_native
        .cfi_startproc
        str     x19, [sp, #-32]!        // 8-byte Folded Spill
        stp     x29, x30, [sp, #16]     // 16-byte Folded Spill
        add     x29, sp, #16            // =16
        .cfi_def_cfa w29, 16
        .cfi_offset w30, -8
        .cfi_offset w29, -16
        .cfi_offset w19, -32
        cbz     x0, .LBB5_3
        bl      _ZN7android6Parcel10writeFloatEf
        cbz     w0, .LBB5_3
        mrs     x8, TPIDR_EL0
        ldr     x8, [x8, #56]
        mov     w19, w0
        ldr     x0, [x8, #56]
        ldr     x8, [x0]
        ldr     x8, [x8]
        blr     x8
        mov     w2, w19
        mov     w3, wzr
        bl      signalExceptionForError
        ldp     x29, x30, [sp, #16]     // 16-byte Folded Reload
        ldr     x19, [sp], #32          // 8-byte Folded Reload
        b       MRT_CheckThrowPendingExceptionUnw
.LBB5_3:
        ldp     x29, x30, [sp, #16]     // 16-byte Folded Reload
        ldr     x19, [sp], #32          // 8-byte Folded Reload
        ret
.Lfunc_end9:
        .size   Landroid_2Fos_2FParcel_3B_7CnativeWriteFloat_7C_28JF_29V_native, .Lfunc_end9-Landroid_2Fos_2FParcel_3B_7CnativeWriteFloat_7C_28JF_29V_native
        .cfi_endproc
                                        // -- End function
        .section        .text.Landroid_2Fos_2FParcel_3B_7CnativeWriteLong_7C_28JJ_29V_native,"ax",@progbits
        .globl  Landroid_2Fos_2FParcel_3B_7CnativeWriteLong_7C_28JJ_29V_native // -- Begin function Landroid_2Fos_2FParcel_3B_7CnativeWriteLong_7C_28JJ_29V_native
        .p2align        2
        .type   Landroid_2Fos_2FParcel_3B_7CnativeWriteLong_7C_28JJ_29V_native,@function
Landroid_2Fos_2FParcel_3B_7CnativeWriteLong_7C_28JJ_29V_native: // @Landroid_2Fos_2FParcel_3B_7CnativeWriteLong_7C_28JJ_29V_native
        .cfi_startproc
// %bb.0:
        str     x19, [sp, #-32]!        // 8-byte Folded Spill
        stp     x29, x30, [sp, #16]     // 16-byte Folded Spill
        add     x29, sp, #16            // =16
        .cfi_def_cfa w29, 16
        .cfi_offset w30, -8
        .cfi_offset w29, -16
        .cfi_offset w19, -32
        cbz     x0, .LBB6_3
// %bb.1:
        bl      _ZN7android6Parcel10writeInt64El
        cbz     w0, .LBB6_3
// %bb.2:
        //APP
        mrs     x8, TPIDR_EL0
        //NO_APP
        ldr     x8, [x8, #56]
        mov     w19, w0
        ldr     x0, [x8, #56]
        ldr     x8, [x0]
        ldr     x8, [x8]
        blr     x8
        mov     w2, w19
        mov     w3, wzr
        bl      signalExceptionForError
        ldp     x29, x30, [sp, #16]     // 16-byte Folded Reload
        ldr     x19, [sp], #32          // 8-byte Folded Reload
        b       MRT_CheckThrowPendingExceptionUnw
.LBB6_3:
        ldp     x29, x30, [sp, #16]     // 16-byte Folded Reload
        ldr     x19, [sp], #32          // 8-byte Folded Reload
        ret
.Lfunc_end8:
        .size   Landroid_2Fos_2FParcel_3B_7CnativeWriteLong_7C_28JJ_29V_native, .Lfunc_end8-Landroid_2Fos_2FParcel_3B_7CnativeWriteLong_7C_28JJ_29V_native
        .cfi_endproc
                                        // -- End function
        .section        .text.Landroid_2Fos_2FParcel_3B_7CnativeReadString_7C_28J_29Ljava_2Flang_2FString_3B_native,"ax",@progbits
        .globl  Landroid_2Fos_2FParcel_3B_7CnativeReadString_7C_28J_29Ljava_2Flang_2FString_3B_native // -- Begin function Landroid_2Fos_2FParcel_3B_7CnativeReadString_7C_28J_29Ljava_2Flang_2FString_3B_native
        .p2align        2
        .type   Landroid_2Fos_2FParcel_3B_7CnativeReadString_7C_28J_29Ljava_2Flang_2FString_3B_native,@function
Landroid_2Fos_2FParcel_3B_7CnativeReadString_7C_28J_29Ljava_2Flang_2FString_3B_native: // @Landroid_2Fos_2FParcel_3B_7CnativeReadString_7C_28J_29Ljava_2Flang_2FString_3B_native
        .cfi_startproc
// %bb.0:
        sub     sp, sp, #48             // =48
        str     x19, [sp, #16]          // 8-byte Folded Spill
        stp     x29, x30, [sp, #32]     // 16-byte Folded Spill
        add     x29, sp, #32            // =32
        .cfi_def_cfa w29, 16
        .cfi_offset w30, -8
        .cfi_offset w29, -16
        .cfi_offset w19, -32
        mrs     x19, TPIDR_EL0
        ldr     x8, [x19, #40]
        str     x8, [sp, #8]
        cbz     x0, .LBB3_4
// %bb.1:
        mov     x1, sp
        bl      _ZNK7android6Parcel19readString16InplaceEPm
        cbz     x0, .LBB3_4
// %bb.2:
        ldr     x1, [sp]
        lsr     x8, x1, #31
        cbnz    x8, .LBB3_7
// %bb.3:
                                        // kill: def $w1 killed $w1 killed $x1
        bl      MRT_NewHeapJStr
.LBB3_4:
        ldr     x8, [x19, #40]
        ldr     x9, [sp, #8]
        cmp     x8, x9
        b.ne    .LBB3_6
// %bb.5:
        ldp     x29, x30, [sp, #32]     // 16-byte Folded Reload
        ldr     x19, [sp, #16]          // 8-byte Folded Reload
        add     sp, sp, #48             // =48
        ret
.LBB3_6:
        bl      __stack_chk_fail
.LBB3_7:
        bl      __ubsan_handle_implicit_conversion_minimal_abort
.Lfunc_end3_ReadString:
        .size   Landroid_2Fos_2FParcel_3B_7CnativeReadString_7C_28J_29Ljava_2Flang_2FString_3B_native, .Lfunc_end3_ReadString-Landroid_2Fos_2FParcel_3B_7CnativeReadString_7C_28J_29Ljava_2Flang_2FString_3B_native
        .cfi_endproc
                                        // -- End function
        .section        .text.Landroid_2Fos_2FParcel_3B_7CnativeReadInt_7C_28J_29I_native,"ax",@progbits
        .globl  Landroid_2Fos_2FParcel_3B_7CnativeReadInt_7C_28J_29I_native // -- Begin function Landroid_2Fos_2FParcel_3B_7CnativeReadInt_7C_28J_29I_native
        .p2align        2
        .type   Landroid_2Fos_2FParcel_3B_7CnativeReadInt_7C_28J_29I_native,@function
Landroid_2Fos_2FParcel_3B_7CnativeReadInt_7C_28J_29I_native: // @Landroid_2Fos_2FParcel_3B_7CnativeReadInt_7C_28J_29I_native
        .cfi_startproc
// %bb.0:
        cbz     x0, .LBB4_2
// %bb.1:
        b       _ZNK7android6Parcel9readInt32Ev
.LBB4_2:
        ret
.Lfunc_end4_nativeReadInt:
        .size   Landroid_2Fos_2FParcel_3B_7CnativeReadInt_7C_28J_29I_native, .Lfunc_end4_nativeReadInt-Landroid_2Fos_2FParcel_3B_7CnativeReadInt_7C_28J_29I_native
        .cfi_endproc
                                        // -- End function
        .section        .text.Landroid_2Fos_2FParcel_3B_7CnativeWriteInterfaceToken_7C_28JLjava_2Flang_2FString_3B_29V_native,"ax",@progbits
        .globl  Landroid_2Fos_2FParcel_3B_7CnativeWriteInterfaceToken_7C_28JLjava_2Flang_2FString_3B_29V_native // -- Begin function Landroid_2Fos_2FParcel_3B_7CnativeWriteInterfaceToken_7C_28JLjava_2Flang_2FString_3B_29V_native
        .p2align        2
        .type   Landroid_2Fos_2FParcel_3B_7CnativeWriteInterfaceToken_7C_28JLjava_2Flang_2FString_3B_29V_native,@function
Landroid_2Fos_2FParcel_3B_7CnativeWriteInterfaceToken_7C_28JLjava_2Flang_2FString_3B_29V_native: // @Landroid_2Fos_2FParcel_3B_7CnativeWriteInterfaceToken_7C_28JLjava_2Flang_2FString_3B_29V_native
        .cfi_startproc
// %bb.0:
        str     x28, [sp, #-80]!        // 8-byte Folded Spill
        stp     x24, x23, [sp, #16]     // 16-byte Folded Spill
        stp     x22, x21, [sp, #32]     // 16-byte Folded Spill
        stp     x20, x19, [sp, #48]     // 16-byte Folded Spill
        stp     x29, x30, [sp, #64]     // 16-byte Folded Spill
        add     x29, sp, #64            // =64
        sub     sp, sp, #528            // =528
        .cfi_def_cfa w29, 16
        .cfi_offset w30, -8
        .cfi_offset w29, -16
        .cfi_offset w19, -24
        .cfi_offset w20, -32
        .cfi_offset w21, -40
        .cfi_offset w22, -48
        .cfi_offset w23, -56
        .cfi_offset w24, -64
        .cfi_offset w28, -80
        mrs     x24, TPIDR_EL0
        ldr     x8, [x24, #40]
        stur    x8, [x29, #-72]
        cbz     x0, .LBB5_15
// %bb.1:
        mov     x22, x1
        cbz     x1, .LBB5_15
// %bb.2:
        ldr     w9, [x22, #8]
        tbnz    w9, #31, .LBB5_26
// %bb.3:
        mov     x19, x0
        asr     w8, w9, #1
        add     x20, x22, #16           // =16
        tbnz    w9, #0, .LBB5_5
// %bb.4:
        sxtw    x2, w8
        add     x0, sp, #8              // =8
        mov     x1, x20
        bl      _ZN7android8String16C1EPKDsm
        add     x1, sp, #8              // =8
        mov     x0, x19
        bl      _ZN7android6Parcel19writeInterfaceTokenERKNS_8String16E
        add     x0, sp, #8              // =8
        b       .LBB5_14
.LBB5_5:
        cmp     w9, #513                // =513
        b.gt    .LBB5_17
// %bb.6:
        cmp     w9, #2                  // =2
        sxtw    x2, w8
        b.lt    .LBB5_13
// %bb.7:
        add     x9, sp, #8              // =8
        add     x10, x9, x2, lsl #1
        orr     x11, x9, #0x2
        cmp     x10, x11
        csel    x10, x10, x11, hi
        mvn     x11, x9
        add     x10, x10, x11
        lsr     x10, x10, #1
        add     x11, x10, #1            // =1
        cmp     x11, #16                // =16
        add     x10, sp, #8             // =8
        b.lo    .LBB5_11
// %bb.8:
        and     x12, x11, #0xfffffffffffffff0
        add     x13, sp, #8             // =8
        add     x20, x20, x12
        add     x10, x13, x12, lsl #1
        add     x13, x13, #16           // =16
        add     x14, x22, #24           // =24
        mov     x15, x12
.LBB5_9:                                // =>This Inner Loop Header: Depth=1
        ldp     d0, d1, [x14, #-8]
        subs    x15, x15, #16           // =16
        add     x14, x14, #16           // =16
        ushll   v0.8h, v0.8b, #0
        ushll   v1.8h, v1.8b, #0
        stp     q0, q1, [x13, #-16]
        add     x13, x13, #32           // =32
        b.ne    .LBB5_9
// %bb.10:
        cmp     x11, x12
        b.eq    .LBB5_13
.LBB5_11:
        add     x8, x9, w8, sxtw #1
.LBB5_12:                               // =>This Inner Loop Header: Depth=1
        ldrb    w9, [x20], #1
        strh    w9, [x10], #2
        cmp     x10, x8
        b.lo    .LBB5_12
.LBB5_13:
        mov     x0, sp
        add     x1, sp, #8              // =8
        bl      _ZN7android8String16C1EPKDsm
        mov     x1, sp
        mov     x0, x19
        bl      _ZN7android6Parcel19writeInterfaceTokenERKNS_8String16E
        mov     x0, sp
.LBB5_14:
        bl      _ZN7android8String16D1Ev
.LBB5_15:
        ldr     x8, [x24, #40]
        ldur    x9, [x29, #-72]
        cmp     x8, x9
        b.ne    .LBB5_25
.LBB5_16:
        add     sp, sp, #528            // =528
        ldp     x29, x30, [sp, #64]     // 16-byte Folded Reload
        ldp     x20, x19, [sp, #48]     // 16-byte Folded Reload
        ldp     x22, x21, [sp, #32]     // 16-byte Folded Reload
        ldp     x24, x23, [sp, #16]     // 16-byte Folded Reload
        ldr     x28, [sp], #80          // 8-byte Folded Reload
        ret
.LBB5_17:
        sxtw    x23, w8
        orr     w1, wzr, #0x2
        mov     x0, x23
        bl      calloc
        cbz     x0, .LBB5_15
// %bb.18:
        add     x8, x0, x23, lsl #1
        mov     x21, x0
        cmp     x8, x0
        b.ls    .LBB5_24
// %bb.19:
        lsl     x9, x23, #1
        sub     x9, x9, #1              // =1
        lsr     x9, x9, #1
        add     x10, x9, #1             // =1
        cmp     x10, #16                // =16
        mov     x9, x21
        b.lo    .LBB5_23
// %bb.20:
        and     x11, x10, #0xfffffffffffffff0
        add     x12, x21, #16           // =16
        add     x20, x20, x11
        add     x9, x21, x11, lsl #1
        add     x13, x22, #24           // =24
        mov     x14, x11
.LBB5_21:                               // =>This Inner Loop Header: Depth=1
        ldp     d0, d1, [x13, #-8]
        subs    x14, x14, #16           // =16
        add     x13, x13, #16           // =16
        ushll   v0.8h, v0.8b, #0
        ushll   v1.8h, v1.8b, #0
        stp     q0, q1, [x12, #-16]
        add     x12, x12, #32           // =32
        b.ne    .LBB5_21
// %bb.22:
        cmp     x10, x11
        b.eq    .LBB5_24
.LBB5_23:                               // =>This Inner Loop Header: Depth=1
        ldrb    w10, [x20], #1
        strh    w10, [x9], #2
        cmp     x9, x8
        b.lo    .LBB5_23
.LBB5_24:
        add     x0, sp, #8              // =8
        mov     x1, x21
        mov     x2, x23
        bl      _ZN7android8String16C1EPKDsm
        add     x1, sp, #8              // =8
        mov     x0, x19
        bl      _ZN7android6Parcel19writeInterfaceTokenERKNS_8String16E
        add     x0, sp, #8              // =8
        bl      _ZN7android8String16D1Ev
        mov     x0, x21
        bl      free
        ldr     x8, [x24, #40]
        ldur    x9, [x29, #-72]
        cmp     x8, x9
        b.eq    .LBB5_16
.LBB5_25:
        bl      __stack_chk_fail
.LBB5_26:
        bl      __ubsan_handle_implicit_conversion_minimal_abort
.Lfunc_end5:
        .size   Landroid_2Fos_2FParcel_3B_7CnativeWriteInterfaceToken_7C_28JLjava_2Flang_2FString_3B_29V_native, .Lfunc_end5-Landroid_2Fos_2FParcel_3B_7CnativeWriteInterfaceToken_7C_28JLjava_2Flang_2FString_3B_29V_native
        .cfi_endproc
                                        // -- End function

        .section        .text.Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native_inner,"ax",@progbits
        .local  Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native_inner // -- Begin function Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native_inner
        .p2align        2
        .type   Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native_inner,@function
Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native_inner: // @Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native_inner
        .cfi_startproc
// %bb.0:
        str     x28, [sp, #-96]!        // 8-byte Folded Spill
        stp     x26, x25, [sp, #16]     // 16-byte Folded Spill
        stp     x24, x23, [sp, #32]     // 16-byte Folded Spill
        stp     x22, x21, [sp, #48]     // 16-byte Folded Spill
        stp     x20, x19, [sp, #64]     // 16-byte Folded Spill
        stp     x29, x30, [sp, #80]     // 16-byte Folded Spill
        add     x29, sp, #80            // =80
        sub     sp, sp, #528            // =528
        .cfi_def_cfa w29, 16
        .cfi_offset w30, -8
        .cfi_offset w29, -16
        .cfi_offset w19, -24
        .cfi_offset w20, -32
        .cfi_offset w21, -40
        .cfi_offset w22, -48
        .cfi_offset w23, -56
        .cfi_offset w24, -64
        .cfi_offset w25, -72
        .cfi_offset w26, -80
        .cfi_offset w28, -96
        mrs     x26, TPIDR_EL0
        ldr     x8, [x26, #40]
        stur    x8, [x29, #-88]
        cbz     x0, .LBB6_24
// %bb.1:
        mov     x23, x1
        cbz     x1, .LBB6_24
// %bb.2:
        mov     x21, x0
        bl      _ZN7android14IPCThreadState4selfEv
        mov     x19, x0
        bl      _ZNK7android14IPCThreadState19getStrictModePolicyEv
        ldr     w9, [x23, #8]
        tbnz    w9, #31, .LBB6_28
// %bb.3:
        mov     w20, w0
        asr     w8, w9, #1
        add     x22, x23, #16           // =16
        tbnz    w9, #0, .LBB6_5
// %bb.4:
        sxtw    x2, w8
        add     x0, sp, #8              // =8
        mov     x1, x22
        bl      _ZN7android8String16C1EPKDsm
        add     x1, sp, #8              // =8
        mov     x0, x21
        mov     x2, x19
        bl      _ZNK7android6Parcel16enforceInterfaceERKNS_8String16EPNS_14IPCThreadStateE
        mov     w21, w0
        add     x0, sp, #8              // =8
        bl      _ZN7android8String16D1Ev
        tbnz    w21, #0, .LBB6_22
        b       .LBB6_24
.LBB6_5:
        cmp     w9, #513                // =513
        b.gt    .LBB6_14
// %bb.6:
        cmp     w9, #2                  // =2
        sxtw    x2, w8
        b.lt    .LBB6_13
// %bb.7:
        add     x9, sp, #8              // =8
        add     x10, x9, x2, lsl #1
        orr     x11, x9, #0x2
        cmp     x10, x11
        csel    x10, x10, x11, hi
        mvn     x11, x9
        add     x10, x10, x11
        lsr     x10, x10, #1
        add     x11, x10, #1            // =1
        cmp     x11, #16                // =16
        add     x10, sp, #8             // =8
        b.lo    .LBB6_11
// %bb.8:
        and     x12, x11, #0xfffffffffffffff0
        add     x13, sp, #8             // =8
        add     x22, x22, x12
        add     x10, x13, x12, lsl #1
        add     x13, x13, #16           // =16
        add     x14, x23, #24           // =24
        mov     x15, x12
.LBB6_9:                                // =>This Inner Loop Header: Depth=1
        ldp     d0, d1, [x14, #-8]
        subs    x15, x15, #16           // =16
        add     x14, x14, #16           // =16
        ushll   v0.8h, v0.8b, #0
        ushll   v1.8h, v1.8b, #0
        stp     q0, q1, [x13, #-16]
        add     x13, x13, #32           // =32
        b.ne    .LBB6_9
// %bb.10:
        cmp     x11, x12
        b.eq    .LBB6_13
.LBB6_11:
        add     x8, x9, w8, sxtw #1
.LBB6_12:                               // =>This Inner Loop Header: Depth=1
        ldrb    w9, [x22], #1
        strh    w9, [x10], #2
        cmp     x10, x8
        b.lo    .LBB6_12
.LBB6_13:
        mov     x0, sp
        add     x1, sp, #8              // =8
        bl      _ZN7android8String16C1EPKDsm
        mov     x1, sp
        mov     x0, x21
        mov     x2, x19
        bl      _ZNK7android6Parcel16enforceInterfaceERKNS_8String16EPNS_14IPCThreadStateE
        mov     w21, w0
        mov     x0, sp
        bl      _ZN7android8String16D1Ev
        tbnz    w21, #0, .LBB6_22
        b       .LBB6_24
.LBB6_14:
        sxtw    x25, w8
        orr     w1, wzr, #0x2
        mov     x0, x25
        bl      calloc
        cbz     x0, .LBB6_22
// %bb.15:
        add     x8, x0, x25, lsl #1
        mov     x24, x0
        cmp     x8, x0
        b.ls    .LBB6_21
// %bb.16:
        lsl     x9, x25, #1
        sub     x9, x9, #1              // =1
        lsr     x9, x9, #1
        add     x10, x9, #1             // =1
        cmp     x10, #16                // =16
        mov     x9, x24
        b.lo    .LBB6_20
// %bb.17:
        and     x11, x10, #0xfffffffffffffff0
        add     x12, x24, #16           // =16
        add     x22, x22, x11
        add     x9, x24, x11, lsl #1
        add     x13, x23, #24           // =24
        mov     x14, x11
.LBB6_18:                               // =>This Inner Loop Header: Depth=1
        ldp     d0, d1, [x13, #-8]
        subs    x14, x14, #16           // =16
        add     x13, x13, #16           // =16
        ushll   v0.8h, v0.8b, #0
        ushll   v1.8h, v1.8b, #0
        stp     q0, q1, [x12, #-16]
        add     x12, x12, #32           // =32
        b.ne    .LBB6_18
// %bb.19:
        cmp     x10, x11
        b.eq    .LBB6_21
.LBB6_20:                               // =>This Inner Loop Header: Depth=1
        ldrb    w10, [x22], #1
        strh    w10, [x9], #2
        cmp     x9, x8
        b.lo    .LBB6_20
.LBB6_21:
        add     x0, sp, #8              // =8
        mov     x1, x24
        mov     x2, x25
        bl      _ZN7android8String16C1EPKDsm
        add     x1, sp, #8              // =8
        mov     x0, x21
        mov     x2, x19
        bl      _ZNK7android6Parcel16enforceInterfaceERKNS_8String16EPNS_14IPCThreadStateE
        mov     w21, w0
        add     x0, sp, #8              // =8
        bl      _ZN7android8String16D1Ev
        mov     x0, x24
        bl      free
        tbz     w21, #0, .LBB6_24
.LBB6_22:
        mov     x0, x19
        bl      _ZNK7android14IPCThreadState19getStrictModePolicyEv
        cmp     w20, w0
        b.eq    .LBB6_25
// %bb.23:
        //APP
        //mrs     x8, TPIDR_EL0
        //NO_APP
        //ldr     x8, [x8, #56]
        //mov     w19, w0
        //ldr     x0, [x8, #56]
        //ldr     x8, [x0]
        //ldr     x8, [x8]
        //blr     x8
        //mov     w1, w19
        //bl      _ZN7android28set_dalvik_blockguard_policyEP7_JNIEnvi
        bl      Landroid_2Fos_2FStrictMode_3B_7CsetBlockGuardPolicy_7C_28I_29V
        ldr     x8, [x26, #40]
        ldur    x9, [x29, #-88]
        cmp     x8, x9
        b.eq    .LBB6_26
        b       .LBB6_27
.LBB6_24:
        //APP
        mrs     x8, TPIDR_EL0
        //NO_APP
        ldr     x8, [x8, #56]
        ldr     x0, [x8, #56]
        ldr     x8, [x0]
        ldr     x8, [x8]
        blr     x8
        adrp    x1, .L.str.7
        adrp    x2, .L.str.8
        add     x1, x1, :lo12:.L.str.7
        add     x2, x2, :lo12:.L.str.8
        bl      jniThrowException
.LBB6_25:
        ldr     x8, [x26, #40]
        ldur    x9, [x29, #-88]
        cmp     x8, x9
        b.ne    .LBB6_27
.LBB6_26:
        add     sp, sp, #528            // =528
        ldp     x29, x30, [sp, #80]     // 16-byte Folded Reload
        ldp     x20, x19, [sp, #64]     // 16-byte Folded Reload
        ldp     x22, x21, [sp, #48]     // 16-byte Folded Reload
        ldp     x24, x23, [sp, #32]     // 16-byte Folded Reload
        ldp     x26, x25, [sp, #16]     // 16-byte Folded Reload
        ldr     x28, [sp], #96          // 8-byte Folded Reload
        ret
.LBB6_27:
        bl      __stack_chk_fail
.LBB6_28:
        bl      __ubsan_handle_implicit_conversion_minimal_abort
.Lfunc_end6:
        .size   Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native_inner, .Lfunc_end6-Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native_inner
        .cfi_endproc
                                        // -- End function
        .section        .text.Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native,"ax",@progbits
        .globl  Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native // -- Begin function Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native
        .p2align        2
        .type   Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native,@function
Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native: // @Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native
        .cfi_startproc
// %bb.0:
        stp     x29, x30, [sp, #-16]!   // 16-byte Folded Spill
        mov     x29, sp
        .cfi_def_cfa w29, 16
        .cfi_offset w30, -8
        .cfi_offset w29, -16
        bl      Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native_inner
        bl      MRT_CheckThrowPendingExceptionUnw
        ldp     x29, x30, [sp], #16     // 16-byte Folded Reload
        b       MRT_SetReliableUnwindContextStatus
.Lfunc_end7:
        .size   Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native, .Lfunc_end7-Landroid_2Fos_2FParcel_3B_7CnativeEnforceInterface_7C_28JLjava_2Flang_2FString_3B_29V_native
        .cfi_endproc
                                        // -- End function

        .type   .L.str,@object          // @.str
        .section        .rodata.str1.1,"aMS",@progbits,1
.L.str:
        .asciz  "java/lang/RuntimeException"
        .size   .L.str, 27

        .type   .L.str.1,@object        // @.str.1
.L.str.1:
        .asciz  "Unknown error"
        .size   .L.str.1, 14

        .type   .L.str.2,@object        // @.str.2
.L.str.2:
        .asciz  "java/lang/OutOfMemoryError"
        .size   .L.str.2, 27

        .type   .L.str.3,@object        // @.str.3
.L.str.3:
        .asciz  "java/lang/UnsupportedOperationException"
        .size   .L.str.3, 40

        .type   .L.str.4,@object        // @.str.4
.L.str.4:
        .asciz  "java/lang/IllegalArgumentException"
        .size   .L.str.4, 35

        .type   .L.str.5,@object        // @.str.5
.L.str.5:
        .asciz  "Unknown binder error code. 0x%x"
        .size   .L.str.5, 32

        .type   .L.str.6,@object        // @.str.6
.L.str.6:
        .asciz  "android/os/RemoteException"
        .size   .L.str.6, 27

        .type   .L.str.7,@object        // @.str.7
.L.str.7:
        .asciz  "java/lang/SecurityException"
        .size   .L.str.7, 28

        .type   .L.str.8,@object        // @.str.8
.L.str.8:
        .asciz  "Binder invocation to an incorrect interface"
        .size   .L.str.8, 44

#Libframework_end
