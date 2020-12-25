#! /usr/bin/env bash

# Formats all source files using clang-format. This is simple but tedious to
# type every time.

dirs="ai common client server tools utility"

# Debian-based systems have clang-format-xx, use the most recent
clang_format=$(which clang-format)
for version in $(seq 7 11); do
  which 2>/dev/null clang-format-$version && clang_format=clang-format-$version
done

[ -z "$clang_format" ] && exit 1

echo "Using $clang_format:" >&2
$clang_format --version >&2

# Run it on all files
find $dirs -\( -name '*.h' -or -name '*.cpp' -\) -exec $clang_format -i '{}' \;
