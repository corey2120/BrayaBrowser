# Braya Browser TODO

**Current Version:** 1.0.2
**Last Updated:** November 7, 2024

## High Priority

### UI/UX Improvements
- [x] Add tab groups visual UI (backend ready, UI complete!)
  - [x] Color-coded tab group indicators (4px colored border)
  - [x] Create new group dialog with 8 color presets
  - [x] Add tabs to groups via context menu
  - [ ] Drag-and-drop to organize tabs into groups
  - [ ] Group collapse/expand animations

- [x] Implement split view functionality ✨
  - [x] Side-by-side tab viewing (horizontal/vertical)
  - [x] Split toggle button in toolbar
  - [x] Keyboard shortcut (Ctrl+Shift+D)
  - [x] Resize handle between panes
  - [ ] Synchronized scrolling option (future enhancement)

- [x] Tab management enhancements
  - [x] Tab pinning UI (right-click menu + visual indicator)
  - [x] Tab muting UI (right-click menu + clickable audio icon)
  - [x] Tab preview on hover (300px preview popover) 👁️
  - [x] Recently closed tabs (Ctrl+Shift+T + context menu)

- [ ] Theme system enhancements
  - [ ] Add color picker UI for custom accent colors
  - [x] Add more theme presets (Gruvbox, Catppuccin, One Dark, Solarized, Monokai) 🎨
  - [ ] Theme export/import functionality
  - [ ] Custom CSS support (userChrome.css)

### Performance
- [ ] Optimize bookmarks bar loading for large collections
- [ ] Implement lazy loading for history entries
- [x] Cache favicon downloads to reduce network requests 💾
- [ ] Profile and optimize extension initialization

### Extensions System
- [ ] Complete browser.tabs API implementation
- [ ] Add browser.windows API support
- [ ] Implement content script injection for all sites
- [ ] Add extension permissions UI
- [ ] Create extension store/marketplace browser
- [ ] Test compatibility with popular extensions:
  - [ ] uBlock Origin (partially working)
  - [ ] Dark Reader
  - [ ] Grammarly

## Medium Priority

### Features
- [x] Session restore on crash/restart (auto-saves all tabs, groups, pins, mutes) 💾
- [ ] Private browsing mode
- [ ] Profiles support (multiple user profiles)
- [ ] Sync across devices (bookmarks, history, passwords)
- [ ] Advanced find in page (regex support, match case)
- [ ] Web developer tools integration
- [ ] Custom search engines management UI

### Settings & Customization
- [ ] Import settings from other browsers
- [ ] Export/backup all browser data
- [ ] Keyboard shortcuts customization UI
- [ ] Toolbar button visibility controls
- [ ] Sidebar customization (width, position, auto-hide)
- [ ] Font customization (UI font, size, monospace)
- [ ] Animation controls (speed, enable/disable)
- [ ] Per-site settings (zoom, permissions)

### Security & Privacy
- [ ] Cookie management UI
- [ ] Site permissions manager
- [ ] Clear browsing data dialog
- [ ] Do Not Track settings
- [ ] Enhanced tracking protection UI
- [ ] Certificate viewer

## Low Priority

### Polish
- [ ] Improve error messages and dialogs
- [ ] Add tooltips for all UI elements
- [ ] Loading animations and transitions
- [ ] Onboarding/welcome screen for first launch
- [ ] What's New dialog for updates
- [ ] In-app help documentation

### Nice to Have
- [ ] RSS feed reader integration
- [ ] Screenshot annotation tools
- [ ] PDF viewer with annotations
- [ ] Built-in note-taking
- [ ] Read-it-later integration
- [ ] Pocket/Instapaper sync
- [ ] QR code generator for current URL
- [ ] Page translation (via extension or built-in)

## Technical Debt
- [ ] Update deprecated GTK4 API calls
  - [ ] Replace GtkComboBoxText with GtkDropDown
  - [ ] Replace GtkMessageDialog with modern dialogs
  - [ ] Replace GtkFileChooserDialog with GtkFileDialog
  - [ ] Replace gtk_image_new_from_pixbuf with gtk_image_new_from_paintable
- [ ] Add comprehensive unit tests
- [ ] Add integration tests
- [ ] Improve error handling throughout codebase
- [ ] Add logging framework
- [ ] Document all classes and functions
- [ ] Refactor large functions into smaller units

## Documentation
- [x] Create README.md with build instructions
- [ ] User manual/wiki
- [ ] Developer documentation
- [ ] Extension development guide
- [ ] Contribution guidelines
- [ ] Code of conduct

## Known Issues
- [ ] Popup windows at vwhub.com may cause crashes (needs investigation)

## Recently Completed ✓
- [x] Tab Preview Fixes - Fixed GTK grabbing issues, auto-hides properly, no more lag 🐛
- [x] Split View Fixes - Properly moves tabs between stacks, requires 2+ tabs 🔀
- [x] Downloads Button Improvements - Icon instead of emoji, only visible during active downloads 📥
- [x] Favicon Caching - Domain-based cache to reduce network requests 💾
- [x] Tab Preview on Hover - 300px live preview popover when hovering tabs 👁️
- [x] 5 New Theme Presets - Gruvbox, Catppuccin, One Dark, Solarized, Monokai 🎨
- [x] Session restore - auto-saves/restores all tabs, groups, pins, mutes 💾
- [x] Split View functionality with side-by-side browsing (Ctrl+Shift+D) ✨
- [x] Tab groups visual UI with color-coded borders and creation dialog
- [x] Recently closed tabs with Ctrl+Shift+T shortcut and context menu
- [x] Tab pinning UI with visual indicators (📌) and context menu
- [x] Tab muting UI with clickable audio indicators (🔊/🔇)
- [x] Theme persistence across browser restarts (JSON save/load)
- [x] CSS variables system for dynamic theming
- [x] Theme preset system (Zen, Arc, Nord, Dracula, Tokyo Night)
- [x] Theme switching in Settings → Appearance
- [x] Compact Firefox-style UI refinements
- [x] URL bar extension and proper sizing
- [x] Bookmarks bar edge-to-edge display (removed gaps)
- [x] Navigation button spacing optimization
- [x] Version 1.0.2 release - UI refinements
- [x] Bookmarks bar Zen-style integration
- [x] Tab favicon display fix
- [x] Sidebar cleanup and polish
- [x] Password manager with encryption
- [x] Extension system foundation
- [x] Basic bookmarks management
- [x] History tracking
- [x] Download manager
- [x] Multiple themes (Dark, Light, Industrial)

---

## Next Session Focus

### Priority 1: Theme System Enhancement
- Add color picker widgets for theme customization
- Implement theme export/import (JSON format)
- Add more theme presets (Gruvbox, Catppuccin, One Dark, Solarized)
- Create real-time preview system

### Priority 2: Extension System
- Investigate and fix vwhub.com popup crashes
- Complete browser.tabs API implementation
- Test more extensions for compatibility
- Add extension permissions UI

### Priority 3: UI Features
- Implement tab pinning UI
- Add tab muting UI with sound indicators
- Create recently closed tabs menu
- Add tab preview on hover

---

**Notes:**
- Use Ctrl+D to bookmark current page
- Use Ctrl+B to toggle sidebar
- Extension installation: Download .xpi/.crx files to auto-install
- Theme presets available in Settings → Appearance
