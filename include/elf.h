#ifndef _ELF_H
#define _ELF_H

/*
 * Minimal ELF definitions for the static i386 loader in fs/exec.c.
 * Field layout from the System V ABI (as in glibc's <elf.h>, which the
 * Linux 0.01 remake by Abdel Benamrouche copied in patch_rm_3.0); only
 * what the loader uses is defined here.
 */

typedef unsigned long	Elf32_Addr;
typedef unsigned short	Elf32_Half;
typedef unsigned long	Elf32_Off;
typedef unsigned long	Elf32_Word;

#define EI_NIDENT	16

typedef struct {
	unsigned char	e_ident[EI_NIDENT];	/* magic number and other info */
	Elf32_Half	e_type;			/* object file type */
	Elf32_Half	e_machine;		/* architecture */
	Elf32_Word	e_version;
	Elf32_Addr	e_entry;		/* entry point virtual address */
	Elf32_Off	e_phoff;		/* program header table file offset */
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;		/* program header table entry size */
	Elf32_Half	e_phnum;		/* program header table entry count */
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	Elf32_Word	p_type;			/* segment type */
	Elf32_Off	p_offset;		/* segment file offset */
	Elf32_Addr	p_vaddr;		/* segment virtual address */
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;		/* segment size in file */
	Elf32_Word	p_memsz;		/* segment size in memory */
	Elf32_Word	p_flags;
	Elf32_Word	p_align;
} Elf32_Phdr;

#define ELFMAG0		0x7f		/* e_ident[0..3] = "\177ELF" */
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'
#define EI_CLASS	4
#define ELFCLASS32	1
#define EI_DATA		5
#define ELFDATA2LSB	1		/* little endian */

#define ET_EXEC		2		/* executable file */
#define EM_386		3		/* Intel 80386 */

#define PT_LOAD		1		/* loadable program segment */

#define AT_NULL		0		/* end of auxiliary vector */

#endif
