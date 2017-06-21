include ../../../etc/etc/os-release
include ../../../Config

SRCS = $(filter-out dbg.c disasm.c,$(wildcard *.c))
#ifeq $($(DEBUG),1)
SRCS += dbg.c disasm.c
#endif
SRCS+= $(wildcard *.S)
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
CFLAGS = -DDEBUG=$(DEBUG) -DOPTIMIZE=$(OPTIMIZE) -DOSZ_NAME='$(NAME)' -DOSZ_VER='$(VERSION)' -DOSZ_ARCH='"$(ARCH)"' -DOSZ_BUILD='$(BUILD)' \
 -Wall -fpic -ffreestanding -nostdinc -I ../../../etc/include -include platform.h -include ../env.h

all: $(OBJS)

isrs.S: isr.h isrs.sh isr.c
	@./isrs.sh $(ARCH) 2>/dev/null || true

%.o: %.S isrs.S
	@#echo "  src		$(ARCH)/$<"
	$(O)$(CC) -D_AS=1 $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c isrs.S
	@#echo "  src		$(ARCH)/$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $< -o $@

clean:
	@rm *.o 2>/dev/null || true
