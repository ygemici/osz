include ../../Config
TARGET = lib/sys/core

BINTARGET = ../../bin/root/$(TARGET)
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
LIBS =
ARCHOBJS = $(filter-out $(ARCH)/start.o,$(wildcard $(ARCH)/*.o))
CFLAGS = -DDEBUG=$(DEBUG) -Wall -fpic -ffreestanding -fvisibility=hidden -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-sse3

all: $(OBJS) $(BINTARGET)

%.o: %.c
	@$(CC) -I../../etc/include -I. -I./$(ARCH) -include ./$(ARCH)/platform.h $(OPT) $(CFLAGS) -c $< -o $@

platform:
	@make -e --no-print-directory -C $(ARCH) all | grep -v 'Nothing to be done' || true

font.o:
	@cp ../../etc/system.8x16.psf ./font
	@objcopy -O elf64-x86-64 -B i386 -I binary font font.o
	@rm font

logo.o:
	@objcopy -O elf64-x86-64 -B i386 -I binary logo.tga logo.o

$(BINTARGET): platform font.o logo.o $(SRCS)
	@echo "  src		$(TARGET) ($(ARCH))"
	@$(CC) -nostdlib -nodefaultlibs -nostartfiles $(ARCH)/start.o $(OBJS) $(ARCHOBJS) font.o logo.o -Xlinker --build-id=none -Xlinker --nmagic -T $(ARCH)/supervisor.ld -o $(BINTARGET) $(LIBS)
ifeq ($(DEBUG),1)
	@echo "0000000000200000 _usercode" >../../bin/core.sym
ifneq ($(wildchard /usr/bin/nm),)
	@nm -s $(BINTARGET) | cut -b 1-17,20- | grep -v -e '^000000000000' | sort >>../../bin/core.sym
else
	@readelf -s $(BINTARGET) | cut -b 9-25,60- | tail -n +4 | grep -v -e '^000000000000' | sort >>../../bin/core.sym
endif
endif
	@strip -s -K fb -K bootboot -K environment $(BINTARGET)
	@echo -n "OS/Z" | dd conv=notrunc of=$(BINTARGET) bs=1 seek=0 1>/dev/null 2>/dev/null
	@echo -ne '\003' | dd conv=notrunc of=$(BINTARGET) bs=1 seek=16 1>/dev/null 2>/dev/null

clean:
	@rm *.o $(ARCH)/*.o $(BINTARGET) ../../bin/*.sym 2>/dev/null || true
