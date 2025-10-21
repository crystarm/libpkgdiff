# Alt Package Comparator

A C/C++ project: a library and a CLI utility for comparing binary packages between two branches of ALT Linux (e.g., p11 and sisyphus) by architecture.

## Features

- Getting data from the official ALT Linux API
- Comparison of package lists:
- Which are in one branch, but are missing in the other
  - Which versions have a higher `version-release`?
- Work with each architecture separately (`x86_64`, `i586`, `aarch64`, `e2k`)
- Output the result as JSON
- Build and run in an isolated ALT Linux container (Docker or Podman)

## Assembly and installation

bash:
```
make
sudo make install
```

## Example of running a CLI utility

bash:
```
compare-cli p11 sisyphus
```

The output is a JSON object with the comparison results.

## Use via container

bash:
```
docker build -t alt-pkgdiff container/
docker run --rm alt-pkgdiff
```

The result log will be saved in `/var/log/pkgdiff.json` inside the container.

## License

MIT
