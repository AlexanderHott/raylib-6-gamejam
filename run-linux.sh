#!/usr/bin/env bash

set -euo pipefail

cmake -S . -B build -DGLFW_BUILD_X11=ON -DGLFW_BUILD_WAYLAND=ON && cmake --build build
(
  cd ./build/raylib-game-template

  gamescope -w 720 -h 720 -W 720 -H 720 -- ./raylib-game-template
  # ./raylib-game-template
)
