IMG_NAME := disk.img
IMG_SIZE := 16

CFLAGS := -g
CFLAGS += -DDEBUG

all: boot kernel lib usr

boot: $(IMG_NAME)
	$(MAKE) -C boot IMG_PATH=../$(IMG_NAME)

kernel: $(IMG_NAME) lib
	$(MAKE) -C kernel IMG_PATH=../$(IMG_NAME) CFLAGS="$(CFLAGS)"

lib: $(IMG_NAME)
	$(MAKE) -C lib CFLAGS="$(CFLAGS)"

usr: $(IMG_NAME) lib
	$(MAKE) -C usr IMG_PATH=../$(IMG_NAME) CFLAGS="$(CFLAGS)"

$(IMG_NAME):
	dd if=/dev/zero of=$(IMG_NAME) bs=1M count=$(IMG_SIZE)
	parted -s $(IMG_NAME) mklabel msdos mkpart primary fat16 1MiB 100%
	parted -s $(IMG_NAME) set 1 boot on
	mkfs.fat -F 16 --offset=2048 $(IMG_NAME)

mount: $(IMG_NAME)
	sudo losetup -P /dev/loop0 $(IMG_NAME)
	sudo mount /dev/loop0p1 ./mnt --mkdir

umount:
	- sudo umount ./mnt            
	- sudo losetup -d /dev/loop0

qemu: all
	qemu-system-i386 -m 1G -drive format=raw,file=$(IMG_NAME)

bochs: all
	- rm -f disk.img.lock
	bochs -f bochsrc.cfg -q

bochs-gdb: all
	- rm -f disk.img.lock
	bochs-gdb -f bochsrc.gdb -q

clean:
	$(MAKE) -C boot clean
	$(MAKE) -C kernel clean
	$(MAKE) -C lib clean
	$(MAKE) -C usr clean
	rm -f $(IMG_NAME)

.PHONY: all clean mount qemu bochs bochs-gdb umount boot kernel lib