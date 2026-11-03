# Plan: Linux 0.11 with a modern GCC (branch gcc9-minimal)

Base: official Linux 0.11 (`5fb68cc`, December 8, 1991). Each commit adds one small step (or records a
test point, Tn) and updates this file, so it lists only the steps done so far. Reference port:
oldlinux-0.11 (yuan-xy/Linux-0.11).

Rules: (1) easy to hard; (2) no unnecessary changes; (3) assembly unchanged unless required;
(4) tiny rootfs; (5) many small commits, one small feature each.

## Steps so far

### Phase 1: Minimal build setup

| Step | Commit | Notes |
|---|---|---|
| 1.1 | gitignore: ignore build outputs | Object files, archives, a.out (default gcc output), Image, System.map, tools/system, tools/build, boot/bootsect, boot/setup. |
| 1.2 | Makefile: use host GNU as/ld for i386 ELF | AS=as --32, LD=ld, LDFLAGS=-m elf_i386 -Ttext 0 -e startup_32 -M (was 1991 gas/gld, a.out). |

### Phase 2: 16-bit boot code (as86 -> GNU as)

| Step | Commit | Notes |
|---|---|---|
| 2.1 | boot/bootsect.s: convert as86 syntax to GNU as (.code16) | Line-by-line translation, original comments, no behaviour change; ROOT_DEV stays 0x306. |
| 2.2 | Makefile: assemble boot sector with as + ld + objcopy -O binary | Raw 512-byte boot/bootsect instead of as86/ld86 Minix binary. |
| 2.3 | Makefile: build a boot-sector-only floppy image by default | `make` builds `bootimage` = boot/bootsect only (512 bytes). Padding to `FLOPPY_DISK_SIZE` (1474560 = 1.44 MB) is left commented out until later steps need it. `all: Image` returns once setup and system build. |
| 2.4 | Makefile: add 'run' target to boot bootimage in QEMU | `make run`: `qemu-system-i386 -boot a -drive format=raw,file=bootimage,if=floppy -display curses` plus `-monitor unix:qemu-monitor.sock,server,nowait`: QEMU monitor (and `quit`) from a second terminal with `nc -U qemu-monitor.sock`. |
| 2.5 | tools/build.c: accept raw bootsect/setup binaries | Minix header checks removed. |
| 2.6 | tools/build.c: get MAJOR/MINOR from `<sys/sysmacros.h>` | Host tool fix: modern `<linux/fs.h>` lacks MAJOR()/MINOR(). |
| 2.7 | Makefile: build tools/build as a host program | Plain host gcc, not the kernel CFLAGS. |
| 2.8 | boot/bootsect.s: print a message before jumping to setup | New: second BIOS int 0x10 (AH=13h) message "Jumping to setup ..." after the system is loaded (119 bytes left in the boot sector). |
| **T1** | test: boot sector prints its messages | `make run` (bootimage = bootsect only): "Loading system ..." and "Jumping to setup ..." (passed). |
| 2.9 | boot/setup.s: convert as86 syntax to GNU as (.code16) | Line-by-line translation, original comments; keeps 8042 A20 and lmsw (oldlinux switched to port 0x92 and cr0, and added lab debug prints). |
| 2.10 | boot/setup.s: print "Now we are in setup ..." | New: print_msg helper (BIOS int 0x10 AH=13h, es:bp + cx) and a first message when setup starts. |
| 2.11 | boot/setup.s: print "Entering protected mode ..." | New: last BIOS message, printed before cli and before the system is moved over the interrupt vectors; then the cursor position at 0x90000 is saved again so the kernel console starts below it. |
| 2.12 | Makefile: assemble setup with as + ld + objcopy -O binary | Same recipe as the boot sector; boot/setup is a raw binary. |
| 2.13 | Makefile: add setup to bootimage | `bootimage` = bootsect + setup in sectors 2-5 (dd). Boots to "Entering protected mode ...", then resets (no system at address 0 yet). |
