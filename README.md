# 🐕 Braya Browser

A fast, minimal web browser built with WebKit and GTK4.

![License](https://img.shields.io/badge/license-MIT-blue.svg)

## Features

- 🚀 **Fast** - Powered by WebKit engine
- 🎨 **Clean Design** - Minimal, distraction-free interface
- 🔒 **Privacy First** - Built-in tracker blocking
- ⌨️ **Keyboard Shortcuts** - Full keyboard navigation
- 🐕 **Custom Home Page** - Quick access to favorite sites

## Screenshots

*Coming soon*

## Building

### Requirements
- GTK4
- WebKit2GTK-4.1
- CMake 3.10+
- C++17 compiler

### Build Steps

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install libgtk-4-dev libwebkit2gtk-4.1-dev cmake g++

# Clone and build
git clone <your-repo-url>
cd braya-browser-cpp
./build.sh

# Run
./build/braya-browser
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+T` | New Tab |
| `Ctrl+W` | Close Tab |
| `Ctrl+L` | Focus URL Bar |
| `Ctrl+R` | Reload Page |
| `Alt+Left` | Go Back |
| `Alt+Right` | Go Forward |
| `F12` | Developer Tools |

## Current Status

✅ Working:
- Tab management
- Navigation (back/forward/reload/home)
- Custom home page with quick links
- Bookmarks bar
- Developer tools
- Keyboard shortcuts

🚧 In Progress:
- History tracking
- Downloads management
- Find in page
- Bookmarks manager
- Customizable home page

See [CURRENT_STATUS.md](CURRENT_STATUS.md) for detailed roadmap.

## Contributing

This is a personal project but suggestions and bug reports are welcome!

## License

MIT License - See LICENSE file for details

## About

Braya Browser is a lightweight, minimal web browser focused on simplicity and speed. Built as an alternative to bloated modern browsers, it aims to provide just the essentials you need for browsing the web.
