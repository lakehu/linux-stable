/*
 * hello-elf: a static i386 ELF test program without libc, linked at 0.
 * It prints its build stamp (date, time, gcc version) and what the kernel
 * passes at entry: %edx, argc/argv[], envp[] and the auxiliary vector
 * (i386 ELF ABI), then exits with code 0.
 */

static int sys3(int nr, long a, long b, long c)
{
	int ret;

	__asm__ __volatile__("int $0x80" : "=a" (ret)
		: "0" (nr), "b" (a), "c" (b), "d" (c) : "memory");
	return ret;
}

static void put(const char * s)
{
	int n = 0;

	while (s[n])
		n++;
	sys3(4, 1, (long) s, n);		/* write(1, s, n) */
}

static void putnum(unsigned long v)
{
	char buf[12];
	int i = sizeof(buf) - 1;

	buf[i] = 0;
	do
		buf[--i] = '0' + v % 10;
	while (v /= 10);
	put(buf + i);
}

void cmain(unsigned long edx, unsigned long * sp)
{
	int argc = sp[0], i;
	char ** argv = (char **) (sp + 1);
	char ** envp = argv + argc + 1;

	put("hello from a static ELF program, built " __DATE__ " " __TIME__
		" with gcc " __VERSION__ "\n");
	put("edx at entry: ");
	put(edx ? "NOT 0\n" : "0\n");
	put("argc: ");
	putnum(argc);
	put("\n");
	for (i = 0 ; i < argc ; i++) {
		put("argv: ");
		put(argv[i]);
		put("\n");
	}
	for (i = 0 ; envp[i] ; i++) {
		put("envp: ");
		put(envp[i]);
		put("\n");
	}
	put(envp[i + 1] == 0 ? "auxv: AT_NULL\n" : "auxv: not empty\n");
	sys3(1, 0, 0, 0);			/* exit(0) */
}

/* entry: cmain(%edx, initial %esp) */
__asm__(".text\n.globl _start\n_start:\n"
	"\tmovl %esp,%eax\n\tpushl %eax\n\tpushl %edx\n\tcall cmain\n");
