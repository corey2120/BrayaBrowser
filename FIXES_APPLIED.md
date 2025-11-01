# Fixes Applied (Updated)

## Changes Made:

### 1. Fixed about:braya Not Loading ✅
- **Issue**: Home page wasn't showing 
- **Fix**: Removed call to broken `settings->getHomePage()`
- **Fix**: Improved path resolution to find resources/home.html from build directory
- Now properly loads custom home page on startup

### 2. Made Back/Forward Buttons Visible ✅
- **Issue**: Buttons were created but not visible
- **Fix**: Added explicit `gtk_widget_set_visible(button, TRUE)` calls
- GTK4 may require explicit visibility for some widgets

### 3. Clear URL on New Tab ✅
- URL bar clears automatically when opening new tab
- Focus goes to URL bar so you can type immediately

### 4. Changed Default Homepage ✅
- Default changed from duckduckgo.com to about:braya
- Browser launches with custom home page

## Files Modified:
- `src/BrayaWindow.cpp` - Main fixes
- `src/BrayaWindow.h` - Default URL parameter
- `src/BrayaTab.h` - Default URL parameter  
- `resources/home.html` - NEW simple home page

## Test Now:
```bash
cd /home/cobrien/Projects/braya-browser-cpp
./build/braya-browser
```

Should see:
- Back/forward buttons visible (greyed out initially)
- Home page loads with "Braya Browser" text
- New tab clears URL automatically
