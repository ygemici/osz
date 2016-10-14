include Config

all: todogen util boot system apps images

todogen:
	@grep -ni 'TODO:' `find . 2>/dev/null` 2>/dev/null | grep -v Binary | grep -v grep >TODO.txt || true

boot: loader
	@date +'#define _OS_Z_BUILD "%Y-%m-%d %H:%M:%S UTC"' >etc/include/lastbuild.h
	@echo '#define _OS_Z_ARCH "$(ARCH)"' >>etc/include/lastbuild.h
	@echo '#define _OS_Z_LOADER "$(LOADER)"' >>etc/include/lastbuild.h
	@echo "LOADER"
	@make -e --no-print-directory -C loader/$(LOADER) | grep -v 'Nothing to be done' | grep -v 'rm bootboot'

util: tools
	@echo "TOOLS"
	@make --no-print-directory -C tools all | grep -v 'Nothing to be done' || true

system: src
	@echo "CORE"
	@make -e --no-print-directory -C src/core all | grep -v 'Nothing to be done' || true
	@make -e --no-print-directory -C src/libc all | grep -v 'Nothing to be done' || true

apps: src
	@echo "APPLICATIONS"
	@make -e --no-print-directory -C src all | grep -v 'Nothing to be done' || true

images: tools
	@echo "IMAGES"
	@make -e --no-print-directory -C tools images 2>&1 | grep -v 'Nothing to be done' | grep -v 'lowercase' || true

vdi: images
	@make -e --no-print-directory -C tools vdi | grep -v 'Nothing to be done' || true

vdmk: images
	@make -e --no-print-directory -C tools vdmk | grep -v 'Nothing to be done' || true

clean:
	@#make -e --no-print-directory -C loader/$(LOADER) clean
	@make -e --no-print-directory -C src clean
	@make -e --no-print-directory -C tools clean
	@make -e --no-print-directory -C tools imgclean

test: testq

testefi:
	@echo "TEST"
	@echo
	qemu-system-x86_64 -name OS/Z -bios bios-TianoCoreEFI.bin -m 256 -d guest_errors -hda fat:bin/ESP -option-rom loader/bootboot.rom -monitor stdio

testq:
	@echo "TEST"
	@echo
	qemu-system-x86_64 -name OS/Z -sdl -m 256 -d guest_errors -hda bin/disk.dd -option-rom loader/bootboot.bin -monitor stdio

testb:
	@echo "TEST"
	@echo
	bochs -f etc/bochs.rc -q
