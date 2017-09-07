C=gcc
CFLAGS=-I. -g -std=c99 -Wall
DEPS = watchdog.h containers.h
DEPS2 = containers.h
OBJ = watchdog.o containers.o
OBJ2 = tests.o containers.o

all: watchdog watchdog_tests

%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

watchdog: $(OBJ)
		gcc -o $@ $^ $(CFLAGS)

#%.o: %.c $(DEPS2)
#		$(CC) -c -o $@ $< $(CFLAGS)

watchdog_tests: $(OBJ2)
		gcc -o $@ $^ $(CFLAGS)
