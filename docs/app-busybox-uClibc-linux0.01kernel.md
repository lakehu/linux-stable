# Running a modern static BusyBox (uClibc-ng) on the Linux 0.01 ELF kernel

Status of 2026-11-05. Kernel: the **prebuilt `Image`** from mariuz/linux-0.01 (Abdel Benamrouche's 2007 port,
with an ELF loader and a Linux-numbered syscall table). It hasn't been rebuilt.
Userland: **BusyBox 1.37.0**, static, built with **gcc 13.3.0 + uClibc-ng 1.0.59** by Buildroot 2025.02.

## 1. Goal

Run small static i386 programs built with today's tools on 0.01, with BusyBox as the target. We don't need the
old 0.01 hard-disk rootimage or a Buildroot rootfs. All we need is one `busybox` binary, which is copied into
the existing MINIX disk image `hd_oldlinux.img`.

## 2. What the 0.01 ELF kernel can and cannot do

| Property | Consequence for user programs |
|---|---|
| ELF loader copies each `SHT_PROGBITS` section to `sh_addr` (sections with `sh_addr==0` are skipped), creates `SHT_NOBITS` as bss | program must be **static** and linked at **low addresses** |
| each task has a 64 MB linear window (`base = nr * 64MB`) | everything (text, data, bss, stack) must be below 0x4000000; the default i386 base 0x08048000 (128 MB) is unusable |
| `sys_call_table` has 320 entries in Linux numbering; `nr_system_calls = 319`; above that → `bad_sys_call` returns -1 (errno 1, **EPERM**) | syscalls ≥ 319 (statx 383, clock_gettime64 403, …) appear as EPERM |
| unimplemented numbers inside the table → `sys_null` → **-ENOSYS** | e.g. `set_thread_area` (243), `mmap2`, `exit_group` |
| implemented: 0–66 (classic 0.11 set), `stat/lstat/fstat` 106–108, `uname` 122, `getdents` 141, `stat64/lstat64/fstat64` 195–197, `getdents64` 220, `fcntl64` 221 | that's enough for simple file utilities |
| no `set_thread_area`, no `%gs` TLS segment | any libc that uses TLS (`%gs:0x0`, `%gs:0x14` stack guard) crashes |
| `execve` doesn't clear registers (`%edx` keeps a stale value) | breaks libc start code that trusts `%edx` (see 5.4) |

The 2008 binaries in `hd_oldlinux.img` (bash 3.2, coreutils; gcc 3.3.6 + dietlibc, linked at 0) have
no `%gs` uses, which is why they work.

## 3. Why uClibc-ng

Tried / considered on the Ubuntu 20.04 host:

| libc | Result |
|---|---|
| none (`-nostdlib`, raw `int $0x80`) | **works** (hello-nolibc, `-Wl,-Ttext=0x100`), but useless for BusyBox |
| glibc static (Ubuntu `gcc -m32 -static`) | **segfault**: calls `set_thread_area` (243 → ENOSYS), then uses `%gs:…` for TLS / stack protector |
| dietlibc 0.34 (Ubuntu `dietlibc-dev` i386) | **segfault**: same TLS use via `%gs` |
| musl | Ubuntu packages are amd64 only; musl always sets up TLS (`set_thread_area`) on i386 anyway |
| newlib | Ubuntu has only ARM bare-metal / headers; it has no Linux i386 syscall layer |
| BusyBox built-in libc | doesn't exist (`libbb` is a helper library on top of a libc) |
| **uClibc-ng with `BR2_PTHREADS_NONE`** | **chosen**: without threads it has no TLS and no `%gs`; it's small, supports static linking, and is still maintained (1.0.51 → 1.0.59 during the 2025.02 LTS) |

**Buildroot over Yocto:** Yocto dropped uClibc, and newlib is only for bare metal there. Buildroot builds a
uClibc-ng cross toolchain plus BusyBox in one step, and `dl/` already had every tarball.

## 4. Buildroot setup (what was changed)

* Tree: `/DATA2/uie75906/lfs/buildroot`, branch `mylfs_2025.02` (b81b330a83). **No file in the git tree is
  modified.**
* Separate output dir: `O=/DATA2/uie75906/lfs/buildroot/output-linux001` (covered by `.gitignore` `/output*`).
  The existing x86_64/glibc `output/` and the top-level `.config` stay untouched, because switching arch and libc
  in place would need `make clean`.
* Downloads: the shared `dl/` (symlink → `../2022-2026-buildroot`). Nothing is downloaded and nothing is deleted.

Files in `output-linux001/`:

| File | Purpose |
|---|---|
| `linux001_defconfig` | the Buildroot defconfig below |
| `mk.sh` | wrapper: `make O=… UCLIBC_VERSION=1.0.59 "$@"` |
| `patches/uclibc/1.0.59/uclibc.hash` | sha256 of `uClibc-ng-1.0.59.tar.xz` (same as upstream `origin/2025.02.x`) |
| `patches/uclibc/1.0.59/0001-i386-crt1-ignore-edx-for-static.patch` | `%edx` fix, see 5.4 |
| `uclibc-linux001.fragment` | `# UCLIBC_USE_TIME64 is not set`, see 5.5 |
| `busybox-linux001.config` | minimal BusyBox config |
| `build.log`, `bb*.log` | build logs |

`linux001_defconfig`:
```
BR2_x86_i486=y
BR2_TOOLCHAIN_BUILDROOT_UCLIBC=y
BR2_KERNEL_HEADERS_VERSION=y
BR2_DEFAULT_KERNEL_VERSION="6.12.9"
BR2_PACKAGE_HOST_LINUX_HEADERS_CUSTOM_6_12=y
BR2_PTHREADS_NONE=y
BR2_STATIC_LIBS=y
BR2_INIT_NONE=y
BR2_SYSTEM_BIN_BUSYBOX=y
BR2_PACKAGE_BUSYBOX_CONFIG="/DATA2/uie75906/lfs/buildroot/output-linux001/busybox-linux001.config"
# BR2_TARGET_ROOTFS_TAR is not set
BR2_GLOBAL_PATCH_DIR="/DATA2/uie75906/lfs/buildroot/output-linux001/patches"
BR2_UCLIBC_CONFIG_FRAGMENT_FILES="/DATA2/uie75906/lfs/buildroot/output-linux001/uclibc-linux001.fragment"
```
No kernel, no host QEMU and no rootfs image, compared with `configs/qemu_x86_defconfig`. Kernel headers 6.12.9
come from the `dl/linux/linux-6.12.9.tar.xz` that's already there. Resulting toolchain:
`i486-buildroot-linux-uclibc`, binutils 2.43.1, gcc 13.3.0.

BusyBox config: made with `allnoconfig` on the 1.37.0 tarball, plus `STATIC`, hush (`SH_IS_HUSH`, if/loops/case/
functions/echo/printf/test/export) and these applets: basename cat cp dd dirname echo expr false head ln ls mkdir
mv printf pwd rm seq sleep sort sync tail test touch true uname uniq wc yes. Plus:
```
CONFIG_EXTRA_LDFLAGS="-Wl,-Ttext-segment=0 -Wl,--build-id=none"
```

Build and rebuild:
```bash
O=/DATA2/uie75906/lfs/buildroot/output-linux001
cd /DATA2/uie75906/lfs/buildroot
env -u LD_LIBRARY_PATH make O=$O BR2_DEFCONFIG=$O/linux001_defconfig defconfig
env -u LD_LIBRARY_PATH $O/mk.sh                       # full build (toolchain + busybox)
env -u LD_LIBRARY_PATH $O/mk.sh uclibc-dirclean uclibc   # after changing uClibc patch/fragment
env -u LD_LIBRARY_PATH $O/mk.sh busybox-dirclean busybox # after changing busybox config
# result: $O/build/busybox-1.37.0/busybox  (unstripped, 204 KB; stripped 152 KB)
```

## 5. Problems met and fixes

### 5.1 Buildroot refuses to start: `LD_LIBRARY_PATH` contains cwd
`LD_LIBRARY_PATH=/home/uie75906/.local/lib:/home/uie75906/usr/lib:` ends with `:`, which counts as the current
directory. **Fix:** run make with `env -u LD_LIBRARY_PATH` (only for the build; the login environment
stays as it is).

### 5.2 `ERROR: No hash found for uClibc-ng-1.0.59.tar.xz`
2025.02 pins uClibc-ng **1.0.51**, which isn't in `dl/`. `dl/uclibc/` has 1.0.37, 1.0.39 and **1.0.59**. Upstream
bumped the 2025.02.x LTS branch to 1.0.59 (commit 0565093a1a, 2026-08-08). `mylfs_2025.02` was branched from the
original 2025.02 tag, so it doesn't have that bump. Buildroot checks hashes even when
`BR2_DOWNLOAD_FORCE_CHECK_HASHES` is off. **Fix:** `UCLIBC_VERSION=1.0.59` on the make command line (`mk.sh`) +
a hash file in `BR2_GLOBAL_PATCH_DIR/uclibc/1.0.59/uclibc.hash` (identical to upstream's hash).
*Cleaner long-term:* rebase `mylfs_2025.02` onto `origin/2025.02.x` (or cherry-pick 0565093a1a); then
both workarounds go away.

### 5.3 Sections outside the 64 MB window
The first attempt used `-Wl,-Ttext=0x100` (as for hello-nolibc). That only moves `.text`. `.init` stayed at
0x08049000 and `.note.gnu.property` at 0x08048154. **Fix:** `-Wl,-Ttext-segment=0` moves the whole image:
headers at 0, `.init` 0x1000, `.text` 0x1020, `.rodata` 0x1e000, `.data/.bss` end ≈ 0x28c30.
(Side effect: the binary no longer runs on the host, because Linux refuses to map page 0 (`vm.mmap_min_addr`).
mariuz's binaries behave the same way.)

### 5.4 Every command segfaults at exit
Output was printed, then `Segmentation fault`. Found with `qemu -d int` / `-d exec,nochain -dfilter`: after
`write()`, `exit()` → `__uClibc_fini()` → `if (__rtld_fini) __rtld_fini();` jumped to 0x2fde5 and ran through
zeroed memory. `_start` (crt1.S) stores **`%edx` at process entry** as `rtld_fini`. The i386 ELF ABI says the
kernel puts a function pointer there (0 for static programs; Linux clears it in `ELF_PLAT_INIT`). 0.01's
`execve` leaves `%edx` with a stale value. **Fix:** uClibc patch
`0001-i386-crt1-ignore-edx-for-static.patch`, `xorl %edx,%edx` before `pushl %edx` in
`libc/sysdeps/linux/i386/crt1.S`. That's only acceptable because this toolchain is static-only. A kernel-side
fix would be to zero the user registers in `do_execve`.

### 5.5 `ls: /bin: Operation not permitted`
Syscall trace showed **383 `statx`** and **403 `clock_gettime64`**. Buildroot 2025.02 builds uClibc-ng with
`UCLIBC_USE_TIME64=y` (it only disables it for headers < 5.1), and time64 uses the new syscalls. They're
above `nr_system_calls`, so the kernel returns -1 = EPERM. **Fix:** uClibc config fragment
`# UCLIBC_USE_TIME64 is not set`. After that uClibc uses `stat64` (195), `fstat64` (197), `getdents64` (220) and
`time` (13), which mariuz implements. (32-bit `time_t` is irrelevant for 0.01.)

## 6. Test

Test scripts are in `/run/user/500286754/oldlinux-test/`:
* `minixadd.py IMG OFFSET DIR NAME HOSTFILE`: adds a file to a MINIX v1 fs (7 direct + 1 indirect zone,
  max 519 KB); partition 1 of `hd_oldlinux.img` is at byte offset 10240.
* `bbtype.sh OUT "cmd" …`: boots QEMU headless, types the commands via monitor `sendkey`, dumps VGA text
  memory. `GAP=` sets the seconds per command; `QEXTRA="-d int -D log"` logs every `int 0x80`/exception, and
  `intparse.py log` prints the syscalls/faults of the last `execve`.

```bash
cd /run/user/500286754/oldlinux-test
O=/DATA2/uie75906/lfs/buildroot/output-linux001
$O/host/bin/i486-buildroot-linux-uclibc-strip -o bb-run/busybox $O/build/busybox-1.37.0/busybox
cp mariuz-run/hd_oldlinux.img bb-run/hd_bb.img          # always start from a clean copy
python3 -I minixadd.py bb-run/hd_bb.img 10240 /bin busybox bb-run/busybox
qemu-system-i386 -drive format=raw,file=bb-run/Image,index=0,if=floppy -boot a \
    -hdb bb-run/hd_bb.img -m 8 -machine pc-i440fx-2.5 -display curses
```

Result (screen of the 0.01 kernel):
```
# /bin/busybox echo hi; echo rc=$?
hi
rc=0
# /bin/busybox uname -a
linux 0.01-3.x nodename release  3.x i386
# /bin/busybox ls -l /bin/busybox
-rwxr-xr-x    1 0        0           152560 Jan  1  1970 /bin/busybox
# /bin/busybox seq 3
1
2
3
# /bin/busybox ls /bin
Segmentation fault
```
Checks on the binary: `ELF 32-bit LSB executable, Intel 80386, statically linked`; `objdump -d | grep -c %gs`
= 0; no `GLIBC_` strings; `.comment` = `GCC: (Buildroot 2025.02-1-gb81b330a83) 13.3.0`.

## 7. Known issues (not fixed)

1. **Directory listing crashes (`ls DIR`, any `readdir`).** mariuz's `sys_getdents64` (`fs/sys_getdents.c`,
   `minix_getdents64`) is not the Linux ABI: it returns **one entry per call**, the **return value is the name
   length**, and **`d_reclen` = name length** (`d_off`/`d_type` aren't set). Linux returns the number of bytes
   filled, with real record lengths. uClibc's readdir computes `d_reclen - offsetof(d_name)` (negative), so
   `memcpy` runs off the 64 MB window and segfaults. `sys_getdents` (141) has the same design.
   Options: (a) a uClibc readdir patch for this behaviour (keeps the prebuilt Image); (b) fix
   `minix_getdents64` in the kernel, which needs building mariuz's kernel (as86 or a GAS conversion of its boot code).
2. **No applet names.** Commands must be typed as `/bin/busybox ls`. 0.01 has `link()` but no symlinks;
   hard links (e.g. `/bin/bb-ls`) could be added to the image (`minixadd.py` has no link support yet).
3. **Not tested yet:** hush as an interactive shell (`busybox sh`; needs `fork`/`execve`/`waitpid`/signals and
   tty ioctls), `cp/mv/rm/mkdir`, `sort`, `dd`, and pipes/redirection inside hush.
4. **Syscalls that return ENOSYS/EPERM on 0.01** and may affect other applets: `mmap/mmap2` (uClibc malloc
   uses `brk`, which works), `exit_group`, `rt_sigaction` family, `getcwd`, `readlink`, `clock_gettime`,
   `set_thread_area` (TLS libcs can't work at all).
5. **Time:** `ls -l` shows `Jan 1 1970`. The 0.01 MINIX inode times / `time()` path wasn't investigated.
6. The `%edx` and TIME64 workarounds live only in `output-linux001/` (not committed anywhere). The
   uClibc 1.0.59 override disappears once `mylfs_2025.02` follows `origin/2025.02.x`.
