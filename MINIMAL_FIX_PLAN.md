# Minimal Fix Plan - Only What Was Asked

## User's Issues:
1. "we don't have a back or forward button" - They exist but maybe not visible?
2. "home and reload are too far away from the URL box" - Need to move closer
3. "when i open a new tab i have to manually delete the url" - Clear URL on new tab
4. "launches duck duck go" - Change default homepage  
5. "I would like it to launch just a home screen about the app" - Create about page

## Minimal Changes Only:

### 1. Back/Forward Buttons - Just ensure they're visible
- Check if icon names are correct
- Maybe they need to be shown explicitly with gtk_widget_show()?
- Don't change sizes, colors, or styling

### 2. Move Home/Reload Closer to URL
- Reduce spacing between buttons and URL entry
- Don't change button appearance

### 3. Clear URL on New Tab
- In createTab(), clear the URL entry
- Simple one-line fix

### 4 & 5. Custom Home Page
- Create simple about:braya handler
- Create minimal HTML home page
- Change default from duckduckgo.com to about:braya

