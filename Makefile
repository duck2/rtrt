CFLAGS=-O3 -g -Wall -Werror
LDFLAGS=-flto -lOpenCL

all: rtrt

rtrt: rtrt.c
	gcc $(CFLAGS) $(LDFLAGS) rtrt.c -o rtrt
