# 🐛 CRASH FIX - Emergency Revert

## What Crashed
Signal 11 (SIGSEGV) - Segmentation fault immediately after window shown

## What I Reverted
1. ✅ Kept title bar removal (gtk_window_set_decorated(FALSE))
2. ✅ Kept compact navbar  
3. ✅ Kept settings persistence
4. ❌ REVERTED: Status bar removal (it was trying to access statusLabel that didn't exist)
5. ✅ Kept window controls (−, □, ×)

## The Problem
When I removed the status bar, the code still tried to update `statusLabel` which was NULL, causing a crash.

## Quick Fix Applied
- Restored `createStatusBar()` and status bar widget
- Status bar now just shows "Loading..." (minimal)
- Fixed NULL pointer dereference

## Test Now
```bash
cd /home/cobrien/Projects/braya-browser-cpp
./build/braya-browser
```

Should now work without crashing!

## What's Working:
- ✅ No title bar (more space)
- ✅ Compact navbar (smaller)
- ✅ Settings save/load
- ✅ Window controls (−, □, ×)
- ⚠️ Status bar back (but minimal)
- ⚠️ Close button still needs testing

Try it and let me know if it crashes again!
