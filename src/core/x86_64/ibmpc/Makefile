include ../../../../etc/sys/etc/os-release
include ../../../../Config

SRCS = $(wildcard *.c)
SRCS+= $(wildcard *.S)
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
CFLAGS = -DDEBUG=$(DEBUG) -DOPTIMIZE=$(OPTIMIZE) -DOSZ_NAME='$(NAME)' -DOSZ_VER='$(VERSION)' -DOSZ_ARCH='"$(ARCH)"' -DOSZ_BUILD='$(BUILD)' \
 -Wall -fpic -ffreestanding -nostdinc -nostdlib -fno-stack-protector -I../../../../etc/include

all: $(OBJS)

%.o: %.S
	@#echo "  src		$(ARCH)/$(PLATFORM)/$<"
	$(O)$(CC) -D_AS=1 $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c
	@#echo "  src		$(ARCH)/$(PLATFORM)/$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $< -o $@

clean:
	@rm *.o 2>/dev/null || true
