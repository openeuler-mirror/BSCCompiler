	.local	Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I
	.type	Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I, %function
	.word .Lmethod_desc.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I - .
Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I:
.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I9:
	.cfi_startproc
	.cfi_personality 155, DW.ref.__mpl_personality_v0
	ldr	w2, [x0,#12]         // Ljava/lang/String;.hash
	cbnz	w2, .Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I1
	ldr	w1, [x0,#8]          // length + flag for compressed string
	lsr	w4, w1, #1
	cbz	w4, .Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I1
	mov	w5, #0               // start offset
	add	x3, x0, #16          // start address
	tbz	w1, #0, .Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I4
.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I3:
	ldrb	w6, [x3,w5,SXTW #0]  // compressed string
	lsl	w1, w2, #5
	add	w5, w5, #1
	sub	w1, w1, w2
	cmp	w5, w4
	add	w2, w1, w6
	blt	.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I3
	b	.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I6
.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I4:
	ldrh	w6, [x3,w5,SXTW #1]  // normal string
	lsl	w1, w2, #5
	add	w5, w5, #1
	sub	w1, w1, w2
	cmp	w5, w4
	add	w2, w1, w6
	blt	.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I4
.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I6:
	str	w2, [x0,#12]         // hash = h
.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I1:
	mov	w0, w2
.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I12:
	ret
.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I11:
	b	.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I12
.Label.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I10:
	.cfi_endproc
.Label.end.Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I:
	.size	Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I, .-Ljava_2Flang_2FString_3B_7ChashCode_7C_28_29I
	.word   0x55555555
