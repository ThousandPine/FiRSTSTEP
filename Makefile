IMG_NAME = disk.img
IMG_SIZE = 16

all: boot kernel

boot: $(IMG_NAME)
	$(MAKE) -C boot IMG_PATH=../$(IMG_NAME)

kernel: $(IMG_NAME)
	$(MAKE) -C kernel IMG_PATH=../$(IMG_NAME)

$(IMG_NAME):
	dd if=/dev/zero of=$(IMG_NAME) bs=1M count=$(IMG_SIZE)
	parted -s $(IMG_NAME) mklabel msdos mkpart primary fat16 1MiB 100%
	parted -s $(IMG_NAME) set 1 boot on
	mkfs.fat -F 16 --offset=2048 $(IMG_NAME)

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
	$(MAKE) -C boot clean
	$(MAKE) -C kernel clean

.PHONY: boot kernel