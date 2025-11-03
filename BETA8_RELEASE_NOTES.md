# Braya Browser v1.0.1-beta8 Release Notes

**Release Date**: November 3, 2024  
**Build**: 1.0.1-0.8.beta8  

## 🎉 Major Feature Release: Quick Wins + Visual Bookmarks

This release adds **7 major new features** that make Braya feel like a complete, modern browser!

---

## 🆕 What's New

### 📖 Reader Mode
**Distraction-free reading at your fingertips!**

- **Clean Layout**: Extracts main content and removes ads, navbars, sidebars
- **Beautiful Typography**: Georgia serif, optimal line-height, centered layout
- **Smart Detection**: Finds article content intelligently
- **Toggle On/Off**: Press again to return to normal view
- **Keyboard Shortcut**: `Alt+Shift+R`
- **Toolbar Button**: 📄 icon in navbar

### 📸 Screenshot Tool
**Capture any webpage instantly!**

- **Full Visible Area**: Captures what you see on screen
- **Auto-Save**: Saves to `~/Pictures/` with timestamp
- **PNG Format**: High-quality screenshots
- **Confirmation Dialog**: Shows save location
- **Keyboard Shortcut**: `Ctrl+Shift+S`
- **Toolbar Button**: 📷 icon in navbar

### 📚 Visual Bookmarks Bar
**Beautiful bookmarks with real favicons!**

- **Real Favicons**: Automatically downloads and caches site icons
- **Modern Design**: Clean, minimal with hover effects
- **Quick Add**: Click ⭐ star or press `Ctrl+D`
- **Tooltips**: Hover to see full title and URL
- **Favicon Cache**: Stored in `~/.config/braya-browser/favicons`
- **Smooth Animations**: Hover to see lift effect

### ⭐ Speed Dial / New Tab Page
**Quick access to your favorite sites!**

- **Grid Layout**: 4 columns, 3 rows (12 sites)
- **Large Icons**: 64px favicons for easy recognition
- **Hover Effects**: Smooth lift animation with shadow
- **Add Tile**: Plus button to add more bookmarks
- **Auto-Populate**: Shows your most recent bookmarks
- **Beautiful Design**: White tiles on subtle gray background

### 📌 Tab Pinning (Backend)
**Keep important tabs always visible**

- Backend infrastructure complete
- Pin/unpin functionality ready
- State tracking implemented
- UI coming in next release

### 🔇 Tab Muting (Backend)
**Control audio from tabs**

- Backend infrastructure complete
- Mute/unmute using WebKit API
- State tracking implemented
- UI coming in next release

---

## 🎨 UI/UX Improvements

### Modern Styling
- Smooth transitions on all interactive elements
- Box shadows for depth
- Hover effects with lift animations
- Clean, minimal design language
- Consistent spacing and margins

### Keyboard Shortcuts
Added 4 new shortcuts:
- `Alt+Shift+R` - Toggle Reader Mode
- `Ctrl+Shift+S` - Take Screenshot
- `Ctrl+D` - Bookmark Current Page
- `Ctrl+Shift+P` - Password Manager (from Beta7)

---

## 🔧 Technical Details

### New Components

**Reader Mode** (`BrayaTab.cpp`)
- JavaScript content extraction
- CSS styling injection
- Article detection algorithms
- Toggle state management

**Screenshot Tool** (`BrayaWindow.cpp`)
- WebKit snapshot API integration
- GdkTexture to PNG conversion
- File dialog with confirmation
- Timestamp filename generation

**Visual Bookmarks** (`BrayaBookmarks.cpp`)
- Favicon downloading and caching
- Scrollable bookmarks bar
- Dynamic bookmark rendering
- Folder support infrastructure

**Speed Dial** (`BrayaBookmarks.cpp`)
- GTK Grid layout
- Tile-based design
- Favorite bookmarks selection
- Add bookmark integration

### Files Modified
- `BrayaWindow.h/cpp` - Window-level features
- `BrayaTab.h/cpp` - Tab-level features
- `BrayaBookmarks.h/cpp` - Bookmark system
- `style.css` - Visual styling
- `braya-browser.spec` - Package metadata
- `build-rpm.sh` - Build script

### Lines of Code
- **Added**: ~500+ lines
- **Reader Mode**: ~120 lines
- **Screenshot**: ~60 lines
- **Visual Bookmarks**: ~250 lines
- **Speed Dial**: ~100 lines
- **CSS**: ~60 lines

---

## 📦 Installation

### Fresh Install
```bash
sudo dnf install ./rpm-output/braya-browser-1.0.1-0.8.beta8.fc43.x86_64.rpm
```

### Upgrade from Beta7
```bash
sudo dnf upgrade ./rpm-output/braya-browser-1.0.1-0.8.beta8.fc43.x86_64.rpm
```

---

## 🧪 Testing Guide

### Test Reader Mode
1. Visit a news article or blog post
2. Click 📄 button in toolbar (or press `Alt+Shift+R`)
3. **Expected**: Clean, centered reading view
4. Press again to exit reader mode

### Test Screenshot Tool
1. Navigate to any page
2. Click 📷 button (or press `Ctrl+Shift+S`)
3. **Expected**: Dialog shows "Screenshot saved to: ~/Pictures/..."
4. Check `~/Pictures/` for PNG file

### Test Visual Bookmarks
1. Visit a site with a favicon
2. Click ⭐ star button (or press `Ctrl+D`)
3. **Expected**: Bookmark appears in bookmarks bar with favicon
4. Hover over bookmark to see tooltip
5. Check `~/.config/braya-browser/favicons/` for cached icon

### Test Speed Dial
1. Add several bookmarks (use Ctrl+D)
2. Open new tab
3. **Expected**: Grid of bookmarks with favicons
4. Hover over tiles to see lift animation

---

## 🎯 Comparison: Beta7 vs Beta8

| Feature | Beta7 | Beta8 | Change |
|---------|-------|-------|--------|
| Reader Mode | ❌ | ✅ | **NEW** |
| Screenshot Tool | ❌ | ✅ | **NEW** |
| Visual Bookmarks | ❌ | ✅ | **NEW** |
| Speed Dial | ❌ | ✅ | **NEW** |
| Favicon Caching | ❌ | ✅ | **NEW** |
| Tab Pinning | ❌ | ⚠️ Backend | **NEW** |
| Tab Muting | ❌ | ⚠️ Backend | **NEW** |
| Keyboard Shortcuts | 3 | 7 | **+4** |
| Chrome Import Fix | ✅ | ✅ | Maintained |
| Password Key Icons | ✅ | ✅ | Maintained |

---

## 📊 Statistics

### This Release
- **New Features**: 7 (4 complete, 3 backend)
- **Keyboard Shortcuts**: +4 new shortcuts
- **Files Modified**: 8 files
- **Code Added**: ~500 lines
- **Build Time**: ~3 minutes
- **Package Size**: 2.3 MB

### Overall Progress (v1.0.0 → Beta8)
- **Total Features**: 35+
- **Password Manager**: ✅ Complete with AES-256
- **Visual Polish**: ✅ Modern UI/UX
- **Bookmarks**: ✅ Visual with favicons
- **Quick Access**: ✅ Speed dial
- **Content Tools**: ✅ Reader mode, screenshots
- **Tab Features**: ⚠️ Backend ready

---

## 🚀 What Makes Beta8 Special

### User Experience
- **Complete Feature Set**: Browser feels mature and polished
- **Visual Polish**: Modern design with animations
- **Instant Access**: Speed dial, visual bookmarks
- **Content Focus**: Reader mode for better reading
- **Quick Actions**: Screenshot, bookmark with one click

### Technical Excellence
- **Clean Code**: Well-organized, maintainable
- **Modern APIs**: WebKit 6.0, GTK4
- **Performance**: Fast, responsive
- **Security**: AES-256 encryption for passwords
- **Storage**: Smart caching for favicons

---

## 🐛 Known Issues

1. **Tab pinning/muting UI** not yet visible (backend ready)
2. **Speed dial** only shows recent bookmarks (not most visited yet)
3. **Reader mode** may not work on all sites (depends on page structure)
4. **Bookmark folders** not yet implemented in UI

---

## 🔜 What's Next

### High Priority (Beta9)
- [ ] Tab pinning visual indicators
- [ ] Tab mute button on tabs
- [ ] Bookmark folder dropdowns
- [ ] Drag & drop bookmark reordering

### Medium Priority
- [ ] Extensions/add-ons support (BIG!)
- [ ] Import bookmarks from Chrome/Firefox
- [ ] Picture-in-Picture video
- [ ] Page translation

### Polish
- [ ] Dark mode for reader mode
- [ ] Screenshot annotation tool
- [ ] Bookmark sync
- [ ] Most visited tracking

---

## 🙏 Thank You!

This release adds **7 major features** that make Braya feel like a **real, complete browser**!

**What's Working**:
- ✅ Full-featured password manager
- ✅ Visual bookmarks with favicons
- ✅ Reader mode for clean reading
- ✅ Screenshot tool
- ✅ Speed dial for quick access
- ✅ Modern, polished UI

**What's New**:
- 📖 Reader Mode
- 📸 Screenshots
- 📚 Visual Bookmarks
- ⭐ Speed Dial

Test it out and let us know what you think!

---

**Package**: `braya-browser-1.0.1-0.8.beta8.fc43.x86_64.rpm`  
**Size**: 2.3 MB  
**Build Date**: November 3, 2024  
**Contact**: corey@braya.dev  
**GitHub**: https://github.com/corey2120/BrayaBrowser

---

## 🎉 Braya is becoming AMAZING! 🎉

7 new features, beautiful UI, and it feels like a complete browser now! 🚀
