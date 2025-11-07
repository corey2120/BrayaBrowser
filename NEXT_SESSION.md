# Next Session Plan - Beta 12: UI Polish & RPM Release

**Date**: November 6, 2025
**Target Version**: 1.0.1-beta12
**Focus**: UI fixes and new beta RPM with WebExtension support
**Estimated Time**: 2-3 hours

**Alternative Track**: Extension messaging system (see EXTENSION_MESSAGING_PLAN.md) - 7-10 hours

---

## 🎉 Previous Session Accomplishments

### WebExtension Support - COMPLETE!
We successfully implemented full WebExtension support in Beta 11:

**What Works**:
- ✅ Chrome/Firefox Extension API compatibility
- ✅ Extension installation from .zip files
- ✅ Background pages with proper context isolation
- ✅ Content script injection (document_start and document_idle)
- ✅ Extension toolbar buttons with popup UI
- ✅ Persistent storage for installed extensions
- ✅ Message passing between content and background scripts
- ✅ Working extensions: uBlock Origin, Port Authority

**Technical Implementation**:
- Browser API injection (tabs, storage, runtime, webRequest, webNavigation, etc.)
- Extension manifest parsing (v2 and v3)
- Permissions system
- Extension button UI with icons
- Popup WebView with extension context
- Content script matching and injection
- Extension configuration persistence

**Files Modified**:
- `src/braya_extensions.cpp` - Core extension system
- `src/braya_extension_api.h` - API definitions
- `webextension/braya-web-extension.c` - Web process extension
- Extension API JavaScript files

---

## 🎯 This Session Goals - Beta 12

### Priority 1: UI Fixes (1-2 hours)

#### Issue 1: Bookmarks Bar Too Large
**Problem**: Bookmarks bar takes up too much vertical space

**Files to Check**:
- `resources/theme-dark.css` (and theme-light.css, theme-industrial.css)
- `src/braya_bookmarks.cpp`

**Possible Fixes**:
- Reduce bookmark button height (currently likely 32-36px, target 24-28px)
- Reduce padding/margins on bookmark items
- Make favicon size smaller if needed
- Reduce bookmarks bar container padding

**CSS selectors to look for**:
```css
#bookmarksBar { }
.bookmark-button { }
.bookmark-item { }
```

#### Issue 2: Double Dog Logo in Upper Left
**Problem**: Logo appears twice in the window title area

**Files to Check**:
- `src/braya_window.cpp` - Window initialization
- `src/main.cpp` - Application setup
- GTK widget hierarchy

**Possible Causes**:
- GtkHeaderBar might have logo set
- Window icon being added twice
- Application icon overlapping with window decoration

**Areas to investigate**:
- `gtk_window_set_icon()`
- `gtk_window_set_default_icon()`
- Application icon vs window icon

#### Issue 3: General UI Review
**Areas to Check**:
- Button spacing consistency across toolbar
- Extension button sizes
- Tab bar height
- Address bar padding
- Settings dialog layout
- Download manager spacing

**Test Scenarios**:
- Small window (800x600)
- Large window (1920x1080)
- Different DPI settings
- All three themes (dark, light, industrial)

---

### Priority 2: Build Beta 12 RPM (1 hour)

#### Step 1: Update Version Numbers
**Files to Update**:
```
braya-browser.spec → Version: 1.0.1, Release: beta12
CMakeLists.txt → project version if present
src/main.cpp → version string if hardcoded
```

#### Step 2: Update Release Notes
**Create**: `RELEASE_NOTES_BETA12.md`

**Content to include**:
```markdown
# Braya Browser Beta 12 Release Notes

## Major Features
- **WebExtension Support** - Full Chrome/Firefox extension compatibility!
  - Install extensions from .zip files
  - Working extensions: uBlock Origin, Port Authority
  - Extension toolbar buttons with popups
  - Content script injection
  - Background pages
  - Message passing

## UI Improvements
- Fixed bookmarks bar size (more compact)
- Fixed double logo issue in window title
- Improved spacing and layout consistency

## Bug Fixes
- Extension API stability improvements
- Better content script isolation
- Fixed extension storage persistence

## Known Issues
- Some extensions may not work due to missing APIs
- Extension settings UI coming in Beta 13

## Installation
```

#### Step 3: Build RPM
**Commands**:
```bash
# Clean previous build
make clean
rm -rf build/

# Build project
./build.sh

# Create source tarball
cd ..
tar czf braya-browser-1.0.1-beta12.tar.gz braya-browser-cpp/ \
  --exclude=.git --exclude=build --exclude=*.o

# Build RPM
rpmbuild -ba braya-browser.spec

# Test installation
sudo dnf remove braya-browser
sudo dnf install ~/rpmbuild/RPMS/x86_64/braya-browser-1.0.1-beta12.*.rpm
```

#### Step 4: Test Installation
**Test Checklist**:
- [ ] RPM installs without errors
- [ ] Desktop file appears in applications menu
- [ ] Icon shows correctly
- [ ] Browser launches
- [ ] Extensions load on startup
- [ ] uBlock Origin works
- [ ] Settings persist
- [ ] Bookmarks load
- [ ] Passwords load

---

## 📋 Detailed Task Breakdown

### Task 1: Fix Bookmarks Bar Size (30-45 min)

1. **Identify current size**:
   ```bash
   # Check theme files for bookmarks styling
   grep -n "bookmark" resources/theme-*.css
   ```

2. **Measure and adjust**:
   - Current height: ~36px (estimate)
   - Target height: ~26px
   - Reduce padding from 8px to 4px
   - Reduce button height

3. **Test changes**:
   - Rebuild and launch browser
   - Check with 5+ bookmarks
   - Test hover effects still work
   - Verify icons align properly

4. **Apply to all themes**:
   - Dark theme
   - Light theme
   - Industrial theme

### Task 2: Fix Double Logo (20-30 min)

1. **Identify the duplicate**:
   ```bash
   # Search for icon setting code
   grep -rn "set_icon\|window_icon\|app_icon" src/
   ```

2. **Common causes to check**:
   - `gtk_window_set_icon()` called multiple times
   - `gtk_window_set_default_icon()` plus window-specific icon
   - GtkHeaderBar title widget has image

3. **Fix approach**:
   - Remove one of the duplicate icon settings
   - Or ensure only application icon is set, not per-window

4. **Test**:
   - Launch browser
   - Open multiple windows
   - Check window switcher
   - Check taskbar

### Task 3: UI Review & Polish (20-30 min)

1. **Check toolbar**:
   - All buttons same height?
   - Consistent spacing?
   - Extension buttons aligned?

2. **Check tabs**:
   - Tab height comfortable?
   - Close buttons accessible?
   - New tab button visible?

3. **Check address bar**:
   - Height matches toolbar?
   - Padding comfortable for typing?
   - Security icons visible?

4. **Check settings**:
   - Dialog not too wide/tall?
   - All options visible?
   - Scrollbars if needed?

### Task 4: Build & Test RPM (30-45 min)

1. **Update files**:
   - Version in .spec file
   - Release notes
   - Changelog entry

2. **Build process**:
   - Clean build
   - Create tarball
   - Build RPM
   - Check for errors

3. **Test installation**:
   - Remove old version
   - Install new version
   - Launch and verify all features
   - Check extension loading

4. **Document**:
   - Save RPM location
   - Note any issues
   - Update TODO.md

---

## 🔍 Known Areas to Investigate

### Bookmarks Bar CSS Structure
Likely structure:
```css
#bookmarksBar {
  /* Container styling */
  padding: 4px;  /* Reduce this */
}

.bookmark-button {
  min-height: 26px;  /* Reduce from ~36px */
  padding: 2px 8px;  /* Reduce vertical padding */
}

.bookmark-icon {
  width: 16px;
  height: 16px;
  margin-right: 4px;
}
```

### Window Icon Code Locations
Check in `src/braya_window.cpp`:
```cpp
// Look for duplicate icon setting
gtk_window_set_icon_name(GTK_WINDOW(window), "braya-browser");
// vs
gtk_window_set_default_icon_name("braya-browser");
// Only one should be needed
```

### Extension Button Sizing
If extension buttons look off:
```cpp
// In BrayaExtensions::updateExtensionButtons()
// Check button widget sizing
gtk_widget_set_size_request(button, 32, 32);  // Make consistent
```

---

## 🧪 Testing Checklist

### Before Making RPM
- [ ] All UI fixes applied
- [ ] Tested in all three themes
- [ ] No GTK warnings on console
- [ ] Extensions still load
- [ ] Bookmarks bar looks good
- [ ] No double logo

### After Building RPM
- [ ] RPM builds without errors
- [ ] File size reasonable (~10-20 MB)
- [ ] All dependencies listed
- [ ] Desktop file included
- [ ] Icons included

### After Installing RPM
- [ ] Application appears in menu
- [ ] Icon shows correctly
- [ ] Launches without errors
- [ ] Settings load
- [ ] Extensions load (uBlock, Port Authority)
- [ ] Bookmarks work
- [ ] Passwords work
- [ ] Can browse websites
- [ ] Can install new extension

---

## 📝 Files to Modify

### Likely Changes
1. `resources/theme-dark.css` - Bookmarks bar styling
2. `resources/theme-light.css` - Bookmarks bar styling
3. `resources/theme-industrial.css` - Bookmarks bar styling
4. `src/braya_window.cpp` - Fix double logo
5. `braya-browser.spec` - Version bump
6. `RELEASE_NOTES_BETA12.md` - New file

### Possible Changes
7. `src/braya_extensions.cpp` - Button sizing
8. `src/main.cpp` - Version string
9. `CMakeLists.txt` - Version if present

---

## 🎯 Success Criteria

### Must Have
- ✅ Bookmarks bar is noticeably more compact
- ✅ Only ONE dog logo in window
- ✅ Beta 12 RPM builds successfully
- ✅ RPM installs and browser launches
- ✅ Extensions still work

### Should Have
- ✅ All UI spacing looks consistent
- ✅ No visual regressions
- ✅ All themes updated
- ✅ Release notes complete

### Nice to Have
- ✅ Other minor UI improvements discovered
- ✅ Performance improvements if found
- ✅ Additional theme polish

---

## 📦 Deliverables

1. **Code Changes**:
   - Bookmarks bar CSS fixes
   - Double logo fix
   - Any other UI improvements

2. **RPM Package**:
   - `braya-browser-1.0.1-beta12.fc43.x86_64.rpm`
   - Located in `~/rpmbuild/RPMS/x86_64/`

3. **Documentation**:
   - `RELEASE_NOTES_BETA12.md`
   - Updated `TODO.md`
   - Updated `README.md` if needed

4. **Testing**:
   - Installation test report
   - Screenshot of fixed UI
   - Extension functionality verified

---

## 🚀 Next Steps (Beta 13+)

After Beta 12 is released:

1. **Tab UI Features**:
   - Tab pinning visual indicators
   - Tab mute button with icons
   - Improved tab overflow handling

2. **Extension Enhancements**:
   - Extension store/marketplace UI
   - Auto-update mechanism
   - Better extension settings UI
   - Permission prompts

3. **Reader Mode**:
   - Dark mode toggle
   - Font size controls
   - Typography options

4. **Privacy Features**:
   - Built-in tracker stats
   - Privacy dashboard
   - HTTPS-only mode option

---

## 💡 Tips & Reminders

- **Backup before major changes**: `git stash` or `git commit`
- **Test incrementally**: Don't make all changes at once
- **Check all themes**: Easy to forget light/industrial themes
- **Verify extensions**: Make sure fixes don't break extension system
- **Document changes**: Update comments and docs as you go
- **Clean builds**: When in doubt, `make clean && make`

---

## 📞 Help Resources

- **GTK4 Docs**: https://docs.gtk.org/gtk4/
- **WebExtension API**: https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions
- **RPM Packaging**: https://rpm-packaging-guide.github.io/
- **CSS Debugging**: Use GTK Inspector (`Ctrl+Shift+D`)

---

**Ready to build Beta 12!** 🚀

The extension support is complete and working great. Now we just need to polish the UI and ship this awesome release!

---

## Alternative: Extension Messaging System (Future Session)

If you prefer to work on making uBlock Origin fully functional instead of UI polish, see the comprehensive plan in **EXTENSION_MESSAGING_PLAN.md**.

**What it fixes**:
- CORS errors preventing popup from loading
- Real message passing between popup and background page
- Port connections for persistent communication
- Storage persistence across browser restarts

**Result**: uBlock Origin popup will load and fully function (toggle blocking, update filters, etc.)

**Time Required**: 7-10 hours (1-2 sessions)

**Priority Decision**:
- **Option A**: UI fixes + RPM (2-3 hours) - Ships Beta 12 quickly
- **Option B**: Extension messaging (7-10 hours) - Makes uBlock fully work but delays Beta 12

Choose based on what's more important: shipping a polished release or making extensions fully functional.
