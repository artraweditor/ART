#!/bin/bash
# scripts/build_art_cli.sh
set -e

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE="Release" -DENABLE_GUI=OFF ..
make -j$(sysctl -n hw.logicalcpu 2>/dev/null || nproc) art-cli
echo "ART-cli built successfully."
