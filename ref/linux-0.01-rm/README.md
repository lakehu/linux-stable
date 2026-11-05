# Reference: Linux 0.01 remake (Abdel Benamrouche, 2007-2008)

The ELF loader and the extra system calls on this branch follow the ideas of
the **Linux 0.01 remake** by **Abdel Benamrouche** (draconux@gmail.com). The
remake ran bash-3.2, coreutils-6.9, dietlibc-0.31, make, ncurses and vim as
ELF programs on a gcc-4 build of Linux 0.01. These are his original patch
files, unmodified, kept as a reference. Nothing in this directory is built.

| File | Date | Content (from `note_patch`) |
|---|---|---|
| `note_patch` | 2007-11-25 | description of all remake patches, from official 0.01 to rm-3.5 |
| `patch_rm_3.0` | 2007-12-24 | "use elf binary format instead of a.out": `fs/exec.c` ELF loader (section headers), `include/elf.h`, `stdint.h`, `bits/*` |
| `patch_rm_3.1` | 2007-12 | "added getdents system call": `fs/sys_getdents.c`, `sys_null`, `include/linux/sys.h`, `kernel/system_call.s` |
| `patch_rm_3.2` | 2007-12 | asm constraint bug fix (`sys_getdents.c`, `init/main.c`, `mm/memory.c`) |
| `patch_rm_3.3` | 2007-12-29 | "some change in system call": `fcntl64`, `lstat`, `stat64/fstat64/lstat64`, `getdents64`, `oldolduname`, 320-entry syscall table with `sys_null(nr)` |
| `patch_rm_3.4` | 2008-01-13 | main Makefile: gawk instead of `cut` for file sizes |
| `patch_rm_3.5` | 2008-01-24 | "fix a bug in the elf loader" (thanks to Wei Hu), use `stat` in Makefile |

## Sources

Fetched on 2026-11-05 from the Internet Archive, because draconux.free.fr no
longer answers:

* News: <https://web.archive.org/web/20110720221018/http://draconux.free.fr/os_dev/linux0.01_news.html>
* About: <https://web.archive.org/web/2011/http://draconux.free.fr/os_dev/linux0.01.html>
* Downloads: <https://web.archive.org/web/2011/http://draconux.free.fr/os_dev/linux0.01_download.html>
* Patches: `https://web.archive.org/web/2011id_/http://draconux.free.fr/download/os-dev/linux0.01/linux-0.01-rm-3.x/patch_rm_3.N`
* Patch notes: `https://web.archive.org/web/2011id_/http://draconux.free.fr/download/os-dev/linux0.01/doc/note_patch`
* Also archived there: tarballs `linux-0.01-rm-{1.7,2.2,3.0..3.5}.tgz`,
  the `hd_oldlinux.img.zip` root disk, and the ported userland sources
  (`userland_src/{bash-3.2,coreutils-6.9,dietlibc-0.31,bin86-0.16.17,binutils-2.18}-ol.tgz`).
* The sources were also mirrored on kernel.org
  (`/pub/linux/kernel/people/abdel/linux_0.01/`, gone today).
* Announcements: LKML post (2008-01-01), OSNews 19123 "Dusting off the 0.01
  Linux kernel" (2008-01-07), LWN, KernelTrap, Linux Journal 169.
* mariuz/linux-0.01 on GitHub (2013) is rm-3.5, with later warning and keyboard
  fixes by isoux (2019).

## How this branch differs

* The ELF loader uses the **program headers** (`PT_LOAD`), not the section
  headers, and keeps the a.out loader for the existing 0.11 binaries.
* `getdents`/`getdents64` follow the **Linux ABI** (several records per call,
  byte count as return value, real `d_reclen`). The remake returns one entry
  with `d_reclen` = name length, which only works with its own dietlibc
  (`readdir64()` in `dietlibc-0.31-ol` re-reads after each entry) and crashes
  uClibc-ng.
* `execve` clears `%edx` as the i386 ELF ABI requires. In the remake it keeps
  a stale value, and uClibc-ng calls it as `rtld_fini` at exit.
