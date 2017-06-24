include ../Config
NOINITRD = 1
INITRD = ../bin/ESP/BOOTBOOT/INITRD
ESPFILE = ../bin/esp.part
SYSFILE = ../bin/sys.part
USRFILE = ../bin/usr.part
VARFILE = ../bin/var.part
HOMEFILE = ../bin/home.part
DDIMAGE = ../bin/disk.dd
VDIIMAGE = ../bin/disk.vdi
VMDKIMAGE = ../bin/disk.vmdk
SRCS = mkfs.c elftool.c
OPT  = -g
LIBS =
OBJS = $(SRCS:.c=.o)

CFLAGS = -Wall -fshort-wchar

all: selfcheck $(OBJS)

%.o: %.c
	@echo "  src		$<"
	@$(CC) $(OPT) $(CFLAGS) $< -o $@ $(LIBS)

clean:
	@rm *.o 2>/dev/null || true

imgclean:
	@rm -rf ../bin/*.txt ../bin/*.sym ../bin/*.part ../bin/*.vdi ../bin/*.vmdk ../bin/*.rom ../bin/initrd ../bin/usr ../bin/ESP 2>/dev/null || true

esp: initrd
	@echo "  mkfs		boot (ESP)"
ifneq ($(NOINITRD),1)
	@cp -f ../etc/etc/sys/config ../bin/ESP/BOOTBOOT/CONFIG 2>/dev/null || true
ifneq ("$(wildcard ../loader/bootboot.bin)","")
	@cp ../loader/bootboot.bin ../bin/ESP/BOOTBOOT/LOADER || true
endif
endif
ifneq ("$(wildcard ../loader/bootboot.efi)","")
	@cp ../loader/bootboot.efi ../bin/ESP/EFI/BOOT/BOOTX64.EFI || true
endif
	@dd if=/dev/zero of=$(ESPFILE) bs=1024 count=$(ESPSIZE) >/dev/null 2>/dev/null
	@mkfs.vfat -F 12 -n "EFI System" $(ESPFILE) 2>/dev/null >/dev/null
	@mkdir ../bin/tmp 2>/dev/null || true
	@sudo /usr/bin/mount -o loop,user $(ESPFILE) ../bin/tmp
	@sudo /usr/bin/cp -r ../bin/ESP/* ../bin/tmp/ || true
	@sudo /usr/bin/umount -f /dev/loop* 2>/dev/null || true
	@rmdir ../bin/tmp

initrd:
ifeq ($(NOINITRD),1)
	@echo "  mkfs		root"
else
	@echo "  mkfs		initrd"
endif
	@# populate with static files
	@cp -r ../etc/etc ../bin/initrd || true
	@cp -r ../etc/root ../bin/initrd || true
ifeq ($(NOINITRD),1)
ifneq ("$(wildcard ../loader/bootboot.bin)","")
	@cp ../loader/bootboot.bin ../bin/initrd/lib/sys/loader || true
endif
	@./mkfs.o $(SYSFILE) ../bin/initrd
else
	@./mkfs.o $(INITRD) ../bin/initrd
	@gzip $(INITRD) && mv $(INITRD).gz $(INITRD) || true
	@#./mkfs.o $(INITRD) initrdrom ../bin/initrd.rom
endif

usr: $(USRFILE)

$(USRFILE): ../bin/usr
	@echo "  mkfs		usr"
	@cp -r ../etc/include ../bin/usr/core || true
	@dd if=/dev/zero of=$(USRFILE) bs=1024 count=$(USRSIZE) >/dev/null 2>/dev/null
	@./mkfs.o $(USRFILE) ../bin/usr
	@./mkfs.o $(USRFILE) union bin/ "/bin" "/usr/*/bin"
	@./mkfs.o $(USRFILE) union include/ "/usr/*/include"
	@./mkfs.o $(USRFILE) union lib/ "/usr/*/lib"
	@./mkfs.o $(USRFILE) union share/ "/usr/*/share"

var: $(VARFILE)

$(VARFILE): ../bin/var
	@echo "  mkfs		var"
	@dd if=/dev/zero of=$(VARFILE) bs=1024 count=$(VARSIZE) >/dev/null 2>/dev/null

home: $(HOMEFILE)

$(HOMEFILE): ../bin/home
	@echo "  mkfs		home"
	@dd if=/dev/zero of=$(HOMEFILE) bs=1024 count=$(HOMESIZE) >/dev/null 2>/dev/null

images: initrd esp $(USRFILE) $(VARFILE) $(HOMEFILE)
	@echo "  mkfs		bin/disk.dd"
	@./mkfs.o disk

vdi: $(DDIMAGE)
	@echo "  mkfs		bin/disk.vdi"
	@rm -f $(VDIIMAGE) 2>/dev/null || true
	@VBoxManage convertfromraw $(DDIMAGE) $(VDIIMAGE)
	@VBoxManage internalcommands sethduuid $(VDIIMAGE) 45a68a8d-9a2b-42ba-91f9-99c7259bfa8a 2>/dev/null || true

vmdk: $(DDIMAGE)
	@#echo "  mkfs		bin/disk.vdmk"
	@VBoxManage convertdd $(DDIMAGE) $(VMDKIMAGE) --format VMDK

selfcheck:
ifeq ("$(wildcard ../bin)","")
	@mkdir ../bin
endif
ifeq ("$(wildcard ../bin/ESP/EFI/BOOT)","")
	@mkdir -p ../bin/ESP/EFI/BOOT
endif
ifneq ($(NOINITRD),1)
ifeq ("$(wildcard ../bin/ESP/BOOTBOOT)","")
	@mkdir ../bin/ESP/BOOTBOOT
endif
endif
ifeq ("$(wildcard ../bin/initrd)","")
	@# to be kept in memory
	@mkdir ../bin/initrd || true
	@mkdir ../bin/initrd/bin || true
	@mkdir ../bin/initrd/etc || true
	@mkdir ../bin/initrd/etc/sys || true
	@mkdir ../bin/initrd/lib || true
	@mkdir ../bin/initrd/lib/sys || true
	@mkdir ../bin/initrd/root || true
	@mkdir ../bin/initrd/sbin || true
ifeq ($(NOINITRD),1)
	@mkdir ../bin/initrd/dev || true
	@mkdir ../bin/initrd/home || true
	@mkdir ../bin/initrd/run || true
	@# mount points
	@mkdir ../bin/initrd/sys || true
	@mkdir ../bin/initrd/usr || true
	@mkdir ../bin/initrd/var || true
	@mkdir ../bin/initrd/tmp || true
endif
endif
ifeq ("$(wildcard ../bin/usr)","")
	@mkdir ../bin/usr || true
	@mkdir ../bin/usr/core || true
	@mkdir ../bin/usr/core/bin
	@mkdir ../bin/usr/core/etc
	@mkdir ../bin/usr/core/include
	@mkdir ../bin/usr/core/lib || true
	@mkdir ../bin/usr/core/share || true
endif
ifeq ("$(wildcard ../bin/var)","")
	@mkdir ../bin/var || true
endif
ifeq ("$(wildcard ../bin/home)","")
	@mkdir ../bin/home || true
	@mkdir ../bin/home/bzt || true
endif
