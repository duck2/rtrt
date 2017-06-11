CC=cc
CFLAGS=-O3 -g -Wall -Werror
LDFLAGS=-flto -lOpenCL -lglut -lGLEW -lGL

all: rtrt

rtrt: rtrt.c scene.h
	$(CC) $(CFLAGS)  rtrt.c -o rtrt  $(LDFLAGS)
