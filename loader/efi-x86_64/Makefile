include ../../Config
ARCH               = $(shell uname -m | sed s,i[3456789]86,ia32,)
GNUEFI_INCLUDES    = -I/usr/include -I. -I/usr/include/efi -I/usr/include/efi/$(ARCH) -I/usr/include/efi/protocol
GNUEFI_CRT_OBJS    = crt0-efi-$(ARCH).o
GNUEFI_LDS         = elf_$(ARCH)_efi.lds

CFLAGS = -DPRINT_DEBUG=$(DEBUG) -mno-red-zone -mno-mmx -mno-sse -O2 -fpic -Wall -Wextra -Werror -fshort-wchar -fno-strict-aliasing -ffreestanding -fno-stack-protector -fno-stack-check -DCONFIG_$(ARCH) -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 

LDFLAGS = -nostdlib
LDFLAGS += -shared -Bsymbolic -L. $(GNUEFI_CRT_OBJS)

TARGET  = bootboot.efi

all: zlib $(TARGET)

zlib:
	@make -e --no-print-directory -C zlib_inflate all

%.efi: %.so
	@echo "  src		efi-x86_64"
	@objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-$(ARCH) --subsystem=13 $^ $@
	@cp $(TARGET) ../../bin/ESP/EFI/BOOT/BOOTX64.EFI
	@../../tools/efirom.o $(TARGET) ../bootboot.rom
	@mv $(TARGET) ..

%.so: %.o
	@$(LD) $(LDFLAGS) $^ zlib_inflate/*.o -o $@ -lefi -lgnuefi -T $(GNUEFI_LDS)

%.o: %.c
	@$(CC) -I../../src/core $(GNUEFI_INCLUDES) $(CFLAGS) -c $< -o $@

%.o: %.S
	@$(CC) -I../../src/core $(GNUEFI_INCLUDES) $(CFLAGS) -c $< -o $@

clean:
	@make -e --no-print-directory -C zlib_inflate clean
	@rm bootboot.o ../$(TARGET) ../bootboot.rom *.so *.efi ../../bin/ESP/EFI/BOOT/BOOTX64.EFI 2>/dev/null || true

