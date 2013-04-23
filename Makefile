# Change the following values to suit your system.

CFLAGS=`sdl-config --cflags` -W -Wall -ggdb -O2 -fomit-frame-pointer -funroll-loops

SDL_LIB=`sdl-config --libs`
SDL_IMAGE_LIB=-lSDL_image
OPENGL_LIB=-L/usr/X11R6/include -lGL

CC=gcc

.PHONY: default joust clean converter 

default: joust.c
	$(CC) $(CFLAGS) joust.c -o joust $(SDL_LIB)

clean:
	rm -f $(SDL_TARGETS) $(IMAGE_TARGETS) $(OPENGL_TARGETS)
	rm -f *~


