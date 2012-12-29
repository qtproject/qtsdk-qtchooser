CONFIG += testcase
CONFIG += parallel_test
CONFIG -= app_bundle
TARGET = tst_qtchooser

QT     -= gui
QT     += testlib

SOURCES += tst_qtchooser.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
