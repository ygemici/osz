SRCS = $(wildcard *.c)
OPT  = -g -I. -I/usr/include/efi -I/usr/include/efi/protocol -I/usr/include/efi/x86_64
LIBS =
OBJS = $(SRCS:.c=.o)

CFLAGS = -mno-red-zone -mno-mmx -mno-sse -O2 -fpic -Wall -fshort-wchar -fno-strict-aliasing -ffreestanding -fno-stack-protector -fno-stack-check -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11

all: $(OBJS)

%.o: %.c
	@echo "  src		efi/$<"
	@$(CC) $(OPT) $(CFLAGS) -c $<

clean:
	@rm *.o 2>/dev/null || true
