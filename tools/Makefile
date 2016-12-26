include ../Config
INITRD = ../bin/ESP/BOOTBOOT/INITRD
ESPFILE = ../bin/esp.part
USRFILE = ../bin/usr.part
VARFILE = ../bin/var.part
HOMEFILE = ../bin/home.part
DDIMAGE = ../bin/disk.dd
VDIIMAGE = ../bin/disk.vdi
VDMKIMAGE = ../bin/disk.vdmk
SRCS = $(filter-out dumpelf.c dynamic.c,$(wildcard *.c))
OPT  = -g
LIBS =
OBJS = $(SRCS:.c=.o)

CFLAGS = -g -Wall -fshort-wchar

all: selfcheck $(OBJS)

%.o: %.c
	@echo "  src		$<"
	@$(CC) -I/usr/include/efi -I/usr/include/efi/x86_64 $(OPT) $(CFLAGS) $< -o $@ $(LIBS)

clean:
	@rm *.o 2>/dev/null || true

imgclean:
	@rm -rf ../bin/*.sym ../bin/*.part ../bin/*.vdi ../bin/*.vdmk ../bin/root ../bin/usr ../bin/ESP 2>/dev/null || true

esp: initrd
	@echo "  mkfs		ESP"
	@#cp ../bin/initrd.gz ../bin/ESP/BOOTBOOT/INITRD || true
	@cp -f ../etc/CONFIG ../bin/ESP/BOOTBOOT/CONFIG 2>/dev/null || true
ifneq ("$(wildcard ../loader/bootboot.efi)","")
	@cp ../loader/bootboot.efi ../bin/ESP/EFI/BOOT/BOOTX64.EFI || true
endif
ifneq ("$(wildcard ../loader/bootboot.bin)","")
	@cp ../loader/bootboot.bin ../bin/ESP/BOOTBOOT/LOADER || true
endif
	@dd if=/dev/zero of=$(ESPFILE) bs=1024 count=$(ESPSIZE) >/dev/null 2>/dev/null
	@mkfs.vfat -F 12 -n "EFI SYSTEM" $(ESPFILE) >/dev/null
	@mkdir ../bin/tmp 2>/dev/null || true
	@sudo mount -o loop $(ESPFILE) ../bin/tmp
	@sudo cp -r ../bin/ESP/* ../bin/tmp/ || true
	@sudo umount -f /dev/loop* 2>/dev/null || true
	@rmdir ../bin/tmp

initrd:
	@echo "  mkfs		initrd"
	@./mkfs.o create $(INITRD) ../bin/root

usr:
	@echo "  mkfs		usr"
	@dd if=/dev/zero of=$(USRFILE) bs=1024 count=$(USRSIZE) >/dev/null 2>/dev/null
	@./mkfs.o create $(USRFILE) ../bin/usr

var:
	@echo "  mkfs		var"
	@dd if=/dev/zero of=$(VARFILE) bs=1024 count=$(VARSIZE) >/dev/null 2>/dev/null

home:
	@echo "  mkfs		home"
	@dd if=/dev/zero of=$(HOMEFILE) bs=1024 count=$(HOMESIZE) >/dev/null 2>/dev/null

images: initrd esp usr var home
	@echo "  mkfs		bin/disk.dd"
	./mkfs.o disk

vdi: $(DDIMAGE)
	@echo "  mkfs		bin/disk.vdi"
	@VBoxManage convertfromraw $(DDIMAGE) $(VDIIMAGE)

vdmk: $(DDIMAGE)
	@#echo "  mkfs		bin/disk.vdmk"
	@#VBoxManage convertfromraw $(DDIMAGE) $(VDMKIMAGE)

selfcheck:
ifeq ("$(wildcard ../bin)","")
	@mkdir ../bin
endif
ifeq ("$(wildcard ../bin/ESP/EFI/BOOT)","")
	@mkdir -p ../bin/ESP/EFI/BOOT
endif
ifeq ("$(wildcard ../bin/ESP/BOOTBOOT)","")
	@mkdir ../bin/ESP/BOOTBOOT
endif
ifeq ("$(wildcard ../bin/root)","")
	@# to be kept in memory
	@mkdir ../bin/root || true
	@mkdir ../bin/root/bin || true
	@mkdir ../bin/root/dev || true
	@mkdir ../bin/root/etc || true
	@mkdir ../bin/root/home || true
	@mkdir ../bin/root/lib || true
	@mkdir ../bin/root/lib/sys || true
	@mkdir ../bin/root/root || true
	@mkdir ../bin/root/sbin || true
	@mkdir ../bin/root/run || true
	@# mount points
	@mkdir ../bin/root/sys || true
	@mkdir ../bin/root/usr || true
	@mkdir ../bin/root/var || true
	@mkdir ../bin/root/tmp || true
	@# populate with files
	@cp -r ../etc/etc ../bin/root || true
	@cp -r ../etc/root ../bin/root || true
endif
ifeq ("$(wildcard ../bin/usr)","")
	@mkdir ../bin/usr || true
	@mkdir ../bin/usr/bin
	@mkdir ../bin/usr/include
	@cp -r ../etc/include ../bin/usr || true
	@mkdir ../bin/usr/lib || true
	@mkdir ../bin/usr/share || true
endif
ifeq ("$(wildcard ../bin/var)","")
	@mkdir ../bin/var || true
endif
ifeq ("$(wildcard ../bin/home)","")
	@mkdir ../bin/home || true
endif
