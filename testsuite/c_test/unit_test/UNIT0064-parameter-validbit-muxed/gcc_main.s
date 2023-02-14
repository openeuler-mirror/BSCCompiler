	.arch armv8-a
	.file	"main.c"
	.text
	.align	2
	.p2align 3,,7
	.global	foo
	.type	foo, %function
foo:
	mov	x1, 60484
	movk	x1, 0xccd8, lsl 16
	movk	x1, 0x1, lsl 32
	add	x0, x0, x1
	ret
	.size	foo, .-foo
	.section	.text.startup,"ax",@progbits
	.align	2
	.p2align 3,,7
	.global	main
	.type	main, %function
main:
	stp	x29, x30, [sp, -16]!
	mov	w0, -1
	add	x29, sp, 0
	bl	func_1
	mov	x0, 1
	bl	foo
	bl	func_2
	mov	w0, 0
	ldp	x29, x30, [sp], 16
	ret
	.size	main, .-main
	.ident	"GCC: (Ubuntu/Linaro 5.5.0-12ubuntu1) 5.5.0 20171010"
	.section	.note.GNU-stack,"",@progbits
