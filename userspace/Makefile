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
	$(MAKE) -C bin/mkdir
	$(MAKE) -C bin/rmdir
	$(MAKE) -C bin/rm
	$(MAKE) -C bin/time
	$(MAKE) -C bin/rmmod
	$(MAKE) -C bin/insmod
	$(MAKE) -C bin/mount

clean:
	$(MAKE) -C lib/libc clean
	$(MAKE) -C bin/sh clean
	$(MAKE) -C bin/hello clean
	$(MAKE) -C bin/ls clean
	$(MAKE) -C bin/touch clean
	$(MAKE) -C bin/cat clean
	$(MAKE) -C bin/mkdir clean
	$(MAKE) -C bin/rmdir clean
	$(MAKE) -C bin/rm clean
	$(MAKE) -C bin/time clean
	$(MAKE) -C bin/rmmod clean
	$(MAKE) -C bin/insmod clean
	$(MAKE) -C bin/mount clean

.PHONY: all libc programs clean
