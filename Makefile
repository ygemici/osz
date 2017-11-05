# OS/Z - an operating system for hackers
# Use is subject to license terms. Copyright 2017 bzt (bztsrc@github), CC-by-nc-sa

include Config
export O = @

all: chkconf clrdd todogen util boot system apps usrs images

# housekeeping

chkconf:
ifeq ($(ARCH),aarch64)
ifneq ($(PLATFORM),rpi)
	@echo Invalid ARCH and PLATFORM combination in Config, PLATFORM != rpi
	@false
endif
else
ifeq ($(PLATFORM),rpi)
	@echo Invalid ARCH and PLATFORM combination in Config, ARCH != aarch64
	@false
endif
endif

clrdd:
	@rm bin/disk.dd bin/osZ-latest-$(ARCH)-$(PLATFORM).dd 2>/dev/null || true

todogen:
	@echo " --- Error fixes ---" >TODO.txt
	@grep -ni 'FIXME:' `find . 2>/dev/null|grep -v './bin'` 2>/dev/null | grep -v Binary | grep -v grep >>TODO.txt || true
	@echo " --- Features ---" >>TODO.txt
	@grep -ni 'TODO:' `find . 2>/dev/null|grep -v './bin'` 2>/dev/null | grep -v Binary | grep -v grep >>TODO.txt || true

# boot loader

boot: loader/bootboot.bin loader/bootboot.efi

loader/kernel.img:
	@echo "LOADER"
	@make -e --no-print-directory -C loader/$(ARCH)-rpi | grep -v 'Nothing to be done' | grep -v 'rm bootboot'

loader/bootboot.bin:
	@echo "LOADER"
	@make -e --no-print-directory -C loader/$(ARCH)-bios | grep -v 'Nothing to be done' | grep -v 'rm bootboot'

loader/bootboot.efi:
	@echo "LOADER"
	@make -e --no-print-directory -C loader/$(ARCH)-efi | grep -v 'Nothing to be done' | grep -v 'rm bootboot'

# utilities to use during compilation

util: tools
	@cat etc/sys/etc/os-release | grep -v ^BUILD | grep -v ^ARCH | grep -v ^PLATFORM >/tmp/os-release
	@mv /tmp/os-release etc/sys/etc/os-release
	@date +'BUILD = "%Y-%m-%d %H:%M:%S UTC"' >>etc/sys/etc/os-release
	@echo 'ARCH = "$(ARCH)"' >>etc/sys/etc/os-release
	@echo 'PLATFORM = "$(PLATFORM)"' >>etc/sys/etc/os-release
	@echo "TOOLS"
	@make --no-print-directory -e -C tools all | grep -v 'Nothing to be done' || true

# the core

system: src
	@echo "CORE"
	@make -e --no-print-directory -C src system | grep -v 'Nothing to be done' || true

# user space programs

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

# disk image stuff

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
	@rm bin/disk.dd.lock 2>/dev/null || true
	@rm bin/disk.dd 2>/dev/null || true
	@make -e --no-print-directory -C src clean
	@make -e --no-print-directory -C tools clean
	@make -e --no-print-directory -C tools imgclean

# testing

debug: bin/osZ-latest-$(ARCH)-$(PLATFORM).dd
ifeq ($(DEBUG),1)
	@#qemu-system-x86_64 -s -S -name OS/Z -sdl -m 32 -d guest_errors -hda bin/osZ-latest-$(ARCH)-$(PLATFORM).dd -cpu IvyBridge,+ssse3,+avx,+x2apic -serial stdio
	qemu-system-x86_64 -s -S -name OS/Z -bios /usr/share/qemu/bios-TianoCoreEFI.bin -m 64 -hda bin/osZ-latest-$(ARCH)-$(PLATFORM).dd -serial stdio
else
	@echo Compiled without debugging symbols! Set DEBUG = 1 in Config and recompile.
endif

gdb:
	@gdb -w -x "etc/script.gdb" || true
	@pkill qemu

ifeq ($(ARCH),x86_64)
test: testq
else
test: testr
endif

testesp: 
	@echo "TEST"
	@echo
	qemu-system-x86_64 -name OS/Z -bios /usr/share/qemu/bios-TianoCoreEFI.bin -m 64 -hda fat:bin/ESP -enable-kvm -cpu host,+ssse3,+avx,+x2apic -serial mon:stdio

teste: bin/osZ-latest-$(ARCH)-$(PLATFORM).dd
	@echo "TEST"
	@echo
	@#qemu-system-x86_64 -name OS/Z -bios /usr/share/qemu/bios-TianoCoreEFI.bin -m 64 -hda bin/osZ-latest-$(ARCH)-$(PLATFORM).dd -option-rom loader/bootboot.rom -d guest_errors -enable-kvm -cpu host,+avx,+x2apic -serial mon:stdio
	qemu-system-x86_64 -name OS/Z -bios /usr/share/qemu/bios-TianoCoreEFI.bin -m 64 -hda bin/osZ-latest-$(ARCH)-$(PLATFORM).dd -option-rom bin/initrd.rom -enable-kvm -cpu host,+ssse3,+avx,+x2apic -serial mon:stdio

testq: bin/osZ-latest-$(ARCH)-$(PLATFORM).dd
	@echo "TEST"
	@echo
	@#qemu-system-x86_64 -no-hpet -name OS/Z -sdl -m 32 -d guest_errors -hda bin/osZ-latest-$(ARCH)-$(PLATFORM).dd -option-rom loader/bootboot.bin -enable-kvm -cpu host,+avx,+x2apic,enforce -serial mon:stdio
	@#qemu-system-x86_64 -no-hpet -name OS/Z -sdl -m 32 -d guest_errors -hda bin/osZ-latest-$(ARCH)-$(PLATFORM).dd -option-rom loader/bootboot.bin -enable-kvm -machine kernel-irqchip=on -cpu host,+avx,+x2apic,enforce -serial mon:stdio
	@#qemu-system-x86_64 -name OS/Z -sdl -m 32 -d guest_errors -hda bin/osZ-latest-$(ARCH)-$(PLATFORM).dd -enable-kvm -cpu host,+ssse3,+avx,+x2apic -serial mon:stdio
	qemu-system-x86_64 -name OS/Z -sdl -m 32 -d guest_errors,int -drive file=bin/osZ-latest-$(ARCH)-$(PLATFORM).dd,format=raw -enable-kvm -cpu host,+ssse3,+avx,+x2apic -serial mon:stdio

testb: bin/osZ-latest-$(ARCH)-$(PLATFORM).dd
	@echo "TEST"
	@echo
	@rm bin/disk.dd.lock 2>/dev/null || true
	@ln -s osZ-latest-$(ARCH)-$(PLATFORM).dd bin/disk.dd 2>/dev/null || true
	@#stupid bochs panic when symbols not found...
	@cat etc/bochs.rc | grep -v debug_symbols >etc/b
	@mv etc/b etc/bochs.rc
ifneq ($(wildcard bin/core.sym),)
	echo "debug_symbols: file=bin/core.sym, offset=0" >>etc/bochs.rc
endif
ifneq ($(wildcard /usr/local/bin/bochs),)
	/usr/local/bin/bochs -f etc/bochs.rc -q
else
	bochs -f etc/bochs.rc -q
endif
	@rm bin/disk.dd.lock 2>/dev/null || true
	@rm bin/disk.dd 2>/dev/null || true

testv: bin/disk.vdi
	@echo "TEST"
	@echo
	VBoxManage startvm "OS/Z"

testr: bin/osZ-latest-aarch64-rpi.dd
	@echo "TEST"
	@echo
	qemu-system-aarch64 -name OS/Z -sdl -M raspi3 -kernel loader/bootboot.img -drive file=bin/osZ-latest-$(ARCH)-$(PLATFORM).dd,if=sd,format=raw -serial mon:stdio -d int
