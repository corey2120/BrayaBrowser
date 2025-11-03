# Braya Browser TODO & Roadmap

**Last Updated**: November 3, 2024  
**Current Version**: 1.0.1-beta8

---

## 🎉 What's Complete (Beta8)

### Core Features ✅
- [x] WebKit rendering engine
- [x] Tab management with groups
- [x] Bookmarks system
- [x] History tracking
- [x] Download manager
- [x] Settings/customization (60+ options)
- [x] Three themes (Dark, Light, Industrial)

### Password Manager ✅
- [x] AES-256-CBC encryption
- [x] Auto-fill with Safari-style detection
- [x] Multi-account support
- [x] Chrome CSV import (fixed)
- [x] Bitwarden integration
- [x] Visual key icons in fields
- [x] Focus-triggered autofill

### Quick Wins ✅
- [x] Reader Mode (Alt+Shift+R)
- [x] Screenshot Tool (Ctrl+Shift+S)
- [x] Visual Bookmarks Bar with favicons
- [x] Speed Dial / New Tab Page
- [x] Favicon caching system
- [x] Tab pinning backend
- [x] Tab muting backend

### UI/UX ✅
- [x] Modern styling with animations
- [x] Keyboard shortcuts (7 total)
- [x] Hover effects
- [x] Visual polish

---

## 🚀 High Priority - Next Session (Beta9)

### Tab Features UI
- [ ] **Tab pinning visual indicators**
  - Pin icon on tab
  - Smaller pinned tabs
  - Keep left-aligned
  - Save pinned state
  
- [ ] **Tab mute button**
  - Speaker icon on tabs with audio
  - Click to mute/unmute
  - Visual indicator (🔇/🔊)
  - Right-click menu option

### Bookmark Improvements
- [ ] **Folder dropdowns**
  - Click folder to see submenu
  - Nested folder support
  - Organize by topic/category
  
- [ ] **Drag & drop reordering**
  - Reorder bookmarks in bar
  - Move between folders
  - Visual feedback during drag
  
- [ ] **Bookmark import**
  - Import from Chrome
  - Import from Firefox
  - Import from HTML file

### Reader Mode Polish
- [ ] **Dark mode option**
  - Toggle dark/light reader theme
  - Respect system theme
  - Save preference
  
- [ ] **Font size controls**
  - Increase/decrease text size
  - Different font options
  - Line height adjustment

---

## 🎯 Medium Priority - Beta10+

### Extensions/Add-ons Support (HUGE!)
- [ ] **Extension API**
  - WebExtension API compatibility
  - Chrome extension support
  - Basic manifest v3 support
  
- [ ] **Extension Manager**
  - Enable/disable extensions
  - Update management
  - Permissions system
  
- [ ] **Popular Extensions**
  - uBlock Origin
  - DarkReader
  - Privacy Badger

### Privacy Features
- [ ] **Built-in ad blocker**
  - Basic blocking rules
  - Toggle on/off
  - Whitelist sites
  
- [ ] **Tracker blocker**
  - Block tracking scripts
  - Privacy dashboard
  - Stats display
  
- [ ] **HTTPS-only mode**
  - Force HTTPS
  - Warning for HTTP sites
  - Auto-upgrade option

### Media Features
- [ ] **Picture-in-Picture**
  - Floating video window
  - Always on top
  - Resize/move controls
  
- [ ] **Media controls in toolbar**
  - Play/pause from any tab
  - Volume control
  - Album art display

### Page Tools
- [ ] **Page translation**
  - Detect language
  - Translate button
  - Google Translate integration
  
- [ ] **Find in all tabs**
  - Search across open tabs
  - Show results count
  - Jump between matches

---

## 💎 Polish & Nice-to-Have

### Bookmarks
- [ ] Bookmark sync (cloud)
- [ ] Most visited tracking
- [ ] Bookmark tags
- [ ] Search bookmarks
- [ ] Duplicate detection
- [ ] Broken link checker

### Screenshots
- [ ] Annotation tools
- [ ] Full page capture (scrolling)
- [ ] Selection capture
- [ ] Copy to clipboard
- [ ] Edit before saving

### Reader Mode
- [ ] Reading progress indicator
- [ ] Estimated reading time
- [ ] Save articles for later
- [ ] Export to PDF
- [ ] Text-to-speech

### Tab Management
- [ ] Tab groups with names
- [ ] Workspaces/sessions
- [ ] Tab search
- [ ] Recently closed tabs
- [ ] Tab sleeping (memory)

### Performance
- [ ] Lazy tab loading
- [ ] GPU acceleration settings
- [ ] Memory usage optimization
- [ ] Startup performance
- [ ] Cache management

---

## 🔮 Future Vision (v2.0+)

### Sync & Cloud
- [ ] Cross-device sync
- [ ] Account system
- [ ] Cloud bookmarks
- [ ] Cloud passwords
- [ ] Settings sync

### Mobile
- [ ] Android version
- [ ] iOS version (if possible)
- [ ] Mobile sync

### AI Features
- [ ] Page summarization
- [ ] Smart search
- [ ] Content recommendations
- [ ] Auto-categorization

### Developer Tools
- [ ] Enhanced devtools
- [ ] Network inspector
- [ ] Performance profiler
- [ ] Console improvements

### Social
- [ ] Share pages
- [ ] Collaborative browsing
- [ ] Notes/annotations
- [ ] Shared bookmarks

---

## 🐛 Known Issues to Fix

### Beta8 Issues
1. Tab pinning/muting have no UI yet (backend ready)
2. Speed dial only shows recent bookmarks (not most visited)
3. Reader mode may fail on some complex sites
4. Bookmark folders not visible in UI

### General Issues
1. Some GTK deprecation warnings
2. No custom master password UI
3. No system keychain integration yet
4. No extension support yet

---

## 📊 Progress Tracking

### Version 1.0 Goals
- [x] Core browser functionality (100%)
- [x] Password manager (100%)
- [x] Bookmarks system (90% - folders UI pending)
- [x] Visual polish (85%)
- [ ] Tab features (60% - UI pending)
- [ ] Privacy features (20%)
- [ ] Extensions (0%)

### Overall Completion
**Estimated: 75%** for a "complete" browser experience

**What's Working Great**:
- Core browsing ✅
- Password management ✅
- Visual bookmarks ✅
- Reader mode & screenshots ✅

**What Needs Work**:
- Tab UI polish ⚠️
- Extensions support ❌
- Privacy features ⚠️
- Sync/cloud ❌

---

## 🎯 Recommended Next Steps

### For Beta9 (Next Session):
1. Add tab pinning visual indicators
2. Add tab mute button UI
3. Implement bookmark folder dropdowns
4. Add drag & drop for bookmarks
5. Polish reader mode (dark theme)

**Estimated Time**: 3-4 hours

### For Beta10:
1. Start extensions infrastructure
2. Add basic ad blocker
3. Implement PiP video
4. Add page translation

**Estimated Time**: 6-8 hours

---

## 💡 Feature Ideas (Brainstorm)

### User-Requested
- Split-screen browsing
- Vertical tabs option
- Container tabs (Firefox-style)
- Custom search engines
- Download scheduler
- Session restore
- Auto-refresh pages
- Weather in toolbar
- Calculator in toolbar
- Notes sidebar

### Technical Improvements
- Faster startup time
- Better memory management
- WebRTC support enhancements
- Hardware acceleration
- Battery optimization
- Update mechanism

### Competitive Features
- Chrome extension compatibility
- Firefox container tabs
- Safari privacy report
- Vivaldi customization level
- Arc browser spaces
- Brave rewards (optional)

---

## 📝 Notes

### What Makes Braya Special
- **Beautiful UI**: Modern, polished design
- **Customization**: 60+ options (Vivaldi-level)
- **Security**: AES-256 password encryption
- **Privacy-focused**: No tracking, local-first
- **Fast**: WebKit engine performance
- **Native**: GTK4, feels like part of the system
- **Open Source**: Transparent development

### Target Audience
- Linux users who want Chrome features
- Privacy-conscious users
- Power users who want customization
- Developers who need devtools
- People tired of bloated browsers

### Success Metrics
- Daily active users
- Extension ecosystem growth
- Community contributions
- Performance benchmarks
- User satisfaction scores

---

**Current State**: Beta8 - Feature-complete core browser! 🎉  
**Next Milestone**: Beta9 - Tab UI polish + bookmarks  
**Big Goal**: Beta15 - Extension support (game changer!)

Last session accomplished: **9 major features in one day!** 🚀
