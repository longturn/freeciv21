#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
# SPDX-FileCopyrightText: XHawk87 <hawk87@hotmail.co.uk>
set -e

cd "$(dirname "$0")/.."

case "${1:-}" in
  clean)
    docker builder prune --all --force --verbose
    rm --recursive --force --verbose ./build-docker ./build-clang
    exit
  ;;
  server-image)
    ./scripts/docker-build.sh build
    docker build --tag freeciv21-server:"${SERVER_VERSION:-${2:-latest}}" \
        --file .docker/Dockerfile.server \
        --build-arg CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}" \
        --build-arg UID="${UID:-556}" \
        --build-arg GID="${GID:-${UID:-556}}" .
    exit
  ;;
esac

docker build --tag freeciv21-builder:latest \
      --file .docker/Dockerfile.builder \
      --build-arg CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}" .

docker run --rm \
  --volume "$(pwd -P):/usr/src/freeciv21" \
  --env CMAKE_BUILD_PREFIX \
  --env FREECIV_ENABLE_TOOLS \
  --env FREECIV_ENABLE_SERVER \
  --env FREECIV_ENABLE_NLS \
  --env FREECIV_ENABLE_CIVMANUAL \
  --env FREECIV_ENABLE_CLIENT \
  --env FREECIV_ENABLE_FCMP_CLI \
  --env FREECIV_ENABLE_FCMP_QT \
  --env FREECIV_ENABLE_RULEDIT \
  --env FREECIV_ENABLE_RULEUP \
  --env FREECIV_ENABLE_MANPAGES \
  --env FREECIV_USE_VCPGK \
  --env FREECIV_DOWNLOAD_FONTS \
  --env CMAKE_BUILD_TYPE \
  --env CMAKE_C_FLAGS_DEBUG \
  --env CMAKE_CXX_FLAGS_DEBUG \
  --env CMAKE_INSTALL_PREFIX \
  freeciv21-builder:latest \
  /usr/src/freeciv21/.docker/build.sh "$@"

