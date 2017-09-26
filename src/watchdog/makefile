C=gcc
CFLAGS=-I. -I ../dependencies/libart/src -g -std=c99 -Wall
DEPS = watchdog.h containers.h ../dependencies/libart/src/art.h
DEPS2 = containers.h
OBJ = watchdog.o containers.o ../dependencies/libart/src/art.o
OBJ2 = tests.o containers.o ../dependencies/libart/src/art.o

all: watchdog watchdog_tests

%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

watchdog: $(OBJ)
		gcc -o $@ $^ $(CFLAGS)

#%.o: %.c $(DEPS2)
#		$(CC) -c -o $@ $< $(CFLAGS)

watchdog_tests: $(OBJ2)
		gcc -o $@ $^ $(CFLAGS)
