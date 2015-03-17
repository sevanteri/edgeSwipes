CFLAGS = -std=c11 -levdev -I../libevdev/libevdev/

.PHONY: all
all: edgeSwipes

.PHONY: clean
clean:
	@rm -f *.o edgeSwipes
