#!/bin/bash
echo "🐕 Building Braya Browser (C++)..."
mkdir -p build
cd build
cmake ..
make -j$(nproc)
echo ""
echo "✅ Build complete!"
echo "Run with: ./build/braya-browser"
