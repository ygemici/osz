SRCS = $(wildcard *.c)
SRCS+= $(filter-out crt.S,$(wildcard *.S))
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
LIBS =

CFLAGS = -Wall -fpic -ffreestanding -fPIC

all: $(OBJS) ok

ok:
	@true

%.o: %.S
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $< -o $@

clean:
	@rm *.o 2>/dev/null || true
