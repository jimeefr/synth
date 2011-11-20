CFLAGS=-Os -Wall

main: main.o synth.o gui.o
	ld -dynamic-linker /lib/ld-linux.so.2 main.o synth.o gui.o /usr/lib/libSDL.so /usr/lib/libGL.so /usr/lib/libGLU.so -o main
	@strip -s -R .comment -R .gnu.version main >/dev/null
	@echo "uncompressed size: " `wc -c main`
	@7z a -tGZip -mx=9 main.gz main >/dev/null
	@echo 'a=/tmp/I;tail -n+2 $$0|zcat>$$a;chmod +x $$a;$$a;exit' > main
	@cat main.gz >> main
	@rm main.gz
	@echo "compressed size:   " `wc -c main`

clean:
	rm -f *.o main
