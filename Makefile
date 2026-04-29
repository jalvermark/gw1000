CFLAGS=-Wall -lm -lxo

PROGS=req dewpoint decode getwx battsig

ALL: $(PROGS)

clean:
	rm $(PROGS)

req: req.c

dewpoint: dewpoint.c

decode: decode.c

getwx: getwx.c

battsig: battsig.c
