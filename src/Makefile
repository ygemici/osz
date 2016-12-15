DRVS = $(wildcard drv/*/*/*) $(wildcard drv/*/*)
LIBS = $(filter-out libc Makefile,$(wildcard lib/*))
APPS = $(filter-out core lib Makefile,$(wildcard *))

drivers: $(DRVS)
	@cat `find drv -name pci.ids|sort` >../bin/root/etc/pci.ids
	@$(CC) -D __executable_start=0 -nostdlib -nodefaultlibs -nostartfiles ../bin/root/lib/crt0.o drv/main.c -T executable.ld -Xlinker --build-id=none -o ../bin/root/sbin/system

all: $(LIBS) $(APPS)

.PHONY: Makefile

%: Makefile
	@test -f $@/Makefile && make -e -C $@ all | grep -v 'Nothing to be done' || true

clean:
	@rm */*.o */*/*.o */*/*/*.o */*/*/*/*.o ../bin/root/sbin/system 2>/dev/null || true
