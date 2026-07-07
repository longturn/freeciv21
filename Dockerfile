# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
# SPDX-FileCopyrightText: XHawk87 <hawk87@hotmail.co.uk>

ARG BUILDER_IMAGE=builder_local

# Build stage
FROM debian:stable-20260406-slim AS builder_local

ARG BUILD_TYPE=Debug
ARG DEBIAN_FRONTEND=noninteractive

# Copy sources from build context (Dockerfile placed at repo root)
COPY . /usr/src/freeciv21

WORKDIR /usr/src/freeciv21

# Build dependencies
RUN bash <<'END_RUN'
set -e

deps=(
    cmake=3.31.6-2
    ninja-build=1.12.1-1
    g++=4:14.2.0-1
    python3=3.13.5-1
    qtbase5-dev=5.15.15+dfsg-6+deb13u1
    libkf5archive-dev=5.116.0-1
    liblua5.3-dev=5.3.6-2+b4
    libsqlite3-dev=3.46.1-7+deb13u1
    gettext=0.23.1-2
    zlib1g-dev=1:1.3.dfsg+really1.3.1-1+b1
)
if [[ "$BUILD_TYPE" == "Debug" ]]; then
  deps+=(
      libunwind-dev=1.8.1-0.1
      libdw-dev=0.192-4
  )
fi
apt-get update && apt-get install -y --no-install-recommends "${deps[@]}"
rm -rf /var/lib/apt/lists/*
build_opts=(
    -DFREECIV_ENABLE_TOOLS=OFF
    -DFREECIV_ENABLE_SERVER=ON
    -DFREECIV_ENABLE_CLIENT=OFF
    -DFREECIV_ENABLE_NLS=OFF
    -DFREECIV_DOWNLOAD_FONTS=0
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
    -DCMAKE_C_FLAGS_DEBUG="-Og -ggdb"
    -DCMAKE_CXX_FLAGS_DEBUG="-Og -ggdb"
)
cmake . -B build -G Ninja "${build_opts[@]}"
cmake --build build
cmake --build build --target install
END_RUN

FROM ${BUILDER_IMAGE} AS builder

# Runtime stage
FROM debian:stable-20260406-slim

ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_TYPE=Debug
ARG UID=556
ARG GID=${UID}

# Copy installed files from builder
COPY --from=builder /usr/bin/freeciv21-server /usr/bin/freeciv21-server
COPY --from=builder /usr/share/freeciv21 /usr/share/freeciv21

# Minimal runtime deps
RUN bash <<'END_RUN'
set -e
deps=(
    libqt5gui5t64=5.15.15+dfsg-6+deb13u1
    libkf5archive5=5.116.0-1
    liblua5.3-0=5.3.6-2+b4
    libreadline8t64=8.2-6
    libqt5network5t64=5.15.15+dfsg-6+deb13u1
    luarocks=3.8.0+dfsg1-1
    lua5.3=5.3.6-2+b4
    liblua5.3-dev=5.3.6-2+b4
    libsqlite3-dev=3.46.1-7+deb13u1
    libpq-dev=17.10-0+deb13u1
    build-essential=12.12
    pkg-config=1.8.1-4
)
if [[ "$BUILD_TYPE" == "Debug" ]]; then
  deps+=(
      libdw-dev=0.192-4
      gdb=16.3-1
  )
fi
apt-get update && apt-get install -y --no-install-recommends "${deps[@]}"
rm -rf /var/lib/apt/lists/*
mkdir -p /usr/local/share/freeciv21
groupadd --system --gid ${GID} freeciv21
useradd --create-home --home-dir /home/freeciv21 \
        --system --uid ${UID} --gid freeciv21 \
        --shell /usr/sbin/nologin freeciv21
chown freeciv21:freeciv21 /home/freeciv21
eval $(luarocks path --bin --lua-version 5.3)
luarocks --lua-version 5.3 install md5 1.3-1
luarocks --lua-version 5.3 install luasql-sqlite3 2.8.0-1
luarocks --lua-version 5.3 install luasql-postgres 2.8.0-1 \
    PGSQL_INCDIR=/usr/include/postgresql PGSQL_LIBDIR=/usr/lib
END_RUN

EXPOSE 5556/tcp

USER freeciv21
WORKDIR /home/freeciv21

ENTRYPOINT ["/usr/bin/freeciv21-server"]
