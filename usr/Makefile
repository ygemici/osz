APPS = $(filter-out Makefile README.md,$(wildcard *))
ifeq (gensyms,$(firstword $(MAKECMDGOALS)))
CMD = gensyms
else
CMD = all
endif

all: $(APPS)

gensyms: $(APPS)

.PHONY: Makefile

%: Makefile
	@test -f $@/Makefile && make -e -C $@ $(CMD) | grep -v 'Nothing to be done' || true

clean:
	@rm -rf ../bin/usr */*.o */*/*.o */*/*/*.o */*/*/*/*.o 2>/dev/null || true
