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
| **T2** | test: setup enters protected mode and jumps to 32-bit code | TEMPORARY: boot/head.s replaced by a 32-bit stub (writes "PM OK" to VGA memory and loops); bootimage gets a system made of head.o. `make run` shows the 4 boot messages and "PM OK", no reset (passed). Reverted in 4.3. |
| 2.14 | Makefile, .vscode: debug in QEMU with gdb or VS Code | `make debug` = `make run` + `-S -gdb tcp::41234` (CPU stopped, gdb server on port 41234). .vscode/launch.json (cppdbg, gdb, symbols from the linked kernel) + tasks.json (kill qemu, build, run = make debug), in the style of the lts5.10 setup. AS = as --32 -g (head.s with source lines; images unchanged); C gets -g in 3.2, 3.5-3.8. Session stops at startup_32 (hbreak after connect). Checked with gdb: startup_32 () at boot/head.s:13, cs=8, PE. |
| 2.15 | Makefile: run QEMU with -no-reboot -no-shutdown | On a triple fault QEMU pauses and keeps the screen, instead of rebooting in a loop (screen flashing). |
| 2.16 | tools/build.c: accept raw system binary | Makefile turns tools/system (ELF) into a raw binary with objcopy; build.c no longer skips/checks a 1024-byte a.out header. |
| 2.17 | Makefile: default root device without host /dev/hd6 | `ROOT_DEV=` (empty): build.c then uses its default (3,6) = /dev/hd6; modern hosts have no /dev/hd6 to stat. |

### Phase 3: Compiler flags and sub-Makefiles

| Step | Commit | Notes |
|---|---|---|
| 3.1 | Makefile: drop Linus-only gcc flags | Remove -fcombine-regs and -mstring-insns (Linus's patched gcc 1.x; modern gcc rejects them). |
| 3.2 | Makefile: build 32-bit freestanding code | Add -m32 (host gcc is 64-bit), -fno-builtin (strcpy/memcpy etc. not treated as gcc builtins), -fno-stack-protector (no __stack_chk_fail), -std=gnu89 (1991 C rules, e.g. extern inline), -g (line information for gdb/VS Code; raw images unchanged). |
| 3.3 | Makefile: keep a.out symbol names (-fleading-underscore) | C symbols get a leading underscore as in 1991 a.out (main -> _main), so the assembly (_main, _printk, _stack_start, ...) stays unchanged. |
| 3.4 | Makefile: no position-independent code (-fno-pie) | Ubuntu gcc defaults to PIE: GOT and get_pc_thunk, and with -fleading-underscore an unresolvable __GLOBAL_OFFSET_TABLE_. The kernel is linked at fixed address 0. |
| 3.5 | kernel/Makefile: use the host toolchain and modern gcc flags | gas/gld/gar -> as/ld/ar (as with -g), same CFLAGS changes as the top Makefile (incl. -g). |
| 3.6 | kernel/{blk_drv,chr_drv,math}/Makefile: use the host toolchain and modern gcc flags | Same change for the driver and math emulation Makefiles (archives built with ar). |
| 3.7 | fs/Makefile: use the host toolchain and modern gcc flags | Same change for the file system Makefile. |
| 3.8 | mm/Makefile, lib/Makefile: use the host toolchain and modern gcc flags | Same change for the memory management and library Makefiles. |

### Phase 4: 32-bit assembly fixes

| Step | Commit | Notes |
|---|---|---|
| 4.1 | init/main.c: temporary minimal main() for testing head.s | TEMPORARY: `#define MINIMAL_MAIN` block in front of the untouched original: main() prints "Hello from main()", tiny VGA printk (no %), stack_start. Not linked yet. Removed once the whole kernel builds. |
| 4.2 | Makefile: link init/main.o into the bootimage system | System = head.o + main.o. With the T2 stub still in head.s, `make run` still shows "PM OK". |
| 4.3 | boot/head.s: restore the original (revert the T2 stub) | Back to Linus's head.s. Does not assemble until 4.4 (.align 3). |
| 4.4 | head.s: .align in bytes for ELF | `.align 3` (a.out: 2^3 = 8 bytes) is an error in ELF i386 as, where the number is a byte count -> `.align 8` before _idt/_gdt. The other `.align 2` still assemble and are left alone. `make run` shows "Hello from main()". |
| 4.5 | keyboard.S: xorl %al,%al -> xorl %eax,%eax | Modern gas rejects the size mismatch. 1991 gas took the size from the 'l' suffix and the register number from %al, i.e. it assembled 'xorl %eax,%eax' - needed, as %eax is used as a 32-bit index. The 'indirect call without *' warning is left alone. (oldlinux-0.11: 'xor %al,%al', clears only %al.) |
| **T3** | test: head.s sets up protected mode and paging and calls C code | `make run` (bootimage = bootsect + setup + head.s + minimal main.c): the 4 boot messages, then "Hello from main()" on its own line; no reset (passed). QEMU monitor: GDT 0x5cb8 (code/data 16 MB, base 0), IDT 0x54b8 (all ignore_int), CR0=0x80000013 (PE+PG), CR3=0, first 16 MB identity mapped, EIP in main(); DF still set by setup_paging (fixed in 5.12). |

### Phase 5: C / inline-asm compile fixes

| Step | Commit | Notes |
|---|---|---|
| 5.1 | sched.h: _set_base/_set_limit don't clobber input %edx | Input "d" was also in the clobber list (impossible constraint for modern gcc). Save/restore %edx inside the asm instead (as oldlinux-0.11). |
| 5.2 | string.h: input registers changed by the asm become outputs | Modern gcc rejects inputs that are also clobbered. Changed input regs become early-clobber dummy outputs tied to the inputs (numbered refs to them -> register names, g operands renumbered), __volatile__ where there are no real outputs, "memory" clobber. strtok unchanged. Checked in user space with -O2. (oldlinux-0.11 just drops the clobbers.) |
