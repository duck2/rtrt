CC=cc
CFLAGS=-O3 -g -Wall -Werror
LDFLAGS=-flto -lOpenCL -lglfw -lGLEW -lGL

all: rtrt

rtrt: rtrt.c
	$(CC) $(CFLAGS) $(LDFLAGS) rtrt.c -o rtrt
