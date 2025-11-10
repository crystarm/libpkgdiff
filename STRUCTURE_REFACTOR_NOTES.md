
# Structural refactor notes

- Public header `include/pkgdiff.h` now contains **only** the library API and filter constants.
- UI coloring macros moved to internal header `src/ansi.h`.
- UI splash prototype moved to internal `src/ui.h`.
- Networking prototype for `fetch_packages_json()` moved to internal `src/net.h`.
- Source files that print colored output now `#include "ansi.h"`.
- `main.c` that calls `hello_pkgdiff()` now `#include "ui.h"`.
- Any unit that calls `fetch_packages_json()` now `#include "net.h"`.
- Build artifacts removed from the archive (`*.o`, `libpkgdiff.so`, `pkgdiff`).

This keeps the public API surface small and removes UI details from the installed headers.
