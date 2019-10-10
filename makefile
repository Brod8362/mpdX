CFLAGS = $(shell pkg-config gtk+-3.0 --cflags) -g3
LIBS = $(shell pkg-config gtk+-3.0 --libs) -lmpdclient

main: resources.o main.o mpd_actions.o actions.o song_search_dialog.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

resources.c: resources/window.xml
	glib-compile-resources --target=$@ --generate-source $^

clean:
	rm -rf .tmp
	rm -f main *.o
	rm -f resources.c

.PHONY: clean
