LDLIBS = -lmpdclient

main:
	mkdir .tmp
	glib-compile-resources --target=.tmp/resources.c --generate-source resources/window.xml
	gcc $$(pkg-config gtk+-3.0 --cflags) main.c .tmp/resources.c -o main $$(pkg-config gtk+-3.0 --libs) $(LDLIBS) -g3
	rm -rf .tmp

clean:
	rm -rf .tmp
	rm main
