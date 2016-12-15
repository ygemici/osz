include ../../Config
TARGET = lib/sys/core

BINTARGET = ../../bin/root/$(TARGET)
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
LIBS =
ARCHOBJS = $(filter-out $(ARCH)/start.o,$(wildcard $(ARCH)/*.o))
CFLAGS = -DDEBUG=$(DEBUG) -Wall -fpic -ffreestanding -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-sse3

all: $(OBJS) $(BINTARGET)

%.o: %.c
	@$(CC) -I../../etc/include -I. -I./$(ARCH) $(OPT) $(CFLAGS) -c $< -o $@

platform:
	@make -e --no-print-directory -C $(ARCH) all | grep -v 'Nothing to be done' || true

font.o:
	@cp ../../etc/system.8x16.psf ./font
	@objcopy -O elf64-x86-64 -B i386 -I binary font font.o
	@rm font

$(BINTARGET): platform font.o $(SRCS)
	@echo "  src		$(TARGET) ($(ARCH))"
	@$(CC) -nostdlib -nodefaultlibs -nostartfiles $(ARCH)/start.o $(OBJS) $(ARCHOBJS) font.o -Xlinker --build-id=none -Xlinker --nmagic -T link.ld -o $(BINTARGET) $(LIBS)
	@#strip -s -K fb -K bootboot -K environment $(BINTARGET)

clean:
	@rm *.o $(ARCH)/*.o $(BINTARGET) 2>/dev/null || true
