CONFIG += testcase
CONFIG += parallel_test
CONFIG -= app_bundle
TARGET = tst_qtchooser
requires(contains(QT_CONFIG,private_tests))

QT     -= gui
QT     += testlib

SOURCES += tst_qtchooser.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
