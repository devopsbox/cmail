.PHONY: clean
CFLAGS?=-Wall -g
LDLIBS=-lsqlite3 -lnettle -lgnutls

all: popd
	mv popd cmail-popd

popd: popd.c

run: all
	valgrind --leak-check=full ./cmail-popd devel-popd.conf nodetach

clean:
	$(RM) cmail-popd
