CFLAGS = -std=c11 -Wall -pedantic -levdev -I../libevdev/libevdev/

.PHONY: all
all: edgeSwipes

.PHONY: debug
debug: CFLAGS += -DDEBUG
debug: clean
debug: all

.PHONY: clean
clean:
	@rm -f *.o edgeSwipes
