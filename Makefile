CFLAGS=-Os
LDFLAGS=-lSDL

main: main.o synth.o

clean:
	rm -f *.o main
