/*
 * tools/hello.s - the first user program for the tiny root file system.
 * It is installed as /bin/sh, which init() in init/main.c executes.
 *
 * Linked at address 0 (user code and data segments both start at the
 * process base in 0.11), and wrapped into an a.out ZMAGIC file by
 * tools/mkaout.py.
 */
.code32
.text
.globl _start
_start:
	movl $4,%eax		# __NR_write
	movl $1,%ebx		# stdout (/dev/tty0, set up by init)
	movl $msg,%ecx
	movl $len,%edx
	int $0x80
1:	movl $29,%eax		# __NR_pause: sleep forever, so init does
	int $0x80		# not start us again and again
	jmp 1b

msg:	.ascii "hello from userspace\n\r"
len = . - msg
