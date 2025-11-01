# Braya Browser - Version 1.0.1 Plan

**Release Date Target:** Mid-November 2024  
**Focus:** Polish, Bug Fixes, and Icon Improvements

---

## 🎯 Primary Goals

1. **Fix App Icon Display** - Make Braya Browser look professional in GNOME app store/launcher
2. **Fix Downloads** - Ensure downloads work reliably without crashes
3. **Fix Home Screen Auto-load** - about:braya should load on startup automatically
4. **Minor UI/UX Improvements** - Small tweaks for better user experience

---

## 📋 Detailed Task List

### A. App Icon Fixes (HIGH PRIORITY)
**Problem:** When installed via RPM, the app looks terrible in GNOME app launcher

**Tasks:**
- [ ] Create high-quality SVG icon (512x512 or scalable)
- [ ] Design proper Braya Browser logo/icon
- [ ] Test icon appears in multiple sizes (16x16, 32x32, 48x48, 128x128, 256x256)
- [ ] Ensure icon shows in:
  - [ ] GNOME app launcher
  - [ ] Window title bar
  - [ ] Alt+Tab switcher
  - [ ] Dock/taskbar
- [ ] Update .desktop file with correct icon path
- [ ] Update RPM spec to properly install icon to hicolor theme
- [ ] Add PNG fallback icons for non-SVG support

**Files to Modify:**
- `resources/icons/braya-browser.svg` (create/improve)
- `resources/icons/` (add PNG variants)
- `braya-browser.spec` (icon installation)
- `src/main.cpp` (set window icon programmatically)

---

### B. Download Manager Fixes (HIGH PRIORITY)
**Problem:** Downloads crash when attempting to download Chrome or other files

**Current Issues:**
- Download icon shows but downloads don't work
- Browser crashes on download attempt
- Signal 'download-started' is invalid warning at startup

**Tasks:**
- [ ] Fix WebKit download signal connections
- [ ] Implement proper download progress tracking
- [ ] Add download notification system
- [ ] Create download directory if doesn't exist (~/.config/braya-browser/downloads)
- [ ] Show download progress in UI
- [ ] Handle download completion/failure
- [ ] Add "Open folder" button after download completes
- [ ] Add download cancel functionality
- [ ] Test with various file types (PDF, images, executables, archives)

**Files to Modify:**
- `src/BrayaWindow.cpp` (download signal handling)
- `src/BrayaWindow.h` (download methods)

---

### C. Home Screen Auto-load Fix (MEDIUM PRIORITY)
**Problem:** about:braya doesn't load on first startup, only after clicking home button

**Tasks:**
- [ ] Debug why about:braya isn't loading on first tab creation
- [ ] Ensure home URL loads on:
  - [ ] Application first launch
  - [ ] New tab creation (Ctrl+T)
  - [ ] New window
- [ ] Add delay/callback if needed for WebView initialization
- [ ] Test with clean config (no previous settings)

**Files to Modify:**
- `src/BrayaWindow.cpp` (tab creation, window initialization)
- `src/main.cpp` (application startup)

---

### D. Minor UI/UX Improvements (LOW PRIORITY)

**Tasks:**
- [ ] Improve error page styling for about:braya
- [ ] Add keyboard shortcut help (Ctrl+?)
- [ ] Improve tab transition animations
- [ ] Add tooltip delays/improvements
- [ ] Fix any CSS inconsistencies in themes
- [ ] Improve settings panel responsiveness
- [ ] Add version number to about page
- [ ] Improve first-run experience message

**Files to Modify:**
- `src/BrayaWindow.cpp` (various UI tweaks)
- Resource files (CSS, HTML for about pages)

---

## 🔧 Technical Improvements

### Code Quality
- [ ] Add more error handling for edge cases
- [ ] Improve logging for debugging
- [ ] Clean up any compiler warnings
- [ ] Add comments to complex functions
- [ ] Remove unused debug code/logs

### Performance
- [ ] Profile startup time
- [ ] Optimize CSS loading
- [ ] Reduce memory footprint where possible

### Stability
- [ ] Add crash recovery for downloads
- [ ] Improve WebKit signal handling
- [ ] Add graceful degradation for missing features

---

## 📦 Build & Distribution

### RPM Package
- [ ] Update version to 1.0.1 in `braya-browser.spec`
- [ ] Update changelog in spec file
- [ ] Test RPM installation/uninstallation
- [ ] Test RPM upgrade from 1.0.0 → 1.0.1
- [ ] Verify all files installed correctly
- [ ] Test on clean Fedora/RHEL system

### Release Assets
- [ ] Create tarball with `build-release.sh`
- [ ] Generate SHA256 checksums
- [ ] Create release notes
- [ ] Update README.md with new features/fixes
- [ ] Tag release as v1.0.1 in git

---

## 🧪 Testing Checklist

### Before Release
- [ ] Fresh install from RPM on clean system
- [ ] Upgrade from v1.0.0 to v1.0.1
- [ ] Icon displays correctly in all contexts
- [ ] Downloads work for various file types
- [ ] Home screen loads on first startup
- [ ] All keyboard shortcuts work
- [ ] All themes work correctly
- [ ] Settings persist across restarts
- [ ] Tab groups function properly
- [ ] Bookmarks save/load correctly
- [ ] History tracks properly
- [ ] Find in page works
- [ ] All buttons (back/forward/home/reload) function
- [ ] New tab creation works
- [ ] Tab closing works
- [ ] Multiple windows work

---

## 📝 Documentation Updates

- [ ] Update README.md with v1.0.1 changes
- [ ] Add installation instructions for RPM
- [ ] Document keyboard shortcuts
- [ ] Add troubleshooting section
- [ ] Include screenshots with new icon
- [ ] Update feature list

---

## 🎯 Success Criteria

Version 1.0.1 is ready for release when:

1. ✅ App icon looks professional in GNOME (primary goal)
2. ✅ Downloads work without crashes (primary goal)
3. ✅ Home screen auto-loads on startup (primary goal)
4. ✅ No critical bugs or crashes in basic usage
5. ✅ RPM installs cleanly on Fedora/RHEL
6. ✅ All v1.0.0 features still work

---

## 🚀 Next Steps (v1.0.2+)

*Not for this release, but future consideration:*
- Session restore (reopen tabs on restart)
- Extension support
- Sync between devices
- Privacy/tracking protection
- Reader mode
- Developer tools integration
- Profile management
- Import bookmarks from other browsers

---

## 📅 Timeline Estimate

- **Day 1-2:** App icon design and implementation
- **Day 3-4:** Download manager fixes and testing
- **Day 5:** Home screen auto-load fix
- **Day 6:** Minor improvements and code cleanup
- **Day 7:** Testing, documentation, and release preparation
- **Day 8:** Build RPM, tag release, publish

**Total:** ~1 week of focused development

---

**Priority Order:**
1. App Icon (makes or breaks first impression)
2. Downloads (critical functionality)
3. Home screen auto-load (quality of life)
4. Minor improvements (nice to have)

Let's ship a polished v1.0.1! 🚀
