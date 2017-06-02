all:
	@echo "  src		rpi-AArch64 (Raspberry Pi 3+)"
	@gcc -c bootboot.S -o bootboot.o
	@objcopy -O binary bootboot.o ../kernel.img
	@rm bootboot.o

clean:
	@rm *.o ../kernel.img >/dev/null 2>/dev/null || true
