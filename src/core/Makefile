include ../../Config
TARGET = lib/core

BINTARGET = ../../bin/root/$(TARGET)
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
LIBS =
ARCHOBJS = $(filter-out $(ARCH)/start.o,$(wildcard $(ARCH)/*.o))
CFLAGS = -Wall -fpic -ffreestanding -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-sse3

all: $(OBJS) platf $(BINTARGET)

%.o: %.c
	@$(CC) -I../../etc/include -I. $(OPT) $(CFLAGS) -c $< -o $@

platf:
	@make -e --no-print-directory -C $(ARCH) all | grep -v 'Nothing to be done' || true

$(BINTARGET): platf $(SRCS)
	@echo "  src		$(TARGET) ($(ARCH))"
	@$(CC) -nostdlib -nodefaultlibs -nostartfiles $(ARCH)/start.o $(OBJS) $(ARCHOBJS) -Xlinker --build-id=none -Xlinker --nmagic -T link.ld -o _.o $(LIBS)
	@strip -s -K fb -K bootboot -K environment _.o
	@mv _.o $(BINTARGET)

clean:
	@rm *.o $(ARCH)/*.o $(BINTARGET) 2>/dev/null || true
