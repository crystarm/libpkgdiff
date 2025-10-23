#!/usr/bin/env bash
set -euo pipefail
IMAGE="${IMAGE:-altpkgdiff:dev}"
podman build -t "$IMAGE" -f Containerfile .
echo "Built $IMAGE"
