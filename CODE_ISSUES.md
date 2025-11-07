# Braya Browser - Code Issues & Findings

**Date**: November 4, 2025

## Compilation Issues

### 1. GTK4 API Deprecations (28 Total)

#### Critical Updates Needed
- [ ] `G_APPLICATION_FLAGS_NONE` → `G_APPLICATION_DEFAULT_FLAGS` (1 place)
  - File: `src/main.cpp:53`
  - Fix: Replace enum value

- [ ] `GtkComboBoxText` → `GtkDropDown` + `GtkStringList` (13 places)
  - Files: `src/BrayaSettings.cpp`
  - Lines: 170, 171-175, 226-230, 488, 498-502, 520, 636
  - Issue: Deprecated GTK3 API
  - Fix: Migrate to modern GTK4 dropdown model

- [ ] `GtkColorButton` → Modern color chooser (1 place)
  - File: `src/BrayaCustomization.cpp:447`
  - Issue: Deprecated in GTK4
  - Fix: Use `GtkColorDialog` instead

- [ ] `GtkFileChooserNative` → `GtkFileDialog` (2 places)
  - File: `src/BrayaPasswordManager.cpp:471, 516`
  - Issue: Deprecated in GTK4
  - Fix: Use modern file dialog API

### Priority
- **High**: These will break in future GTK4 versions
- **Timeline**: Should update within next 2-3 releases
- **Effort**: 2-3 hours total

---

## Runtime Issues

### 1. Git Status Conflicts
**Status**: Uncommitted changes in icon files

```
Modified:
- resources/icons/braya-browser-128.png
- resources/icons/braya-browser-16.png
- resources/icons/braya-browser-256.png
- resources/icons/braya-browser-32.png
- resources/icons/braya-browser-48.png
- resources/icons/braya-browser-512.png
- resources/icons/braya-browser-64.png

Deleted:
- resources/icons/braya-browser.svg
```

**Action Needed**:
1. Verify if SVG deletion was intentional
2. If yes: Commit all changes with proper message
3. If no: Restore SVG from git history

**Effort**: 5 minutes

---

### 2. TODO.md Conflicts with Actual Status

**Issues**:
- TODO.md says "Bookmarks bar completely broken" but BOOKMARKS_COMPLETE.md confirms it's working
- TODO.md shows "down from 75% due to broken bookmarks" but bookmarks are fixed
- Some features listed as not started are actually implemented

**Current Status** (vs TODO.md):
- ✅ Bookmarks bar (says broken, actually works)
- ✅ Tab pinning backend (says incomplete, actually done)
- ✅ Tab muting backend (says incomplete, actually done)
- ✅ Reader mode (says incomplete, actually works - just needs polish)

**Action Needed**: Update TODO.md to match current status

**Effort**: 30 minutes

---

## Code Quality Issues

### 1. BrayaWindow.cpp Too Large
**Size**: 1,681 lines
**Issue**: Single file has too many responsibilities

**Suggested Refactoring**:
- Extract bookmark bar logic to separate class
- Extract settings dialog to separate component
- Extract tab UI logic to separate module
- Extract keyboard shortcuts to separate manager

**Effort**: 4-6 hours
**Priority**: Medium (code maintenance)

---

### 2. Inconsistent Error Messages
**Pattern**: Many "ERROR: X" messages without context

**Examples**:
- `std::cerr << "ERROR: bookmarksBar is NULL!"` (BrayaWindow.cpp:483)
- `std::cerr << "ERROR: Could not find GtkBox widget!"` (BrayaWindow.cpp:516)
- `std::cerr << "ERROR: bookmarksManager is NULL!"` (BrayaWindow.cpp:524)

**Improvement**: Add line numbers, function names, and recovery suggestions

**Effort**: 1 hour (find and replace pattern)

---

### 3. Resource Path Management
**Issue**: Multiple methods to find resources

**Files affected**:
- `src/BrayaWindow.cpp`: `getResourcePath()`
- `src/BrayaTab.cpp`: `getResourcePath()`
- `src/BrayaCustomization.cpp`: Inline path logic
- `src/BrayaSettings.cpp`: Multiple file open methods

**Suggestion**: Create utility class `ResourceManager` with centralized logic

**Effort**: 2 hours
**Priority**: Low (code organization)

---

### 4. Password Master Key Security
**Current**: Auto-derived from username
**Issue**: Not user-controlled
**Code**: `BrayaPasswordManager.cpp:24-27`

```cpp
// Current (auto-generated)
std::string keyMaterial = std::string(g_get_user_name()) + "_braya_browser";
deriveKey(keyMaterial);
```

**Recommendation**: 
1. Add master password setup dialog on first run
2. Store hashed password, not plaintext
3. Support master password reset

**Effort**: 2-3 hours
**Priority**: High (security)

---

## Missing Features (Backend Ready)

### 1. Tab Pinning UI
**Backend Status**: Fully implemented in BrayaTab
**Missing**: Visual UI wiring

**Files**:
- `src/BrayaTab.h`: Lines 24-26 (isPinned, setPinned, pinned member)
- `src/BrayaTab.cpp`: Pinning state tracking exists

**What's needed**:
1. Add pin icon to tab button
2. Make pinned tabs smaller (e.g., 32px instead of 120px)
3. Keep pinned tabs on left side
4. Add visual indicator (small pin icon)
5. Wire up right-click menu to "Pin Tab" option
6. Save/load pinned state

**Effort**: 2-3 hours

---

### 2. Tab Muting UI
**Backend Status**: Fully implemented in BrayaTab
**Missing**: Visual UI wiring

**Files**:
- `src/BrayaTab.h`: Lines 27-28 (isMuted, setMuted, muted member)
- `src/BrayaTab.cpp`: Muting integration exists

**What's needed**:
1. Detect tabs with audio (WebKit API)
2. Add speaker icon to tab button (when audio playing)
3. Click to mute/unmute
4. Visual indicator for muted tabs (🔇 icon)
5. Add "Mute Tab" to right-click context menu
6. Save mute state

**Effort**: 2-3 hours

---

### 3. Bookmarks Folder Dropdowns
**Backend Status**: Folders exist in data structure
**Missing**: UI to display and interact with folders

**Current State**:
- Bookmarks have `folder` field
- Can edit folder via right-click → Edit
- Folders don't show in bar UI

**What's needed**:
1. Detect all folders in bookmarks
2. Create folder button for each folder (📁 Folder Name ▼)
3. Implement GtkPopover for dropdown
4. Show bookmarks in folder when clicked
5. Allow adding new folders from UI
6. Visual folder icon styling

**Effort**: 2-3 hours

---

## TODO Items from Comments

### BrayaBookmarks.cpp:484
```cpp
// TODO: Open bookmark in browser
```
**Status**: Already implemented (bookmarks bar navigation works)
**Action**: Remove outdated TODO comment

---

### BrayaPasswordManager.h:72
```cpp
// Enhanced encryption (TODO: migrate to libsodium for production)
```
**Status**: Current AES-256-CBC is production-ready
**Note**: libsodium would be an enhancement, not critical
**Priority**: Low

---

### BrayaHistory.cpp:249
```cpp
// TODO: Navigate to URL (need reference to window)
```
**Status**: History navigation implemented
**Action**: Remove outdated TODO comment

---

## Security Audit Notes

### Strong Points
- AES-256-CBC encryption (industry standard)
- OpenSSL used correctly
- Proper file permissions (0700 for config dir)
- WebKit2GTK sandbox isolation
- No obvious SQL injection (JSON file storage)

### Areas to Audit
1. **Password Storage**:
   - Verify master key derivation is secure
   - Check for password leaks in memory
   - Review encryption IV handling

2. **File Handling**:
   - Check path traversal vulnerabilities
   - Verify temporary file cleanup
   - Audit downloaded file handling

3. **JavaScript Execution**:
   - Verify password detection script is safe
   - Check WebKit context isolation
   - Review content injection points

### Recommendations
- [ ] Run code through security scanner (cppcheck, clang-analyzer)
- [ ] Conduct manual security code review
- [ ] Add unit tests for encryption
- [ ] Document security model clearly

---

## Memory Management Issues

### Potential Concerns
1. **GTK Widget Ownership**:
   - Some widgets use g_object_new but cleanup pattern unclear
   - Multiple window/dialog pointers stored
   - Review reference counting

2. **C++ Object Lifecycle**:
   - unique_ptr usage is good
   - Some raw pointers used (verify ownership)
   - Lambda captures in signal handlers

### Recommended Review
- Run with valgrind: `valgrind ./build/braya-browser`
- Check for leaks on application exit
- Profile memory with many tabs open

---

## Build System Issues

### CMakeLists.txt Improvements
1. **Version sync**: Version in CMakeLists.txt (1.0.0) vs actual (1.0.1-beta10)
2. **Resource paths**: Could use CMAKE variables instead of hardcoded paths
3. **Test setup**: No test target defined
4. **Install paths**: Could verify against standard Linux filesystem hierarchy

---

## Documentation Issues

### Files Needing Updates
1. **TODO.md**: Conflicts with actual implementation status
2. **README.md**: Version number discrepancies
3. **CMakeLists.txt**: Version number mismatch
4. **Code comments**: Several outdated TODO comments in source

---

## Summary of Issues by Severity

### Critical (Fix Before Release)
- [ ] Verify icon file status (SVG deleted?)
- [ ] Implement master password setup dialog

### High Priority (Next 2 Sessions)
- [ ] Update GTK4 deprecated APIs (28 warnings)
- [ ] Update TODO.md to match actual status
- [ ] Remove outdated TODO comments in code

### Medium Priority (Nice to Have)
- [ ] Refactor BrayaWindow.cpp into smaller components
- [ ] Add Tab Pinning UI (backend exists)
- [ ] Add Tab Muting UI (backend exists)
- [ ] Implement Bookmarks Folder dropdowns

### Low Priority (Future)
- [ ] Add automated tests
- [ ] Improve error messages
- [ ] Centralize resource management
- [ ] Performance optimization

---

## Testing Recommendations

### Manual Testing Checklist
- [ ] Open multiple tabs and verify tab operations
- [ ] Test all bookmarks operations (add, edit, delete)
- [ ] Test password autofill on websites
- [ ] Test reader mode on various websites
- [ ] Test screenshot tool
- [ ] Test all settings tabs
- [ ] Test theme switching
- [ ] Test keyboard shortcuts
- [ ] Test with many tabs (memory usage)
- [ ] Test history and search
- [ ] Test downloads
- [ ] Test on different screen sizes

### Automated Testing Needed
- [ ] Unit tests for password encryption
- [ ] Unit tests for bookmark operations
- [ ] Integration tests for tab management
- [ ] UI tests for settings dialog
- [ ] Security tests for password handling

---

**End of Code Issues Report**
