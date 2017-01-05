include ../../../Config

SRCS = $(wildcard *.c)
SRCS+= $(wildcard *.S)
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
LIBS =

CFLAGS = -DDEBUG=$(DEBUG) -DOPTIMIZE=$(OPTIMIZE) -DOSZ_CORE=1 -Wall -fpic -ffreestanding

all: $(OBJS) ok

isrs.S: isr.h isrs.sh isr.c
	@./isrs.sh $(ARCH) 2>/dev/null || true

ok:
	@true

%.o: %.S
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $< -o $@

clean:
	@rm *.o 2>/dev/null || true
