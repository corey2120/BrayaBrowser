#!/bin/bash
# BrayaBrowser AUR Package Build Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="1.0.6"

echo "╔══════════════════════════════════════════════════════════════════════════════╗"
echo "║           BrayaBrowser AUR Package Builder                                   ║"
echo "╚══════════════════════════════════════════════════════════════════════════════╝"
echo ""

# Step 1: Create source tarball
echo "📦 Creating source tarball..."
cd "$SCRIPT_DIR/.."
tar --exclude='./BrayaBrowser/build*' \
    --exclude='./BrayaBrowser/.git' \
    --exclude='./BrayaBrowser/*.rpm' \
    --exclude='./BrayaBrowser/*.tar.gz' \
    --exclude='./braya-browser-aur' \
    --exclude='./braya-browser-*.tar.gz' \
    -czf "braya-browser-${VERSION}.tar.gz" \
    --transform "s,^./BrayaBrowser,braya-browser-${VERSION}," \
    ./BrayaBrowser
echo "✓ Created: $(pwd)/braya-browser-${VERSION}.tar.gz"

# Step 2: Create build directory
echo ""
echo "📁 Setting up build directory..."
BUILD_DIR="$SCRIPT_DIR/../braya-browser-aur"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
echo "✓ Build directory: $BUILD_DIR"

# Step 3: Copy necessary files
echo ""
echo "📋 Copying package files..."
cp "$SCRIPT_DIR/../braya-browser-${VERSION}.tar.gz" .
cp "$SCRIPT_DIR/PKGBUILD.local" ./PKGBUILD
echo "✓ Files copied"

# Step 4: Clean previous build
echo ""
echo "🧹 Cleaning previous build..."
rm -rf src pkg *.pkg.tar.zst
echo "✓ Cleaned"

# Step 5: Build package
echo ""
echo "🔨 Building package..."
makepkg -sf --noconfirm

# Step 6: Summary
echo ""
echo "╔══════════════════════════════════════════════════════════════════════════════╗"
echo "║                              BUILD COMPLETE!                                 ║"
echo "╚══════════════════════════════════════════════════════════════════════════════╝"
echo ""
echo "📦 Package created:"
ls -lh *.pkg.tar.zst | grep -v debug
echo ""
echo "🚀 To install:"
echo "   cd $BUILD_DIR"
echo "   sudo pacman -U braya-browser-${VERSION}-1-x86_64.pkg.tar.zst"
echo ""
echo "▶️  To run:"
echo "   braya-browser"
echo ""
