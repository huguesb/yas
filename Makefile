
CC            = gcc
DEFINES       = -DYAS_USE_READLINE
CFLAGS        = -pipe -std=c99 -O2 -pipe -Wall -Wextra -W $(DEFINES)
LINK          = gcc
LFLAGS        = 
LIBS          = -lreadline -lncurses
DEL_FILE      = rm -f
SYMLINK       = ln -f -s
DEL_DIR       = rmdir
MOVE          = mv -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p

####### Output directory

OBJECTS_DIR   = ./

####### Files

SOURCES       = memory.c \
		dstring.c \
		input.c \
		command.c \
		argv.c \
		task.c \
		exec.c \
		main.c 
OBJECTS       = memory.o \
		dstring.o \
		input.o \
		command.o \
		argv.o \
		task.o \
		exec.o \
		main.o
DESTDIR       = 
TARGET        = yas

first: all

####### Build rules

all: $(TARGET)

$(TARGET):  $(OBJECTS)  
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)


clean: FORCE 
	-$(DEL_FILE) $(OBJECTS)

####### Compile

memory.o: memory.c memory.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o memory.o memory.c

dstring.o: dstring.c dstring.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o dstring.o dstring.c

input.o: input.c input.h \
		memory.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o input.o input.c

command.o: command.c command.h \
		memory.h \
		dstring.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o command.o command.c

argv.o: argv.c argv.h \
		memory.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o argv.o argv.c

task.o: task.c task.h \
		command.h \
		argv.h \
		memory.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o task.o task.c

exec.o: exec.c exec.h \
		command.h \
		memory.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o exec.o exec.c

main.o: main.c memory.h \
		input.h \
		command.h \
		exec.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o main.o main.c

FORCE:
