# Braya Browser

A modern, privacy-focused web browser built with C++ and WebKitGTK.

**Version:** 1.0.9  
**Platform:** Linux (GTK4)  
**Engine:** WebKitGTK 6.0 (WebKit2GTK)

---

## Features

### Core
- **WebKit Engine** — Fast, modern web standards support (WebKitGTK 6.0)
- **Vertical Tab Bar** — Sidebar with tab management, tab groups, and color coding
- **Password Manager** — AES-256 encrypted, auto-fill and auto-save
- **Bookmarks** — Visual bar with folders, nesting, drag-and-drop, and favicons
- **History** — Full browsing history with search
- **Downloads** — Built-in download manager
- **Themes** — Dark, Light, and Industrial themes

### Privacy & Security
- **Ad Blocker** — uBlock Origin filter lists (EasyList, EasyPrivacy, uBlock filters)
- **Tracker Blocking** — Compiled WebKit content rules for zero-overhead blocking
- **HTTPS-Only Mode** — Enforce secure connections
- **Cookie & Site Data Clearing** — One-click from the toolbar
- **Security Level Selector** — Off / Minimal / Standard / Strict / Custom

### Browser Features
- **Tab Groups** — Organize tabs with color coding
- **Tab Suspension** — Automatically suspend inactive tabs to save memory
- **Split View** — Side-by-side or top-bottom dual pane browsing
- **Reader Mode** — Distraction-free reading (`Alt+Shift+R`)
- **Find in Page** — Full-text search (`Ctrl+F`)
- **Screenshots** — Save page screenshots (`Ctrl+Shift+S`)
- **Extension Support** — Load Firefox (`.xpi`) and Chrome (`.crx`) extensions
- **Favicon Display** — JS-based favicon fetch with ICO and PNG fallback

### Password Manager
- AES-256-CBC encryption with random IVs
- Auto-save on form submission
- Auto-fill on field focus with inline dropdown
- Multi-account support per site
- Chrome CSV import/export
- Status toast notifications
- Optional multi-step login capture

---

## Build

### Dependencies (Fedora / RPM)
```bash
sudo dnf install gcc-c++ cmake gtk4-devel webkitgtk6.0-devel \
    openssl-devel json-glib-devel libsodium-devel libcurl-devel
```

### Build from Source
```bash
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install
```

### Build RPM Package
```bash
./build-rpm.sh
sudo dnf install rpm-output/braya-browser-1.0.9-*.x86_64.rpm
```

---

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+T` | New tab |
| `Ctrl+W` | Close tab |
| `Ctrl+Tab` | Next tab |
| `Ctrl+Shift+Tab` | Previous tab |
| `Ctrl+L` | Focus address bar |
| `Ctrl+D` | Bookmark current page |
| `Ctrl+B` | Toggle sidebar |
| `Ctrl+F` | Find in page |
| `Ctrl+R` | Reload page |
| `Ctrl+Shift+S` | Take screenshot |
| `Alt+Shift+R` | Reader mode |
| `Ctrl+K` | Password manager |
| `Ctrl+J` | Downloads |
| `F12` | Developer tools |

---

## Project Structure

```
BrayaBrowser/
├── src/
│   ├── main.cpp
│   ├── BrayaWindow.*          # Main window
│   ├── BrayaTab.*             # Per-tab WebView + favicon
│   ├── BrayaSettings.*        # Settings dialog
│   ├── BrayaBookmarks.*       # Bookmarks manager
│   ├── BrayaHistory.*         # History manager
│   ├── BrayaDownloads.*       # Download manager
│   ├── BrayaPasswordManager.* # Password manager
│   ├── adblocker/
│   │   └── BrayaAdBlocker.*   # uBlock Origin ad-blocker
│   └── extensions/            # Extension system
├── resources/                 # CSS themes, icons, JS, HTML
├── CMakeLists.txt
├── braya-browser.spec         # RPM spec
└── build-rpm.sh               # RPM build script
```

---

## Version History

| Version | Date | Changes |
|---|---|---|
| **1.0.9** | Dec 2025 | WebKitGTK 6.0 / Fedora 44, hardware accel fix, tab suspension, split view |
| **1.0.7** | Dec 2025 | uBlock Origin ad-blocker, JS favicon fetch, URL bar select-all on focus |
| **1.0.6** | Nov 2025 | Password manager update, UI refinements |
| **1.0.5** | Nov 2025 | Bookmark folders, nesting, drag-and-drop |
| **1.0.1** | Nov 2024 | Password manager, extension support |
| **1.0.0** | Nov 2024 | Initial release |

---

## License

MIT License — see [LICENSE](LICENSE)

## Credits

Built by Corey O'Brien  
https://braya.dev

---

*Built with C++, GTK4, and WebKitGTK*
