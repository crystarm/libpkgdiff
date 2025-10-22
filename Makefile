CC = gcc
CFLAGS = -fPIC -Iinclude -Wall -Wextra
LDFLAGS = -shared
TARGET_LIB = libpkgdiff.so
TARGET_CLI = compare-cli

SRC_LIB = src/pkgdiff.c
SRC_CLI = src/main.c
OBJ_LIB = pkgdiff.o

PREFIX ?= /usr/local

all: $(TARGET_LIB) $(TARGET_CLI)

$(TARGET_LIB): $(SRC_LIB)
	$(CC) $(CFLAGS) -c $(SRC_LIB) -o $(OBJ_LIB)
	$(CC) $(LDFLAGS) -o $(TARGET_LIB) $(OBJ_LIB) -lcurl -ljansson

$(TARGET_CLI): $(SRC_CLI) $(TARGET_LIB)
	$(CC) -Iinclude $(SRC_CLI) -L. -lpkgdiff -lcurl -ljansson -o $(TARGET_CLI)

install:
	install -D -m 0755 $(TARGET_LIB) $(PREFIX)/lib/$(TARGET_LIB)
	install -D -m 0755 $(TARGET_CLI) $(PREFIX)/bin/$(TARGET_CLI)
	ldconfig

clean:
	rm -f *.o $(TARGET_LIB) $(TARGET_CLI)
