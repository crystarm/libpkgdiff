# Compiler and flags
CC       = gcc
CFLAGS   = -fPIC -Iinclude -Isrc -Wall -Wextra -O2 -fvisibility=hidden
LDFLAGS  = -shared

# Targets
TARGET_LIB = libpkgdiff.so
TARGET_CLI = pkgdiff

# Sources
SRC_LIB = src/u.c src/ui.c src/net.c src/cmp.c
SRC_CLI = src/main.c
OBJ_LIB = $(SRC_LIB:.c=.o)

# Install prefixes
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib

# Auto-switch to lib64 on common 64‑bit arches if LIBDIR wasn't overridden
UNAME_M := $(shell uname -m)
ifeq ($(LIBDIR),$(PREFIX)/lib)
  ifneq (,$(filter x86_64 aarch64 ppc64le s390x,$(UNAME_M)))
    LIBDIR := $(PREFIX)/lib64
  endif
endif

# RPATH so the CLI can find the just-built .so from the build tree
CLI_RPATH = -Wl,-rpath,'$$ORIGIN'

.PHONY: all clean install rpm

all: $(TARGET_LIB) $(TARGET_CLI)

$(TARGET_LIB): $(OBJ_LIB)
	$(CC) $(LDFLAGS) -o $@ $(OBJ_LIB) -lcurl -ljansson

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_CLI): $(SRC_CLI) $(TARGET_LIB)
	$(CC) -Iinclude $(CLI_RPATH) $(SRC_CLI) -L. -lpkgdiff -lcurl -ljansson -o $@

install:
	install -d "$(DESTDIR)$(BINDIR)" "$(DESTDIR)$(LIBDIR)"
	install -m 0755 "$(TARGET_CLI)" "$(DESTDIR)$(BINDIR)/$(TARGET_CLI)"
	install -m 0644 "$(TARGET_LIB)" "$(DESTDIR)$(LIBDIR)/$(TARGET_LIB)"
	@# Run ldconfig only when installing to the real root (not during RPM build with DESTDIR)
	@if [ -z "$(DESTDIR)" ]; then echo "Running ldconfig"; ldconfig; fi

clean:
	rm -f src/*.o "$(TARGET_LIB)" "$(TARGET_CLI)"

# Build RPM from the current Git HEAD using rpmdevtools
rpm:
	@command -v rpmbuild >/dev/null || { echo "rpmbuild not found. Install: rpm-build rpmdevtools"; exit 1; }
	@command -v rpmdev-setuptree >/dev/null || { echo "rpmdevtools not found. Install: rpmdevtools"; exit 1; }
	rpmdev-setuptree
	@VER=$$(git describe --tags --always 2>/dev/null | sed 's/^v//' || echo 0.1.0); \
	mkdir -p $$HOME/rpmbuild/SOURCES; \
	git archive --format=tar.gz --prefix=libpkgdiff-$$VER/ -o $$HOME/rpmbuild/SOURCES/libpkgdiff-$$VER.tar.gz HEAD; \
	install -Dm0644 packaging/libpkgdiff.spec $$HOME/rpmbuild/SPECS/libpkgdiff.spec; \
	echo "Building RPM with version $$VER"; \
	rpmbuild -ba $$HOME/rpmbuild/SPECS/libpkgdiff.spec
