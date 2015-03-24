CC = gcc
CFLAGS = -std=c11 -Wall -pedantic -I../libevdev/libevdev/\
		 -D_XOPEN_SOURCE=700

LDLIBS = -lrt -levdev -lconfig -pthread

OBJS = timer.c edgeSwipes.c

.PHONY: all
all: CFLAGS += -DDEBUG
all: clean
all:
	$(CC) $(CFLAGS) $(OBJS) $(LDLIBS) -o edgeSwipes

.PHONY: final
final: clean
final: edgeSwipes

.PHONY: clean
clean:
	@rm -f *.o edgeSwipes
