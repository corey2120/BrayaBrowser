# Braya Browser v1.0.0 🎉

A modern, highly customizable web browser built with C++ and WebKit.

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)

## 🌟 Features

### Core Browsing
- **WebKit Engine** - Fast, modern web standards support
- **Tab Management** - Full tab controls with tab groups
- **Navigation** - Back, forward, reload, home buttons
- **Custom Home Page** - Beautiful about:braya landing page
- **Multi-tab Support** - Unlimited tabs with smooth switching

### Organization
- **Tab Groups** - Color-coded, named groups with collapse/expand
- **Bookmarks Manager** (Ctrl+B) - Full CRUD with folders and search
- **History Tracking** (Ctrl+H) - Complete browsing history with search
- **Find in Page** (Ctrl+F) - Real-time search with match counter

### Customization (Vivaldi-Level!)
**60+ Customization Options:**
- 🎨 **Colors** (20+ options) - Every UI element customizable
- ✍️ **Typography** (15+ options) - Fonts, sizes, weights, spacing
- 📐 **Layout** (15+ options) - Dimensions, padding, border radius
- ✨ **Effects** (10+ options) - Shadows, glow, animations, transparency

### Themes
- **Dark Theme** (Default) - Easy on the eyes
- **Light Theme** - Clean and bright
- **Industrial Theme** - Professional look
- **Custom Themes** - Build your own with the customization panel

### Settings
- **General** - Homepage, search engine, startup behavior
- **Appearance** - Themes and advanced customization
- **Privacy** - Tracking protection, cookies, cache management
- **Security** - Security settings and warnings
- **Advanced** - Developer tools and experimental features

### Downloads & More
- Download manager with progress tracking
- Privacy-focused design
- Keyboard shortcuts
- Professional UI with GTK4

## 📦 Installation

### Quick Install (Tarball)
```bash
# Download and extract
tar -xzf braya-browser-1.0.0.tar.gz
cd release-1.0.0

# Install
sudo ./install.sh
```

### From Source
```bash
# Install dependencies (Fedora/RHEL)
sudo dnf install gtk4-devel webkit2gtk4.1-devel cmake gcc-c++

# Clone and build
git clone https://github.com/corey2120/BrayaBrowser.git
cd BrayaBrowser
./build.sh

# Run
./build/braya-browser
```

## 🔨 Building

### Standard Build
```bash
./build.sh
./build/braya-browser
```

### Build Release Package
```bash
./build-release.sh
```

This creates `braya-browser-1.0.0.tar.gz` with install/uninstall scripts.

### Build RPM (Optional)
```bash
sudo dnf install rpm-build rpmdevtools
./build-rpm.sh
```

## ⌨️ Keyboard Shortcuts

- `Ctrl+T` - New tab
- `Ctrl+W` - Close tab
- `Ctrl+Tab` - Next tab
- `Ctrl+Shift+Tab` - Previous tab
- `Ctrl+L` - Focus URL bar
- `Ctrl+H` - History
- `Ctrl+B` - Bookmarks
- `Ctrl+F` - Find in page
- `Ctrl+R` / `F5` - Reload
- `Alt+Left` - Back
- `Alt+Right` - Forward
- `Alt+Home` - Home

## 🎨 Customization

Access the advanced customization panel:
1. Click the ⚙️ Settings button
2. Go to "Appearance" tab
3. Click "🎨 Advanced Customization"
4. Customize 60+ options across 4 categories!

## 🏗️ Technical Details

- **Language:** C++17
- **UI Framework:** GTK4
- **Rendering Engine:** WebKit2GTK 6.0
- **Build System:** CMake
- **Platform:** Linux

## 📋 Requirements

- GTK4
- WebKit2GTK 4.1+
- CMake 3.10+
- GCC 7+ or Clang 5+

## 🤝 Contributing

Contributions welcome! Please open an issue or pull request.

## 📄 License

MIT License - See LICENSE file for details

## 🎯 Roadmap

- [x] Core browsing functionality
- [x] Tab groups
- [x] History & bookmarks
- [x] Advanced customization
- [x] Multiple themes
- [ ] Extensions support
- [ ] Sync across devices
- [ ] Split view mode
- [ ] Tab stacking

## 🙏 Credits

Built with passion by Corey O'Brien

Powered by WebKit and GTK

---

**Braya Browser v1.0.0** - The most customizable browser you'll ever use! 🚀
