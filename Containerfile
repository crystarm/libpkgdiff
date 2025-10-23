# syntax=docker/dockerfile:1
# Multi-stage build (Podman compatible)
FROM debian:bookworm-slim AS build
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends     build-essential make ca-certificates     libcurl4-openssl-dev libjansson-dev   && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . /src
RUN make clean && make && make install PREFIX=/usr/local

FROM debian:bookworm-slim AS runtime
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends     ca-certificates libcurl4 libjansson4   && rm -rf /var/lib/apt/lists/*
COPY --from=build /usr/local/bin/compare-cli /usr/local/bin/compare-cli
COPY --from=build /usr/local/lib/libpkgdiff.so /usr/local/lib/libpkgdiff.so
COPY picture.txt /work/picture.txt
ENV LD_LIBRARY_PATH=/usr/local/lib
WORKDIR /work
ENTRYPOINT ["/usr/local/bin/compare-cli"]
