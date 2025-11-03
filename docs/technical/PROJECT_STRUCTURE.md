# Braya Browser - Project Structure

## 📁 Root Directory

```
braya-browser-cpp/
├── 📄 README.md                    # Main documentation
├── 📄 LICENSE                      # GPL-3.0 License
├── 📄 VERSION_1.0.1_PLAN.md        # Current version plan
├── 📄 SESSION_SUMMARY.md           # Latest work summary
├── 📄 TODO_NEXT_SESSION.md         # Next session priorities
├── 📄 CMakeLists.txt               # Build configuration
├── 📄 braya-browser.desktop        # Desktop entry file
├── 📄 braya-browser.spec           # RPM spec file
├── 🔧 build.sh                     # Quick build script
├── 🔧 build-release.sh             # Release build script
├── 🔧 build-rpm.sh                 # RPM package builder
├── 📁 src/                         # Source code
├── 📁 resources/                   # Assets (CSS, icons, HTML)
├── 📁 build/                       # Build output (gitignored)
└── 📁 .archive/                    # Old files (gitignored)
```

## 📁 Source Code (`src/`)

```
src/
├── main.cpp                        # Entry point
├── BrayaWindow.{h,cpp}             # Main browser window
├── BrayaTab.{h,cpp}                # Tab management
├── BrayaSettings.{h,cpp}           # Settings dialog
├── BrayaHistory.{h,cpp}            # History manager
├── BrayaDownloads.{h,cpp}          # Download manager
├── BrayaBookmarks.{h,cpp}          # Bookmarks manager
├── BrayaPasswordManager.{h,cpp}    # Password manager ✨ NEW
├── BrayaCustomization.{h,cpp}      # Theme customization
└── TabGroup.{h,cpp}                # Tab grouping
```

## 📁 Resources (`resources/`)

```
resources/
├── home.html                       # Browser home page
├── theme-dark.css                  # Dark theme
├── theme-light.css                 # Light theme  
├── theme-industrial.css            # Industrial theme
├── style.css                       # Legacy styles
├── password-detect.js              # Password form detection ✨ NEW
└── icons/
    ├── braya-browser.png           # Main icon (256x256)
    └── braya-logo-small.png        # Toolbar icon
```

## 🔨 Building

```bash
# Quick build
./build.sh

# Release build with tarball
./build-release.sh

# Build RPM package
./build-rpm.sh
```

## 🎯 Current Version: 1.0.0 → 1.0.1

### ✅ Completed (v1.0.1)
- Password Manager (core functionality)
- Basic settings with sidebar
- Icon improvements

### 🚧 In Progress
- Password auto-fill/auto-save
- Settings redesign
- Downloads fix

### 📦 Next Up
- Flatpak packaging
- Complete v1.0.1 release
