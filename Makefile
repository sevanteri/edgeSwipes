CFLAGS = -std=c11 -Wall -pedantic -levdev -I../libevdev/libevdev/

.PHONY: all
all: CFLAGS += -DDEBUG
all: clean
all: edgeSwipes

.PHONY: final
final: clean
final: edgeSwipes

.PHONY: clean
clean:
	@rm -f *.o edgeSwipes
