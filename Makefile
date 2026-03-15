cc = gcc
target = main

uname_s := $(shell uname -s)

ifeq ($(uname_s),Darwin)
	cflags = -I/opt/X11/include
	ldflags = -L/opt/X11/lib -lX11 -lm
else ifeq ($(uname_s),Linux)
	cflags =
	ldflags = -lX11 -lm
else
	cflags =
	ldflags =
endif

felix:
	$(cc) $(cflags) -o $(target) $(target).c $(ldflags)
	./$(target)

.PHONY: felix
