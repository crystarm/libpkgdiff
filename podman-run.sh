#!/usr/bin/env bash
set -euo pipefail

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

# Determine TTY flags only if attached to a terminal
TTY_FLAGS=""
if [[ -t 0 && -t 1 ]]; then
  TTY_FLAGS="-it"
fi

# SELinux on Fedora/RHEL (and sometimes with Docker on Fedora) needs :Z on bind mounts
VOLUME_LABEL=""
if command -v getenforce >/dev/null 2>&1; then
  if [[ "$(getenforce 2>/dev/null || true)" =~ Enforcing|Permissive ]]; then
    VOLUME_LABEL=":Z"
  fi
elif [[ -e /sys/fs/selinux/enforce ]]; then
  # Best-effort fallback
  VOLUME_LABEL=":Z"
fi

# User mapping to avoid root-owned files on the host
USER_FLAGS=""
if [[ "${ENGINE}" == "podman" ]]; then
  # Rootless podman can keep the host UID/GID
  USER_FLAGS="--userns=keep-id"
else
  # Docker: set user explicitly
  USER_FLAGS="-u $(id -u):$(id -g)"
fi

# Image tag (override with IMAGE_TAG if desired)
IMAGE_TAG="${IMAGE_TAG:-libpkgdiff:alt}"

# Working directory is the current directory
HOST_PWD="${PWD}"
CONTAINER_WORKDIR="/work"

# Prepare cache on the host so it's writable and persistent
mkdir -p "${HOST_PWD}/.cache/libpkgdiff/sources"

# Pass through useful env (add more if your app needs them)
ENV_FLAGS=(
  -e XDG_CACHE_HOME="/work/.cache"
  -e LIBPKGDIFF_SOURCES_DIR="/work/.cache/libpkgdiff/sources"
  -e SSL_CERT_DIR="/etc/ssl/certs"
)

# Run container
exec "${ENGINE}" run --rm ${TTY_FLAGS} \
  ${USER_FLAGS} \
  -v "${HOST_PWD}:${CONTAINER_WORKDIR}${VOLUME_LABEL}" \
  -w "${CONTAINER_WORKDIR}" \
  "${ENV_FLAGS[@]}" \
  "${IMAGE_TAG}" "$@"
