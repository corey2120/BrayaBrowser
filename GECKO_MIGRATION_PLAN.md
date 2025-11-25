# Braya Browser - Gecko Migration Plan

**Date Started:** November 25, 2025
**Branch:** gecko-migration
**Target:** v2.0 "Gecko Edition"

---

## Why We're Migrating

### WebKit Limitations We've Hit:
1. **Performance** - Scrolling lag on Twitter/YouTube/heavy sites
2. **Extension Support** - Can't run real Chrome/Firefox extensions
3. **Video Playback** - Required workarounds (software rendering)
4. **Password Manager** - Limited autofill APIs
5. **Windows Port** - WebKitGTK is Linux-only
6. **Modern Web APIs** - WebKit always lags behind

### What Gecko Gives Us:
✅ Firefox-level performance (much better than WebKit)
✅ Full Firefox extension support (millions available)
✅ Better video/media handling
✅ Superior autofill APIs
✅ Cross-platform potential (Windows, macOS)
✅ Modern web API support
✅ **Keep 90%+ of our GTK4 UI code**

---

## Gecko Embedding Options

### Option 1: libxul (Traditional) - **CHOSEN**

**What it is:**
- Traditional Firefox embedding API
- Ships with Firefox (`libxul.so`)
- Desktop-focused design
- Used by old Firefox extensions, Pale Moon, etc.

**Status:**
- Deprecated by Mozilla (2015) but still works
- Firefox still ships libxul
- Documentation old but complete
- Known to work on Linux

**Location on System:**
```
/usr/lib64/firefox/libxul.so
/usr/lib64/firefox/libmozgtk.so
/usr/lib64/firefox/libmozsandbox.so
```

**Why we chose this:**
- ✅ Proven to work on desktop
- ✅ Known path (examples exist)
- ✅ Desktop-first (not Android)
- ✅ Faster timeline (3-4 months)
- ⚠️ Risk: Mozilla could break it
- ⚠️ Old API, requires research

---

### Option 2: GeckoView (Modern)

**What it is:**
- New embedding API for Firefox Android
- Clean, modern design
- Official Mozilla project

**Status:**
- Android-only officially
- Desktop is "unsupported but possible"
- Would require porting work

**Why we didn't choose:**
- Experimental on desktop
- Longer timeline (6+ months)
- Pioneering territory

---

### Option 3: Firefox as Subprocess (Electron-style)

**What it is:**
- Run Firefox in background
- Communicate via IPC
- Wrapper approach

**Why we didn't choose:**
- Higher memory overhead
- Less integrated feel
- Not true embedding

---

## Current System Information

**Firefox Version:** 145.0.1
**Location:** /usr/lib64/firefox/
**libxul.so:** Present ✅
**Platform:** Fedora 43, x86_64

---

## Migration Architecture

### What Changes:

```
BrayaTab.cpp:
  OLD: WebKitWebView* webView = webkit_web_view_new();
  NEW: nsIWebBrowser* browser = CreateGeckoBrowser();

Files needing heavy changes:
- src/BrayaTab.cpp (WebView creation/management)
- src/BrayaTab.h (WebView type definitions)
- src/BrayaWindow.cpp (WebView callbacks)
- CMakeLists.txt (Link to libxul)
```

### What Stays the Same (90%):

✅ All GTK4 UI code
✅ BrayaWindow layout
✅ Sidebar, toolbar, navigation
✅ Bookmarks system
✅ History system
✅ Downloads manager
✅ Settings/customization
✅ Themes
✅ Tab groups
✅ Session management

**The UI stays 100% identical - just the rendering engine changes!**

---

## Phase 1: Research & Setup (Weeks 1-2)

### Week 1: Environment Setup

**Tasks:**
- [x] Install Firefox (already have 145.0.1)
- [x] Locate libxul.so (found at /usr/lib64/firefox/)
- [ ] Find libxul header files or documentation
- [ ] Research XPCOM (Gecko's component system)
- [ ] Study minimal embedding examples
- [ ] Set up development environment

**Resources to find:**
- libxul API documentation
- XPCOM interface documentation
- Embedding examples (old Mozilla docs, Pale Moon source)
- nsIWebBrowser interface reference

### Week 2: Minimal Prototype

**Goal:** Create "Hello World" with GTK4 + libxul

**Prototype requirements:**
- Creates GTK4 window
- Embeds Gecko browser widget
- Loads a URL (google.com)
- Basic navigation works

**Deliverable:** `prototype/gecko-hello.cpp`

---

## Phase 2: Core Migration (Weeks 3-8)

### Week 3-4: Basic WebView Replacement

**Replace WebKitWebView with nsIWebBrowser:**
- Create Gecko widget in BrayaTab
- URL loading/navigation
- Page load events
- Back/forward/reload
- Get current URL
- Set user agent

### Week 5-6: Tab Management

**Multi-tab support:**
- Create/destroy tabs dynamically
- Switch between tabs
- Tab lifecycle management
- Memory cleanup

### Week 7-8: Advanced Features

**Core browser functionality:**
- Downloads
- Context menus
- Find in page
- Zoom
- Page title updates
- Favicon loading

---

## Phase 3: Feature Integration (Weeks 9-12)

### Week 9: History & Bookmarks

**Integration:**
- History recording on page loads
- Bookmark navigation
- Favicon caching
- URL autocomplete

### Week 10: Password Manager

**Better autofill:**
- Use Gecko's password APIs
- Proper form detection
- Better autofill reliability
- Test on vwhub.com

### Week 11: Extensions

**Firefox extension support:**
- Load Firefox extensions
- Extension API compatibility
- Test popular extensions:
  - uBlock Origin
  - Dark Reader
  - Bitwarden

### Week 12: Session Management

**Save/restore:**
- Tab persistence
- Group restoration
- Window state
- Crash recovery

---

## Phase 4: Testing & Stabilization (Weeks 13-16)

### Week 13: Performance Testing

**Benchmarks:**
- Twitter scrolling (target: 60 FPS)
- YouTube playback (no lag)
- Gmail loading (fast)
- Discord messaging (smooth)
- 50+ tabs stress test

### Week 14: Bug Fixing

**Focus areas:**
- Memory leaks
- Crash recovery
- Edge cases
- UI glitches

### Week 15: Windows Port Investigation

**Research:**
- libxul on Windows
- GTK4 Windows compatibility
- What breaks, what works
- Feasibility assessment

### Week 16: Final Polish

**Pre-release:**
- Performance optimization
- Documentation
- Release notes
- Final testing

---

## Success Criteria

v2.0 will be considered successful if:

✅ Twitter scrolls at 60 FPS (vs current 15-20 FPS)
✅ YouTube videos play smoothly
✅ Can run Firefox extensions (uBlock Origin works)
✅ Password manager autofill works on vwhub.com
✅ Can handle 50+ tabs without lag
✅ No crashes in 8 hour session
✅ UI looks identical to v1.0.9
✅ Memory usage ≤ WebKit version
✅ Startup time ≤ 2 seconds

---

## Risk Mitigation

### Risk: libxul breaks in future Firefox update

**Mitigation:**
- Bundle specific Firefox version
- Pin to known-working libxul
- Community will help if it breaks (Pale Moon does this)

### Risk: Takes longer than 16 weeks

**Mitigation:**
- It's okay, no hard deadline
- Can release beta early
- Part-time work = flexible timeline

### Risk: Can't figure out libxul API

**Mitigation:**
- Can pivot to GeckoView
- Can pivot to CEF (Chromium)
- Can stay on WebKit (have v1.0.9)

### Risk: Burnout

**Mitigation:**
- Take breaks
- Celebrate small wins
- Ship incremental progress
- This is a marathon, not sprint

---

## Timeline Summary

**Conservative estimate (part-time):**
- Weeks 1-2: Research & prototype
- Weeks 3-8: Core migration (6 weeks)
- Weeks 9-12: Feature integration (4 weeks)
- Weeks 13-16: Testing & polish (4 weeks)

**Total: 16 weeks (4 months)**

**Optimistic estimate:** 12 weeks (3 months)
**Realistic estimate:** 16-20 weeks (4-5 months)

---

## Next Steps (This Week)

### Today (Nov 25):
- [x] Create this migration plan
- [ ] Research libxul documentation sources
- [ ] Find embedding examples online
- [ ] Study XPCOM basics

### This Week:
- [ ] Set up prototype directory
- [ ] Write minimal GTK4 + libxul test
- [ ] Get basic page loading working
- [ ] Document findings

### Next Week:
- [ ] Expand prototype with navigation
- [ ] Test multi-window/tab creation
- [ ] Assess feasibility
- [ ] Begin core migration if viable

---

## Resources to Research

### Documentation:
- [ ] MDN: XPCOM documentation
- [ ] Old Mozilla embedding guide (archive.org)
- [ ] Pale Moon source code (they use libxul)
- [ ] Old XULRunner examples

### Example Projects:
- [ ] Pale Moon browser source
- [ ] Old Mozilla embedding samples
- [ ] Gecko-based email clients

### Community:
- [ ] Mozilla dev mailing lists
- [ ] Pale Moon forums
- [ ] Reddit r/firefox developers

---

## Questions to Answer

1. **Is libxul stable enough for production?**
   - Test: Run prototype for days, check for crashes

2. **Can we link against system libxul or need to bundle?**
   - Test: Try both approaches

3. **What's the minimum Firefox version we need?**
   - Test: Try with different Firefox versions

4. **Does it work on Wayland?**
   - Test: Run prototype on Wayland session

5. **What's the memory overhead vs WebKit?**
   - Test: Compare with current browser

---

## Backup Plan

If libxul doesn't work out:

**Plan B:** Try CEF (Chromium Embedded Framework)
- Better documented
- Larger community
- Similar timeline
- Trade-off: Chrome not Firefox

**Plan C:** Improve WebKit version
- Release v1.1 with favicon fixes
- Accept limitations
- Niche browser strategy

---

## Communication

**Branch:** gecko-migration
**Tag:** v1.0.9-webkit (last WebKit release)
**Target:** v2.0-gecko (first Gecko release)

**Progress updates:** Commit frequently with descriptive messages

---

**Let's build the Firefox-based browser that Linux deserves! 🦊🐕**
