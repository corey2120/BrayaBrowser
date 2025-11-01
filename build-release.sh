#!/bin/bash
# Braya Browser v1.0.0 Release Package Builder

set -e

VERSION="1.0.0"
NAME="braya-browser"
RELEASE_DIR="release-${VERSION}"

echo "🚀 Creating Braya Browser v${VERSION} Release Package..."

# Clean previous builds
rm -rf ${RELEASE_DIR} ${NAME}-${VERSION}.tar.gz

# Create release directory structure
echo "📁 Creating release structure..."
mkdir -p ${RELEASE_DIR}/{bin,share/braya-browser/resources,share/applications}

# Build the browser
echo "🔨 Building browser..."
./build.sh

# Copy binary
echo "📦 Packaging files..."
cp build/braya-browser ${RELEASE_DIR}/bin/

# Copy resources
cp resources/*.css ${RELEASE_DIR}/share/braya-browser/resources/
cp resources/*.html ${RELEASE_DIR}/share/braya-browser/resources/

# Create desktop file
cat > ${RELEASE_DIR}/share/applications/${NAME}.desktop << 'DESKTOP_EOF'
[Desktop Entry]
Version=1.0
Type=Application
Name=Braya Browser
Comment=Modern, highly customizable web browser
Exec=braya-browser %U
Icon=braya-browser
Categories=Network;WebBrowser;
MimeType=text/html;text/xml;application/xhtml+xml;x-scheme-handler/http;x-scheme-handler/https;
Keywords=browser;web;internet;
StartupNotify=true
DESKTOP_EOF

# Create install script
cat > ${RELEASE_DIR}/install.sh << 'INSTALL_EOF'
#!/bin/bash
# Braya Browser Installation Script

if [ "$EUID" -ne 0 ]; then 
    echo "Please run with sudo: sudo ./install.sh"
    exit 1
fi

echo "Installing Braya Browser v1.0.0..."

# Copy files
cp bin/braya-browser /usr/local/bin/
chmod 755 /usr/local/bin/braya-browser

mkdir -p /usr/local/share/braya-browser/resources
cp share/braya-browser/resources/* /usr/local/share/braya-browser/resources/

mkdir -p /usr/local/share/applications
cp share/applications/braya-browser.desktop /usr/local/share/applications/

# Update desktop database
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database /usr/local/share/applications
fi

echo "✅ Installation complete!"
echo "Launch with: braya-browser"
INSTALL_EOF

chmod +x ${RELEASE_DIR}/install.sh

# Create uninstall script
cat > ${RELEASE_DIR}/uninstall.sh << 'UNINSTALL_EOF'
#!/bin/bash
# Braya Browser Uninstallation Script

if [ "$EUID" -ne 0 ]; then 
    echo "Please run with sudo: sudo ./uninstall.sh"
    exit 1
fi

echo "Uninstalling Braya Browser..."

rm -f /usr/local/bin/braya-browser
rm -rf /usr/local/share/braya-browser
rm -f /usr/local/share/applications/braya-browser.desktop

if command -v update-desktop-database &> /dev/null; then
    update-desktop-database /usr/local/share/applications
fi

echo "✅ Uninstallation complete!"
UNINSTALL_EOF

chmod +x ${RELEASE_DIR}/uninstall.sh

# Copy documentation
cp README.md ${RELEASE_DIR}/
cp LICENSE ${RELEASE_DIR}/

# Create tarball
echo "📦 Creating tarball..."
tar -czf ${NAME}-${VERSION}.tar.gz ${RELEASE_DIR}

# Create checksums
echo "🔐 Creating checksums..."
sha256sum ${NAME}-${VERSION}.tar.gz > ${NAME}-${VERSION}.tar.gz.sha256

echo ""
echo "✅ RELEASE PACKAGE CREATED!"
echo ""
echo "📦 Package: ${NAME}-${VERSION}.tar.gz"
echo "📝 Checksum: ${NAME}-${VERSION}.tar.gz.sha256"
echo ""
echo "To install:"
echo "  tar -xzf ${NAME}-${VERSION}.tar.gz"
echo "  cd ${RELEASE_DIR}"
echo "  sudo ./install.sh"
echo ""
echo "To uninstall:"
echo "  cd ${RELEASE_DIR}"
echo "  sudo ./uninstall.sh"
