# 🐕 Braya Browser - Feature Roadmap

**Starting:** History Tracking (Feature #1)

## Feature Implementation Order

### ✅ Done
- Tab management
- Navigation controls
- Custom home page
- Bookmarks bar

### 🚧 Current: History Tracking
Track and display browsing history

**Implementation Plan:**
1. Create BrayaHistory class to store history
2. Track page visits (URL, title, timestamp)
3. Store in SQLite database or JSON file
4. Create history viewer UI
5. Add keyboard shortcut (Ctrl+H)
6. Add clear history option

**Files to create/modify:**
- `src/BrayaHistory.h` (new)
- `src/BrayaHistory.cpp` (new)
- `src/BrayaWindow.cpp` (connect to history)
- `CMakeLists.txt` (add new files)

### 📋 Up Next
2. Downloads management
3. Find in page (Ctrl+F)
4. Bookmarks manager
5. Tab groups/organization
6. Customizable home page
7. Themes/appearance options

---

Let's start with #1: History Tracking!
