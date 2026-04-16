#!/bin/bash
# Braya Browser RPM Build Script

set -e

VERSION="1.0.9"
NAME="braya-browser"
TARBALL="${NAME}-${VERSION}.tar.gz"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "🚀 Building Braya Browser v${VERSION} RPM Package..."

# Setup RPM build environment
echo "Setting up RPM build directories..."
mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# Create source tarball from the project directory
echo "Creating source tarball..."
tar --exclude='.git' \
    --exclude='build' \
    --exclude='cmake-build-debug' \
    --exclude='rpm-output' \
    --exclude='*.rpm' \
    --exclude='*.log' \
    --exclude='.gitignore' \
    --transform "s,^BrayaBrowser,${NAME}-${VERSION}," \
    -czf ~/rpmbuild/SOURCES/${TARBALL} \
    -C "$PROJECT_DIR" BrayaBrowser/

# Copy spec file
echo "Copying spec file..."
cp "$SCRIPT_DIR/braya-browser.spec" ~/rpmbuild/SPECS/

# Build RPM
echo "Building RPM package..."
rpmbuild -ba ~/rpmbuild/SPECS/braya-browser.spec

# Copy built RPMs back to project directory
echo "Copying built RPMs..."
mkdir -p "$SCRIPT_DIR/rpm-output"
cp ~/rpmbuild/RPMS/x86_64/${NAME}-${VERSION}-*.rpm "$SCRIPT_DIR/rpm-output/" 2>/dev/null || true
cp ~/rpmbuild/SRPMS/${NAME}-${VERSION}-*.src.rpm "$SCRIPT_DIR/rpm-output/" 2>/dev/null || true

echo ""
echo "RPM BUILD COMPLETE!"
echo ""
echo "Package location: ~/rpmbuild/RPMS/x86_64/"
echo "Also copied to:   $SCRIPT_DIR/rpm-output/"
echo ""
echo "To install:"
echo "  sudo dnf install $SCRIPT_DIR/rpm-output/${NAME}-${VERSION}-${RELEASE}.*.x86_64.rpm"
