include ../../../Config
TARGET = lib/libc.so

BINTARGET = ../../../bin/root/$(TARGET)
SRCS = $(filter-out service.c,$(wildcard *.c))
OBJS = $(SRCS:.c=.o)
LIBS =

# without the -no flags, gcc generates extra, unwanted xmm code in printf...
CFLAGS = -Wall -fPIC -ffreestanding -mno-mmx -mno-sse -mno-sse2 -mno-sse3

all: $(OBJS) $(BINTARGET)

%.o: %.c
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $< -o $@

$(BINTARGET): $(SRCS)
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $(ARCH)/crt0.S -o ../../../bin/root/lib/crt0.o
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c service.c -o ../../../bin/service.o
	@echo "  src		$(TARGET) ($(ARCH))"
	@make -e --no-print-directory -C $(ARCH) all | grep -v 'Nothing to be done' || true
	@$(CC) -nostdlib -nodefaultlibs -nostartfiles -shared -Bsymbolic -Bshareable -L../../../bin/root/lib *.o $(ARCH)/*.o -Xlinker --build-id=sha1 -T ../../core/$(ARCH)/sharedlib.ld -o $(BINTARGET) $(LIBS)

clean:
	@rm *.o *.so $(ARCH)/*.o $(BINTARGET) ../../../bin/root/lib/crt0.o ../../../bin/service.o 2>/dev/null || true
