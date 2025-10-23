#!/usr/bin/env bash
set -euo pipefail
IMAGE="${IMAGE:-altpkgdiff:dev}"

# Build if image missing
if ! podman image exists "$IMAGE"; then
  podman build -t "$IMAGE" -f Containerfile .
fi

# Mount current dir so output JSON stays on host
exec podman run --rm -it   -v "$PWD":/work   "$IMAGE" "$@"
