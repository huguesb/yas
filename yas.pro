
TEMPLATE = app
TARGET = yas

CONFIG -= qt
CONFIG += readline
QMAKE_CFLAGS += -std=c99

readline {
    DEFINES += YAS_USE_READLINE
    LIBS += -lreadline -lncurses
}

HEADERS += memory.h dstring.h input.h command.h argv.h task.h exec.h
SOURCES += memory.c dstring.c input.c command.c argv.c task.c exec.c main.c
