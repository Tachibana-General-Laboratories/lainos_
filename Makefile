# Cross-compiling (e.g., on Mac OS X)
#TOOLPREFIX = i386-jos-elf-

# Using native tools (e.g., on X86 Linux)
#TOOLPREFIX =

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
$(warning TOOLPREFIX environment variable is not set. Compilation may fail.)
endif

# If the makefile can't find QEMU, specify its path here
#QEMU =

# Try to infer the correct QEMU
ifndef QEMU
QEMU = $(shell if which qemu > /dev/null; \
	then echo qemu; exit; \
	else \
	qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
	if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "*** or have you tried setting the QEMU variable in Makefile?" 1>&2; \
	echo "***" 1>&2; exit 1)
endif

export ROOT = $(shell pwd)
export KERNEL = $(ROOT)/kernel
INC = -I$(KERNEL) -I$(KERNEL)/include \
	  -I$(KERNEL)/fs -I$(KERNEL)/hardware \
	  -I$(KERNEL)/syscall -I$(KERNEL)/proc \
	  -I$(KERNEL)/lock \
	  -I$(ROOT)/userspace/lib/ulib

export CC = $(TOOLPREFIX)gcc
export AS = $(TOOLPREFIX)gas
export LD = $(TOOLPREFIX)ld
export OBJCOPY = $(TOOLPREFIX)objcopy
export OBJDUMP = $(TOOLPREFIX)objdump
#CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 -Wall -MD -ggdb -m32 -Werror -fno-omit-frame-pointer
export CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -MD -ggdb -m32 -Werror -fno-omit-frame-pointer
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
CFLAGS += $(INC)
export ASFLAGS = -m32 -gdwarf-2 -Wa,-divide
ASFLAGS += $(INC)
# FreeBSD ld wants ``elf_i386_fbsd''
export LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null)

xv6.img: kernel/bootblock kernel/kernel fs.img
	dd if=/dev/zero of=xv6.img count=10000
	dd if=kernel/bootblock of=xv6.img conv=notrunc
	dd if=kernel/kernel of=xv6.img seek=1 conv=notrunc

kernel/bootblock:
	$(MAKE) -C kernel bootblock

kernel/kernel:
	$(MAKE) -C kernel kernel

tags: $(OBJS) entryother.S _init
	etags *.S *.c

ULIB = userspace/lib/ulib/ulib.o userspace/lib/ulib/usys.o userspace/lib/ulib/printf.o userspace/lib/ulib/umalloc.o

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

userspace/tests/_forktest: userspace/tests/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o userspace/tests/_forktest \
		userspace/tests/forktest.o userspace/lib/ulib/ulib.o userspace/lib/ulib/usys.o
	$(OBJDUMP) -S userspace/tests/_forktest > userspace/tests/forktest.asm

mkfs: mkfs.c kernel/fs/fs.h
	gcc -Ikernel -Werror -Wall -o mkfs mkfs.c

mkfs2: mkfs2.c kernel/fs/fs.h
	gcc -Ikernel -Werror -Wall -o mkfs2 mkfs2.c

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

UPROGS=\
	userspace/core/_cat\
	userspace/core/_echo\
	userspace/tests/_forktest\
	userspace/core/_grep\
	userspace/core/_init\
	userspace/core/_kill\
	userspace/core/_ln\
	userspace/core/_ls\
	userspace/core/_mkdir\
	userspace/core/_rm\
	userspace/core/_sh\
	userspace/tests/_stressfs\
	userspace/tests/_usertests\
	userspace/core/_wc\
	userspace/core/_zombie\

fs.img: mkfs2 README.md $(UPROGS)
	mkdir -p fs
	cp README.md fs
	cp $(UPROGS) fs
	./mkfs2 fs.img fs

-include *.d

clean:
	$(MAKE) -C kernel clean
	find -name "*.d" -delete
	find -name "*.o" -delete
	find -name "*.asm" -delete
	find -name "*.sym" -delete
	rm -f */*.tex */*.dvi */*.idx */*.aux */*.log */*.ind */*.ilg \
	*/*.o */*.d */*.asm */*.sym syscall/vectors.S bootblock entryother \
	*.tex *.dvi *.idx *.aux *.log *.ind *.ilg *.o *.d *.asm *.sym \
	initcode initcode.out xv6.img fs.img kernelmemfs mkfs mkfs2 \
	.gdbinit \
	$(UPROGS)

# make a printout
FILES = $(shell grep -v '^\#' runoff.list)
PRINT = runoff.list runoff.spec README toc.hdr toc.ftr $(FILES)

xv6.pdf: $(PRINT)
	./runoff
	ls -l xv6.pdf

print: xv6.pdf

# run in emulators

bochs : fs.img xv6.img
	if [ ! -e .bochsrc ]; then ln -s dot-bochsrc .bochsrc; fi
	bochs -q

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 1
endif
QEMUOPTS = -hdb fs.img xv6.img -smp $(CPUS) -m 512 $(QEMUEXTRA)

qemu: fs.img xv6.img
	$(QEMU) -serial mon:stdio $(QEMUOPTS)

qemu-memfs: xv6memfs.img
	$(QEMU) xv6memfs.img -smp $(CPUS)

qemu-nox: fs.img xv6.img
	$(QEMU) -nographic $(QEMUOPTS)

.gdbinit: .gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

qemu-gdb: fs.img xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -serial mon:stdio $(QEMUOPTS) -S $(QEMUGDB)

qemu-nox-gdb: fs.img xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) -nographic $(QEMUOPTS) -S $(QEMUGDB)

.PHONY: dist-test dist
