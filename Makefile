TARGET = bootsect.bin
CC = gcc
IMG_NAME = disk.img
IMG_SIZE = 64

all: $(TARGET)

$(TARGET): bootsect.S
	$(CC) -nostdlib -nostartfiles -no-pie -Ttext=0x7C00 -o $@ $<
	objcopy -S -O binary -j .text $@ $@

$(IMG_NAME):
	dd if=/dev/zero of=$(IMG_NAME) bs=1M count=$(IMG_SIZE)
	parted -s $(IMG_NAME) mklabel msdos mkpart primary fat32 1MiB 100%

install: $(TARGET) $(IMG_NAME)
	./write_bootsect.sh $(TARGET) $(IMG_NAME)

qemu:
	qemu-system-i386 -m 1G -nographic -drive format=raw,file=$(IMG_NAME)

clean:
	rm -f $(TARGET)
