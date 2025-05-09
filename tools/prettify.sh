#!/bin/sh

set -x
find ./src -iname '*.h' -o -iname '*.c' | xargs clang-format --verbose  -i -style=llvm
#find ./solpaq -iname '*.h' -o -iname '*.c' | xargs clang-format --verbose  -i -style=llvm