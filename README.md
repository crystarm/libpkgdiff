# libpkgdiff (CLI: `pkgdiff`)

Compare **binary package lists** between ALT Linux branches (default: `p11` ↔ `sisyphus`).  
Fetches JSON from the official ALT API, parses it, shows short **human‑readable previews** in the terminal, and can save **full JSON** lists to files.

- Library: `libpkgdiff.so`
- CLI: **`pkgdiff`**
- Dependencies: `libcurl` (HTTP), `jansson` (JSON)

---

## Features

- Pull package lists from `rdb.altlinux.org` (REST API).
- **Architecture filter** via `--arch` (e.g. only `x86_64`).
- Show **10‑item previews** (one package per line) of unique packages:
  - present only in `p11` (not in `sisyphus`)
  - present only in `sisyphus` (not in `p11`)
- Optionally save full unique lists as JSON:
  - `p11_only.json`
  - `sisyphus_only.json`
- Minimal, readable TUI: ASCII frames + colored status lines.
- Greeting art (ASCII/Braille) loaded from a file (see **Environment**).

---

## Quick start (RPM on ALT Linux)

If you have the RPM package (recommended for reviewers):

```bash
sudo apt-get install -y ./libpkgdiff-<version>.x86_64.rpm
pkgdiff               # run
pkgdiff --arch x86_64 # only one architecture
```

The package installs:
- `/usr/bin/pkgdiff` (CLI)
- `/usr/lib64/libpkgdiff.so` (library)
- `/usr/share/libpkgdiff/picture.txt` (optional greeting art, if packaged)

Runtime dependencies (libcurl, jansson, …) are resolved automatically by ALT’s auto‑requires when installing the RPM.

---

## Build from source

### Requirements

**ALT Linux (inside VM or bare‑metal):**
```bash
sudo apt-get update
sudo apt-get install -y gcc make libcurl-devel pkgconfig(jansson) ca-certificates
```

**Debian/Ubuntu:**
```bash
sudo apt update
sudo apt install -y build-essential libcurl4-openssl-dev libjansson-dev
```

### Build
```bash
make clean && make
# run from build dir
LD_LIBRARY_PATH=. ./compare-cli
LD_LIBRARY_PATH=. ./compare-cli --arch x86_64
```

### System‑wide install (optional)
```bash
sudo make install PREFIX=/usr
sudo ldconfig
pkgdiff --arch x86_64   # now available globally
```

> The CLI binary is installed as **`/usr/bin/pkgdiff`**.

---

## Podman (containerized build & run)

Multi‑stage image (Debian base) that compiles the project and runs the CLI.

```bash
# in project root (Containerfile present)
podman build -t altpkgdiff:dev -f Containerfile .

# Run with your working dir mounted (so JSON files are saved on host)
podman run --rm -it -v "$PWD":/work altpkgdiff:dev
podman run --rm -it -v "$PWD":/work altpkgdiff:dev --arch x86_64
```

If you use the provided helpers:
```bash
./podman-build.sh
./podman-run.sh --arch aarch64
```

---

## QEMU/KVM + ALT VM (used for testing)

1) On the host (Debian), install QEMU/KVM and create a VM with ALT p11.  
2) Inside ALT VM install deps:  
   `sudo apt-get install -y gcc make libcurl-devel pkgconfig(jansson)`  
3) Copy the project into VM (e.g. `rsync` over SSH with port‑forwarding) and build.  
4) Run `pkgdiff` (or `LD_LIBRARY_PATH=. ./compare-cli` from the build dir).

This is how the solution was validated.

---

## Usage

```bash
pkgdiff                # interactive, default: p11 vs sisyphus
pkgdiff --arch x86_64  # compare only x86_64 packages

# From the build directory (without make install):
LD_LIBRARY_PATH=. ./compare-cli --arch aarch64
```

What you’ll see:
1. Greeting and optional art (press Enter to continue).
2. For each branch:
   - “Connecting to rdb.altlinux.org”
   - `Endpoint: GET /api/export/branch_binary_packages/<branch>`
   - “Requesting branch list…”
   - “✔ Download complete” + size
   - “✔ Looks like valid JSON”
3. Totals:
   ```
   Fetched NNNNN packages from branch1 (p11)
   Fetched MMMMM packages from branch2 (sisyphus)
   ```
4. Prompts:
   - Show 10‑item previews? `[y/N]` (one line per package)
   - Save full unique lists? `[y/N]` → `p11_only.json`, `sisyphus_only.json`

> Full lists are large; saving can take noticeable time.

---

## CLI options

```
--arch <name>    Filter by architecture (e.g. x86_64, aarch64). Exact match.
-h, --help       Show brief usage.
```

Branches are currently fixed to `p11` and `sisyphus` (per test task).

---

## Environment

```
PKGDIFF_ART=/path/to/picture.txt
```
If set, the greeting art is loaded from this path. Otherwise the program tries:
1) `./picture.txt` (current directory)  
2) `/usr/share/libpkgdiff/picture.txt`  
3) `/usr/local/share/libpkgdiff/picture.txt`  

If nothing is found, the program continues without art.

---

## Troubleshooting

**`error while loading shared libraries: libpkgdiff.so`**  
Run from the build dir with:
```bash
LD_LIBRARY_PATH=. ./compare-cli
```
or install system‑wide:
```bash
sudo make install PREFIX=/usr
sudo ldconfig
pkgdiff
```

**Network/API issues**  
Temporary network or API hiccups will be shown in red; re‑run later.

**Container shows no output**  
Remember the program waits for Enter after the greeting prompt.

---

## Project structure

```
.
├── Makefile
├── README.md
├── Containerfile
├── picture.txt
├── podman-build.sh
├── podman-run.sh
├── include/
│   └── pkgdiff.h
└── src/
    ├── main.c
    ├── ui.c
    ├── net.c
    ├── cmp.c
    ├── u.h
    └── u.c
```

---

## License
MIT
