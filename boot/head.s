/*
 * TEMPORARY (test T2): this replaces Linus's boot/head.s with a tiny
 * 32-bit stub, so setup's jump into protected mode can be tested before
 * the kernel can be built. It is reverted to the original in step 4.3.
 *
 * setup jumps here (address 0) with ljmp $8,$0, in 32-bit protected mode.
 * Write "PM OK" straight into VGA text memory (row 12) and loop.
 */
.code32
.text
.globl startup_32
startup_32:
	movl $0x10,%eax			# GDT data segment from setup.s
	mov %ax,%ds
	movl $0xb8000+12*160,%edi	# VGA text memory, row 12, column 0
	movl $msg,%esi
1:	movb (%esi),%al
	testb %al,%al
	jz 2f
	movb %al,(%edi)
	movb $0x0f,1(%edi)		# bright white on black
	addl $2,%edi
	incl %esi
	jmp 1b
2:	jmp 2b
msg:	.asciz "PM OK"
