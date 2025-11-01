# 🐕 BRAYA BROWSER - NEW DESIGN (Zen/Firefox Style)

## Target Design:

### 1. **Vertical Sidebar - Firefox/Zen Style**
- Collapsed: ~40px wide (just icons)
- Show favicon or tab number
- Compact tab items
- Clean, minimal

### 2. **Bookmarks Bar - Chrome Style**
- Show on new tab (home page)
- Auto-hide when navigating away
- Show again on new tabs
- Just like Chrome's behavior

### 3. **Essential Area - Zen Style**
- Pinned tabs / frequently used
- Quick access section
- Always accessible

### 4. **Layout:**
```
┌─────┬──────────────────────────────────┐
│ 🐕  │  [Nav] [URL Bar........] [⚙]    │ <- Compact navbar
├─────┼──────────────────────────────────┤
│  1  │  📑 Bookmarks (show on new tab)  │ <- Auto-hiding bookmarks
│  2  ├──────────────────────────────────┤
│  +  │                                  │
│     │     WEB CONTENT (max space)      │
│  ⚙  │                                  │
└─────┴──────────────────────────────────┘
```

## Implementation:

1. Sidebar: 40px collapsed, show tab numbers
2. Tabs: Minimal, just indicators
3. Bookmarks: Chrome-style auto-hide
4. Remove hover expansion
5. Everything compact and clean

Let's rebuild this properly!
