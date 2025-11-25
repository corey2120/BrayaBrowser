# Braya Browser Crash Analysis & Fixes

## Executive Summary
**16 crashes in 5 days** (Nov 17-22, 2025) - All Signal 11 (SIGSEGV) or Signal 6 (SIGABRT)

## Root Cause: GTK Event Queue Race Condition

### The Problem

When closing a tab, the following sequence occurs:

```
1. User clicks close button
2. Lambda callback fires (BrayaWindow.cpp:1449)
3. Callback calls closeTab(index) (line 1480)
4. closeTab() disconnects ALL signals (line 1804)
5. closeTab() clears gesture controller data (line 1831-1832)
6. closeTab() removes tab from vector (line 1848)
7. closeTab() RETURNS to lambda callback
8. Lambda exits normally
9. ❌ CRASH: Other events ALREADY QUEUED fire with dangling pointers
```

### Why Crashes Happen

**GTK event processing is asynchronous.** Events queued BEFORE signal disconnection will STILL execute after tab destruction.

**Crash Vectors Identified:**

1. **Preview Timer Callback** (line 1588-1640)
   - If `g_timeout_add` timer fires AFTER tab button removed
   - Accesses `g_object_get_data()` on destroyed widget
   - **SIGSEGV when dereferencing nullptr**

2. **Hover Controller Leave** (line 1644-1660)
   - Mouse leave event queued before destruction
   - Tries to access `g_object_get_data(tabBtn, "close-button")`
   - **SIGSEGV on freed memory**

3. **Motion Controller Enter** (line 1567-1640)
   - Mouse enter event creates snapshot callback
   - Snapshot completes AFTER tab destroyed
   - Accesses popover on freed widget
   - **SIGSEGV**

4. **Close Button Gesture** (line 1449-1485)
   - Double-click protection exists (line 1461-1467)
   - BUT: If two separate tabs' close buttons clicked rapidly
   - Index iteration (line 1471-1476) may find wrong tab
   - **Out-of-bounds access**

### Signal Disconnection Timing Issue

```cpp
// BrayaWindow.cpp:1804 - Disconnects signals
g_signal_handlers_disconnect_matched(tabBtn, G_SIGNAL_MATCH_DATA, ...);

// BUT: Events already in GTK main loop STILL execute!
// Example event queue state:
// [mouse-enter] [click] [timer-fire] [mouse-leave]
//               ^
//               We disconnect here
//               But timer-fire and mouse-leave already queued!
```

## Fixes Required

### Fix 1: Add "Destroying" Flag (Immediate Mitigation)

Prevent callbacks from executing during/after destruction:

```cpp
// BrayaWindow.h - Add member:
std::set<GtkWidget*> destroyingTabs;

// BrayaWindow.cpp:1780 - In closeTab():
destroyingTabs.insert(tabBtn);

// BrayaWindow.cpp:1567 - In ALL callbacks:
auto enterCallback = +[](GtkEventControllerMotion* controller, double x, double y, gpointer data) {
    GtkWidget* tabBtn = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
    BrayaWindow* window = static_cast<BrayaWindow*>(data);

    // ✅ CHECK IF DESTROYING
    if (window->destroyingTabs.count(tabBtn) > 0) {
        return;  // Abort - tab being destroyed
    }

    // ... rest of callback
};
```

### Fix 2: Block Signals Instead of Disconnecting

```cpp
// Instead of:
g_signal_handlers_disconnect_matched(tabBtn, ...);

// Use:
g_signal_handlers_block_matched(tabBtn, G_SIGNAL_MATCH_DATA, ...);
// Then disconnect LATER after all events processed
```

### Fix 3: Validate Widgets Before Access

```cpp
// Before EVERY g_object_get_data call:
if (!tabBtn || !GTK_IS_WIDGET(tabBtn)) return;

// Before accessing cached pointers:
GtkWidget* closeBtn = GTK_WIDGET(g_object_get_data(G_OBJECT(tabBtn), "close-button"));
if (!closeBtn || !GTK_IS_WIDGET(closeBtn)) return;  // ✅ VALIDATE
```

### Fix 4: Cancel ALL Pending Operations

```cpp
// BrayaWindow.cpp:1806 - Before cleanup, cancel EVERYTHING:

// 1. Cancel preview timer
guint* timer = (guint*)g_object_get_data(G_OBJECT(tabBtn), "preview-timer");
if (timer && *timer > 0) {
    g_source_remove(*timer);
    delete timer;
}

// 2. Cancel any pending snapshot operations
// Add snapshot tracking:
std::map<GtkWidget*, bool> pendingSnapshots;
// Cancel in closeTab()

// 3. Force popover cleanup BEFORE signal disconnection
GtkWidget* popover = GTK_WIDGET(g_object_get_data(G_OBJECT(tabBtn), "preview-popover"));
if (popover && GTK_IS_WIDGET(popover)) {
    gtk_popover_popdown(GTK_POPOVER(popover));  // ✅ Hide first
    g_usleep(1000);  // Let GTK process
    gtk_widget_unparent(popover);
}
```

### Fix 5: Favicon Cache RAII (Memory Safety)

```cpp
// BrayaWindow.h - Replace raw pointers:
template<typename T>
class GObjectPtr {
private:
    T* ptr = nullptr;
public:
    GObjectPtr() : ptr(nullptr) {}
    GObjectPtr(T* p) : ptr(p) { if (ptr) g_object_ref(ptr); }
    ~GObjectPtr() { if (ptr) g_object_unref(ptr); }

    GObjectPtr(const GObjectPtr& other) : ptr(other.ptr) {
        if (ptr) g_object_ref(ptr);
    }

    GObjectPtr& operator=(T* p) {
        if (ptr) g_object_unref(ptr);
        ptr = p;
        if (ptr) g_object_ref(ptr);
        return *this;
    }

    T* get() const { return ptr; }
    operator T*() const { return ptr; }
    operator bool() const { return ptr != nullptr; }
};

// Replace:
std::map<std::string, GdkTexture*> faviconCache;

// With:
std::map<std::string, GObjectPtr<GdkTexture>> faviconCache;

// Now ref counting is automatic!
```

### Fix 6: WebView Crash Handler

```cpp
// BrayaTab.cpp:1380 - Implement actual recovery:
void BrayaTab::onWebProcessCrashed(WebKitWebView* webView,
                                    WebKitWebProcessTerminationReason reason,
                                    gpointer userData) {
    BrayaTab* tab = static_cast<BrayaTab*>(userData);
    if (!tab) return;

    std::cerr << "💥 WebView process crashed! Reason: " << reason << std::endl;

    // Show error page
    const char* errorHtml =
        "<!DOCTYPE html>"
        "<html><body style='font-family:sans-serif;text-align:center;padding:50px;'>"
        "<h1>😔 Page Crashed</h1>"
        "<p>The web page crashed unexpectedly.</p>"
        "<button onclick='location.reload()'>Reload Page</button>"
        "</body></html>";

    webkit_web_view_load_html(webView, errorHtml, tab->url.c_str());

    // Mark tab as crashed
    tab->title = "💥 Crashed: " + tab->title;

    // Update UI
    if (tab->titleChangedCallback) {
        tab->titleChangedCallback(tab->title);
    }
}
```

## Implementation Priority

### Phase 1: Quick Mitigation (1-2 hours)
1. ✅ Add destroying flag to all hover/preview callbacks
2. ✅ Add widget validation before ALL `g_object_get_data` calls
3. ✅ Implement WebView crash handler

### Phase 2: Structural Fixes (4-6 hours)
4. ✅ Replace favicon cache with RAII wrappers
5. ✅ Add signal blocking instead of immediate disconnection
6. ✅ Force synchronous cleanup of popovers/timers

### Phase 3: Testing (2-3 hours)
7. ✅ Stress test with rapid tab opening/closing
8. ✅ Test with preview popover triggering during closure
9. ✅ Valgrind memory leak detection
10. ✅ Run under GDB to confirm no crashes

## Testing Strategy

### Stress Test Script:
```bash
# Open 50 tabs rapidly
for i in {1..50}; do
    # Simulate Ctrl+T
    sleep 0.1
done

# Close all tabs rapidly
for i in {1..49}; do
    # Simulate Ctrl+W
    sleep 0.05
done

# Expected: No crashes, no memory leaks
```

### Valgrind Check:
```bash
G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    ./braya-browser
```

## Success Criteria

- ✅ Zero crashes during 100 tab open/close cycles
- ✅ Zero memory leaks in 10-minute session
- ✅ Favicon cache properly cleaned up
- ✅ WebView crashes show error page instead of SIGSEGV

## Timeline

- **Fix Development**: 6-8 hours
- **Testing**: 3-4 hours
- **Total**: ~1-2 days

---

**Status**: Ready to implement
**Priority**: CRITICAL - blocks all other development
**Impact**: Eliminates ~16 crashes/week
