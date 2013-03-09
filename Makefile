CFLAGS=-Os -fomit-frame-pointer -nostartfiles -lSDL -lGL

invitro.min: invitro
	strip -s -R .comment -R .gnu.version $^
	sstrip -z $^
	cp unpack.header $@
	gzip -cn9 $^ >> $@
	chmod +x $@

invitro: invitro.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f invitro invitro.o invitro.min
