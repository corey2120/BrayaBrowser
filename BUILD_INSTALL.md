# BrayaBrowser - Build and Installation Guide

## Quick Build (Local)

### 1. Install Dependencies (Arch Linux)
```bash
sudo pacman -S --needed base-devel cmake gtk4 webkitgtk-6.0 openssl json-glib libsodium pkg-config git
```

### 2. Build from Source
```bash
cd /home/cobrien/Documents/BrayaBrowser
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
```

### 3. Install
```bash
cd /home/cobrien/Documents/BrayaBrowser/build
sudo make install
```

### 4. Run
```bash
braya-browser
```

---

## Install via AUR Package

### Method 1: Install Pre-built Package
```bash
cd /home/cobrien/Documents/braya-browser-aur
sudo pacman -U braya-browser-1.0.6-1-x86_64.pkg.tar.zst
```

### Method 2: Build and Install from PKGBUILD (Local Testing)
```bash
# Create source tarball
cd /home/cobrien/Documents/BrayaBrowser
tar --exclude='./build*' --exclude='./.git' --exclude='*.rpm' --exclude='*.tar.gz' \
    -czf ../braya-browser-1.0.6.tar.gz --transform 's,^\.,braya-browser-1.0.6,' .

# Build package
cd /home/cobrien/Documents
mkdir -p braya-browser-aur && cd braya-browser-aur
cp ../braya-browser-1.0.6.tar.gz .
cp ../BrayaBrowser/PKGBUILD.local ./PKGBUILD
makepkg -si

# Or just install without building again
sudo pacman -U braya-browser-1.0.6-1-x86_64.pkg.tar.zst
```

---

## Publishing to AUR

### Prerequisites
1. AUR account (https://aur.archlinux.org)
2. SSH key added to AUR account
3. Git configured with your email

### Steps to Publish

1. **Clone AUR repository** (first time only):
```bash
cd ~/aur
git clone ssh://aur@aur.archlinux.org/braya-browser.git
cd braya-browser
```

2. **Copy files to AUR repo**:
```bash
cp /home/cobrien/Documents/BrayaBrowser/PKGBUILD .
cp /home/cobrien/Documents/BrayaBrowser/.SRCINFO .
```

3. **Review and commit**:
```bash
git add PKGBUILD .SRCINFO
git commit -m "Update to version 1.0.6"
```

4. **Push to AUR**:
```bash
git push
```

### Update AUR Package (for future versions)

1. Update version in PKGBUILD
2. Regenerate .SRCINFO:
   ```bash
   makepkg --printsrcinfo > .SRCINFO
   ```
3. Commit and push as above

---

## Installing from AUR (For End Users)

Once published to AUR, users can install using an AUR helper:

### Using yay
```bash
yay -S braya-browser
```

### Using paru
```bash
paru -S braya-browser
```

### Manual installation from AUR
```bash
git clone https://aur.archlinux.org/braya-browser.git
cd braya-browser
makepkg -si
```

---

## Uninstall

```bash
sudo pacman -R braya-browser
```

---

## Troubleshooting

### Missing Dependencies
If you get errors about missing dependencies:
```bash
sudo pacman -S gtk4 webkitgtk-6.0 openssl json-glib libsodium
```

### Build Errors
If you encounter build errors, make sure all build dependencies are installed:
```bash
sudo pacman -S base-devel cmake pkg-config gcc make git
```

### Clean Build
If you need to start fresh:
```bash
cd /home/cobrien/Documents/BrayaBrowser
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
```

---

## Package Information

- **Package Name**: braya-browser
- **Version**: 1.0.6
- **Architecture**: x86_64
- **License**: MIT
- **Homepage**: https://github.com/corey2120/BrayaBrowser
- **Maintainer**: Corey O'Brien <corey@braya.dev>

## Dependencies

### Runtime Dependencies
- gtk4
- webkitgtk-6.0
- openssl
- json-glib
- libsodium

### Build Dependencies
- cmake
- gcc
- make
- pkg-config
- git

---

## Notes

- The package installs to `/usr/bin/braya-browser`
- Resources are installed to `/usr/share/braya-browser/`
- Icons are installed to `/usr/share/icons/hicolor/`
- Desktop file is installed to `/usr/share/applications/`
- Web extension library is installed to `/usr/lib/braya-browser/web-extensions/`
