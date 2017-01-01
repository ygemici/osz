include ../../../Config

SRCS = $(wildcard *.c)
SRCS+= $(wildcard *.S)
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
LIBS =

CFLAGS = -DDEBUG=$(DEBUG) -DOSZ_CORE=1 -Wall -fpic -ffreestanding

all: isrs.S $(OBJS) ok

isrs.S: isr.h isrs.sh
	@./isrs.sh $(ARCH) 2>/dev/null || true

ok:
	@true

%.o: %.S
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $< -o $@

clean:
	@rm *.o 2>/dev/null || true
