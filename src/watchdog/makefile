C=gcc
CFLAGS=-I. -I ../dependencies/libart/src -g -std=c99 -Wall -lpthread
DEPS = watchdog.h containers.h ../dependencies/libart/src/art.h definitions.h
DEPS2 = containers.h definitions.h
OBJ = watchdog.o containers.o ../dependencies/libart/src/art.o definitions.o
OBJ2 = tests.o containers.o ../dependencies/libart/src/art.o definitions.o

all: watchdog watchdog_ctrl watchdog_tests

%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

watchdog: $(OBJ)
		gcc -o $@ $^ $(CFLAGS)

#%.o: %.c $(DEPS2)
#		$(CC) -c -o $@ $< $(CFLAGS)

watchdog_tests: $(OBJ2)
		gcc -o $@ $^ $(CFLAGS)

watchdog_ctrl: watchdog_ctrl.o
		gcc watchdog_ctrl.c -o watchdog_ctrl
