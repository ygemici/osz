include ../../../Config
TARGET = lib/libc.so

BINTARGET = ../../../bin/root/$(TARGET)
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
LIBS =
CLL  = ../../../tools/cll.o

CFLAGS = -fpic -ffreestanding

all: $(OBJS) $(BINTARGET)

%.o: %.c
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $< -o $@

$(BINTARGET): $(SRCS)
	@$(CC) -I../../../etc/include $(OPT) $(CFLAGS) -c $(ARCH)/crt.S  -o ../../../bin/root/lib/crt.o
	@echo "  src		$(TARGET) ($(ARCH))"
	@make -e --no-print-directory -C $(ARCH) all | grep -v 'Nothing to be done' || true
	@$(CLL) *.c
	@$(CC) -nostdlib -nodefaultlibs -nostartfiles -shared -Bsymbolic $(OBJS) $(filter-out crt.o,$(wildcard $(ARCH)/*.o)) cl.o -o $(BINTARGET) $(LIBS)

clean:
	@rm cl.S *.o *.so $(ARCH)/*.o $(BINTARGET) ../../../bin/root/lib/crt.o 2>/dev/null || true
