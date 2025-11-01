# 🐕 Braya Browser - Testing Guide

## Quick Test Checklist

### Basic Functionality
- [ ] Launch browser - does it start without crashing?
- [ ] Navigate to a website (e.g., duckduckgo.com)
- [ ] Create multiple tabs (Ctrl+T or + button)
- [ ] Switch between tabs - do they maintain state?
- [ ] Close tabs (Ctrl+W) - does it handle correctly?
- [ ] Use back/forward buttons - do they work?
- [ ] Type in URL bar and press Enter
- [ ] Search from URL bar (type query without domain)
- [ ] Click bookmark from bookmarks bar

### Tab Icon Testing
- [ ] Open github.com - does favicon appear?
- [ ] Open youtube.com - does favicon appear?
- [ ] Open news.ycombinator.com - does favicon appear?
- [ ] Create new tab - shows bullet (•)
- [ ] Tab tooltip shows correct title and URL

### Settings Testing
- [ ] Click settings button (⚙ at bottom of sidebar)
- [ ] Settings dialog opens without crash
- [ ] Navigate through all tabs (General, Appearance, Security, Privacy, Advanced)
- [ ] Change theme - does dropdown work?
- [ ] Adjust font size - does spinner work?
- [ ] Toggle switches (JavaScript, WebGL, etc.)
- [ ] Change home page URL
- [ ] Change search engine
- [ ] Click "Apply & Save" - does it work?
- [ ] Close settings dialog

### Stability Testing
- [ ] Keep browser open for 5+ minutes
- [ ] Open 5+ tabs
- [ ] Navigate to heavy websites (YouTube, Twitter/X)
- [ ] Reload pages multiple times
- [ ] Rapidly switch between tabs
- [ ] Close and reopen tabs
- [ ] Enter invalid URLs
- [ ] Test with slow/failing network

### UI/UX Testing
- [ ] Sidebar width is compact (56px)
- [ ] Tabs are properly sized (48x48)
- [ ] Dark theme is consistent
- [ ] Hover effects work on buttons
- [ ] Active tab is highlighted
- [ ] Status bar shows loading states
- [ ] Bookmarks bar hides after first navigation
- [ ] Bookmarks bar shows on new tab

### Keyboard Shortcuts
- [ ] Ctrl+T - Creates new tab
- [ ] Ctrl+W - Closes current tab
- [ ] Ctrl+L - Focuses URL bar
- [ ] Enter in URL bar - Navigates

### Known Issues to Watch For

#### Crashes
- **Random crashes**: Document when it happens (what action, which website, how long running)
- **URL entry crash**: Does it crash when typing specific URLs?
- **Settings crash**: Does opening/closing settings cause issues?
- **Tab switching crash**: Does rapidly switching tabs cause crashes?

#### Tab Icons
- **Missing favicons**: Which websites don't show icons?
- **Generic icons**: Does it show letters instead of favicons?
- **Large tabs**: Are tabs too big or too small?

#### Visual Issues
- **Rounded corners**: Do windows have rounded corners?
- **Scrollbar jumps**: Does tab sidebar scrollbar behave oddly?
- **Layout problems**: Any spacing/alignment issues?

## Testing Websites

Good test sites for features:

### Heavy Content
- **YouTube**: Video playback, complex UI
- **Twitter/X**: Lots of JavaScript, infinite scroll
- **GitHub**: Clean design, should show favicon well
- **Reddit**: Complex layout, lots of images

### Standards Compliance
- **HTML5 Test**: https://html5test.com/
- **CSS Test**: https://www.css3test.com/
- **WebGL Test**: https://get.webgl.org/

### Speed/Performance
- **Google PageSpeed**: https://pagespeed.web.dev/
- **WebPageTest**: https://www.webpagetest.org/

### Security
- **Qualys SSL Test**: https://www.ssllabs.com/ssltest/
- **Security Headers**: https://securityheaders.com/

## Reporting Issues

When you find an issue, please note:
1. **What you were doing** - Specific actions taken
2. **What happened** - Actual behavior
3. **What you expected** - Expected behavior
4. **Browser log** - Check `braya-test-run2.log`
5. **Reproducibility** - Can you make it happen again?
6. **System info** - CPU usage, memory usage

## Log Files

Check these for debugging:
- `braya-test-run2.log` - Most recent run
- `braya-debug.log` - Debug output
- `braya-cpp-run.log` - Previous run

View logs:
```bash
cd /home/cobrien/Projects/braya-browser-cpp
tail -f braya-test-run2.log  # Watch live
```

## Performance Monitoring

Check resource usage:
```bash
# While browser is running
ps aux | grep braya-browser
top -p $(pgrep braya-browser)
```

## Clean Rebuild

If something seems really broken:
```bash
cd /home/cobrien/Projects/braya-browser-cpp
rm -rf build
./build.sh
./build/braya-browser
```

## What to Test Next

### Priority 1: Stability
- Can you keep it running for 30 minutes?
- Can you open 10+ tabs without crash?
- Can you reload pages 20+ times?

### Priority 2: Features
- Do tab icons show reliably?
- Do settings actually work?
- Does bookmarks bar behave correctly?

### Priority 3: Polish
- Is the UI smooth?
- Are animations nice?
- Do colors look good?

---

**Remember**: This is alpha software! Crashes are expected. The goal is to identify patterns and fix root causes.
