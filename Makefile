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
	qglinfo \
	qhelpconverter \
	qhelpgenerator \
	qmake \
	qml1plugindump \
	qmlbundle \
	qmlmin \
	qmlplugindump \
	qmlprofiler \
	qmlscene \
	qmltestrunner \
	qmlviewer \
	qtconfig \
	rcc \
	uic \
	uic3 \
	xmlpatterns \
	xmlpatternsvalidator \

# keep the above line empty

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

uninstall:
	cd src/qtchooser && $(MAKE) uninstall
	-for tool in $(TOOLS); do rm -f "$(INSTALL_ROOT)$(bindir)/$$tool"; done

tests/auto/Makefile: tests/auto/auto.pro
	cd tests/auto && qmake -o Makefile auto.pro

check: tests/auto/Makefile
	cd src/qtchooser && $(MAKE) check
	cd tests/auto && $(MAKE) check

HEAD          = HEAD
dist: .git
	@ \
	{ rev=$$(git describe --tags $(HEAD) 2>/dev/null) && \
	  name=qtchooser-$${rev#v}; } || \
	{ rev=$$(git rev-parse --short $(HEAD)) && \
	  name=qtchooser-$$(git rev-list $(HEAD) | wc -l)-g$$rev; } && \
	echo "Creating package $$name" >&2 && \
	git archive --prefix="$$name/" -9 --format=tar.gz -o $$name.tar.gz $$rev && \
	git archive --prefix="$$name/" -9 --format=zip -v -o $$name.zip $$rev

distcheck: .git
	git archive --prefix=qtchooser-distcheck/ --format=tar $(HEAD) | tar -xf -
	cd qtchooser-distcheck && $(MAKE) check
	-rm -rf qtchooser-distcheck

.PHONY: all install uninstall check clean distclean dist
