APPS = $(filter-out core libc Makefile,$(wildcard *))

all: $(APPS)

.PHONY: Makefile

%: Makefile
	@test -f $@/Makefile && make -e -C $@ all | grep -v 'Nothing to be done' || true

clean:
	@rm */*.o */*/*.o 2>/dev/null || true
