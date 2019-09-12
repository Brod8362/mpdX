CFLAGS = $(shell pkg-config gtk+-3.0 --cflags) -g3
LIBS = $(shell pkg-config gtk+-3.0 --libs) -lmpdclient

main: resources.c main.c mpd_actions.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

resources.c: resources/window.xml
	glib-compile-resources --target=$@ --generate-source $^

clean:
	rm -rf .tmp
	rm -f main *.o
	rm resources.c

.PHONY: clean
