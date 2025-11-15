# 🐕 Braya Browser

A modern, highly customizable web browser built with C++ and WebKit.

**Version:** 1.0.6
**Platform:** Linux (GTK4)
**Engine:** WebKit2GTK 6.0

## Features

### Core Functionality
- **WebKit Engine** - Fast, modern web standards support
- **Vertical Tab Bar** - Sidebar with tab management
- **Extension Support** - Load Firefox & Chrome extensions (.xpi, .crx)
- **Password Manager** - AES-256 encrypted, auto-fill/auto-save
- **Bookmarks** - Visual bookmarks bar with folders, nesting, and favicons
- **History** - Full browsing history tracking
- **Downloads** - Built-in download manager
- **Themes** - Dark, Light, and Industrial themes

### Advanced Features
- **Tab Groups** - Organize tabs with color coding
- **Reader Mode** - Distraction-free reading (Alt+Shift+R)
- **Screenshots** - Save page screenshots (Ctrl+Shift+S)
- **Find in Page** - Advanced search (Ctrl+F)
- **Privacy Settings** - Ad blocking, tracker blocking, HTTPS-only mode, one-click cookie & site-data clearing
- **Customization** - 60+ appearance and behavior options

### Password Manager
- AES-256-CBC encryption with random IVs
- Auto-save on form submission
- Auto-fill on field focus
- Multi-account support per site
- Chrome CSV import/export
- Inline autofill dropdown anchored to fields with status toast
- Optional multi-step login capture (toggle in settings)

## Build Instructions

### Dependencies
```bash
sudo dnf install gcc-c++ cmake gtk4-devel webkit2gtk4.1-devel openssl-devel json-glib-devel libsodium-devel
```

### Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
```

### Install
```bash
sudo make install
```

### Build RPM Package
```bash
# Create source tarball
tar --exclude='./build' --exclude='./.git' -czf ~/rpmbuild/SOURCES/braya-browser-1.0.6.tar.gz --transform 's,^\.,braya-browser-1.0.6,' .

# Build RPM
rpmbuild -bb braya-browser.spec

# Install
sudo dnf install ~/rpmbuild/RPMS/x86_64/braya-browser-1.0.6-1.fc43.x86_64.rpm
```

## Usage

### Keyboard Shortcuts
- `Ctrl+T` - New tab
- `Ctrl+W` - Close tab
- `Ctrl+Tab` - Next tab
- `Ctrl+Shift+Tab` - Previous tab
- `Ctrl+L` - Focus address bar
- `Ctrl+D` - Bookmark current page
- `Ctrl+B` - Toggle sidebar
- `Ctrl+F` - Find in page
- `Ctrl+R` - Reload page
- `Ctrl+Shift+S` - Take screenshot
- `Alt+Shift+R` - Reader mode
- `Ctrl+K` - Password manager
- `Ctrl+J` - Downloads
- `F12` - Developer tools (WebKit Inspector)

### Extension Installation
Download `.xpi` (Firefox) or `.crx` (Chrome) files and they will auto-install.

## Project Structure
```
braya-browser-cpp/
├── src/                    # Source files
│   ├── main.cpp           # Entry point
│   ├── BrayaWindow.*      # Main window
│   ├── BrayaTab.*         # Tab implementation
│   ├── BrayaSettings.*    # Settings dialog
│   ├── BrayaBookmarks.*   # Bookmarks manager
│   ├── BrayaHistory.*     # History manager
│   ├── BrayaDownloads.*   # Download manager
│   ├── BrayaPasswordManager.* # Password manager
│   └── extensions/        # Extension system
├── resources/             # CSS themes, icons, assets
├── build/                 # Build output (gitignored)
├── CMakeLists.txt        # Build configuration
├── braya-browser.spec    # RPM spec file
└── TODO.md               # Development roadmap

```

## Development

### Version History
- **1.0.6** (Nov 15, 2025) - Password manager update and minor UI updates
- **1.0.5** (Nov 13, 2025) - Bookmark folders with nesting, drag-and-drop organization, context menus
- **1.0.2** (Nov 7, 2024) - UI refinements, enhanced bookmarks bar
- **1.0.1** (Nov 2-3, 2024) - Beta releases, password manager, extensions
- **1.0.0** (Nov 1, 2024) - Initial release

### Contributing
See `TODO.md` for the development roadmap and feature requests.

## License
MIT License - See LICENSE file for details

## Credits
Built by Corey O'Brien
Website: https://braya.dev

---

**Built with ❤️ using C++, GTK4, and WebKit**
