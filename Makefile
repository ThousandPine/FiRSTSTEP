TARGET = bootsect.bin setup.bin kernel
LD = ld
CC = gcc
CFLAGS = -m32 -nostdlib -nostartfiles -fno-builtin -fno-pie -fno-pic
IMG_NAME = disk.img
IMG_SIZE = 16

all: $(TARGET)

bootsect.bin: bootsect.S
	$(CC) $(CFLAGS) -Ttext=0x7C00 -o $@ $<
	objcopy -S -O binary -j .text $@ $@

setup.o: setup.S
	$(CC) $(CFLAGS) -c -o $@ $<

setupmain.o: setupmain.c
	$(CC) $(CFLAGS) -c -o $@ $<

setup.bin: setup.o setupmain.o
	$(LD) -m elf_i386 -T link.ld -o $@ $^
	objcopy -S -O binary $@ $@

kernel: init.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(IMG_NAME):
	dd if=/dev/zero of=$(IMG_NAME) bs=1M count=$(IMG_SIZE)
	parted -s $(IMG_NAME) mklabel msdos mkpart primary fat16 1MiB 100%
	parted -s $(IMG_NAME) set 1 boot on
	mkfs.fat -F 16 --offset=2048 $(IMG_NAME)

install: $(TARGET) $(IMG_NAME)
	./write_bootsect.sh bootsect.bin $(IMG_NAME)
	./write_setup.sh setup.bin $(IMG_NAME)
	./install_kernel.sh kernel $(IMG_NAME)

qemu:
	qemu-system-i386 -m 1G -nographic -drive format=raw,file=$(IMG_NAME)

bochs:
	bochs -f bochsrc.cfg -q

clean:
	rm -f $(TARGET) *.o
