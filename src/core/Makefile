include ../../etc/sys/etc/os-release
include ../../Config
TARGET = sys/core

BINTARGET = ../../bin/initrd/$(TARGET)
SRCS = $(filter-out dbg.c,$(wildcard *.c))
#ifeq $($(DEBUG),1)
SRCS += dbg.c
#endif
OBJS = $(SRCS:.c=.o)
LIBS =
LDFLAGS = -nostdlib -nostartfiles --build-id=none -x
ARCHOBJS = $(filter-out $(ARCH)/start.o,$(wildcard $(ARCH)/*.o))
ARCHOBJS+= $(wildcard $(ARCH)/$(PLATFORM)/*.o)
CFLAGS = -DDEBUG=$(DEBUG) -DOPTIMIZE=$(OPTIMIZE) -DOSZ_NAME='$(NAME)' -DOSZ_VER='$(VERSION)' \
 -DOSZ_ARCH='"$(ARCH)"' -DOSZ_PLATFORM='"$(PLATFORM)"' -DOSZ_BUILD='$(BUILD)' \
 -fpic -fno-stack-protector -nostdlib -nostdinc -I. -I./$(ARCH) -I./$(ARCH)/$(PLATFORM) -I../../etc/include \
 -Wall -ffreestanding
# gcc with -O2 generates ud2/brk instruction...
ifeq ($(ARCH),x86_64)
CFLAGS += -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-sse3
else
CFLAGS +=
endif
ifeq ($(DEBUG),1)
CFLAGS +=
else
LDFLAGS += -s
endif

all: $(OBJS) $(BINTARGET)

main.o: main.c ../../etc/sys/etc/os-release
	@#echo "  src		$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c
	@#echo "  src		$<"
	$(O)$(CC) $(OPT) $(CFLAGS) -c $< -o $@

platform:
	$(O)make -e --no-print-directory -C $(ARCH)/$(PLATFORM) all | grep -v 'Nothing to be done' || true
	$(O)make -e --no-print-directory -C $(ARCH) all | grep -v 'Nothing to be done' || true

font.o: ../../etc/system.8x16.psf
	@cp ../../etc/system.8x16.psf ./font
	$(O)$(LD) -r -b binary -o font.o font
	@rm font

logo.o: ../../etc/logo.tga
	@cp ../../etc/logo.tga ./logo
	$(O)$(LD) -r -b binary -o logo.o logo
	@rm logo

$(BINTARGET): platform font.o logo.o $(SRCS)
	@echo "  lnk		$(TARGET) ($(ARCH)-$(PLATFORM))"
	$(O)$(LD) $(LDFLAGS) -T link.ld $(ARCH)/start.o $(OBJS) $(ARCHOBJS) font.o logo.o -o $(BINTARGET) $(LIBS)
ifeq ($(DEBUG),1)
	@echo "0000000000200000 _init" >../../bin/core.sym
	@../../tools/elftool.o -s 0 $(BINTARGET) | sort >>../../bin/core.sym
endif
	@# avoid accidental execution of core
	@#echo -n "OS/Z" | dd conv=notrunc of=$(BINTARGET) bs=1 seek=0 1>/dev/null 2>/dev/null
	@echo -ne '\003' | dd conv=notrunc of=$(BINTARGET) bs=1 seek=16 1>/dev/null 2>/dev/null
	@# create an empty drivers file
	@touch ../../bin/initrd/sys/drivers

clean:
	@rm *.o $(ARCH)/*.o $(ARCH)/$(PLATFORM)/*.o $(BINTARGET) ../../bin/*.sym 2>/dev/null || true
