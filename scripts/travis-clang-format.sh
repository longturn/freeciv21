#! /usr/bin/env bash

# Formats all source files using clang-format. This is simple but tedious to
# type every time.

dirs="ai common client server tools utility"

# Debian-based systems have clang-format-xx, use the most recent
clang_format=$(which clang-format)
for version in $(seq 7 11); do
  which 2>&1 >/dev/null clang-format-$version && clang_format=clang-format-$version
done

[ -z "$clang_format" ] && exit 1

echo -n "Using $clang_format: " >&2
$clang_format --version >&2

# Run it on all files changed with respect to the target branch
# Save the diff, then display it and exit 1 if it wasn't empty.
git-clang-format --diff $(git merge-base $TRAVIS_BRANCH HEAD) HEAD >format.diff

# Error out only if it looks like a patch
if grep format.diff 'diff --git' &>/dev/null; then
  echo 'clang-format found formatting mistakes in some of the files you changed.' >&2
  echo 'Here are the changes you need to apply:' >&2
  echo '===================================================================' >&2
  cat format.diff
  echo '===================================================================' >&2
  echo 'End of the clang-format diff' >&2
  exit 1
fi
