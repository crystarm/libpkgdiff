# Project layout migration

This repository was restructured to a clearer separation of library vs CLI:

```
include/           # public API headers (pkgdiff.h)
src/
  lib/             # internal library sources/headers
    ansi.h
    net.c net.h
    cmp.c
    util.c util.h
  cli/             # CLI frontend
    main.c
    ui.c ui.h
containers/
  Containerfile    # two-stage build
packaging/
  libpkgdiff.spec  # RPM skeleton
```

Notable changes:
- `src/u.c`/`src/u.h` renamed to `src/lib/util.c`/`src/lib/util.h`.
- UI is internal (no `PKGDIFF_API` in UI); `hello_pkgdiff` is not exported in `include/pkgdiff.h`.
- New `Makefile` builds `libpkgdiff.so` and `pkgdiff` with `-Iinclude -Isrc/lib -Isrc/cli`.
- Added `.gitignore` with sane defaults.
- Removed built artifacts from the tree.
