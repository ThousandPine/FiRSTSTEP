TARGET = boot/bootsect.bin boot/setup.bin kernel/kernel
LD = ld
CC = gcc
CFLAGS = -I ./inc/ -m32 -nostdlib -nostartfiles -fno-builtin -fno-stack-protector -fno-pie -fno-pic -march=i386
IMG_NAME = disk.img
IMG_SIZE = 16

all: $(TARGET)

boot/bootsect.bin: boot/bootsect.S
	$(CC) $(CFLAGS) -Ttext=0x7C00 -o $@ $<
	objcopy -S -O binary -j .text $@ $@

boot/setup.o: boot/setup.S
	$(CC) $(CFLAGS) -c -o $@ $<

boot/setupmain.o: boot/setupmain.c
	$(CC) $(CFLAGS) -c -o $@ $<

boot/setup.bin: boot/setup.o boot/setupmain.o
	$(LD) -m elf_i386 -T boot/link.ld -o $@ $^
	objcopy -S -O binary $@ $@

kernel/kernel: kernel/init.c
	$(CC) $(CFLAGS) -o $@ $<

$(IMG_NAME):
	dd if=/dev/zero of=$(IMG_NAME) bs=1M count=$(IMG_SIZE)
	parted -s $(IMG_NAME) mklabel msdos mkpart primary fat16 1MiB 100%
	parted -s $(IMG_NAME) set 1 boot on
	mkfs.fat -F 16 --offset=2048 $(IMG_NAME)

install: $(TARGET) $(IMG_NAME)
	boot/write_bootsect.sh boot/bootsect.bin $(IMG_NAME)
	boot/write_setup.sh boot/setup.bin $(IMG_NAME)
	kernel/install_kernel.sh kernel/kernel $(IMG_NAME)

mount:
	sudo losetup -P /dev/loop0 $(IMG_NAME)
	sudo mount /dev/loop0p1 ./mnt --mkdir

umount:
	- sudo umount ./mnt            
	- sudo losetup -d /dev/loop0

qemu:
	qemu-system-i386 -m 1G -drive format=raw,file=$(IMG_NAME)

bochs:
	bochs -f bochsrc.cfg -q

clean:
	rm -f $(TARGET) *.o ./**/*.o
