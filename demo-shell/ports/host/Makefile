# Host build of the demo shell, for trying the command set without hardware.
build/demo-shell-host: ../../src/shell.c ../../src/shell.h port.c
	@mkdir -p build
	cc -Os -Wall -Wextra -o $@ ../../src/shell.c port.c

clean:
	rm -rf build

.PHONY: clean
