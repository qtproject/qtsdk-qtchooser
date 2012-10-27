all:
	cd src/qtchooser && $(MAKE)

install:
	cd src/qtchooser && $(MAKE) install

uninstall:
	cd src/qtchooser && $(MAKE) uninstall

tests/auto/Makefile: tests/auto/auto.pro
	cd tests/auto && qmake -o Makefile auto.pro

check: all tests/auto/Makefile
	cd tests/auto && $(MAKE) check

.PHONY: all install uninstall check
