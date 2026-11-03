#
# if you want the ram-disk device, define this to be the
# size in blocks.
#
RAMDISK = #-DRAMDISK=512

AS86	=as86 -0 -a
LD86	=ld86 -0

AS	=as --32 -g
LD	=ld
LDFLAGS	=-m elf_i386 -Ttext 0 -e startup_32 -M
CC	=gcc $(RAMDISK)
CFLAGS	=-Wall -O -fstrength-reduce -fomit-frame-pointer \
-m32 -fno-builtin -fno-stack-protector -std=gnu89 -fleading-underscore -fno-pie -g
CPP	=cpp -nostdinc -Iinclude

#
# ROOT_DEV specifies the default root-device when making the image.
# This can be either FLOPPY, /dev/xxxx or empty, in which case the
# default of /dev/hd6 is used by 'build'.
#
ROOT_DEV=

ARCHIVES=kernel/kernel.o mm/mm.o fs/fs.o
DRIVERS =kernel/blk_drv/blk_drv.a kernel/chr_drv/chr_drv.a
MATH	=kernel/math/math.a
LIBS	=lib/lib.a

.c.s:
	$(CC) $(CFLAGS) \
	-nostdinc -Iinclude -S -o $*.s $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-nostdinc -Iinclude -c -o $*.o $<

all:	Image

# 1.44MB floppy: 80 tracks * 2 heads * 18 sectors * 512 bytes
FLOPPY_DISK_SIZE = 1474560

# Boot the image in QEMU, text screen in the terminal.
# QEMU monitor (registers, memory, quit): in a second terminal run
#   nc -U qemu-monitor.sock
# and type e.g. 'info registers' or 'quit'.
# -no-reboot -no-shutdown: on a triple fault, stop (keeping the screen)
# instead of rebooting in a loop.
run: Image
	qemu-system-i386 -boot a -drive format=raw,file=Image,if=floppy -display curses -no-reboot -no-shutdown -monitor unix:qemu-monitor.sock,server,nowait

# Like 'run', but for a debugger: QEMU starts its gdb server on TCP port 41234
# (-gdb tcp::41234) and stops the CPU before the first instruction (-S). Connect with
#   gdb -ex 'target remote :41234' tools/system
# or with VS Code (.vscode/launch.json, "Run and Debug": Linux 0.11).
debug: Image
	qemu-system-i386 -boot a -drive format=raw,file=Image,if=floppy -display curses -no-reboot -no-shutdown -monitor unix:qemu-monitor.sock,server,nowait -S -gdb tcp::41234

Image: boot/bootsect boot/setup tools/system tools/build
	objcopy -O binary -R .note -R .comment tools/system tools/kernel
	tools/build boot/bootsect boot/setup tools/kernel $(ROOT_DEV) > Image
	rm -f tools/kernel
# Not needed yet: QEMU boots the image as it is. Enable if a full 1.44MB
# floppy image is needed.
#	truncate -s $(FLOPPY_DISK_SIZE) Image
	sync

disk: Image
	dd bs=8192 if=Image of=/dev/PS0

# First user program for the tiny root file system (an a.out ZMAGIC file)
tools/hello: tools/hello.s tools/mkaout.py
	$(AS) -o tools/hello.o tools/hello.s
	$(LD) -m elf_i386 -Ttext 0 -e _start -o tools/hello.elf tools/hello.o
	objcopy -O binary tools/hello.elf tools/hello.bin
	python3 tools/mkaout.py tools/hello.bin tools/hello
	rm -f tools/hello.o tools/hello.elf tools/hello.bin

tools/build: tools/build.c
	gcc -o tools/build tools/build.c

boot/head.o: boot/head.s

tools/system:	boot/head.o init/main.o \
		$(ARCHIVES) $(DRIVERS) $(MATH) $(LIBS)
	$(LD) $(LDFLAGS) boot/head.o init/main.o \
	$(ARCHIVES) \
	$(DRIVERS) \
	$(MATH) \
	$(LIBS) \
	-o tools/system > System.map

kernel/math/math.a:
	(cd kernel/math; make)

kernel/blk_drv/blk_drv.a:
	(cd kernel/blk_drv; make)

kernel/chr_drv/chr_drv.a:
	(cd kernel/chr_drv; make)

kernel/kernel.o:
	(cd kernel; make)

mm/mm.o:
	(cd mm; make)

fs/fs.o:
	(cd fs; make)

lib/lib.a:
	(cd lib; make)

boot/setup: boot/setup.s
	$(AS) -o boot/setup.o boot/setup.s
	$(LD) -m elf_i386 -Ttext 0 -e start -o boot/setup.elf boot/setup.o
	objcopy -O binary -j .text boot/setup.elf boot/setup
	rm -f boot/setup.elf

boot/bootsect:	boot/bootsect.s
	$(AS) -o boot/bootsect.o boot/bootsect.s
	$(LD) -m elf_i386 -Ttext 0 -e start -o boot/bootsect.elf boot/bootsect.o
	objcopy -O binary -j .text boot/bootsect.elf boot/bootsect
	rm -f boot/bootsect.elf

tmp.s:	boot/bootsect.s tools/system
	(echo -n "SYSSIZE = (";ls -l tools/system | grep system \
		| cut -c25-31 | tr '\012' ' '; echo "+ 15 ) / 16") > tmp.s
	cat boot/bootsect.s >> tmp.s

clean:
	rm -f Image System.map tmp_make core boot/bootsect boot/setup
	rm -f init/*.o tools/system tools/build tools/hello boot/*.o
	(cd mm;make clean)
	(cd fs;make clean)
	(cd kernel;make clean)
	(cd lib;make clean)

backup: clean
	(cd .. ; tar cf - linux | compress - > backup.Z)
	sync

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in init/*.c;do echo -n "init/";$(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile
	(cd fs; make dep)
	(cd kernel; make dep)
	(cd mm; make dep)

### Dependencies:
init/main.o : init/main.c include/unistd.h include/sys/stat.h \
  include/sys/types.h include/sys/times.h include/sys/utsname.h \
  include/utime.h include/time.h include/linux/tty.h include/termios.h \
  include/linux/sched.h include/linux/head.h include/linux/fs.h \
  include/linux/mm.h include/signal.h include/asm/system.h include/asm/io.h \
  include/stddef.h include/stdarg.h include/fcntl.h 
