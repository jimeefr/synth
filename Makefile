CFLAGS=-Os

main: main.o synth.o
	ld -dynamic-linker /lib/ld-linux.so.2 main.o synth.o /usr/lib/libSDL.so -o main
	@strip -s -R .comment -R .gnu.version main >/dev/null
	@7z a -tGZip -mx=9 main.gz main >/dev/null
	@echo 'dd bs=1 skip=73<$$0 2>/dev/null|gunzip>/tmp/C;chmod +x /tmp/C;/tmp/C;exit' > main
	@cat main.gz >> main
	@rm main.gz

clean:
	rm -f *.o main
