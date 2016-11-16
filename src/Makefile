DRVS = $(wildcard drv/*/*/*) $(wildcard drv/*/*) $(wildcard drv/*)
LIBS = $(filter-out libc Makefile,$(wildcard lib/*))
APPS = $(filter-out core lib Makefile,$(wildcard *))

drivers: $(DRVS)
	@cat `find drv -name pci.ids|sort` >../bin/root/etc/pci.ids

all: $(LIBS) $(APPS)

.PHONY: Makefile

%: Makefile
	@test -f $@/Makefile && make -e -C $@ all | grep -v 'Nothing to be done' || true

clean:
	@rm */*.o */*/*.o 2>/dev/null || true
