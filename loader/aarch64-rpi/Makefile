all: boot.S bootboot.c fs.h
	@echo "  src		aarch64-rpi (Raspberry Pi 3+)"
	@aarch64-elf-gcc -Wall -O -ffreestanding -nostdinc -c boot.S -o boot.o
	@aarch64-elf-gcc -Wall -O -ffreestanding -nostdinc -I ../../etc/include/ -I ./ -c tinflate.c -o tinflate.o
	@aarch64-elf-gcc -Wall -O -ffreestanding -nostdinc -I ../../etc/include/ -I ./ -c bootboot.c -o bootboot.o
	@aarch64-elf-ld -nostdlib boot.o bootboot.o tinflate.o -T link.ld -o bootboot.elf
	@aarch64-elf-objcopy -O binary bootboot.elf ../kernel8.img
	@rm bootboot.elf *.o

getfw:
	wget -q https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/LICENCE.broadcom
	wget -q https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/bootcode.bin
	wget -q https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/fixup.dat
	wget -q https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/start.elf

cleanfw:
	@rm LICENCE.broadcom bootcode.bin fixup.dat start.elf

clean:
	@rm *.o *.elf >/dev/null 2>/dev/null || true
