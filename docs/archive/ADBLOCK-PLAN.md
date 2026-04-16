# Braya Browser - Built-in Ad-Blocker Implementation Plan

## Overview
Native ad-blocker built directly into Braya Browser with full user customization and control.

## Goals
- **Performance**: Native C++ implementation, faster than extensions
- **Privacy**: No third-party dependencies, all blocking happens locally
- **Customization**: Full user control over blocking behavior
- **Simplicity**: Easy to use with sensible defaults

## Architecture

### Core Components

#### 1. BrayaAdBlocker (Core Engine)
**Location**: `src/adblocker/BrayaAdBlocker.{h,cpp}`

**Responsibilities**:
- Load and parse filter lists
- Compile rules into WebKit content blocking format
- Manage whitelist/blacklist
- Update filter lists
- Track statistics (blocked requests)

**Key Methods**:
```cpp
class BrayaAdBlocker {
    bool loadFilterList(const std::string& path);
    void compileRules();
    void enable();
    void disable();
    void setSecurityLevel(SecurityLevel level);
    bool shouldBlock(const std::string& url, const std::string& domain);
    void addToWhitelist(const std::string& domain);
    void removeFromWhitelist(const std::string& domain);
    int getBlockedCount() const;
};
```

#### 2. Filter List Parser
**Location**: `src/adblocker/FilterListParser.{h,cpp}`

**Responsibilities**:
- Parse EasyList format
- Support different rule types:
  - URL blocking rules (e.g., `||ads.example.com^`)
  - Element hiding rules (e.g., `##.advertisement`)
  - Exception rules (e.g., `@@||example.com^`)
  - Domain-specific rules (e.g., `example.com##.ad`)

**Formats Supported**:
- EasyList/Adblock Plus syntax
- uBlock Origin syntax
- Custom rules

#### 3. Settings Manager
**Location**: `src/adblocker/AdBlockSettings.{h,cpp}`

**Responsibilities**:
- Save/load settings to JSON
- Manage security levels
- Track enabled features
- Store whitelist

**Settings Structure**:
```json
{
  "enabled": true,
  "security_level": "standard",
  "features": {
    "block_ads": true,
    "block_trackers": true,
    "block_social": false,
    "block_cryptominers": true,
    "block_popups": true,
    "block_autoplay": true,
    "remove_cookie_warnings": false,
    "block_nsfw": false
  },
  "filter_lists": [
    {
      "name": "EasyList",
      "url": "https://easylist.to/easylist/easylist.txt",
      "enabled": true,
      "last_updated": "2025-11-13T10:00:00Z"
    },
    {
      "name": "EasyPrivacy",
      "url": "https://easylist.to/easylist/easyprivacy.txt",
      "enabled": true,
      "last_updated": "2025-11-13T10:00:00Z"
    }
  ],
  "whitelist": [
    "example.com",
    "trusted-site.org"
  ],
  "custom_rules": [
    "||custom-ad-server.com^"
  ],
  "stats": {
    "total_blocked": 12345,
    "blocked_today": 123
  }
}
```

#### 4. UI Components

**Settings Dialog**:
- Master enable/disable toggle
- Security level dropdown
- Feature checkboxes grid
- Filter list manager
- Whitelist editor
- Statistics display

**Toolbar Integration**:
- Shield icon showing blocked count
- Click to see site-specific stats
- Quick toggle for current site
- Access to settings

## Security Levels

### Off
- No blocking whatsoever
- All features disabled

### Minimal (Security Only)
- Block known malware domains
- Block phishing sites
- Block cryptominers
- Everything else allowed

### Standard (Recommended)
- Block ads
- Block trackers
- Block malware
- Block cryptominers
- Block pop-ups
- Allow social media widgets
- Allow annoyances

### Strict (Maximum Protection)
- Block everything suspicious
- Block ads
- Block all trackers
- Block social media widgets
- Block annoyances
- Remove cookie warnings
- Block autoplay videos
- Aggressive blocking

### Custom
- User selects individual features
- Can enable/disable each blocking type
- Can add custom rules
- Full control

## Filter Lists

### Default Lists (Auto-updated)
1. **EasyList** - General ad blocking
2. **EasyPrivacy** - Tracker blocking
3. **Malware Domains** - Security
4. **CoinBlocker** - Cryptominer blocking

### Optional Lists
- Fanboy's Annoyances
- Regional lists (by country)
- Social media blocking
- Custom user lists

### Update Schedule
- Check for updates daily
- Update in background
- Show last update time
- Manual update button

## Implementation Phases

### Phase 1: Core Engine (Week 1) ✅ COMPLETED
- [x] Create BrayaAdBlocker class
- [x] Basic URL blocking using WebKit API
- [x] Settings file structure
- [x] Load/save settings
- [x] Security level presets (OFF/MINIMAL/STANDARD/STRICT/CUSTOM)
- [x] Feature toggles (8 blocking features)
- [x] Statistics tracking structure

**Notes**: FilterListParser postponed - using basic pattern matching initially for faster deployment.

### Phase 2: WebKit Integration (Week 1-2) ✅ COMPLETED
- [x] Integrate with WebKitUserContentManager
- [x] Create WebKitUserContentFilter rules
- [x] Compile rules to JSON format
- [x] Apply to all tabs automatically
- [x] Test blocking functionality

**Files Created**:
- `src/adblocker/BrayaAdBlocker.h` (~150 lines)
- `src/adblocker/BrayaAdBlocker.cpp` (~600 lines)

### Phase 3: Settings System (Week 2) ✅ COMPLETED
- [x] Implement security levels
- [x] Whitelist management
- [x] Statistics tracking
- [x] Settings persistence (JSON)
- [x] Default configuration

**Settings Location**: `~/.config/braya-browser/adblock-settings.json`

### Phase 4: UI Implementation (Week 2-3) 🚧 IN PROGRESS
- [ ] Add settings dialog
- [ ] Create toolbar shield icon
- [ ] Site-specific toggle
- [ ] Statistics display
- [ ] Filter list manager UI

**Current Status**: Backend complete, ready for UI integration

### Phase 5: Advanced Features (Week 3-4)
- [ ] Custom rules editor
- [ ] Element hiding (cosmetic filtering)
- [ ] Import/export settings
- [ ] Advanced statistics
- [ ] Performance monitoring

### Phase 6: Testing & Polish (Week 4)
- [ ] Test on popular websites
- [ ] Performance benchmarks
- [ ] Fix edge cases
- [ ] User documentation
- [ ] Release

## WebKit Content Blocking API

### Using WebKitUserContentFilterStore

```cpp
// Create filter store
WebKitUserContentFilterStore* store = webkit_user_content_filter_store_new(path);

// Compile rules from JSON
const char* rules = "[{\"trigger\":{\"url-filter\":\".*\"},\"action\":{\"type\":\"block\"}}]";
webkit_user_content_filter_store_save(store, "adblock",
    g_bytes_new(rules, strlen(rules)), nullptr, callback, nullptr);

// Apply filter to content manager
WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager(webview);
webkit_user_content_manager_add_filter(manager, filter);
```

### Rule Format (JSON)
```json
[
  {
    "trigger": {
      "url-filter": ".*ads.*",
      "resource-type": ["image", "script"],
      "if-domain": ["*"]
    },
    "action": {
      "type": "block"
    }
  }
]
```

## Performance Considerations

### Optimization Strategies
1. **Rule Compilation**: Compile filter lists once, cache results
2. **Lazy Loading**: Only load active filter lists
3. **Incremental Updates**: Don't recompile entire list on changes
4. **Memory Efficient**: Store compiled rules, not raw text
5. **Background Processing**: Update lists without blocking UI

### Benchmarks to Track
- Time to compile filter lists
- Memory usage of compiled rules
- Request evaluation time
- Page load time impact
- CPU usage

## User Documentation

### Quick Start Guide
1. Enable ad-blocker in settings
2. Choose security level
3. Browse normally
4. Click shield icon to see stats

### Advanced Usage
- Adding custom rules
- Managing whitelists
- Importing filter lists
- Troubleshooting false positives

## Future Enhancements

### Possible Features
- [ ] Sync settings across devices
- [ ] Community-contributed filter lists
- [ ] Machine learning based blocking
- [ ] Advanced element picker tool
- [ ] Block tracking pixels/analytics
- [ ] Cookie consent auto-decline
- [ ] HTTPS upgrade enforcement
- [ ] Fingerprinting protection
- [ ] WebRTC leak prevention

## Testing Plan

### Test Cases
1. **Basic Blocking**
   - Test on ad-heavy sites (YouTube, news sites)
   - Verify ads are blocked
   - Ensure page functionality intact

2. **Whitelist**
   - Add site to whitelist
   - Verify ads appear
   - Remove from whitelist
   - Verify blocking resumes

3. **Security Levels**
   - Test each level
   - Verify different blocking behavior
   - Ensure custom level works

4. **Performance**
   - Measure page load times
   - Compare with/without ad-blocker
   - Benchmark against uBlock Origin

5. **Edge Cases**
   - Data URLs
   - Inline scripts
   - Dynamic content
   - Single-page apps

## Success Metrics

### Goals
- Block 95%+ of ads on popular sites
- Less than 50ms added page load time
- Less than 10MB memory overhead
- Zero false positives on popular sites
- User satisfaction score >4.5/5

## Resources

### Filter Lists
- EasyList: https://easylist.to/
- AdGuard Filters: https://adguard.com/kb/general/ad-filtering/create-own-filters/
- uBlock Origin Lists: https://github.com/uBlockOrigin/uAssets

### Documentation
- WebKit Content Blocking: https://webkit.org/blog/3476/content-blockers-first-look/
- Adblock Plus Syntax: https://help.adblockplus.org/hc/en-us/articles/360062733293

### Similar Projects
- Brave Browser's ad-blocker
- uBlock Origin
- AdGuard

---

## Progress Summary

**Created**: 2025-11-13
**Updated**: 2025-11-13
**Status**: Phase 4 - UI Implementation In Progress

### Completed Work
- ✅ Core ad-blocker engine implemented (~750 lines C++)
- ✅ WebKit content blocking integration complete
- ✅ Settings persistence with JSON
- ✅ 5 security level presets configured
- ✅ 8 feature toggles implemented
- ✅ Whitelist management system
- ✅ Statistics tracking structure
- ✅ Integrated into BrayaWindow initialization
- ✅ Applied to all browser tabs automatically
- ✅ Browser running successfully with ad-blocker enabled

### Current Work
- 🚧 Adding UI controls to settings dialog
- 🚧 Creating toolbar shield icon
- 🚧 Implementing statistics display

**Next Steps**:
1. Add ad-blocker section to BrayaSettings dialog
2. Create toolbar button to show blocking stats
3. Add per-site toggle functionality
4. Test on ad-heavy websites
