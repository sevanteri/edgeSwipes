CC = gcc
CFLAGS = -std=c11 -Wall -pedantic -I../libevdev/libevdev/\
		 -D_XOPEN_SOURCE=700

LDLIBS = -lrt -levdev -lconfig -pthread

OBJS = tapper.c edgeSwipes.c

stuff:
	$(CC) $(CFLAGS) $(OBJS) $(LDLIBS) -o edgeSwipes

.PHONY: all
all: CFLAGS += -DDEBUG
all: clean
all: stuff

.PHONY: final
final: clean
final: stuff

.PHONY: clean
clean:
	@rm -f *.o edgeSwipes
