all:
	glib-compile-resources resources.gresource.xml --generate-source
	glib-compile-resources resources.gresource.xml --generate-header
	gcc -Wall $(shell pkg-config --cflags gtk4) -o main ./*.c $(shell pkg-config --libs gtk4) -lvlc -ltag_c

clean:
	rm resources.c resources.h main