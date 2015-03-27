CC = gcc
CFLAGS = -std=c11 -Wall -pedantic \
		 -I../libs/libevdev/libevdev/ \
		 -D_XOPEN_SOURCE=700

LDLIBS = -lrt -levdev -lconfig -pthread

OBJS = tapper.c edgeSwipes.c

.PHONY: all
all: CFLAGS += -DDEBUG
all: clean
all: stuff

stuff:
	$(CC) $(CFLAGS) $(OBJS) $(LDLIBS) -o edgeSwipes

.PHONY: final
final: clean
final: stuff

.PHONY: clean
clean:
	@rm -f *.o edgeSwipes
