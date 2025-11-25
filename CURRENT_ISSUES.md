# Braya Browser - Current Issues & Solutions

**Date:** 2025-11-23
**Status:** 4 Issues Identified

---

## 🐛 **Issue Summary**

| # | Issue | Severity | Estimated Fix Time |
|---|-------|----------|-------------------|
| 1 | Autofill prompt not appearing | Medium | 1-2 hours |
| 2 | Video playback problems | High | 2-3 hours |
| 3 | Random crashes | High | 3-4 hours |
| 4 | Scrolling lag on heavy sites | Medium | 1-2 hours |

---

## Issue 1: Autofill Prompt Not Appearing

### **Current Behavior:**
- Autofill detection works (logs show "🔑 Autofill requested")
- Passwords are detected and matched correctly
- But the popover UI doesn't appear on screen

### **Root Cause:**
The autofill popover is being created with the WebView as parent, but WebView might not support child popovers properly in GTK4/WebKit2GTK.

### **Solution:**
```cpp
// In showAutofillSuggestions(), change line 1042:
// OLD:
gtk_widget_set_parent(popover, GTK_WIDGET(webView));

// NEW: Use the tab button or overlay as parent
GtkWidget* overlay = gtk_widget_get_ancestor(GTK_WIDGET(webView), GTK_TYPE_OVERLAY);
GtkWidget* parent = overlay ? overlay : tabButton;
gtk_widget_set_parent(popover, parent);
```

**Alternative Fix:** Create a notification bar at the top of the page instead of a popover.

---

## Issue 2: Video Playback Problems

### **Current Behavior:**
- Videos don't play or appear jumbled
- Audio might work but video is corrupted

### **Root Cause:**
WebKit hardware acceleration and codec support not properly enabled.

### **Solution:**

**Step 1: Enable Hardware Acceleration**
```cpp
// In BrayaTab.cpp, after creating WebView settings:
webkit_settings_set_hardware_acceleration_policy(
    settings,
    WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS
);

webkit_settings_set_enable_webgl(settings, TRUE);
webkit_settings_set_enable_media_stream(settings, TRUE);
```

**Step 2: Ensure Codecs**
```bash
# Install required codecs (Fedora):
sudo dnf install gstreamer1-plugins-{base,good,bad-free,ugly-free}
sudo dnf install gstreamer1-libav

# Install non-free codecs (for H.264/MP4):
sudo dnf install gstreamer1-plugins-{bad-nonfree,ugly}
```

**Step 3: Enable Media Capabilities**
```cpp
webkit_settings_set_enable_media_capabilities(settings, TRUE);
webkit_settings_set_media_playback_allows_inline(settings, TRUE);
```

---

## Issue 3: Random Crashes

### **Current Behavior:**
- Browser crashes after long use
- Crashes when attempting to play videos
- Signal 11 (SIGSEGV) crashes

### **Root Causes:**
1. Video-related: Missing codec causes WebKit crash
2. Memory leaks in suspended tabs
3. WebView destruction race conditions

### **Solutions:**

**Already Fixed:**
- ✅ destroyingTabs set prevents callback races
- ✅ Widget validation in callbacks
- ✅ Signal blocking before disconnection
- ✅ GObjectPtr RAII for automatic cleanup

**Additional Fixes Needed:**

**1. Suspended Tab Cleanup:**
```cpp
// Add to suspendTab():
void BrayaTab::suspendTab() {
    if (!webView || suspended) return;

    // Cancel any pending loads
    webkit_web_view_stop_loading(webView);

    // Clear cache for this tab
    webkit_website_data_manager_clear(...);

    // existing code...
}
```

**2. Video Crash Prevention:**
```cpp
// Add signal handler for web process crashes:
g_signal_connect(webView, "web-process-terminated",
    G_CALLBACK(+[](WebKitWebView* view,
                   WebKitWebProcessTerminationReason reason,
                   gpointer userData) {
        auto* tab = static_cast<BrayaTab*>(userData);
        if (reason == WEBKIT_WEB_PROCESS_CRASHED) {
            // Reload the tab instead of crashing browser
            webkit_web_view_reload(view);
        }
    }), this);
```

**3. Memory Limits:**
```cpp
// Set memory limits on WebContext:
webkit_web_context_set_cache_model(
    context,
    WEBKIT_CACHE_MODEL_WEB_BROWSER
);

webkit_web_context_set_process_model(
    context,
    WEBKIT_PROCESS_MODEL_MULTIPLE_SECONDARY_PROCESSES
);
```

---

## Issue 4: Scrolling Lag on Heavy Websites

### **Current Behavior:**
- Twitter/X and other heavy sites lag when scrolling
- Frame drops and stuttering

### **Root Causes:**
1. No GPU acceleration for scrolling
2. JavaScript performance not optimized
3. Compositing layers not enabled

### **Solutions:**

**1. Enable Smooth Scrolling:**
```cpp
webkit_settings_set_enable_smooth_scrolling(settings, TRUE);
```

**2. Enable Accelerated Compositing:**
```cpp
webkit_settings_set_enable_accelerated_2d_canvas(settings, TRUE);
webkit_settings_set_enable_webgl(settings, TRUE);
```

**3. Optimize JavaScript:**
```cpp
webkit_settings_set_enable_page_cache(settings, TRUE);
webkit_settings_set_enable_javascript_markup(settings, TRUE);
```

**4. Increase Process Limits:**
```cpp
// Allow more web processes for better performance
g_object_set(G_OBJECT(context),
    "process-swap-on-cross-site-navigation-enabled", TRUE,
    NULL);
```

**5. Disable Heavy Ad-Blocker on Performance Mode:**
```cpp
// Add performance mode that disables ad-blocking on heavy sites
if (performanceMode) {
    webkit_user_content_filter_store_remove(...);
}
```

---

## 📝 **Recommended Implementation Order**

### **Quick Wins (< 2 hours):**
1. ✅ **Enable hardware acceleration** - Will fix videos AND scrolling
2. ✅ **Add web process crash handler** - Prevents video crashes
3. ✅ **Fix autofill popover parent** - Makes autofill work

### **Medium Priority (2-4 hours):**
4. ⏳ **Install codecs** - User needs to do this via dnf
5. ⏳ **Add smooth scrolling settings** - Improves UX
6. ⏳ **Memory limits** - Reduces long-session crashes

### **Long Term (4+ hours):**
7. ⏳ **Suspended tab cleanup** - Better memory management
8. ⏳ **Performance profiling** - Identify specific bottlenecks

---

## 🔧 **Quick Fix Script**

Create a file called `apply-fixes.sh`:

```bash
#!/bin/bash
# Quick fixes for Braya Browser

echo "Installing video codecs..."
sudo dnf install -y gstreamer1-plugins-{base,good,bad-free,bad-nonfree,ugly-free,ugly}
sudo dnf install -y gstreamer1-libav

echo "Codecs installed! Now rebuild browser with hardware acceleration enabled."
```

---

## 🎯 **Next Steps**

**Option A: Fix Everything Now (6-8 hours)**
- Apply all fixes in one session
- Thoroughly test each fix
- Ship stable v1.0.9

**Option B: Priority Fixes Only (2-3 hours)**
- Hardware acceleration (fixes videos + scrolling)
- Crash handlers (prevents crashes)
- Quick autofill fix
- Ship v1.0.9, defer other fixes to v1.1.0

**Option C: User Testing First**
- Test current build for 1-2 days
- Document specific crash scenarios
- Apply targeted fixes based on real usage

---

## 💡 **My Recommendation**

**Do Option B** - Priority fixes for immediate impact:

1. **Enable hardware acceleration** (30 min)
   - Fixes both video playback AND scrolling lag
   - Biggest bang for buck

2. **Add crash recovery handlers** (1 hour)
   - Catches web process crashes
   - Prevents browser from going down with WebKit

3. **Fix autofill popover** (30 min)
   - Change parent widget
   - Test on vwhub.com

4. **Quick test** (30 min)
   - Test videos on YouTube
   - Test scrolling on Twitter
   - Test autofill on vwhub.com

**Total Time: 2.5-3 hours**
**Impact: Fixes all 4 reported issues**

Then you can use it for real work and identify any remaining edge cases!

---

**Ready to start? Which approach do you prefer?** 🚀
