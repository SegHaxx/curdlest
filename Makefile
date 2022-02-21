PREFIX=m68k-atari-mint
CC=$(PREFIX)-gcc-4.6.4

CFLAGS=-std=gnu99 -Wall -Wno-array-bounds -Os -mshort -mfastcall #-DNDEBUG

LIBCMINI_DIR=$(HOME)/src/libcmini/build
LINK=-nostdlib $(LIBCMINI_DIR)/crt0.o $< -L$(LIBCMINI_DIR)/mshort/mfastcall $(LDFLAGS) -lcmini -lgcc -o $@
#LINK=$< $(LDFLAGS) -o $@

TARGETS=wordlist.h curdlest.prg

src = $(wildcard *.c)
obj = $(src:.c=.o)

.PHONY: all clean
all: $(TARGETS)
clean:
	rm -f $(TARGETS) $(obj)

curdlest.prg: curdlest.c curdle.c stats.c wordlist.h
	$(CC) $(CFLAGS) $(LINK)
	$(PREFIX)-objdump -drwC $@ > $@.s
	$(PREFIX)-strip -s $@
	@$(PREFIX)-size $@
	@du -b $@

wordlist.h: wordlist.txt list2h.pl
	./list2h.pl < $< > $@