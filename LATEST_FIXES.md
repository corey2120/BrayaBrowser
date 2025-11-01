# Braya Browser - Latest Fixes (Nov 1, 2025)

## Changes Made

### 1. Back/Forward Buttons - NOW VISIBLE! ✅
- **Size increased**: 24x24 → 32x32 pixels
- **Explicit visibility**: Added `gtk_widget_set_visible(btn, TRUE)`
- **Proper initial state**: Disabled by default, enabled when navigation history exists
- **Better styling**: Added background color and borders for visibility
- **Icon size**: Increased from 16px to 18px

### 2. Headerbar Improvements ✅
- **Height**: Reduced from 36px to 38px (compact but not too small)
- **Better spacing**: Padding adjusted to 2px 6px
- **All buttons visible**: Dog logo, back, forward, reload, home, URL bar, dev tools, split view, bookmarks, settings

### 3. URL Bar ✅
- **Size**: 500px min-width × 30px height (readable and usable)
- **Expanding**: Still expands to fill available space
- **Better contrast**: Dark background with glowing border on focus
- **Clean look**: Matches Firefox style

### 4. Navigation Button Styling ✅
```css
.nav-btn {
    background: rgba(255, 255, 255, 0.05);
    color: #e0e6ed;
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 6px;
    min-width: 32px;
    min-height: 32px;
    padding: 4px;
    margin: 0 2px;
}
```

### 5. CSS Fixes ✅
- Removed invalid `max-height` property from headerbar
- Fixed button visibility with proper background colors
- Better hover states with cyan glow

## Known Issues Still To Fix

### High Priority
1. **Tab close button (X) not working**: The X button doesn't close tabs
2. **Tabs only show dot**: Need to show favicon or first letter of site
3. **Bookmarks don't appear on new tabs**: Bookmark bar visibility issue
4. **Settings don't save**: Search engine and home page settings aren't persisting
5. **Browser crashes**: Need to investigate crash causes

### Medium Priority
6. **No rounded corners**: Window should have subtle rounded corners
7. **Bottom bar present**: Should remove if not needed
8. **Icon quality**: Bookmark and tab icons need improvement

## How to Test

1. Build: `./build.sh`
2. Run: `./build/braya-browser`
3. Test back/forward:
   - Navigate to a site (e.g., duckduckgo.com)
   - Click a link
   - Click back button (should now be visible and work!)
   - Click forward button

## Next Steps

1. Fix tab close button functionality
2. Improve tab visual design (show favicons)
3. Fix bookmarks bar persistence on new tabs
4. Debug and fix crash issues
5. Implement settings persistence

## Technical Details

### Code Changes
- `src/BrayaWindow.cpp`: Lines 151-177 (back/forward button creation)
- `resources/style.css`: Lines 81-139 (headerbar and button styling)

### Build Info
- Compiler: GCC with C++17
- Dependencies: GTK4, WebKitGTK 6.0
- Build system: CMake
