include ../../../Config

SRCS = $(wildcard *.c)
SRCS+= $(wildcard *.S)
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
LIBS =

CFLAGS = -DDEBUG=$(DEBUG) -Wall -fpic -ffreestanding

all: isrs syscall.o $(OBJS) ok

syscall.o:
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c ../../lib/libc/$(ARCH)/syscall.S -o syscall.o

isrs:
	@test isr.h -nt isrs.S -o isrs.sh -nt isrs.S && ./isrs.sh $(ARCH) 2>/dev/null || true

ok:
	@true

%.o: %.S
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $< -o $@

clean:
	@rm *.o 2>/dev/null || true
