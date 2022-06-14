CC = gcc
INCLUDE_DIR = inc -I/usr/include/SDL
CFLAGS = -Wall -ansi -std=c99
LDFLAGS = -L/usr/lib -lSDL $(sdl-config --static-libs) -lm

all: chip8

chip8: chip8.c chip8.h
	${CC} ${CFLAGS} -o $@ $^ `sdl2-config --cflags --libs`

clean:
	rm -f *.o
	