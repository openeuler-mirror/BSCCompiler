    .local    Ljava_2Flang_2FString_3B_7Cequals_7C_28Ljava_2Flang_2FObject_3B_29Z
    .type    Ljava_2Flang_2FString_3B_7Cequals_7C_28Ljava_2Flang_2FObject_3B_29Z, %function
    .word .Lmethod_desc.Ljava_2Flang_2FString_3B_7Cequals_7C_28Ljava_2Flang_2FObject_3B_29Z - .
Ljava_2Flang_2FString_3B_7Cequals_7C_28Ljava_2Flang_2FObject_3B_29Z:
.Label.Ljava_2Flang_2FString_3B_7Cequals_7C_28Ljava_2Flang_2FObject_3B_29Z26:    //label order 60736
    .cfi_startproc
    .cfi_personality 155, DW.ref.__mpl_personality_v0
        cbz     x1, .Lstring_equals_false  // if the other is null
        cmp     x0, x1
        beq     .Lstring_equals_true       // same address, return true
        ldp     x4, x2, [x0], #16          // read class shadow and count of this
        ldp     x5, x3, [x1], #16
        cmp     w4, w5
        bne     .Lstring_equals_false      // if not the same class
        cmp     w2, w3                     //  Length and compression flag are different
        bne     .Lstring_equals_morecheck
        and     w3, w2, #1
        lsr     w2, w2, w3                 // get the number of bytes
        cbz     w2, .Lstring_equals_true   //  zero length
        add     w2, w2, #7
        lsr     w5, w2, 4
        cbz     w5, .Lstring_equals_tail2   // size is shorter than 8
        add     x5, x0, x5, lsl 4
.Lstring_equals_loophead:
        ldp     x3, x6, [x0], #16
        ldp     x4, x7, [x1], #16
        cmp     x3, x4
        bne     .Lstring_equals_false
        cmp     x6, x7
        bne     .Lstring_equals_false
        cmp     x0, x5
        bne     .Lstring_equals_loophead
.Lstring_equals_tail:
        tbz     w2, #3, .Lstring_equals_true
.Lstring_equals_tail2:
        ldr     x3, [x0]
        ldr     x2, [x1]
        cmp     x3, x2
        bne     .Lstring_equals_false
.Lstring_equals_true:
        mov     w0, 1
        ret
.Lstring_equals_false:
        mov     w0, 0
        ret
.Lstring_equals_morecheck:             // check for compressed bits are diff byte size are equal
        eor     w2, w2, w3
        cmp     w2, #1
        bne     .Lstring_equals_false
        tbnz    w3, #0, .Lstring_equals_loop2
        mov     x2, x0
        mov     x0, x1
        mov     x1, x2    // swap x0 and x1 to make sure that x0 is normal and x1 is compressed
.Lstring_equals_loop2:
        lsr     w3, w3, #1
        // x0: normal, x1; compressed, w3: length
.Lstring_equals_loop2_head:
        subs    w3, w3, #1
        tbnz    w3, #31, .Lstring_equals_true
        ldrh    w4, [x0,w3,SXTW #1]  // normal string
        ldrb    w5, [x1,w3,SXTW #0]  // compressed string
        cmp     w4, w5
        beq     .Lstring_equals_loop2_head
        b       .Lstring_equals_false
    .cfi_endproc
.Label.end.Ljava_2Flang_2FString_3B_7Cequals_7C_28Ljava_2Flang_2FObject_3B_29Z:
    .size    Ljava_2Flang_2FString_3B_7Cequals_7C_28Ljava_2Flang_2FObject_3B_29Z, .-Ljava_2Flang_2FString_3B_7Cequals_7C_28Ljava_2Flang_2FObject_3B_29Z
    .word   0x55555555
