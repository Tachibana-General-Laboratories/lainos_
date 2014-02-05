OBJS = \
	fs/bio.o\
	fs/exec.o\
	fs/file.o\
	fs/fs.o\
	fs/ide.o\
	fs/log.o\
	fs/sysfile.o\
	hardware/console.o\
	hardware/ioapic.o\
	hardware/kbd.o\
	hardware/lapic.o\
	hardware/mp.o\
	hardware/picirq.o\
	hardware/timer.o\
	hardware/uart.o\
	proc/kalloc.o\
	proc/proc.o\
	proc/swtch.o\
	proc/vm.o\
	syscall/syscall.o\
	syscall/sysproc.o\
	syscall/trapasm.o\
	syscall/trap.o\
	syscall/vectors.o\
	lock/spinlock.o\
	pipe.o\
	string.o\
	main.o\

bootblock: bootasm.S bootmain.c
	$(CC) $(CFLAGS) -fno-pic -O -nostdinc -I. -c bootmain.c
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c bootasm.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o bootblock.o bootasm.o bootmain.o
	$(OBJDUMP) -S bootblock.o > bootblock.asm
	$(OBJCOPY) -S -O binary -j .text bootblock.o bootblock
	./sign.pl bootblock

entryother: entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c entryother.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o bootblockother.o entryother.o
	$(OBJCOPY) -S -O binary -j .text bootblockother.o entryother
	$(OBJDUMP) -S bootblockother.o > entryother.asm

initcode: initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c initcode.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm

kernel: $(OBJS) entry.o entryother initcode kernel.ld
	$(LD) $(LDFLAGS) -T kernel.ld -o kernel entry.o $(OBJS) -b binary initcode entryother
	$(OBJDUMP) -S kernel > kernel.asm
	$(OBJDUMP) -t kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym

MEMFSOBJS = $(filter-out fs/ide.o,$(OBJS)) memide.o
kernelmemfs: $(MEMFSOBJS) entry.o entryother initcode kernel.ld ../fs.img
	rm -f fs.img
	cp ../fs.img fs.img
	$(LD) $(LDFLAGS) -T kernel.ld -o kernelmemfs entry.o $(MEMFSOBJS) -b binary initcode entryother fs.img
	rm -f fs.img
	$(OBJDUMP) -S kernelmemfs > kernelmemfs.asm
	$(OBJDUMP) -t kernelmemfs | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernelmemfs.sym

tags: $(OBJS) entryother.S _init
	etags *.S *.c

syscall/vectors.S: syscall/vectors.pl
	perl syscall/vectors.pl > syscall/vectors.S

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

-include *.d

clean:
	find -name "*.d" -delete
	find -name "*.o" -delete
	find -name "*.asm" -delete
	find -name "*.sym" -delete
	rm -f */*.tex */*.dvi */*.idx */*.aux */*.log */*.ind */*.ilg \
	*/*.o */*.d */*.asm */*.sym syscall/vectors.S bootblock entryother \
	*.tex *.dvi *.idx *.aux *.log *.ind *.ilg *.o *.d *.asm *.sym \
	initcode initcode.out kernel xv6.img fs.img kernelmemfs mkfs \
	.gdbinit \
	$(UPROGS)

