.PHONY: all libc programs clean

all: libc programs

libc:
	$(MAKE) -C lib/libc

programs: libc
	$(MAKE) -C bin/sh
	$(MAKE) -C bin/hello
	$(MAKE) -C bin/ls
	$(MAKE) -C bin/touch
	$(MAKE) -C bin/cat 

clean:
	$(MAKE) -C lib/libc clean
	$(MAKE) -C bin/sh clean
	$(MAKE) -C bin/hello clean
	$(MAKE) -C bin/ls clean
	$(MAKE) -C bin/touch clean
	$(MAKE) -C bin/cat clean

.PHONY: all libc programs clean
