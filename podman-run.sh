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

# TTY only if attached
TTY_FLAGS=""
if [[ -t 0 && -t 1 ]]; then
  TTY_FLAGS="-it"
fi

# SELinux (:Z) when applicable
VOLUME_LABEL=""
if command -v getenforce >/dev/null 2>&1; then
  if [[ "$(getenforce 2>/dev/null || true)" =~ Enforcing|Permissive ]]; then
    VOLUME_LABEL=":Z"
  fi
elif [[ -e /sys/fs/selinux/enforce ]]; then
  VOLUME_LABEL=":Z"
fi

# User mapping
USER_FLAGS=""
if [[ "${ENGINE}" == "podman" ]]; then
  USER_FLAGS="--userns=keep-id"
else
  USER_FLAGS="-u $(id -u):$(id -g)"
fi

# Local tag (must match build default)
IMAGE_TAG="${IMAGE_TAG:-localhost/libpkgdiff:alt}"

# Working dir mapping
HOST_PWD="${PWD}"
CONTAINER_WORKDIR="/work"

# Ensure cache on host
mkdir -p "${HOST_PWD}/.cache/libpkgdiff/sources"

# Env passthrough
ENV_FLAGS=(
  -e XDG_CACHE_HOME="/work/.cache"
  -e LIBPKGDIFF_SOURCES_DIR="/work/.cache/libpkgdiff/sources"
  -e SSL_CERT_DIR="/etc/ssl/certs"
)

# Helper: image existence for both engines
image_exists() {
  if [[ "${ENGINE}" == "podman" ]]; then
    "${ENGINE}" image exists "$1"
  else
    "${ENGINE}" image inspect "$1" >/dev/null 2>&1
  fi
}

# Ensure image is present locally; if not — build it
if ! image_exists "${IMAGE_TAG}"; then
  echo "Local image ${IMAGE_TAG} not found. Building it now..."
  CONTAINER_ENGINE="${ENGINE}" IMAGE_TAG="${IMAGE_TAG}" bash "${SCRIPT_DIR}/podman-build.sh"
  if ! image_exists "${IMAGE_TAG}"; then
    echo "Error: build did not produce ${IMAGE_TAG}" >&2
    exit 1
  fi
fi

# Podman-only: forbid pulling if tag absent, to avoid localhost registry attempts
PULL_FLAG=""
if [[ "${ENGINE}" == "podman" ]]; then
  PULL_FLAG="--pull=never"
fi

# Run
exec "${ENGINE}" run --rm ${TTY_FLAGS} ${PULL_FLAG}   ${USER_FLAGS}   -v "${HOST_PWD}:${CONTAINER_WORKDIR}${VOLUME_LABEL}"   -w "${CONTAINER_WORKDIR}"   "${ENV_FLAGS[@]}"   "${IMAGE_TAG}" "$@"
