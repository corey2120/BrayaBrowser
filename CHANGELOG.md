# Braya Browser Changelog

All notable changes to Braya Browser will be documented in this file.

## [1.0.8] - 2025-11-22

### Added
- Session persistence (cookies, localStorage, IndexedDB)
- Ad-blocker shield UI with toggle popover
- Bookmarks import/export (HTML & JSON formats)
- Settings button in ad-blocker shield popover

### Changed
- Ad-blocker shield now shows 🛡️ emoji (blue when on, grey when off)
- Cleaned up ad-blocker settings page (removed statistics UI)
- Removed hello-world test extension

### Fixed
- Navigation button crashes (back, forward, home)
- Settings crash when opening ad-blocker tab
- Multiple memory safety issues from v1.0.7

## [1.0.7] - 2025-11-22

### Added
- Per-tab UserContentManager (prevents SIGABRT crashes)
- Deferred tab setup for 40x faster tab creation
- WebKitUserContentFilterStore for ad-blocker
- Proper ad-blocker with specific domain blocking

### Fixed
- SIGABRT crash when opening 6-8+ tabs
- Memory leaks in popup windows
- Use-after-free in deferred setup
- Out-of-bounds array access in popup creation
- g_object_unref balance in password manager
- NULL pointer in onCreateNewWindow

### Performance
- Tab creation: 2000ms → 50ms (40x improvement)
- Memory leaks: 5+ → 0 (100% fixed)

## [1.0.6] - 2025-11-17

### Changed
- UI updates and various fixes
- Branding refinements

## [1.0.5] - Earlier

### Added
- Password manager with Safari-style UI
- Extension system
- Tab groups
- Bookmarks management
- Download management
- History tracking

---

For detailed release notes, see V1.0.8_RELEASE_NOTES.md
