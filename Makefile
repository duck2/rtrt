CC=cc
CFLAGS=-O3 -g -Wall -Werror -march=native
LDFLAGS=-flto -lOpenCL -lglut -lGLEW -lGL -lm

OBJS=rtrt.o sset.o sxmlc.o v4_avx.o
HEADERS=sset.h linmath.h v4_avx.h

all: rtrt

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

rtrt: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o rtrt

clean:
	rm -f rt
	rm -f *.o

.PHONY: clean

