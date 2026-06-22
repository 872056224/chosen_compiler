#!/usr/bin/env bash
set -euo pipefail

echo "============================================"
echo "  LL1 Compiler — Linux Build Script"
echo "============================================"
echo ""

# Detect available CPU cores
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Check dependencies
for cmd in cmake make g++; do
    if ! command -v "$cmd" &>/dev/null; then
        echo "[ERROR] $cmd not found. Please install build-essential and cmake:"
        echo "        Ubuntu/Debian: sudo apt install build-essential cmake"
        echo "        Fedora:        sudo dnf install gcc-c++ cmake make"
        echo "        Arch:          sudo pacman -S gcc cmake make"
        exit 1
    fi
done

echo "[1/4] Configuring with Unix Makefiles..."
cmake ../.. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "[ERROR] CMake configuration failed."
    exit 1
fi

echo ""
echo "[2/4] Building compiler (parallel: $NPROC jobs)..."
cmake --build . --config Release -j"$NPROC"
if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed."
    exit 1
fi

echo ""
echo "[3/4] Running tests..."
ctest --output-on-failure
if [ $? -ne 0 ]; then
    echo "[WARNING] Some tests failed. Check output above."
else
    echo "[OK] All tests passed."
fi

echo ""
echo "[4/4] Compiler binary: tools/ll1c/ll1c"
echo ""
echo "============================================"
echo "  Build complete!"
echo ""
echo "  Usage:"
echo "    ./tools/ll1c/ll1c source.ll1 -o output.asm"
echo "    ./tools/ll1c/ll1c source.ll1 --new-codegen --opt -o output.asm"
echo "============================================"
