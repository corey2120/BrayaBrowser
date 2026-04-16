# Add This to Your README.md

Add this section to your README.md to show AUR installation instructions:

```markdown
## Installation

### Arch Linux (AUR)

[![AUR version](https://img.shields.io/aur/version/braya-browser)](https://aur.archlinux.org/packages/braya-browser)
[![AUR votes](https://img.shields.io/aur/votes/braya-browser)](https://aur.archlinux.org/packages/braya-browser)

BrayaBrowser is available on the Arch User Repository (AUR):

#### Using an AUR helper (recommended):
```bash
# Using yay
yay -S braya-browser

# Using paru
paru -S braya-browser
```

#### Manual installation:
```bash
git clone https://aur.archlinux.org/braya-browser.git
cd braya-browser
makepkg -si
```

### Build from Source (Other Distributions)

#### Dependencies (Arch/Manjaro):
```bash
sudo pacman -S --needed base-devel cmake gtk4 webkitgtk-6.0 openssl json-glib libsodium pkg-config git
```

#### Dependencies (Fedora):
```bash
sudo dnf install gcc-c++ cmake gtk4-devel webkit2gtk4.1-devel openssl-devel json-glib-devel libsodium-devel
```

#### Build & Install:
```bash
git clone https://github.com/corey2120/BrayaBrowser.git
cd BrayaBrowser
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install
```

## Quick Start

After installation, launch BrayaBrowser:
```bash
braya-browser
```

Or find it in your application menu under "Internet" or "Web Browser".
```

---

## Alternative Compact Version

For a more compact README section:

```markdown
## Installation

### 🐧 Arch Linux (AUR)
```bash
yay -S braya-browser
```
Or: `paru -S braya-browser`

### 🔨 Build from Source
```bash
# Install dependencies (Arch)
sudo pacman -S base-devel cmake gtk4 webkitgtk-6.0 openssl json-glib libsodium

# Build
git clone https://github.com/corey2120/BrayaBrowser.git
cd BrayaBrowser
mkdir build && cd build
cmake .. && make -j$(nproc)
sudo make install
```

Run with: `braya-browser`
```

---

## Badges to Add at Top of README

Add these after your title:

```markdown
# 🐕 Braya Browser

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![AUR version](https://img.shields.io/aur/version/braya-browser)](https://aur.archlinux.org/packages/braya-browser)
[![GitHub release](https://img.shields.io/github/v/release/corey2120/BrayaBrowser)](https://github.com/corey2120/BrayaBrowser/releases)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-blue)](https://www.linux.org/)
[![WebKit](https://img.shields.io/badge/engine-WebKit-orange)](https://webkit.org/)
```

---

## Social Media Announcement Template

Use this to announce on Reddit, Twitter, etc:

```
🎉 BrayaBrowser is now available on the AUR!

A modern, customizable web browser built with C++ and WebKit for Linux.

Features:
✅ Vertical tabs
✅ Extension support (Firefox & Chrome)
✅ Built-in password manager (AES-256)
✅ Tab groups & reader mode
✅ Multiple themes
✅ Privacy-focused

Install on Arch Linux:
$ yay -S braya-browser

Source: https://github.com/corey2120/BrayaBrowser
AUR: https://aur.archlinux.org/packages/braya-browser

#Linux #OpenSource #ArchLinux #AUR #WebBrowser #Privacy
```
