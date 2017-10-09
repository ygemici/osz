all:
	@echo "  src		aarch64-rpi (Raspberry Pi 3+)"
	@#aarch64-elf-gcc boot.S bootboot.c -o bootboot.o
	@aarch64-elf-objcopy -O binary bootboot.o ../kernel8.img
	@rm bootboot.o

clean:
	@rm *.o ../kernel8.img >/dev/null 2>/dev/null || true
