# Braya Browser - Version 1.0.1 Plan

**Release Date Target:** Mid-November 2024  
**Focus:** Polish, Bug Fixes, and Icon Improvements

---

## 🎯 Primary Goals

1. **Fix App Icon Display** - Make Braya Browser look professional in GNOME app store/launcher
2. **Fix Downloads** - Ensure downloads work reliably without crashes
3. **Fix Home Screen Auto-load** - about:braya should load on startup automatically
4. **Tab Manager Improvements** - Tab system needs significant work and polish
5. **Settings Screen Redesign** - Current settings UI is terrible and needs complete overhaul
6. **Minor UI/UX Improvements** - Small tweaks for better user experience

---

## 📋 Detailed Task List

### A. App Icon Fixes (HIGH PRIORITY)
**Problem:** When installed via RPM, the app looks terrible in GNOME app launcher

**Tasks:**
- [x] Create high-quality icon (added brayacpp PNG files)
- [ ] **Fix corner logo - replace dog emoji with proper icon (needs work on cropping/sizing)**
- [ ] Test icon appears in multiple sizes (16x16, 32x32, 48x48, 128x128, 256x256)
- [ ] Ensure icon shows in:
  - [ ] GNOME app launcher
  - [ ] Window title bar
  - [ ] Alt+Tab switcher
  - [ ] Dock/taskbar
- [ ] Update .desktop file with correct icon path
- [x] Update RPM spec to properly install icon to hicolor theme
- [ ] Add PNG fallback icons for non-SVG support
- [ ] Set window icon programmatically in code

**Files to Modify:**
- `resources/icons/braya-browser.svg` (create/improve)
- `resources/icons/` (add PNG variants)
- `braya-browser.spec` (icon installation) ✅
- `src/main.cpp` (set window icon programmatically)
- `src/BrayaWindow.cpp` (corner logo - needs proper cropping solution)

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

### D. Tab Manager Improvements (HIGH PRIORITY)
**Problem:** Tab system needs significant work and better functionality

**Current Issues:**
- Tab switching is clunky
- Tab close buttons not intuitive
- No visual feedback on active tab
- Tab reordering doesn't work or is missing
- Favicon display inconsistent
- Tab overflow handling poor

**Tasks:**
- [ ] Improve tab visual design (active/inactive states)
- [ ] Fix tab close button behavior and position
- [ ] Add drag-and-drop tab reordering
- [ ] Improve favicon loading and display
- [ ] Add tab overflow handling (scrolling/dropdown)
- [ ] Better tab animations and transitions
- [ ] Add tab preview on hover
- [ ] Middle-click to close tab (if not working)
- [ ] Tab pinning functionality
- [ ] Audio indicator for tabs playing sound
- [ ] Better new tab button placement

**Files to Modify:**
- `src/BrayaTab.cpp` (tab behavior)
- `src/BrayaTab.h` (tab interface)
- `src/BrayaWindow.cpp` (tab management)
- `resources/style.css` (tab styling)

---

### E. Settings Screen Redesign (HIGH PRIORITY)
**Problem:** Current settings UI is terrible and needs complete overhaul

**Current Issues:**
- Layout is confusing
- Hard to find settings
- Poor organization
- Ugly design
- Settings don't apply properly
- No visual hierarchy

**Tasks:**
- [ ] Complete redesign of settings dialog layout
- [ ] Organize settings into logical groups
- [ ] Add search/filter for settings
- [ ] Improve visual design and spacing
- [ ] Add icons for setting categories
- [ ] Better form controls (switches, dropdowns)
- [ ] Live preview of theme changes
- [ ] Apply settings immediately (no restart required)
- [ ] Add reset to defaults button
- [ ] Better descriptions for each setting
- [ ] Keyboard navigation support
- [ ] Save/cancel confirmation dialogs

**Files to Modify:**
- `src/BrayaSettings.cpp` (complete rewrite/refactor)
- `src/BrayaSettings.h` (settings interface)
- `resources/style.css` (settings styling)

---

### F. Minor UI/UX Improvements (LOW PRIORITY)

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

## 🎯 Priority Order for v1.0.1

1. **App Icon** - First impressions matter (HIGHEST)
2. **Downloads** - Critical functionality (HIGHEST)
3. **Tab Manager** - Core user experience (HIGH)
4. **Settings Screen** - Must be usable (HIGH)
5. **Home screen auto-load** - Quality of life (MEDIUM)
6. **Minor improvements** - Nice to have (LOW)

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
- [ ] **Tab manager works smoothly (switching, closing, reordering)**
- [ ] **Settings screen is usable and applies changes correctly**
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
3. ✅ Tab manager is smooth and intuitive (primary goal)
4. ✅ Settings screen is well-designed and functional (primary goal)
5. ✅ Home screen auto-loads on startup (secondary goal)
6. ✅ No critical bugs or crashes in basic usage
7. ✅ RPM installs cleanly on Fedora/RHEL
8. ✅ All v1.0.0 features still work

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

- **Day 1:** App icon implementation and testing ✅
- **Day 2-3:** Tab manager improvements and polish
- **Day 4-5:** Settings screen complete redesign
- **Day 6-7:** Download manager fixes and testing
- **Day 8:** Home screen auto-load fix
- **Day 9:** Minor improvements and code cleanup
- **Day 10:** Testing, documentation, and release preparation
- **Day 11:** Build RPM, tag release, publish

**Total:** ~2 weeks of focused development

---

**Priority Order:**
1. App Icon ✅ (makes or breaks first impression)
2. Tab Manager (core user experience)
3. Settings Screen (must be usable)
4. Downloads (critical functionality)
5. Home screen auto-load (quality of life)
6. Minor improvements (nice to have)

Let's ship a polished v1.0.1! 🚀
