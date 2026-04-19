CFLAGS=-Wall -lm -lxo

PROGS=req dewpoint decode sock battsig

ALL: $(PROGS)

clean:
	rm $(PROGS)

req: req.c

dewpoint: dewpoint.c

decode: decode.c

sock: sock.c

battsig: battsig.c
