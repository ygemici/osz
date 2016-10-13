all:
	@# NOTE: GNU assembler cannot mix 16 bit real mode with 32 bit prot mode instructions
	@echo "  src		mb-x86_64 (MultiBoot / BIOS)"
	@fasm mbr.asm ../mbr.bin >/dev/null
	@fasm bootboot.asm ../bootboot.bin >/dev/null

clean:
	@rm *.o ../mbr.bin ../bootboot.bin >/dev/null 2>/dev/null || true
