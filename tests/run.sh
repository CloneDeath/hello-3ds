#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/host-tests
cc -std=c11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -Isource \
  tests/test_game.c source/game.c source/world.c source/save.c source/render.c \
  -lm -o build/host-tests/test-game
ASAN_OPTIONS=detect_leaks=0 build/host-tests/test-game build/host-tests
