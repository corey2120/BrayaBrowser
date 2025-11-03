# Braya Browser Mobile Strategy - Roadmap to v1.5+

**Created**: November 3, 2024 - 2:35 AM  
**Vision**: Cross-platform browser with seamless sync  
**Timeline**: v1.5 (Android) → v2.0 (iOS)

---

## 🎯 The Vision

**Goal**: Make Braya a complete cross-platform ecosystem where everything syncs seamlessly between desktop and mobile.

**User Experience**:
- Install Braya on Linux desktop ✅
- Install Braya on Android phone 📱
- Sign in once, everything syncs 🔄
- Bookmarks, passwords, history, tabs - everywhere
- Continue reading on any device
- Optional iOS later 🍎

---

## 🗺️ Roadmap Overview

### Phase 1: Desktop Perfection (v1.0 - Current)
**Status**: 70% complete (Beta8)  
**Timeline**: Now - December 2024

Focus on making desktop version amazing:
- ✅ Core browser features
- ✅ Password manager (AES-256)
- ✅ Reader mode & screenshots
- 🔴 Fix bookmarks bar (urgent)
- ⚠️ Tab UI polish
- ⚠️ Extensions support (big!)

**Goal**: Rock-solid desktop browser before mobile

### Phase 2: Sync Infrastructure (v1.2-1.3)
**Timeline**: January - February 2025

Build the backend needed for sync:
- [ ] Cloud sync service (self-hosted option)
- [ ] Account system (optional, privacy-first)
- [ ] End-to-end encryption
- [ ] Sync protocol (bookmarks, passwords, settings)
- [ ] Conflict resolution
- [ ] Offline-first architecture

**Goal**: Desktop can sync with itself across machines

### Phase 3: Android Version (v1.5)
**Timeline**: March - June 2025

Build native Android browser:
- [ ] WebView-based Android app
- [ ] Native UI (Material Design)
- [ ] Sync integration
- [ ] Password autofill (Android)
- [ ] Reader mode mobile
- [ ] Tab sync
- [ ] Share to Braya

**Goal**: Full-featured Android browser with sync

### Phase 4: iOS Version (v2.0+)
**Timeline**: July 2025+

iOS browser (if possible):
- [ ] WKWebView-based iOS app
- [ ] Native UI (iOS HIG)
- [ ] Sync integration
- [ ] Safari extension (for password autofill)
- [ ] Reader mode iOS
- [ ] Tab sync

**Goal**: Complete the ecosystem

---

## 🤖 Android Version (v1.5) - Detailed Plan

### Technology Stack

**Option 1: Native Android (Kotlin)**
```
Pros:
✅ Native performance
✅ Full system integration
✅ WebView control
✅ Material Design native
✅ Play Store friendly

Cons:
❌ Learn Kotlin/Android
❌ Separate codebase
❌ More maintenance
```

**Option 2: React Native / Flutter**
```
Pros:
✅ Cross-platform code
✅ Faster development
✅ iOS version easier
✅ Modern frameworks

Cons:
❌ WebView limitations
❌ Less native feel
❌ Framework overhead
```

**Recommendation**: **Native Android (Kotlin)** for best experience

### Core Features (MVP)

**Browser Basics**:
- [ ] WebView rendering (Chromium)
- [ ] Tab management
- [ ] Bookmarks (synced)
- [ ] History (synced)
- [ ] Downloads
- [ ] Private browsing

**Sync Features**:
- [ ] Bookmarks sync ⭐ CRITICAL
- [ ] Password sync (encrypted)
- [ ] History sync
- [ ] Open tabs sync
- [ ] Settings sync
- [ ] Reader list sync

**Mobile-Specific**:
- [ ] Reader mode
- [ ] Data saver
- [ ] QR code scanner
- [ ] Share to apps
- [ ] Download manager
- [ ] Night mode
- [ ] Gesture navigation

**Android Integration**:
- [ ] Autofill service (passwords)
- [ ] Share handler
- [ ] Default browser
- [ ] Home screen widgets
- [ ] Quick settings tile
- [ ] Split screen support

### UI/UX Design

**Bottom Navigation** (thumb-friendly):
```
┌─────────────────────┐
│   Address Bar       │
├─────────────────────┤
│                     │
│   Content Area      │
│                     │
├─────────────────────┤
│ [◀] [▶] [⟲] [≡] [⋮]│
└─────────────────────┘
```

**Key Screens**:
1. **Main Browser** - Content + bottom bar
2. **Tab Switcher** - Card-based (Chrome style)
3. **Bookmarks** - Material Design list
4. **History** - Timeline view
5. **Settings** - Grouped preferences
6. **Sync Status** - Connection indicator

**Gestures**:
- Swipe left/right - Back/Forward
- Long press - Context menu
- Pull down - Refresh
- Swipe up from bottom - Show tabs
- Double tap - Zoom

### Sync Protocol Design

**Architecture**:
```
Desktop (Linux) ←→ Sync Server ←→ Mobile (Android)
      ↓                              ↓
  Local DB                      Local DB
  (SQLite)                      (Room/SQLite)
```

**Sync Strategy**:
- **Push**: Changes sent immediately
- **Pull**: Check for updates every 5 min (configurable)
- **Conflict**: Last-write-wins with timestamps
- **Encryption**: E2E encrypted before upload

**Sync Items**:
1. **Bookmarks** (priority 1)
   - URL, title, folder, timestamp
   - Favicon URL (download separately)
   - Tags, notes (optional)

2. **Passwords** (priority 2)
   - Already encrypted (AES-256)
   - Sync encrypted blob only
   - Never decrypt on server

3. **History** (priority 3)
   - URL, title, timestamp, duration
   - Privacy: Optional, can disable
   - Auto-clean after 90 days

4. **Open Tabs** (priority 4)
   - URL, title, scroll position
   - Send on close/minimize
   - "Continue on phone" feature

5. **Settings** (priority 5)
   - Theme, search engine, etc.
   - Per-device overrides
   - Sync preferences

**Data Format** (JSON):
```json
{
  "type": "bookmark",
  "id": "uuid-here",
  "data": {
    "url": "https://example.com",
    "title": "Example",
    "folder": "Work",
    "timestamp": 1699000000
  },
  "device_id": "desktop-linux",
  "encrypted": false
}
```

---

## 🍎 iOS Version (v2.0) - Considerations

### Challenges

**WebKit Limitation**:
- Must use WKWebView (Apple requirement)
- No other rendering engines allowed
- Limited to Safari's capabilities

**App Store Rules**:
- Can't duplicate Safari exactly
- Must offer "unique value"
- Browser extensions harder

**Solution**:
Focus on sync and privacy as differentiators:
- Cross-platform sync (vs Safari iCloud)
- Linux/Android integration
- Privacy-first (no tracking)
- Open source

### Possible Approach

**Option 1: Full Browser**
- Native iOS browser with WKWebView
- Limited by Apple's restrictions
- Hard to publish

**Option 2: Safari Extension**
- Password autofill extension
- Bookmark sync extension
- Easier approval
- Uses Safari rendering

**Option 3: Content Blocker + Extension**
- Privacy/ad blocking
- Bookmark manager
- "Companion app" to desktop

**Recommendation**: **Start with Safari Extension** (easier approval)

---

## 🔐 Sync Infrastructure Design

### Backend Options

**Option 1: Self-Hosted (Recommended)**
```
Tech: Node.js + PostgreSQL + Redis
Deployment: Docker container
Cost: Free (user hosts)
Privacy: Total control

Pros:
✅ User owns their data
✅ No server costs for you
✅ Maximum privacy
✅ Open source

Cons:
❌ Setup required
❌ Users need server/VPS
```

**Option 2: Managed Service**
```
Tech: Firebase / Supabase
Cost: Free tier + paid
Privacy: Good (encrypted)

Pros:
✅ Zero setup for users
✅ Reliable infrastructure
✅ Easy development

Cons:
❌ Monthly costs (scaling)
❌ Less control
```

**Option 3: Hybrid**
```
- Offer managed service (paid)
- Provide self-host option (free)
- Best of both worlds
```

**Recommendation**: **Hybrid approach**
- Free: Self-hosted (Docker)
- Paid: Managed ($3/month for convenience)
- Users choose

### Sync Server Features

**Core**:
- [ ] User authentication (JWT)
- [ ] Data storage (encrypted)
- [ ] REST API
- [ ] WebSocket (real-time)
- [ ] Conflict resolution
- [ ] Version control

**Privacy**:
- [ ] Zero-knowledge encryption
- [ ] Can't read user data
- [ ] Optional local-only mode
- [ ] GDPR compliant
- [ ] Data export

**Performance**:
- [ ] Redis caching
- [ ] Efficient delta sync
- [ ] Compression
- [ ] CDN for static assets

---

## 📱 Development Phases

### v1.2: Sync Prep (Desktop)
**Timeline**: January 2025 (4 weeks)

**Goals**:
- Add account system (optional)
- Implement sync protocol
- Test desktop-to-desktop sync
- Build simple sync server

**Deliverables**:
- Desktop can sync between machines
- Docker-compose for self-hosting
- Basic web dashboard
- Documentation

### v1.3: Sync Polish (Desktop)
**Timeline**: February 2025 (3 weeks)

**Goals**:
- Conflict resolution
- Sync status UI
- Background sync
- Error handling

**Deliverables**:
- Rock-solid sync
- "Continue on device" feature
- Sync health dashboard
- Troubleshooting tools

### v1.4: Mobile Prep
**Timeline**: March 2025 (2 weeks)

**Goals**:
- Mobile API design
- Mobile data structures
- Beta program setup

**Deliverables**:
- Mobile-optimized API
- Test infrastructure
- Beta signup page

### v1.5: Android Release
**Timeline**: April-June 2025 (12 weeks)

**Weeks 1-4**: Core Browser
- WebView integration
- Basic navigation
- Tab management
- Bookmarks (local)

**Weeks 5-8**: Sync Integration
- Connect to sync server
- Bookmark sync
- Password sync
- History sync

**Weeks 9-10**: Mobile Features
- Reader mode
- Downloads
- Share integration
- Gestures

**Weeks 11-12**: Polish & Testing
- Bug fixes
- Performance
- Beta testing
- Play Store prep

**Deliverables**:
- Android app on Play Store
- Full sync with desktop
- Native password autofill
- Reader mode

---

## 💰 Monetization Strategy

### Free Tier
- Desktop browser (always free)
- Self-hosted sync (always free)
- Android app (free)
- iOS app (free)
- Community support

### Paid Tier ($3-5/month)
- Managed sync service
- Priority support
- Advanced features:
  - Unlimited devices
  - History sync (90+ days)
  - Tab groups sync
  - Custom themes
  - Advanced privacy tools

### Pro Tier ($10/month)
- Everything in Paid
- Custom sync server URL
- White-label option
- API access
- Premium support

**Philosophy**: 
- Core always free
- Pay for convenience (managed hosting)
- Never paywall basic features
- Open source always available

---

## 🎯 Success Metrics

### v1.5 Goals (Android Launch)

**Downloads**:
- 10,000 installs (first month)
- 50,000 installs (6 months)
- 4.0+ rating on Play Store

**Sync Usage**:
- 30% of users enable sync
- 100,000 synced bookmarks/day
- 99.9% sync success rate

**Retention**:
- 50% day-7 retention
- 30% day-30 retention
- 20% become power users

**Revenue** (optional):
- 5% of users on paid plan
- $10,000 MRR by month 6
- Sustainable hosting costs

---

## 🚀 Quick Start Path

### If Starting Today:

**Month 1-2**: Desktop Perfection
- Fix bookmarks (urgent!)
- Add extensions support
- Polish UI/UX
- Reach v1.0 stable

**Month 3-4**: Sync Foundation
- Build sync server
- Add desktop sync
- Test thoroughly
- v1.2 release

**Month 5-6**: Mobile Planning
- Design Android UI
- Set up dev environment
- Create mockups
- v1.3 release

**Month 7-9**: Android Development
- Build core app
- Integrate sync
- Beta testing
- v1.5 alpha

**Month 10-12**: Android Launch
- Polish and optimize
- Play Store submission
- Marketing push
- v1.5 stable

**Year 2**: iOS + Growth
- Safari extension
- Potential iOS app
- Feature expansion
- v2.0

---

## 💡 Alternative: PWA Approach

### Progressive Web App
Instead of native apps, build a PWA:

**Pros**:
✅ One codebase for all platforms
✅ Faster development
✅ No app store approval
✅ Instant updates
✅ Works on desktop too

**Cons**:
❌ Limited system integration
❌ No full WebView control
❌ Less native feel
❌ Can't be default browser

**Verdict**: PWA is great for **bookmark manager** but not full browser.

**Hybrid Idea**:
- Native Android browser (v1.5)
- PWA bookmark/password manager (v1.3)
- PWA works everywhere (iOS, desktop)
- Native app for full experience

---

## 📋 Technology Choices

### Desktop (Current)
- **Language**: C++
- **UI**: GTK4
- **Engine**: WebKit
- **Platform**: Linux

### Android (v1.5)
- **Language**: Kotlin
- **UI**: Jetpack Compose
- **Engine**: Android WebView (Chromium)
- **Platform**: Android 8.0+ (API 26+)

### iOS (v2.0)
- **Language**: Swift
- **UI**: SwiftUI
- **Engine**: WKWebView
- **Platform**: iOS 15+

### Sync Server
- **Backend**: Node.js + Express
- **Database**: PostgreSQL
- **Cache**: Redis
- **Hosting**: Docker + VPS
- **Optional**: Supabase

### Shared
- **Sync Protocol**: JSON over HTTPS
- **Encryption**: AES-256-GCM + RSA
- **Auth**: JWT tokens
- **Real-time**: WebSocket

---

## 📝 Next Steps (Immediate)

### For Mobile Planning:

1. **Finish Desktop v1.0** (Next 2 months)
   - Fix bookmarks bar ⚠️
   - Add extensions
   - Reach feature-complete
   - Stable release

2. **Research & Design** (Next month)
   - Study Android development
   - Design mobile UI mockups
   - Plan sync architecture
   - Write technical specs

3. **Prototype Sync** (Next 2 months)
   - Build simple sync server
   - Test desktop-to-desktop
   - Validate encryption
   - Performance testing

4. **Start Android** (Month 4+)
   - Set up Android Studio
   - Create project structure
   - Build MVP browser
   - Test on real devices

---

## 🎊 The Dream (v2.0+)

Imagine:
- 📱 Open link on desktop → instantly on phone
- 🔖 Bookmark on phone → appears on desktop
- 🔐 Password saved once → works everywhere
- 📖 Reading article on desktop → continue on phone
- 🎨 Change theme → syncs to all devices
- 🗂️ Organize bookmarks → updates everywhere
- 🔒 All encrypted, all private, all yours

**That's the vision!** 🚀

---

**Status**: Roadmap created, ready to plan!  
**Next**: Focus on desktop v1.0, then sync infrastructure  
**Timeline**: Android by mid-2025, iOS later  
**Philosophy**: Privacy-first, user-owned data, open source

Mobile Braya is coming! 📱✨
