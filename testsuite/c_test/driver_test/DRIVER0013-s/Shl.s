	#	/home/guowanlong/Maple/OpenArkCompiler/testsuite/c_test/x64_be_enable_test/ENABLE0004-Shl/mplcg
	#	Compiling 
	#	Be options
	.file	"/home/guowanlong/Maple/OpenArkCompiler/testsuite/c_test/x64_be_enable_test/ENABLE0004-Shl/./Shl.me.mpl"
	.file	1 "/home/guowanlong/Maple/OpenArkCompiler/testsuite/c_test/x64_be_enable_test/ENABLE0004-Shl/./Shl.me.mpl"
	.file	2 "/home/guowanlong/Maple/OpenArkCompiler/testsuite/c_test/x64_be_enable_test/ENABLE0004-Shl/Shl.c"

	.text
	.align 5
	.globl	main
	.type	main, %function
main:
.L.1__1:
	.loc 2 1 5
	.cfi_startproc
	stp	x29, x30, [sp,#-48]!
	.cfi_def_cfa_offset 48
	.cfi_offset 30, -40
	.loc 2 2 7
	mov	w0, #1
	str	w0, [sp,#16]
	.loc 2 3 7
	ldr	w0, [sp,#16]
	lsl	w0, w0, #4
	str	w0, [sp,#20]
	.loc 2 4 7
	ldr	w0, [sp,#16]
	ldr	w1, [sp,#20]
	lsl	w0, w0, w1
	str	w0, [sp,#24]
	.loc 2 5 3
	adrp	x0, .LUstr_1
	add	x0, x0, :lo12:.LUstr_1
	mov	x0, x0
	ldr	w1, [sp,#20]
	mov	w1, w1
	ldr	w2, [sp,#24]
	mov	w2, w2
	bl	printf
	str	w0, [sp,#28]
	.loc 2 6 3
	mov	w0, #0
	b	.L.1__2
.L.1__2:
	ldp	x29, x30, [sp], #48
	.cfi_restore 30
	.cfi_def_cfa 31, 0
	ret	
.L.1__3:
	.cfi_endproc
	.size	main, .-main
	.section	.rodata
	.align 3
.LUstr_1:
	.string	"%d,%d\n"
