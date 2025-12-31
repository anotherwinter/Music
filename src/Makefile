RESOURCES_DIR=./resources
SRC := $(shell find . -name '*.c')

all:
	glib-compile-resources --sourcedir=$(RESOURCES_DIR) $(RESOURCES_DIR)/resources.gresource.xml --generate-source
	glib-compile-resources --sourcedir=$(RESOURCES_DIR) $(RESOURCES_DIR)/resources.gresource.xml --generate-header
	gcc -ggdb -Wall $(shell pkg-config --cflags gtk4) -o main $(SRC) $(shell pkg-config --libs gtk4) -lvlc -ltag_c

clean:
	rm $(RESOURCES_DIR)/resources.c $(RESOURCES_DIR)/resources.h main