include ../../../Config
TARGET = lib/libc.so

BINTARGET = ../../../bin/initrd/$(TARGET)
SRCS = $(filter-out service.c dbg.c,$(wildcard *.c))
OBJS = $(SRCS:.c=.o)
LIBS =

# without the -no flags, gcc generates extra, unwanted xmm code in printf...
CFLAGS = -Wall -fPIC -ffreestanding -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -nostdinc -I../../../etc/include -I. -I../../core/$(ARCH)
ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG=1
endif

all: $(OBJS) $(BINTARGET)

%.o: %.c
	@#echo "  src		$<"
	@$(CC) $(OPT) $(CFLAGS) -c $< -o $@

$(BINTARGET): $(SRCS)
	@$(CC) $(OPT) $(CFLAGS) -c $(ARCH)/crt0.S -o ../../../bin/initrd/lib/crt0.o
	@$(CC) $(OPT) $(CFLAGS) -c service.c -o ../../../bin/service.o
	@make -e --no-print-directory -C $(ARCH) all | grep -v 'Nothing to be done' || true
	@echo "  lnk		$(TARGET) ($(ARCH))"
	@$(CC) -nostdlib -nodefaultlibs -nostartfiles -shared -Bsymbolic -Bshareable -L../../../bin/initrd/lib *.o $(ARCH)/*.o -Xlinker --build-id=sha1 -T ../../core/$(ARCH)/sharedlib.ld -o $(BINTARGET) $(LIBS)

clean:
	@rm *.o *.so $(ARCH)/*.o $(BINTARGET) ../../../bin/initrd/lib/crt0.o ../../../bin/service.o 2>/dev/null || true
