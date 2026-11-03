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
