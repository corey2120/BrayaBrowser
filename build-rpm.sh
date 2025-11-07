#!/bin/bash
# Braya Browser v1.0.1-beta12 RPM Build Script

set -e

VERSION="1.0.1"
NAME="braya-browser"
TARBALL="${NAME}-${VERSION}.tar.gz"

echo "🚀 Building Braya Browser v${VERSION}-beta12 RPM Package..."

# Setup RPM build environment
echo "📁 Setting up RPM build directories..."
mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# Create source tarball
echo "📦 Creating source tarball..."
cd ..
tar --exclude='.git' \
    --exclude='build' \
    --exclude='rpm-output' \
    --exclude='release-1.0.0' \
    --exclude='*.log' \
    --exclude='.gitignore' \
    --exclude='BUTTON_FIX_COMPLETE.md' \
    --exclude='CRASH_FIX.md' \
    --exclude='CURRENT_STATUS.md' \
    --exclude='DOWNLOAD_FIX.md' \
    --exclude='END_OF_DAY_STATUS.md' \
    --exclude='FEATURE_*.md' \
    --exclude='FIXES_APPLIED.md' \
    --exclude='IMPROVEMENTS_PLAN.md' \
    --exclude='LATEST_FIXES.md' \
    --exclude='MINIMAL_FIX_PLAN.md' \
    --exclude='NEW_DESIGN_PLAN.md' \
    --exclude='NEXT_IMPROVEMENTS.md' \
    --exclude='QUICK_WINS.md' \
    --exclude='README_TABS.md' \
    --exclude='REAL_REFINEMENT.md' \
    --exclude='REFINEMENT_*.md' \
    --exclude='ROADMAP_TO_AMAZING.md' \
    --exclude='SETTINGS_*.md' \
    --exclude='SPACE_OPTIMIZATION.md' \
    --exclude='TAB_*.md' \
    --exclude='TAB_*.txt' \
    --exclude='TASK.md' \
    --exclude='TESTING_GUIDE.md' \
    --exclude='THIS_SESSION.md' \
    --exclude='TINY_CLOSE_FIX.md' \
    --exclude='UI_*.md' \
    --exclude='UI_*.txt' \
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
