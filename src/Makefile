DRVS = $(wildcard drivers/*/*/*) $(wildcard drivers/*/*)
LIBS = $(filter-out libc drivers Makefile README.md,$(wildcard lib/*))
APPS = $(filter-out core lib drivers Makefile README.md,$(wildcard *))
ifeq (gensyms,$(firstword $(MAKECMDGOALS)))
CMD = gensyms
else
CMD = all
endif
export O

drivers: $(DRVS)

apps: $(APPS)

libs: $(LIBS)

all: system $(LIBS) $(APPS)

system:
	@make -e --no-print-directory -C core all | grep -v 'Nothing to be done' || true
	@make -e --no-print-directory -C lib/libc all | grep -v 'Nothing to be done' || true

gensyms: $(APPS)

.PHONY: Makefile

%: Makefile
	@test -f $@/Makefile && (test ! -f platforms || grep -q "$(ARCH)" platforms) && make -e -C $@ $(CMD) | grep -v 'Nothing to be done' || true

clean:
	@rm ../bin/*.o */*.o */*/*.o */*/*/*.o */*/*/*/*.o 2>/dev/null || true
