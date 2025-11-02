# TODO: Next Session
**Priority Order for Next Work Session**

## 🔥 CRITICAL - Do First
1. **Fix Downloads Crash** (30-45 min)
   - Debug WebKit download signals
   - Implement proper download handling
   - Test with various file types
   - Files: `src/BrayaWindow.cpp`, `src/BrayaDownloads.cpp`

2. **Complete Password Manager Auto-Fill** (1-2 hours)
   - Inject password-detect.js into pages
   - Set up WebKit UserContentManager
   - Add message handler for password capture
   - Show "Save password?" prompt dialog
   - Auto-fill forms with saved credentials
   - Files: `src/BrayaTab.cpp`, `src/BrayaPasswordManager.cpp`

## 🎨 HIGH PRIORITY - Polish
3. **Settings Screen Redesign** (2-3 hours)
   - Complete Vivaldi-style sidebar layout
   - Add extensive color customization options
   - Add layout controls (spacing, sizes, positions)
   - Live preview of changes
   - Better organization and search
   - File: `src/BrayaSettings.cpp`

4. **Home Page Auto-Load Fix** (15-30 min)
   - Debug why about:braya doesn't load on first startup
   - Ensure it loads on new tabs and new windows
   - File: `src/BrayaTab.cpp`, `src/BrayaWindow.cpp`

## 📦 PACKAGING - Flatpak
5. **Create Flatpak Package** (1-2 hours)
   - Create flatpak manifest (org.braya.Browser.yml)
   - Set up build dependencies
   - Test installation and running
   - Publish to Flathub (optional)
   - New files: `org.braya.Browser.yml`, `com.braya.Browser.metainfo.xml`

## 🔧 NICE TO HAVE
6. **Tab Manager Improvements**
   - Better visual feedback
   - Drag-drop reordering
   - Close buttons that work properly
   - File: `src/BrayaWindow.cpp`

7. **Built-in Ad Blocker** (2-3 hours)
   - Implement filter list support (EasyList, EasyPrivacy)
   - Block ads and trackers
   - Settings toggle
   - New files: `src/BrayaAdBlocker.h`, `src/BrayaAdBlocker.cpp`

8. **Icon Polish**
   - Replace dog emoji with proper icon
   - Test icon appears in GNOME launcher, Alt+Tab, etc
   - Generate all required sizes
   - Files: `resources/icons/`, `src/BrayaWindow.cpp`

## 📋 DOCUMENTATION
9. **Update Documentation**
   - README.md with installation instructions
   - Building guide
   - Feature list
   - Screenshots
   - Contributing guidelines

## 🚀 VERSION 2.0 PLANNING
- **Extension API Support** - Major feature, 4-6 weeks
  - WebExtension API implementation
  - Firefox Add-on Store compatibility
  - Bitwarden integration
  - uBlock Origin support
- **Sync Features** - Firefox Sync protocol
- **Developer Tools** - Enhanced debugging
- **Mobile Version** - Touch-optimized UI

---

## Quick Start Next Session
```bash
cd /home/cobrien/Projects/braya-browser-cpp
./build.sh
./build/braya-browser
```

## Files to Focus On
1. `src/BrayaPasswordManager.cpp` - Complete auto-fill
2. `src/BrayaSettings.cpp` - Redesign UI
3. `src/BrayaDownloads.cpp` - Fix crashes
4. Create: `org.braya.Browser.yml` - Flatpak manifest

## Testing Checklist for Next Session
- [ ] Downloads work without crashing
- [ ] Passwords auto-save on login
- [ ] Passwords auto-fill on return visit
- [ ] Settings dialog looks professional
- [ ] Home page loads automatically
- [ ] Flatpak builds and installs
- [ ] All features stable for 10+ minutes of use

---
**Estimated Time to Complete v1.0.1:** 6-8 hours
**Estimated Time for Flatpak:** 1-2 hours  
**Total Next Session:** ~8-10 hours of focused work
