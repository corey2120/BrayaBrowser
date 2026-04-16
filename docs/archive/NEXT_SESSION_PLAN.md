# Braya Browser - Next Session Plan
**Current Version:** v1.0.1-beta2
**Target Version:** v1.1.0 or v1.0.2
**Priority:** High-impact UI/UX improvements and feature additions

---

## ✅ PASSWORD MANAGER STATUS (Beta 3)

### All Features Working:
- ✅ **Auto-save/auto-fill** - Working perfectly
- ✅ **Manual CRUD operations** - Working perfectly
- ✅ **CSV import** - Working with success/error dialogs
- ✅ **CSV export** - Working with success dialog showing file path
- ✅ **Bitwarden sync** - Working with proper error handling
- ✅ **Bitwarden import** - Working with UI feedback
- ✅ **Bitwarden export** - Working with instructions

### All Import/Export Fixed (Beta 2 & 3):
**Beta 2 Fixes:**
1. ✅ Improved Bitwarden import logic
2. ✅ Added jq dependency check
3. ✅ Better session handling
4. ✅ Creates cache directory automatically
5. ✅ Checks login status before import
6. ✅ User-friendly dialogs for Bitwarden operations

**Beta 3 Fixes:**
1. ✅ CSV import shows success/error dialogs
2. ✅ CSV export shows success dialog with file path
3. ✅ All import/export methods have proper UI feedback
4. ✅ Users get clear confirmation of all operations

### Password Manager: ✅ FULLY FUNCTIONAL

---

## 🔥 CRITICAL - Do First (Session Start)

### 1. Icon & Branding Overhaul (1-2 hours)
**Priority:** CRITICAL - First impression matters!

#### Issues to Fix:
- ❌ Current icon is low quality/placeholder
- ❌ Shows "com.braya.browser" in task switcher
- ❌ No proper icon shown when app is running
- ❌ Desktop integration looks unprofessional

#### Tasks:
1. **Create Professional Icon** (45 min)
   - Design 512x512 PNG icon in modern browser style
   - Use blue/teal color scheme (matching accent)
   - Include "B" lettermark or wave/globe symbol
   - Export multiple sizes: 16, 24, 32, 48, 64, 128, 256, 512
   - Consider using Inkscape for vector → multiple sizes

2. **Fix Desktop Integration** (30 min)
   - Update `braya-browser.desktop` file
   - Change `Name=` from showing app ID to "Braya Browser"
   - Set proper `Icon=` reference
   - Fix StartupWMClass to match application ID
   - Test in GNOME Shell / KDE Plasma
   - Files: `braya-browser.desktop`, `src/main.cpp`

3. **Update Application Window** (15 min)
   - Set proper window title
   - Set window icon programmatically
   - Fix GTK application ID display
   - Files: `src/BrayaWindow.cpp`, `src/main.cpp`

4. **Install Icon Properly** (15 min)
   - Install all icon sizes to correct paths
   - Update CMakeLists.txt for icon installation
   - Update RPM spec for icon files
   - Run `gtk-update-icon-cache` after install
   - Files: `CMakeLists.txt`, `braya-browser.spec`

**Expected Outcome:** Professional icon appears in launcher, taskbar, Alt+Tab, and window title

---

## 🎨 HIGH PRIORITY - Visual Polish

### 2. Website Favicons in Tabs (2-3 hours)
**Priority:** HIGH - Essential browser feature

#### Current Issue:
- Tabs show first letter of title or bullet point
- No website favicon displayed
- Looks unprofessional compared to Firefox/Zen

#### Implementation Plan:
1. **Improve Favicon Loading** (1 hour)
   - Current code tries to load favicons but has issues
   - Debug `onFaviconChanged()` callback in `BrayaTab.cpp:132`
   - Ensure favicon texture is valid before display
   - Add fallback to favicon.ico fetching if WebKit fails
   - Files: `src/BrayaTab.cpp:132-162`, `src/BrayaTab.cpp:164-227`

2. **Favicon Caching** (30 min)
   - Cache favicons to disk (avoid re-downloading)
   - Use `~/.cache/braya-browser/favicons/`
   - Key by domain hash
   - Files: New `src/BrayaFaviconCache.h`, `src/BrayaFaviconCache.cpp`

3. **Loading States** (30 min)
   - Show spinner while loading
   - Show default globe icon for no favicon
   - Show letter fallback only if no favicon available
   - Smooth transitions between states

4. **Testing** (30 min)
   - Test on major sites (Google, GitHub, Reddit, etc.)
   - Test on sites without favicons
   - Test favicon updates on navigation
   - Ensure no memory leaks with favicon textures

**Expected Outcome:** Tabs display website favicons like Firefox/Zen

---

### 3. Toolbar Icons Redesign (1-2 hours)
**Priority:** HIGH - Current icons are placeholder quality

#### Issues:
- 🔑 Password manager icon (just emoji)
- 📥 Downloads icon (just emoji)
- 📚 Bookmarks icon (just emoji)
- ⚙️ Settings icon (just emoji)
- All look unprofessional

#### Tasks:
1. **Design Icon Set** (1 hour)
   - Create SVG icons for:
     - Password manager (key or shield)
     - Downloads (arrow down to tray)
     - Bookmarks (star or bookmark ribbon)
     - Settings (gear)
     - History (clock with arrow)
   - Match Braya color scheme
   - 24x24px base size
   - Export to PNG with @2x variants

2. **Implement Icon System** (30 min)
   - Replace emoji text with proper icons
   - Use GtkImage with icon files
   - Add hover states
   - Add tooltips with keyboard shortcuts
   - Files: `src/BrayaWindow.cpp:300-340`

3. **Update Resources** (15 min)
   - Add icons to `resources/icons/toolbar/`
   - Update CMakeLists.txt to install icons
   - Update RPM spec
   - Files: `CMakeLists.txt`, `braya-browser.spec`

**Expected Outcome:** Professional-looking toolbar matching modern browser standards

---

## ⚙️ SETTINGS OVERHAUL - Vivaldi-Style

### 4. Settings UI Complete Redesign (3-4 hours)
**Priority:** HIGH - Current settings are basic

#### Current Issues:
- Basic tabbed interface
- Limited customization options
- Not Vivaldi-level customization
- No search functionality
- No live preview

#### Target Design (Vivaldi-inspired):
```
┌─────────────────────────────────────────────────┐
│  ⚙️  Settings                            [X]     │
├──────────┬──────────────────────────────────────┤
│          │                                       │
│ 🎨 Appearance                                   │
│          │  ┌─────────────────────────────┐    │
│ 🌐 General│  │ Theme                       │    │
│          │  │ ○ Dark  ○ Light  ○ Industrial│    │
│ 🔒 Privacy│  └─────────────────────────────┘    │
│          │                                       │
│ 🔐 Security ┌─────────────────────────────┐    │
│          │  │ Colors                       │    │
│ 🔑 Passwords│ Accent Color: [████] #00d9ff │    │
│          │  │ Background:   [████] #0a0f14 │    │
│ 📥 Downloads│ Sidebar:      [████] #0f1419 │    │
│          │  └─────────────────────────────┘    │
│ ⚡ Advanced│                                     │
│          │  ┌─────────────────────────────┐    │
│ 🔧 About │  │ Typography                   │    │
│          │  │ Font Family: [Sans        ▾] │    │
│          │  │ Font Size:   [14          ▾] │    │
│          │  │ Line Height: [1.5         ▾] │    │
│          │  └─────────────────────────────┘    │
│          │                                       │
│          │  [📋 Export Settings] [📥 Import]   │
│          │                                       │
└──────────┴──────────────────────────────────────┘
```

#### Implementation Tasks:

**Phase 1: Layout Redesign** (1.5 hours)
1. Create sidebar navigation (GtkListBox)
2. Create content area with GtkStack
3. Add search bar at top
4. Add settings import/export buttons
5. Files: `src/BrayaSettings.cpp:28-122`

**Phase 2: Appearance Section** (1 hour)
1. Expand color options (20+ colors)
   - Primary color
   - Secondary color
   - Accent color
   - Background colors (multiple)
   - Text colors (multiple)
   - Border colors
   - Hover states
2. Add color pickers for each
3. Add typography controls
   - Font family dropdown
   - Font size slider
   - Font weight
   - Line height
   - Letter spacing
4. Add layout controls
   - Sidebar width
   - Tab bar height
   - Padding values
   - Border radius
   - Spacing values
5. Live preview window
6. Reset to defaults button

**Phase 3: Advanced Options** (30 min)
1. Add "Show Advanced" toggle
2. CSS custom styles editor
3. User CSS injection
4. Theme import/export
5. JSON-based theme format

**Phase 4: Organization** (30 min)
1. Group related settings
2. Add section headers
3. Add descriptions/help text
4. Add keyboard shortcuts display
5. Add about/credits page

**Expected Outcome:** Settings UI rivals Vivaldi's customization depth

---

## 🔌 MAJOR FEATURES - Next Big Steps

### 5. Password Manager UI Polish (1-2 hours)
**Priority:** HIGH - Works but looks rough

#### Issues:
- Dialog layout could be better
- No password strength indicator
- No password generator
- No show/hide password toggle
- No search/filter in password list
- Basic visual design

#### Improvements:
1. **Password Entry Improvements**
   - Add show/hide password button (eye icon)
   - Add password strength meter
   - Add password generator button
   - Add copy username/password buttons
   - Monospace font for password field

2. **Password List Improvements**
   - Add search/filter box
   - Group by domain
   - Show password age
   - Show "last used" date
   - Add bulk selection

3. **Visual Polish**
   - Better spacing and padding
   - Proper headers
   - Icons for each entry (favicon)
   - Color coding for password strength
   - Animations for add/delete

4. **Security Features**
   - Master password option
   - Auto-lock after inactivity
   - Password strength analysis
   - Breach detection (haveibeenpwned.com)

**Files:** `src/BrayaPasswordManager.cpp:193-850`

---

### 6. Plugin/Extension Support (4-6 hours)
**Priority:** MEDIUM - Major feature, significant effort

#### Options:
**Option A: Chrome Web Store (WebExtensions API)**
- Most extensions available
- Well-documented API
- Used by Brave, Edge, Opera

**Option B: Firefox Add-ons**
- Good selection
- Better privacy focus
- More Linux-friendly

**Recommendation:** Start with Chrome Web Store (WebExtensions API)

#### Implementation Plan:

**Phase 1: WebExtensions API Core** (2 hours)
1. Research WebKit GTK extension APIs
2. Implement basic extension loading
3. Create extension manifest parser (JSON)
4. Set up extension sandbox environment
5. Files: New `src/BrayaExtensions.h`, `src/BrayaExtensions.cpp`

**Phase 2: API Implementation** (2-3 hours)
1. Implement core WebExtensions APIs:
   - `chrome.tabs` - Tab management
   - `chrome.windows` - Window management
   - `chrome.storage` - Extension storage
   - `chrome.runtime` - Extension lifecycle
   - `chrome.webRequest` - Request interception (for ad blockers)
   - `chrome.contextMenus` - Context menu items
   - `chrome.bookmarks` - Bookmark access
   - `chrome.history` - History access

2. JavaScript bridge
3. Content script injection
4. Background page support

**Phase 3: Extension UI** (1 hour)
1. Extension manager page (`about:extensions`)
2. Extension icon in toolbar
3. Extension popups
4. Options pages
5. Permission prompts

**Phase 4: Extension Store** (30 min)
1. Add "Install Extension" option
2. Parse Chrome Web Store URLs
3. Download and install .crx files
4. Auto-update mechanism

**Expected Outcome:** Install uBlock Origin, Bitwarden extension, etc.

---

### 7. Enhanced Customization System (2-3 hours)
**Priority:** MEDIUM - "Way more customization"

#### Current State:
- 60+ options available
- Basic theme system
- Some CSS customization

#### Target Additions:

**UI Layout Options:**
1. Tab bar position (top, bottom, left, right)
2. Sidebar position (left, right, auto-hide)
3. Bookmarks bar (always, never, new tab only)
4. Status bar toggle
5. Compact/comfortable/spacious modes
6. Window controls position

**Tab Customization:**
1. Tab width (fixed, flexible, shrink)
2. Tab shape (rounded, square, angled)
3. Close button position
4. Show/hide favicons
5. Show/hide close buttons
6. Tab animations
7. Tab previews on hover

**Color Schemes:**
1. Full color customization (40+ colors)
2. Import/export themes (JSON)
3. Vivaldi theme compatibility
4. Dark mode scheduling (time-based)
5. Per-website themes

**Advanced:**
1. Custom CSS editor
2. JavaScript snippets (power users)
3. Custom keyboard shortcuts
4. Gesture controls
5. Mouse button bindings

**Files:** `src/BrayaSettings.cpp`, `src/BrayaCustomization.cpp`

---

## 📋 ADDITIONAL IMPROVEMENTS

### 8. UI Polish - Quick Wins (1-2 hours)

#### Tab Bar:
- [ ] Improve tab rendering performance
- [ ] Add tab drag-and-drop reordering
- [ ] Fix tab close button positioning
- [ ] Add tab pinning
- [ ] Add tab audio indicators

#### Navigation Bar:
- [ ] Better back/forward button states
- [ ] Add stop button
- [ ] Improve URL bar suggestions
- [ ] Add security indicator (HTTPS lock)
- [ ] Add page loading progress in URL bar

#### General UI:
- [ ] Smooth animations throughout
- [ ] Better loading indicators
- [ ] Improved context menus
- [ ] Better error pages
- [ ] Tooltips everywhere

### 9. Performance Improvements (1 hour)
- [ ] Optimize theme switching
- [ ] Reduce memory usage
- [ ] Faster tab switching
- [ ] Lazy load non-visible tabs
- [ ] Cache compiled CSS

### 10. Documentation (30 min)
- [ ] User guide
- [ ] Keyboard shortcuts reference
- [ ] Extension development guide
- [ ] Theme creation guide
- [ ] FAQ

---

## 🎯 RECOMMENDED SESSION PLAN

### Session Flow (8-10 hours total):

**Hour 1-2: Icons & Branding**
- Fix app icon and desktop integration
- Create professional icon set
- Test task switcher appearance

**Hour 3-4: Favicons in Tabs**
- Debug and fix favicon loading
- Implement favicon caching
- Test on multiple websites

**Hour 5-6: Settings UI Overhaul (Part 1)**
- Redesign settings layout
- Implement sidebar navigation
- Start appearance section

**Hour 7-8: Settings UI Overhaul (Part 2)**
- Complete appearance section
- Add advanced options
- Add search functionality

**Hour 9: Password Manager Polish**
- Add show/hide password
- Add password generator
- Improve visual design

**Hour 10: Toolbar Icons + Testing**
- Replace emoji with proper icons
- Full testing pass
- Bug fixes

---

## 📊 SUCCESS METRICS

### Must Have (v1.1.0):
- ✅ Professional icon in all contexts
- ✅ Website favicons in tabs
- ✅ Vivaldi-style settings UI
- ✅ Professional toolbar icons
- ✅ Polished password manager UI

### Nice to Have:
- ✅ Extension support (basic)
- ✅ Enhanced customization
- ✅ Tab improvements

### Future (v1.2.0+):
- Extension store integration
- Full WebExtensions API compatibility
- Sync across devices
- Mobile version

---

## 🛠️ TECHNICAL DEBT

### To Address:
1. Replace XOR password encryption with libsodium
2. Migrate to system keyring (Secret Service API)
3. Improve error handling throughout
4. Add comprehensive logging
5. Write unit tests for core features
6. Memory leak detection
7. Profile and optimize rendering

---

## 📝 NOTES FOR NEXT SESSION

### Things to Remember:
1. **Icon Tools:** Use Inkscape or Figma for icon creation
2. **Favicon Debug:** Check WebKit favicon signals, may need workaround
3. **Settings UI:** Study Vivaldi's settings for inspiration
4. **Extensions:** WebKit2GTK has extension support via WebKitWebExtension
5. **Testing:** Test on fresh user account to verify defaults

### User Feedback Priority:
1. ⭐⭐⭐⭐⭐ Icon and branding
2. ⭐⭐⭐⭐⭐ Favicons in tabs
3. ⭐⭐⭐⭐⭐ Settings overhaul
4. ⭐⭐⭐⭐ Toolbar icons
5. ⭐⭐⭐⭐ Password manager UI
6. ⭐⭐⭐ Extension support
7. ⭐⭐⭐ More customization

---

## 🎨 DESIGN RESOURCES

### Icon Inspiration:
- Firefox (globe with fox)
- Brave (lion head)
- Vivaldi (red V)
- Chrome (colorful circle)
- Edge (blue wave)

### Color Scheme:
- Primary: `#00d9ff` (cyan/teal)
- Secondary: `#0a0f14` (dark blue)
- Accent: `#00d9ff` (matches primary)
- Success: `#4ade80` (green)
- Warning: `#fbbf24` (yellow)
- Error: `#ef4444` (red)

### Typography:
- UI Font: System default (usually Cantarell on GNOME)
- Monospace: Monospace system default
- Web Content: System sans-serif

---

## 🔄 MIGRATION PATH

### From v1.0.1-beta1 to v1.1.0:
1. Settings format may change (auto-migrate)
2. Icon paths will change (handled by package)
3. Extension directory structure (new)
4. Theme format may change (backward compatible)

### Upgrade Testing:
- Test upgrade from 1.0.0 → 1.1.0
- Test upgrade from 1.0.1-beta1 → 1.1.0
- Verify user data preserved
- Verify settings migration

---

**End of Plan**
**Estimated Time for Full Implementation:** 15-20 hours
**Realistic Target:** 2-3 focused sessions
**Next RPM Version:** 1.1.0-1 or 1.0.2-1 (depending on scope)
