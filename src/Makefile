DRVS = $(wildcard drv/*/*/*) $(wildcard drv/*/*)
LIBS = $(filter-out libc drv Makefile,$(wildcard lib/*))
APPS = $(filter-out core lib Makefile,$(wildcard *))

drivers: $(DRVS)
	@make -e -C drv all | grep -v 'Nothing to be done' || true

all: $(LIBS) $(APPS)

.PHONY: Makefile

%: Makefile
	test -f $@/Makefile && (test ! -f platforms || grep -q "$(ARCH)" platforms) && make -e -C $@ all | grep -v 'Nothing to be done' || true

clean:
	@rm */*.o */*/*.o */*/*/*.o */*/*/*/*.o 2>/dev/null || true
