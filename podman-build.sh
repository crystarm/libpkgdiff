#!/usr/bin/env bash
set -Eeuo pipefail

# Detect container engine (prefer $CONTAINER_ENGINE, then podman, then docker)
ENGINE="${CONTAINER_ENGINE:-}"
if [[ -z "${ENGINE}" ]]; then
  if command -v podman >/dev/null 2>&1; then
    ENGINE="podman"
  elif command -v docker >/dev/null 2>&1; then
    ENGINE="docker"
  else
    echo "Error: neither podman nor docker found. Please install one of them." >&2
    exit 1
  fi
fi

# Resolve paths
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ROOT_DIR="${SCRIPT_DIR}"
CONTAINERFILE="${CONTAINERFILE:-${ROOT_DIR}/containers/Containerfile}"

# Default local tag to avoid short-name prompts and remote pulls
IMAGE_TAG="${IMAGE_TAG:-localhost/libpkgdiff:alt}"

echo "Using engine: ${ENGINE}"
echo "Building image: ${IMAGE_TAG} from ${CONTAINERFILE}"

# Build image
"${ENGINE}" build -t "${IMAGE_TAG}" -f "${CONTAINERFILE}" "${ROOT_DIR}"
echo "Built: ${IMAGE_TAG}"
