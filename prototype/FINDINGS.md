# CEF Prototype Findings

**Date:** November 25, 2025
**Status:** Initial prototype attempt

---

## What We Discovered

### 1. CEF is Available ✅
- Package: `cef` and `cef-devel` in Fedora 43
- Version: CEF 141.0.11 (based on Chromium 141.0.7390.122)
- Size: 232MB (libcef.so)
- Location: `/usr/lib64/cef/`
- Headers: `/usr/include/cef/include/`

### 2. Installation Works ✅
```bash
sudo dnf install cef cef-devel
```

No compilation from source needed!

---

## Build Issues Encountered

### Issue 1: Header Include Path
**Problem:** CEF headers use relative includes like `#include "include/cef_base.h"`

**Solution:** Include directory must be `/usr/include/cef` not `/usr/include/cef/include`

### Issue 2: GLib Macro Conflicts ⚠️
**Problem:** CEF's `cef_base.h` defines macros that conflict with GLib's `G_DEFINE_AUTOPTR_CLEANUP_FUNC`

**Error:**
```
error: a function-definition is not allowed here before '{' token
in expansion of macro 'G_DEFINE_AUTOPTR_CLEANUP_FUNC'
```

**Root cause:** CEF was designed for Qt/Windows, not GTK. The macro systems clash.

---

## The Hard Reality

### CEF + GTK4 is Problematic

**Why it's difficult:**
1. **Macro conflicts** - CEF and GLib define conflicting macros
2. **Different paradigms** - CEF expects Qt-style or Windows-style integration
3. **Limited GTK examples** - Most CEF examples are Qt or wxWidgets
4. **X11/Wayland complexity** - Need platform-specific window handling

**Existing GTK + CEF projects:**
- [ChromiumGtk](https://github.com/GSharpKit/ChromiumGtk) - Uses C#/GtkSharp (not C++)
- [cefcapi GTK example](https://github.com/cztomczak/cefcapi) - Uses GTK2/GTK3, not GTK4
- No known GTK4 + CEF C++ examples

---

## What This Means

### Option A: Fight Through CEF + GTK4 Issues
**Effort:** 2-4 weeks just to get basic window working
**Risk:** High - may hit more incompatibilities
**Outcome:** Uncertain - might not be solvable

### Option B: Use Qt Instead of GTK
**Effort:** 6-8 months (rewrite entire UI)
**Risk:** Medium
**Outcome:** Would work (CEF + Qt is proven)
**But:** Loses beautiful GTK4 design

### Option C: Stay with WebKit
**Effort:** 0 (already working)
**Risk:** None
**Outcome:** Known limitations, but stable

---

## Recommendation

**Do NOT attempt CEF with GTK4 C++**

**Why:**
1. Macro conflicts are non-trivial to resolve
2. No existing examples to learn from
3. Would require wrapper layer to isolate CEF from GTK
4. Time investment too high with uncertain outcome

**Better options:**
1. **Stay with WebKit** - Known, working, stable
2. **Use Electron** - If you must have Chromium (but it's JavaScript)
3. **Wait for better embedding** - Maybe GeckoView desktop someday

---

## What We Learned

### WebKit is the Right Choice for GTK4 ✅

**Why:**
- Designed for GTK integration
- No macro conflicts
- Excellent documentation
- Active GTK community support
- Native Linux focus

**CEF/Chromium is for:**
- Qt applications
- Windows applications
- Electron (JavaScript)
- Not GTK C++ applications

---

## Final Assessment

**Question:** Can we migrate Braya Browser to CEF?

**Answer:** Technically possible but impractical

**Reality:**
- Would need to solve macro conflicts
- Create abstraction layer
- Pioneer GTK4 + CEF integration
- 4-6 months of experimental work
- High risk of failure

**Better path:**
- Perfect WebKit version
- Focus on features that work
- Target Linux enthusiasts who value native GTK
- Accept we won't match Chrome performance

---

## Next Steps

### Option 1: Polish WebKit v1.1 (RECOMMENDED)
**Timeline:** 2-3 weeks
**Tasks:**
- Fix favicons properly
- Implement tab folder save/restore
- Optimize what we can
- Ship stable v1.1

### Option 2: Attempt CEF Anyway (NOT RECOMMENDED)
**Timeline:** 4-6 weeks minimum
**Tasks:**
- Solve macro conflicts
- Create CEF wrapper layer
- Test extensively
- Might fail

### Option 3: Research Alternatives
**Timeline:** 1-2 weeks
**Tasks:**
- Look into Electron (but it's JavaScript)
- Check if Qt is worth learning
- See if GeckoView desktop is closer

---

## Conclusion

**CEF exists and is packaged, but GTK4 integration is not viable for a solo developer.**

**WebKit remains the best engine for:**
- GTK4 applications
- Native Linux browsers
- Solo/small team development
- Privacy-focused users

**Recommendation:** Focus on making the best WebKit-based browser possible rather than fighting CEF integration.

---

**Files in this prototype:**
- `cef-hello.cpp` - Attempted CEF + GTK4 integration
- `CMakeLists.txt` - Build configuration
- `FINDINGS.md` - This document

**Status:** Prototype incomplete due to macro conflicts
**Decision:** Do not proceed with CEF migration
