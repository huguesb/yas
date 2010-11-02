
TEMPLATE = app
TARGET = yas

CONFIG -= qt
CONFIG += readline
QMAKE_CFLAGS += -std=c99

readline {
    DEFINES += YAS_USE_READLINE
    LIBS += -lreadline -lncurses
}

HEADERS += memory.h dstring.h input.h command.h exec.h
SOURCES += memory.c dstring.c input.c command.c exec.c main.c
