#!/bin/bash
set -e

cd "$(dirname "$0")"

echo "=== Football 11v11 Build Script ==="

# Check for cmake
if ! command -v cmake &>/dev/null; then
    echo "CMake not found. Installing via Homebrew..."
    if ! command -v brew &>/dev/null; then
        echo "Please install Homebrew first: https://brew.sh"
        echo "Then run: brew install cmake"
        exit 1
    fi
    brew install cmake
fi

mkdir -p build
cd build

echo "Configuring..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build . --config Release -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "Build complete! Run the game with:"
echo "  ./build/Football11v11"
echo ""
