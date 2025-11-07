# Braya Browser TODO & Roadmap

**Last Updated**: November 6, 2025 - 7:00 AM
**Current Version**: 1.0.1-beta11 (in progress)
**Recent**: WebExtension support COMPLETE! uBlock Origin & other extensions working! 🎉

---

## ✅ COMPLETED - Beta 9, 10, & 11

### Bookmarks Bar - FIXED! ✅ (Beta 9)
**Status**: COMPLETE - All issues resolved!

**Fixed Issues**:
- [x] Bookmarks bar now visible and functional
- [x] Bookmarks navigate when clicked
- [x] Show/hide toggle working (Ctrl+Shift+B)
- [x] Can edit existing bookmarks
- [x] Can delete bookmarks from bar
- [x] Right-click context menu working
- [x] Bar renders properly without GTK errors

### Professional Icon - COMPLETE! ✨ (Beta 10)
**Status**: COMPLETE - Beautiful professional icon!

**Completed**:
- [x] Modern gradient design with brand colors
- [x] Detailed browser window representation
- [x] Traffic lights, tabs, and address bar
- [x] Multiple sizes (16px to 512px + SVG)
- [x] Proper freedesktop.org installation
- [x] Icon cache integration
- [x] Professional appearance in all contexts

### WebExtension Support - COMPLETE! 🎉 (Beta 11)
**Status**: FULLY FUNCTIONAL - Chrome/Firefox extensions working!

**Implemented**:
- [x] WebExtension API infrastructure
- [x] Chrome/Firefox Extension API compatibility
- [x] Extension installation from .zip files
- [x] Background pages with proper context isolation
- [x] Content script injection at document_start/idle
- [x] Extension toolbar buttons with popups
- [x] Persistent storage for installed extensions
- [x] Extension icons in toolbar
- [x] Browser API injection (tabs, storage, runtime, webRequest, etc.)
- [x] Message passing between content and background
- [x] Working extensions: uBlock Origin, Port Authority
- [x] Extension manifest parsing (v2 & v3)
- [x] Permissions system

---

## 🚨 HIGH PRIORITY - Next Session (UI Polish & Beta 12 Release)

### UI Polish & Fixes - PRIORITY FOR NEXT SESSION
- [ ] **Fix bookmarks bar size** - Too large, needs to be more compact
- [ ] **Fix double dog logo in upper left** - Duplicate icon issue
- [ ] **Review and fix other UI issues**:
  - Spacing/padding inconsistencies
  - Button sizes
  - Icon alignment
  - Theme consistency
- [ ] **Build new Beta 12 RPM** with all fixes and WebExtension support

### Tab Features UI
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

## 🚀 High Priority - After Bookmarks Fixed

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

## 🎯 Medium Priority - Beta12+

### Extensions/Add-ons Support - COMPLETE! ✅
- [x] **Extension API**
  - WebExtension API compatibility
  - Chrome extension support
  - Basic manifest v2 & v3 support

- [x] **Extension Manager**
  - Enable/disable extensions (via settings)
  - Extension installation from zip
  - Permissions system

- [x] **Popular Extensions Working**
  - uBlock Origin ✅
  - Port Authority ✅
  - Ready for more extensions

### Extension Enhancements (Future)
- [ ] Extension store/marketplace UI
- [ ] Auto-update for extensions
- [ ] Extension settings UI
- [ ] Permission prompts on install
- [ ] More extension APIs (notifications, downloads, etc.)

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

### Bookmarks (After Core Works!)
- [ ] Folder dropdowns
- [ ] Drag & drop reordering
- [ ] Bookmark import (Chrome/Firefox)
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

### CRITICAL - Beta8 Issues 🔴
1. **Bookmarks bar completely broken** (TOP PRIORITY!)
   - Not visible
   - Clicks don't work
   - No edit/delete
   - No show/hide toggle
   
### Other Beta8 Issues
2. Tab pinning/muting have no UI yet (backend ready)
3. Speed dial only shows recent bookmarks (not most visited)
4. Reader mode may fail on some complex sites
5. Bookmark folders not visible in UI

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
- [x] Reader mode & screenshots (100%)
- [x] **Bookmarks system (100%)**
- [x] **Extensions support (95%)**
- [x] Visual polish (85%)
- [ ] Tab features (60% - UI pending)
- [ ] Privacy features (25% - via uBlock)
- [ ] RPM packaging (needs update for Beta 12)

### Overall Completion
**Estimated: 85%** for a "complete" browser experience
*(Up from 70% due to extension support!)*

**What's Working Great**:
- Core browsing ✅
- Password management ✅
- Reader mode & screenshots ✅
- Bookmarks system ✅
- **WebExtension support ✅**
- Visual styling ✅

**What Needs Polish**:
- UI sizing/spacing ⚠️ (bookmarks bar too large, double logo)
- Tab UI features ⚠️

**What Needs Work**:
- Privacy features (enhanced by extensions now)
- Sync/cloud ❌
- RPM packaging update ⚠️

---

## 🎯 Recommended Next Steps

### For Beta12 (UI Polish - Next Session):
**PRIORITY 1: UI Fixes (1-2 hours)**
1. Fix bookmarks bar size (too large, make more compact)
2. Fix double dog logo in upper left corner
3. Review UI for other spacing/sizing issues
4. Test across different window sizes

**PRIORITY 2: Build Beta 12 RPM (1 hour)**
5. Update version to 1.0.1-beta12
6. Build new RPM with all fixes and WebExtension support
7. Test installation on clean system
8. Update release notes with extension support

**Estimated Time**: 2-3 hours total

### For Beta13 (Future):
1. Tab pinning visual indicators
2. Tab mute button UI
3. Extension store/marketplace UI
4. Reader mode dark theme
5. More extension API support

**Estimated Time**: 4-6 hours

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

**Current State**: Beta11 - WebExtension support COMPLETE! 🎉
**Next Milestone**: Beta12 - UI Polish & new RPM release
**Big Achievement**: Extension support working (uBlock Origin, Port Authority, etc.)!

Last session accomplished: **Full WebExtension API with working extensions!** 🚀

**NEXT SESSION: UI fixes (bookmarks bar size, double logo) and Beta 12 RPM!** 🎯
