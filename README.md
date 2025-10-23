# Alt Package Comparator (libpkgdiff + CLI)

A small C/C++ utility that compares **binary package** sets between two ALT Linux branches
(default: `p11` vs `sisyphus`). It fetches JSON from the public ALT API, parses it, shows
human‑readable previews in the terminal, and can save full JSON lists to files.

- Library: `libpkgdiff.so`
- CLI: `compare-cli`
- Dependencies: `libcurl` (HTTP), `jansson` (JSON)

## Features
- Fetch package lists from `rdb.altlinux.org`.
- **Filter by architecture** via `--arch` (e.g. only `x86_64`).
- Show 10‑item previews (one line per package) of unique packages:
  - present only in `p11` (not in `sisyphus`)
  - present only in `sisyphus` (not in `p11`)
- Optionally save **full JSON** lists:
  - `p11_only.json`
  - `sisyphus_only.json`
- Simple, readable TUI: ASCII frames + colored status lines.

## Requirements

### Debian/Ubuntu
```bash
sudo apt update
sudo apt install -y build-essential libcurl4-openssl-dev libjansson-dev
```

### ALT Linux (inside VM or container)
```bash
sudo apt-get update
sudo apt-get install -y gcc make libcurl-devel libjansson-devel
```

## Build
```bash
make clean && make
```
This produces:
- `libpkgdiff.so`
- `compare-cli` (linked against the shared library)

> If running from the build folder, set runtime lib path:
> ```bash
> LD_LIBRARY_PATH=. ./compare-cli
> ```

## Usage

Basic:
```bash
LD_LIBRARY_PATH=. ./compare-cli
```

With architecture filter (`--arch`):
```bash
LD_LIBRARY_PATH=. ./compare-cli --arch x86_64
LD_LIBRARY_PATH=. ./compare-cli --arch aarch64
```
What `--arch` does:
- Only packages with the exact `arch` (e.g. `x86_64`) are considered during indexing and comparison.
- Both previews and saved JSON lists will contain **only that architecture**.

Help:
```bash
./compare-cli --help
```

## What you’ll see

1) Greeting and (optional) ASCII art from `picture.txt`.
2) For **each branch** (e.g. `p11`, then `sisyphus`):
   - A framed line like `Connecting to rdb.altlinux.org`
   - `Endpoint: GET /api/export/branch_binary_packages/<branch>`
   - `Requesting branch list...`
   - `✔ Download complete`
   - `• Received XX.XX MB for branch '<branch>'`
   - `✔ Looks like valid JSON`
3) After parsing:
```
Fetched NNNNN packages from branch1 (p11)
Fetched MMMMM packages from branch2 (sisyphus)
```
4) Prompts:
- Show 10‑item previews? `[y/N]`
- Save full unique lists? `[y/N]`

Saved files (if confirmed):
- `p11_only.json`
- `sisyphus_only.json`

> Note: Saving full lists can take time (they are large).

## Project Structure
```
.
├─ include/
│  └─ pkgdiff.h
├─ src/
│  ├─ pkgdiff.c
│  └─ main.c
├─ picture.txt         # optional ASCII/Braille art
└─ Makefile
```

## Troubleshooting

**`./compare-cli: error while loading shared libraries: libpkgdiff.so: cannot open shared object file`**  
Run with explicit library path or install the library:
```bash
LD_LIBRARY_PATH=. ./compare-cli
# or install system-wide
sudo make install
sudo ldconfig
compare-cli
```

**Network issues**  
If the API is temporarily unreachable, you’ll see a red failure line. Re-run later.

## License
MIT (or as required by your assignment).
