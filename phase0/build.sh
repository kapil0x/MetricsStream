#!/bin/bash
# Build script for Phase 0 PoC

set -e  # Exit on error

echo "Building Phase 0 PoC..."
echo ""

# Create build directory
mkdir -p build
cd build

# Run CMake
cmake ..

# Build
make

echo ""
echo "✅ Build complete!"
echo ""
echo "Run the server:"
echo "  ./build/phase0_poc"
echo ""
echo "Or run the demo:"
echo "  ./demo.sh"
