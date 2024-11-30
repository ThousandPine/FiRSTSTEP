TARGET = bootsect.bin setup.bin
CC = gcc
IMG_NAME = disk.img
IMG_SIZE = 64

all: $(TARGET)

bootsect.bin: bootsect.S
	$(CC) -m32 -nostdlib -nostartfiles -no-pie -Ttext=0x7C00 -o $@ $<
	objcopy -S -O binary -j .text $@ $@

setup.bin: setup.S
	$(CC) -m32 -nostdlib -nostartfiles -no-pie -Ttext=0x0 -o $@ $<
	objcopy -S -O binary -j .text $@ $@

$(IMG_NAME):
	dd if=/dev/zero of=$(IMG_NAME) bs=1M count=$(IMG_SIZE)
	parted -s $(IMG_NAME) mklabel msdos mkpart primary fat32 1MiB 100%

install: $(TARGET) $(IMG_NAME)
	./write_bootsect.sh bootsect.bin $(IMG_NAME)
	./write_setup.sh setup.bin $(IMG_NAME)

qemu:
	qemu-system-i386 -m 1G -nographic -drive format=raw,file=$(IMG_NAME)

bochs:
	bochs -f bochsrc.cfg -q

clean:
	rm -f $(TARGET)
