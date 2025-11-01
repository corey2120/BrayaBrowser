# Feature #4: Bookmarks Manager

## Goal
Full bookmark management system with folders and organization

## Implementation Plan

1. **BrayaBookmarks class**
   - Store bookmarks in JSON file
   - Support folders/categories
   - Add, edit, delete bookmarks
   - Search bookmarks

2. **UI Components**
   - Bookmarks manager dialog (Ctrl+B or Ctrl+Shift+O)
   - Tree view with folders
   - Add/Edit/Delete buttons
   - Search bar
   - Import/Export options

3. **Features**
   - Create folders
   - Drag & drop to organize (future)
   - Right-click context menu
   - Double-click to open bookmark
   - "Add bookmark" button in UI

4. **Storage**
   - JSON file: ~/.config/braya/bookmarks.json
   - Format: { name, url, folder, timestamp }

Let's build it!
