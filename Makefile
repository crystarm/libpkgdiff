    CC = gcc
    CFLAGS = -fPIC -Iinclude -Isrc -Wall -Wextra -O2 -fvisibility=hidden
    LDFLAGS = -shared
    TARGET_LIB = libpkgdiff.so
    TARGET_CLI = compare-cli
    SRC_LIB = src/u.c src/ui.c src/net.c src/cmp.c
    SRC_CLI = src/main.c
    OBJ_LIB = $(SRC_LIB:.c=.o)
    PREFIX ?= /usr/local

    all: $(TARGET_LIB) $(TARGET_CLI)

    $(TARGET_LIB): $(OBJ_LIB)
	$(CC) $(LDFLAGS) -o $(TARGET_LIB) $(OBJ_LIB) -lcurl -ljansson

    src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

    $(TARGET_CLI): $(SRC_CLI) $(TARGET_LIB)
	$(CC) -Iinclude $(SRC_CLI) -L. -lpkgdiff -lcurl -ljansson -o $(TARGET_CLI)

    install:
	install -D -m 0755 $(TARGET_LIB) $(PREFIX)/lib/$(TARGET_LIB)
	install -D -m 0755 $(TARGET_CLI) $(PREFIX)/bin/$(TARGET_CLI)
	ldconfig

    clean:
	rm -f src/*.o $(TARGET_LIB) $(TARGET_CLI)