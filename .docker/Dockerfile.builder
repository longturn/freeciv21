# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
# SPDX-FileCopyrightText: XHawk87 <hawk87@hotmail.co.uk>

# Build stage
FROM debian:stable-20260406-slim

ARG CMAKE_BUILD_TYPE=Debug
ARG DEBIAN_FRONTEND=noninteractive

# Build dependencies
RUN bash <<'END_RUN'
set -e

deps=(
    clang=1:19.0-63
    clang-format-17=1:17.0.6-22+b2
    clang-tidy=1:19.0-63
    clang-tools=1:19.0-63
    cmake=3.31.6-2
    g++=4:14.2.0-1
    gettext=0.23.1-2
    git=1:2.47.3-0+deb13u1
    libkf6archive-dev=6.13.0-2
    liblua5.3-dev=5.3.6-2+b4
    libsdl2-mixer-dev=2.8.1+dfsg-2
    libsqlite3-dev=3.46.1-7+deb13u2
    python3=3.13.5-1
    python3-sphinx=8.1.3-5
    qt6-base-dev=6.8.2+dfsg-9+deb13u2
    qt6-multimedia-dev=6.8.2-8
    qt6-svg-dev=6.8.2-3
    zlib1g-dev=1:1.3.dfsg+really1.3.1-1+b1
)
case "$(uname -m)" in
  aarch64|arm64)
    deps+=(
      ninja-build=1.12.1-1+b1
    ) ;;
  x86_64|amd64)
    deps+=( 
      ninja-build=1.12.1-1
    ) ;;
  *) echo "Unsupported arch: $(uname -m)" >&2; exit 1 ;; \
esac
if [[ ${CMAKE_BUILD_TYPE} == "Debug" ]]; then
  deps+=(
      libunwind-dev=1.8.1-0.1
      libdw-dev=0.192-4
  )
fi
apt-get update && apt-get install -y --no-install-recommends "${deps[@]}"
rm -rf /var/lib/apt/lists/*
groupadd --system freeciv21
useradd --create-home --home-dir /home/freeciv21 \
        --system --gid freeciv21 \
        --shell /usr/sbin/nologin freeciv21
END_RUN

ENTRYPOINT ["/bin/bash"]
