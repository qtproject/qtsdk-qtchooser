prefix = /usr
bindir = $(prefix)/bin

####### Compiler, tools and options

INSTALL_PROGRAM = install -m 755 -p
DEL_FILE      = rm -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p

####### Files

SOURCES       = main.cpp
OBJECTS       = main.o
TARGET        = qtchooser

OBJECTS_TEST  = main-test.o
TARGET_TEST   = test/qtchooser

first: all
check: $(TARGET_TEST)

####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(OBJECTS)
	$(CXX) $(LFLAGS) -o $(TARGET) $(OBJECTS)

$(TARGET_TEST):  $(OBJECTS_TEST)
	$(MKDIR) test
	$(CXX) $(LFLAGS) -o $(TARGET_TEST) $(OBJECTS_TEST)

clean:
	-$(DEL_FILE) $(OBJECTS) $(OBJECTS_TEST)
	-$(DEL_FILE) *~ core *.core

distclean: clean
	-$(DEL_FILE) $(TARGET) $(TARGET_TEST)

install: $(TARGET)
	$(MKDIR) "$(INSTALL_ROOT)$(bindir)"
	$(INSTALL_PROGRAM) $(TARGET) "$(INSTALL_ROOT)$(bindir)/$(TARGET)"

uninstall:
	-$(DEL_FILE) "$(INSTALL_ROOT)$(bindir)/$(TARGET)"


####### Compile

main.o: main.cpp
	$(CXX) -c -Wall -Wextra $(CXXFLAGS) $(INCPATH) -o main.o main.cpp

main-test.o: main.cpp
	$(CXX) -c -Wall -Wextra -DQTCHOOSER_TEST_MODE -g $(CXXFLAGS) $(INCPATH) -o main-test.o main.cpp

####### Install

install:   FORCE

uninstall:   FORCE

FORCE:

