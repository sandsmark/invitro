CFLAGS=-Os -fomit-frame-pointer -lSDL -lGL

#invitro.min: invitro
#	strip -s -R .comment -R .gnu.version $^
#	sstrip -z $^
#	cp unpack.header $@
#	gzip -cn9 $^ >> $@

invitro: invitro.o
	$(LD) -dynamic-linker /lib/ld-linux.so.2 $^ /usr/lib/libSDL.so /usr/lib/libGL.so /usr/lib/libpthread.so /usr/lib/libc.so -o $@

invitro.o: invitro.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -f invitro invitro.o invitro.min
