include Config
export O = @

all: clrdd todogen util boot system apps usrs images

clrdd:
	@rm bin/disk.dd 2>/dev/null || true

todogen:
	@echo " --- Error fixes ---" >TODO.txt
	@grep -ni 'FIXME:' `find . 2>/dev/null|grep -v './bin'` 2>/dev/null | grep -v Binary | grep -v grep >>TODO.txt || true
	@echo " --- Features ---" >>TODO.txt
	@grep -ni 'TODO:' `find . 2>/dev/null|grep -v './bin'` 2>/dev/null | grep -v Binary | grep -v grep >>TODO.txt || true

boot: loader/bootboot.bin loader/bootboot.efi

loader/kernel.img:
	@echo "LOADER"
	@make -e --no-print-directory -C loader/rpi-$(ARCH) | grep -v 'Nothing to be done' | grep -v 'rm bootboot'

loader/bootboot.bin:
	@echo "LOADER"
	@make -e --no-print-directory -C loader/mb-$(ARCH) | grep -v 'Nothing to be done' | grep -v 'rm bootboot'

loader/bootboot.efi:
	@echo "LOADER"
	@make -e --no-print-directory -C loader/efi-$(ARCH) | grep -v 'Nothing to be done' | grep -v 'rm bootboot'

util: tools
	@date +'#define OSZ_BUILD "%Y-%m-%d %H:%M:%S UTC"' >etc/include/lastbuild.h
	@echo '#define OSZ_ARCH_$(ARCH) 1' >>etc/include/lastbuild.h
	@echo "TOOLS"
	@make --no-print-directory -e -C tools all | grep -v 'Nothing to be done' || true

system: src
	@echo "CORE"
	@make -e --no-print-directory -C src system | grep -v 'Nothing to be done' || true

apps: src
	@echo "BASE"
	@make -e --no-print-directory -C src libs | grep -v 'Nothing to be done' || true
	@make -e --no-print-directory -C src apps | grep -v 'Nothing to be done' || true
	@echo "DRIVERS"
	@make -e --no-print-directory -C src drivers | grep -v 'Nothing to be done' || true
ifeq ($(DEBUG),1)
	@make -e --no-print-directory -C src gensyms 2>&1 | grep -v 'Nothing to be done' | grep -v 'No rule to make target' || true
endif

usrs: usr
	@echo "USERSPACE"
	@make -e --no-print-directory -C usr all | grep -v 'Nothing to be done' || true

images: tools
	@echo "IMAGES"
	@make -e --no-print-directory -C tools images | grep -v 'Nothing to be done' | grep -v 'lowercase' || true

bin/disk.vdi: vdi

vdi: images
	@make -e --no-print-directory -C tools vdi | grep -v 'Nothing to be done' || true

bin/disk.vmdk: vmdk

vmdk: images
	@make -e --no-print-directory -C tools vmdk | grep -v 'Nothing to be done' || true

clean:
	@make -e --no-print-directory -C src clean
	@make -e --no-print-directory -C tools clean
	@make -e --no-print-directory -C tools imgclean

debug:
ifeq ($(DEBUG),1)
	qemu-system-x86_64 -s -S -hda bin/disk.dd -cpu IvyBridge,+avx,+x2apic -serial stdio
else
	@echo Compiled without debugging symbols! Set DEBUG = 1 in Config and recompile.
endif

gdb:
	@gdb -w -x "etc/script.gdb"
	@pkill qemu

test: testq

testefi:
	@echo "TEST"
	@echo
	@#qemu-system-x86_64 -name OS/Z -bios /usr/share/qemu/bios-TianoCoreEFI.bin -m 64 -hda fat:bin/ESP -option-rom loader/bootboot.rom -d guest_errors -monitor stdio
	@#qemu-system-x86_64 -name OS/Z -bios /usr/share/qemu/bios-TianoCoreEFI.bin -m 64 -hda bin/disk.dd -option-rom loader/bootboot.rom -d guest_errors -enable-kvm -cpu host,+avx,+x2apic -serial mon:stdio
	qemu-system-x86_64 -name OS/Z -bios /usr/share/qemu/bios-TianoCoreEFI.bin -m 64 -hda bin/disk.dd -serial mon:stdio

testq:
	@echo "TEST"
	@echo
	@#qemu-system-x86_64 -no-hpet -name OS/Z -sdl -m 32 -d guest_errors -hda bin/disk.dd -option-rom loader/bootboot.bin -enable-kvm -cpu host,+avx,+x2apic,enforce -serial mon:stdio
	@#qemu-system-x86_64 -no-hpet -name OS/Z -sdl -m 32 -d guest_errors -hda bin/disk.dd -option-rom loader/bootboot.bin -enable-kvm -machine kernel-irqchip=on -cpu host,+avx,+x2apic,enforce -serial mon:stdio
	qemu-system-x86_64 -name OS/Z -sdl -m 32 -d guest_errors -hda bin/disk.dd -enable-kvm -cpu host,+x2apic -serial mon:stdio

testb:
	@echo "TEST"
	@echo
	@rm bin/disk.dd.lock 2>&1 >dev/null || true
ifneq ($(wildcard /usr/local/bin/bochs),)
	/usr/local/bin/bochs -f etc/bochs.rc -q
else
	bochs -f etc/bochs.rc -q
endif

testv: bin/disk.vdi
	@echo "TEST"
	@echo
	VBoxManage startvm "OS/Z"
