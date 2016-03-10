MKDIR  = mkdir -p
prefix = /usr
bindir = $(prefix)/bin
TOOLS = assistant \
	designer \
	lconvert \
	linguist \
	lrelease \
	lupdate \
	moc \
	pixeltool \
	qcollectiongenerator \
	qdbus \
	qdbuscpp2xml \
	qdbusviewer \
	qdbusxml2cpp \
	qdoc \
	qdoc3 \
	qhelpconverter \
	qhelpgenerator \
	qlalr \
	qmake \
	qml \
	qml1plugindump \
	qmlbundle \
	qmleasing \
	qmlimportscanner \
	qmllint \
	qmlmin \
	qmlplugindump \
	qmlprofiler \
	qmlscene \
	qmltestrunner \
	qmlviewer \
	qtconfig \
	qtdiag \
	qtpaths \
	qtplugininfo \
	rcc \
	uic \
	uic3 \
	xmlpatterns \
	xmlpatternsvalidator \

# keep the above line empty

MACTOOLS = macdeployqt
QMAKE = qmake

all:
	cd src/qtchooser && $(MAKE)

clean:
	-cd src/qtchooser && $(MAKE) clean
	-cd tests/auto && $(MAKE) clean

distclean:
	-cd src/qtchooser && $(MAKE) distclean
	-cd tests/auto && $(MAKE) distclean

install:
	cd src/qtchooser && $(MAKE) install
	for tool in $(TOOLS); do ln -sf qtchooser "$(INSTALL_ROOT)$(bindir)/$$tool"; done
	case `uname -s` in Darwin) \
	    for tool in $(MACTOOLS); do ln -sf qtchooser "$(INSTALL_ROOT)$(bindir)/$$tool"; done \
	;; esac
	$(MKDIR) $(INSTALL_ROOT)$(prefix)/share/man/man1
	install -m 644 -p doc/qtchooser.1 $(INSTALL_ROOT)$(prefix)/share/man/man1

uninstall:
	cd src/qtchooser && $(MAKE) uninstall
	-for tool in $(TOOLS); do rm -f "$(INSTALL_ROOT)$(bindir)/$$tool"; done
	case `uname -s` in Darwin) \
	    for tool in $(MACTOOLS); do rm -f "$(INSTALL_ROOT)$(bindir)/$$tool"; done \
	;; esac

tests/auto/Makefile: tests/auto/auto.pro
	cd tests/auto && $(QMAKE) -o Makefile auto.pro

check: all tests/auto/Makefile
	cd src/qtchooser && $(MAKE) check
	cd tests/auto && $(MAKE) check

HEAD          = HEAD
dist: .git
	@ \
	{ rev=$$(git describe --exact-match --tags $(HEAD) 2>/dev/null) && \
	  name=qtchooser-$${rev#v}; } || \
	{ rev=$$(git rev-parse --short $(HEAD)) && \
	  name=qtchooser-$$(git rev-list $(HEAD) | wc -l)-g$$rev; } && \
	echo "Creating package $$name" >&2 && \
	git archive --prefix="$$name/" -9 --format=tar.gz -o $$name.tar.gz $$rev && \
	git archive --prefix="$$name/" -9 --format=zip -v -o $$name.zip $$rev

tagdist: .git
	@git describe --exact-match --tags $(HEAD) >/dev/null 2>&1 || { \
	  n=$$(git rev-list --count $(HEAD)) && \
	  echo "Creating tag v$$n" && \
	  git tag -a -m "qtchooser release $$n" v$$n; }
	$(MAKE) dist

distcheck: .git
	git archive --prefix=qtchooser-distcheck/ --format=tar $(HEAD) | tar -xf -
	cd qtchooser-distcheck && $(MAKE) check
	-rm -rf qtchooser-distcheck

.PHONY: all install uninstall check clean distclean dist tagdist distcheck
