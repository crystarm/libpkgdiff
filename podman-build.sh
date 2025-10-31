#!/usr/bin/env bash
set -euo pipefail
podman build -t libpkgdiff:alt -f Containerfile .
