PREFIX=m68k-atari-mint
CC=$(PREFIX)-gcc-4.6.4

CFLAGS=-std=gnu99 -Wall -Wno-array-bounds -Os -mshort -mfastcall

LIBCMINI_DIR=$(HOME)/src/libcmini/build
LINK=-nostdlib $(LIBCMINI_DIR)/crt0.o $< memset.o -L$(LIBCMINI_DIR)/mshort/mfastcall $(LDFLAGS) -lcmini -lgcc -o $@
#LINK=$< $(LDFLAGS) -o $@

TARGETS=wordlist.h curdlest.prg CurdleST.zip CurdleST.st

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

CurdleST.zip: curdlest.prg CURDLEST.RSC README.TXT
	@rm -f $@
	@7za a -mx9 -bb1 $@ AUTO/AUTODATE.PRG $^ 

CurdleST.st: curdlest.prg CURDLEST.RSC README.TXT
	rm -f $@
	mkdosfs -n CURDLEST -C $@ 720
	@mmd -i $@ AUTO
	@mcopy -v -m -i $@ AUTO/AUTOWARP.PRG AUTO/WARP9_ST.PRX ::/AUTO
	@mcopy -v -m -i $@ AUTO/AUTODATE.PRG ::/AUTO
	@mcopy -v -m -i $@ DESKTOP.INF $^ ::/
