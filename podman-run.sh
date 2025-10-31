#!/usr/bin/env bash
set -euo pipefail
podman run --rm -it -v "$PWD":/work libpkgdiff:alt "$@"
