CFLAGS=-Os -fomit-frame-pointer -nostartfiles -lSDL -lGL -std=gnu99 -g

invitro: invitro.c
	$(CC) $(CFLAGS) -o $@ $^

invitro.min: invitro
	strip -s -R .comment -R .gnu.version $^
	sstrip -z $^
	cp unpack.header $@
	zopfli -c $^ >> $@
	chmod +x $@

clean:
	rm -f invitro invitro.o invitro.min
