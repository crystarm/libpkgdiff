CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -fPIC -fvisibility=hidden
CFLAGS += -Iinclude -Isrc/lib -Isrc/cli
# Try pkg-config for cflags too
CFLAGS += $(shell pkg-config --cflags libcurl jansson 2>/dev/null)

LDLIBS += $(shell pkg-config --libs libcurl jansson 2>/dev/null)
ifeq ($(strip $(LDLIBS)),)
LDLIBS += -lcurl -ljansson
endif

LIBSO := libpkgdiff.so
BIN   := pkgdiff

LIBSRC := \
    src/lib/util.c \
    src/lib/net.c  \
    src/lib/cmp.c

CLISRC := \
    src/cli/main.c \
    src/cli/ui.c

LIBOBJ := $(LIBSRC:.c=.o)
CLIOBJ := $(CLISRC:.c=.o)

.PHONY: all clean rpm install

all: $(LIBSO) $(BIN)


# Ensure exported symbols are visible from lib by defining BUILDING_PKGDIFF
src/lib/%.o: src/lib/%.c
	$(CC) $(CFLAGS) -DBUILDING_PKGDIFF -c -o $@ $<

$(LIBSO): $(LIBOBJ)
	$(CC) -shared -o $@ $(LIBOBJ)

$(BIN): $(CLIOBJ) $(LIBSO)
	$(CC) -o $@ $(CLIOBJ) -L. -Wl,--no-as-needed -lpkgdiff -Wl,--as-needed $(LDLIBS) -Wl,-rpath,'$$ORIGIN'

clean:
	rm -f $(LIBOBJ) $(CLIOBJ) $(LIBSO) $(BIN)

# Optional RPM build; requires packaging/libpkgdiff.spec
rpm: all
	@if [ ! -f packaging/libpkgdiff.spec ]; then \
	  echo "packaging/libpkgdiff.spec not found"; exit 1; \
	fi
	rpmbuild --define "_sourcedir $(PWD)" \
	         --define "_specdir $(PWD)/packaging" \
	         --define "_builddir $(PWD)/build" \
	         --define "_srcrpmdir $(PWD)/dist" \
	         --define "_rpmdir $(PWD)/dist" \
	         -ba packaging/libpkgdiff.spec

install:
	@echo "Add your 'install' logic or use packaging/"
