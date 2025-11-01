#!/bin/bash
echo "🐕 Building Braya Browser (C++)..."
cd build
cmake ..
make -j$(nproc)
echo ""
echo "✅ Build complete!"
echo "Run with: ./build/braya-browser"
