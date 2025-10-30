# ---------- build stage ----------
FROM alt:p11 AS build

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y \
      gcc make ca-certificates \
      libcurl-devel \
      "pkgconfig(jansson)" && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /work
COPY . /work

RUN make clean && make

# ---------- runtime stage ----------
FROM alt:p11 AS run
RUN apt-get update && \
    apt-get install -y \
      ca-certificates \
      libcurl \
      jansson && \
    rm -rf /var/lib/apt/lists/*

COPY --from=build /work/compare-cli /usr/bin/pkgdiff
COPY --from=build /work/libpkgdiff.so /usr/lib64/libpkgdiff.so
COPY --from=build /work/picture.txt /usr/share/libpkgdiff/picture.txt

ENV LD_LIBRARY_PATH=/usr/lib64
ENTRYPOINT ["/usr/bin/pkgdiff"]
