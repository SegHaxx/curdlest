PREFIX=m68k-atari-mint
CC=$(PREFIX)-gcc-4.6.4

CFLAGS=-std=gnu99 -Wall -Wno-array-bounds -Os -mshort -mfastcall

LIBCMINI_DIR=$(HOME)/src/libcmini/build
LINK=-nostdlib $(LIBCMINI_DIR)/crt0.o $< memset.o -L$(LIBCMINI_DIR)/mshort/mfastcall $(LDFLAGS) -lcmini -lgcc -o $@
#LINK=$< $(LDFLAGS) -o $@

TARGETS=wordlist.h curdlest.prg

src = $(wildcard *.c)
obj = $(src:.c=.o)

.PHONY: all clean
all: $(TARGETS)
clean:
	rm -f $(TARGETS) $(obj)

curdlest.prg: curdlest.c curdle.c stats.c memset.o wordlist.h
	$(CC) $(CFLAGS) $(LINK)
	$(PREFIX)-objdump -drwC $@ > $@.s
	$(PREFIX)-strip -s $@
	@$(PREFIX)-size $@
	@du -b $@

memset.o: memset.c
	$(CC) $(CFLAGS) -c $< -o $@

wordlist.h: wordlist.txt list2h.pl
	./list2h.pl < $< > $@
