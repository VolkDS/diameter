#!/bin/bash

ROOT_DIR=$(git rev-parse --show-toplevel)

find ${ROOT_DIR}/examples/ -iname '*.h' -o -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format-15 -i
find ${ROOT_DIR}/include/ -iname '*.h' -o -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format-15 -i
find ${ROOT_DIR}/src/ -iname '*.h' -o -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format-15 -i
find ${ROOT_DIR}/tests/ -iname '*.h' -o -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format-15 -i
