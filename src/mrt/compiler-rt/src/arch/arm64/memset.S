// this file implements memset or similar functionalities

        .text
        .align 2
        .global ROSAllocZero
        .hidden ROSAllocZero         // remove plt within so
        .type ROSAllocZero, %function
ROSAllocZero:
        .cfi_startproc

// this works on sizes >= 8, since rosalloc doesn't need any size
// smaller than that

// based on profiling, the most frequent size should be [16, 32],
// followed by (32, 96], followed by the others, i.e., [8, 16) and (96, +inf);
// notice that size [8, 16) is not frequent at all

// x0: dst, x1: size, x2: dstend, x3: dstmid

        dup     v0.16B, wzr
        add     x2, x0, x1
        cmp     x1, 32
        b.hi    .Lhi32
        cmp     x1, 16
        b.lo    .Llo16

        str     q0, [x0]
        str     q0, [x2, -16]
        ret

.Lhi32: // size > 32
        stp     q0, q0, [x0]         // head 32
        stp     q0, q0, [x2, -32]    // tail 32
        cmp     x1, 256
        b.hs    .Lhs256 // optimisation for pages?
        add     x3, x0, x1, lsr #1
        stp     q0, q0, [x3, -16]    // mid 32
        cmp     x1, 96
        b.hi    .Lhi96
        ret

.Lhi96: // size > 96
        // stp     q0, q0, [x0]      // head 32, done in .Lhi32
        stp     q0, q0, [x0, 32]
        stp     q0, q0, [x0, 64]
        stp     q0, q0, [x3, -48]
        // stp     q0, q0, [x3, -16] // mid 32, done in .Lhi32
        stp     q0, q0, [x3, 16]
        stp     q0, q0, [x2, -96]
        stp     q0, q0, [x2, -64]
        // stp     q0, q0, [x2, -32] // tail 32, done in .Lhi32
        ret

.Llo16: // size < 16
        str     xzr, [x0]
        str     xzr, [x2, -8]
        ret

.Lhs256: // size >= 256
        mrs     x4, dczid_el0
        tbnz    w4, 4, .Lnozva
        and     w4, w4, 15
        cmp     w4, 4
        b.ne    .Lzva128

.Lzva64:
        // stp     q0, q0, [x0]      // head 32, done in .Lhi32
        stp     q0, q0, [x0, 32]
        bic     x0, x0, 63
        sub     x5, x2, 128
1:
        add     x0, x0, 64
        dc      zva, x0
        cmp     x5, x0
        b.hi    1b
        stp     q0, q0, [x2, -64]
        // stp     q0, q0, [x2, -32] // tail 32, done in .Lhi32
        ret

.Lzva128:
        cmp     w4, 5
        b.ne    .Lnozva

        // stp     q0, q0, [x0]      // head 32, done in .Lhi32
        stp     q0, q0, [x0, 32]
        stp     q0, q0, [x0, 64]
        stp     q0, q0, [x0, 96]
        bic     x0, x0, 127
        sub     x5, x2, 128
        sub     x5, x5, 128
1:
        add     x0, x0, 128
        dc      zva, x0
        cmp     x5, x0
        b.hi    1b
        stp     q0, q0, [x2, -128]
        stp     q0, q0, [x2, -96]
        stp     q0, q0, [x2, -64]
        // stp     q0, q0, [x2, -32] // tail 32, done in .Lhi32
        ret

.Lnozva:
        // stp     q0, q0, [x0]      // head 32, done in .Lhi32
        sub     x5, x2, 64
1:
        add     x0, x0, 32
        stp     q0, q0, [x0]
        cmp     x5, x0
        b.hi    1b
        // stp     q0, q0, [x2, -32] // tail 32, done in .Lhi32
        ret

        .cfi_endproc
        .size ROSAllocZero, .-ROSAllocZero
