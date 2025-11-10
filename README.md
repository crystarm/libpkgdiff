# libpkgdiff — ALT Package Comparator
CLI: **`pkgdiff`** · Library: **`libpkgdiff.so`** · License: MIT

> **CLI name:** the build now produces a binary named `pkgdiff` (previously `compare-cli`). Update your scripts if you used the old name.

`libpkgdiff` fetches and compares **binary package lists** between ALT Linux branches (by default `p11` ↔ `sisyphus`).  
It downloads JSON from the official ALT repo database API, caches the results locally, and renders human‑friendly, aligned summaries in the terminal. The same library functions power the bundled CLI.

---

## Highlights

- **Two comparison modes**
  - **Unique packages per branch:** show what exists **only** in `p11` or **only** in `sisyphus` (with optional `--arch` filter).
  - **Common packages with versions:** for packages present in **both** branches, show **all**, **only equal**, or **only different** versions.
- **Local caching** of downloaded JSON (per‑branch) with a **2‑hour TTL** to avoid unnecessary network trips.
- **Clean separation** of public API vs CLI internals; a small `include/pkgdiff.h` and private headers in `src/lib` and `src/cli`.
- **Container‑friendly**: multi‑stage `Containerfile` + `podman-build.sh` / `podman-run.sh`.
- **ALT RPM packaging**: skeleton spec in `packaging/libpkgdiff.spec` (version **1.1**).

---

## Build & run

### Dependencies
- **C toolchain:** `gcc`, `make`, `pkg-config`
- **Libraries:** `libcurl` (HTTP), `jansson` (JSON)

**ALT p11 (apt‑rpm):**
```bash
sudo apt-get update
sudo apt-get install -y gcc make pkg-config libcurl-devel "pkgconfig(jansson)"
```

**Debian/Ubuntu:**
```bash
sudo apt update
sudo apt install -y build-essential pkg-config libcurl4-openssl-dev libjansson-dev
```

### Build
```bash
make clean && make
```

### Run (from the build dir)
```bash
LD_LIBRARY_PATH=. ./pkgdiff [--arch x86_64]
```

> Tip: the app prints an ASCII splash once; then interactive prompts guide you.

---

## CLI usage

```
Usage: pkgdiff [--arch <name>]

Main menu:
  1) Unique packages per branch (p11 vs sisyphus)
  2) Common packages with versions
     ├─ Show all common packages
     ├─ Show packages with the same versions in both branches
     └─ Show packages with different versions between branches
```

- `--arch` is optional (e.g. `--arch x86_64`, `--arch noarch`).  
- Outputs are **aligned** regardless of package name length; equal/different versions are color‑coded.

---

## Caching & file locations

- JSON for each branch is cached under **sources dir**:
  - Default (XDG aware):  
    - `$XDG_CACHE_HOME/libpkgdiff/sources` or `~/.cache/libpkgdiff/sources`
  - Override with **`LIBPKGDIFF_SOURCES_DIR`**
- Derived or saved results go under **results dir**:
  - Default (XDG aware):  
    - `$XDG_DATA_HOME/libpkgdiff/results` or `~/.local/share/libpkgdiff/results`
  - Override with **`LIBPKGDIFF_RESULTS_DIR`**
- Cache TTL is **2 hours**. Fresh runs display a message like:  
  `Using cached JSON for branch 'p11' (...)`  
  If the cache is older than 2 hours, the branch JSON is fetched again.

Each cached branch uses two files:
```
<dir>/<branch>.json   # payload
<dir>/<branch>.meta   # metadata (timestamps, etc.)
```

Environment variables the app respects:
```
LIBPKGDIFF_SOURCES_DIR   # override cache location
LIBPKGDIFF_RESULTS_DIR   # override results location
XDG_CACHE_HOME           # used if set
XDG_DATA_HOME            # used if set
SSL_CERT_DIR             # pass-through for containers if needed
```

---

## Library API (C)

Public header: **`include/pkgdiff.h`**

Core functions:
```c
void pkgdiff_unique_pkgs(const char *branch1, const char *branch2,
                         const char *arch_filter);

void pkgdiff_common_pkgs_with_versions(const char *branch1, const char *branch2,
                                       const char *arch_filter,
                                       const char *save_path);

/* Extended: filter the common set (all/equal/different) */
void pkgdiff_common_pkgs_with_versions_ex(const char *branch1, const char *branch2,
                                          const char *arch_filter,
                                          const char *save_path,
                                          int filter);

/* Filter constants */
#define PKGDIFF_FILTER_ALL     0
#define PKGDIFF_FILTER_EQUAL   1
#define PKGDIFF_FILTER_DIFFER  2
```

All functions print a formatted view to stdout. When `save_path` is non‑NULL, a JSON preview is also written there.  
Cache & result directories can be queried internally via:
```c
void pkgdiff_get_sources_dir(char *out, size_t out_sz);
void pkgdiff_get_results_dir(char *out, size_t out_sz);
```

---

## Example outputs

- **Unique packages**: lists items present only in one branch, grouped by branch.
- **Common with versions**: shows lines like  
  `• <name> [<arch>] versions: (p11: <verrel>) (sisyphus: <verrel>)  [= / ≠]`  
  with bold/green highlights; equal versions are marked with a subtle `[=]` badge.

---

## Packaging (ALT Linux RPM)

A ready‑to‑use spec is included: **`packaging/libpkgdiff.spec`** (Version **1.1**, Release `alt1`).

**Quick SRPM/RPM build:**
```bash
# from repo root
make clean && make
rpmbuild --define "_sourcedir $PWD"          --define "_specdir   $PWD/packaging"          --define "_builddir  $PWD/build"          --define "_srcrpmdir $PWD/dist"          --define "_rpmdir    $PWD/dist"          -ba packaging/libpkgdiff.spec
# results under ./dist/
```

The package installs:
```
/usr/bin/pkgdiff
/usr/lib*/libpkgdiff.so
```

> **Note:** The spec treats the shared library as **private** to the CLI (soname not stabilised).

---

## Containers (Podman/Docker)

**Build the image:**
```bash
./podman-build.sh
# env overrides:
#   CONTAINER_ENGINE=podman|docker
#   IMAGE_TAG=my/libpkgdiff:alt
#   CONTAINERFILE=containers/Containerfile
```

**Run the containerized CLI against your working dir (mounted at /work):**
```bash
./podman-run.sh [--arch x86_64]
# defaults:
#   -e XDG_CACHE_HOME="/work/.cache"
#   -e LIBPKGDIFF_SOURCES_DIR="/work/.cache/libpkgdiff/sources"
```

The entrypoint is `pkgdiff`; logs and cache stay in the mounted workdir.

---

## Troubleshooting

- **Missing colors / garbled escape sequences:** your terminal might not support ANSI colors—set `TERM=xterm-256color`.
- **“Using cached JSON ...” when you expect a fresh fetch:** the TTL is 2 hours; delete files in the sources dir or set `LIBPKGDIFF_SOURCES_DIR` to an empty temp location.
- **`libcurl` / `jansson` link errors:** ensure `pkg-config --libs libcurl jansson` returns valid flags; otherwise edit `Makefile` to add `-lcurl -ljansson` manually (already present as a fallback).

---

## Roadmap

- Optional output paging and non‑interactive mode flags.
- Configurable cache TTL.
- More branch pairs and offline comparison from saved JSONs.

---

## License

MIT — see `LICENSE`.

---

## Acknowledgements

Thanks to the ALT Linux community and maintainers of `libcurl` and `jansson`.


---

## RPM Packaging Tutorial (ALT Linux) — Step by step

> This project ships a ready spec: **`packaging/libpkgdiff.spec`** (Version **1.1**, Release `alt1`).  
> Below is a concise, reproducible flow for ALT **p11** (works similarly on Sisyphus).

### 1) Install tools
```bash
sudo apt-get update
sudo apt-get install -y rpmdevtools rpm-build hasher git-core gcc make \
                        libcurl-devel "pkgconfig(jansson)"
```

### 2) Prepare the rpmbuild tree
```bash
rpmdev-setuptree   # creates ~/rpmbuild/{SOURCES,SPECS,BUILD,RPMS,SRPMS}
```

### 3) Build directly (outside hasher)
From the repo root:
```bash
# Clean & build binaries first (optional)
make clean && make

# Build SRPM and RPMs in ./dist using defines
rpmbuild --define "_sourcedir $PWD" \
         --define "_specdir   $PWD/packaging" \
         --define "_builddir  $PWD/build" \
         --define "_srcrpmdir $PWD/dist" \
         --define "_rpmdir    $PWD/dist" \
         -ba packaging/libpkgdiff.spec
# Artifacts land under ./dist/
```

### 4) Build in a clean chroot with **hasher** (recommended)
```bash
# one-time setup
sudo hasher-useradd "$USER"
newgrp hasher
hsh --initroot

# produce a source RPM from your working tree
rpmbuild -bs packaging/libpkgdiff.spec \
         --define "_sourcedir $PWD" \
         --define "_specdir   $PWD/packaging" \
         --define "_srcrpmdir $PWD/dist"

# build inside hasher (clean environment)
hasher ./dist/libpkgdiff-1.1-alt1.src.rpm
# result paths will be printed by hasher; usually under /var/hasher/... and copied to the working dir depending on config
```

### Useful ALT links
- RPM overview (ALT Wiki, EN): https://en.altlinux.org/RPM  
- **hasher** — clean/reproducible builds (EN): https://en.altlinux.org/Hasher  
- **gear** — packaging from git (EN): https://en.altlinux.org/Gear/Introduction  
- git.alt quickstart (EN): https://en.altlinux.org/Git.alt_quickstart  
- Developer portal / Main page (EN): https://en.altlinux.org/Main_Page  
- Packaging HOWTO (RU): https://www.altlinux.org/ALT_Packaging_HOWTO

> Note: Some pages are Russian‑only; they still serve as authoritative references for ALT packaging conventions and policies.

---

## Project Structure (recap)

```
libpkgdiff/ (repo root)
├─ include/
│  └─ pkgdiff.h                  # public C API
├─ src/
│  ├─ lib/                       # internal library (not installed as stable ABI)
│  │  ├─ ansi.h                  # ANSI color macros
│  │  ├─ util.c  util.h          # filesystem, env, formatting helpers
│  │  ├─ net.c   net.h           # HTTP fetch (libcurl), cache handling
│  │  └─ cmp.c                   # comparison routines (unique/common/equal/differ)
│  └─ cli/                       # CLI frontend
│     ├─ main.c                  # entrypoint / interactive menu
│     ├─ ui.c   ui.h             # TTY UI helpers (layout, alignment, badges)
│     └─ (private headers)       # if any
├─ containers/
│  └─ Containerfile              # 2-stage build on ALT p11
├─ packaging/
│  └─ libpkgdiff.spec            # RPM spec (Version 1.1, Release alt1)
├─ podman-build.sh               # container image build helper
├─ podman-run.sh                 # convenience runner with cache binds
├─ Makefile                      # builds lib + CLI (binary: pkgdiff)
├─ LICENSE
└─ README.md                     # you are here
```
