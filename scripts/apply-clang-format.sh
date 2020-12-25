#! /usr/bin/env bash

# Formats all source files using clang-format. This is simple but tedious to
# type every time.

dirs="ai common client server tools utility"
find $dirs -\( -name '*.h' -or -name '*.cpp' -\) -exec clang-format -i '{}' \;
