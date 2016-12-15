include ../../../Config
TARGET = lib/libc.so

BINTARGET = ../../../bin/root/$(TARGET)
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
LIBS =
CLL  = ../../../tools/cll.o

CFLAGS = -fpic -fPIC -ffreestanding

all: $(OBJS) $(BINTARGET)

%.o: %.c
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $< -o $@

$(BINTARGET): $(SRCS)
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $(ARCH)/crt0.S -o ../../../bin/root/lib/crt0.o
	@echo "  src		$(TARGET) ($(ARCH))"
	make -e --no-print-directory -C $(ARCH) all | grep -v 'Nothing to be done' || true
	@$(CLL) *.c
	@$(CC) -nostdlib -nodefaultlibs -nostartfiles -shared -Bsymbolic *.o -Xlinker --build-id=none -T $(ARCH)/link.ld -Wl,-soname,libc -o $(BINTARGET) $(LIBS)

clean:
	@rm cl.S *.o *.so $(ARCH)/*.o $(BINTARGET) ../../../bin/root/lib/crt0.o 2>/dev/null || true
