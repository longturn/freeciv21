# Build stage
FROM debian:stable-slim AS builder

ARG BUILD_TYPE=Debug
ARG DEBIAN_FRONTEND=noninteractive

# Copy sources from build context (Dockerfile placed at repo root)
COPY . /usr/src/freeciv21

WORKDIR /usr/src/freeciv21

# Build dependencies
RUN set -- cmake ninja-build g++ python3 qt6-base-dev libkf6archive-dev \
           liblua5.3-dev libsqlite3-dev gettext libz-dev; \
    [ "$BUILD_TYPE" != "Debug" ] || set -- "$@" libunwind-dev libdw-dev; \
    apt-get update \
 && apt-get install -y --no-install-recommends "$@" \
 && rm -rf /var/lib/apt/lists/* \
 && cmake . -B build -G Ninja \
      -DFREECIV_ENABLE_TOOLS=OFF \
      -DFREECIV_ENABLE_SERVER=ON \
      -DFREECIV_ENABLE_CLIENT=OFF \
      -DFREECIV_ENABLE_NLS=OFF \
      -DFREECIV_DOWNLOAD_FONTS=0 \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DCMAKE_C_FLAGS_DEBUG="-Og -ggdb" \
      -DCMAKE_CXX_FLAGS_DEBUG="-Og -ggdb" \
 && cmake --build build \
 && cmake --build build --target install

# Runtime stage
FROM debian:stable-slim

ARG DEBIAN_FRONTEND=noninteractive

# Copy installed files from builder
COPY --from=builder /usr/bin/freeciv21-server /usr/bin/freeciv21-server
COPY --from=builder /usr/share/freeciv21 /usr/share/freeciv21

# Minimal runtime deps
RUN set -- libqt6gui6 libkf6archive6 liblua5.3-0 libreadline8t64 \
           libqt6network6; \
    [ "$BUILD_TYPE" = "Debug" ] || set -- "$@" libdw-dev bash; \
    apt-get update && apt-get install -y --no-install-recommends "$@" \
 && rm -rf /var/lib/apt/lists/* \
 && useradd --system --create-home --home-dir /home/freeciv21 \
            --shell /usr/sbin/nologin freeciv21 \
    && chown -R freeciv21:freeciv21 /home/freeciv21 \
    && mkdir -p /usr/local/share/freeciv21

EXPOSE 5556/tcp

USER freeciv21
WORKDIR /home/freeciv21

ENTRYPOINT ["/usr/bin/freeciv21-server"]

