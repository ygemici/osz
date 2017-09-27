all:
	@echo "  src		rpi-AArch64 (Raspberry Pi 3+)"
	@gcc -c boot.S -o bootboot.o
	@objcopy -O binary bootboot.o ../kernel8.img
	@rm bootboot.o

clean:
	@rm *.o ../kernel8.img >/dev/null 2>/dev/null || true
