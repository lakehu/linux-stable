# gdb-dashboard template: 32-bit x86 kernel in QEMU (e.g. Linux 0.11)
# Loaded automatically by .gdbinit when gdb starts in this directory;
# by hand: gdb -x x86-qemu.gdb tools/system
# Needs gdb-dashboard (https://github.com/cyrus-and/gdb-dashboard) loaded first.

set pagination off

# compact screen: source, disassembly, the interesting registers, call stack
dashboard -layout source assembly registers stack
dashboard source -style height 8
dashboard assembly -style height 6
# general, segment and control registers only (no xmm0-7, fs_base, k_gs_base, efer ...)
dashboard registers -style list 'eax ebx ecx edx esi edi ebp esp eip eflags cs ds ss cr0 cr3'
dashboard stack -style limit 3
