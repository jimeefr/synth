CFLAGS=-Os -Wall
LDFLAGS=
LSDL=-lSDL

ifeq ($(shell uname -m),x86_64)
  LSDL=-lSDL-1.2
  CFLAGS+= -m32
  LDFLAGS+= -m elf_i386
endif

main: main.o synth.o gui.o
	@echo "synth size: " `wc -c synth.o`
	ld $(LDFLAGS) -dynamic-linker /lib/ld-linux.so.2 main.o synth.o gui.o $(LSDL) -lGL -o main
	@echo "unstripped size: " `wc -c main`
	@strip -s -R .comment -R .gnu.version -R .eh_frame main >/dev/null
	@echo "uncompressed size: " `wc -c main`
	@7z a -tGZip -mx=9 main.gz main >/dev/null
	@echo 'tail -n+2 $$0|zcat>I;chmod +x I;./I;rm I;exit' > main
	@cat main.gz >> main
	@rm main.gz
	@echo "compressed size:   " `wc -c main`

clean:
	rm -f *.o main
