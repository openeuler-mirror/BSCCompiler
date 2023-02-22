	#	/home/guowanlong/Maple/OpenArkCompiler/testsuite/c_test/x64_be_enable_test/ENABLE0004-Shl/mplcg
	#	Compiling 
	#	Be options
	.file	"/home/guowanlong/Maple/OpenArkCompiler/testsuite/c_test/x64_be_enable_test/ENABLE0004-Shl/./main.me.mpl"

	.text
	.align 5
	.globl	main
	.type	main, %function
main:
.L.92__1:
	.cfi_startproc
	stp	x29, x30, [sp,#-64]!
	.cfi_def_cfa_offset 64
	.cfi_offset 30, -56
	str	w0, [sp,#32]
	str	x1, [sp,#40]
	mov	w0, #5
	str	w0, [sp,#16]
	mov	w0, #6
	str	w0, [sp,#20]
	ldr	w0, [sp,#16]
	mov	w0, w0
	ldr	w1, [sp,#20]
	mov	w1, w1
	bl	max
	str	w0, [sp,#24]
	adrp	x1, .LUstr_1
	add	x1, x1, :lo12:.LUstr_1
	mov	x0, x1
	ldr	w1, [sp,#24]
	mov	w1, w1
	bl	printf
	str	w0, [sp,#28]
	mov	w0, #0
	b	.L.92__2
.L.92__2:
	ldp	x29, x30, [sp], #64
	.cfi_restore 30
	.cfi_def_cfa 31, 0
	ret	
.L.92__3:
	.cfi_endproc
	.size	main, .-main
	.section	.rodata
	.align 3
.LUstr_1:
	.string	"%d\n"
