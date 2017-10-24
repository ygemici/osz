include ../../../etc/sys/etc/os-release
include ../../../Config

SRCS = $(filter-out disasm.c,$(wildcard *.c))
#ifeq $($(DEBUG),1)
SRCS += disasm.c
#endif
SRCS+= $(wildcard *.S)
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
CFLAGS = -DDEBUG=$(DEBUG) -DOPTIMIZE=$(OPTIMIZE) -DOSZ_NAME='$(NAME)' -DOSZ_VER='$(VERSION)' \
 -DOSZ_ARCH='"$(ARCH)"' -DOSZ_PLATFORM='"$(PLATFORM)"' -DOSZ_BUILD='$(BUILD)' \
 -Wall -fpic -ffreestanding -nostdinc -nostdlib -fno-stack-protector -I../../../etc/include -I./$(PLATFORM)

all: $(PLATFORM)/isrs.S $(OBJS)

$(PLATFORM)/isrs.S: isr.h isr.c $(PLATFORM)/platform.h
	@./isrs.sh 2>/dev/null || true

%.o: %.S $(PLATFORM)/isrs.S
	@#echo "  src		$(ARCH)/$<"
	$(O)$(CC) -D_AS=1 $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c $(PLATFORM)/isrs.S
	@#echo "  src		$(ARCH)/$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $< -o $@

clean:
	@rm *.o 2>/dev/null || true
