#!/bin/bash
# Braya Browser v1.0.0 RPM Build Script

set -e

VERSION="1.0.0"
NAME="braya-browser"
TARBALL="${NAME}-${VERSION}.tar.gz"

echo "🚀 Building Braya Browser v${VERSION} RPM Package..."

# Setup RPM build environment
echo "📁 Setting up RPM build directories..."
mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# Create source tarball
echo "📦 Creating source tarball..."
cd ..
tar --exclude='.git' \
    --exclude='build' \
    --exclude='*.log' \
    --exclude='*.md' \
    --exclude='.gitignore' \
    --transform "s,^braya-browser-cpp,${NAME}-${VERSION}," \
    -czf ~/rpmbuild/SOURCES/${TARBALL} braya-browser-cpp/
cd braya-browser-cpp

# Copy spec file
echo "📋 Copying spec file..."
cp braya-browser.spec ~/rpmbuild/SPECS/

# Build RPM
echo "🔨 Building RPM package..."
rpmbuild -ba ~/rpmbuild/SPECS/braya-browser.spec

# Copy built RPMs to current directory
echo "📤 Copying built RPMs..."
mkdir -p ./rpm-output
cp ~/rpmbuild/RPMS/x86_64/${NAME}-${VERSION}-*.rpm ./rpm-output/ 2>/dev/null || true
cp ~/rpmbuild/RPMS/noarch/${NAME}-${VERSION}-*.rpm ./rpm-output/ 2>/dev/null || true
cp ~/rpmbuild/SRPMS/${NAME}-${VERSION}-*.src.rpm ./rpm-output/ 2>/dev/null || true

echo ""
echo "✅ RPM BUILD COMPLETE!"
echo ""
echo "📦 Package location: ~/rpmbuild/RPMS/"
echo "📦 Also copied to: ./rpm-output/"
echo ""
echo "To install:"
echo "  sudo dnf install ./rpm-output/${NAME}-${VERSION}-1.*.x86_64.rpm"
echo ""
echo "Or:"
echo "  sudo rpm -ivh ./rpm-output/${NAME}-${VERSION}-1.*.x86_64.rpm"
