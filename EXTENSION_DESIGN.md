# Braya Browser Extension System - Architecture Design

**Date**: 2025-01-04
**Status**: Design Phase
**Goal**: Enable browser extensions with a pragmatic, phased approach

---

## Executive Summary

Braya Browser will implement a **two-tier extension system**:
1. **Phase 1**: Native WebKit Extensions (C++ .so modules) - 2-3 weeks
2. **Phase 2**: WebExtensions API (Chrome/Firefox compatible) - 3-6 months

This approach provides immediate value (ad-blocking, custom features) while building toward full WebExtensions compatibility.

---

## Phase 1: Native WebKit Extensions (MVP)

### Architecture Overview

**Components**:
```
braya-browser (UI Process)
    ↓ spawns & manages
webkit-web-process (Renderer)
    ↓ loads at startup
braya-extension.so (Native Extension)
    ↓ provides
- Content script injection
- Request interception
- DOM manipulation
- JavaScript API exposure
```

### Key Files to Create

```
src/extensions/
├── BrayaExtensionManager.h         # Extension loader & lifecycle
├── BrayaExtensionManager.cpp
├── BrayaExtensionAPI.h             # JavaScript API bindings
├── BrayaExtensionAPI.cpp
├── web-extension/
│   ├── web-extension-main.cpp      # .so entry point
│   ├── ContentScriptInjector.cpp   # Inject JS into pages
│   └── RequestInterceptor.cpp      # Block/modify requests
└── builtin/
    ├── AdBlocker.cpp                # Built-in ad blocking
    └── DarkReader.cpp               # Built-in dark mode
```

### Implementation Details

#### 1. Extension Manager (UI Process)

**BrayaExtensionManager.h**:
```cpp
class BrayaExtensionManager {
public:
    void initialize(WebKitWebContext* context);
    void loadExtension(const std::string& path);
    void enableBuiltinExtension(const std::string& name);

private:
    WebKitWebContext* m_context;
    std::vector<std::string> m_loadedExtensions;
};
```

**Responsibilities**:
- Set WebKit extension directory
- Pass initialization data to web process
- Handle IPC from extensions (via D-Bus or custom protocol)

#### 2. Web Extension Module (.so)

**web-extension-main.cpp**:
```cpp
extern "C" {
    G_MODULE_EXPORT void webkit_web_extension_initialize(
        WebKitWebExtension* extension
    ) {
        // Called when web process starts
        g_signal_connect(extension, "page-created",
            G_CALLBACK(on_page_created), nullptr);
    }
}

static void on_page_created(
    WebKitWebExtension* extension,
    WebKitWebPage* page,
    gpointer user_data
) {
    // Inject content scripts
    // Set up request handlers
    // Expose JavaScript APIs
}
```

#### 3. Content Script Injection

```cpp
void injectContentScript(WebKitWebPage* page, const std::string& script) {
    WebKitFrame* frame = webkit_web_page_get_main_frame(page);
    JSCContext* js_context = webkit_frame_get_js_context(frame);

    jsc_context_evaluate(js_context, script.c_str(), -1);
}
```

#### 4. Request Interception (Ad Blocking)

```cpp
gboolean onSendRequest(
    WebKitWebPage* page,
    WebKitURIRequest* request,
    WebKitURIResponse* response,
    gpointer user_data
) {
    const char* uri = webkit_uri_request_get_uri(request);

    if (isBlockedUrl(uri)) {
        return TRUE;  // Block request
    }

    return FALSE;  // Allow request
}
```

### Built-in Extension: Ad Blocker

**Features**:
- Load EasyList filter rules (plaintext format)
- Pattern matching (wildcards, domains)
- ~10,000 rules = ~5MB memory
- 90% of ads blocked

**Filter Format**:
```
||doubleclick.net^
||googlesyndication.com^
##.advertisement
##div[id^="ad-"]
```

**Configuration UI**:
- Settings → Extensions → Ad Blocker
  - Enable/Disable toggle
  - Add custom filters
  - Whitelist domains
  - View blocked count

### IPC Between Processes

**Options**:
1. **D-Bus** (GNOME Web uses this)
   - System-wide message bus
   - Well-documented
   - Slight overhead

2. **WebKitUserMessage** (modern approach)
   - Direct IPC between UI ↔ Web process
   - Less boilerplate
   - Requires WebKit 2.28+

**Recommendation**: Use WebKitUserMessage

**Example**:
```cpp
// UI Process → Web Process
WebKitUserMessage* msg = webkit_user_message_new(
    "reload-filters",
    g_variant_new_string(filter_data)
);
webkit_web_view_send_message_to_page(webview, msg, nullptr, nullptr, nullptr);

// Web Process receives
g_signal_connect(page, "user-message-received",
    G_CALLBACK(on_message_received), nullptr);
```

### Timeline: Phase 1

| Week | Task |
|------|------|
| 1 | Create extension module skeleton, IPC setup |
| 2 | Implement content script injection, test with console.log |
| 3 | Add request interception, basic ad blocker (1000 rules) |
| 4 | Full EasyList integration, settings UI, testing |

**Deliverable**: Working ad blocker extension, ~90% coverage

---

## Phase 2: WebExtensions API (Full Compatibility)

### Architecture Overview

**Components**:
```
Manifest.json Parser
    ↓
Background Page Runtime (isolated context)
    ↓
Browser API Implementation:
- chrome.tabs
- chrome.bookmarks
- chrome.storage
- chrome.webRequest (limited)
- chrome.downloads
    ↓
Content Script Manager
    ↓
UI Integration (toolbar buttons, popups)
```

### Required Browser APIs

**Tier 1** (Essential, ~100 hours):
- `chrome.runtime` - Extension lifecycle, messaging
- `chrome.tabs` - Tab creation, navigation, queries
- `chrome.storage` - Persistent key-value storage
- `chrome.browserAction` - Toolbar buttons with popups
- `chrome.contextMenus` - Right-click menu items

**Tier 2** (Common, ~80 hours):
- `chrome.bookmarks` - Already have backend!
- `chrome.downloads` - Already have backend!
- `chrome.cookies` - WebKit cookie API
- `chrome.history` - Already have backend!
- `chrome.notifications` - System notifications

**Tier 3** (Advanced, ~120 hours):
- `chrome.webRequest` - **BLOCKED** (needs WebKit changes)
  - Workaround: Use web extension for basic blocking
- `chrome.webNavigation` - Navigation events
- `chrome.windows` - Multi-window support

### Manifest.json Format

**Support ManifestV2** (mature, widely used):
```json
{
  "manifest_version": 2,
  "name": "My Extension",
  "version": "1.0",
  "permissions": ["tabs", "storage", "https://*/*"],
  "background": {
    "scripts": ["background.js"],
    "persistent": false
  },
  "browser_action": {
    "default_icon": "icon.png",
    "default_popup": "popup.html"
  },
  "content_scripts": [{
    "matches": ["<all_urls>"],
    "js": ["content.js"]
  }]
}
```

### Background Page Runtime

**Challenges**:
1. Run JavaScript in isolated context (not in any tab)
2. Keep persistent state (even when pages close)
3. Handle async messaging from content scripts

**Solution**: Hidden WebKitWebView
```cpp
class BackgroundPageRunner {
    WebKitWebView* m_hiddenView;  // Never shown to user

    void load(const std::string& scriptPath) {
        // Load background.js into hidden view
        // Inject browser APIs
        // Keep alive
    }
};
```

### Extension Manager UI

**Location**: Settings → Extensions

**Features**:
- List installed extensions (name, icon, version)
- Enable/Disable toggles
- Remove button
- "Load unpacked" for development
- Permissions display
- Update check (future: extension store)

### Security Model

**Sandboxing**:
- Each extension gets isolated JavaScript context
- No direct file system access
- Network requests restricted to `permissions`
- Content scripts cannot access other extensions

**Permissions**:
- User approves on install
- Display requested permissions clearly
- Reject extensions requesting excessive permissions

### Extension Discovery

**Phase 2A**: Manual loading only
- Developer mode: Load unpacked extension folder
- .crx file support (Chrome extension package)

**Phase 2B**: Extension repository
- Central catalog (like Chrome Web Store)
- Curated list of verified extensions
- One-click install

### Timeline: Phase 2

| Month | Task |
|-------|------|
| 1 | Manifest parser, extension loader, basic runtime |
| 2 | `chrome.tabs`, `chrome.storage`, `chrome.runtime` APIs |
| 3 | Background page runner, content script injection |
| 4 | `chrome.browserAction`, toolbar UI, popups |
| 5 | Tier 2 APIs (bookmarks, downloads, history) |
| 6 | Extension manager UI, testing, documentation |

**Deliverable**: 80% Chrome extension compatibility

---

## Technical Decisions

### Why Not Use Existing Libraries?

**Option 1**: Fork GNOME Web's extension code
- **Pros**: Already works, well-tested
- **Cons**: GPL license (Braya is MIT), tightly coupled to GNOME

**Option 2**: Use libwebextensions (Nyxt)
- **Pros**: MIT license, interesting architecture
- **Cons**: Abandoned, uses Scheme runtime

**Option 3**: Build from scratch
- **Pros**: Full control, MIT license, learn deeply
- **Cons**: More work, reinventing wheel

**Decision**: Build from scratch, reference GNOME Web design

### JavaScript Engine

**Options**:
1. JavaScriptCore (JSC) - WebKit's built-in JS engine
2. V8 - Chrome's JS engine

**Decision**: Use JSC
- Already available (WebKitGTK dependency)
- Perfect integration with WebKit APIs
- Good performance (within 10% of V8)

### Storage Backend

**Options**:
1. SQLite database (chrome.storage implementation)
2. JSON files per extension
3. GSettings (GNOME)

**Decision**: SQLite
- Atomic transactions
- Easy querying
- Cross-platform

**Schema**:
```sql
CREATE TABLE extension_storage (
    extension_id TEXT NOT NULL,
    key TEXT NOT NULL,
    value TEXT,
    PRIMARY KEY (extension_id, key)
);
```

---

## Compatibility Matrix

### Target Extensions (Phase 2)

| Extension | Works? | Notes |
|-----------|--------|-------|
| uBlock Origin | ❌ | Needs `webRequest` API |
| Dark Reader | ✅ | Uses `tabs`, `storage` |
| LastPass | ⚠️ | Needs autofill API |
| Grammarly | ⚠️ | Heavy DOM manipulation |
| Honey | ✅ | Uses `tabs`, `cookies` |
| Pocket | ✅ | Uses `browserAction`, `tabs` |

**Success Metric**: 50% of top 100 Chrome extensions work

---

## Performance Considerations

### Memory Overhead

**Per Extension**:
- Manifest + JS code: ~1-5 MB
- Background page: ~10-30 MB (WebView)
- Content scripts: ~5-10 MB per tab

**10 extensions**: ~150-300 MB total

**Optimization**:
- Lazy load extensions
- Unload unused background pages after 5 minutes
- Share JSC contexts where safe

### Startup Time

**Goals**:
- No impact on cold start (<0.1s)
- Load extensions in background thread
- Defer non-essential extensions

---

## Testing Strategy

### Phase 1 Testing

1. **Unit Tests**: Content injection, request blocking
2. **Integration Tests**: Load test extension, verify behavior
3. **Real-World**: Block ads on top 100 sites

### Phase 2 Testing

1. **API Compliance**: Test each chrome.* API against spec
2. **Extension Tests**: Load real extensions, verify functionality
3. **Performance**: Memory usage, startup time benchmarks

**Test Extensions** (controlled):
- Simple popup extension
- Content script (modify page)
- Background script (persistent state)
- All APIs combined

---

## Documentation

### For Users

- `docs/EXTENSIONS_USER_GUIDE.md`
  - How to install extensions
  - Recommended extensions
  - Troubleshooting

### For Developers

- `docs/EXTENSIONS_DEV_GUIDE.md`
  - API reference
  - Example extensions
  - Debugging tips
  - Migration from Chrome

---

## Open Questions

1. **Extension Store**: Host our own or partner with existing?
2. **ManifestV3**: Support alongside V2, or V2 only initially?
3. **Extension Signing**: Require signatures? How to implement?
4. **Auto-updates**: Check for updates daily? Use what protocol?

---

## Success Criteria

**Phase 1 (Native Extensions)**:
- ✅ Ad blocker blocks 90% of ads on top 100 sites
- ✅ Content script injection works on all sites
- ✅ <50 MB memory overhead
- ✅ Settings UI for enable/disable

**Phase 2 (WebExtensions API)**:
- ✅ 10+ Chrome APIs implemented
- ✅ 5 popular extensions work without modification
- ✅ Extension manager UI complete
- ✅ Developer documentation published
- ✅ <200 MB memory overhead for 5 extensions

---

## Resources

- [WebKit2GTK Web Extensions](https://webkitgtk.org/reference/webkit2gtk/stable/ch02.html)
- [Chrome Extensions API](https://developer.chrome.com/docs/extensions/reference/)
- [MDN WebExtensions](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions)
- [GNOME Web Source](https://gitlab.gnome.org/GNOME/epiphany/-/tree/main/src/webextension)
- [Manifest V2 Spec](https://developer.chrome.com/docs/extensions/mv2/)

---

**Next Steps**: Review design → Approve → Begin Phase 1 implementation
